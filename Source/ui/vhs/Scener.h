// Scenerna på bildskärmen: en trådramsscen per vy och kanal, i designkoordinater.
//
// Varje scen är uppdelad i lager efter hur ofta de ändras, så att Bildskarm kan cacha det mesta:
//
//   bakgrund      himmel, fabrik, golv och fasta maskindelar – ritas om bara när scenen byts
//   halvstatiskt  stegstaplarna (utan spelhuvud), trumetiketten, LFO-formerna – ritas om när
//                 mönstret, valet eller formen ändras
//   sprajtar      kolven i 24 vevvinklar, hammaren, bilan, mixertornen – ritas en gång och
//                 flyttas, väljs eller klipps per bildruta
//   levande       spelhuvud, oscilloskop, gnistor, trumpelare, skrivare – ritas varje bildruta
//                 (med den billigare levandeLinje)
//
// Allt ritas i rutan r (skärmens designstorlek, ungefär 640 × 304) med samma ordförråd som stilprovet:
// röda golvlinjer, cyan och vita maskindelar, fosforglöd (skarm::glodlinje).
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>
#include <cmath>
#include <vector>
#include "../stil/Palett.h"
#include "../stil/Textur.h"
#include "../stil/OSDTypsnitt.h"
#include "../stil/Skarm.h"
#include "SkarmData.h"

namespace km::vhs::scen
{

using km::stil::Colour;
using km::stil::Point;
using km::stil::Rectangle;
using km::stil::pi;
using km::stil::skarm::V3;
using km::stil::skarm::Kamera;
using km::stil::skarm::Placering;
namespace farg = km::stil::farg;
namespace sk = km::stil::skarm;

// Scenerna. Vy::Sekvens visar samma scen som Vy::Kanal (panelen visar rutnätet utanför skärmen).
enum class Scen { Kolvbasen = 0, Kolvbas2, Stampverket, Hugget, Mixer, Effekter, Modulation };
inline constexpr int antalScener = 7;

inline Scen scenFor (Vy vy, int kanal)
{
    switch (vy)
    {
        case Vy::Mixer:      return Scen::Mixer;
        case Vy::Effekter:   return Scen::Effekter;
        case Vy::Modulation: return Scen::Modulation;
        case Vy::Kanal:
        case Vy::Sekvens:    break;
    }
    switch (juce::jlimit (0, 3, kanal))
    {
        case 1:  return Scen::Kolvbas2;
        case 2:  return Scen::Stampverket;
        case 3:  return Scen::Hugget;
        default: return Scen::Kolvbasen;
    }
}

inline bool harKolvar (Scen s)    { return s == Scen::Kolvbasen || s == Scen::Kolvbas2; }
inline bool harFallvikt (Scen s)  { return s == Scen::Stampverket || s == Scen::Hugget; }
inline bool harSteg (Scen s)      { return s == Scen::Kolvbasen || s == Scen::Kolvbas2 || s == Scen::Hugget; }

// Stampverkets tolv trummor i spårordning (som Trumma i Stampverket.h), förkortade för skärmen.
inline const char* trumKort (int i)
{
    static const char* namn[12] { "BD", "SD", "SD2", "CP", "CH", "OH", "CY", "LT", "HT", "MET", "SML", "KO" };
    return namn[juce::jlimit (0, 11, i)];
}

// ---------------------------------------------------------------------------------------------
// Rörelsen: tillståndet som animeras mellan bildrutorna. Bildskarm räknar fram det ur SkarmData.

struct Rorelse
{
    float vev    = 0.95f;                  // vevvinkel (radianer, 0 = övre dödläge)
    float fall   = 0.0f;                   // hammaren/bilan: 0 = uppe, 1 = nere
    float gnistor = 0.0f;                  // 1 vid nedslaget, avtar
    std::array<float, 12> pelare {};       // Stampverkets trummor: puls 0..1
    std::array<float, 5>  niva {}, topp {}; // mixern: visad nivå och topphållning (0..1, i dB-skala)

    static constexpr int spar = 96;         // historik för modulationsskrivarna
    std::array<std::array<float, spar>, 4> historik {};   // lfo 1, lfo 2, menv 1, menv 2
    int skriv = 0;                          // nästa plats i historiken

    static constexpr int ekon = 4;          // Effekter: äldre oscilloskopsvep som drar sig bakåt
    std::array<std::array<float, 128>, ekon> svep {};
    int svepSkriv = 0;
};

// ---------------------------------------------------------------------------------------------
// Kameror och hjälpare

inline constexpr float golvY = -3.2f;

inline Kamera golvKamera (Rectangle<float> r)
{
    Kamera k;
    k.mitt = { r.getCentreX(), r.getY() + r.getHeight() * 0.62f };
    k.brannvidd = r.getHeight() * 1.1f;
    k.lutning = 0.18f;
    return k;
}

inline Kamera objektKamera (Rectangle<float> r, float brannvidd = 1.30f)
{
    Kamera k;
    k.mitt = { r.getCentreX(), r.getY() + r.getHeight() * 0.535f };
    k.brannvidd = r.getHeight() * brannvidd;
    k.lutning = 0.30f;
    return k;
}

// Kolvarnas placeringar: en i Kolvbasen (som stilprovet), två bredvid varandra i Kolvbas 2.
inline int antalKolvar (Scen s) { return s == Scen::Kolvbas2 ? 2 : (s == Scen::Kolvbasen ? 1 : 0); }

inline Kamera kolvKamera (Rectangle<float> r, Scen s)
{
    return objektKamera (r, s == Scen::Kolvbas2 ? 1.02f : 1.30f);
}

inline Placering kolvPlacering (Scen s, int i)
{
    Placering pl;
    pl.vridning = 0.55f;
    if (s == Scen::Kolvbas2)
    {
        pl.vridning = i == 0 ? 0.62f : 0.48f;
        pl.pos = { i == 0 ? -4.05f : -1.15f, 0.55f, i == 0 ? 0.6f : 0.0f };
    }
    else pl.pos = { -1.9f, 0.25f, 0.0f };
    return pl;
}

// Fallvikternas maskiner (hammaren, bilan) är högre än kolven och ses lite längre ifrån.
inline Kamera maskinKamera (Rectangle<float> r)
{
    Kamera k = objektKamera (r, 1.0f);
    k.mitt.y = r.getY() + r.getHeight() * 0.51f;
    return k;
}

// Fallvikternas maskiner (hammaren, bilan) använder objektkameran och en egen placering.
inline Placering maskinPlacering() { Placering pl; pl.vridning = 0.50f; pl.pos = { -1.9f, 0.25f, 0.0f }; return pl; }

// Rätblock (tolv kanter).
inline void lada (juce::Path& p, const Kamera& k, const Placering& pl, V3 a, V3 b)
{
    const V3 h[8] { { a.x, a.y, a.z }, { b.x, a.y, a.z }, { b.x, a.y, b.z }, { a.x, a.y, b.z },
                    { a.x, b.y, a.z }, { b.x, b.y, a.z }, { b.x, b.y, b.z }, { a.x, b.y, b.z } };
    for (int i = 0; i < 4; ++i)
    {
        sk::linje (p, k, pl, h[i], h[(i + 1) % 4]);
        sk::linje (p, k, pl, h[i + 4], h[(i + 1) % 4 + 4]);
        sk::linje (p, k, pl, h[i], h[i + 4]);
    }
}

// Cirkel i yz-planet (liggande cylinder längs x).
inline void cirkelX (juce::Path& p, const Kamera& k, const Placering& pl, V3 c, float r, int seg = 28)
{
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * pi * (float) i / (float) seg;
        const auto s = k (pl ({ c.x, c.y + r * std::sin (a), c.z + r * std::cos (a) }));
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
}


// Liten OSD-text inne i scenen (en del av bilden, så den går genom bandet som allt annat).
inline void scenText (juce::Graphics& g, const juce::String& t, float x, float y, float punkt, Colour f, bool centrerad = false)
{
    if (centrerad) x -= km::stil::osd::bredd (t, punkt) * 0.5f;
    g.setColour (f);
    km::stil::osd::ritaRaw (g, t, x, y, punkt);
}

// Glödlinje för det som ritas varje bildruta: samma utseende som skarm::glodlinje men utan det
// breda yttersta glödskiktet och med fasade hörn (hälften så dyr att stryka).
inline void levandeLinje (juce::Graphics& g, const juce::Path& p, Colour f, float tj, float glod = 1.0f)
{
    const auto led = juce::PathStrokeType::beveled;
    g.setColour (f.withAlpha (f.getFloatAlpha() * 0.20f * glod)); g.strokePath (p, juce::PathStrokeType (tj * 3.4f, led, juce::PathStrokeType::butt));
    g.setColour (f);                                               g.strokePath (p, juce::PathStrokeType (tj, led, juce::PathStrokeType::butt));
    g.setColour (f.interpolatedWith (juce::Colours::white, 0.55f).withAlpha (0.7f * f.getFloatAlpha()));
    g.strokePath (p, juce::PathStrokeType (tj * 0.4f, led, juce::PathStrokeType::butt));
}

// ---------------------------------------------------------------------------------------------
// Gemensam grund: svart med fosforton, himmel, fabrik och golv (som stilprovet).

inline void ritaGrund (juce::Graphics& g, Rectangle<float> r, bool fabrik = true)
{
    g.setColour (farg::skarmSvart);
    g.fillRect (r);
    {
        juce::ColourGradient fosfor (Colour (0xff0b1014), r.getCentre(), farg::skarmSvart, r.getTopLeft(), true);
        g.setGradientFill (fosfor);
        g.fillRect (r);
    }
    const auto golv = golvKamera (r);
    const float hor = golv.horisont();
    sk::ritaHimmel (g, r, hor);
    if (fabrik) sk::ritaFabrik (g, r, hor);
    sk::ritaGolv (g, r, golv, golvY, 0.35f);
}

// Oscilloskopfönster (ram, prickat rutnät, hörnvinklar) utan innehåll.
inline void ritaFonster (juce::Graphics& g, Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.fillRect (r);
    for (int i = 1; i < 8; ++i)
        for (int j = 1; j < 4; ++j)
        {
            g.setColour (farg::skarmCyan.withAlpha (j == 2 ? 0.35f : 0.18f));
            g.fillRect (r.getX() + r.getWidth() * (float) i / 8.0f - 0.5f, r.getY() + r.getHeight() * (float) j / 4.0f - 0.5f, 1.0f, 1.0f);
        }
    g.setColour (farg::skarmCyan.withAlpha (0.18f));
    g.drawLine (r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 0.5f);

    juce::Path hv;
    const float l = juce::jmin (9.0f, r.getHeight() * 0.3f);
    struct Horn { float x, y, dx, dy; };
    const Horn horn[] { { r.getX(), r.getY(), 1.0f, 1.0f }, { r.getRight(), r.getY(), -1.0f, 1.0f },
                        { r.getX(), r.getBottom(), 1.0f, -1.0f }, { r.getRight(), r.getBottom(), -1.0f, -1.0f } };
    for (const auto& h : horn)
    {
        hv.startNewSubPath (h.x + h.dx * l, h.y); hv.lineTo (h.x, h.y); hv.lineTo (h.x, h.y + h.dy * l);
    }
    sk::glodlinje (g, hv, farg::skarmVit, 1.0f, 0.6f);
}

// Rutorna i de olika scenerna (designkoordinater i r).
inline Rectangle<float> scopeRuta (Rectangle<float> r)
{
    return { r.getX() + r.getWidth() * 0.60f, r.getY() + r.getHeight() * 0.10f, r.getWidth() * 0.35f, r.getHeight() * 0.30f };
}

inline Rectangle<float> modRuta (Rectangle<float> r, int i)   // 0, 1 = LFO; 2, 3 = envelopes
{
    const float x = r.getX() + r.getWidth() * (i % 2 == 0 ? 0.06f : 0.52f), w = r.getWidth() * 0.42f;
    return i < 2 ? Rectangle<float> (x, r.getY() + r.getHeight() * 0.205f, w, r.getHeight() * 0.31f)
                 : Rectangle<float> (x, r.getY() + r.getHeight() * 0.615f, w, r.getHeight() * 0.155f);
}

// ---------------------------------------------------------------------------------------------
// Stegstaplarna på golvet: sexton steg (sidan där spelhuvudet är), höjd efter nivån, rött = accent.

inline int stegSida (const SkarmData& d)
{
    const int langd = juce::jlimit (1, 64, d.langd);
    return d.steg >= 0 ? (d.steg % langd) / 16 : 0;
}

inline float stegHojd (float niva) { return niva <= 0.01f ? 0.06f : (niva >= 0.95f ? 2.1f : 0.55f + niva * 1.15f); }

inline void stegLada (juce::Path& p, const Kamera& k, int i, float h)
{
    const float z = 7.0f;
    const float x0 = 0.8f + (float) i * 0.5f, x1 = x0 + 0.34f, z0 = z, z1 = z + 0.34f;
    const V3 b[4] { { x0, golvY, z0 }, { x1, golvY, z0 }, { x1, golvY, z1 }, { x0, golvY, z1 } };
    for (int j = 0; j < 4; ++j)
    {
        const V3 a = b[j], c = b[(j + 1) % 4];
        sk::linje (p, k, {}, a, c);
        sk::linje (p, k, {}, { a.x, a.y + h, a.z }, { c.x, c.y + h, c.z });
        sk::linje (p, k, {}, a, { a.x, a.y + h, a.z });
    }
}


// Nyckel för stegstaplarna: ändras bara när mönstret, längden eller sidan ändras.
inline uint64_t stegNyckel (const SkarmData& d)
{
    uint64_t h = 1469598103934665603ull;
    auto mix = [&h] (uint64_t v) { h ^= v; h *= 1099511628211ull; };
    const int sida = stegSida (d), langd = juce::jlimit (1, 64, d.langd);
    mix ((uint64_t) sida); mix ((uint64_t) langd);
    for (int i = 0; i < 16; ++i)
    {
        const int idx = sida * 16 + i;
        mix (idx < langd ? (uint64_t) std::lround (d.stegNiva[(size_t) idx] * 64.0f) : 999u);
    }
    return h;
}

inline void ritaStegStaplar (juce::Graphics& g, Rectangle<float> r, const SkarmData& d)
{
    const auto k = golvKamera (r);
    const int sida = stegSida (d), langd = juce::jlimit (1, 64, d.langd);
    juce::Path cyan, rott, tyst, utanfor;
    for (int i = 0; i < 16; ++i)
    {
        const int idx = sida * 16 + i;
        if (idx >= langd) { stegLada (utanfor, k, i, 0.03f); continue; }
        const float n = d.stegNiva[(size_t) idx];
        stegLada (n <= 0.01f ? tyst : (n >= 0.95f ? rott : cyan), k, i, stegHojd (n));
    }
    sk::glodlinje (g, utanfor, farg::skarmCyan.withAlpha (0.12f), 0.6f, 0.4f);
    sk::glodlinje (g, tyst, farg::skarmCyan.withAlpha (0.35f), 0.8f, 0.8f);
    sk::glodlinje (g, cyan, farg::skarmCyan, 0.8f, 0.8f);
    sk::glodlinje (g, rott, farg::skarmRod, 0.8f, 0.8f);
}

inline void ritaSpelhuvud (juce::Graphics& g, Rectangle<float> r, const SkarmData& d)
{
    if (d.steg < 0) return;
    const int langd = juce::jlimit (1, 64, d.langd);
    const int idx = d.steg % langd;
    juce::Path p;
    stegLada (p, golvKamera (r), idx % 16, stegHojd (d.stegNiva[(size_t) idx]) + 0.02f);
    levandeLinje (g, p, farg::skarmVit, 0.9f, 1.0f);
}

// ---------------------------------------------------------------------------------------------
// Stampverket: en fallhammare (städ, pelare, tvärbalk, cylinder) och tolv trumpelare på golvet.

inline constexpr float hammareUppe = 0.25f, hammareNere = -1.75f, hammareHojd = 0.85f;

inline void ritaPressram (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = maskinKamera (r);
    const auto pl = maskinPlacering();
    juce::Path ram, stad;
    lada (stad, k, pl, { -1.6f, -2.95f, -1.1f }, { 1.6f, -2.6f, 1.1f });     // bottenplatta
    lada (stad, k, pl, { -1.05f, -2.6f, -0.8f }, { 1.05f, -1.95f, 0.8f });   // städet
    lada (stad, k, pl, { -0.7f, -1.95f, -0.55f }, { 0.7f, -1.75f, 0.55f });  // undre verktyget
    for (float x : { -1.85f, 1.45f })
        lada (ram, k, pl, { x, -2.6f, -0.22f }, { x + 0.4f, 2.45f, 0.22f });  // pelarna
    lada (ram, k, pl, { -2.0f, 2.45f, -0.4f }, { 2.0f, 2.85f, 0.4f });        // tvärbalken
    sk::cirkel (ram, k, pl, { 0, 2.85f, 0 }, 0.42f, true, 24);                // cylindern
    sk::cirkel (ram, k, pl, { 0, 3.25f, 0 }, 0.42f, true, 24);
    for (int i = 0; i < 6; ++i)
    {
        const float a = 2.0f * pi * (float) i / 6.0f + 0.3f;
        sk::linje (ram, k, pl, { 0.42f * std::cos (a), 2.85f, 0.42f * std::sin (a) }, { 0.42f * std::cos (a), 3.25f, 0.42f * std::sin (a) });
    }
    // Stagens diagonaler i pelarna.
    for (float x : { -1.85f, 1.45f })
        for (int i = 0; i < 5; ++i)
        {
            const float y0 = -2.6f + (float) i * 1.01f;
            sk::linje (ram, k, pl, { x, y0, 0.22f }, { x + 0.4f, y0 + 1.01f, 0.22f });
        }
    sk::glodlinje (g, ram, farg::skarmRod.withAlpha (0.85f), 0.9f, 0.8f);
    sk::glodlinje (g, stad, farg::skarmVit.withAlpha (0.85f), 0.9f, 0.8f);
}

// Hammaren (baren) i övre läget; Bildskarm flyttar den längs fallVektor.
inline void ritaHammare (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = maskinKamera (r);
    const auto pl = maskinPlacering();
    const float y0 = hammareUppe, y1 = hammareUppe + hammareHojd;
    juce::Path p, ribbor;
    lada (p, k, pl, { -1.2f, y0 + 0.2f, -0.75f }, { 1.2f, y1, 0.75f });
    lada (p, k, pl, { -0.75f, y0, -0.5f }, { 0.75f, y0 + 0.2f, 0.5f });      // övre verktyget
    for (int i = 1; i < 6; ++i)
    {
        const float x = -1.2f + 2.4f * (float) i / 6.0f;
        sk::linje (ribbor, k, pl, { x, y0 + 0.2f, 0.75f }, { x, y1, 0.75f });
    }
    sk::linje (ribbor, k, pl, { -1.2f, y0 + 0.55f, 0.75f }, { 1.2f, y0 + 0.55f, 0.75f });
    sk::glodlinje (g, ribbor, farg::skarmCyan.withAlpha (0.6f), 0.7f, 0.6f);
    sk::glodlinje (g, p, farg::skarmCyan, 1.0f, 1.0f);
}

inline V3 hammareKolvstang (float fall) { return { 0.0f, hammareUppe + hammareHojd + (hammareNere - hammareUppe) * fall, 0.0f }; }

// ---------------------------------------------------------------------------------------------
// Hugget: en giljotin (bänk med kubb, stolpar, tvärbalk) och en sned bila med tyngd.

inline constexpr float bilaUppe = 0.85f, bilaNere = -2.0f;

inline void ritaGiljotin (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = maskinKamera (r);
    const auto pl = maskinPlacering();
    juce::Path ram, bank;
    lada (bank, k, pl, { -1.8f, -2.95f, -0.9f }, { 1.8f, -2.35f, 0.9f });     // bänken
    cirkelX (bank, k, pl, { -1.25f, -2.05f, 0.0f }, 0.3f);                   // kubben (liggande stock)
    cirkelX (bank, k, pl, { 1.25f, -2.05f, 0.0f }, 0.3f);
    for (int i = 0; i < 6; ++i)
    {
        const float a = 2.0f * pi * (float) i / 6.0f;
        sk::linje (bank, k, pl, { -1.25f, -2.05f + 0.3f * std::sin (a), 0.3f * std::cos (a) },
                                { 1.25f, -2.05f + 0.3f * std::sin (a), 0.3f * std::cos (a) });
    }
    for (float x : { -1.75f, 1.4f })
    {
        lada (ram, k, pl, { x, -2.35f, -0.18f }, { x + 0.35f, 3.0f, 0.18f });
        sk::linje (ram, k, pl, { x + (x < 0 ? 0.35f : 0.0f), -2.0f, 0.0f }, { x + (x < 0 ? 0.35f : 0.0f), 3.0f, 0.0f });   // spåret
    }
    lada (ram, k, pl, { -2.0f, 3.0f, -0.35f }, { 2.0f, 3.35f, 0.35f });
    // Snedstag vid foten.
    sk::linje (ram, k, pl, { -1.75f, -1.2f, 0.0f }, { -2.4f, -2.35f, 0.0f });
    sk::linje (ram, k, pl, { 1.75f, -1.2f, 0.0f }, { 2.4f, -2.35f, 0.0f });
    sk::glodlinje (g, ram, farg::skarmRod.withAlpha (0.85f), 0.9f, 0.8f);
    sk::glodlinje (g, bank, farg::skarmVit.withAlpha (0.85f), 0.9f, 0.8f);
}

inline void ritaBila (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = maskinKamera (r);
    const auto pl = maskinPlacering();
    const float bas = bilaUppe;            // eggens lägsta punkt
    juce::Path blad, egg, tyngd;
    const float xa = -1.35f, xb = 1.35f, topp = bas + 1.25f, lag = bas, hog = bas + 0.55f;
    for (float z : { -0.05f, 0.05f })
    {
        blad.startNewSubPath (k (pl ({ xa, topp, z })));
        blad.lineTo (k (pl ({ xb, topp, z })));
        blad.lineTo (k (pl ({ xb, hog, z })));
        blad.startNewSubPath (k (pl ({ xa, lag, z })));
        blad.lineTo (k (pl ({ xa, topp, z })));
        egg.startNewSubPath (k (pl ({ xa, lag, z })));
        egg.lineTo (k (pl ({ xb, hog, z })));
    }
    // Fasen längs eggen.
    blad.startNewSubPath (k (pl ({ xa, lag + 0.22f, 0.05f })));
    blad.lineTo (k (pl ({ xb, hog + 0.22f, 0.05f })));
    lada (tyngd, k, pl, { -1.35f, topp, -0.25f }, { 1.35f, topp + 0.5f, 0.25f });
    for (int i = 1; i < 4; ++i)
    {
        const float x = -1.35f + 2.7f * (float) i / 4.0f;
        sk::linje (tyngd, k, pl, { x, topp, 0.25f }, { x, topp + 0.5f, 0.25f });
    }
    sk::glodlinje (g, tyngd, farg::skarmCyan, 0.9f, 0.9f);
    sk::glodlinje (g, blad, farg::skarmCyan.withAlpha (0.85f), 0.9f, 0.8f);
    sk::glodlinje (g, egg, farg::skarmVit, 1.2f, 1.2f);
}

// Fallviktens förflyttning på skärmen från uppe till nere (designkoordinater).
inline Point<float> fallVektor (Rectangle<float> r, Scen s)
{
    const auto k = maskinKamera (r);
    const auto pl = maskinPlacering();
    const float a = s == Scen::Hugget ? bilaUppe : hammareUppe, b = s == Scen::Hugget ? bilaNere : hammareNere;
    return k (pl ({ 0.0f, b, 0.0f })) - k (pl ({ 0.0f, a, 0.0f }));
}

// ---------------------------------------------------------------------------------------------
// Stampverkets trumpelare: tolv lådor på golvet som slår upp vid varje slag.

inline void pelareLada (juce::Path& p, const Kamera& k, int i, float h)
{
    const float z = 7.0f, x0 = 0.9f + (float) i * 0.64f, x1 = x0 + 0.44f, z0 = z, z1 = z + 0.44f;
    const V3 b[4] { { x0, golvY, z0 }, { x1, golvY, z0 }, { x1, golvY, z1 }, { x0, golvY, z1 } };
    for (int j = 0; j < 4; ++j)
    {
        const V3 a = b[j], c = b[(j + 1) % 4];
        sk::linje (p, k, {}, a, c);
        sk::linje (p, k, {}, { a.x, a.y + h, a.z }, { c.x, c.y + h, c.z });
        sk::linje (p, k, {}, a, { a.x, a.y + h, a.z });
    }
}


// Den valda trummans etikett (halvstatiskt lager: ändras bara när valet ändras).
inline void ritaTrumEtikett (juce::Graphics& g, Rectangle<float> r, int vald)
{
    const auto k = golvKamera (r);
    juce::Path p;
    for (int i = 0; i < 12; ++i) pelareLada (p, k, i, 0.06f);
    sk::glodlinje (g, p, farg::skarmCyan.withAlpha (0.35f), 0.8f, 0.6f);
    const auto fot = k ({ 0.9f + (float) vald * 0.64f + 0.22f, golvY, 7.0f });
    scenText (g, trumKort (vald), fot.x, fot.y + 6.0f, 1.6f, farg::skarmVit, true);
}

// ---------------------------------------------------------------------------------------------
// Mixern: fyra kanaltorn och mastern, tolv segment var, på golvet.

inline constexpr int mixSegment = 12;

inline void mixTorn (int i, float& x0, float& x1)
{
    const float xs[5] { -5.0f, -2.9f, -0.8f, 1.3f, 3.6f };
    x0 = xs[i]; x1 = x0 + (i == 4 ? 1.7f : 1.2f);
}

inline void mixSegmentLada (juce::Path& p, const Kamera& k, int torn, int seg, bool fram = false)
{
    float x0, x1; mixTorn (torn, x0, x1);
    const float z0 = 5.0f, z1 = 5.6f, y0 = golvY + 0.08f + (float) seg * 0.42f, y1 = y0 + 0.28f;
    if (fram)     // bara framsidan (fyllda segment)
    {
        p.startNewSubPath (k ({ x0, y0, z0 })); p.lineTo (k ({ x1, y0, z0 })); p.lineTo (k ({ x1, y1, z0 })); p.lineTo (k ({ x0, y1, z0 }));
        p.closeSubPath();
        return;
    }
    lada (p, k, {}, { x0, y0, z0 }, { x1, y1, z1 });
}

inline const char* mixNamn (int i)
{
    static const char* n[5] { "KB1", "KB2", "STV", "HUG", "MST" };
    return n[juce::jlimit (0, 4, i)];
}

inline void ritaMixerGrund (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = golvKamera (r);
    juce::Path p;
    for (int t = 0; t < 5; ++t)
        for (int s = 0; s < mixSegment; ++s) mixSegmentLada (p, k, t, s);
    sk::glodlinje (g, p, farg::skarmCyan.withAlpha (0.16f), 0.6f, 0.4f);
    for (int t = 0; t < 5; ++t)
    {
        float x0, x1; mixTorn (t, x0, x1);
        const auto fot = k ({ (x0 + x1) * 0.5f, golvY, 5.0f });
        scenText (g, mixNamn (t), fot.x, fot.y + 5.0f, 1.5f, t == 4 ? farg::skarmVit : farg::skarmCyan.withAlpha (0.9f), true);
    }
}

// Ett helt tänt torn (alla segment), som sprajt. vit = topphållningens färg.
inline void ritaTorn (juce::Graphics& g, Rectangle<float> r, int t, bool vit)
{
    const auto k = golvKamera (r);
    juce::Path cyan, rott, fyllC, fyllR;
    for (int sg = 0; sg < mixSegment; ++sg)
    {
        const bool hog = sg >= mixSegment - 3;
        mixSegmentLada (hog && ! vit ? rott : cyan, k, t, sg);
        mixSegmentLada (hog && ! vit ? fyllR : fyllC, k, t, sg, true);
    }
    if (vit)
    {
        g.setColour (farg::skarmVit.withAlpha (0.18f)); g.fillPath (fyllC);
        sk::glodlinje (g, cyan, farg::skarmVit, 0.8f, 0.9f);
        return;
    }
    g.setColour (farg::skarmCyan.withAlpha (0.16f)); g.fillPath (fyllC);
    g.setColour (farg::skarmRod.withAlpha (0.22f));  g.fillPath (fyllR);
    sk::glodlinje (g, cyan, farg::skarmCyan, 0.8f, 0.8f);
    sk::glodlinje (g, rott, farg::skarmRod, 0.8f, 0.9f);
}

// Skärmhöjden (design-y) för snittet under segment 'seg' i tornet t: allt under snittet är segment < seg.
inline float tornSnitt (Rectangle<float> r, int t, int seg)
{
    const auto k = golvKamera (r);
    float x0, x1; mixTorn (t, x0, x1);
    const float y = golvY + 0.08f + (float) seg * 0.42f - 0.07f;     // mitt i glipan
    return k ({ (x0 + x1) * 0.5f, y, 5.0f }).y;
}


// ---------------------------------------------------------------------------------------------
// Effekter: oscilloskopet över golvet, med äldre svep som drar sig bakåt i djupet.

inline constexpr float effektMitt = 0.4f, effektAmp = 1.9f;


inline void svepBana (juce::Path& p, const Kamera& k, const float* v, int n, float z)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = -8.0f + 16.0f * (float) i / (float) (n - 1);
        const auto s = k ({ x, effektMitt + juce::jlimit (-1.0f, 1.0f, v[i]) * effektAmp, z });
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
}

inline void ritaEffektGrund (juce::Graphics& g, Rectangle<float> r)
{
    const auto k = golvKamera (r);
    const float topp = effektMitt + effektAmp, botten = effektMitt - effektAmp;
    // Mätramen runt svepet: nollinjen, toppnivåerna (streckade) och hörnvinklar.
    juce::Path noll, horn, streckad;
    sk::linje (noll, k, {}, { -8.0f, effektMitt, 1.0f }, { 8.0f, effektMitt, 1.0f });
    // Varje kant streckas för sig (createDashedStroke binder annars ihop delbanorna).
    const float monster[] { 3.0f, 4.0f };
    auto streck = [&] (V3 a, V3 b)
    {
        juce::Path l, d;
        sk::linje (l, k, {}, a, b);
        juce::PathStrokeType (0.8f).createDashedStroke (d, l, monster, 2);
        streckad.addPath (d);
    };
    for (float y : { topp, botten }) streck ({ -8.0f, y, 1.0f }, { 8.0f, y, 1.0f });
    for (float x : { -8.0f, 8.0f })
    {
        const float dx = x < 0.0f ? 0.6f : -0.6f;
        for (float y : { topp, botten })
        {
            const float dy = y > effektMitt ? -0.5f : 0.5f;
            horn.startNewSubPath (k ({ x + dx, y, 1.0f })); horn.lineTo (k ({ x, y, 1.0f })); horn.lineTo (k ({ x, y + dy, 1.0f }));
        }
        streck ({ x, topp, 1.0f }, { x, botten, 1.0f });
    }
    g.setColour (farg::skarmCyan.withAlpha (0.45f));
    g.fillPath (streckad);
    sk::glodlinje (g, horn, farg::skarmVit, 1.0f, 0.6f);
    sk::glodlinje (g, noll, farg::skarmCyan.withAlpha (0.5f), 0.6f, 0.5f);
    // Skalstreck längs nollinjen och de äldre svepens nollinjer bakåt i djupet.
    juce::Path skal, djup;
    for (int i = 0; i <= 16; ++i)
    {
        const float x = -8.0f + (float) i;
        const float l = i % 4 == 0 ? 0.25f : 0.1f;
        sk::linje (skal, k, {}, { x, effektMitt - l, 1.0f }, { x, effektMitt + l, 1.0f });
    }
    for (int e = 1; e < Rorelse::ekon; ++e)
        sk::linje (djup, k, {}, { -8.0f, effektMitt, 1.0f + (float) e * 2.6f }, { 8.0f, effektMitt, 1.0f + (float) e * 2.6f });
    sk::glodlinje (g, skal, farg::skarmVit.withAlpha (0.6f), 0.6f, 0.5f);
    sk::glodlinje (g, djup, farg::skarmRod.withAlpha (0.35f), 0.6f, 0.4f);
    scenText (g, "OSC", k ({ -8.0f, topp, 1.0f }).x + 2.0f, k ({ -8.0f, topp, 1.0f }).y + 5.0f, 1.4f, farg::skarmCyan.withAlpha (0.9f));
}

// ---------------------------------------------------------------------------------------------
// Modulationen: två LFO-skrivare och två envelope-skrivare.

inline const char* lfoNamn (int form)
{
    static const char* n[6] { "SINUS", "TRIANGEL", "FYRKANT", "SAGTAND", "SLUMP", "GLID" };
    return n[juce::jlimit (0, 5, form)];
}

inline float lfoIdeal (int form, float fas)
{
    fas -= std::floor (fas);
    switch (form)
    {
        case 0: return std::sin (2.0f * pi * fas);
        case 1: return fas < 0.5f ? 4.0f * fas - 1.0f : 3.0f - 4.0f * fas;
        case 2: return fas < 0.5f ? 1.0f : -1.0f;
        case 3: return 1.0f - 2.0f * fas;
        case 4: { const int i = (int) (fas * 8.0f); const float v[8] { 0.6f, -0.3f, 0.9f, -0.8f, 0.1f, -0.5f, 0.75f, -0.1f }; return v[i & 7]; }
        default:
        {
            const float v[9] { 0.6f, -0.3f, 0.9f, -0.8f, 0.1f, -0.5f, 0.75f, -0.1f, 0.6f };
            const float t = fas * 8.0f; const int i = (int) t; const float f = t - (float) i;
            return v[i] + (v[i + 1] - v[i]) * (0.5f - 0.5f * std::cos (pi * f));
        }
    }
}

inline void ritaModGrund (juce::Graphics& g, Rectangle<float> r)
{
    for (int i = 0; i < 4; ++i) ritaFonster (g, modRuta (r, i));
    for (int i = 0; i < 2; ++i)
    {
        const auto e = modRuta (r, i + 2);
        scenText (g, i == 0 ? "MENV 1" : "MENV 2", e.getX() + 2.0f, e.getY() - 12.0f, 1.4f, farg::skarmVit.withAlpha (0.85f));
    }
}

// Halvstatiskt: formens ideala kurva (svag) och namnet, för båda LFO:erna.
inline void ritaLfoFormer (juce::Graphics& g, Rectangle<float> r, const SkarmData& d)
{
    for (int i = 0; i < 2; ++i)
    {
        const auto f = modRuta (r, i);
        juce::Path p;
        for (int j = 0; j <= 96; ++j)
        {
            const float fas = (float) j / 96.0f * 2.0f;
            const Point<float> s { f.getX() + f.getWidth() * (float) j / 96.0f, f.getCentreY() - lfoIdeal (d.lfoForm[(size_t) i], fas) * f.getHeight() * 0.36f };
            if (j == 0) p.startNewSubPath (s); else p.lineTo (s);
        }
        juce::Path streckad;
        const float monster[] { 3.0f, 3.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (streckad, p, monster, 2);
        g.setColour (farg::skarmRod.withAlpha (0.6f));
        g.fillPath (streckad);
        scenText (g, juce::String ("LFO ") + juce::String (i + 1) + "  " + lfoNamn (d.lfoForm[(size_t) i]),
                  f.getX() + 2.0f, f.getY() - 12.0f, 1.4f, farg::skarmCyan);
    }
}

// ---------------------------------------------------------------------------------------------
// Lagren per scen

// Bakgrunden (cachas per scen och storlek).
inline void ritaBakgrund (juce::Graphics& g, Rectangle<float> r, Scen s)
{
    ritaGrund (g, r, s != Scen::Modulation);
    switch (s)
    {
        case Scen::Kolvbasen:
        case Scen::Kolvbas2:    ritaFonster (g, scopeRuta (r)); break;
        case Scen::Stampverket: ritaPressram (g, r); break;
        case Scen::Hugget:      ritaGiljotin (g, r); ritaFonster (g, scopeRuta (r)); break;
        case Scen::Mixer:       ritaMixerGrund (g, r); break;
        case Scen::Effekter:    ritaEffektGrund (g, r); break;
        case Scen::Modulation:  ritaModGrund (g, r); break;
    }
}

// Halvstatiskt lager: nyckeln avgör när det ritas om (0 = scenen har inget sådant lager).
inline uint64_t halvstatiskNyckel (Scen s, const SkarmData& d)
{
    if (harSteg (s)) return stegNyckel (d);
    if (s == Scen::Stampverket) return 0x5100u + (uint64_t) juce::jlimit (0, 11, d.valdTrumma);
    if (s == Scen::Modulation)  return 0x3300u + (uint64_t) d.lfoForm[0] * 16u + (uint64_t) d.lfoForm[1];
    return 0;
}

inline void ritaHalvstatiskt (juce::Graphics& g, Rectangle<float> r, Scen s, const SkarmData& d)
{
    if (harSteg (s))                 ritaStegStaplar (g, r, d);
    else if (s == Scen::Stampverket) ritaTrumEtikett (g, r, juce::jlimit (0, 11, d.valdTrumma));
    else if (s == Scen::Modulation)  ritaLfoFormer (g, r, d);
}


// Mini-oscilloskopet i kanalscenerna: utsignalen och dess topp-envelope (streckad).
inline void ritaScope (juce::Graphics& g, Rectangle<float> f, const SkarmData& d)
{
    const int n = 128;
    juce::Path v, env;
    float topp = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float a = d.scope[(size_t) (i * 2)], b = d.scope[(size_t) (i * 2 + 1)];
        const float s = juce::jlimit (-1.0f, 1.0f, (a + b) * 0.5f);
        topp = juce::jmax (std::abs (s), topp * 0.93f);
        const float x = f.getX() + f.getWidth() * (float) i / (float) (n - 1);
        const Point<float> p { x, f.getCentreY() - s * f.getHeight() * 0.44f };
        const Point<float> e { x, f.getCentreY() - topp * f.getHeight() * 0.44f };
        if (i == 0) { v.startNewSubPath (p); env.startNewSubPath (e); } else { v.lineTo (p); env.lineTo (e); }
    }
    g.setColour (farg::skarmRod.withAlpha (0.55f));
    g.strokePath (env, juce::PathStrokeType (0.8f));
    levandeLinje (g, v, farg::skarmCyan, 1.1f, 1.0f);
}

// Gnistor vid nedslaget: korta strålar ut från träffpunkten.
inline void ritaGnistor (juce::Graphics& g, Point<float> c, float styrka, uint32_t fro)
{
    if (styrka <= 0.02f) return;
    km::stil::Slump rnd (fro);
    juce::Path p;
    for (int i = 0; i < 14; ++i)
    {
        const float a = pi + rnd.enhet() * pi;           // uppåt och åt sidorna
        const float l0 = 6.0f + rnd.enhet() * 10.0f, l1 = l0 + (8.0f + rnd.enhet() * 26.0f) * styrka;
        const float sida = (i % 2 == 0) ? 1.0f : -1.0f;
        const Point<float> riktning { sida * std::abs (std::cos (a)) * 1.6f, std::sin (a) * 0.5f };
        p.startNewSubPath (c + riktning * l0);
        p.lineTo (c + riktning * l1);
    }
    levandeLinje (g, p, farg::skarmVit.withAlpha (juce::jmin (1.0f, styrka * 1.3f)), 0.8f, styrka);
    // Ljusblixt längs golvet.
    g.setColour (farg::skarmVit.withAlpha (0.10f * styrka));
    g.fillEllipse (c.x - 60.0f, c.y - 6.0f, 120.0f, 12.0f);
}

inline void ritaLevande (juce::Graphics& g, Rectangle<float> r, Scen s, const SkarmData& d, const Rorelse& m, uint32_t fro)
{
    switch (s)
    {
        case Scen::Kolvbasen:
        case Scen::Kolvbas2:
            ritaScope (g, scopeRuta (r), d);
            ritaSpelhuvud (g, r, d);
            break;

        case Scen::Hugget:
        {
            ritaScope (g, scopeRuta (r), d);
            ritaSpelhuvud (g, r, d);
            const auto k = maskinKamera (r);
            ritaGnistor (g, k (maskinPlacering() ({ 0.0f, -1.8f, 0.4f })), m.gnistor, fro);
            break;
        }

        case Scen::Stampverket:
        {
            const auto k = maskinKamera (r);
            const auto pl = maskinPlacering();
            juce::Path stang;
            const V3 fot = hammareKolvstang (m.fall);
            for (float x : { -0.12f, 0.12f })
                sk::linje (stang, k, pl, { x, 2.85f, 0.0f }, { x, fot.y, 0.0f });
            levandeLinje (g, stang, farg::skarmVit, 0.9f, 0.8f);
            ritaGnistor (g, k (pl ({ 0.0f, -1.75f, 0.6f })), m.gnistor, fro);

            // Trumpelarna.
            const auto gk = golvKamera (r);
            juce::Path cyan, vit;
            const int vald = juce::jlimit (0, 11, d.valdTrumma);
            for (int i = 0; i < 12; ++i)
            {
                const float p = m.pelare[(size_t) i];
                if (p < 0.02f && i != vald) continue;
                pelareLada (i == vald ? vit : cyan, gk, i, 0.06f + p * 2.2f);
            }
            levandeLinje (g, cyan, farg::skarmCyan, 0.8f, 0.8f);
            levandeLinje (g, vit, farg::skarmVit, 0.9f, 1.0f);
            break;
        }

        case Scen::Mixer:       break;     // tornen är sprajtar (ritaTorn), Bildskarm klipper dem efter nivån

        case Scen::Effekter:
        {
            const auto k = golvKamera (r);
            // Äldre svep längre bak, från rött till svagt (enkla linjer, ingen glöd).
            for (int e = Rorelse::ekon - 1; e >= 1; --e)
            {
                const auto& v = m.svep[(size_t) ((m.svepSkriv - e + Rorelse::ekon * 4) % Rorelse::ekon)];
                juce::Path p;
                svepBana (p, k, v.data(), (int) v.size(), 1.0f + (float) e * 2.6f);
                g.setColour (farg::skarmRod.withAlpha (0.75f - 0.18f * (float) e));
                g.strokePath (p, juce::PathStrokeType (0.9f));
            }
            std::array<float, 128> nu {};
            for (size_t i = 0; i < nu.size(); ++i) nu[i] = (d.scope[i * 2] + d.scope[i * 2 + 1]) * 0.5f;
            juce::Path p;
            svepBana (p, k, nu.data(), (int) nu.size(), 1.0f);
            levandeLinje (g, p, farg::skarmCyan, 1.2f, 1.1f);
            break;
        }

        case Scen::Modulation:
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto f = modRuta (r, i).reduced (2.0f, 0.0f);
                const auto& h = m.historik[(size_t) i];
                juce::Path p;
                Point<float> sist;
                const bool env = i >= 2;
                for (int j = 0; j < Rorelse::spar; ++j)
                {
                    const float v = h[(size_t) ((m.skriv + j) % Rorelse::spar)];
                    const float y = env ? f.getBottom() - 3.0f - v * (f.getHeight() - 6.0f)
                                        : f.getCentreY() - v * f.getHeight() * 0.36f;
                    sist = { f.getX() + f.getWidth() * (float) j / (float) (Rorelse::spar - 1), y };
                    if (j == 0) p.startNewSubPath (sist); else p.lineTo (sist);
                }
                levandeLinje (g, p, env ? farg::skarmRod : farg::skarmCyan, 1.0f, 1.0f);
                g.setColour (farg::skarmVit);
                g.fillEllipse (sist.x - 2.6f, sist.y - 2.6f, 5.2f, 5.2f);
                g.setColour (farg::skarmVit.withAlpha (0.2f));
                g.fillEllipse (sist.x - 6.0f, sist.y - 6.0f, 12.0f, 12.0f);
            }
            break;
        }
    }
}

} // namespace km::vhs::scen
