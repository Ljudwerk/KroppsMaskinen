// Hårdvaran: chassit och reglagen på den svarta lådan, ritade som fria funktioner.
//
// Varje funktion ritar ett reglage i ett givet läge (värde, tänd/släckt) och känner inte till någon
// parameter. Fas 3 lägger tunna juce::Component-omslag runt dem; stilprovet anropar dem direkt.
// Ljuset faller uppifrån vänster. Allt mäts i designpixlar (1200 × 840-panelen).
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <set>
#include "Palett.h"
#include "Textur.h"
#include "Linjeskrift.h"

namespace km::stil
{

using juce::Colour;
using juce::Point;
using juce::Rectangle;
constexpr float pi = juce::MathConstants<float>::pi;

// Panelens två ytor (metall och plast), skapade en gång per skalfaktor och delade av alla reglage.
struct Ytor
{
    juce::Image metall, plast;
    float skala = 0.0f;

    void forbered (int w, int h, float s)
    {
        if (std::abs (s - skala) < 0.001f && metall.isValid() && metall.getWidth() >= (int) (w * s)) return;
        skala  = s;
        metall = borstadMetall (w, h, s, farg::chassi, 4.6f, 87u);
        plast  = plastnarv (w, h, s, farg::plast, 2.6f, 1987u);
    }
    juce::FillType metallFyll() const { return juce::FillType (metall, juce::AffineTransform::scale (1.0f / skala)); }
    juce::FillType plastFyll() const  { return juce::FillType (plast,  juce::AffineTransform::scale (1.0f / skala)); }
};

// ---------------------------------------------------------------------------------------------
// Tryck

// Silvertryck: tunna versaler med generös spärrning.
inline void tryck (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, float hojd = 6.0f,
                   juce::Justification just = juce::Justification::centred, Colour f = farg::silver,
                   float tjocklek = 0.8f, float spar = 2.4f)
{
    g.setColour (f);
    linjeskrift::rita (g, text, ruta, { hojd, tjocklek, spar, 1.0f }, just);
}

// Stora smala versaler (modellnamn, kanalnamn).
inline void rubrik (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, float hojd, Colour f,
                    juce::Justification just = juce::Justification::centredLeft, float tjocklek = 2.0f, float spar = 2.2f)
{
    g.setColour (f);
    linjeskrift::rita (g, text, ruta, { hojd, tjocklek, spar, 0.82f }, just);
}

// ---------------------------------------------------------------------------------------------
// Konstruktivistiska accenter

// En platt pil med rak skaft och trekantig spets (El Lissitzky snarare än pilikon).
inline void ritaPil (juce::Graphics& g, Point<float> fran, Point<float> till, float bredd, Colour f)
{
    const auto d = till - fran;
    const float len = d.getDistanceFromOrigin();
    if (len < 1.0f) return;
    const auto u = d / len, n = Point<float> (-u.y, u.x);
    const float spets = juce::jmin (len * 0.45f, bredd * 2.6f);
    const auto bas = till - u * spets;
    juce::Path p;
    p.startNewSubPath (fran + n * (bredd * 0.5f));
    p.lineTo (bas + n * (bredd * 0.5f));
    p.lineTo (bas + n * (bredd * 1.35f));
    p.lineTo (till);
    p.lineTo (bas - n * (bredd * 1.35f));
    p.lineTo (bas - n * (bredd * 0.5f));
    p.lineTo (fran - n * (bredd * 0.5f));
    p.closeSubPath();
    g.setColour (f);
    g.fillPath (p);
}

// Diagonala ränder i en ruta (varningsmärkning / dekor).
inline void ritaDiagonaler (juce::Graphics& g, Rectangle<float> ruta, Colour f, float bredd, float avstand, bool framat = true)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (ruta.toNearestInt());
    g.setColour (f);
    const float h = ruta.getHeight();
    for (float x = ruta.getX() - h; x < ruta.getRight() + h; x += avstand)
    {
        juce::Path p;
        if (framat) p.addQuadrilateral (x, ruta.getBottom(), x + bredd, ruta.getBottom(), x + bredd + h, ruta.getY(), x + h, ruta.getY());
        else        p.addQuadrilateral (x, ruta.getY(), x + bredd, ruta.getY(), x + bredd + h, ruta.getBottom(), x + h, ruta.getBottom());
        g.fillPath (p);
    }
}

// Ett parallellogram (lutande block), t.ex. bakom modellbeteckningen.
inline juce::Path parallellogram (Rectangle<float> r, float lutning)
{
    juce::Path p;
    p.addQuadrilateral (r.getX() + lutning, r.getY(), r.getRight(), r.getY(),
                        r.getRight() - lutning, r.getBottom(), r.getX(), r.getBottom());
    return p;
}

// ---------------------------------------------------------------------------------------------
// Chassi

// Infällt fält i svart plast: mörk överkant (skugga), ljus underkant (ljuset fångas av fasen).
inline void ritaInfalltFalt (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor, float radie = 3.0f)
{
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRoundedRectangle (r.expanded (1.0f).translated (0.0f, 1.0f), radie + 1.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.expanded (1.0f), radie + 1.0f);
    juce::Path form;
    form.addRoundedRectangle (r, radie);
    g.setFillType (ytor.plastFyll());
    g.fillPath (form);
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (form);
    g.setGradientFill ({ juce::Colours::black.withAlpha (0.55f), r.getX(), r.getY(),
                         juce::Colours::transparentBlack, r.getX(), r.getY() + 7.0f, false });
    g.fillRect (r.withHeight (8.0f));
}

// Krysspårsskruv.
inline void ritaSkruv (juce::Graphics& g, Point<float> c, float r, float vinkel = 0.4f)
{
    const auto ruta = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.fillEllipse (ruta.expanded (0.8f).translated (0.0f, 0.6f));
    g.setGradientFill ({ Colour (0xff5a5c62), c.x - r, c.y - r, Colour (0xff141416), c.x + r * 0.7f, c.y + r, false });
    g.fillEllipse (ruta);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    for (int i = 0; i < 2; ++i)
    {
        const float a = vinkel + (float) i * pi * 0.5f;
        const Point<float> u (std::cos (a), std::sin (a));
        g.drawLine ({ c - u * (r * 0.7f), c + u * (r * 0.7f) }, r * 0.28f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (ruta.reduced (0.4f), 0.6f);
}

// ---------------------------------------------------------------------------------------------
// Lysdioder

// Rund 3 mm-diod i svart infattning. styrka 0 = släckt, 1 = full.
inline void ritaLysdiod (juce::Graphics& g, Point<float> c, float r, Colour f, float styrka)
{
    const auto lins = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black);
    g.fillEllipse (lins.expanded (r * 0.45f));
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (lins.expanded (r * 0.45f), 0.6f);
    if (styrka > 0.01f)
    {
        juce::ColourGradient glod (f.withAlpha (0.42f * styrka), c, f.withAlpha (0.0f), c.translated (r * 5.0f, 0.0f), true);
        g.setGradientFill (glod);
        g.fillEllipse (lins.expanded (r * 4.0f));
        juce::ColourGradient kropp (f.brighter (0.9f).interpolatedWith (juce::Colours::white, 0.35f), c.translated (-r * 0.2f, -r * 0.25f),
                                    f.darker (0.35f), c.translated (r, r), true);
        kropp.addColour (0.45, f);
        g.setGradientFill (kropp);
        g.fillEllipse (lins);
    }
    else
    {
        juce::ColourGradient kropp (f.darker (2.4f).withMultipliedSaturation (0.8f), c.translated (-r * 0.3f, -r * 0.3f),
                                    Colour (0xff080202), c.translated (r, r), true);
        g.setGradientFill (kropp);
        g.fillEllipse (lins);
    }
    g.setColour (juce::Colours::white.withAlpha (styrka > 0.01f ? 0.55f : 0.28f));
    g.fillEllipse (Rectangle<float> (r * 0.7f, r * 0.45f).withCentre (c.translated (-r * 0.3f, -r * 0.45f)));
}

// LED-stapel (nivåmätare), nederst först. De två översta segmenten är röda.
inline void ritaLedStapel (juce::Graphics& g, Rectangle<float> r, float niva, int antal, bool lodrat = true)
{
    g.setColour (juce::Colours::black);
    g.fillRect (r.expanded (1.5f));
    const float steg = (lodrat ? r.getHeight() : r.getWidth()) / (float) antal;
    for (int i = 0; i < antal; ++i)
    {
        const bool pa = (float) (i + 1) / (float) antal <= niva + 0.001f;
        const Colour f = i >= antal - 2 ? farg::lysRod : (i >= antal - 4 ? farg::lysOrange : Colour (0xffffb21e));
        auto seg = lodrat ? Rectangle<float> (r.getX(), r.getBottom() - steg * (float) (i + 1), r.getWidth(), steg)
                          : Rectangle<float> (r.getX() + steg * (float) i, r.getY(), steg, r.getHeight());
        seg = seg.reduced (lodrat ? 0.0f : 0.8f, lodrat ? 0.8f : 0.0f);
        g.setColour (pa ? f : f.darker (3.2f).withMultipliedSaturation (0.7f));
        g.fillRect (seg);
        if (pa)
        {
            g.setColour (f.withAlpha (0.18f));
            g.fillRect (seg.expanded (1.5f));
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.fillRect (seg.withHeight (seg.getHeight() * 0.35f));
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Rattar

enum class RattTyp { liten, stor, rod };

// Svart rattkjol med räfflor, svarvad metallkåpa och silverstreck. varde 0..1 (−135° … +135°).
inline void ritaRatt (juce::Graphics& g, Point<float> c, float r, float varde, RattTyp typ = RattTyp::liten,
                      bool skalstreck = true, int antalStreck = 11)
{
    const float a0 = -pi * 0.75f, a1 = pi * 0.75f;
    auto polar = [c] (float a, float radie) { return c + Point<float> (std::sin (a), -std::cos (a)) * radie; };

    if (skalstreck)
    {
        for (int i = 0; i < antalStreck; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / (float) (antalStreck - 1);
            const bool stort = i == 0 || i == antalStreck - 1 || i == antalStreck / 2;
            g.setColour (stort ? farg::silver : farg::silverMatt);
            g.drawLine ({ polar (a, r * 1.17f), polar (a, r * (stort ? 1.42f : 1.32f)) }, stort ? 1.1f : 0.8f);
        }
    }

    // Skugga på panelen.
    {
        const auto sc = c.translated (r * 0.12f, r * 0.3f);
        juce::ColourGradient sk (juce::Colours::black.withAlpha (0.75f), sc, juce::Colours::black.withAlpha (0.0f),
                                 sc.translated (r * 1.3f, 0.0f), true);
        sk.addColour (0.6, juce::Colours::black.withAlpha (0.35f));
        g.setGradientFill (sk);
        g.fillEllipse (Rectangle<float> (r * 2.6f, r * 2.6f).withCentre (sc));
    }

    // Kjolen med räfflor.
    const auto kjol = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (Colour (0xff0e0e10));
    g.fillEllipse (kjol);
    const int raffor = juce::jmax (24, (int) (r * 2.6f));
    const float ljusVinkel = -pi * 0.3f;   // ljuset uppifrån vänster
    for (int i = 0; i < raffor; ++i)
    {
        const float a = 2.0f * pi * (float) i / (float) raffor;
        const float l = 0.5f + 0.5f * std::cos (a - ljusVinkel);
        g.setColour (Colour::greyLevel (0.05f + 0.30f * l * l));
        g.drawLine ({ polar (a, r * 0.8f), polar (a, r * 0.97f) }, juce::jmax (0.6f, r * 0.045f));
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (kjol.reduced (0.3f), 0.6f);

    // Kåpan: svarvad metall, ljusreflexen ligger som två motstående kilar (konisk glans).
    const float rk = r * (typ == RattTyp::stor ? 0.70f : 0.74f);
    const auto kapa = Rectangle<float> (rk * 2.0f, rk * 2.0f).withCentre (c);
    const Colour bas = typ == RattTyp::rod ? Colour (0xff5e0a0d) : Colour (0xff1b1b1f);
    const Colour glans = typ == RattTyp::rod ? Colour (0xffff6a5c) : (typ == RattTyp::stor ? Colour (0xffb4b7bd) : Colour (0xff8c8f96));
    g.setColour (juce::Colours::black);
    g.fillEllipse (kapa.expanded (0.8f));
    const int kilar = 120;
    for (int i = 0; i < kilar; ++i)
    {
        const float a = 2.0f * pi * (float) i / (float) kilar;
        const float s = 0.62f * std::pow (std::abs (std::cos (a + 0.55f)), 7.0f)
                      + 0.25f * std::pow (std::abs (std::cos (a - 0.95f)), 28.0f)
                      + 0.05f * std::sin (a * 9.0f) * std::sin (a * 9.0f);
        juce::Path kil;
        kil.addPieSegment (kapa, a, a + 2.0f * pi / (float) kilar + 0.01f, 0.0f);
        g.setColour (bas.interpolatedWith (glans, juce::jlimit (0.0f, 1.0f, s)));
        g.fillPath (kil);
    }
    // Svarvspår: fina koncentriska ringar.
    for (float rr = rk * 0.18f; rr < rk; rr += juce::jmax (0.9f, rk * 0.07f))
    {
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.drawEllipse (Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c), 0.4f);
    }
    {
        juce::ColourGradient rund (juce::Colours::white.withAlpha (0.10f), c.translated (-rk * 0.4f, -rk * 0.5f),
                                   juce::Colours::black.withAlpha (0.35f), c.translated (rk * 0.6f, rk * 0.8f), true);
        g.setGradientFill (rund);
        g.fillEllipse (kapa);
    }
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.35f), c.x, c.y - rk, juce::Colours::black.withAlpha (0.5f), c.x, c.y + rk, false });
    g.drawEllipse (kapa, 0.8f);

    // Silverstrecket, över kåpa och kjol.
    const float a = a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, varde);
    const float tj = juce::jmax (1.3f, r * 0.085f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine ({ polar (a, r * 0.30f).translated (0.4f, 0.8f), polar (a, r * 0.98f).translated (0.4f, 0.8f) }, tj + 0.6f);
    g.setColour (typ == RattTyp::rod ? farg::vitt : Colour (0xffe9ebef));
    g.drawLine ({ polar (a, r * 0.30f), polar (a, r * 0.98f) }, tj);
}

// ---------------------------------------------------------------------------------------------
// Knappar

// Liten rektangulär plastknapp (transport, vyer, kanaler). Ikonen/texten ritas av anroparen.
inline Rectangle<float> ritaKnapp (juce::Graphics& g, Rectangle<float> r, bool intryckt = false, Colour kropp = Colour (0xff232328))
{
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.expanded (1.5f), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (r.expanded (2.0f).translated (0.0f, 0.5f), 3.5f, 0.6f);
    auto k = intryckt ? r.translated (0.0f, 0.8f).withTrimmedBottom (0.8f) : r;
    const Colour topp = intryckt ? kropp.darker (0.25f) : kropp.brighter (0.25f);
    g.setColour (kropp.darker (0.8f));
    g.fillRoundedRectangle (k, 2.2f);
    auto yta = k.withTrimmedBottom (intryckt ? 1.0f : 2.5f).reduced (0.6f, 0.4f);
    g.setGradientFill ({ topp, yta.getX(), yta.getY(), kropp, yta.getX(), yta.getBottom(), false });
    g.fillRoundedRectangle (yta, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (intryckt ? 0.06f : 0.22f));
    g.drawLine (yta.getX() + 2.0f, yta.getY() + 0.5f, yta.getRight() - 2.0f, yta.getY() + 0.5f, 0.7f);
    return yta;
}

// TR-liknande stegknapp i gruppens färg. Fyrkantig, lätt konkav topp, synlig läpp mot underkanten.
inline void ritaStegknapp (juce::Graphics& g, Rectangle<float> r, int grupp, bool intryckt = false)
{
    const Colour f = farg::stegGrupp (grupp);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.expanded (2.0f), 3.5f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r.expanded (2.5f).translated (0.0f, 0.6f), 4.0f, 0.7f);

    auto k = intryckt ? r.translated (0.0f, 1.2f).withTrimmedBottom (1.2f) : r;
    g.setColour (f.darker (1.1f));
    g.fillRoundedRectangle (k, 2.5f);                                        // läppen
    auto topp = k.withTrimmedBottom (intryckt ? 2.0f : 4.5f).reduced (0.8f, 0.5f);
    const Colour fy = intryckt ? f.darker (0.2f) : f;
    g.setGradientFill ({ fy.brighter (0.18f), topp.getX(), topp.getY(), fy.darker (0.12f), topp.getX(), topp.getBottom(), false });
    g.fillRoundedRectangle (topp, 2.0f);
    {
        // Konkav topp: mörkare mitt, ljusare kanter.
        juce::ColourGradient skal (juce::Colours::black.withAlpha (0.10f), topp.getCentre(),
                                   juce::Colours::black.withAlpha (0.0f), topp.getCentre().translated (topp.getWidth() * 0.55f, 0.0f), true);
        g.setGradientFill (skal);
        g.fillRoundedRectangle (topp.reduced (2.0f), 2.0f);
    }
    // Fin matt struktur i plasten.
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (topp.toNearestInt());
        Slump sl ((uint32_t) (r.getX() * 13.0f + r.getY() * 7.0f + 1.0f));
        for (int i = 0; i < (int) (topp.getWidth() * topp.getHeight() * 0.12f); ++i)
        {
            g.setColour ((sl.enhet() < 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f));
            g.fillRect (topp.getX() + sl.enhet() * topp.getWidth(), topp.getY() + sl.enhet() * topp.getHeight(), 0.8f, 0.8f);
        }
    }
    g.setColour (juce::Colours::white.withAlpha (intryckt ? 0.12f : 0.4f));
    g.drawLine (topp.getX() + 2.0f, topp.getY() + 0.6f, topp.getRight() - 2.0f, topp.getY() + 0.6f, 0.8f);
}

// ---------------------------------------------------------------------------------------------
// Sjusegmentsdisplay

// Segmentbitar: a=1 b=2 c=4 d=8 e=16 f=32 g=64.
inline uint8_t segmentMask (juce::juce_wchar c)
{
    switch (c)
    {
        case '0': return 0x3f; case '1': return 0x06; case '2': return 0x5b; case '3': return 0x4f;
        case '4': return 0x66; case '5': case 'S': return 0x6d; case '6': return 0x7d; case '7': return 0x07;
        case '8': return 0x7f; case '9': return 0x6f; case '-': return 0x40; case 'A': return 0x77;
        case 'b': return 0x7c; case 'C': return 0x39; case 'c': return 0x58; case 'd': return 0x5e;
        case 'E': return 0x79; case 'F': return 0x71; case 'H': return 0x76; case 'h': return 0x74;
        case 'J': return 0x1e; case 'L': return 0x38; case 'n': return 0x54; case 'o': return 0x5c;
        case 'P': return 0x73; case 'r': return 0x50; case 't': return 0x78; case 'U': return 0x3e;
        case 'u': return 0x1c; case 'y': return 0x6e; case '_': return 0x08;
        default: return 0;
    }
}

// Segmentens former i en siffercell (utan lutning).
inline juce::Path segmentForm (Rectangle<float> cell, uint8_t mask)
{
    const float w = cell.getWidth(), h = cell.getHeight(), t = w * 0.19f, gap = t * 0.14f;
    const float x0 = cell.getX(), y0 = cell.getY();
    juce::Path p;
    auto vagrat = [&] (float y, float xa, float xb)
    {
        xa += t * 0.5f + gap; xb -= t * 0.5f + gap;
        p.startNewSubPath (xa, y); p.lineTo (xa + t * 0.5f, y - t * 0.5f); p.lineTo (xb - t * 0.5f, y - t * 0.5f);
        p.lineTo (xb, y); p.lineTo (xb - t * 0.5f, y + t * 0.5f); p.lineTo (xa + t * 0.5f, y + t * 0.5f); p.closeSubPath();
    };
    auto lodrat = [&] (float x, float ya, float yb)
    {
        ya += t * 0.5f + gap; yb -= t * 0.5f + gap;
        p.startNewSubPath (x, ya); p.lineTo (x + t * 0.5f, ya + t * 0.5f); p.lineTo (x + t * 0.5f, yb - t * 0.5f);
        p.lineTo (x, yb); p.lineTo (x - t * 0.5f, yb - t * 0.5f); p.lineTo (x - t * 0.5f, ya + t * 0.5f); p.closeSubPath();
    };
    const float xl = x0 + t * 0.5f, xr = x0 + w - t * 0.5f, yt = y0 + t * 0.5f, ym = y0 + h * 0.5f, yb = y0 + h - t * 0.5f;
    if (mask & 0x01) vagrat (yt, xl, xr);
    if (mask & 0x02) lodrat (xr, yt, ym);
    if (mask & 0x04) lodrat (xr, ym, yb);
    if (mask & 0x08) vagrat (yb, xl, xr);
    if (mask & 0x10) lodrat (xl, ym, yb);
    if (mask & 0x20) lodrat (xl, yt, ym);
    if (mask & 0x40) vagrat (ym, xl, xr);
    return p;
}

// Rita lysande segment: mjuk glöd, kropp och ljus kärna.
inline void ritaTandaSegment (juce::Graphics& g, const juce::Path& p, Colour f, float glod)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (f.withAlpha (0.07f));
        g.strokePath (p, juce::PathStrokeType ((float) i * glod, juce::PathStrokeType::curved));
    }
    g.setColour (f);
    g.fillPath (p);
    g.setColour (f.brighter (0.6f).withAlpha (0.45f));
    g.strokePath (p, juce::PathStrokeType (glod * 0.25f));
}

// Sjusegmentsdisplay bakom rödtonat glas. Texten tolkas så: siffror/bokstäver är celler, '.' tänder
// föregående cells decimalpunkt, ':' är ett smalt kolon, ' ' en släckt cell. Släckta segment syns svagt.
inline void ritaSjusegment (juce::Graphics& g, Rectangle<float> r, const juce::String& text,
                            Colour f = farg::segment, float sifferHojd = 0.0f, bool ram = true)
{
    if (ram)
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.expanded (2.5f).translated (0.0f, 0.6f), 3.5f, 0.6f);
        g.setGradientFill ({ farg::segmentGlas.brighter (0.15f), r.getX(), r.getY(), farg::segmentGlas.darker (0.5f), r.getX(), r.getBottom(), false });
        g.fillRoundedRectangle (r, 2.0f);
    }

    struct Cell { uint8_t mask; bool dp; bool kolon; };
    juce::Array<Cell> celler;
    for (auto p = text.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        if (c == '.' && ! celler.isEmpty() && ! celler.getLast().kolon) { celler.getReference (celler.size() - 1).dp = true; continue; }
        if (c == ':') { celler.add ({ 0, false, true }); continue; }
        celler.add ({ segmentMask (c), false, false });
    }
    const float h = sifferHojd > 0.0f ? sifferHojd : r.getHeight() * 0.62f;
    const float w = h * 0.52f, mellan = w * 0.38f, kolonB = w * 0.42f;
    float total = 0.0f;
    for (int i = 0; i < celler.size(); ++i) total += (celler[i].kolon ? kolonB : w) + (i > 0 ? mellan : 0.0f);
    const float lutning = 0.10f;
    float x = r.getCentreX() - total * 0.5f + h * lutning * 0.5f;
    const float y = r.getCentreY() - h * 0.5f;
    const auto skev = juce::AffineTransform::shear (-lutning, 0.0f).translated (lutning * (y + h), 0.0f);
    const float t = w * 0.19f;

    for (int i = 0; i < celler.size(); ++i)
    {
        const auto& c = celler[i];
        if (c.kolon)
        {
            juce::Path pa;
            const float d = t * 0.95f;
            pa.addEllipse (x + kolonB * 0.5f - d * 0.5f, y + h * 0.30f - d * 0.5f, d, d);
            pa.addEllipse (x + kolonB * 0.5f - d * 0.5f, y + h * 0.72f - d * 0.5f, d, d);
            pa.applyTransform (skev);
            ritaTandaSegment (g, pa, f, t * 0.9f);
            x += kolonB + mellan;
            continue;
        }
        const Rectangle<float> cell (x, y, w, h);
        auto spok = segmentForm (cell, 0x7f);
        spok.addEllipse (x + w + mellan * 0.25f, y + h - t, t, t);
        spok.applyTransform (skev);
        g.setColour (f.withAlpha (0.075f));
        g.fillPath (spok);
        auto pa = segmentForm (cell, c.mask);
        if (c.dp) pa.addEllipse (x + w + mellan * 0.25f, y + h - t, t, t);
        pa.applyTransform (skev);
        if (! pa.isEmpty()) ritaTandaSegment (g, pa, f, t * 0.9f);
        x += w + mellan;
    }

    if (ram)
    {
        // Glasreflex i övre halvan.
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.09f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.0f), r.getX(), r.getCentreY(), false });
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.5f).reduced (1.0f, 0.5f), 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
    }
}

// ---------------------------------------------------------------------------------------------
// Spak och fader

// Vågrät spak (SPÄNNINGEN): spår med skalstreck och ett brett handtag. varde 0..1.
inline void ritaSpak (juce::Graphics& g, Rectangle<float> r, float varde, Colour handtag = farg::tryckRott, int streck = 11)
{
    const float cy = r.getCentreY();
    const float marg = 14.0f;
    const auto spar = Rectangle<float> (r.getX() + marg, cy - 2.5f, r.getWidth() - 2.0f * marg, 5.0f);
    for (int i = 0; i < streck; ++i)
    {
        const float x = spar.getX() + spar.getWidth() * (float) i / (float) (streck - 1);
        const bool stort = i % 5 == 0;
        g.setColour (stort ? farg::silver : farg::silverMatt);
        g.drawLine (x, r.getY() + 1.0f, x, r.getY() + (stort ? 6.0f : 4.0f), stort ? 1.0f : 0.7f);
        g.drawLine (x, r.getBottom() - 1.0f, x, r.getBottom() - (stort ? 6.0f : 4.0f), stort ? 1.0f : 0.7f);
    }
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (spar, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (spar.getX() + 2.0f, spar.getBottom() + 0.6f, spar.getRight() - 2.0f, spar.getBottom() + 0.6f, 0.6f);

    const float x = spar.getX() + spar.getWidth() * juce::jlimit (0.0f, 1.0f, varde);
    const auto h = Rectangle<float> (24.0f, r.getHeight() - 12.0f).withCentre ({ x, cy });
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (h.translated (1.5f, 3.0f).expanded (1.0f), 3.0f);
    g.setColour (handtag.darker (1.2f));
    g.fillRoundedRectangle (h, 2.5f);
    auto topp = h.withTrimmedBottom (3.0f).reduced (1.0f, 0.5f);
    g.setGradientFill ({ handtag.brighter (0.15f), topp.getX(), topp.getY(), handtag.darker (0.25f), topp.getX(), topp.getBottom(), false });
    g.fillRoundedRectangle (topp, 2.0f);
    for (int i = -2; i <= 2; ++i)   // greppränder
    {
        const float gx = topp.getCentreX() + (float) i * 3.2f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (gx, topp.getY() + 3.0f, gx, topp.getBottom() - 3.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawLine (gx + 0.9f, topp.getY() + 3.0f, gx + 0.9f, topp.getBottom() - 3.0f, 0.5f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine (topp.getX() + 2.0f, topp.getY() + 0.6f, topp.getRight() - 2.0f, topp.getY() + 0.6f, 0.7f);
}

// Lodrät fader med svart lock och vitt streck. varde 0..1 (nederst = 0).
inline void ritaFader (juce::Graphics& g, Rectangle<float> r, float varde)
{
    const float cx = r.getCentreX(), marg = 9.0f;
    const auto spar = Rectangle<float> (cx - 2.0f, r.getY() + marg, 4.0f, r.getHeight() - 2.0f * marg);
    for (int i = 0; i <= 10; ++i)
    {
        const float y = spar.getBottom() - spar.getHeight() * (float) i / 10.0f;
        const bool stort = i % 5 == 0;
        g.setColour (stort ? farg::silver : farg::silverMatt);
        g.drawLine (r.getX() + 1.0f, y, r.getX() + (stort ? 7.0f : 5.0f), y, stort ? 1.0f : 0.7f);
        g.drawLine (r.getRight() - 1.0f, y, r.getRight() - (stort ? 7.0f : 5.0f), y, stort ? 1.0f : 0.7f);
    }
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (spar, 2.0f);
    const float y = spar.getBottom() - spar.getHeight() * juce::jlimit (0.0f, 1.0f, varde);
    const auto lock = Rectangle<float> (r.getWidth() - 14.0f, 16.0f).withCentre ({ cx, y });
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (lock.translated (1.0f, 3.0f).expanded (1.0f), 2.5f);
    g.setColour (Colour (0xff0a0a0c));
    g.fillRoundedRectangle (lock, 2.0f);
    g.setGradientFill ({ Colour (0xff3a3a40), lock.getX(), lock.getY(), Colour (0xff121215), lock.getX(), lock.getCentreY(), false });
    g.fillRoundedRectangle (lock.withHeight (lock.getHeight() * 0.5f).reduced (0.8f, 0.5f), 1.5f);
    g.setColour (Colour (0xff1c1c20));
    g.fillRoundedRectangle (lock.withTrimmedTop (lock.getHeight() * 0.5f).reduced (0.8f, 0.5f), 1.5f);
    g.setColour (farg::vitt);
    g.drawLine (lock.getX() + 2.0f, lock.getCentreY(), lock.getRight() - 2.0f, lock.getCentreY(), 1.2f);
}

// ---------------------------------------------------------------------------------------------
// VU-mätare

// Analog VU-mätare med visare, bakgrundsbelyst skala (svagt bärnsten) och glas. dB −20…+3.
// 'del' ritar en del i taget, så att panelen kan spara tavlan och glaset som bilder:
// 0 allt, 1 tavlan (allt under visaren), 2 bara visaren, 3 täckplåten och glaset.
inline void ritaVU (juce::Graphics& g, Rectangle<float> r, float dB, const juce::String& etikett, int del = 0)
{
    const bool tavla = del == 0 || del == 1, visare = del == 0 || del == 2, glas = del == 0 || del == 3;
    if (tavla)
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.expanded (2.5f).translated (0.0f, 0.6f), 3.5f, 0.6f);
    }
    const auto yta = r.reduced (3.0f);
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (yta.toNearestInt());

    const Point<float> nav (yta.getCentreX(), yta.getBottom() + yta.getHeight() * 0.38f);
    const float R = yta.getHeight() * 1.02f;
    auto vinkel = [] (float d)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        const float f = (std::pow (10.0f, juce::jlimit (-20.0f, 3.0f, d) / 20.0f) - lo) / (hi - lo);
        return juce::degreesToRadians (-46.0f + 92.0f * f);
    };
    auto polar = [nav] (float a, float radie) { return nav + Point<float> (std::sin (a), -std::cos (a)) * radie; };
    if (tavla)
    {
    {
        juce::ColourGradient ljus (Colour (0xff4a2a10), nav.translated (0.0f, -yta.getHeight() * 0.9f),
                                   Colour (0xff0e0806), nav.translated (yta.getWidth() * 0.75f, -yta.getHeight() * 0.9f), true);
        g.setGradientFill (ljus);
        g.fillRect (yta);
    }

    // Skalbågen: tunn silverbåge, tjock röd över 0 VU.
    {
        juce::Path svart, rod;
        svart.addCentredArc (nav.x, nav.y, R * 0.86f, R * 0.86f, 0.0f, vinkel (-20.0f), vinkel (0.0f), true);
        rod.addCentredArc (nav.x, nav.y, R * 0.875f, R * 0.875f, 0.0f, vinkel (0.0f), vinkel (3.0f), true);
        g.setColour (Colour (0xffe8dcc4));
        g.strokePath (svart, juce::PathStrokeType (0.9f));
        g.setColour (farg::lysRod);
        g.strokePath (rod, juce::PathStrokeType (3.2f));
    }
    const int markeringar[] { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (int m : markeringar)
    {
        const float a = vinkel ((float) m);
        const bool stor = m == -20 || m == -10 || m == -5 || m == 0 || m == 3;
        g.setColour (m > 0 ? farg::lysRod : Colour (0xffe8dcc4));
        g.drawLine ({ polar (a, R * 0.86f), polar (a, R * (stor ? 0.97f : 0.93f)) }, stor ? 1.1f : 0.8f);
        if (stor || m == -3)
        {
            const auto p = polar (a, R * 1.06f);
            g.setColour (m > 0 ? farg::lysRod : Colour (0xffe8dcc4));
            linjeskrift::rita (g, juce::String (std::abs (m)), Rectangle<float> (20.0f, 8.0f).withCentre (p),
                               { 5.0f, 0.75f, 1.6f, 1.0f }, juce::Justification::centred);
        }
    }
    g.setColour (Colour (0xffe8dcc4));
    linjeskrift::rita (g, "VU", Rectangle<float> (40.0f, 10.0f).withCentre (polar (0.0f, R * 0.55f)), { 8.0f, 1.0f, 2.0f, 1.0f });
    g.setColour (farg::silverMatt);
    linjeskrift::rita (g, etikett, Rectangle<float> (12.0f, 8.0f).withCentre ({ yta.getX() + 10.0f, yta.getBottom() - 9.0f }), { 6.0f, 0.9f, 2.0f, 1.0f });
    g.setColour (farg::lysRod.withAlpha (0.9f));
    linjeskrift::rita (g, "+", Rectangle<float> (10.0f, 8.0f).withCentre (polar (vinkel (3.0f) + 0.05f, R * 0.75f)), { 5.0f, 0.9f, 1.0f, 1.0f });

    }

    // Visaren med skugga.
    if (visare)
    {
    const float a = vinkel (dB);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine ({ polar (a, R * 0.3f).translated (2.0f, 2.5f), polar (a, R * 0.99f).translated (2.0f, 2.5f) }, 1.4f);
    g.setColour (Colour (0xfff4efe6));
    g.drawLine ({ polar (a, R * 0.3f), polar (a, R * 0.99f) }, 1.1f);

    }
    if (! glas) return;

    // Svart täckplåt över navet.
    {
        juce::Path plat;
        plat.addCentredArc (nav.x, nav.y, R * 0.47f, R * 0.47f, 0.0f, -pi * 0.5f, pi * 0.5f, true);
        plat.closeSubPath();
        g.setGradientFill ({ Colour (0xff1c1c20), nav.x, nav.y - R * 0.47f, Colour (0xff050506), nav.x, yta.getBottom(), false });
        g.fillPath (plat);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.strokePath (plat, juce::PathStrokeType (0.6f));
    }
    // Glas: diagonal reflex.
    {
        juce::Path reflex;
        reflex.addQuadrilateral (yta.getX(), yta.getY(), yta.getX() + yta.getWidth() * 0.55f, yta.getY(),
                                 yta.getX() + yta.getWidth() * 0.25f, yta.getBottom(), yta.getX(), yta.getBottom());
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.08f), yta.getX(), yta.getY(), juce::Colours::white.withAlpha (0.0f), yta.getCentreX(), yta.getBottom(), false });
        g.fillPath (reflex);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRect (yta, 1.0f);
    }
}

// ---------------------------------------------------------------------------------------------
// Tangentbord

// Klaviatur med benvita och svarta tangenter, röd filtlist överst. 'nedtryckta' är halvtoner från första C.
inline void ritaTangentbord (juce::Graphics& g, Rectangle<float> r, int antalVita, const std::set<int>& nedtryckta)
{
    g.setColour (juce::Colours::black);
    g.fillRect (r.expanded (2.0f));
    const float vb = r.getWidth() / (float) antalVita;
    static const int vitTon[7] { 0, 2, 4, 5, 7, 9, 11 };
    for (int i = 0; i < antalVita; ++i)
    {
        const int ton = (i / 7) * 12 + vitTon[i % 7];
        const bool ned = nedtryckta.count (ton) > 0;
        auto k = Rectangle<float> (r.getX() + (float) i * vb, r.getY(), vb, r.getHeight()).reduced (0.6f, 0.0f);
        if (ned) k = k.withTrimmedTop (1.0f);
        g.setGradientFill ({ ned ? Colour (0xffaaa59a) : Colour (0xffd6d2c8), k.getX(), k.getY(),
                             ned ? Colour (0xff938e84) : Colour (0xffb9b4a9), k.getX(), k.getBottom(), false });
        g.fillRoundedRectangle (k.withTrimmedTop (-2.0f), 2.0f);
        g.setColour (Colour (0xff8d887e));
        g.fillRect (k.withTop (k.getBottom() - (ned ? 2.0f : 4.0f)));
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (k.getX() + 0.5f, k.getY(), k.getX() + 0.5f, k.getBottom() - 4.0f, 0.6f);
        if (ned)
        {
            g.setGradientFill ({ juce::Colours::black.withAlpha (0.25f), k.getX(), k.getY(), juce::Colours::transparentBlack, k.getX(), k.getY() + 18.0f, false });
            g.fillRect (k.withHeight (18.0f));
        }
    }
    static const int svartEfter[5] { 0, 1, 3, 4, 5 };   // efter C, D, F, G, A
    static const int svartTon[5]   { 1, 3, 6, 8, 10 };
    for (int okt = 0; okt * 7 < antalVita; ++okt)
        for (int j = 0; j < 5; ++j)
        {
            const int vi = okt * 7 + svartEfter[j];
            if (vi + 1 >= antalVita) continue;
            const int ton = okt * 12 + svartTon[j];
            const bool ned = nedtryckta.count (ton) > 0;
            const float bb = vb * 0.6f, bh = r.getHeight() * 0.62f;
            auto k = Rectangle<float> (r.getX() + (float) (vi + 1) * vb - bb * 0.5f, r.getY(), bb, bh);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (k.translated (1.5f, 2.0f));
            g.setColour (Colour (0xff060607));
            g.fillRoundedRectangle (k, 1.5f);
            auto topp = k.reduced (1.6f, 0.0f).withTrimmedBottom (ned ? 3.0f : 6.0f);
            g.setGradientFill ({ Colour (0xff17171a), topp.getX(), topp.getY(), Colour (ned ? 0xff1d1d21 : 0xff2b2b30), topp.getX(), topp.getBottom(), false });
            g.fillRect (topp);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawLine (topp.getX() + 1.0f, topp.getBottom(), topp.getRight() - 1.0f, topp.getBottom(), 0.8f);
            g.drawLine (topp.getX() + 0.5f, topp.getY(), topp.getX() + 0.5f, topp.getBottom(), 0.5f);
        }
    // Röd filtlist och skugga från panelkanten.
    g.setColour (Colour (0xff560a0d));
    g.fillRect (r.withHeight (3.5f));
    g.setColour (Colour (0xff7a1014));
    g.drawLine (r.getX(), r.getY() + 3.2f, r.getRight(), r.getY() + 3.2f, 0.6f);
    g.setGradientFill ({ juce::Colours::black.withAlpha (0.5f), r.getX(), r.getY() + 3.5f, juce::Colours::transparentBlack, r.getX(), r.getY() + 12.0f, false });
    g.fillRect (r.withTop (r.getY() + 3.5f).withHeight (9.0f));
}

// ---------------------------------------------------------------------------------------------
// Ikoner för transportknapparna (ritas i ikonens färg av anroparen).

enum class Ikon { spolaBakat, spela, stopp, spelaIn, spolaFram, paus, bakat };

inline juce::Path ikon (Ikon typ, Rectangle<float> r)
{
    juce::Path p;
    const float s = juce::jmin (r.getWidth(), r.getHeight());
    const auto c = r.getCentre();
    switch (typ)
    {
        case Ikon::spela:
            p.addTriangle (c.x - s * 0.32f, c.y - s * 0.38f, c.x - s * 0.32f, c.y + s * 0.38f, c.x + s * 0.4f, c.y); break;
        case Ikon::bakat:
            p.addTriangle (c.x + s * 0.32f, c.y - s * 0.38f, c.x + s * 0.32f, c.y + s * 0.38f, c.x - s * 0.4f, c.y); break;
        case Ikon::stopp:
            p.addRectangle (Rectangle<float> (s * 0.62f, s * 0.62f).withCentre (c)); break;
        case Ikon::spelaIn:
            p.addEllipse (Rectangle<float> (s * 0.66f, s * 0.66f).withCentre (c)); break;
        case Ikon::paus:
            p.addRectangle (c.x - s * 0.3f, c.y - s * 0.32f, s * 0.2f, s * 0.64f);
            p.addRectangle (c.x + s * 0.1f, c.y - s * 0.32f, s * 0.2f, s * 0.64f); break;
        case Ikon::spolaBakat:
            p.addTriangle (c.x, c.y - s * 0.3f, c.x, c.y + s * 0.3f, c.x - s * 0.5f, c.y);
            p.addTriangle (c.x + s * 0.5f, c.y - s * 0.3f, c.x + s * 0.5f, c.y + s * 0.3f, c.x, c.y); break;
        case Ikon::spolaFram:
            p.addTriangle (c.x, c.y - s * 0.3f, c.x, c.y + s * 0.3f, c.x + s * 0.5f, c.y);
            p.addTriangle (c.x - s * 0.5f, c.y - s * 0.3f, c.x - s * 0.5f, c.y + s * 0.3f, c.x, c.y); break;
    }
    return p;
}

} // namespace km::stil
