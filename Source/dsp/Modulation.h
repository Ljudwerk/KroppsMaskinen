// Modulationen – två LFO:er, två envelopes, modhjul, aftertouch och anslag, och sex kopplingar där vilken
// källa som helst kan styra vilket mål som helst.
//
// LFO:erna kan gå fritt eller i takt med Taktverket (4 takter … 1/16). Envelopes startas av en vald kanal:
// Kolvbasen, Kolvbas 2, Hugget, bastrumman eller alla toner.
//
// Modulationen räknas om var 32:a sample och läggs på motorns parametrar, så att allt den gör hörs som
// vanliga inställningar som rör sig.
//
// Utgår från Modulationsraden i KrangBox, med nya mål och utlösare.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>

namespace km
{

enum class LfoForm { Sinus = 0, Triangel, Fyrkant, Sagtand, Slump, Glid };
enum class ModKalla { Ingen = 0, Lfo1, Lfo2, Env1, Env2, Modhjul, Aftertouch, Anslag };
enum class ModMal
{
    Inget = 0, KolvbasenFilter, Kolvbas2Filter, HuggetKlang, KolvbasenPuls, HuggetTonhojd,
    Drivet, Grindrummet, Ekot, Spanningen
};
enum class EnvUtlosare { Kolvbasen = 0, Kolvbas2, Hugget, Bastrumma, Alla };

inline constexpr int antalKallor = 8, antalMal = 10, antalKopplingar = 6;

struct LfoParams
{
    LfoForm form = LfoForm::Sinus;
    float hz = 2.0f;        // i fritt läge
    int synk = 0;           // 0 = fri, 1 = 4 takter, 2 = 2 takter, 3 = 1 takt, 4 = 1/2, 5 = 1/4, 6 = 1/8, 7 = 1/16
};

struct ModEnvParams
{
    float attack = 0.005f, fall = 0.3f, hall = 0.0f, slapp = 0.2f;
    EnvUtlosare utlosare = EnvUtlosare::Kolvbasen;
};

struct ModKoppling { ModKalla kalla = ModKalla::Ingen; ModMal mal = ModMal::Inget; float mangd = 0.0f; };

struct ModParams
{
    std::array<LfoParams, 2> lfo;
    std::array<ModEnvParams, 2> env;
    std::array<ModKoppling, antalKopplingar> kopplingar;
};

class Modulation
{
public:
    static constexpr int kontrollTakt = 32;

    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        fas.fill (0.0);
        varv = { -1, -1 };
        haller.fill (0.0f); mal.fill (0.0f);
        envNiva.fill (0.0f); envSteg = { 0, 0 };
        varden.fill (0.0f);
        gate.fill (false);
    }

    void setParams (const ModParams& p) { params = p; }

    // En ton startade (eller slutade) i en kanal; envelopes med den utlösaren startar om.
    void tonPa (EnvUtlosare kalla, float anslag)
    {
        for (size_t i = 0; i < 2; ++i)
            if (params.env[i].utlosare == kalla || params.env[i].utlosare == EnvUtlosare::Alla)
            {
                envSteg[i] = 1;
                gate[i] = true;
            }
        if (kalla != EnvUtlosare::Bastrumma) hastighet = anslag;
    }
    void tonAv (EnvUtlosare kalla)
    {
        for (size_t i = 0; i < 2; ++i)
            if (params.env[i].utlosare == kalla || params.env[i].utlosare == EnvUtlosare::Alla)
                gate[i] = false;
    }

    void setModhjul (float v) { hjul = v; }
    void setAftertouch (float v) { tryck = v; }

    // Stegar fram en kontrollperiod. ppq < 0: ingen takt (LFO:er i takt går då fritt i tempot).
    void stega (int sampel, double ppq, double bpm)
    {
        static constexpr double fjardedelar[8] = { 0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25 };
        for (size_t i = 0; i < 2; ++i)
        {
            const auto& lp = params.lfo[i];
            if (lp.synk > 0)
            {
                const double langd = fjardedelar[std::clamp (lp.synk, 1, 7)];
                if (ppq >= 0.0) fas[i] = ppq / langd;
                else fas[i] += (double) sampel * bpm / 60.0 / sr / langd;
            }
            else fas[i] += (double) sampel * (double) lp.hz / sr;

            const double frac = fas[i] - std::floor (fas[i]);
            const auto nytt = (long long) std::floor (fas[i]);
            if (nytt != varv[i])
            {
                varv[i] = nytt;
                haller[i] = mal[i];
                mal[i] = slump.bipolar();
            }
            float v = 0.0f;
            switch (lp.form)
            {
                case LfoForm::Sinus:    v = (float) std::sin (twoPi * frac); break;
                case LfoForm::Triangel: v = (float) (frac < 0.5 ? 4.0 * frac - 1.0 : 3.0 - 4.0 * frac); break;
                case LfoForm::Fyrkant:  v = frac < 0.5 ? 1.0f : -1.0f; break;
                case LfoForm::Sagtand:  v = (float) (1.0 - 2.0 * frac); break;
                case LfoForm::Slump:    v = mal[i]; break;
                case LfoForm::Glid:     v = haller[i] + (mal[i] - haller[i]) * (float) (0.5 - 0.5 * std::cos (pi * frac)); break;
            }
            varden[(size_t) ModKalla::Lfo1 + i] = v;
        }

        // Envelopes (exponentiella, en gång per kontrollperiod).
        for (size_t i = 0; i < 2; ++i)
        {
            const auto& e = params.env[i];
            float& niva = envNiva[i];
            const double dt = (double) sampel;
            auto koef = [&] (float tid) { return (float) (1.0 - std::exp (-dt / (std::max (0.001, (double) tid * 0.35) * sr))); };
            if (envSteg[i] == 1) { niva += (1.05f - niva) * koef (e.attack); if (niva >= 1.0f) { niva = 1.0f; envSteg[i] = 2; } }
            else if (envSteg[i] == 2) { niva += (e.hall - niva) * koef (e.fall); if (! gate[i]) envSteg[i] = 3; }
            else if (envSteg[i] == 3) niva -= niva * koef (e.slapp);
            if (envSteg[i] == 2 && ! gate[i] && e.hall <= 0.0f) envSteg[i] = 3;
            varden[(size_t) ModKalla::Env1 + i] = niva;
        }
        varden[(size_t) ModKalla::Modhjul] = hjul;
        varden[(size_t) ModKalla::Aftertouch] = tryck;
        varden[(size_t) ModKalla::Anslag] = hastighet;
    }

    float kallVarde (ModKalla k) const { return varden[(size_t) k]; }

    // Summan av alla kopplingar till ett mål, ungefär −1..1.
    float malVarde (ModMal m) const
    {
        float s = 0.0f;
        for (const auto& k : params.kopplingar)
            if (k.mal == m && k.kalla != ModKalla::Ingen) s += varden[(size_t) k.kalla] * k.mangd;
        return std::clamp (s, -2.0f, 2.0f);
    }

    bool aktiv() const
    {
        for (const auto& k : params.kopplingar)
            if (k.kalla != ModKalla::Ingen && k.mal != ModMal::Inget && std::abs (k.mangd) > 0.0005f) return true;
        return false;
    }

private:
    double sr = 48000.0;
    ModParams params;
    std::array<double, 2> fas {};
    std::array<long long, 2> varv { -1, -1 };
    std::array<float, 2> haller {}, mal {}, envNiva {};
    std::array<int, 2> envSteg {};
    std::array<bool, 2> gate {};
    std::array<float, antalKallor> varden {};
    float hjul = 0.0f, tryck = 0.0f, hastighet = 0.0f;
    Slump slump { 0x4d4f4455u };
};

} // namespace km
