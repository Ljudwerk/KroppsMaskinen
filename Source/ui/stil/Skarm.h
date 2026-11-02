// Skärmen: tjock-TV:n i panelens mitt. Trådramsscenen (lager 2), bildröret och glaset.
//
// Scenen ritas med vektorlinjer i rött, cyan och vitt på svart: ett golvrutnät mot horisonten, en
// fabrikssilhuett i fjärran och kanalens motiv – för Kolvbasen en kolv med vevstake och vevaxel i ett
// kylflänsat cylinderlopp, plus en vågform i ett oscilloskopfönster. Bilden går sedan genom bandet
// (Band.h) och bildröret (sveplinjer, buktande glas) innan den ritas i infattningen.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <cmath>
#include <functional>
#include "Palett.h"
#include "Textur.h"
#include "OSDTypsnitt.h"
#include "Hardvara.h"

namespace km::stil::skarm
{

// ---------------------------------------------------------------------------------------------
// Enkel 3D: en kamera som tittar längs +z, lätt nedåt. y är uppåt i världen.

struct V3 { float x = 0, y = 0, z = 0; };

struct Kamera
{
    Point<float> mitt;          // skärmpunkt för världens origo
    float brannvidd = 350.0f;   // pixlar per enhet på avståndet 1
    float avstand   = 9.0f;
    float lutning   = 0.18f;    // radianer nedåt

    float djup (V3 p) const { return -p.y * std::sin (lutning) + p.z * std::cos (lutning) + avstand; }
    Point<float> operator() (V3 p) const
    {
        const float y2 = p.y * std::cos (lutning) + p.z * std::sin (lutning);
        const float z2 = juce::jmax (0.05f, djup (p));
        return { mitt.x + brannvidd * p.x / z2, mitt.y - brannvidd * y2 / z2 };
    }
    float horisont() const { return mitt.y - brannvidd * std::tan (lutning); }
};

// Objektets placering: vridning kring y och förflyttning.
struct Placering
{
    float vridning = 0.0f; V3 pos;
    V3 operator() (V3 p) const
    {
        const float c = std::cos (vridning), s = std::sin (vridning);
        return { p.x * c + p.z * s + pos.x, p.y + pos.y, -p.x * s + p.z * c + pos.z };
    }
};

// Vektorlinje med fosforglöd: bred svag, mellan, och skarp kärna.
inline void glodlinje (juce::Graphics& g, const juce::Path& p, Colour f, float tj, float glod = 1.0f)
{
    const juce::PathStrokeType::JointStyle led = juce::PathStrokeType::curved;
    g.setColour (f.withAlpha (0.07f * glod)); g.strokePath (p, juce::PathStrokeType (tj * 6.0f, led, juce::PathStrokeType::rounded));
    g.setColour (f.withAlpha (0.18f * glod)); g.strokePath (p, juce::PathStrokeType (tj * 2.6f, led, juce::PathStrokeType::rounded));
    g.setColour (f);                          g.strokePath (p, juce::PathStrokeType (tj, led, juce::PathStrokeType::rounded));
    g.setColour (f.interpolatedWith (juce::Colours::white, 0.55f).withAlpha (0.7f));
    g.strokePath (p, juce::PathStrokeType (tj * 0.4f, led, juce::PathStrokeType::rounded));
}

// Projicerad cirkel i xz-planet (vågrät) eller xy-planet (stående).
inline void cirkel (juce::Path& p, const Kamera& k, const Placering& pl, V3 c, float r, bool vagrat, int seg = 48)
{
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * pi * (float) i / (float) seg;
        const V3 q = vagrat ? V3 { c.x + r * std::cos (a), c.y, c.z + r * std::sin (a) }
                            : V3 { c.x + r * std::cos (a), c.y + r * std::sin (a), c.z };
        const auto s = k (pl (q));
        if (i == 0) p.startNewSubPath (s); else p.lineTo (s);
    }
}

inline void linje (juce::Path& p, const Kamera& k, const Placering& pl, V3 a, V3 b)
{
    p.startNewSubPath (k (pl (a)));
    p.lineTo (k (pl (b)));
}

// ---------------------------------------------------------------------------------------------
// Scenens delar

// Golvrutnät i perspektiv mot horisonten, tonat mot fjärran.
inline void ritaGolv (juce::Graphics& g, Rectangle<float> r, const Kamera& k, float golvY, float fas = 0.0f)
{
    juce::Path langs, tvars;
    for (int i = -16; i <= 16; ++i)
        linje (langs, k, {}, { (float) i * 1.4f, golvY, -4.0f }, { (float) i * 1.4f, golvY, 80.0f });
    for (int j = 0; j < 40; ++j)
    {
        const float z = -4.0f + ((float) j + fas) * 1.4f;
        linje (tvars, k, {}, { -24.0f, golvY, z }, { 24.0f, golvY, z });
    }
    const float hor = k.horisont();
    auto ton = [&] (float a)
    {
        juce::ColourGradient t (farg::skarmRod.withAlpha (0.0f), r.getX(), hor, farg::skarmRod.withAlpha (a), r.getX(), r.getBottom(), false);
        t.addColour (0.25, farg::skarmRod.withAlpha (0.35f * a));
        return t;
    };
    for (auto* p : { &langs, &tvars })
    {
        g.setGradientFill (ton (0.12f)); g.strokePath (*p, juce::PathStrokeType (4.0f));
        g.setGradientFill (ton (0.35f)); g.strokePath (*p, juce::PathStrokeType (1.8f));
        g.setGradientFill (ton (1.0f));  g.strokePath (*p, juce::PathStrokeType (0.8f));
    }
    // Horisontlinjen.
    juce::Path h;
    h.startNewSubPath (r.getX(), hor); h.lineTo (r.getRight(), hor);
    glodlinje (g, h, farg::skarmRod, 1.0f, 0.8f);
}

// Fabrikssilhuett vid horisonten: sågtandstak, skorstenar och en gasklocka – i tunna linjer.
inline void ritaFabrik (juce::Graphics& g, Rectangle<float> r, float hor)
{
    juce::Path p;
    const float w = r.getWidth();
    float x = r.getX() + w * 0.03f;
    p.startNewSubPath (r.getX(), hor);
    p.lineTo (x, hor);
    // Sågtandstak (shedtak).
    for (int i = 0; i < 6; ++i)
    {
        p.lineTo (x, hor - 9.0f);
        p.lineTo (x + 16.0f, hor - 17.0f);
        p.lineTo (x + 16.0f, hor - 9.0f);
        x += 16.0f;
    }
    p.lineTo (x, hor);
    // Skorstenar.
    const float sk[] { 0.31f, 0.345f, 0.70f };
    const float skh[] { 62.0f, 48.0f, 70.0f };
    for (int i = 0; i < 3; ++i)
    {
        const float sx = r.getX() + w * sk[i];
        p.startNewSubPath (sx, hor); p.lineTo (sx + 2.0f, hor - skh[i]); p.lineTo (sx + 8.0f, hor - skh[i]); p.lineTo (sx + 10.0f, hor);
        for (int b = 1; b <= 3; ++b) { p.startNewSubPath (sx + 1.5f, hor - skh[i] * 0.25f * (float) b); p.lineTo (sx + 8.5f, hor - skh[i] * 0.25f * (float) b); }
    }
    // Hall med lanternin.
    const float hx = r.getX() + w * 0.76f;
    p.startNewSubPath (hx, hor); p.lineTo (hx, hor - 22.0f); p.lineTo (hx + 70.0f, hor - 22.0f); p.lineTo (hx + 70.0f, hor);
    p.startNewSubPath (hx + 20.0f, hor - 22.0f); p.lineTo (hx + 20.0f, hor - 29.0f); p.lineTo (hx + 50.0f, hor - 29.0f); p.lineTo (hx + 50.0f, hor - 22.0f);
    // Gasklocka (fackverk).
    const float gx = r.getX() + w * 0.88f, gb = 34.0f, gh = 30.0f;
    p.startNewSubPath (gx, hor); p.lineTo (gx, hor - gh); p.lineTo (gx + gb, hor - gh); p.lineTo (gx + gb, hor);
    for (int i = 0; i < 4; ++i)
    {
        const float a = gx + gb * (float) i / 4.0f, b = gx + gb * (float) (i + 1) / 4.0f;
        p.startNewSubPath (a, hor); p.lineTo (b, hor - gh);
        p.startNewSubPath (a, hor - gh); p.lineTo (b, hor);
    }
    glodlinje (g, p, farg::skarmRod.withAlpha (0.55f), 0.7f, 0.6f);
}

// Svaga himmelslinjer ovanför horisonten (avtagande täthet uppåt).
inline void ritaHimmel (juce::Graphics& g, Rectangle<float> r, float hor)
{
    float y = hor - 4.0f, d = 3.0f;
    for (int i = 0; i < 9 && y > r.getY(); ++i)
    {
        g.setColour (farg::skarmCyan.withAlpha (0.16f * (1.0f - (float) i / 9.0f)));
        g.drawLine (r.getX(), y, r.getRight(), y, 0.6f);
        y -= d; d *= 1.55f;
    }
}

// Kolven: kolv med ringspår, kolvbult, vevstake och vevaxel, i ett kylflänsat cylinderlopp.
// 'vevvinkel' i radianer (0 = övre dödläge).
inline void ritaKolv (juce::Graphics& g, const Kamera& k, const Placering& pl, float vevvinkel)
{
    const V3 vev { 0.0f, -1.9f, 0.0f };
    const float rv = 0.62f, L = 2.0f, rk = 0.92f;
    const V3 vevtapp { vev.x + rv * std::sin (vevvinkel), vev.y + rv * std::cos (vevvinkel), 0.0f };
    const float bultY = vev.y + rv * std::cos (vevvinkel) + std::sqrt (L * L - std::pow (rv * std::sin (vevvinkel), 2.0f));
    const float botten = bultY - 0.5f, topp = bultY + 0.68f;

    // Cylinderloppet med kylflänsar längs hela längden (rött, svagt).
    {
        juce::Path lopp, flansar;
        const float rl = rk + 0.12f, y0 = -0.35f, y1 = 2.05f;
        cirkel (lopp, k, pl, { 0, y0, 0 }, rl, true);
        cirkel (lopp, k, pl, { 0, y1, 0 }, rl, true);
        for (int i = 0; i < 8; ++i)
        {
            const float a = 2.0f * pi * (float) i / 8.0f + 0.2f;
            linje (lopp, k, pl, { rl * std::cos (a), y0, rl * std::sin (a) }, { rl * std::cos (a), y1, rl * std::sin (a) });
        }
        for (int f = 0; f < 8; ++f)
        {
            const float y = -0.1f + (float) f * 0.29f;
            cirkel (flansar, k, pl, { 0, y, 0 }, 1.36f, true, 56);
            for (int i = 0; i < 4; ++i)   // flänsarnas ytterkanter binds ihop i fyra punkter
            {
                const float a = pi * 0.5f * (float) i + 0.6f;
                if (f < 7) linje (flansar, k, pl, { 1.36f * std::cos (a), y, 1.36f * std::sin (a) },
                                                  { 1.36f * std::cos (a), y + 0.29f, 1.36f * std::sin (a) });
            }
        }
        glodlinje (g, flansar, farg::skarmRod.withAlpha (0.45f), 0.7f, 0.5f);
        glodlinje (g, lopp, farg::skarmRod.withAlpha (0.8f), 0.8f, 0.7f);

        // Topplock och tändstift.
        juce::Path t;
        cirkel (t, k, pl, { 0, y1 + 0.18f, 0 }, 0.75f, true);
        linje (t, k, pl, { 0, y1 + 0.18f, 0 }, { 0, y1 + 0.75f, 0 });
        cirkel (t, k, pl, { 0, y1 + 0.42f, 0 }, 0.16f, true, 6);
        cirkel (t, k, pl, { 0, y1 + 0.55f, 0 }, 0.16f, true, 6);
        glodlinje (g, t, farg::skarmVit, 0.8f, 0.6f);
    }

    // Vevaxeln: axel längs z, två vevslängar (motvikter), vevtapp.
    {
        juce::Path p;
        linje (p, k, pl, { vev.x, vev.y, -1.6f }, { vev.x, vev.y, 1.6f });
        for (float z : { -0.32f, 0.32f })
        {
            // Vevskivorna: hel skiva med navet och en mindre ring runt vevtappen.
            cirkel (p, k, pl, { vev.x, vev.y, z }, 0.95f, false, 40);
            cirkel (p, k, pl, { vev.x, vev.y, z }, 0.2f, false, 16);
            cirkel (p, k, pl, { vevtapp.x, vevtapp.y, z }, 0.26f, false, 16);
        }
        for (int i = 0; i < 6; ++i)   // skivornas kanter binds ihop
        {
            const float a = 2.0f * pi * (float) i / 6.0f + vevvinkel;
            linje (p, k, pl, { vev.x + 0.95f * std::sin (a), vev.y + 0.95f * std::cos (a), -0.32f },
                             { vev.x + 0.95f * std::sin (a), vev.y + 0.95f * std::cos (a), 0.32f });
        }
        cirkel (p, k, pl, { vevtapp.x, vevtapp.y, 0.0f }, 0.16f, false, 16);
        glodlinje (g, p, farg::skarmCyan.withAlpha (0.85f), 0.9f, 0.8f);
    }

    // Vevstaken: storända runt vevtappen, två balkar upp till kolvbulten.
    {
        juce::Path p;
        const V3 bult { 0.0f, bultY, 0.0f };
        for (float z : { -0.14f, 0.14f })
        {
            cirkel (p, k, pl, { vevtapp.x, vevtapp.y, z }, 0.34f, false, 24);
            cirkel (p, k, pl, { bult.x, bult.y, z }, 0.2f, false, 16);
            const auto d = Point<float> (bult.x - vevtapp.x, bult.y - vevtapp.y);
            const auto n = Point<float> (-d.y, d.x) / d.getDistanceFromOrigin();
            for (float s : { -1.0f, 1.0f })
                linje (p, k, pl, { vevtapp.x + n.x * 0.2f * s, vevtapp.y + n.y * 0.2f * s, z },
                                 { bult.x + n.x * 0.12f * s, bult.y + n.y * 0.12f * s, z });
        }
        glodlinje (g, p, farg::skarmVit, 0.9f, 0.7f);
    }

    // Kolven: mantel, ringspår och krona.
    {
        juce::Path p, ringar;
        cirkel (p, k, pl, { 0, botten, 0 }, rk, true, 64);
        cirkel (p, k, pl, { 0, topp, 0 }, rk, true, 64);
        for (int i = 0; i < 16; ++i)
        {
            const float a = 2.0f * pi * (float) i / 16.0f;
            linje (p, k, pl, { rk * std::cos (a), botten, rk * std::sin (a) }, { rk * std::cos (a), topp, rk * std::sin (a) });
        }
        // Kronan: urfräsning och ventilfickor.
        cirkel (p, k, pl, { 0, topp, 0 }, rk * 0.62f, true, 48);
        cirkel (p, k, pl, { -0.35f, topp, 0.0f }, 0.2f, true, 20);
        cirkel (p, k, pl, { 0.35f, topp, 0.0f }, 0.2f, true, 20);
        // Kolvbulten (genom manteln).
        cirkel (p, k, pl, { 0, bultY, -rk }, 0.2f, false, 20);
        linje (p, k, pl, { 0, bultY + 0.2f, -rk - 0.05f }, { 0, bultY + 0.2f, rk + 0.05f });
        linje (p, k, pl, { 0, bultY - 0.2f, -rk - 0.05f }, { 0, bultY - 0.2f, rk + 0.05f });
        glodlinje (g, p, farg::skarmCyan, 1.0f, 1.0f);
        for (float y : { topp - 0.12f, topp - 0.22f, topp - 0.32f })
            cirkel (ringar, k, pl, { 0, y, 0 }, rk + 0.015f, true, 64);
        glodlinje (g, ringar, farg::skarmVit, 1.0f, 1.0f);
    }
}

// Oscilloskopfönster med Kolvbasens vågform: fyra filtrerade sågtandstoner med accent och fall.
inline void ritaVagform (juce::Graphics& g, Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.fillRect (r);
    // Prickat rutnät.
    for (int i = 1; i < 8; ++i)
        for (int j = 1; j < 4; ++j)
        {
            g.setColour (farg::skarmCyan.withAlpha (j == 2 ? 0.35f : 0.18f));
            g.fillRect (r.getX() + r.getWidth() * (float) i / 8.0f - 0.5f, r.getY() + r.getHeight() * (float) j / 4.0f - 0.5f, 1.0f, 1.0f);
        }
    g.setColour (farg::skarmCyan.withAlpha (0.18f));
    g.drawLine (r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 0.5f);

    // Hörnvinklar (siktesmarkeringar).
    juce::Path hv;
    const float l = 9.0f;
    struct Horn { float x, y, dx, dy; };
    const Horn horn[] { { r.getX(), r.getY(), 1.0f, 1.0f }, { r.getRight(), r.getY(), -1.0f, 1.0f },
                        { r.getX(), r.getBottom(), 1.0f, -1.0f }, { r.getRight(), r.getBottom(), -1.0f, -1.0f } };
    for (const auto& h : horn)
    {
        hv.startNewSubPath (h.x + h.dx * l, h.y); hv.lineTo (h.x, h.y); hv.lineTo (h.x, h.y + h.dy * l);
    }
    glodlinje (g, hv, farg::skarmVit, 1.0f, 0.6f);

    // Vågformen.
    juce::Path v, env;
    const int n = (int) r.getWidth() * 2;
    float lp = 0.0f, lp2 = 0.0f;
    const float accent[4] { 1.0f, 0.55f, 0.8f, 0.5f };
    const float tonhojd[4] { 6.0f, 6.0f, 9.0f, 12.0f };
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n * 4.0f;
        const int ton = juce::jmin (3, (int) t);
        const float fas = t - (float) ton;
        const float e = accent[ton] * std::exp (-fas * 2.2f) * (fas < 0.72f ? 1.0f : std::exp (-(fas - 0.72f) * 25.0f));
        const float saw = 2.0f * (fas * tonhojd[ton] - std::floor (fas * tonhojd[ton])) - 1.0f;
        const float brytning = 0.10f + 0.5f * e * e;          // filtret öppnar med envelopet
        lp  += (saw - lp) * brytning;
        lp2 += (lp - lp2) * brytning;
        const float y = r.getCentreY() - lp2 * e * r.getHeight() * 0.44f;
        const float x = r.getX() + r.getWidth() * (float) i / (float) n;
        if (i == 0) v.startNewSubPath (x, y); else v.lineTo (x, y);
        const float ey = r.getCentreY() - e * r.getHeight() * 0.44f;
        if (i == 0) env.startNewSubPath (x, ey); else env.lineTo (x, ey);
    }
    juce::Path streckad;
    const float monster[] { 3.0f, 3.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (streckad, env, monster, 2);
    g.setColour (farg::skarmRod.withAlpha (0.8f));
    g.fillPath (streckad);
    glodlinje (g, v, farg::skarmCyan, 1.1f, 1.0f);

    // Stegmarkeringar under vågen.
    for (int i = 0; i < 4; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / 4.0f;
        g.setColour (farg::skarmVit.withAlpha (0.6f));
        g.fillRect (x, r.getBottom() - 4.0f, 1.0f, 4.0f);
    }
}

// Kolvbasens sexton steg som staplar på golvet: höjd = ton, rött = accent, vitt = spelhuvudet.
inline void ritaStegStaplar (juce::Graphics& g, const Kamera& k, float golvY, float z, int spelhuvud = 5)
{
    const float hojd[16] { 0.9f, 0, 0.9f, 1.5f, 0, 0, 0.6f, 0, 0.9f, 0, 1.2f, 0.9f, 0, 0, 1.8f, 0 };
    const bool accent[16] { true, false, false, false, false, false, false, false, true, false, false, false, false, false, true, false };
    for (int i = 0; i < 16; ++i)
    {
        const float x0 = 0.8f + (float) i * 0.5f, x1 = x0 + 0.34f, z0 = z, z1 = z + 0.34f;
        const float h = hojd[i] > 0.0f ? hojd[i] * 1.25f : 0.06f;
        juce::Path p;
        const V3 b[4] { { x0, golvY, z0 }, { x1, golvY, z0 }, { x1, golvY, z1 }, { x0, golvY, z1 } };
        for (int j = 0; j < 4; ++j)
        {
            const V3 a = b[j], c = b[(j + 1) % 4];
            linje (p, k, {}, a, c);
            linje (p, k, {}, { a.x, a.y + h, a.z }, { c.x, c.y + h, c.z });
            linje (p, k, {}, a, { a.x, a.y + h, a.z });
        }
        const Colour f = i == spelhuvud ? farg::skarmVit : (accent[i] ? farg::skarmRod : farg::skarmCyan);
        glodlinje (g, p, hojd[i] > 0.0f || i == spelhuvud ? f : f.withAlpha (0.35f), 0.8f, 0.8f);
    }
}

// Hela Kolvbasen-scenen i rutan r (skärmens koordinater, origo uppe till vänster).
inline void ritaKolvbasScen (juce::Graphics& g, Rectangle<float> r, float vevvinkel = 0.95f)
{
    // Svart med en svag fosforton mot mitten.
    g.setColour (farg::skarmSvart);
    g.fillRect (r);
    {
        juce::ColourGradient fosfor (Colour (0xff0b1014), r.getCentre(), farg::skarmSvart, r.getTopLeft(), true);
        g.setGradientFill (fosfor);
        g.fillRect (r);
    }
    // Golvet och objektet har var sin kamera: golvet ses nästan från sidan (låg horisont),
    // kolven lite uppifrån så att kronan och ringarna syns.
    Kamera golv;
    golv.mitt = { r.getCentreX(), r.getY() + r.getHeight() * 0.62f };
    golv.brannvidd = r.getHeight() * 1.1f;
    golv.lutning = 0.18f;
    const float hor = golv.horisont();
    ritaHimmel (g, r, hor);
    ritaFabrik (g, r, hor);
    ritaGolv (g, r, golv, -3.2f, 0.35f);
    ritaStegStaplar (g, golv, -3.2f, 7.0f);

    ritaVagform (g, { r.getX() + r.getWidth() * 0.60f, r.getY() + r.getHeight() * 0.10f,
                      r.getWidth() * 0.35f, r.getHeight() * 0.30f });

    Kamera obj;
    obj.mitt = { r.getCentreX(), r.getY() + r.getHeight() * 0.535f };
    obj.brannvidd = r.getHeight() * 1.30f;
    obj.lutning = 0.30f;
    Placering pl;
    pl.vridning = 0.55f;
    pl.pos = { -1.9f, 0.25f, 0.0f };
    ritaKolv (g, obj, pl, vevvinkel);
}

// ---------------------------------------------------------------------------------------------
// Bildröret

// Rutan som bildrörets glas har: rundade hörn och lätt utbuktande sidor.
inline juce::Path glasForm (Rectangle<float> r, float radie = 26.0f, float buk = 4.0f)
{
    juce::Path p;
    const float x0 = r.getX(), y0 = r.getY(), x1 = r.getRight(), y1 = r.getBottom(), cx = r.getCentreX(), cy = r.getCentreY();
    p.startNewSubPath (x0 + radie, y0);
    p.quadraticTo (cx, y0 - buk, x1 - radie, y0);
    p.quadraticTo (x1, y0, x1, y0 + radie);
    p.quadraticTo (x1 + buk, cy, x1, y1 - radie);
    p.quadraticTo (x1, y1, x1 - radie, y1);
    p.quadraticTo (cx, y1 + buk, x0 + radie, y1);
    p.quadraticTo (x0, y1, x0, y1 - radie);
    p.quadraticTo (x0 - buk, cy, x0, y0 + radie);
    p.quadraticTo (x0, y0, x0 + radie, y0);
    p.closeSubPath();
    return p;
}

// Sveplinjer och utbuktning (bildrörets egen geometri). Ändrar bilden på plats.
inline void bildror (juce::Image& bild, float skala, float linjeStyrka = 0.30f, float buk = 0.045f)
{
    const int W = bild.getWidth(), H = bild.getHeight();
    juce::Image kopia = bild.createCopy();
    juce::Image::BitmapData in (kopia, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData ut (bild, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < H; ++y)
    {
        // Sveplinjens profil: en mörk linje varannan designpixel, mjukt avrundad i högre skala.
        const float yd = (float) y / skala;
        const float linje = 1.0f - linjeStyrka * (0.5f - 0.5f * std::cos (2.0f * pi * (yd + 0.5f) / 2.0f));
        for (int x = 0; x < W; ++x)
        {
            const float u = (float) x / (float) (W - 1) * 2.0f - 1.0f, v = (float) y / (float) (H - 1) * 2.0f - 1.0f;
            const float r2 = u * u * 0.8f + v * v * 0.8f;
            const float f = 1.0f - buk + buk * r2;     // inåt i mitten → bilden buktar ut
            const float sx = ((u * f + 1.0f) * 0.5f) * (float) (W - 1), sy = ((v * f + 1.0f) * 0.5f) * (float) (H - 1);
            const int ix = (int) std::floor (sx), iy = (int) std::floor (sy);
            const float fx = sx - (float) ix, fy = sy - (float) iy;
            float rr = 0, gg = 0, bb = 0;
            for (int j = 0; j < 2; ++j)
                for (int i = 0; i < 2; ++i)
                {
                    const int px = juce::jlimit (0, W - 1, ix + i), py = juce::jlimit (0, H - 1, iy + j);
                    const float w = (i ? fx : 1.0f - fx) * (j ? fy : 1.0f - fy);
                    const auto c = in.getPixelColour (px, py);
                    rr += w * c.getRed(); gg += w * c.getGreen(); bb += w * c.getBlue();
                }
            ut.setPixelColour (x, y, juce::Colour (tillByte (rr * linje), tillByte (gg * linje), tillByte (bb * linje)));
        }
    }
}

// Glaset ovanpå bilden: vinjettering, inre kantskugga och reflexer från rummet.
inline void ritaGlas (juce::Graphics& g, Rectangle<float> r, const juce::Path& form)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (form);
    {
        juce::ColourGradient vinj (juce::Colours::transparentBlack, r.getCentre(), juce::Colours::black.withAlpha (0.78f),
                                   r.getTopLeft().translated (-r.getWidth() * 0.06f, 0.0f), true);
        vinj.addColour (0.55, juce::Colours::transparentBlack);
        vinj.addColour (0.82, juce::Colours::black.withAlpha (0.25f));
        g.setGradientFill (vinj);
        g.fillRect (r.expanded (8.0f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.strokePath (form, juce::PathStrokeType (5.0f));
    // Stor mjuk reflex uppe till vänster (ett fönster i rummet).
    {
        juce::Path refl;
        refl.addRoundedRectangle (r.getX() + r.getWidth() * 0.04f, r.getY() + r.getHeight() * 0.05f,
                                  r.getWidth() * 0.34f, r.getHeight() * 0.32f, 18.0f);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                             juce::Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.3f, r.getY() + r.getHeight() * 0.38f, false });
        g.fillPath (refl);
    }
    // Smal blank båge längs överkanten.
    {
        juce::Path bage;
        bage.startNewSubPath (r.getX() + r.getWidth() * 0.08f, r.getY() + 9.0f);
        bage.quadraticTo (r.getCentreX(), r.getY() + 2.0f, r.getRight() - r.getWidth() * 0.08f, r.getY() + 9.0f);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.0f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.0f), r.getRight(), r.getY(), false });
        juce::ColourGradient bg (juce::Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.08f, 0.0f,
                                 juce::Colours::white.withAlpha (0.0f), r.getRight() - r.getWidth() * 0.08f, 0.0f, false);
        bg.addColour (0.3, juce::Colours::white.withAlpha (0.16f));
        bg.addColour (0.6, juce::Colours::white.withAlpha (0.05f));
        g.setGradientFill (bg);
        g.strokePath (bage, juce::PathStrokeType (2.0f));
    }
    // Diagonal glans över hela glaset.
    {
        juce::Path d;
        d.addQuadrilateral (r.getX() + r.getWidth() * 0.55f, r.getY(), r.getX() + r.getWidth() * 0.68f, r.getY(),
                            r.getX() + r.getWidth() * 0.43f, r.getBottom(), r.getX() + r.getWidth() * 0.30f, r.getBottom());
        g.setColour (juce::Colours::white.withAlpha (0.022f));
        g.fillPath (d);
    }
}

// Infattningen: tjock svart plast med fasad öppning mot glaset.
inline void ritaInfattning (juce::Graphics& g, Rectangle<float> yttre, Rectangle<float> glas, const Ytor& ytor)
{
    // Skugga och kropp.
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (yttre.translated (0.0f, 3.0f).expanded (2.0f), 10.0f);
    juce::Path kropp;
    kropp.addRoundedRectangle (yttre, 9.0f);
    g.setFillType (ytor.plastFyll());
    g.fillPath (kropp);
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.07f), yttre.getX(), yttre.getY(),
                         juce::Colours::black.withAlpha (0.3f), yttre.getX(), yttre.getBottom(), false });
    g.fillPath (kropp);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawLine (yttre.getX() + 9.0f, yttre.getY() + 0.7f, yttre.getRight() - 9.0f, yttre.getY() + 0.7f, 0.8f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (yttre, 9.0f, 1.0f);

    // Fasen in mot glaset: ljus nedtill (fångar ljus), mörk upptill.
    const auto fas = glas.expanded (9.0f);
    juce::Path fasForm = glasForm (fas, 32.0f, 4.0f);
    g.setGradientFill ({ Colour (0xff050506), fas.getX(), fas.getY(), Colour (0xff2c2c31), fas.getX(), fas.getBottom(), false });
    g.fillPath (fasForm);
    g.setColour (juce::Colours::black);
    g.fillPath (glasForm (glas.expanded (2.0f), 27.0f, 4.0f));
}

} // namespace km::stil::skarm
