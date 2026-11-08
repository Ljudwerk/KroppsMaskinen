// Motorn – kopplar ihop Taktverket, kanalerna, Drivet, mixern och effekterna.
// Allt räknas sample för sample, så att stegen, ratchets och gatelängderna hamnar exakt.
//
// Kanaler: Kolvbasen, Kolvbas 2, Stampverket och Hugget. Varje kanal har en bank med 16 mönster (A och B),
// egen längd (polymeter), Drivet, nivå, panorering, sändning A (Grindrummet) och B (Ekot), tyst och solo.
// Byter man mönster medan sekvensern går sker bytet på nästa takt.
//
// Kolvbas 2 kan följa Kolvbasen: den spelar då Kolvbasens mönster, förskjutet 0–3 sextondelar och
// transponerat, med sin egen klang.
//
// Mastern: kanalerna + Grindrummet + Ekot → Svajet → Klämman (med bastrumman som sidokedja) → Kassetten →
// master → DC-spärr → toppbegränsare.
//
// Var 32:a sample räknas modulationen, Spänningen och Varvtalets musikaliska läge fram och läggs på
// parametrarna.
//
// MIDI: kanal 10 spelar Stampverket (GM-trumkartan), kanal 2 Kolvbas 2, kanal 3 Hugget, övriga Kolvbasen –
// när sekvensern står still. När den går flyttar en tangent alla tonkanaler (transponering från grundtonen).
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "Kolvbasen.h"
#include "Stamning.h"
#include "Stampverket.h"
#include "Hugget.h"
#include "Drivet.h"
#include "Effekter.h"
#include "Modulation.h"
#include "../takt/Monster.h"
#include "../takt/Laten.h"
#include "../takt/Taktverket.h"

namespace km
{

struct KanalParams
{
    int langd = 16;               // 1..64 steg
    int monster = 0;              // 0..15
    Variant variant = Variant::A;
    DrivParams drivet;
    float niva = 0.8f;
    float pan = 0.0f;             // −1 vänster .. +1 höger
    float sandA = 0.0f;           // till Grindrummet
    float sandB = 0.0f;           // till Ekot
    bool tyst = false, solo = false;
    bool midi = false;            // spelas av inkommande MIDI i stället för av sekvensern
};

// Varvtalet: 45 varv, 33 varv eller 33 varv med pitchen uppskruvad 8 %.
enum class Varv { V45 = 0, V33, V33plus };
enum class VarvLage { Musikaliskt = 0, Skivspelare };
inline double varvFaktor (Varv v) { return v == Varv::V45 ? 1.0 : (v == Varv::V33 ? 100.0 / 135.0 : 100.0 / 135.0 * 1.08); }

struct MotorParams
{
    std::array<KanalParams, antalKanaler> kanaler {};
    std::array<KolvbasParams, 2> kolvbas {};
    StampverketParams stampverket;
    HuggParams hugget;
    std::array<bool, antalSpar> sparTyst {};
    TaktInstallningar takt;
    int grundton = 33;            // MIDI-ton för basgångens 0 (A1)
    int huggOktav = 0;            // Hugget spelar två oktaver över grundtonen, plus detta
    bool foljKolvbasen = false;   // Kolvbas 2 spelar Kolvbasens mönster
    int forskjutning = 0;         // 0..3 sextondelar
    int transponering = 0;        // −24..+24 halvtoner
    GrindrumParams grindrum;
    EkoParams eko;
    SvajParams svaj;
    KlamParams klam;
    KassettParams kassetten;
    float master = 0.65f;
    float spanning = 0.0f;        // 0..1: Spänningen
    double musikalisktVarv = 1.0; // Varvtalet i musikaliskt läge (1 = av; skivspelarläget sköts av processorn)
    bool latLage = false;         // låtläget: Låten styr mönster, varianter och tystnad takt för takt
    bool latLoopa = true;         // efter sista raden: börja om (annars tyst)
    int stamning = 0;             // StamningTyp
    int stamningTon = 0;          // tonikan: 0 = grundtonen, 1–12 = C … H
    ModParams mod;
};

// Mönstren som motorn spelar (ägs av processorn; gränssnittet skriver, ljudtråden läser).
struct Banker
{
    TonBank kolvbasen, kolvbas2, hugget;
    TrumBank trummor;
    Laten laten;
    TonBank& ton (int kanal) { return kanal == 0 ? kolvbasen : (kanal == 1 ? kolvbas2 : hugget); }
    const TonBank& ton (int kanal) const { return kanal == 0 ? kolvbasen : (kanal == 1 ? kolvbas2 : hugget); }
};

class Motor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& k : kolvbas) { k.prepare (sampleRate); k.setStamning (&stamningTabell); }
        hugget.setStamning (&stamningTabell);
        stamningNyckel = -1;
        uppdateraStamning();
        stampverket.prepare (sampleRate);
        hugget.prepare (sampleRate);
        taktverket.prepare (sampleRate);
        for (auto& d : drivet) d.prepare (sampleRate);
        grindrum.prepare (sampleRate);
        kassetten.prepare (sampleRate);
        eko.prepare (sampleRate);
        svaj.prepare (sampleRate);
        klam.prepare (sampleRate);
        modulation.prepare (sampleRate);
        for (auto& d : dc) d.prepare (sampleRate);
        limiterSlapp = (float) std::exp (-1.0 / (0.08 * sampleRate));
        nivaKoef = tidskonstant (0.01, sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& k : kolvbas) k.reset();
        stampverket.reset();
        hugget.reset();
        taktverket.reset();
        for (auto& d : drivet) d.reset();
        grindrum.reset(); eko.reset(); svaj.reset(); klam.reset(); kassetten.reset();
        modulation.reset();
        for (auto& d : dc) d.reset();
        limiterEnv = 0.0f;
        forstaBlocket = true;
        transponering = 0;
        basHaller = { false, false };
        huggHaller = -1;
        antalVantande = 0;
        antalHuggAv = 0;
        kontrollRaknare = 0;
        gateFore.fill (false);
        for (int k = 0; k < antalKanaler; ++k)
        {
            aktivtMonster[(size_t) k] = bas.kanaler[(size_t) k].monster;
            spelarMonster[(size_t) k].store (-1);
            aktuelltSteg[(size_t) k].store (-1);
        }
    }

    // Grundinställningarna från parametrarna; modulation, Spänningen och Varvtalet läggs på i tillampa().
    void setParams (const MotorParams& p)
    {
        bas = p;
        uppdateraStamning();
        modulation.setParams (p.mod);
        tillampa();
    }

    void process (float* left, float* right, int numSamples, const juce::MidiBuffer& midi,
                  const Transport& transport, bool egenKlocka, const Banker& banker)
    {
        auto takt = bas.takt;
        if (! transport.hostSpelar) takt.tempo *= bas.musikalisktVarv;   // egen klocka: musikaliskt varvtal sänker tempot
        const int antalSteg = taktverket.process (takt, transport, egenKlocka, numSamples);
        const bool gar = taktverket.gar();
        if (taktverket.hoppade())
        {
            for (auto& k : kolvbas) k.allNotesOff();
            hugget.allNotesOff();
            basHaller = { false, false };
            huggHaller = -1;
            antalVantande = 0;
            antalHuggAv = 0;
            latOmvardera = true;
        }
        if (! gar)
        {
            for (int k = 0; k < antalKanaler; ++k)
            {
                aktivtMonster[(size_t) k] = bas.kanaler[(size_t) k].monster;
                spelarMonster[(size_t) k].store (-1, std::memory_order_relaxed);
                aktuelltSteg[(size_t) k].store (-1, std::memory_order_relaxed);
            }
            latRadNu.store (-1, std::memory_order_relaxed);
        }

        auto midiIt = midi.cbegin();
        int stegIndex = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            if (kontrollRaknare-- <= 0)
            {
                kontrollRaknare = Modulation::kontrollTakt - 1;
                modulation.stega (Modulation::kontrollTakt, taktverket.ppqVid (i), taktverket.bpm());
                foljGates();
                tillampa();
            }

            for (; midiIt != midi.cend() && (*midiIt).samplePosition <= i; ++midiIt)
                hanteraMidi ((*midiIt).getMessage(), gar);

            while (stegIndex < antalSteg && taktverket.handelse (stegIndex).offset <= i)
                spelaSteg (taktverket.handelse (stegIndex++).k, banker);

            tickVantande();

            // --- Kanalerna: torr signal, Drivet, nivå och panorering.
            float hl = 0.0f, hr = 0.0f;
            hugget.tick (hl, hr);
            const float b0 = drivet[0].tick (kolvbas[0].tick());
            const float b1 = drivet[1].tick (kolvbas[1].tick());
            const float tr = drivet[2].tick (stampverket.tick());
            hl = drivet[3].tick (hl);
            hr = drivet[4].tick (hr);

            float L = 0.0f, R = 0.0f, aL = 0.0f, aR = 0.0f, bL = 0.0f, bR = 0.0f;
            auto kanal = [&] (size_t k, float l, float r)
            {
                kanalNiva[k] += (mal[k] - kanalNiva[k]) * nivaKoef;
                l *= panL[k] * kanalNiva[k];
                r *= panR[k] * kanalNiva[k];
                blockTopp[k] = std::max (blockTopp[k], std::max (std::abs (l), std::abs (r)));
                L += l; R += r;
                aL += l * effektiv.kanaler[k].sandA; aR += r * effektiv.kanaler[k].sandA;
                bL += l * effektiv.kanaler[k].sandB; bR += r * effektiv.kanaler[k].sandB;
            };
            kanal (0, b0, b0);
            kanal (1, b1, b1);
            kanal (2, tr, tr);
            kanal (3, hl, hr);

            // --- Effekterna
            float gL, gR, eL, eR;
            grindrum.tick (aL, aR, gL, gR);
            eko.tick (bL, bR, eL, eR);
            L += gL + eL;
            R += gR + eR;
            svaj.tick (L, R);
            klam.tick (L, R, stampverket.bastrumma() * kanalNiva[2]);   // den glidande nivån: tyst och solo knäpper inte i sidokedjan
            kassetten.tick (L, R);

            L = dc[0].tick (L * effektiv.master);
            R = dc[1].tick (R * effektiv.master);

            // Toppbegränsare (länkad): snabb attack, 80 ms släpp, tak strax under 0 dBFS.
            const float a = std::max (std::abs (L), std::abs (R));
            limiterEnv = a > limiterEnv ? a : limiterEnv * limiterSlapp + a * (1.0f - limiterSlapp);
            if (limiterEnv > tak) { const float g = tak / limiterEnv; L *= g; R *= g; }

            left[i] = juce::jlimit (-1.0f, 1.0f, L);
            if (right != nullptr) right[i] = juce::jlimit (-1.0f, 1.0f, R);

            if (--scopeRaknare <= 0)
            {
                scopeRaknare = scopeSteg;
                const int pos = scopePos.load (std::memory_order_relaxed);
                scope[(size_t) pos].store (0.5f * (L + R), std::memory_order_relaxed);
                scopePos.store ((pos + 1) % scopeLangd, std::memory_order_relaxed);
            }
        }

        for (size_t k = 0; k < (size_t) antalKanaler; ++k)
        {
            float f = kanalTopp[k].load (std::memory_order_relaxed);
            while (blockTopp[k] > f && ! kanalTopp[k].compare_exchange_weak (f, blockTopp[k])) {}
            blockTopp[k] = 0.0f;
        }
        ppqNu.store (taktverket.ppqVid (numSamples), std::memory_order_relaxed);
    }

    // Spolar tillbaka: sekvensern börjar om från steg 0 (egen klocka).
    void spola() { taktverket.reset(); }

    // För gränssnittet.
    std::array<std::atomic<int>, antalKanaler> aktuelltSteg {}, spelarMonster {};
    std::array<std::atomic<uint32_t>, antalKanaler> kanalSlag {};   // ökar för varje slag/ton
    std::array<std::atomic<uint32_t>, antalSpar> trumSlag {};       // ökar för varje trumslag
    std::array<std::atomic<float>, antalKanaler> kanalTopp {};      // topp sedan gränssnittet läste (byts mot 0)
    std::atomic<double> ppqNu { -1.0 };                             // positionen vid blockets slut, −1 = stilla
    static constexpr int scopeLangd = 512, scopeSteg = 4;           // var fjärde sampel, ≈ 43 ms
    std::array<std::atomic<float>, scopeLangd> scope {};
    std::atomic<int> scopePos { 0 };
    std::atomic<juce::int64> antalSpeladeSteg { 0 };
    std::atomic<int> latRadNu { -1 }, latTaktNu { 0 };              // låtläget: spelande rad (−1 = ingen) och takt i raden
    int getTransponering() const { return transponering; }
    float getKlamReduktionDb() const { return klam.getReduktionDb(); }
    const Modulation& getModulation() const { return modulation; }

    // Den egna (Scala-)stämningen, som processorn äger.
    void setEgenStamning (const EgenStamning* e) { egenStamning = e; stamningNyckel = -1; }
    const StamningTabell& getStamningTabell() const { return stamningTabell; }

private:
    // Räknar om stämningstabellen när stämningen, tonikan, grundtonen eller den egna skalan ändras.
    void uppdateraStamning()
    {
        const juce::int64 nyckel = (juce::int64) bas.stamning + 16 * bas.stamningTon + 256 * bas.grundton
                                 + 65536 * (juce::int64) (bas.stamning == (int) StamningTyp::Egen && egenStamning != nullptr ? egenStamning->getVersion() : 0);
        if (nyckel == stamningNyckel) return;
        stamningNyckel = nyckel;
        const auto typ = (StamningTyp) juce::jlimit (0, antalStamningar - 1, bas.stamning);
        const int ankare = tonikaTangent (bas.grundton, bas.stamningTon);
        if (typ == StamningTyp::Egen && egenStamning != nullptr) raknaTabell (egenStamning->hamta(), ankare, stamningTabell);
        else raknaTabell (Skala::inbyggd (typ), ankare, stamningTabell);
    }

    // Lägger på Varvtalet (musikaliskt), Spänningen och modulationen och skickar ut parametrarna.
    void tillampa()
    {
        effektiv = bas;
        auto& e = effektiv;
        const bool mod = modulation.aktiv();
        auto m = [&] (ModMal mm) { return mod ? modulation.malVarde (mm) : 0.0f; };

        // --- Varvtalet, musikaliskt läge: tonhöjden sänks och allt klingar längre, som en långsammare skiva.
        const double f = bas.musikalisktVarv;
        if (f < 0.999)
        {
            const float st = (float) (12.0 * std::log2 (f));
            const float tid = (float) (1.0 / f);
            for (auto& k : e.kolvbas)
            {
                k.tonhojdOffset += st;
                k.fall *= tid; k.slapp *= tid; k.filterFall *= tid; k.glid *= tid;
            }
            e.hugget.tonhojdOffset += st;
            e.hugget.attack *= tid; e.hugget.fall *= tid; e.hugget.slapp *= tid;
            e.stampverket.stamningOffset += st;
            e.stampverket.tidFaktor *= tid;
        }

        // --- Spänningen: hela maskinen mot mer energi, nivån kompenserad.
        const float spanBas = latAktiv && ! latSlut && latNu.spanning >= 0 ? (float) latNu.spanning * 0.01f : bas.spanning;
        const float s = std::clamp (spanBas + m (ModMal::Spanningen), 0.0f, 1.0f);
        if (s > 0.001f)
        {
            for (auto& k : e.kolvbas)
            {
                k.accent = std::min (1.0f, k.accent + 0.3f * s);
                k.envMangd = std::min (1.0f, k.envMangd + 0.25f * s);
                k.filterFall *= 1.0f - 0.4f * s;
                k.resonans = std::min (0.95f, k.resonans + 0.1f * s);
                k.drive = std::min (1.0f, k.drive + 0.3f * s);
            }
            for (auto& k : e.kanaler)
            {
                if (k.drivet.lage == DrivLage::Av) { k.drivet.lage = DrivLage::Ror; k.drivet.mangd = 0.5f * s; }
                else k.drivet.mangd = std::min (1.0f, k.drivet.mangd + 0.4f * s);
            }
            auto& sv = e.kanaler[(size_t) Kanal::Stampverket];
            sv.sandA = std::min (1.0f, sv.sandA + 0.3f * s);
            e.grindrum.hallTid += 0.1f * s;
            e.eko.aterkoppling = std::min (0.9f, e.eko.aterkoppling + 0.2f * s);
            e.klam.troskelDb -= 6.0f * s;
            e.klam.efterDb += 2.0f * s;
            e.klam.pump = std::min (1.0f, e.klam.pump + 0.12f * s);
            e.hugget.klang = std::min (1.0f, e.hugget.klang + 0.2f * s);
        }

        // --- Modulationen
        if (mod)
        {
            e.kolvbas[0].cutoffHz = std::clamp (e.kolvbas[0].cutoffHz * std::exp2 (3.0f * m (ModMal::KolvbasenFilter)), 20.0f, 16000.0f);
            e.kolvbas[1].cutoffHz = std::clamp (e.kolvbas[1].cutoffHz * std::exp2 (3.0f * m (ModMal::Kolvbas2Filter)), 20.0f, 16000.0f);
            e.kolvbas[0].pulsbredd = std::clamp (e.kolvbas[0].pulsbredd + 0.3f * m (ModMal::KolvbasenPuls), 0.05f, 0.5f);
            e.hugget.klang = std::clamp (e.hugget.klang + 0.5f * m (ModMal::HuggetKlang), 0.0f, 1.0f);
            e.hugget.tonhojdOffset += 2.0f * m (ModMal::HuggetTonhojd);
            const float d = m (ModMal::Drivet);
            for (auto& k : e.kanaler) k.drivet.mangd = std::clamp (k.drivet.mangd + 0.5f * d, 0.0f, 1.0f);
            e.grindrum.niva *= std::max (0.0f, 1.0f + m (ModMal::Grindrummet));
            e.eko.niva *= std::max (0.0f, 1.0f + m (ModMal::Ekot));
        }

        kolvbas[0].setParams (e.kolvbas[0]);
        kolvbas[1].setParams (e.kolvbas[1]);
        stampverket.setParams (e.stampverket);
        hugget.setParams (e.hugget);
        for (size_t k = 0; k < 3; ++k) drivet[k].setParams (e.kanaler[k].drivet);
        drivet[3].setParams (e.kanaler[3].drivet);
        drivet[4].setParams (e.kanaler[3].drivet);
        grindrum.setParams (e.grindrum);
        eko.setParams (e.eko, taktverket.gar() ? taktverket.bpm() : bas.takt.tempo);
        svaj.setParams (e.svaj);
        klam.setParams (e.klam);
        kassetten.setParams (e.kassetten);

        // Tyst och solo, nivå och panorering (lika effekt, 0 dB i mitten).
        bool nagonSolo = false;
        for (auto& k : e.kanaler) nagonSolo = nagonSolo || k.solo;
        for (size_t k = 0; k < (size_t) antalKanaler; ++k)
        {
            const auto& kp = e.kanaler[k];
            const bool latTyst = latAktiv && (latSlut || latNu.tyst[k]);
            mal[k] = (kp.tyst || latTyst || (nagonSolo && ! kp.solo)) ? 0.0f : kp.niva;
            if (forstaBlocket) kanalNiva[k] = mal[k];
            const float vinkel = (std::clamp (kp.pan, -1.0f, 1.0f) + 1.0f) * (float) pi * 0.25f;
            panL[k] = std::cos (vinkel) * 1.41421356f;
            panR[k] = std::sin (vinkel) * 1.41421356f;
        }
        forstaBlocket = false;
    }

    // Envelopes i modulationen släpps när kanalens ton släpps.
    void foljGates()
    {
        const std::array<bool, 3> nu { kolvbas[0].haller(), kolvbas[1].haller(), hugget.haller() };
        static constexpr std::array<EnvUtlosare, 3> kallor { EnvUtlosare::Kolvbasen, EnvUtlosare::Kolvbas2, EnvUtlosare::Hugget };
        for (size_t i = 0; i < 3; ++i)
        {
            if (gateFore[i] && ! nu[i]) modulation.tonAv (kallor[i]);
            gateFore[i] = nu[i];
        }
    }

    void slaTrumma (Trumma t, float vel)
    {
        stampverket.slag (t, vel);
        trumSlag[(size_t) t].fetch_add (1, std::memory_order_relaxed);
        kanalSlag[(size_t) Kanal::Stampverket].fetch_add (1, std::memory_order_relaxed);
    }
    void raknaSlag (Kanal k) { kanalSlag[(size_t) k].fetch_add (1, std::memory_order_relaxed); }

    void hanteraMidi (const juce::MidiMessage& m, bool gar)
    {
        if (m.isController() && m.getControllerNumber() == 1) { modulation.setModhjul ((float) m.getControllerValue() / 127.0f); return; }
        if (m.isChannelPressure()) { modulation.setAftertouch ((float) m.getChannelPressureValue() / 127.0f); return; }
        if (m.isAftertouch()) { modulation.setAftertouch ((float) m.getAfterTouchValue() / 127.0f); return; }

        if (m.getChannel() == 10)
        {
            if (m.isNoteOn())
                if (const int s = Stampverket::sparForNot (m.getNoteNumber()); s >= 0)
                {
                    slaTrumma ((Trumma) s, m.getFloatVelocity());
                    if (s == 0) modulation.tonPa (EnvUtlosare::Bastrumma, m.getFloatVelocity());
                }
            return;
        }

        if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& k : kolvbas) k.allNotesOff(); hugget.allNotesOff(); return; }

        // MIDI-kanal 1 Kolvbasen, 2 Kolvbas 2, 3 Hugget. När sekvensern går transponerar tonerna i stället,
        // utom till en kanal som spelas från MIDI (då spelar tonen alltid kanalen).
        const int kanal = m.getChannel();
        const int malKanal = kanal == 2 ? 1 : (kanal == 3 ? 3 : 0);
        if (gar && ! bas.kanaler[(size_t) malKanal].midi)
        {
            if (m.isNoteOn()) transponering = juce::jlimit (-24, 24, m.getNoteNumber() - bas.grundton);
            return;
        }
        if (kanal == 3)
        {
            if (m.isNoteOn()) { hugget.noteOn (m.getNoteNumber(), m.getFloatVelocity()); modulation.tonPa (EnvUtlosare::Hugget, m.getFloatVelocity()); raknaSlag (Kanal::Hugget); }
            else if (m.isNoteOff()) hugget.noteOff (m.getNoteNumber());
            return;
        }
        auto& b = kolvbas[kanal == 2 ? 1 : 0];
        if (m.isNoteOn())
        {
            b.noteOn (m.getNoteNumber(), m.getFloatVelocity());
            raknaSlag (kanal == 2 ? Kanal::Kolvbas2 : Kanal::Kolvbasen);
            modulation.tonPa (kanal == 2 ? EnvUtlosare::Kolvbas2 : EnvUtlosare::Kolvbasen, m.getFloatVelocity());
        }
        else if (m.isNoteOff()) b.noteOff (m.getNoteNumber());
    }

    Variant variantAv (Kanal c) const
    {
        return latAktiv ? (Variant) latNu.variant[(size_t) c] : bas.kanaler[(size_t) c].variant;
    }
    int latTransponering() const { return latAktiv && ! latSlut ? latNu.transponering : 0; }

    static int variantFor (Variant v, juce::int64 k, int langd)
    {
        if (v == Variant::AB) return (int) ((k / juce::jmax (1, langd)) & 1);
        return v == Variant::B ? 1 : 0;
    }

    void spelaSteg (juce::int64 k, const Banker& banker)
    {
        antalSpeladeSteg.fetch_add (1, std::memory_order_relaxed);

        // Nytt mönster tar över på nästa takt. I låtläget bestämmer Låten (även direkt efter start eller hopp).
        if (k % 16 == 0 || latOmvardera)
        {
            latOmvardera = false;
            latAktiv = bas.latLage && ! banker.laten.arTom();
            if (latAktiv)
            {
                const auto plats = banker.laten.plats (k / 16, bas.latLoopa);
                latSlut = plats.rad < 0;
                if (! latSlut) latNu = banker.laten.get (plats.rad);
                for (size_t c = 0; c < (size_t) antalKanaler; ++c) aktivtMonster[c] = latNu.monster[c];
                latRadNu.store (plats.rad, std::memory_order_relaxed);
                latTaktNu.store (plats.taktIRad, std::memory_order_relaxed);
                tillampa();
            }
            else
            {
                if (k % 16 == 0)
                    for (size_t c = 0; c < (size_t) antalKanaler; ++c)
                        aktivtMonster[c] = bas.kanaler[c].monster;
                latRadNu.store (-1, std::memory_order_relaxed);
            }
        }

        const double perSteg = taktverket.samplesPerSteg();

        // --- Stampverket (inte när kanalen spelas från MIDI)
        if (! bas.kanaler[(size_t) Kanal::Stampverket].midi)
        {
            const auto& kp = bas.kanaler[(size_t) Kanal::Stampverket];
            const int langd = juce::jlimit (1, maxSteg, kp.langd);
            const int steg = (int) (k % langd);
            const int m = aktivtMonster[(size_t) Kanal::Stampverket];
            const auto& monster = banker.trummor.get (m, variantFor (variantAv (Kanal::Stampverket), k, langd));
            aktuelltSteg[(size_t) Kanal::Stampverket].store (steg, std::memory_order_relaxed);
            spelarMonster[(size_t) Kanal::Stampverket].store (m, std::memory_order_relaxed);
            for (int s = 0; s < TrumMonster::spar; ++s)
            {
                if (bas.sparTyst[(size_t) s]) continue;
                const auto st = monster.get (s, steg);
                if (st.slag == 0) continue;
                if (st.chans > 0 && Taktverket::chansSlag (k, s) >= chansVarden[(size_t) st.chans]) continue;
                const float vel = st.slag == 2 ? 1.0f : 0.72f;
                slaTrumma ((Trumma) s, vel);
                if (s == 0) modulation.tonPa (EnvUtlosare::Bastrumma, vel);
                const int slag = st.ratchet + 1;
                for (int h = 1; h < slag; ++h)
                    vanta (s, (int) (perSteg * h / slag), vel * 0.85f);
            }
        }

        // --- Kolvbasen och Kolvbas 2
        for (int b = 0; b < 2; ++b)
        {
            if (bas.kanaler[(size_t) b].midi) continue;   // spelas från MIDI
            const bool folj = b == 1 && bas.foljKolvbasen;
            const int kalla = folj ? 0 : b;                    // vems mönster som spelas
            if (bas.kanaler[(size_t) kalla].midi) continue;    // följer en kanal som spelas från MIDI
            const auto& kp = bas.kanaler[(size_t) kalla];
            const int langd = juce::jlimit (1, maxSteg, kp.langd);
            const juce::int64 kk = folj ? k - bas.forskjutning : k;
            if (kk < 0) continue;
            const int steg = (int) (kk % langd);
            const int m = aktivtMonster[(size_t) kalla];
            const auto& monster = banker.ton (kalla).get (m, variantFor (variantAv ((Kanal) kalla), kk, langd));
            aktuelltSteg[(size_t) b].store (steg, std::memory_order_relaxed);
            spelarMonster[(size_t) b].store (folj ? -2 : m, std::memory_order_relaxed);   // −2: följer Kolvbasen

            const auto st = monster.get (steg);
            if (st.slag == 0) continue;   // paus: en glidande ton hålls kvar tills nästa ton

            const int extra = (b == 1 ? bas.transponering : 0) + latTransponering();
            const int ton = juce::jlimit (0, 127, bas.grundton + transponering + extra + st.tonhojd());
            const int gate = st.glid ? -1 : juce::jmax (1, (int) (perSteg * tonLangder[(size_t) st.langd]));
            kolvbas[(size_t) b].steg (ton, st.slag == 2, basHaller[(size_t) b], gate);
            basHaller[(size_t) b] = st.glid;
            modulation.tonPa (b == 0 ? EnvUtlosare::Kolvbasen : EnvUtlosare::Kolvbas2, st.slag == 2 ? 1.0f : 0.72f);
            raknaSlag ((Kanal) b);
        }

        // --- Hugget
        if (! bas.kanaler[(size_t) Kanal::Hugget].midi)
        {
            const auto& kp = bas.kanaler[(size_t) Kanal::Hugget];
            const int langd = juce::jlimit (1, maxSteg, kp.langd);
            const int steg = (int) (k % langd);
            const int m = aktivtMonster[(size_t) Kanal::Hugget];
            const auto& monster = banker.hugget.get (m, variantFor (variantAv (Kanal::Hugget), k, langd));
            aktuelltSteg[(size_t) Kanal::Hugget].store (steg, std::memory_order_relaxed);
            spelarMonster[(size_t) Kanal::Hugget].store (m, std::memory_order_relaxed);
            const auto st = monster.get (steg);
            if (st.slag != 0)
            {
                if (huggHaller >= 0) { hugget.noteOff (huggHaller); huggHaller = -1; }
                const int ton = juce::jlimit (0, 127, bas.grundton + 12 * (2 + bas.huggOktav) + transponering + latTransponering() + st.tonhojd());
                const float vel = st.slag == 2 ? 1.0f : 0.72f;
                hugget.noteOn (ton, vel);
                modulation.tonPa (EnvUtlosare::Hugget, vel);
                raknaSlag (Kanal::Hugget);
                if (st.glid) huggHaller = ton;   // hålls till nästa ton
                else if (antalHuggAv < (int) huggAv.size())
                    huggAv[(size_t) antalHuggAv++] = { ton, juce::jmax (1, (int) (perSteg * tonLangder[(size_t) st.langd])) };
            }
        }
    }

    // Ratchets och Huggets toner som ska släppas: händelser senare inom steget.
    struct Vantande { int spar, om; float vel; };
    struct HuggAv { int ton, om; };
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
                slaTrumma ((Trumma) vantande[(size_t) i].spar, vantande[(size_t) i].vel);
                vantande[(size_t) i] = vantande[(size_t) --antalVantande];
            }
            else ++i;
        }
        for (int i = 0; i < antalHuggAv;)
        {
            if (--huggAv[(size_t) i].om <= 0)
            {
                hugget.noteOff (huggAv[(size_t) i].ton);
                huggAv[(size_t) i] = huggAv[(size_t) --antalHuggAv];
            }
            else ++i;
        }
    }

    static constexpr float tak = 0.966f; // −0,3 dBFS

    double sr = 48000.0;
    MotorParams bas, effektiv;
    std::array<Kolvbasen, 2> kolvbas;
    Stampverket stampverket;
    Hugget hugget;
    StamningTabell stamningTabell {};
    const EgenStamning* egenStamning = nullptr;
    juce::int64 stamningNyckel = -1;
    Taktverket taktverket;
    std::array<Drivet, 5> drivet;           // Kolvbasen, Kolvbas 2, Stampverket, Hugget vänster och höger
    Grindrummet grindrum;
    Ekot eko;
    Svajet svaj;
    Klamman klam;
    Kassetten kassetten;
    Modulation modulation;
    std::array<DcSparr, 2> dc;
    float limiterEnv = 0.0f, limiterSlapp = 0.999f, nivaKoef = 0.01f;
    std::array<float, antalKanaler> kanalNiva {}, mal {}, panL {}, panR {}, blockTopp {};
    int scopeRaknare = 0;
    bool latAktiv = false, latSlut = false, latOmvardera = true;
    LatRad latNu;
    std::array<int, antalKanaler> aktivtMonster {};
    int transponering = 0, kontrollRaknare = 0, huggHaller = -1;
    bool forstaBlocket = true;
    std::array<bool, 2> basHaller { false, false };
    std::array<bool, 3> gateFore {};
    std::array<Vantande, 64> vantande {};
    int antalVantande = 0;
    std::array<HuggAv, 32> huggAv {};
    int antalHuggAv = 0;
};

} // namespace km
