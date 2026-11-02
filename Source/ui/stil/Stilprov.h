// Stilprovet: hela VHS-panelen (1200 × 840) och ett detaljark, ritade med fasta exempelvärden.
//
// Ingen koppling till processorn eller parametrarna – bara bilder. Varje sektion är en egen funktion
// med sin ruta, så att fas 3 kan lyfta över dem en och en till riktiga komponenter.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Palett.h"
#include "Textur.h"
#include "Linjeskrift.h"
#include "OSDTypsnitt.h"
#include "Hardvara.h"
#include "Skarm.h"
#include "Band.h"

namespace km::stil::prov
{

constexpr int designBredd = 1200, designHojd = 840;

// Skåra mellan två sektioner av chassit: mörk linje med ljus kant under.
inline void skara (juce::Graphics& g, float y, float x0, float x1)
{
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.fillRect (x0, y - 1.0f, x1 - x0, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRect (x0, y + 1.0f, x1 - x0, 1.0f);
}

// Lysdiod med silvertryck bredvid (t.ex. SP / LP / EP).
inline void diodMedText (juce::Graphics& g, Point<float> c, const juce::String& text, bool pa, Colour f = farg::lysRod)
{
    ritaLysdiod (g, c, 2.4f, f, pa ? 1.0f : 0.0f);
    tryck (g, text, { c.x + 7.0f, c.y - 4.0f, 60.0f, 8.0f }, 5.5f, juce::Justification::centredLeft, pa ? farg::silver : farg::silverMatt);
}

// Ratt med etikett under.
inline void rattMedEtikett (juce::Graphics& g, Point<float> c, float r, float varde, const juce::String& etikett,
                            RattTyp typ = RattTyp::liten)
{
    ritaRatt (g, c, r, varde, typ);
    tryck (g, etikett, { c.x - 45.0f, c.y + r * 1.42f + 6.0f, 90.0f, 8.0f }, 5.6f);
}

// Rubrik för en reglagegrupp: text med tunn linje åt båda håll och nedåtvinklar i ändarna.
inline void gruppRubrik (juce::Graphics& g, const juce::String& text, float x0, float x1, float y)
{
    const linjeskrift::Stil s { 5.6f, 0.8f, 2.4f, 1.0f };
    const float b = linjeskrift::bredd (text, s);
    const float cx = (x0 + x1) * 0.5f;
    g.setColour (farg::silver);
    linjeskrift::rita (g, text, cx - b * 0.5f, y - s.hojd * 0.5f, s);
    g.setColour (farg::silverMatt);
    g.drawLine (x0, y, cx - b * 0.5f - 6.0f, y, 0.8f);
    g.drawLine (cx + b * 0.5f + 6.0f, y, x1, y, 0.8f);
    g.drawLine (x0, y, x0, y + 4.0f, 0.8f);
    g.drawLine (x1, y, x1, y + 4.0f, 0.8f);
}

// ---------------------------------------------------------------------------------------------
// Bandverket (transport, överst)

inline void ritaBandverket (juce::Graphics& g, Rectangle<float> r)
{
    // Lite ljusare list och skåra mot resten.
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.045f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getBottom(), false });
    g.fillRect (r);
    skara (g, r.getBottom(), r.getX(), r.getRight());

    // Modellnamnet: stora smala versaler och ett rött block med modellnumret.
    rubrik (g, "KROPPSMASKINEN", { 26.0f, 18.0f, 300.0f, 24.0f }, 24.0f, farg::vitt, juce::Justification::centredLeft, 2.4f, 2.0f);
    tryck (g, sv ("EBM · NEW BEAT · RYTM- OCH BASMASKIN"), { 27.0f, 52.0f, 300.0f, 8.0f }, 5.5f, juce::Justification::centredLeft, farg::silverMatt);
    {
        const Rectangle<float> blk (270.0f, 18.0f, 82.0f, 24.0f);
        g.setColour (farg::tryckRott);
        g.fillPath (parallellogram (blk, 9.0f));
        rubrik (g, "KM-87", blk.translated (1.0f, 0.0f), 14.0f, farg::vitt, juce::Justification::centred, 2.0f, 2.2f);
        ritaDiagonaler (g, { 352.0f, 18.0f, 30.0f, 24.0f }, farg::tryckRott, 3.0f, 8.0f, true);
        g.setColour (farg::tryckRott);
        g.fillRect (270.0f, 48.0f, 112.0f, 1.5f);
        ritaPil (g, { 330.0f, 56.0f }, { 382.0f, 56.0f }, 1.5f, farg::tryckRott);
    }

    // Transporten.
    const char* etiketter[] { "SPOLA", "SPELA", "STOPP", "SPELA IN" };
    const Ikon ikoner[] { Ikon::spolaBakat, Ikon::spela, Ikon::stopp, Ikon::spelaIn };
    for (int i = 0; i < 4; ++i)
    {
        const Rectangle<float> k (408.0f + (float) i * 50.0f, 26.0f, 42.0f, 28.0f);
        tryck (g, etiketter[i], { k.getX() - 6.0f, 10.0f, k.getWidth() + 12.0f, 8.0f }, 5.5f);
        const bool aktiv = i == 1;
        auto yta = ritaKnapp (g, k, aktiv);
        const auto p = ikon (ikoner[i], yta.reduced (10.0f, 5.0f));
        if (aktiv || i == 3)
        {
            g.setColour (farg::lysRod.withAlpha (aktiv ? 0.25f : 0.0f));
            g.strokePath (p, juce::PathStrokeType (3.0f));
            g.setColour (i == 3 ? farg::tryckRott : farg::lysRod);
        }
        else g.setColour (farg::silver);
        g.fillPath (p);
        if (aktiv) ritaLysdiod (g, { k.getRight() - 6.0f, k.getY() + 6.0f }, 1.8f, farg::lysRod, 1.0f);
    }

    // Räkneverket och bandhastigheten.
    tryck (g, sv ("RÄKNEVERK"), { 612.0f, 10.0f, 120.0f, 8.0f }, 5.5f);
    ritaSjusegment (g, { 612.0f, 24.0f, 120.0f, 32.0f }, "0:12:04");
    diodMedText (g, { 748.0f, 28.0f }, "SP", true, farg::lysOrange);
    diodMedText (g, { 748.0f, 40.0f }, "LP", false, farg::lysOrange);
    diodMedText (g, { 748.0f, 52.0f }, "EP", false, farg::lysOrange);

    // Tempo.
    tryck (g, "TEMPO", { 784.0f, 10.0f, 100.0f, 8.0f }, 5.5f);
    ritaSjusegment (g, { 784.0f, 24.0f, 100.0f, 32.0f }, "125.0");
    tryck (g, "SLAG/MIN", { 784.0f, 62.0f, 100.0f, 8.0f }, 5.0f, juce::Justification::centred, farg::silverMatt);

    // Varvtalet: 45 / 33 / 33 + 8 %.
    tryck (g, "VARVTALET", { 900.0f, 10.0f, 120.0f, 8.0f }, 5.5f);
    const char* varv[] { "45", "33", "+8%" };
    for (int i = 0; i < 3; ++i)
    {
        const Rectangle<float> k (904.0f + (float) i * 40.0f, 34.0f, 32.0f, 18.0f);
        ritaLysdiod (g, { k.getCentreX(), 26.0f }, 2.2f, farg::lysOrange, i == 0 ? 1.0f : 0.0f);
        auto yta = ritaKnapp (g, k, i == 0);
        tryck (g, varv[i], yta, 6.0f, juce::Justification::centred, i == 0 ? farg::vitt : farg::silver, 0.9f, 1.6f);
    }

    // Spänningen: makrospaken.
    g.setColour (farg::tryckRott);
    linjeskrift::rita (g, sv ("SPÄNNINGEN"), { 1036.0f, 10.0f, 146.0f, 8.0f }, { 6.0f, 1.1f, 2.4f, 1.0f }, juce::Justification::centred);
    ritaSpak (g, { 1036.0f, 24.0f, 146.0f, 34.0f }, 0.62f);
    tryck (g, "LUGN", { 1036.0f, 62.0f, 60.0f, 8.0f }, 5.0f, juce::Justification::centredLeft, farg::silverMatt);
    tryck (g, "HELVETE", { 1122.0f, 62.0f, 60.0f, 8.0f }, 5.0f, juce::Justification::centredRight, farg::silverMatt);
}

// ---------------------------------------------------------------------------------------------
// Kanalväljaren (vänster)

inline void ritaKanalvaljare (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor, int vald = 0)
{
    ritaInfalltFalt (g, r, ytor);
    tryck (g, sv ("KANALVÄLJARE"), { r.getX(), r.getY() + 8.0f, r.getWidth(), 8.0f }, 5.6f);
    const char* namn[] { "KOLVBASEN", "KOLVBAS 2", "STAMPVERKET", "HUGGET" };
    const char* typ[]  { "SEKVENSBAS", "ANDRA BASEN", "TRUMMOR", "STÖTAR OCH LEADS" };
    const float niva[] { 0.85f, 0.55f, 0.95f, 0.3f };
    for (int i = 0; i < 4; ++i)
    {
        const Rectangle<float> k (r.getX() + 10.0f, r.getY() + 26.0f + (float) i * 60.0f, r.getWidth() - 20.0f, 50.0f);
        const bool aktiv = i == vald;
        auto yta = ritaKnapp (g, k, aktiv, aktiv ? Colour (0xff29191a) : Colour (0xff222227));
        ritaLysdiod (g, { yta.getX() + 10.0f, yta.getY() + 11.0f }, 2.3f, farg::lysRod, aktiv ? 1.0f : 0.0f);
        tryck (g, "KANAL " + juce::String (i + 1), { yta.getX() + 18.0f, yta.getY() + 7.0f, 90.0f, 8.0f }, 5.0f,
               juce::Justification::centredLeft, farg::silverMatt);
        rubrik (g, namn[i], { yta.getX() + 8.0f, yta.getY() + 18.0f, 150.0f, 12.0f }, 11.0f,
                aktiv ? farg::vitt : farg::silver, juce::Justification::centredLeft, aktiv ? 1.6f : 1.2f, 2.0f);
        tryck (g, juce::String::fromUTF8 (typ[i]), { yta.getX() + 8.0f, yta.getY() + 33.0f, 150.0f, 8.0f }, 4.6f,
               juce::Justification::centredLeft, farg::silverMatt, 0.7f, 2.0f);
        ritaLedStapel (g, { yta.getRight() - 9.0f, yta.getY() + 6.0f, 4.0f, yta.getHeight() - 12.0f }, niva[i], 8, true);
        if (aktiv)
        {
            // Konstruktivistisk pil från vald kanal in mot skärmen.
            ritaPil (g, { k.getRight() + 3.0f, k.getCentreY() }, { r.getRight() + 16.0f, k.getCentreY() }, 5.0f, farg::tryckRott);
        }
    }

    // Mönster för vald kanal.
    const float y = r.getY() + 280.0f;
    tryck (g, sv ("MÖNSTER"), { r.getX() + 12.0f, y, 80.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
    ritaSjusegment (g, { r.getX() + 12.0f, y + 14.0f, 74.0f, 30.0f }, "A07");
    tryck (g, "KEDJA", { r.getX() + 100.0f, y, 70.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
    diodMedText (g, { r.getX() + 104.0f, y + 20.0f }, "AV", false);
    diodMedText (g, { r.getX() + 104.0f, y + 32.0f }, sv ("PÅ"), true);
    diodMedText (g, { r.getX() + 104.0f, y + 44.0f }, sv ("NÄSTA TAKT"), false, farg::lysOrange);
}

// ---------------------------------------------------------------------------------------------
// Bildskärmen (mitten)

inline void ritaBildskarm (juce::Graphics& g, Rectangle<float> yttre, const Ytor& ytor, const band::Installning& in, float skala)
{
    const auto glas = Rectangle<float> (yttre.getX() + 22.0f, yttre.getY() + 16.0f, yttre.getWidth() - 44.0f, yttre.getHeight() - 56.0f);
    skarm::ritaInfattning (g, yttre, glas, ytor);
    const auto bild = band::skarmbild (glas.getWidth(), glas.getHeight(), skala, in,
                                       [] (juce::Graphics& gs, Rectangle<float> r) { skarm::ritaKolvbasScen (gs, r); });
    band::ritaSkarm (g, glas, bild);

    // Infattningens nederkant: tryck, bandstyrka och strömdiod.
    const float y = yttre.getBottom() - 20.0f;
    tryck (g, sv ("BILDSKÄRM"), { yttre.getX() + 22.0f, y - 4.0f, 100.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
    g.setColour (farg::silverMatt);
    g.fillRect (yttre.getX() + 96.0f, y - 0.5f, 120.0f, 1.0f);
    tryck (g, "BANDSTYRKA", { yttre.getRight() - 290.0f, y - 4.0f, 80.0f, 8.0f }, 5.0f, juce::Justification::centredLeft, farg::silverMatt);
    diodMedText (g, { yttre.getRight() - 204.0f, y }, "AV", false, farg::lysOrange);
    diodMedText (g, { yttre.getRight() - 172.0f, y }, "LITE", false, farg::lysOrange);
    diodMedText (g, { yttre.getRight() - 132.0f, y }, "MYCKET", true, farg::lysOrange);
    ritaLysdiod (g, { yttre.getRight() - 30.0f, y }, 2.4f, farg::lysRod, 1.0f);
    tryck (g, "TV", { yttre.getRight() - 58.0f, y - 4.0f, 20.0f, 8.0f }, 5.0f, juce::Justification::centredRight, farg::silverMatt);
}

// ---------------------------------------------------------------------------------------------
// Presets, VU och master (höger)

inline void ritaPresets (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor)
{
    ritaInfalltFalt (g, r, ytor);
    tryck (g, "PRESETS", { r.getX(), r.getY() + 8.0f, r.getWidth(), 8.0f }, 5.6f);

    // Räkneverk och vakuumfluorescensdisplay med presetnamnet.
    ritaSjusegment (g, { r.getX() + 12.0f, r.getY() + 24.0f, 54.0f, 34.0f }, "07");
    {
        const Rectangle<float> v (r.getX() + 74.0f, r.getY() + 24.0f, r.getWidth() - 86.0f, 34.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (v.expanded (2.0f), 3.0f);
        g.setColour (Colour (0xff04110f));
        g.fillRoundedRectangle (v, 2.0f);
        // Svagt rutnät av släckta punkter, som på en riktig matris-VFD.
        const float p = 2.2f;
        const juce::String namn ("KOLV 125");
        const float tx = v.getX() + 10.0f, ty = v.getCentreY() - 3.5f * p;
        g.setColour (farg::vfd.withAlpha (0.06f));
        osd::ritaRaw (g, juce::String::repeatedString (juce::String::charToString (0x25A0), 10), tx, ty, p, 0.75f);
        g.setColour (farg::vfd.withAlpha (0.25f));
        osd::ritaRaw (g, namn, tx - 0.5f, ty - 0.5f, p * 1.0f, 1.0f);
        g.setColour (farg::vfd);
        osd::ritaRaw (g, namn, tx, ty, p, 0.75f);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.07f), v.getX(), v.getY(), juce::Colours::transparentWhite, v.getX(), v.getCentreY(), false });
        g.fillRect (v.withHeight (v.getHeight() * 0.5f).reduced (1.0f, 0.5f));
    }
    // Bläddra och spara.
    {
        const float y = r.getY() + 68.0f;
        auto a = ritaKnapp (g, { r.getX() + 12.0f, y, 34.0f, 18.0f });
        g.setColour (farg::silver); g.fillPath (ikon (Ikon::bakat, a.reduced (9.0f, 3.5f)));
        auto b = ritaKnapp (g, { r.getX() + 52.0f, y, 34.0f, 18.0f });
        g.setColour (farg::silver); g.fillPath (ikon (Ikon::spela, b.reduced (9.0f, 3.5f)));
        auto c = ritaKnapp (g, { r.getX() + 92.0f, y, 60.0f, 18.0f });
        tryck (g, "SPARA", c, 5.6f);
        auto d = ritaKnapp (g, { r.getX() + 158.0f, y, 60.0f, 18.0f });
        tryck (g, "BANK B", d, 5.6f);
        ritaLysdiod (g, { r.getRight() - 22.0f, y + 9.0f }, 2.2f, farg::lysOrange, 0.0f);
    }

    // VU-mätarna.
    const float vy = r.getY() + 102.0f;
    const float vb = (r.getWidth() - 30.0f) * 0.5f;
    ritaVU (g, { r.getX() + 12.0f, vy, vb, 92.0f }, -3.5f, "L");
    ritaVU (g, { r.getX() + 18.0f + vb, vy, vb, 92.0f }, -1.2f, "R");

    // Mastern: stor ratt och masterkedjans två rattar.
    skara (g, r.getY() + 212.0f, r.getX() + 10.0f, r.getRight() - 10.0f);
    gruppRubrik (g, "MASTER", r.getX() + 14.0f, r.getRight() - 14.0f, r.getY() + 228.0f);
    rattMedEtikett (g, { r.getX() + 60.0f, r.getY() + 280.0f }, 27.0f, 0.72f, sv ("NIVÅ"), RattTyp::stor);
    rattMedEtikett (g, { r.getX() + 152.0f, r.getY() + 278.0f }, 15.0f, 0.45f, sv ("KLÄMMAN"));
    rattMedEtikett (g, { r.getX() + 226.0f, r.getY() + 278.0f }, 15.0f, 0.3f, "KASSETTEN");
}

// ---------------------------------------------------------------------------------------------
// Rattpanelen

inline void ritaRattpanel (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor)
{
    // Vyknappar.
    const char* vyer[] { "KANAL", "SEKVENS", "MIXER", "EFFEKTER", "MODULATION" };
    for (int i = 0; i < 5; ++i)
    {
        const Rectangle<float> k (r.getX() + 10.0f + (float) i * 92.0f, r.getY() + 4.0f, 86.0f, 20.0f);
        auto yta = ritaKnapp (g, k, i == 0);
        ritaLysdiod (g, { yta.getX() + 9.0f, yta.getCentreY() }, 2.0f, farg::lysRod, i == 0 ? 1.0f : 0.0f);
        tryck (g, vyer[i], yta.withTrimmedLeft (14.0f), 5.6f, juce::Justification::centred, i == 0 ? farg::vitt : farg::silver);
    }
    // Kanalens namn till höger, med rött block.
    {
        const Rectangle<float> blk (r.getRight() - 236.0f, r.getY() + 4.0f, 30.0f, 20.0f);
        g.setColour (farg::tryckRott);
        g.fillPath (parallellogram (blk, 6.0f));
        rubrik (g, "1", blk, 12.0f, farg::vitt, juce::Justification::centred, 1.8f);
        rubrik (g, "KOLVBASEN", { blk.getRight() + 8.0f, blk.getY() + 2.0f, 200.0f, 16.0f }, 14.0f, farg::vitt,
                juce::Justification::centredLeft, 1.9f, 2.2f);
        ritaDiagonaler (g, { r.getX() + 480.0f, r.getY() + 9.0f, r.getRight() - 246.0f - (r.getX() + 480.0f), 10.0f },
                        juce::Colours::white.withAlpha (0.06f), 2.0f, 6.0f);
    }

    const auto falt = Rectangle<float> (r.getX(), r.getY() + 32.0f, r.getWidth(), r.getHeight() - 32.0f);
    ritaInfalltFalt (g, falt, ytor);
    const float cy = falt.getY() + 54.0f, rr = 17.0f, d = 76.0f;
    const float ry = falt.getY() + 14.0f;

    struct Ratt { const char* namn; float varde; };
    auto grupp = [&] (const char* titel, float x0, std::initializer_list<Ratt> rattar)
    {
        float x = x0;
        for (auto& ra : rattar) { rattMedEtikett (g, { x, cy }, rr, ra.varde, juce::String::fromUTF8 (ra.namn)); x += d; }
        gruppRubrik (g, juce::String::fromUTF8 (titel), x0 - 30.0f, x - d + 30.0f, ry);
        return x;
    };
    grupp ("OSCILLATOR", 58.0f, { { "VÅGFORM", 0.15f }, { "PULSBREDD", 0.5f }, { "SUB", 0.7f }, { "BRUS", 0.1f } });
    float x = grupp ("FILTER", 386.0f, { { "BRYTFREKVENS", 0.32f }, { "RESONANS", 0.68f }, { "ENV-DJUP", 0.75f }, { "ACCENT", 0.6f } });
    // Filtertyp: 24 dB ladder / 12 dB tillstånd.
    {
        const float fx = x - 32.0f;
        diodMedText (g, { fx, cy - 10.0f }, "24 DB", true);
        diodMedText (g, { fx, cy + 2.0f }, "12 DB", false);
        auto k = ritaKnapp (g, { fx - 6.0f, cy + 14.0f, 40.0f, 14.0f });
        tryck (g, "TYP", k, 5.0f);
    }
    grupp ("ENVELOPE", 760.0f, { { "FALL", 0.35f }, { "GATELÄNGD", 0.55f }, { "GLID", 0.2f } });
    grupp ("DRIVET", 1012.0f, { { "MÄNGD", 0.4f }, { "NIVÅ", 0.8f } });
}

// ---------------------------------------------------------------------------------------------
// Stegknapparna

inline void ritaStegrad (juce::Graphics& g, Rectangle<float> r)
{
    const std::set<int> pa { 0, 2, 3, 6, 8, 10, 11, 14 };
    const std::set<int> accent { 0, 8 };
    const int spelhuvud = 5;
    const float bw = 48.0f, gap = 8.0f, grGap = 18.0f;
    float x = r.getX() + 12.0f;
    const float ledY = r.getY() + 12.0f, ky = r.getY() + 24.0f, kh = 40.0f;
    for (int i = 0; i < 16; ++i)
    {
        const Rectangle<float> k (x, ky, bw, kh);
        const bool steg = pa.count (i) > 0;
        const float styrka = i == spelhuvud ? 1.0f : (steg ? 0.85f : 0.0f);
        ritaLysdiod (g, { k.getCentreX(), ledY }, 2.6f, i == spelhuvud ? farg::lysOrange : farg::lysRod, styrka);
        ritaStegknapp (g, k, i / 4, i == spelhuvud);
        if (accent.count (i) > 0)   // accent markeras med en liten röd trekant i tryck
        {
            juce::Path t;
            t.addTriangle (k.getRight() - 9.0f, ledY - 3.0f, k.getRight() - 3.0f, ledY - 3.0f, k.getRight() - 6.0f, ledY + 2.0f);
            g.setColour (farg::tryckRott);
            g.fillPath (t);
        }
        tryck (g, juce::String (i + 1), { k.getX(), k.getBottom() + 6.0f, bw, 8.0f }, 6.0f,
               juce::Justification::centred, i % 4 == 0 ? farg::vitt : farg::silver, i % 4 == 0 ? 1.0f : 0.8f, 1.6f);
        x += bw + gap;
        if (i % 4 == 3)
        {
            // Gruppens hakparentes under siffrorna.
            const float gx0 = x - gap - 4.0f * bw - 3.0f * gap, gx1 = x - gap;
            g.setColour (farg::silverMatt);
            g.drawLine (gx0, ky + kh + 20.0f, gx1, ky + kh + 20.0f, 0.8f);
            g.drawLine (gx0, ky + kh + 17.0f, gx0, ky + kh + 20.0f, 0.8f);
            g.drawLine (gx1, ky + kh + 17.0f, gx1, ky + kh + 20.0f, 0.8f);
            x += grGap - gap;
        }
    }
    // Sidknappar I–IV och A/B.
    const char* sidor[] { "I", "II", "III", "IV", "A/B" };
    const float sx = r.getRight() - 5.0f * 36.0f - 4.0f * 8.0f - 12.0f;
    tryck (g, "SIDA", { sx, r.getY() + 0.0f, 4.0f * 36.0f + 3.0f * 8.0f, 8.0f }, 5.0f, juce::Justification::centred, farg::silverMatt);
    for (int i = 0; i < 5; ++i)
    {
        const Rectangle<float> k (sx + (float) i * 44.0f, ky + 6.0f, 36.0f, 28.0f);
        ritaLysdiod (g, { k.getCentreX(), ledY }, 2.2f, i == 4 ? farg::lysOrange : farg::lysRod, i == 0 ? 1.0f : 0.0f);
        auto yta = ritaKnapp (g, k, i == 0);
        tryck (g, sidor[i], yta, 7.0f, juce::Justification::centred, i == 0 ? farg::vitt : farg::silver, 1.0f, 2.0f);
    }
    tryck (g, sv ("MÖNSTERSIDA · VARIANT"), { sx, ky + kh + 4.0f, 5.0f * 36.0f + 4.0f * 8.0f, 8.0f }, 4.8f,
           juce::Justification::centred, farg::silverMatt);
}

// ---------------------------------------------------------------------------------------------
// Klaviaturen

inline void ritaKlaviatur (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor)
{
    skara (g, r.getY(), r.getX() - 14.0f, r.getRight() + 14.0f);
    const auto vanster = Rectangle<float> (r.getX(), r.getY() + 10.0f, 104.0f, r.getHeight() - 16.0f);
    ritaInfalltFalt (g, vanster, ytor);
    tryck (g, "OKTAV", { vanster.getX(), vanster.getY() + 8.0f, vanster.getWidth(), 8.0f }, 5.6f);
    ritaSjusegment (g, { vanster.getX() + 22.0f, vanster.getY() + 22.0f, 60.0f, 28.0f }, "C2");
    auto a = ritaKnapp (g, { vanster.getX() + 12.0f, vanster.getY() + 60.0f, 36.0f, 18.0f });
    g.setColour (farg::silver); g.fillPath (ikon (Ikon::bakat, a.reduced (10.0f, 3.5f)));
    auto b = ritaKnapp (g, { vanster.getRight() - 48.0f, vanster.getY() + 60.0f, 36.0f, 18.0f });
    g.setColour (farg::silver); g.fillPath (ikon (Ikon::spela, b.reduced (10.0f, 3.5f)));
    tryck (g, "TRANSPONERA", { vanster.getX(), vanster.getBottom() - 14.0f, vanster.getWidth(), 8.0f }, 4.6f,
           juce::Justification::centred, farg::silverMatt);

    const auto kb = Rectangle<float> (r.getX() + 116.0f, r.getY() + 12.0f, r.getWidth() - 116.0f, r.getHeight() - 18.0f);
    ritaTangentbord (g, kb, 22, { 12, 19 });
}

// ---------------------------------------------------------------------------------------------
// Hela panelen

class Panel final : public juce::Component
{
public:
    Panel() { setSize (designBredd, designHojd); setOpaque (true); }

    band::Installning bandLage { true, 1.0f, 0x8701u, sv ("▶ SPELA"), "SP", "0:12:04", "KOLVBASEN" };

    void paint (juce::Graphics& g) override
    {
        const float s = fysiskSkala (g);
        ytor.forbered (designBredd, designHojd, s);
        const auto r = getLocalBounds().toFloat();
        g.setFillType (ytor.metallFyll());
        g.fillRect (r);
        // Svag ljusgradient över chassit (ljuset uppifrån).
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.03f), 0.0f, 0.0f, juce::Colours::black.withAlpha (0.18f), 0.0f, r.getBottom(), false });
        g.fillRect (r);

        ritaBandverket (g, { 0.0f, 0.0f, r.getWidth(), 84.0f });
        ritaKanalvaljare (g, { 14.0f, 96.0f, 178.0f, 352.0f }, ytor);
        ritaBildskarm (g, { 208.0f, 92.0f, 684.0f, 360.0f }, ytor, bandLage, s);
        ritaPresets (g, { 906.0f, 96.0f, 280.0f, 352.0f }, ytor);
        skara (g, 462.0f, 0.0f, r.getWidth());
        ritaRattpanel (g, { 14.0f, 470.0f, 1172.0f, 152.0f }, ytor);
        ritaStegrad (g, { 14.0f, 628.0f, 1172.0f, 92.0f });
        ritaKlaviatur (g, { 14.0f, 724.0f, 1172.0f, 116.0f }, ytor);

        for (auto p : { Point<float> (7.0f, 7.0f), { r.getRight() - 7.0f, 7.0f }, { 7.0f, r.getBottom() - 7.0f }, { r.getRight() - 7.0f, r.getBottom() - 7.0f } })
            ritaSkruv (g, p, 3.6f, 0.3f + p.x * 0.01f);
        g.setColour (juce::Colours::black);
        g.drawRect (r, 1.0f);
    }

private:
    Ytor ytor;
};

// ---------------------------------------------------------------------------------------------
// Detaljarket: närbilder av reglagen, displayen, typsnitten och skärmen med och utan band.

class Detaljer final : public juce::Component
{
public:
    Detaljer() { setSize (1200, 1000); setOpaque (true); }

    void paint (juce::Graphics& g) override
    {
        const float s = fysiskSkala (g);
        ytor.forbered (getWidth(), getHeight(), s);
        const auto r = getLocalBounds().toFloat();
        g.setFillType (ytor.metallFyll());
        g.fillRect (r);

        rubrik (g, "KROPPSMASKINEN KM-87", { 24.0f, 18.0f, 400.0f, 20.0f }, 20.0f, farg::vitt, juce::Justification::centredLeft, 2.2f);
        tryck (g, sv ("STILPROV · DETALJER · BAND 1987"), { 24.0f, 46.0f, 400.0f, 8.0f }, 6.0f, juce::Justification::centredLeft, farg::silverMatt);
        g.setColour (farg::tryckRott);
        g.fillPath (parallellogram ({ 420.0f, 18.0f, 300.0f, 20.0f }, 10.0f));
        ritaDiagonaler (g, { 724.0f, 18.0f, 460.0f, 20.0f }, farg::tryckRott.withAlpha (0.8f), 3.0f, 9.0f);
        skara (g, 66.0f, 0.0f, r.getWidth());

        auto ram = [&] (Rectangle<float> f, const juce::String& titel)
        {
            ritaInfalltFalt (g, f, ytor);
            tryck (g, titel, { f.getX() + 12.0f, f.getY() + 8.0f, f.getWidth() - 24.0f, 8.0f }, 6.0f, juce::Justification::centredLeft);
        };

        // Rattar.
        {
            const Rectangle<float> f (16.0f, 80.0f, 380.0f, 270.0f);
            ram (f, sv ("RATT · SKALA 1:3 OCH 1:1"));
            ritaRatt (g, { f.getX() + 120.0f, f.getY() + 132.0f }, 62.0f, 0.68f, RattTyp::liten);
            ritaRatt (g, { f.getX() + 290.0f, f.getY() + 70.0f }, 17.0f, 0.3f);
            tryck (g, "LITEN", { f.getX() + 245.0f, f.getY() + 102.0f, 90.0f, 8.0f }, 5.6f);
            ritaRatt (g, { f.getX() + 290.0f, f.getY() + 160.0f }, 27.0f, 0.72f, RattTyp::stor);
            tryck (g, "STOR", { f.getX() + 245.0f, f.getY() + 204.0f, 90.0f, 8.0f }, 5.6f);
            tryck (g, sv ("SILVERSTRECK · RÄFFLAD KJOL · SVARVAD KÅPA"), { f.getX() + 12.0f, f.getBottom() - 18.0f, f.getWidth() - 24.0f, 8.0f },
                   5.0f, juce::Justification::centredLeft, farg::silverMatt);
        }
        // Stegknapp med lysdiod, förstorad tre gånger.
        {
            const Rectangle<float> f (408.0f, 80.0f, 300.0f, 270.0f);
            ram (f, sv ("STEGKNAPP MED LYSDIOD · 3:1"));
            {
                juce::Graphics::ScopedSaveState st (g);
                g.addTransform (juce::AffineTransform::scale (3.0f).translated (f.getX() + 18.0f, f.getY() + 30.0f));
                ritaLysdiod (g, { 24.0f, 8.0f }, 2.6f, farg::lysRod, 1.0f);
                ritaStegknapp (g, { 0.0f, 20.0f, 48.0f, 40.0f }, 0, false);
                tryck (g, "1", { 0.0f, 66.0f, 48.0f, 8.0f }, 6.0f, juce::Justification::centred, farg::vitt, 1.0f);
            }
            for (int i = 0; i < 4; ++i)
            {
                const Rectangle<float> k (f.getX() + 190.0f, f.getY() + 40.0f + (float) i * 52.0f, 48.0f, 34.0f);
                ritaLysdiod (g, { k.getRight() + 16.0f, k.getCentreY() }, 2.6f, i == 1 ? farg::lysOrange : farg::lysRod, i == 3 ? 0.0f : 1.0f);
                ritaStegknapp (g, k, i, i == 1);
            }
            tryck (g, sv ("FYRA GRUPPER · RÖD ORANGE BENVIT GRÅ"), { f.getX() + 12.0f, f.getBottom() - 18.0f, f.getWidth() - 24.0f, 8.0f },
                   5.0f, juce::Justification::centredLeft, farg::silverMatt);
        }
        // Sjusegment.
        {
            const Rectangle<float> f (720.0f, 80.0f, 464.0f, 270.0f);
            ram (f, "SJUSEGMENTSDISPLAY");
            ritaSjusegment (g, { f.getX() + 16.0f, f.getY() + 28.0f, 210.0f, 78.0f }, "125.0", farg::segment, 50.0f);
            ritaSjusegment (g, { f.getX() + 236.0f, f.getY() + 28.0f, 212.0f, 78.0f }, "0:12:04", farg::segment, 40.0f);
            ritaSjusegment (g, { f.getX() + 16.0f, f.getY() + 118.0f, 432.0f, 48.0f }, "0123456789-", farg::segment, 30.0f);
            ritaSjusegment (g, { f.getX() + 16.0f, f.getY() + 178.0f, 432.0f, 48.0f }, "AbCdEFHJLnoPrtUy", farg::segment, 26.0f);
            tryck (g, sv ("SLÄCKTA SEGMENT SYNS SVAGT BAKOM DET RÖDA GLASET"), { f.getX() + 12.0f, f.getBottom() - 18.0f, f.getWidth() - 24.0f, 8.0f },
                   5.0f, juce::Justification::centredLeft, farg::silverMatt);
        }
        // OSD-typsnittet och linjeskriften.
        {
            const Rectangle<float> f (16.0f, 362.0f, 1168.0f, 268.0f);
            ram (f, sv ("OSD-TYPSNITTET 5×7 · ALLA TECKEN"));
            const auto svart = Rectangle<float> (f.getX() + 12.0f, f.getY() + 24.0f, f.getWidth() - 24.0f, 142.0f);
            g.setColour (farg::skarmSvart);
            g.fillRect (svart);
            const auto alla = osd::allaTecken();
            const float p = 4.0f;
            const int perRad = (int) ((svart.getWidth() - 24.0f) / (p * 6.0f + p * 2.0f));
            for (int i = 0; i < alla.length(); ++i)
            {
                const float x = svart.getX() + 14.0f + (float) (i % perRad) * (p * 8.0f);
                const float y = svart.getY() + 12.0f + (float) (i / perRad) * (p * 9.0f);
                osd::rita (g, alla.substring (i, i + 1), x, y, p);
            }
            osd::rita (g, sv ("▶ SPELA   ■ STOPP   ● INSP   ‖ PAUS   SP  0:12:04  KANAL 01 KOLVBASEN"), svart.getX() + 14.0f, svart.getBottom() - 22.0f, 2.6f);

            tryck (g, "LINJESKRIFTEN (SILVERTRYCK OCH RUBRIKER)", { f.getX() + 12.0f, f.getY() + 174.0f, 600.0f, 8.0f }, 6.0f, juce::Justification::centredLeft);
            rubrik (g, sv ("ABCDEFGHIJKLMNOPQRSTUVWXYZÅÄÖ 0123456789"), { f.getX() + 12.0f, f.getY() + 194.0f, 1100.0f, 22.0f }, 20.0f, farg::vitt,
                    juce::Justification::centredLeft, 2.0f, 2.4f);
            tryck (g, sv ("BRYTFREKVENS · RESONANS · SPÄNNINGEN · STAMPVERKET · HUGGET · 24 DB · −6,0 % · 1,2 KHZ → ← ↑ ↓"),
                   { f.getX() + 12.0f, f.getY() + 234.0f, 1100.0f, 10.0f }, 7.0f, juce::Justification::centredLeft, farg::silver, 0.9f);
        }
        // Skärmen utan och med bandet.
        {
            const Rectangle<float> f (16.0f, 642.0f, 1168.0f, 344.0f);
            ram (f, sv ("BILDSKÄRMEN · UTAN BAND (VÄNSTER) OCH MED BAND (HÖGER)"));
            const float bw = (f.getWidth() - 36.0f) * 0.5f;
            for (int i = 0; i < 2; ++i)
            {
                const Rectangle<float> yttre (f.getX() + 12.0f + (float) i * (bw + 12.0f), f.getY() + 26.0f, bw, f.getHeight() - 38.0f);
                const auto glas = yttre.reduced (16.0f, 14.0f);
                skarm::ritaInfattning (g, yttre, glas, ytor);
                band::Installning in { i == 1, 1.0f, 0x8701u, sv ("▶ SPELA"), "SP", "0:12:04", "KOLVBASEN" };
                const auto bild = band::skarmbild (glas.getWidth(), glas.getHeight(), s, in,
                                                   [] (juce::Graphics& gs, Rectangle<float> rr) { skarm::ritaKolvbasScen (gs, rr); });
                band::ritaSkarm (g, glas, bild);
            }
        }
    }

private:
    Ytor ytor;
};

} // namespace km::stil::prov
