// Dimman – mattor och drön (fas 5). Polyfon, tolv röster, stereo.
//
// Tre lägen:
//   Syntmatta:   tre svävande sågtänder och en fyrkant en oktav ned, genom ett mjukt lågpass som öppnar sig
//                med anslaget. Den klassiska 80-talsmattan.
//   Kör:         en ljudkälla (sågtänder med vibrato och lite andning) genom tre formantfilter – vokalen glider
//                från "a" över "o" till "i". Kalla, vokala mattor, helt syntetiska.
//   Drönmaskin:  låga sågtänder ringmodulerade mot en sinus i en oharmonisk kvot, en subsinus, långsam
//                tonhöjdsdrift och brus genom ett svepande bandpass. Industridrön.
//
// Rörelse ger varje röst ett eget långsamt filtersvep (och drift i drönmaskinen), Bredd svävning och
// stereobredd, och Ensemble en kör av tre korta modulerade fördröjningar över hela kanalen.
// Ackordminnet är detsamma som Huggets. Drönläget sköts av motorn (Motor.h): det håller grundtonen eller
// ackordet så länge sekvensern går.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include "Hugget.h"
#include "Effekter.h"
#include <array>

namespace km
{

enum class DimmaLage { Syntmatta = 0, Kor, Dronmaskin };
inline constexpr int antalDimmaLagen = 3;

enum class DronLage { Av = 0, Grundton, Ackord };

struct DimmaParams
{
    DimmaLage lage = DimmaLage::Syntmatta;
    int ackord = 0;            // index i ackordTabell (Huggets)
    int oktav = 0;             // −2..+1 (används av motorn)
    float attack = 0.4f;       // sekunder
    float slapp = 1.2f;        // sekunder
    float klang = 0.5f;        // 0..1: filtret
    float rorelse = 0.4f;      // 0..1: långsamma svep och drift
    float bredd = 0.5f;        // 0..1: svävning och stereobredd
    float vokal = 0.0f;        // 0..1: a → o → i (Kör)
    float ensemble = 0.4f;     // 0..1: ensemblens våta andel
    DronLage dron = DronLage::Av;
    float tonhojdOffset = 0.0f; // halvtoner (Varvtalet och modulation)
};

class DimmaRost
{
public:
    void prepare (double s, int nr) { sr = s; nummer = nr; slump = Slump (0x44494d4du + (uint32_t) nr * 131u); reset(); }
    void reset() { aktiv = false; amp = 0.0f; stage = 0; for (auto& f : filter) f.reset(); }

    void starta (double ton_, float vel_, uint64_t ordning_)
    {
        ton = ton_; vel = vel_; ordning = ordning_;
        stage = 1; aktiv = true;
        if (amp <= 0.0f) { for (auto& p : fas) p = slump.unipolar(); for (auto& f : filter) f.reset(); t = 0; }
        lfoFas = slump.unipolar();
        drift = 0.0; driftMal = 0.0;
    }
    void slapp() { if (aktiv && stage < 3) stage = 3; }
    bool arAktiv() const { return aktiv; }
    bool slapps() const { return stage == 3; }
    uint64_t getOrdning() const { return ordning; }

    void tick (const DimmaParams& p, float& l, float& r)
    {
        if (! aktiv) return;
        const double sek = (double) t++ / sr;

        // --- Förstärkaren: mjuk attack, full håll, lång släpp.
        if (stage == 1) { amp += (1.02f - amp) * tidskonstant (std::max (0.005f, p.attack) * 0.4, sr); if (amp >= 1.0f) { amp = 1.0f; stage = 2; } }
        else if (stage == 3) { amp -= amp * tidskonstant (std::max (0.01f, p.slapp) * 0.3, sr); if (amp < 1.0e-4f) { aktiv = false; amp = 0.0f; return; } }

        // --- Rörelsen: ett långsamt svep per röst (0,07–0,35 Hz).
        lfoFas += (0.07 + 0.04 * (nummer % 7)) * (0.5 + p.rorelse) / sr; lfoFas -= std::floor (lfoFas);
        const double lfo = std::sin (twoPi * lfoFas);
        if ((t & 255) == 0) driftMal = (slump.bipolar()) * p.rorelse * 0.15;   // halvtoner
        drift += (driftMal - drift) * 0.0005;

        const double bas = noteToHz (ton + p.tonhojdOffset + (p.lage == DimmaLage::Dronmaskin ? drift : 0.0));
        const double sv = std::exp2 ((6.0 + 14.0 * p.bredd) / 1200.0);   // svävning, 6–20 cent
        double fc = 220.0 * std::exp2 (p.klang * 5.5);                     // 220 Hz .. 10 kHz
        fc *= std::exp2 (lfo * p.rorelse * 0.9);
        float x = 0.0f;

        switch (p.lage)
        {
            case DimmaLage::Syntmatta:
            {
                x = 0.32f * (saw (0, bas) + saw (1, bas * sv) + saw (2, bas / sv)) + 0.25f * square (3, bas * 0.5);
                const float oppna = 1.0f - (float) std::exp (-sek / std::max (0.05, (double) p.attack * 1.5));
                fc *= 0.45 + 0.55 * oppna;
                filter[0].set (std::min (fc, sr * 0.45), 0.9, sr);
                filter[0].tick (x);
                x = filter[0].lp;
                break;
            }
            case DimmaLage::Kor:
            {
                const double vib = 1.0 + 0.004 * (0.3 + p.rorelse) * std::sin (twoPi * (4.8 + 0.3 * (nummer % 3)) * sek);
                const float kalla = 0.5f * (saw (0, bas * vib) + saw (1, bas * vib * sv)) + 0.08f * slump.bipolar();
                // Formanter för a, o och i; vokalen glider mellan dem.
                static constexpr double F[3][3] { { 800.0, 1150.0, 2900.0 }, { 450.0, 800.0, 2830.0 }, { 300.0, 2200.0, 3000.0 } };
                static constexpr double G[3] { 1.0, 0.55, 0.35 };
                const double v = std::clamp ((double) p.vokal, 0.0, 1.0) * 2.0;
                const int a = std::min (1, (int) v);
                const double w = v - a;
                float y = 0.0f;
                for (int i = 0; i < 3; ++i)
                {
                    const double f = F[a][i] + (F[a + 1][i] - F[a][i]) * w;
                    filter[(size_t) i].set (std::min (f * (0.85 + 0.3 * p.klang), sr * 0.45), 6.0 + 4.0 * i, sr);
                    filter[(size_t) i].tick (kalla);
                    y += filter[(size_t) i].bp * (float) G[i];
                }
                x = y * 0.62f;
                break;
            }
            case DimmaLage::Dronmaskin:
            {
                const double kvot = 1.47 + 0.06 * p.rorelse * lfo;               // oharmonisk ringkvot
                ringFas += bas * kvot / sr; ringFas -= std::floor (ringFas);
                subFas += bas * 0.5 / sr; subFas -= std::floor (subFas);
                const float s = 0.5f * (saw (0, bas) + saw (1, bas * sv));
                const float ring = s * (float) std::sin (twoPi * ringFas);
                // Brus genom ett bandpass som sveper långsamt runt fyra gånger grundtonen.
                filter[1].set (std::min (bas * 4.0 * std::exp2 (lfo * 0.8), sr * 0.45), 8.0, sr);
                filter[1].tick (slump.bipolar());
                x = 0.45f * s + 0.45f * ring + 0.5f * (float) std::sin (twoPi * subFas) + 0.6f * filter[1].bp;
                filter[0].set (std::min (fc * 0.6, sr * 0.45), 1.2, sr);
                filter[0].tick (x);
                x = filter[0].lp;
                break;
            }
        }

        const float y = x * amp * (0.45f + 0.55f * vel) * 0.42f;
        // Stereobredd: rösterna sprids åt vänster och höger.
        const float pan = (nummer % 2 == 0 ? -1.0f : 1.0f) * (0.3f + 0.1f * (float) (nummer % 3)) * p.bredd;
        l += y * (1.0f - std::max (0.0f, pan));
        r += y * (1.0f + std::min (0.0f, pan));
    }

private:
    float saw (size_t i, double f)
    {
        const double dt = std::min (0.45, f / sr);
        const float v = (float) (2.0 * fas[i] - 1.0 - polyBlep (fas[i], dt));
        fas[i] += dt; fas[i] -= std::floor (fas[i]);
        return v;
    }
    float square (size_t i, double f)
    {
        const double dt = std::min (0.45, f / sr);
        double v = fas[i] < 0.5 ? 1.0 : -1.0;
        v += polyBlep (fas[i], dt);
        v -= polyBlep (std::fmod (fas[i] + 0.5, 1.0), dt);
        fas[i] += dt; fas[i] -= std::floor (fas[i]);
        return (float) v;
    }

    double sr = 48000.0;
    int nummer = 0, stage = 0;
    double ton = 60.0, lfoFas = 0.0, drift = 0.0, driftMal = 0.0, ringFas = 0.0, subFas = 0.0;
    float vel = 1.0f, amp = 0.0f;
    bool aktiv = false;
    int64_t t = 0;
    uint64_t ordning = 0;
    std::array<double, 4> fas {};
    std::array<Svf, 3> filter;
    Slump slump;
};

class Dimman
{
public:
    static constexpr int antalRoster = 12;

    void prepare (double s)
    {
        sr = s;
        for (int i = 0; i < antalRoster; ++i) roster[(size_t) i].prepare (s, i);
        for (auto& f : ensembleLinje) f.prepare ((int) (s * 0.03));
        reset();
    }
    void reset() { for (auto& r : roster) r.reset(); for (auto& f : ensembleLinje) f.reset(); ensFas = 0.0; }
    void setParams (const DimmaParams& p) { params = p; }
    void setStamning (const std::array<double, 128>* t) { tabell = t; }

    // En ton spelar hela ackordet (utom i drönläget Grundton, som motorn skickar med ackord 0).
    void noteOn (int ton, float vel, int ackord)
    {
        for (int halvtoner : ackordTabell()[(size_t) std::clamp (ackord, 0, 7)])
            if (halvtoner > -99) startaRost (ton + halvtoner, vel, ton);
    }
    void noteOn (int ton, float vel) { noteOn (ton, vel, params.ackord); }
    void noteOff (int ton)
    {
        for (size_t i = 0; i < roster.size(); ++i)
            if (roster[i].arAktiv() && grundTon[i] == ton) roster[i].slapp();
    }
    void allNotesOff() { for (auto& r : roster) r.slapp(); }

    void tick (float& l, float& r)
    {
        float dl = 0.0f, dr = 0.0f;
        for (auto& v : roster) v.tick (params, dl, dr);
        // Ensemblen: tre fördröjningar (5–15 ms) som svajar i 120° fas, vänster och höger spegelvända.
        ensembleLinje[0].skriv (dl);
        ensembleLinje[1].skriv (dr);
        const float m = std::clamp (params.ensemble, 0.0f, 1.0f);
        if (m > 0.001f)
        {
            ensFas += 0.62 / sr; ensFas -= std::floor (ensFas);
            float el = 0.0f, er = 0.0f;
            for (int i = 0; i < 3; ++i)
            {
                const double ph = ensFas + i / 3.0;
                const double ms = 9.0 + 4.0 * std::sin (twoPi * ph) + 0.6 * std::sin (twoPi * (ph * 11.3));
                el += ensembleLinje[0].las (ms * 1.0e-3 * sr);
                er += ensembleLinje[1].las ((18.0 - ms) * 1.0e-3 * sr);
            }
            dl = dl * (1.0f - 0.45f * m) + el * 0.33f * m;
            dr = dr * (1.0f - 0.45f * m) + er * 0.33f * m;
        }
        l = dl; r = dr;
    }

    bool haller() const { for (auto& r : roster) if (r.arAktiv() && ! r.slapps()) return true; return false; }

private:
    void startaRost (int ton, float vel, int grund)
    {
        const double tunad = tabell != nullptr ? (*tabell)[(size_t) std::clamp (ton, 0, 127)] : (double) ton;
        if (std::isnan (tunad)) return;
        int val = -1;
        for (int i = 0; i < antalRoster && val < 0; ++i) if (! roster[(size_t) i].arAktiv()) val = i;
        if (val < 0)
        {
            uint64_t aldst = UINT64_MAX;
            for (int pass = 0; pass < 2 && val < 0; ++pass)
                for (int i = 0; i < antalRoster; ++i)
                    if ((pass == 1 || roster[(size_t) i].slapps()) && roster[(size_t) i].getOrdning() < aldst)
                    { aldst = roster[(size_t) i].getOrdning(); val = i; }
        }
        roster[(size_t) val].starta (tunad, vel, ++raknare);
        grundTon[(size_t) val] = grund;
    }

    double sr = 48000.0, ensFas = 0.0;
    DimmaParams params;
    std::array<DimmaRost, antalRoster> roster;
    std::array<int, antalRoster> grundTon {};
    std::array<Fordrojning, 2> ensembleLinje;
    uint64_t raknare = 0;
    const std::array<double, 128>* tabell = nullptr;
};

} // namespace km
