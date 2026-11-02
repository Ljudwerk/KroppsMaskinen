// Effekterna: Grindrummet (gatad reverb), Ekot (synkat eko), Svajet (chorus) och Klämman (busskompressor).
//
// Grindrummet och Ekot matas från kanalernas sändningar A och B och blandas tillbaka i mastern.
// Svajet och Klämman sitter på mastern, i den ordningen. Klämman kan pressas av bastrumman (sidokedja),
// så att resten av maskinen pumpar i takt.
//
// Allt räknas sample för sample, i stereo.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>
#include <vector>

namespace km
{

// En enkel fördröjningslinje med linjär interpolation.
class Fordrojning
{
public:
    void prepare (int maxSampel) { buf.assign ((size_t) std::max (4, maxSampel + 2), 0.0f); pos = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    void skriv (float x) { buf[(size_t) pos] = x; pos = (pos + 1) % (int) buf.size(); }
    // Läser 'sampel' bakåt från senast skrivna (minst 1).
    float las (double sampel) const
    {
        const int n = (int) buf.size();
        sampel = std::clamp (sampel, 1.0, (double) n - 2.0);
        const int i = (int) sampel;
        const float f = (float) (sampel - i);
        const float a = buf[(size_t) ((pos - i + 2 * n) % n)];
        const float b = buf[(size_t) ((pos - i - 1 + 2 * n) % n)];
        return a + (b - a) * f;
    }

private:
    std::vector<float> buf;
    int pos = 0;
};

// ---------------------------------------------------------------------------------------------
// Grindrummet: ett FDN-rum med fyra linjer, och en grind som öppnas av ljudet som skickas in och stängs
// tvärt efter hålltiden – 80-talets gatade virvel.

struct GrindrumParams
{
    float storlek = 0.6f;    // 0..1
    float hallTid = 0.25f;   // sekunder grinden står öppen efter att ljudet tystnat
    float ton = 0.6f;        // 0 mörkt .. 1 ljust
    float niva = 0.7f;       // returnivå 0..1
};

class Grindrummet
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& l : linjer) l.prepare ((int) (sr * 0.12));
        foljKoef = tidskonstant (0.005, sr);
        hpKoef = (float) (1.0 - std::exp (-twoPi * 300.0 / sr));
        stangKoef = tidskonstant (0.012, sr);
        reset();
    }

    void reset()
    {
        for (auto& l : linjer) l.reset();
        lp.fill (0.0f);
        folj = 0.0f; grind = 0.0f; oppenKvar = 0; hpIn = 0.0f;
    }

    void setParams (const GrindrumParams& p) { params = p; }

    void tick (float inL, float inR, float& utL, float& utR)
    {
        static constexpr std::array<double, 4> ms { 29.7, 37.1, 41.1, 43.7 };
        // Högpass på insignalen (~300 Hz), så att bastrumman inte mullrar i rummet.
        const float ra = (inL + inR) * 0.5f;
        hpIn += (ra - hpIn) * hpKoef;
        const float in = ra - hpIn;

        // Grinden: följer insignalen; öppen så länge den hörs, plus hålltiden.
        const float a = std::abs (in);
        folj += (a - folj) * (a > folj ? 1.0f : foljKoef);
        if (folj > 0.02f) oppenKvar = (int) (params.hallTid * sr);
        else if (oppenKvar > 0) --oppenKvar;
        const float mal = oppenKvar > 0 || folj > 0.02f ? 1.0f : 0.0f;
        grind += (mal - grind) * (mal > grind ? 0.2f : stangKoef);

        // Rummet: fyra linjer, Householder-matris, dämpning i återkopplingen.
        const double skala = 0.5 + params.storlek * 1.3;
        const float g = 0.72f + 0.25f * params.storlek;
        const float damp = 0.15f + 0.8f * (1.0f - params.ton);
        std::array<float, 4> y {};
        for (size_t i = 0; i < 4; ++i)
        {
            y[i] = linjer[i].las (ms[i] * 0.001 * sr * skala);
            lp[i] += (y[i] - lp[i]) * (1.0f - damp);
            y[i] = lp[i];
        }
        const float s = (y[0] + y[1] + y[2] + y[3]) * 0.5f;
        for (size_t i = 0; i < 4; ++i)
            linjer[i].skriv (in + g * (y[i] - s));

        utL = (y[0] - y[2]) * grind * params.niva * 1.4f;
        utR = (y[1] - y[3]) * grind * params.niva * 1.4f;
    }

private:
    double sr = 48000.0;
    GrindrumParams params;
    std::array<Fordrojning, 4> linjer;
    std::array<float, 4> lp {};
    float folj = 0.0f, grind = 0.0f, foljKoef = 0.01f, stangKoef = 0.01f, hpIn = 0.0f, hpKoef = 0.04f;
    int oppenKvar = 0;
};

// ---------------------------------------------------------------------------------------------
// Ekot: synkat eko i BBD- eller bandläge, med ping-pong.

enum class EkoLage { Bbd = 0, Band };
inline constexpr std::array<double, 6> ekoTider { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };   // i sextondelar×… se ekoTidNamn
inline const char* ekoTidNamn (int i)
{
    static const char* n[] = { "1/16", "1/8", "3/16", "1/4", "3/8", "1/2" };
    return n[std::clamp (i, 0, 5)];
}

struct EkoParams
{
    int tid = 2;              // index i ekoTider (fjärdedelar: 1/16 = 0,25 osv.)
    float aterkoppling = 0.4f;
    float ton = 0.5f;         // 0 mörkt .. 1 ljust
    bool pingPong = true;
    EkoLage lage = EkoLage::Bbd;
    float niva = 0.5f;        // returnivå
};

class Ekot
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& l : linjer) l.prepare ((int) (sr * 2.6));
        tidKoef = tidskonstant (0.08, sr);
        reset();
    }

    void reset()
    {
        for (auto& l : linjer) l.reset();
        lp = { 0.0f, 0.0f }; hp = { 0.0f, 0.0f };
        tidNu = 0.0; fas = 0.0;
    }

    void setParams (const EkoParams& p, double bpm) { params = p; tempo = std::clamp (bpm, 20.0, 400.0); }

    void tick (float inL, float inR, float& utL, float& utR)
    {
        const double mal = ekoTider[(size_t) std::clamp (params.tid, 0, 5)] * 60.0 / tempo * sr;
        if (tidNu <= 0.0) tidNu = mal;
        tidNu += (mal - tidNu) * tidKoef;   // mjuk ändring: låter som ett band som byter fart

        // Lite rörelse i fördröjningen: BBD-klockans fladder eller bandets wow.
        fas += (params.lage == EkoLage::Band ? 0.6 : 3.1) / sr;
        if (fas >= 1.0) fas -= 1.0;
        const double djup = params.lage == EkoLage::Band ? 0.0025 : 0.0006;
        const double d = tidNu * (1.0 + djup * std::sin (twoPi * fas));

        const float a = linjer[0].las (d), b = linjer[1].las (d);
        // Tonen i återkopplingen: lågpass (BBD mörkare) och lite högpass så att basen inte byggs upp.
        const float lpKoef = (params.lage == EkoLage::Bbd ? 0.08f : 0.15f) + 0.75f * params.ton;
        lp[0] += (a - lp[0]) * lpKoef; lp[1] += (b - lp[1]) * lpKoef;
        hp[0] += (lp[0] - hp[0]) * 0.01f; hp[1] += (lp[1] - hp[1]) * 0.01f;
        float ea = lp[0] - hp[0], eb = lp[1] - hp[1];
        const float drive = params.lage == EkoLage::Band ? 1.6f : 1.2f;
        ea = mjukKlipp (ea * drive) / drive; eb = mjukKlipp (eb * drive) / drive;

        const float fb = std::clamp (params.aterkoppling, 0.0f, 0.95f);
        const float in = (inL + inR) * 0.5f;
        if (params.pingPong)
        {
            linjer[0].skriv (in + fb * eb);   // vänster matas av höger: ekot studsar
            linjer[1].skriv (fb * ea);
        }
        else
        {
            linjer[0].skriv (inL + fb * ea);
            linjer[1].skriv (inR + fb * eb);
        }
        utL = ea * params.niva;
        utR = eb * params.niva;
    }

private:
    double sr = 48000.0, tempo = 125.0, tidNu = 0.0, fas = 0.0, tidKoef = 0.001;
    EkoParams params;
    std::array<Fordrojning, 2> linjer;
    std::array<float, 2> lp {}, hp {};
};

// ---------------------------------------------------------------------------------------------
// Svajet: chorus som i de gamla polysyntarna – läge I (långsamt), II (snabbare) och I+II (snabbt och grunt).

struct SvajParams
{
    int lage = 0;        // 0 av, 1 = I, 2 = II, 3 = I+II
    float mangd = 0.5f;  // 0..1 (våt andel)
};

class Svajet
{
public:
    void prepare (double s) { sr = s; for (auto& l : linjer) l.prepare ((int) (sr * 0.012)); reset(); }
    void reset() { for (auto& l : linjer) l.reset(); fas = 0.0; }
    void setParams (const SvajParams& p) { params = p; }

    void tick (float& l, float& r)
    {
        const float inL = l, inR = r;
        linjer[0].skriv (inL);
        linjer[1].skriv (inR);
        if (params.lage == 0) return;
        static constexpr double hz[] = { 0.0, 0.513, 0.863, 9.75 };
        static constexpr double djupMs[] = { 0.0, 1.85, 1.85, 0.3 };
        const auto i = (size_t) std::clamp (params.lage, 0, 3);
        fas += hz[i] / sr; if (fas >= 1.0) fas -= 1.0;
        const double tri = fas < 0.5 ? 4.0 * fas - 1.0 : 3.0 - 4.0 * fas;   // −1..1
        const double mitt = 3.5e-3 * sr, djup = djupMs[i] * 1e-3 * sr;
        const float vL = linjer[0].las (mitt + djup * tri);
        const float vR = linjer[1].las (mitt - djup * tri);   // motsatt fas i höger: bredd
        const float m = std::clamp (params.mangd, 0.0f, 1.0f);
        l = inL * (1.0f - 0.5f * m) + vL * m * 0.7f;
        r = inR * (1.0f - 0.5f * m) + vR * m * 0.7f;
    }

private:
    double sr = 48000.0, fas = 0.0;
    SvajParams params;
    std::array<Fordrojning, 2> linjer;
};

// ---------------------------------------------------------------------------------------------
// Klämman: busskompressor med stereolänkad toppdetektor, och "Pump": bastrumman trycker ned allt annat.

struct KlamParams
{
    float troskelDb = -12.0f;
    float kvot = 4.0f;
    float attack = 0.01f;    // sekunder
    float slapp = 0.15f;     // sekunder
    float pump = 0.0f;       // 0..1: hur mycket bastrumman trycker ned (sidokedja)
    float efterDb = 0.0f;    // efterförstärkning
};

class Klamman
{
public:
    void prepare (double s) { sr = s; reset(); }
    void reset() { env = 0.0f; pumpEnv = 0.0f; reduktionDb = 0.0f; }
    void setParams (const KlamParams& p)
    {
        params = p;
        attackKoef = tidskonstant (p.attack, sr);
        slappKoef = tidskonstant (p.slapp, sr);
        pumpSlapp = tidskonstant (std::max (0.05f, p.slapp * 1.5f), sr);
    }

    // sidokedja: bastrummans signal (före mixern).
    void tick (float& l, float& r, float sidokedja)
    {
        const float niva = std::max (std::abs (l), std::abs (r));
        env += (niva - env) * (niva > env ? attackKoef : slappKoef);
        const float envDb = tillDb (env);
        float red = 0.0f;
        if (envDb > params.troskelDb) red = (envDb - params.troskelDb) * (1.0f - 1.0f / std::max (1.0f, params.kvot));

        // Pumpen: en egen envelope på bastrumman, som trycker ned upp till 12 dB.
        const float s = std::abs (sidokedja);
        pumpEnv += (s - pumpEnv) * (s > pumpEnv ? 0.3f : pumpSlapp);
        red += params.pump * 12.0f * std::min (1.0f, pumpEnv * 3.0f);

        reduktionDb = red;
        const float g = std::exp2 ((params.efterDb - red) / 6.0206f);
        l *= g; r *= g;
    }

    float getReduktionDb() const { return reduktionDb; }

private:
    static float tillDb (float g) { return g > 1.0e-6f ? 20.0f * std::log10 (g) : -120.0f; }

    double sr = 48000.0;
    KlamParams params;
    float env = 0.0f, pumpEnv = 0.0f, reduktionDb = 0.0f;
    float attackKoef = 0.01f, slappKoef = 0.001f, pumpSlapp = 0.001f;
};

} // namespace km
