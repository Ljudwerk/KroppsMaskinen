// Motorn – kopplar ihop Taktverket, kanalerna och mixern.
// Allt räknas sample för sample, så att stegen, ratchets och gatelängderna hamnar exakt.
//
// MIDI: kanal 10 spelar Stampverket (GM-trumkartan). Övriga kanaler spelar Kolvbasen när sekvensern står
// still; när den går flyttar en tangent hela basgången (transponering från grundtonen), som på en gammal sekvenser.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Kolvbasen.h"
#include "Stampverket.h"
#include "../takt/Monster.h"
#include "../takt/Taktverket.h"

namespace km
{

struct MotorParams
{
    KolvbasParams kolvbasen;
    StampverketParams stampverket;
    TaktInstallningar takt;
    int trumLangd = 16;        // 1..64 steg
    int basLangd = 16;         // 1..64 steg
    int grundton = 33;         // MIDI-ton för basgångens 0 (A1)
    float basNiva = 0.8f, trumNiva = 0.8f, master = 0.65f;
};

class Motor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        kolvbasen.prepare (sampleRate);
        stampverket.prepare (sampleRate);
        taktverket.prepare (sampleRate);
        dcL.prepare (sampleRate);
        limiterSlapp = (float) std::exp (-1.0 / (0.08 * sampleRate));
        reset();
    }

    void reset()
    {
        kolvbasen.reset();
        taktverket.reset();
        dcL.reset();
        limiterEnv = 0.0f;
        transponering = 0;
        basHaller = false;
        antalVantande = 0;
        aktuelltTrumSteg.store (-1);
        aktuelltBasSteg.store (-1);
    }

    void setParams (const MotorParams& p)
    {
        params = p;
        kolvbasen.setParams (p.kolvbasen);
        stampverket.setParams (p.stampverket);
    }

    void process (float* left, float* right, int numSamples, const juce::MidiBuffer& midi,
                  const Transport& transport, bool egenKlocka, const TrumMonster& trummor, const TonMonster& bas)
    {
        const int antalSteg = taktverket.process (params.takt, transport, egenKlocka, numSamples);
        const bool gar = taktverket.gar();
        if (taktverket.hoppade())
        {
            kolvbasen.allNotesOff();
            basHaller = false;
            antalVantande = 0;
        }
        if (! gar)
        {
            aktuelltTrumSteg.store (-1, std::memory_order_relaxed);
            aktuelltBasSteg.store (-1, std::memory_order_relaxed);
        }

        auto midiIt = midi.cbegin();
        int stegIndex = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            for (; midiIt != midi.cend() && (*midiIt).samplePosition <= i; ++midiIt)
                hanteraMidi ((*midiIt).getMessage(), gar);

            while (stegIndex < antalSteg && taktverket.handelse (stegIndex).offset <= i)
                spelaSteg (taktverket.handelse (stegIndex++).k, trummor, bas);

            tickVantande();

            const float b = kolvbasen.tick() * params.basNiva;
            const float t = stampverket.tick() * params.trumNiva;
            float x = dcL.tick ((b + t) * params.master);

            // Toppbegränsare: snabb attack, 80 ms släpp, tak strax under 0 dBFS.
            const float a = std::abs (x);
            limiterEnv = a > limiterEnv ? a : limiterEnv * limiterSlapp + a * (1.0f - limiterSlapp);
            if (limiterEnv > tak) x *= tak / limiterEnv;
            x = juce::jlimit (-1.0f, 1.0f, x);

            left[i] = x;
            if (right != nullptr) right[i] = x;
        }
    }

    // För gränssnittet: steget som spelas just nu (−1 = stoppad) och transponeringen.
    std::atomic<int> aktuelltTrumSteg { -1 }, aktuelltBasSteg { -1 };
    int getTransponering() const { return transponering; }

private:
    void hanteraMidi (const juce::MidiMessage& m, bool gar)
    {
        if (m.getChannel() == 10)
        {
            if (m.isNoteOn())
                if (const int s = Stampverket::sparForNot (m.getNoteNumber()); s >= 0)
                    stampverket.slag ((Trumma) s, m.getFloatVelocity());
            return;
        }

        if (m.isAllNotesOff() || m.isAllSoundOff()) { kolvbasen.allNotesOff(); return; }

        if (gar)
        {
            if (m.isNoteOn()) transponering = juce::jlimit (-24, 24, m.getNoteNumber() - params.grundton);
            return;
        }
        if (m.isNoteOn()) kolvbasen.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff()) kolvbasen.noteOff (m.getNoteNumber());
    }

    void spelaSteg (juce::int64 k, const TrumMonster& trummor, const TonMonster& bas)
    {
        const double perSteg = taktverket.samplesPerSteg();

        // --- Trummor
        const int ts = (int) (k % juce::jlimit (1, maxSteg, params.trumLangd));
        aktuelltTrumSteg.store (ts, std::memory_order_relaxed);
        for (int s = 0; s < TrumMonster::spar; ++s)
        {
            const auto st = trummor.get (s, ts);
            if (st.slag == 0) continue;
            if (st.chans > 0 && Taktverket::chansSlag (k, s) >= chansVarden[(size_t) st.chans]) continue;
            const float vel = st.slag == 2 ? 1.0f : 0.72f;
            stampverket.slag ((Trumma) s, vel);
            const int slag = st.ratchet + 1;
            for (int h = 1; h < slag; ++h)
                vanta (s, (int) (perSteg * h / slag), vel * 0.85f);
        }

        // --- Kolvbasen
        const int bs = (int) (k % juce::jlimit (1, maxSteg, params.basLangd));
        aktuelltBasSteg.store (bs, std::memory_order_relaxed);
        const auto st = bas.get (bs);
        if (st.slag == 0) return;   // paus: en glidande ton hålls kvar tills nästa ton

        const int ton = juce::jlimit (0, 127, params.grundton + transponering + st.tonhojd());
        const bool legato = basHaller;
        const int gate = st.glid ? -1 : juce::jmax (1, (int) (perSteg * tonLangder[(size_t) st.langd]));
        kolvbasen.steg (ton, st.slag == 2, legato, gate);
        basHaller = st.glid;
    }

    // Ratchets: slag som ska komma senare inom steget.
    struct Vantande { int spar, om; float vel; };
    void vanta (int spar, int om, float vel)
    {
        if (antalVantande < (int) vantande.size()) vantande[(size_t) antalVantande++] = { spar, om, vel };
    }
    void tickVantande()
    {
        for (int i = 0; i < antalVantande;)
        {
            if (--vantande[(size_t) i].om <= 0)
            {
                stampverket.slag ((Trumma) vantande[(size_t) i].spar, vantande[(size_t) i].vel);
                vantande[(size_t) i] = vantande[(size_t) --antalVantande];
            }
            else ++i;
        }
    }

    static constexpr float tak = 0.966f; // −0,3 dBFS

    double sr = 48000.0;
    MotorParams params;
    Kolvbasen kolvbasen;
    Stampverket stampverket;
    Taktverket taktverket;
    DcSparr dcL;
    float limiterEnv = 0.0f, limiterSlapp = 0.999f;
    int transponering = 0;
    bool basHaller = false;
    std::array<Vantande, 64> vantande {};
    int antalVantande = 0;
};

} // namespace km
