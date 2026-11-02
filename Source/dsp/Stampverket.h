// Stampverket – trummorna (fas 0: bastrumma, virvel och stängd hi-hat).
// Alla ljud är syntetiska. Varje ljud har Stämning (halvtoner), Klang (hur länge det klingar) och Nivå.
// Spåren har fasta platser i mönstret, även de som inte är byggda ännu, så att mönsterformatet håller
// när resten av de tolv ljuden kommer i fas 1.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>

namespace km
{

// Spårens ordning i mönstret. Får aldrig ändras när ett bygge är släppt.
enum class Trumma
{
    Bastrumma = 0, Virvel, Virvel2, Handklapp, StangdHihat, OppenHihat,
    Cymbal, TomLag, TomHog, Metall, Small, Kubjallra
};
inline constexpr int antalSpar = 12;

// Ljuden som finns i fas 0, i den ordning de visas och har parametrar.
inline constexpr std::array<Trumma, 3> byggdaTrummor { Trumma::Bastrumma, Trumma::Virvel, Trumma::StangdHihat };
inline constexpr int antalByggda = (int) byggdaTrummor.size();

inline const char* trumNamn (Trumma t)
{
    switch (t)
    {
        case Trumma::Bastrumma:   return "Bastrumma";
        case Trumma::Virvel:      return "Virvel";
        case Trumma::Virvel2:     return "Virvel 2";
        case Trumma::Handklapp:   return "Handklapp";
        case Trumma::StangdHihat: return "Hi-hat";
        case Trumma::OppenHihat:  return "Öppen hi-hat";
        case Trumma::Cymbal:      return "Cymbal";
        case Trumma::TomLag:      return "Tom låg";
        case Trumma::TomHog:      return "Tom hög";
        case Trumma::Metall:      return "Metall";
        case Trumma::Small:       return "Smäll";
        case Trumma::Kubjallra:   return "Kubjällra";
    }
    return "";
}

struct TrumParams
{
    float stamning = 0.0f; // halvtoner −12..+12
    float klang    = 0.5f; // 0..1
    float niva     = 0.8f; // 0..1
};

struct StampverketParams
{
    std::array<TrumParams, antalByggda> trummor {};
};

// Bastrumman: sinus med tonhöjdssvep, ett kort klick och mättnad.
class Bastrumma
{
public:
    void prepare (double s) { sr = s; }
    void trigga (float vel) { t = 0; ph = 0.0; amp = vel; aktiv = true; }

    float tick (const TrumParams& p)
    {
        if (! aktiv) return 0.0f;
        const double sek = (double) t++ / sr;
        const double bas = 48.0 * std::exp2 (p.stamning / 12.0);
        const double svep = std::exp (-sek / 0.022);
        const double f = bas * (1.0 + 3.2 * svep);
        ph += f / sr; if (ph >= 1.0) ph -= 1.0;

        const double fallTid = 0.08 + 0.9 * p.klang * p.klang;
        const float env = (float) std::exp (-sek / (fallTid * 0.45));
        const float klick = sek < 0.003 ? slump.bipolar() * (float) (1.0 - sek / 0.003) * 0.35f : 0.0f;
        const float x = (float) std::sin (twoPi * ph) * env + klick;
        if (env < 1.0e-4f) aktiv = false;
        return mjukKlipp (x * 1.6f) * amp * p.niva;
    }

private:
    double sr = 48000.0, ph = 0.0;
    int64_t t = 0;
    float amp = 1.0f;
    bool aktiv = false;
    Slump slump { 0x42415354u };
};

// Virveln: två toner som ger kroppen och brus genom ett högpass som ger snärjet.
class Virvel
{
public:
    void prepare (double s) { sr = s; hp.reset(); }
    void trigga (float vel) { t = 0; ph1 = ph2 = 0.0; amp = vel; aktiv = true; }

    float tick (const TrumParams& p)
    {
        if (! aktiv) return 0.0f;
        const double sek = (double) t++ / sr;
        const double skala = std::exp2 (p.stamning / 12.0);
        const double svep = 1.0 + 0.5 * std::exp (-sek / 0.008);
        ph1 += 185.0 * skala * svep / sr; if (ph1 >= 1.0) ph1 -= 1.0;
        ph2 += 330.0 * skala * svep / sr; if (ph2 >= 1.0) ph2 -= 1.0;

        const float kropp = (float) ((std::sin (twoPi * ph1) + 0.6 * std::sin (twoPi * ph2)) * std::exp (-sek / 0.045));
        const double brusTid = 0.05 + 0.35 * p.klang;
        const float brusEnv = (float) std::exp (-sek / (brusTid * 0.4));
        hp.set (1600.0 * skala, 0.8, sr);
        hp.tick (slump.bipolar());
        const float snarj = (hp.hp * 0.8f + hp.bp * 0.5f) * brusEnv;

        if (brusEnv < 1.0e-4f) aktiv = false;
        return mjukKlipp ((0.7f * kropp + 0.9f * snarj) * 1.3f) * amp * p.niva;
    }

private:
    double sr = 48000.0, ph1 = 0.0, ph2 = 0.0;
    int64_t t = 0;
    float amp = 1.0f;
    bool aktiv = false;
    Svf hp;
    Slump slump { 0x56495256u };
};

// Hi-hatten: sex fyrkanter i metalliska förhållanden genom bandpass och högpass.
class Hihat
{
public:
    void prepare (double s) { sr = s; bp.reset(); hp.reset(); }
    void trigga (float vel) { t = 0; amp = vel; aktiv = true; }

    float tick (const TrumParams& p)
    {
        if (! aktiv) return 0.0f;
        static constexpr std::array<double, 6> frekvenser { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
        const double skala = std::exp2 (p.stamning / 12.0);
        float metall = 0.0f;
        for (size_t i = 0; i < frekvenser.size(); ++i)
        {
            ph[i] += frekvenser[i] * skala * 1.7 / sr;
            if (ph[i] >= 1.0) ph[i] -= 1.0;
            metall += ph[i] < 0.5 ? 1.0f : -1.0f;
        }
        metall = metall / 6.0f + slump.bipolar() * 0.15f;

        bp.set (10000.0 * skala, 1.2, sr);
        hp.set (7000.0 * skala, 0.7, sr);
        bp.tick (metall);
        hp.tick (bp.bp);

        const double sek = (double) t++ / sr;
        const double fallTid = 0.018 + 0.25 * p.klang * p.klang;
        const float env = (float) std::exp (-sek / (fallTid * 0.45));
        if (env < 1.0e-4f) aktiv = false;
        return hp.hp * env * 2.2f * amp * p.niva;
    }

    void choke() { aktiv = false; }

private:
    double sr = 48000.0;
    std::array<double, 6> ph { 0.0, 0.13, 0.27, 0.41, 0.58, 0.77 };
    int64_t t = 0;
    float amp = 1.0f;
    bool aktiv = false;
    Svf bp, hp;
    Slump slump { 0x48494841u };
};

class Stampverket
{
public:
    void prepare (double sr)
    {
        bastrumma.prepare (sr);
        virvel.prepare (sr);
        hihat.prepare (sr);
    }

    void setParams (const StampverketParams& p) { params = p; }

    // Slår an ett spår (velocity 0..1). Spår som inte är byggda ännu är tysta.
    void slag (Trumma t, float velocity)
    {
        if (t == Trumma::Bastrumma)        bastrumma.trigga (velocity);
        else if (t == Trumma::Virvel)      virvel.trigga (velocity);
        else if (t == Trumma::StangdHihat) hihat.trigga (velocity);
    }

    float tick()
    {
        return bastrumma.tick (params.trummor[0])
             + virvel.tick (params.trummor[1])
             + hihat.tick (params.trummor[2]) * 0.8f;
    }

    // MIDI-noter för spåren (GM-trumkarta där det går), i spårens ordning.
    static int notForSpar (int spar)
    {
        static constexpr std::array<int, antalSpar> noter { 36, 38, 40, 39, 42, 46, 49, 41, 48, 56, 54, 75 };
        return noter[(size_t) std::clamp (spar, 0, antalSpar - 1)];
    }
    static int sparForNot (int note)
    {
        for (int i = 0; i < antalSpar; ++i)
            if (notForSpar (i) == note) return i;
        return -1;
    }

private:
    StampverketParams params;
    Bastrumma bastrumma;
    Virvel virvel;
    Hihat hihat;
};

} // namespace km
