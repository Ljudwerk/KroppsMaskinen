// Grund – små byggstenar som alla kanaler delar: bandbegränsade vågformer, TPT-filter, DC-spärr och mättnad.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace km
{

inline constexpr double pi = 3.14159265358979323846;
inline constexpr double twoPi = 2.0 * pi;

inline double noteToHz (double note) { return 440.0 * std::exp2 ((note - 69.0) / 12.0); }

// PolyBLEP-korrektion för en diskontinuitet vid fas 0 (t = fas 0..1, dt = fasökning per sample).
inline double polyBlep (double t, double dt)
{
    if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

// Mjuk mättnad som är billig och går mot ±1.
inline float mjukKlipp (float x)
{
    if (x > 3.0f)  return 1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Tillståndsvariabelt filter (TPT/Zavalishin). Ger lågpass, bandpass och högpass samtidigt.
struct Svf
{
    void reset() { s1 = s2 = 0.0f; }

    void set (double hz, double q, double sr)
    {
        const double g = std::tan (pi * std::clamp (hz, 10.0, sr * 0.49) / sr);
        k = (float) (1.0 / std::max (0.5, q));
        a1 = (float) (1.0 / (1.0 + g * (g + k)));
        a2 = (float) g * a1;
        a3 = (float) g * a2;
    }

    void tick (float x)
    {
        const float v3 = x - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }

    float lp = 0.0f, bp = 0.0f, hp = 0.0f;

private:
    float s1 = 0.0f, s2 = 0.0f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
};

// DC-spärr: ett högpass runt 10 Hz.
struct DcSparr
{
    void prepare (double sr) { r = (float) std::exp (-twoPi * 10.0 / sr); }
    void reset() { x1 = y1 = 0.0f; }
    float tick (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }

private:
    float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
};

// Liten deterministisk slump (xorshift), så att renderingar blir likadana varje gång.
struct Slump
{
    explicit Slump (uint32_t seed = 0x4b4d3837u) : s (seed ? seed : 1u) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float bipolar() { return (float) (next() & 0xffffffu) / 8388608.0f - 1.0f; }  // −1..1
    float unipolar() { return (float) (next() & 0xffffffu) / 16777216.0f; }       // 0..1

private:
    uint32_t s;
};

// Koefficient för en exponentiell glidning som når ~63 % på tiden 'sekunder'.
inline float tidskonstant (double sekunder, double sr)
{
    return sekunder <= 0.0 ? 1.0f : (float) (1.0 - std::exp (-1.0 / (sekunder * sr)));
}

} // namespace km
