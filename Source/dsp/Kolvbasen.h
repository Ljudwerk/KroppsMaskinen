// Kolvbasen – sekvensbasen. Samma motor spelar både Kolvbasen och Kolvbas 2.
// Två oscillatorer (sågtand som tonar över i puls), synk, en sub en oktav ned, brus, ett eget 24 dB-ladderfilter
// (ZDF, så att cutoff kan ändras varje sample och filterenvelopen blir knivskarp) eller ett tunnare 12 dB-filter,
// och en accentväg som höjer både nivån och filtersvepet, som på de gamla monofona sekvensbasarna.
//
// Glid: en ton som spelas medan förra tonen fortfarande hålls (legato) glider dit och startar inte om envelopes.
//
// Utgår från Basmålen i KrangBox, men filtret och envelopes är omskrivna för snabbare attack och accent.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>

namespace km
{

struct KolvbasParams
{
    float vag        = 0.0f;    // 0 = sågtand, 1 = puls
    float pulsbredd  = 0.5f;    // 0,05..0,5
    float sub        = 0.5f;    // 0..1
    float cutoffHz   = 220.0f;
    float resonans   = 0.35f;   // 0..1 (självsvänger nära 1)
    float envMangd   = 0.6f;    // 0..1, upp till sex oktaver
    float filterFall = 0.18f;   // sekunder
    float accent     = 0.6f;    // 0..1
    float fall       = 0.30f;   // förstärkarens fall, sekunder
    float hall       = 0.6f;    // förstärkarens hållnivå 0..1
    float slapp      = 0.012f;  // släpp, sekunder
    float glid       = 0.045f;  // sekunder
    float drive      = 0.3f;    // 0..1 (filtrets ingång)

    // Fas 1
    float osc2Niva   = 0.0f;    // 0..1
    float osc2Halvtoner = 0.0f; // −24..+24
    float osc2Fin    = 0.0f;    // cent
    bool  synk       = false;   // osc 2 startas om av osc 1
    float brus       = 0.0f;    // 0..1
    int   filterTyp  = 0;       // 0 = 24 dB ladder, 1 = 12 dB (tunnare)

    // Fas 2
    float tonhojdOffset = 0.0f; // halvtoner (Varvtalet)
};

// Ett sample av vågformen (sågtand som tonar över i puls) vid fasen ph, med polyBLEP.
inline double kolvVag (double ph, double dt, double vag, double pw)
{
    const double saw = 2.0 * ph - 1.0 - polyBlep (ph, dt);
    double pulse = ph < pw ? 1.0 : -1.0;
    pulse += polyBlep (ph, dt);
    pulse -= polyBlep (std::fmod (ph + 1.0 - pw, 1.0), dt);
    pulse += 2.0 * pw - 1.0; // ta bort pulsens DC
    return (1.0 - vag) * saw + vag * pulse;
}

// Ett 24 dB-ladderfilter med nollfördröjd återkoppling och mättnad på ingången.
class Ladder
{
public:
    void reset() { s.fill (0.0f); }

    float tick (float x, double cutoffHz, float reso, float drive, double sr)
    {
        const double g = std::tan (pi * std::clamp (cutoffHz, 15.0, sr * 0.45) / sr);
        const float G = (float) (g / (1.0 + g));
        const float b = (float) (1.0 / (1.0 + g));
        const float k = 4.0f * reso;

        // y4 = G⁴·u + S, där S är bidraget från stegens tillstånd.
        const float S = b * (G * G * G * s[0] + G * G * s[1] + G * s[2] + s[3]);
        const float G4 = G * G * G * G;
        const float comp = 1.0f + 0.6f * reso;  // håll kvar basen när resonansen ökar
        float u = (x * comp - k * S) / (1.0f + k * G4);
        u = mjukKlipp (u * drive) / drive;

        float in = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (in - s[(size_t) i]) * G;
            const float y = v + s[(size_t) i];
            s[(size_t) i] = y + v;
            in = y;
        }
        return in;
    }

private:
    std::array<float, 4> s {};
};

class Kolvbasen
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        numHeld = 0;
        ph = ph2 = phSub = 0.0;
        svf.reset();
        ampEnv = filterEnv = 0.0f;
        stage = Stage::Av;
        ladder.reset();
        accentNu = 0.0f;
        gateOffIn = -1;
    }

    void setParams (const KolvbasParams& p) { params = p; }

    // Från keyboardet eller MIDI: håller reda på nedtryckta tangenter för legato och glid.
    void noteOn (int note, float velocity)
    {
        removeHeld (note);
        const bool legato = numHeld > 0 && stage != Stage::Av && stage != Stage::Slapp;
        if (numHeld < (int) held.size()) held[(size_t) numHeld++] = note;
        spela (note, velocity >= 0.95f, legato);
    }

    void noteOff (int note)
    {
        removeHeld (note);
        if (numHeld > 0) targetNote = (double) held[(size_t) numHeld - 1];
        else slapp();
    }

    // Från sekvensern: en ton med accent, som glider dit om legato är sant. gateSamples < 0 = håll tills slapp().
    void steg (int note, bool accent, bool legato, int gateSamples)
    {
        numHeld = 0;
        spela (note, accent, legato && stage != Stage::Av && stage != Stage::Slapp);
        gateOffIn = gateSamples;
    }

    void slapp()
    {
        gateOffIn = -1;
        if (stage != Stage::Av) stage = Stage::Slapp;
    }

    void allNotesOff() { numHeld = 0; slapp(); }

    // Ett sample, mono.
    float tick()
    {
        if (gateOffIn >= 0 && gateOffIn-- == 0) slapp();
        if (stage == Stage::Av) return 0.0f;

        // --- Tonhöjd och oscillatorer
        currentNote += (targetNote - currentNote) * glidKoef;
        const double f = noteToHz (currentNote + params.tonhojdOffset);
        const double dt = f / sr, dtSub = dt * 0.5;

        const double pw = std::clamp ((double) params.pulsbredd, 0.05, 0.5);
        const double vag = params.vag;
        const double o1 = kolvVag (ph, dt, vag, pw);
        const double sub = std::sin (twoPi * phSub);

        // Osc 2: egen stämning, och med synk startas den om varje gång osc 1 börjar en ny period.
        double o2 = 0.0;
        if (params.osc2Niva > 0.0f)
        {
            const double dt2 = std::min (0.45, dt * std::exp2 ((params.osc2Halvtoner + params.osc2Fin * 0.01) / 12.0));
            o2 = kolvVag (ph2, dt2, vag, pw);
            ph2 += dt2; if (ph2 >= 1.0) ph2 -= 1.0;
            if (params.synk && ph + dt >= 1.0) ph2 = (ph + dt - 1.0) / dt * dt2;   // synk: följ osc 1:s nya period
        }

        ph += dt;       if (ph >= 1.0) ph -= 1.0;
        phSub += dtSub; if (phSub >= 1.0) phSub -= 1.0;

        float x = (float) (o1 + params.osc2Niva * o2) * 0.7f / (1.0f + 0.5f * params.osc2Niva)
                + params.sub * (float) sub * 0.6f
                + params.brus * brusKalla.bipolar() * 0.5f;

        // --- Envelopes
        tickEnvelopes();

        // --- Filter
        const float accentSvep = accentNu * params.accent;
        const double okt = 6.0 * params.envMangd * filterEnv * (1.0f + 0.8f * accentSvep) + 1.5 * accentSvep * filterEnv
                         + (currentNote - 36.0) / 12.0 * 0.3;
        const double cutoff = params.cutoffHz * std::exp2 (okt);
        const float drive = 1.0f + 3.0f * params.drive;
        float y;
        if (params.filterTyp == 0)
            y = ladder.tick (x, cutoff, params.resonans * 0.97f, drive, sr);
        else
        {
            // 12 dB: tunnare och mer "piggig" resonans, som på de minsta sekvensbasarna.
            svf.set (cutoff, 0.6 + 11.0 * params.resonans * params.resonans, sr);
            svf.tick (mjukKlipp (x * drive) / drive);
            y = svf.lp * 1.15f;
        }

        // --- Förstärkare
        const float niva = 1.7f * (1.0f + 0.4f * accentSvep);
        y *= ampEnv * niva;
        return mjukKlipp (y * (1.0f + params.drive)) / (1.0f + 0.4f * params.drive);
    }

    bool isActive() const { return stage != Stage::Av; }
    bool haller() const { return stage == Stage::Anslag || stage == Stage::Fall; }
    int heldNote() const { return (int) std::lround (targetNote); }

private:
    enum class Stage { Av, Anslag, Fall, Slapp };

    void spela (int note, bool accent, bool legato)
    {
        targetNote = (double) note;
        glidKoef = legato ? tidskonstant (params.glid, sr) : 1.0f;
        if (legato) return; // glid: inga nya envelopes

        currentNote = targetNote;
        accentNu = accent ? 1.0f : 0.0f;
        stage = Stage::Anslag;
        filterEnv = 1.0f;     // filtret slår till direkt; det är det som ger klicket i attacken
    }

    void tickEnvelopes()
    {
        const float attackKoef = tidskonstant (0.0012, sr);
        const float fallKoef = tidskonstant (params.fall * 0.35, sr);
        const float slappKoef = tidskonstant (params.slapp * 0.35, sr);

        switch (stage)
        {
            case Stage::Anslag:
                ampEnv += (1.05f - ampEnv) * attackKoef;
                if (ampEnv >= 1.0f) { ampEnv = 1.0f; stage = Stage::Fall; }
                break;
            case Stage::Fall:
                ampEnv += (params.hall - ampEnv) * fallKoef;
                break;
            case Stage::Slapp:
                ampEnv -= ampEnv * slappKoef;
                if (ampEnv < 1.0e-4f) { ampEnv = 0.0f; stage = Stage::Av; ladder.reset(); svf.reset(); }
                break;
            case Stage::Av:
                break;
        }

        // Filterenvelopen faller alltid, även medan tonen hålls; accent gör den lite kortare och skarpare.
        const float ff = params.filterFall * (accentNu > 0.5f ? 0.7f : 1.0f);
        filterEnv -= filterEnv * tidskonstant (ff * 0.35, sr);
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int r = 0; r < numHeld; ++r)
            if (held[(size_t) r] != note) held[(size_t) w++] = held[(size_t) r];
        numHeld = w;
    }

    double sr = 48000.0;
    KolvbasParams params;
    Ladder ladder;

    std::array<int, 16> held {};
    int numHeld = 0;
    double targetNote = 36.0, currentNote = 36.0;
    float glidKoef = 1.0f;

    double ph = 0.0, ph2 = 0.0, phSub = 0.0;
    Svf svf;
    Slump brusKalla { 0x4b4f4c56u };
    float ampEnv = 0.0f, filterEnv = 0.0f, accentNu = 0.0f;
    Stage stage = Stage::Av;
    int gateOffIn = -1;
};

} // namespace km
