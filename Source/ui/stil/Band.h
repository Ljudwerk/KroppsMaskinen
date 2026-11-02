// Bandet: VHS-lagret ovanpå skärmen (lager 3).
//
// Bilden behandlas pixel för pixel i fysisk upplösning: radvis darr, kromatisk förskjutning (röda
// och cyanfärgade kanter), utsmetad färg, brus, enstaka tappningar, en svepande ljusare zon och
// spårningsbrus i bildens nederkant. Därefter läggs OSD-texten på (den kommer från bandspelaren,
// inte från bandet, och är därför skarp) och sist bildröret och glaset.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <functional>
#include <vector>
#include "Palett.h"
#include "Textur.h"
#include "OSDTypsnitt.h"
#include "Skarm.h"

namespace km::stil::band
{

struct Installning
{
    bool  band       = true;      // av = bara bildröret (Bandstyrka "av")
    float styrka     = 1.0f;      // 0..1 (Bandstyrka "lite" ≈ 0,5)
    uint32_t fro     = 0x8701u;   // bildruta; byt för att animera bruset
    juce::String lage, kvalitet, rakneverk, kanal;   // OSD-texten
};

// Tillämpar bandets fel på bilden (på plats).
inline void tillampa (juce::Image& bild, float skala, const Installning& in)
{
    const int W = bild.getWidth(), H = bild.getHeight();
    const float s = skala, st = in.styrka;
    std::vector<float> R ((size_t) (W * H)), G (R.size()), B (R.size());
    {
        juce::Image::BitmapData bd (bild, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const auto c = bd.getPixelColour (x, y);
                const size_t i = (size_t) (y * W + x);
                R[i] = c.getRed(); G[i] = c.getGreen(); B[i] = c.getBlue();
            }
    }
    Slump rnd (in.fro);
    const float sparZon = (float) H * 0.90f;          // spårningsbruset börjar här
    const float svepY = (float) H * 0.34f, svepH = (float) H * 0.07f;

    auto prov = [W] (const std::vector<float>& k, int rad, float x)
    {
        x = juce::jlimit (0.0f, (float) (W - 1), x);
        const int i = (int) x; const float f = x - (float) i;
        const int j = juce::jmin (W - 1, i + 1);
        return k[(size_t) (rad * W + i)] * (1.0f - f) + k[(size_t) (rad * W + j)] * f;
    };

    std::vector<float> r2 ((size_t) W), g2 ((size_t) W), b2 ((size_t) W);
    float darr = 0.0f;
    juce::Image::BitmapData ut (bild, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < H; ++y)
    {
        // Radförskjutning: långsamt darr överallt, kraftig i spårningszonen.
        const float yd = (float) y / s;
        darr = darr * 0.9f + rnd.bipolar() * 0.12f;
        float dx = (darr + 0.35f * std::sin (yd * 0.11f)) * s * st;
        float ljus = 0.0f, gra = 0.0f;
        const bool iZon = (float) y >= sparZon;
        if (iZon)
        {
            const float t = ((float) y - sparZon) / ((float) H - sparZon);      // 0..1 nedåt
            dx += (5.0f + 14.0f * t * t + rnd.bipolar() * 3.0f) * s * st;
            gra = (0.25f + 0.45f * t) * st;
            ljus = (rnd.enhet() < 0.08f ? 40.0f : 0.0f) * st;
        }
        const float ds = (yd - svepY / s) / (svepH / s);
        const float svep = std::exp (-ds * ds) * 14.0f * st;               // svepande ljus zon
        if (std::abs (ds) < 1.5f) dx += std::exp (-ds * ds) * 1.2f * s * st;

        // Kromatisk förskjutning: rött åt höger, blått åt vänster → röda och cyan kanter.
        const float kr = 1.7f * s * st;
        for (int x = 0; x < W; ++x)
        {
            r2[(size_t) x] = prov (R, y, (float) x - dx - kr);
            g2[(size_t) x] = prov (G, y, (float) x - dx + kr * 0.25f);
            b2[(size_t) x] = prov (B, y, (float) x - dx + kr);
        }
        // Utsmetad färg (VHS har låg färgupplösning): lådfilter, bredare för r/b än för g.
        auto smeta = [W] (std::vector<float>& k, int rad)
        {
            if (rad <= 0) return;
            std::vector<float> t (k);
            float sum = 0.0f;
            for (int x = -rad; x <= rad; ++x) sum += t[(size_t) juce::jlimit (0, W - 1, x)];
            for (int x = 0; x < W; ++x)
            {
                k[(size_t) x] = sum / (float) (2 * rad + 1);
                sum += t[(size_t) juce::jlimit (0, W - 1, x + rad + 1)] - t[(size_t) juce::jlimit (0, W - 1, x - rad)];
            }
        };
        smeta (r2, (int) std::lround (1.6f * s * st));
        smeta (b2, (int) std::lround (1.6f * s * st));
        smeta (g2, (int) std::lround (0.5f * s * st));

        // Tappningar: korta vita streck här och där.
        int tappStart = -1, tappLangd = 0;
        if (! iZon && rnd.enhet() < 0.004f * st)
        {
            tappStart = (int) (rnd.enhet() * (float) W);
            tappLangd = (int) ((8.0f + rnd.enhet() * 40.0f) * s);
        }
        // Spårningsbrusets vita streck.
        int streckStart = -1, streckLangd = 0;
        if (iZon && rnd.enhet() < 0.45f)
        {
            streckStart = (int) (rnd.enhet() * (float) W);
            streckLangd = (int) ((10.0f + rnd.enhet() * 120.0f) * s);
        }

        for (int x = 0; x < W; ++x)
        {
            float r = r2[(size_t) x], gg = g2[(size_t) x], b = b2[(size_t) x];
            const float brus = rnd.bipolar() * 10.0f * st;
            r += brus + svep + ljus; gg += brus + svep + ljus; b += brus + svep * 1.1f + ljus;
            if (iZon)
            {
                const float sno = rnd.enhet();
                const float v = sno * sno * 255.0f;
                r = r * (1.0f - gra) + v * gra; gg = gg * (1.0f - gra) + v * gra; b = b * (1.0f - gra) + v * gra;
            }
            auto streck = [&] (int start, int langd, float styrkaS)
            {
                if (start < 0 || x < start || x >= start + langd) return;
                const float t = (float) (x - start) / (float) langd;
                const float a = std::sin (t * pi) * styrkaS;
                r += a; gg += a; b += a;
            };
            streck (tappStart, tappLangd, 200.0f);
            streck (streckStart, streckLangd, 230.0f * st);
            ut.setPixelColour (x, y, juce::Colour (tillByte (r), tillByte (gg), tillByte (b)));
        }
    }
}

// Lägger OSD-texten i bilden (designkoordinater, bilden är w × h designpixlar).
inline void ritaOSD (juce::Graphics& g, float w, float h, const Installning& in)
{
    const float p = 2.6f;                          // en OSD-punkt
    const float kant = 26.0f, rad = 7.0f * p;
    if (in.lage.isNotEmpty())      osd::rita (g, in.lage, kant, 20.0f, p);
    if (in.kvalitet.isNotEmpty())  osd::rita (g, in.kvalitet, w - kant - osd::bredd (in.kvalitet, p), 20.0f, p);
    const float yl = h * 0.90f - rad - 14.0f;
    if (in.kanal.isNotEmpty())     osd::rita (g, in.kanal, kant, yl, p);
    if (in.rakneverk.isNotEmpty()) osd::rita (g, in.rakneverk, w - kant - osd::bredd (in.rakneverk, p), yl, p);
}

// Hela skärmbilden: scen → band → OSD → bildrör. Returnerar en bild i fysisk upplösning.
inline juce::Image skarmbild (float w, float h, float skala, const Installning& in,
                              const std::function<void (juce::Graphics&, Rectangle<float>)>& scen)
{
    juce::Image bild (juce::Image::RGB, (int) std::ceil (w * skala), (int) std::ceil (h * skala), true);
    {
        juce::Graphics g (bild);
        g.addTransform (juce::AffineTransform::scale (skala));
        scen (g, { 0.0f, 0.0f, w, h });
    }
    if (in.band)
    {
        tillampa (bild, skala, in);
        juce::Graphics g (bild);
        g.addTransform (juce::AffineTransform::scale (skala));
        ritaOSD (g, w, h, in);
    }
    skarm::bildror (bild, skala, in.band ? 0.32f : 0.22f);
    return bild;
}

// Ritar den färdiga skärmbilden i glaset, med glasets reflexer ovanpå.
inline void ritaSkarm (juce::Graphics& g, Rectangle<float> glas, const juce::Image& bild)
{
    const auto form = skarm::glasForm (glas);
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (form);
        g.setColour (juce::Colours::black);
        g.fillRect (glas.expanded (6.0f));
        g.drawImage (bild, glas, juce::RectanglePlacement::stretchToFit);
    }
    skarm::ritaGlas (g, glas, form);
}

} // namespace km::stil::band
