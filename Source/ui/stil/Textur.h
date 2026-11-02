// Texturer genererade i kod: borstad aluminium och plastnarv. Inga bildfiler.
//
// Texturerna skapas i fysiska pixlar (designstorlek × skalfaktor), så att de är lika skarpa i 200 %
// som i 100 %. Samma frö ger alltid samma bild, så att panelen ser likadan ut varje gång.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <cstdint>
#include <vector>

namespace km::stil
{

// Liten deterministisk slumpgenerator (xorshift32) – samma frö, samma brus, på alla plattformar.
struct Slump
{
    explicit Slump (uint32_t fro) : s (fro != 0 ? fro : 0x9e3779b9u) {}
    uint32_t nasta() noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float enhet() noexcept    { return (float) (nasta() >> 8) * (1.0f / 16777216.0f); }   // 0..1
    float bipolar() noexcept  { return enhet() * 2.0f - 1.0f; }                           // −1..1
    uint32_t s;
};

// Skalfaktorn mellan design och fysiska pixlar för den Graphics vi ritar i.
inline float fysiskSkala (juce::Graphics& g)
{
    return juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
}

inline uint8_t tillByte (float v) noexcept { return (uint8_t) juce::jlimit (0, 255, (int) std::lround (v)); }

// Borstad aluminium: fina längsgående streck (variation mellan raderna) som vandrar långsamt i x-led.
inline juce::Image borstadMetall (int w, int h, float skala, juce::Colour bas, float styrka, uint32_t fro)
{
    const int pw = juce::jmax (1, (int) std::ceil (w * skala)), ph = juce::jmax (1, (int) std::ceil (h * skala));
    juce::Image bild (juce::Image::RGB, pw, ph, false);
    juce::Image::BitmapData bd (bild, juce::Image::BitmapData::writeOnly);
    Slump r (fro);
    const float br = bas.getFloatRed() * 255.0f, bgr = bas.getFloatGreen() * 255.0f, bb = bas.getFloatBlue() * 255.0f;
    float radMinne = 0.0f;
    for (int y = 0; y < ph; ++y)
    {
        // Raderna är korrelerade över ungefär en designpixel, så att strecken inte blir för fina i 200 %.
        const float ny = r.bipolar();
        radMinne = radMinne * (skala > 1.5f ? 0.45f : 0.2f) + ny * 0.8f;
        float vandring = r.bipolar() * 0.5f;
        float streck = 0.0f; int streckKvar = 0;
        for (int x = 0; x < pw; ++x)
        {
            vandring += r.bipolar() * 0.06f;
            vandring *= 0.995f;
            if (streckKvar <= 0 && r.enhet() < 0.0025f) { streck = r.bipolar() * 1.6f; streckKvar = (int) (r.enhet() * 120.0f * skala); }
            const float st = streckKvar-- > 0 ? streck : 0.0f;
            const float v = (radMinne * 0.75f + vandring * 0.35f + st + r.bipolar() * 0.25f) * styrka;
            bd.setPixelColour (x, y, juce::Colour (tillByte (br + v), tillByte (bgr + v), tillByte (bb + v * 1.08f)));
        }
    }
    return bild;
}

// Plastnarv: mjukt kornigt brus med enstaka ljusa korn, som på formsprutad svart plast.
inline juce::Image plastnarv (int w, int h, float skala, juce::Colour bas, float styrka, uint32_t fro)
{
    const int pw = juce::jmax (1, (int) std::ceil (w * skala)), ph = juce::jmax (1, (int) std::ceil (h * skala));
    juce::Image bild (juce::Image::RGB, pw, ph, false);
    juce::Image::BitmapData bd (bild, juce::Image::BitmapData::writeOnly);
    Slump r (fro);
    const float br = bas.getFloatRed() * 255.0f, bgr = bas.getFloatGreen() * 255.0f, bb = bas.getFloatBlue() * 255.0f;
    std::vector<float> forra ((size_t) pw, 0.0f);
    for (int y = 0; y < ph; ++y)
    {
        float vanster = 0.0f;
        for (int x = 0; x < pw; ++x)
        {
            const float n = r.bipolar();
            const float v = (n + vanster + forra[(size_t) x]) / 3.0f;    // lite utsmetat korn
            vanster = n; forra[(size_t) x] = n;
            float ljus = v * styrka;
            if (r.enhet() < 0.002f) ljus += styrka * 2.5f;
            bd.setPixelColour (x, y, juce::Colour (tillByte (br + ljus), tillByte (bgr + ljus), tillByte (bb + ljus)));
        }
    }
    return bild;
}

// Fyller en form med en textur som är skapad i fysiska pixlar för ytan 'yta'.
inline void fyllMedTextur (juce::Graphics& g, const juce::Path& form, const juce::Image& tex,
                           juce::Rectangle<float> yta, float skala)
{
    g.setFillType (juce::FillType (tex, juce::AffineTransform::scale (1.0f / skala).translated (yta.getX(), yta.getY())));
    g.fillPath (form);
}

} // namespace km::stil
