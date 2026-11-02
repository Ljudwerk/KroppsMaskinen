// Hugget – stötar och leads. Polyfont, åtta röster.
//
// Sju lägen:
//   Stötar: Orkesterstöt (tjockt lager av sågtänder och fyrkanter i oktaver och kvinter, ett brusanslag och
//           en tonhöjd som sjunker), Blåsstöt (två sågtänder med ett filter som sväller), Körstöt (formanter
//           som ett "a") och Klockstöt (FM med ett metalliskt anslag).
//   Leads:  Synklead (synkad oscillator vars tonhöjd svepes av envelopen), Pulslead (puls med svävande
//           pulsbredd) och FM-lead.
//
// Ackordminnet: en ton spelar ett lagrat ackord (moll, dur, kvint, oktav …), så att en enda rad i
// sekvensern ger fulla stötar.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>

namespace km
{

enum class HuggLage { Orkester = 0, Blas, Kor, Klocka, Synklead, Pulslead, FmLead };
inline constexpr int antalHuggLagen = 7;

// Ackorden i halvtoner från tonen som spelas.
inline const std::array<std::array<int, 4>, 8>& ackordTabell()
{
    static const std::array<std::array<int, 4>, 8> t { {
        { 0, -99, -99, -99 },   // Ingen
        { 0, 3, 7, -99 },       // Moll
        { 0, 4, 7, -99 },       // Dur
        { 0, 7, 12, -99 },      // Kvint
        { 0, 12, -99, -99 },    // Oktav
        { 0, 3, 7, 10 },        // Moll 7
        { 0, 5, 7, -99 },       // Sus 4
        { 0, 3, 7, 14 },        // Moll 9
    } };
    return t;
}

struct HuggParams
{
    HuggLage lage = HuggLage::Orkester;
    int ackord = 1;            // index i ackordTabell
    float klang = 0.6f;        // 0..1: filter och ljushet
    float attack = 0.002f;     // sekunder
    float fall = 0.35f;        // sekunder
    float hall = 0.0f;         // 0..1
    float slapp = 0.15f;       // sekunder
    float bredd = 0.5f;        // 0..1: svävning och stereobredd
    float sankning = 0.4f;     // 0..1: hur mycket tonhöjden sjunker i anslaget (stötarna)
    float tonhojdOffset = 0.0f; // halvtoner (Varvtalet och modulation)
};

class HuggRost
{
public:
    void prepare (double s, int nr) { sr = s; nummer = nr; slump = Slump (0x48554747u + (uint32_t) nr * 101u); reset(); }
    void reset() { aktiv = false; amp = 0.0f; stage = 0; for (auto& f : filter) f.reset(); }

    void starta (int ton_, float vel_, uint64_t ordning_)
    {
        ton = ton_; vel = vel_; ordning = ordning_;
        t = 0; stage = 1; aktiv = true;
        for (auto& p : fas) p = slump.unipolar();
        fmFas = 0.0;
        for (auto& f : filter) f.reset();
    }

    void slapp() { if (aktiv && stage < 3) stage = 3; }

    bool arAktiv() const { return aktiv; }
    bool slapps() const { return stage == 3; }
    int getTon() const { return ton; }
    uint64_t getOrdning() const { return ordning; }

    // Ett sample, stereo (adderas till l och r).
    void tick (const HuggParams& p, float& l, float& r)
    {
        if (! aktiv) return;
        const double sek = (double) t++ / sr;

        // --- Förstärkarens envelope
        switch (stage)
        {
            case 1: amp += (1.05f - amp) * tidskonstant (std::max (0.0005f, p.attack) * 0.4, sr); if (amp >= 1.0f) { amp = 1.0f; stage = 2; } break;
            case 2: amp += (p.hall - amp) * tidskonstant (p.fall * 0.35, sr); break;
            case 3: amp -= amp * tidskonstant (std::max (0.003f, p.slapp) * 0.35, sr); if (amp < 1.0e-4f) { aktiv = false; return; } break;
            default: break;
        }
        // Utan håll dör stöten ut av sig själv.
        if (stage == 2 && p.hall <= 0.001f && amp < 1.0e-4f) { aktiv = false; return; }

        // --- Tonhöjd: stötarna sjunker i anslaget
        const bool stot = p.lage <= HuggLage::Klocka;
        const double fall = stot ? -p.sankning * 1.5 * (1.0 - std::exp (-sek / 0.12)) : 0.0;
        const double bas = noteToHz (ton + p.tonhojdOffset + fall);
        const double svav = 1.0 + 0.004 * p.bredd;
        const float filterEnv = (float) std::exp (-sek / 0.18);

        float x = 0.0f;
        double fc = 200.0 * std::exp2 (p.klang * 6.0);   // 200 Hz .. 12,8 kHz
        double q = 0.8;
        switch (p.lage)
        {
            case HuggLage::Orkester:
            {
                // Sågtänder i grundton, oktav och kvint, en fyrkant en oktav ned, och brus i anslaget.
                x += saw (0, bas * svav) + saw (1, bas / svav) + 0.7f * saw (2, bas * 2.0) + 0.5f * saw (3, bas * 3.0)
                   + 0.6f * square (4, bas * 0.5);
                x *= 0.3f;
                x += slump.bipolar() * 0.6f * (float) std::exp (-sek / 0.02);
                fc *= 0.6 + 2.5 * filterEnv;
                break;
            }
            case HuggLage::Blas:
            {
                x = 0.45f * (saw (0, bas * svav) + saw (1, bas / svav));
                const float svall = 1.0f - (float) std::exp (-sek / 0.05);   // filtret öppnar sig: blåsets ansats
                fc *= 0.3 + 1.2 * svall * (0.6 + 0.4 * filterEnv);
                break;
            }
            case HuggLage::Kor:
            {
                const double vib = 1.0 + 0.004 * std::sin (twoPi * 5.2 * sek) * std::min (1.0, sek * 3.0);
                const float kalla = 0.5f * (saw (0, bas * vib * svav) + saw (1, bas * vib / svav));
                // Formanterna för ett "a": 800, 1150 och 2900 Hz.
                filter[1].set (800.0, 8.0, sr);  filter[1].tick (kalla);
                filter[2].set (1150.0, 9.0, sr); filter[2].tick (kalla);
                filter[3].set (2900.0, 10.0, sr); filter[3].tick (kalla);
                x = (filter[1].bp + 0.6f * filter[2].bp + 0.3f * filter[3].bp) * 1.0f;
                break;
            }
            case HuggLage::Klocka:
            {
                const double index = (1.0 + 4.0 * p.klang) * std::exp (-sek / 0.25);
                fmFas += bas * 3.5 / sr; fmFas -= std::floor (fmFas);
                fas[0] += bas / sr; fas[0] -= std::floor (fas[0]);
                x = (float) std::sin (twoPi * fas[0] + index * std::sin (twoPi * fmFas)) * 0.6f;
                fc = 16000.0;
                break;
            }
            case HuggLage::Synklead:
            {
                // Huvudoscillatorn är osc 1; osc 2 synkas till den och svepes upp av envelopen.
                const double slavKvot = std::exp2 ((3.0 + 24.0 * p.klang * filterEnv) / 12.0);
                const double dt = bas / sr;
                fas[0] += dt;
                if (fas[0] >= 1.0) { fas[0] -= 1.0; fas[1] = fas[0] * slavKvot; }
                fas[1] += dt * slavKvot; fas[1] -= std::floor (fas[1]);
                x = 0.55f * (float) (2.0 * fas[1] - 1.0) + 0.25f * saw (2, bas * svav);
                fc = 1500.0 + 9000.0 * p.klang;
                break;
            }
            case HuggLage::Pulslead:
            {
                const double pw = 0.5 - 0.4 * (0.5 + 0.5 * std::sin (twoPi * 0.7 * sek + nummer));
                fas[0] += bas / sr; fas[0] -= std::floor (fas[0]);
                x = 0.5f * (fas[0] < pw ? 1.0f : -1.0f) + 0.3f * saw (1, bas * svav);
                fc *= 0.5 + 1.0 * filterEnv;
                q = 1.6;
                break;
            }
            case HuggLage::FmLead:
            {
                const double index = 0.5 + 3.0 * p.klang * (0.4 + 0.6 * filterEnv);
                fmFas += bas * 2.0 / sr; fmFas -= std::floor (fmFas);
                fas[0] += bas / sr; fas[0] -= std::floor (fas[0]);
                x = (float) std::sin (twoPi * fas[0] + index * std::sin (twoPi * fmFas)) * 0.6f;
                fc = 14000.0;
                break;
            }
        }

        filter[0].set (std::min (fc, sr * 0.45), q, sr);
        filter[0].tick (x);
        const float y = filter[0].lp * amp * (0.35f + 0.65f * vel) * 0.5f;

        // Stereobredd: varannan röst åt vänster, varannan åt höger.
        const float pan = (nummer % 2 == 0 ? -1.0f : 1.0f) * p.bredd * 0.6f;
        l += y * (1.0f - std::max (0.0f, pan));
        r += y * (1.0f + std::min (0.0f, pan));
    }

private:
    // Bandbegränsad sågtand och fyrkant med egna faser.
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
    int nummer = 0, ton = 60, stage = 0;
    float vel = 1.0f, amp = 0.0f;
    bool aktiv = false;
    int64_t t = 0;
    uint64_t ordning = 0;
    std::array<double, 5> fas {};
    double fmFas = 0.0;
    std::array<Svf, 4> filter;
    Slump slump;
};

class Hugget
{
public:
    static constexpr int antalRoster = 8;

    void prepare (double sr) { for (int i = 0; i < antalRoster; ++i) roster[(size_t) i].prepare (sr, i); reset(); }
    void reset() { for (auto& r : roster) r.reset(); raknare = 0; }
    void setParams (const HuggParams& p) { params = p; }

    // En ton (från sekvensern eller keyboardet) spelar hela ackordet.
    void noteOn (int ton, float vel)
    {
        for (int halvtoner : ackordTabell()[(size_t) std::clamp (params.ackord, 0, 7)])
            if (halvtoner > -99) startaRost (ton + halvtoner, vel, ton);
    }

    void noteOff (int ton)
    {
        for (auto& r : roster)
            if (r.arAktiv() && grundTon[(size_t) (&r - roster.data())] == ton) r.slapp();
    }

    void allNotesOff() { for (auto& r : roster) r.slapp(); }
    bool haller() const { for (auto& r : roster) if (r.arAktiv() && ! r.slapps()) return true; return false; }

    void tick (float& l, float& r)
    {
        l = r = 0.0f;
        for (auto& v : roster) v.tick (params, l, r);
    }

private:
    void startaRost (int ton, float vel, int grund)
    {
        // Ledig röst, annars den som släpps sedan längst, annars den äldsta.
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
        roster[(size_t) val].starta (ton, vel, ++raknare);
        grundTon[(size_t) val] = grund;
    }

    HuggParams params;
    std::array<HuggRost, antalRoster> roster;
    std::array<int, antalRoster> grundTon {};
    uint64_t raknare = 0;
};

} // namespace km
