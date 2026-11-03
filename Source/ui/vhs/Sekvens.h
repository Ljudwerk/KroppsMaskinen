// Sekvensvyn: hela mönstret för vald kanal och sida, i rattpanelens fält.
//
// Stampverket: tolv spår × sexton steg, som en lysdiodsmatris. Klick som på stegraden (Skift ratchet,
// Alt chans); klick på trummans namn väljer trumman för stegraden.
// Tonkanalerna: en tonrulle, −12 … +12 halvtoner. Klick i en kolumn sätter tonen (eller växlar
// ton → accent → av när man klickar på tonen själv); dra för att flytta den. Högerklick glid,
// Alt oktav, Skift tonlängd.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Stegrad.h"

namespace km::vhs
{

class Sekvens final : public juce::Component
{
public:
    explicit Sekvens (PanelLage& l) : lage (l) { setOpaque (true); }

    static constexpr float etikettB = 64.0f;

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff060607));
        g.fillRect (r);
        const int spel = lage.spelsteg();
        const int langd = lage.langdFor (lage.foljer() ? 0 : lage.kanal);
        const float cw = cellB();

        // Taktgrupper och spelhuvud.
        for (int c = 0; c < 16; ++c)
        {
            const int steg = lage.sida * 16 + c;
            const auto kol = Rectangle<float> (etikettB + (float) c * cw, 0.0f, cw, r.getHeight());
            if (c % 4 == 0) { g.setColour (juce::Colours::white.withAlpha (0.035f)); g.fillRect (kol); }
            if (steg == spel) { g.setColour (farg::lysOrange.withAlpha (0.16f)); g.fillRect (kol); }
            if (steg >= langd) { g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRect (kol); }
        }

        if (! lage.tonKanal()) ritaTrummor (g, spel);
        else ritaToner (g, spel);

        if (lage.foljer())
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (r);
            etikett (g, stil::sv ("KOLVBAS 2 FÖLJER KOLVBASEN – STÄNG AV FÖLJ FÖR ETT EGET MÖNSTER"), r.reduced (20.0f, 0.0f), 6.0f, farg::silver);
        }
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRect (r, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragSteg = -1;
        if (lage.foljer()) return;
        const auto p = e.position;
        if (! lage.tonKanal())
        {
            const int rad = juce::jlimit (0, antalSpar - 1, (int) (p.y / radH()));
            if (p.x < etikettB) { lage.valdTrumma = rad; repaint(); lage.meddela(); return; }
            const int c = kolumn (p.x);
            if (c < 0) return;
            redigering::trumKlick (lage.trum(), rad, lage.sida * 16 + c, e.mods);
            repaint();
            lage.meddela();
            return;
        }
        const int c = kolumn (p.x);
        if (c < 0) return;
        const int steg = lage.sida * 16 + c;
        if (redigering::tonKlick (lage.ton(), steg, e.mods)) { repaint(); lage.meddela(); return; }
        const int ton = tonVid (p.y);
        const auto s = lage.ton().get (steg);
        if (s.slag != 0 && s.halvtoner == ton) redigering::tonVaxla (lage.ton(), steg);
        else redigering::tonHojd (lage.ton(), steg, ton);
        dragSteg = steg;
        repaint();
        lage.meddela();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragSteg < 0) return;
        const auto s = lage.ton().get (dragSteg);
        if (s.slag == 0) return;
        const int ton = tonVid (e.position.y);
        if (ton != s.halvtoner) { redigering::tonHojd (lage.ton(), dragSteg, ton); repaint(); lage.meddela(); }
    }
    void mouseUp (const juce::MouseEvent&) override { dragSteg = -1; }

    void tick()
    {
        juce::int64 h = lage.spelsteg() * 31 + lage.sida * 7 + lage.kanal * 131 + lage.redigera * 977 + lage.valdTrumma * 5323
                      + lage.langdFor (lage.foljer() ? 0 : lage.kanal) * 104729 + (lage.foljer() ? 1 : 0);
        for (int c = 0; c < 16; ++c)
        {
            const int steg = lage.sida * 16 + c;
            if (lage.tonKanal())
            {
                const auto s = lage.ton().get (steg);
                h = h * 1000003 + (s.slag + 3 * (s.halvtoner + 12) + 100 * (s.oktav + 1) + 1000 * s.langd + 10000 * (int) s.glid);
            }
            else
                for (int t = 0; t < antalSpar; ++t)
                {
                    const auto s = lage.trum().get (t, steg);
                    h = h * 31 + (s.slag + 3 * s.chans + 12 * s.ratchet);
                }
        }
        if (h != hash) { hash = h; repaint(); }
    }

private:
    float cellB() const { return ((float) getWidth() - etikettB - 4.0f) / 16.0f; }
    float radH() const { return (float) getHeight() / (float) antalSpar; }
    int kolumn (float x) const
    {
        if (x < etikettB) return -1;
        const int c = (int) ((x - etikettB) / cellB());
        return c >= 0 && c < 16 ? c : -1;
    }
    float tonY (int ton) const { const float h = ((float) getHeight() - 8.0f) / 25.0f; return 4.0f + (float) (12 - ton) * h; }
    int tonVid (float y) const { const float h = ((float) getHeight() - 8.0f) / 25.0f; return juce::jlimit (-12, 12, 12 - (int) std::floor ((y - 4.0f) / h)); }

    void ritaTrummor (juce::Graphics& g, int spel)
    {
        static const char* namn[antalSpar] { "BD", "SD", "SD2", "CP", "CH", "OH", "CY", "LT", "HT", "MET", "", "KO" };
        const float rh = radH(), cw = cellB();
        for (int t = 0; t < antalSpar; ++t)
        {
            const float y = (float) t * rh;
            const bool vald = t == lage.valdTrumma;
            if (vald) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (0.0f, y, (float) getWidth(), rh); }
            etikett (g, t == 10 ? stil::sv ("SMÄLL") : juce::String (namn[t]), { 4.0f, y, etikettB - 8.0f, rh }, juce::jmin (6.5f, rh * 0.5f),
                     vald ? farg::vitt : farg::silverMatt, juce::Justification::centredLeft);
            for (int c = 0; c < 16; ++c)
            {
                const int steg = lage.sida * 16 + c;
                const auto s = lage.trum().get (t, steg);
                const auto cell = Rectangle<float> (etikettB + (float) c * cw, y, cw, rh).reduced (2.0f, 1.2f);
                g.setColour (juce::Colour (0xff1a1a1e));
                g.fillRoundedRectangle (cell, 1.5f);
                if (s.slag == 0) continue;
                auto f = farg::lysRod.withAlpha (s.slag == 2 ? 1.0f : 0.55f);
                if (steg == spel) f = farg::lysOrange;
                g.setColour (f);
                g.fillRoundedRectangle (cell, 1.5f);
                if (s.harTillagg() && rh > 7.0f)
                    etikett (g, redigering::trumText (s), cell, juce::jmin (5.5f, rh * 0.45f), juce::Colours::black.withAlpha (0.8f));
            }
        }
    }

    void ritaToner (juce::Graphics& g, int spel)
    {
        const float cw = cellB(), th = ((float) getHeight() - 8.0f) / 25.0f;
        // Halvtonsraster: grundtonen och oktaverna tydligare.
        for (int t = -12; t <= 12; ++t)
        {
            const float y = tonY (t) + th * 0.5f;
            g.setColour (juce::Colours::white.withAlpha (t % 12 == 0 ? 0.12f : 0.03f));
            g.drawHorizontalLine ((int) y, etikettB, (float) getWidth() - 4.0f);
            if (t % 12 == 0)
                etikett (g, t > 0 ? "+12" : (t < 0 ? stil::sv ("−12") : "0"), { 4.0f, y - 4.0f, etikettB - 8.0f, 8.0f }, 4.4f,
                         farg::silverMatt, juce::Justification::centredLeft);
        }
        float forraY = -1.0f; bool forraGlid = false;
        for (int c = 0; c < 16; ++c)
        {
            const int steg = lage.sida * 16 + c;
            const auto s = lage.ton().get (steg);
            if (s.slag == 0) { forraY = -1.0f; continue; }
            const float andel = s.glid ? 1.0f : (float) juce::jlimit (0.15, 1.0, tonLangder[(size_t) s.langd]);
            const float y = tonY (s.halvtoner);
            const auto blk = Rectangle<float> (etikettB + (float) c * cw + 1.5f, y, (cw - 3.0f) * andel, th).reduced (0.0f, 0.4f);
            if (forraGlid && forraY >= 0.0f)
            {
                g.setColour (farg::lysRod.withAlpha (0.5f));
                g.drawLine (blk.getX() - 1.5f, forraY + th * 0.5f, blk.getX() + 2.0f, y + th * 0.5f, 1.2f);
            }
            g.setColour (steg == spel ? farg::lysOrange : farg::lysRod.withAlpha (s.slag == 2 ? 1.0f : 0.6f));
            g.fillRoundedRectangle (blk, 1.2f);
            if (s.oktav != 0)
                etikett (g, s.oktav > 0 ? stil::sv ("↑") : stil::sv ("↓"), blk.withY (s.oktav > 0 ? y - 8.0f : y + th).withHeight (8.0f).withWidth (12.0f), 4.4f, farg::silver);
            forraY = y; forraGlid = s.glid;
        }
    }

    PanelLage& lage;
    juce::int64 hash = 0;
    int dragSteg = -1;
};

} // namespace km::vhs
