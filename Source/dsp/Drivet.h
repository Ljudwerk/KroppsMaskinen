// Drivet – distorsionen som sitter på varje kanal.
// Tre lägen: Rör (asymmetrisk tanh, jämna övertoner), Overdrive (hårdare, diodliknande klippning) och
// Bitkross (färre bitar och lägre samplingsfrekvens). Rör och Overdrive körs 2x översamplat med ett
// halvbandsfilter; Bitkross körs avsiktligt utan – där är aliasingen ljudet.
// Nivån kompenseras automatiskt (in- och utnivån mäts över ~300 ms), så att mängden förstör ljudet
// utan att bara höja volymen.
//
// Utgår från Krangdistortionen i KrangBox, omskriven för att köras sample för sample.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>

namespace km
{

enum class DrivLage { Av = 0, Ror, Overdrive, Bitkross };

struct DrivParams
{
    DrivLage lage = DrivLage::Av;
    float mangd = 0.3f;   // 0..1
};

// Halvbandsfilter för 2x upp- och nedsampling (fönstrad sinc, 31 tappar).
class Halvband
{
public:
    static constexpr int tappar = 31;

    Halvband()
    {
        constexpr int mitt = tappar / 2;
        double summa = 0.0;
        for (int i = 0; i < tappar; ++i)
        {
            const int n = i - mitt;
            const double sinc = n == 0 ? 0.5 : std::sin (pi * n * 0.5) / (pi * n);
            const double fonster = 0.42 - 0.5 * std::cos (twoPi * i / (tappar - 1)) + 0.08 * std::cos (2.0 * twoPi * i / (tappar - 1));
            h[(size_t) i] = (float) (sinc * fonster);
            summa += h[(size_t) i];
        }
        for (auto& c : h) c = (float) (c / summa);
    }

    void reset() { upp.fill (0.0f); ned.fill (0.0f); ui = ni = 0; }

    // Ett sample in, två ut (i dubbel takt).
    void uppsampla (float x, float& a, float& b)
    {
        skriv (upp, ui, x * 2.0f); a = falta (upp, ui);
        skriv (upp, ui, 0.0f);     b = falta (upp, ui);
    }

    // Två sample in (i dubbel takt), ett ut.
    float nedsampla (float a, float b)
    {
        skriv (ned, ni, a);
        skriv (ned, ni, b);
        return falta (ned, ni);
    }

private:
    static void skriv (std::array<float, tappar>& buf, int& pos, float x) { buf[(size_t) pos] = x; pos = (pos + 1) % tappar; }
    float falta (const std::array<float, tappar>& buf, int pos) const
    {
        float s = 0.0f;
        for (int i = 0; i < tappar; ++i)
            s += h[(size_t) i] * buf[(size_t) ((pos + i) % tappar)];
        return s;
    }

    std::array<float, tappar> h {}, upp {}, ned {};
    int ui = 0, ni = 0;
};

class Drivet
{
public:
    void reset() { hb.reset(); hall = 0.0f; hallRaknare = 0; dc.reset(); inEffekt = utEffekt = 1.0e-6f; }
    void prepare (double sr)
    {
        dc.prepare (sr);
        matKoef = tidskonstant (0.3, sr);
        reset();
    }
    void setParams (const DrivParams& p) { params = p; }

    float tick (float x)
    {
        if (params.lage == DrivLage::Av) return x;
        const float y = forvrang (x);
        inEffekt += (x * x - inEffekt) * matKoef;
        utEffekt += (y * y - utEffekt) * matKoef;
        const float g = std::sqrt ((inEffekt + 1.0e-7f) / (utEffekt + 1.0e-7f));
        return y * std::clamp (g, 0.25f, 2.0f);
    }

private:
    float forvrang (float x)
    {
        const float m = klamp01 (params.mangd);
        switch (params.lage)
        {
            case DrivLage::Av: return x;

            case DrivLage::Ror:
            {
                const float drive = std::exp2 (m * 5.0f);   // 0..30 dB
                float a, b;
                hb.uppsampla (x, a, b);
                a = ror (a, drive); b = ror (b, drive);
                return dc.tick (hb.nedsampla (a, b));
            }

            case DrivLage::Overdrive:
            {
                const float drive = std::exp2 (m * 6.0f);   // 0..36 dB
                float a, b;
                hb.uppsampla (x, a, b);
                a = overdrive (a * drive); b = overdrive (b * drive);
                return dc.tick (hb.nedsampla (a, b));
            }

            case DrivLage::Bitkross:
            {
                const int haller = 1 + (int) (m * m * 11.0f);              // 1..12 sample
                const float nivaer = std::exp2 (12.0f - 9.0f * m);         // 12..3 bitar
                if (hallRaknare++ % haller == 0)
                    hall = std::round (mjukKlipp (x * (1.0f + 2.0f * m)) * nivaer) / nivaer;
                return hall;
            }
        }
        return x;
    }

    static float klamp01 (float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

    // Förspänt rör: jämna övertoner.
    static float ror (float x, float drive)
    {
        constexpr float bias = 0.3f;
        static const float bOff = std::tanh (bias);
        return std::tanh (x * drive + bias) - bOff;
    }

    // Diodklippning: mjuk i ena riktningen, hårdare i den andra.
    static float overdrive (float x)
    {
        return x > 0.0f ? mjukKlipp (x) : std::max (-0.8f, mjukKlipp (x * 1.3f) * 0.8f);
    }

    DrivParams params;
    Halvband hb;
    DcSparr dc;
    float hall = 0.0f, inEffekt = 1.0e-6f, utEffekt = 1.0e-6f, matKoef = 0.0001f;
    int hallRaknare = 0;
};

} // namespace km
