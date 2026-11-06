// Låtvyn: låtens rader som kolumner, sexton per sida, i rattpanelens fält.
//
// Varje kolumn är en rad i låten: takter, mönster för de fyra kanalerna, Spänningen och transponeringen.
// Klick på radnumret väljer raden (KOPIERA, TA BORT och HÄMTA verkar på vald rad); klick på den första
// tomma kolumnen lägger till en rad. Mönsterrutorna: dra eller rulla för mönstret, klicka för varianten
// (A → B → A+B), högerklick tystar kanalen på raden. Takter, Spänningen och transponeringen: klick = upp
// ett steg, högerklick/Alt-klick = ned, dra eller rulla; dubbelklick nollställer (Spänningen blir "–").
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Reglage.h"
#include "PanelLage.h"

namespace km::vhs
{

class LatVy final : public juce::Component
{
public:
    explicit LatVy (PanelLage& l) : lage (l) { setOpaque (true); }

    static constexpr float etikettB = 84.0f;
    static constexpr int antalRaderIVy = 8;   // rubrik, takter, fyra kanaler, Spänningen, transponering
    int valdRad = 0;
    int sida = 0;                             // 0..3, sexton rader per sida

    Laten& laten() { return lage.proc.banker.laten; }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff060607));
        g.fillRect (r);
        const int n = laten().antal();
        const int spelar = lage.proc.latRad();
        const float cw = cellB(), rh = radH();
        static const char* etiketter[antalRaderIVy] { "RAD", "TAKTER", "KOLVB.", "KOLVB. 2", "STAMPV.", "HUGGET", "", "TRANSP." };
        for (int i = 0; i < antalRaderIVy; ++i)
        {
            const juce::String t = i == 6 ? stil::sv ("SPÄNN.") : juce::String (etiketter[i]);
            etikett (g, t, { 4.0f, (float) i * rh, etikettB - 8.0f, rh }, juce::jmin (6.0f, rh * 0.42f), i == 0 ? farg::silver : farg::silverMatt,
                     juce::Justification::centredLeft);
        }
        // Sidpilarna i rubrikraden.
        etikett (g, juce::String (sida + 1) + "/4", { etikettB - 22.0f, 0.0f, 20.0f, rh }, 4.0f, farg::silverMatt, juce::Justification::centredRight);

        for (int c = 0; c < 16; ++c)
        {
            const int rad = sida * 16 + c;
            const float x = etikettB + (float) c * cw;
            const auto kol = Rectangle<float> (x, 0.0f, cw, r.getHeight());
            if (c % 4 == 0) { g.setColour (juce::Colours::white.withAlpha (0.03f)); g.fillRect (kol); }
            if (rad == spelar) { g.setColour (farg::lysOrange.withAlpha (0.16f)); g.fillRect (kol); }
            if (rad == valdRad) { g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawRect (kol.reduced (0.5f), 1.0f); }
            if (rad > n) continue;
            if (rad == n)
            {
                if (n < Laten::maxRader)
                    etikett (g, "+", Rectangle<float> (x, 0.0f, cw, rh), 6.0f, farg::silverMatt);
                continue;
            }
            const auto lr = laten().get (rad);
            auto cell = [&] (int i) { return Rectangle<float> (x, (float) i * rh, cw, rh).reduced (2.0f, 1.0f); };
            const float h = juce::jmin (7.5f, rh * 0.45f);
            etikett (g, juce::String (rad + 1).paddedLeft ('0', 2), cell (0), h, rad == spelar ? farg::lysOrange : farg::vitt);
            etikett (g, juce::String (lr.takter), cell (1), h, farg::lysOrange);
            for (int k = 0; k < antalKanaler; ++k)
            {
                const auto ce = cell (2 + k);
                const bool tyst = lr.tyst[(size_t) k];
                g.setColour (tyst ? juce::Colour (0xff121214) : juce::Colour (0xff1e0c0a));
                g.fillRoundedRectangle (ce, 1.5f);
                const juce::String v = lr.variant[(size_t) k] == 0 ? "A" : (lr.variant[(size_t) k] == 1 ? "B" : "AB");
                etikett (g, tyst ? juce::String ("TYST") : juce::String (lr.monster[(size_t) k] + 1) + v, ce, h,
                         tyst ? farg::silverMatt : farg::lysRod.brighter (0.3f));
            }
            etikett (g, lr.spanning < 0 ? stil::sv ("–") : juce::String (lr.spanning) + "%", cell (6), h, lr.spanning < 0 ? farg::silverMatt : farg::tryckRott);
            etikett (g, lr.transponering == 0 ? juce::String ("0")
                                              : (lr.transponering > 0 ? "+" + juce::String (lr.transponering) : stil::sv ("−") + juce::String (-lr.transponering)),
                     cell (7), h, lr.transponering == 0 ? farg::silverMatt : farg::silver);
        }
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRect (r, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        drog = false;
        cellRad = -1;
        const auto p = e.position;
        if (p.x < etikettB)
        {
            if (radVid (p.y) == 0) { sida = (sida + (e.mods.isShiftDown() ? 3 : 1)) % 4; repaint(); }
            return;
        }
        const int c = juce::jlimit (0, 15, (int) ((p.x - etikettB) / cellB()));
        const int rad = sida * 16 + c, n = laten().antal();
        if (rad > n) return;
        if (rad == n)
        {
            // Ny rad: en kopia av den sista, eller panelens mönster om låten är tom.
            laten().infoga (n, n > 0 ? laten().get (n - 1) : franPanelen());
            valdRad = n;
            andrat();
            return;
        }
        valdRad = rad;
        cellRad = rad;
        cellRadIVy = radVid (p.y);
        startY = p.y;
        startVarde = varde (laten().get (rad), cellRadIVy);
        if (e.mods.isPopupMenu() && cellRadIVy >= 2 && cellRadIVy <= 5)
        {
            auto lr = laten().get (rad);
            lr.tyst[(size_t) (cellRadIVy - 2)] = ! lr.tyst[(size_t) (cellRadIVy - 2)];
            laten().set (rad, lr);
            cellRad = -1;
        }
        andrat();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (cellRad < 0 || cellRadIVy < 1) return;
        const int d = (int) ((startY - e.position.y) / 6.0f);
        if (d == 0 && ! drog) return;
        drog = true;
        sattVarde (cellRad, cellRadIVy, startVarde + d * steg (cellRadIVy));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        // Takter, Spänningen och transponeringen: klick = ett steg upp, högerklick (eller Alt-klick) = ett
        // steg ned. Andra klicket i ett dubbelklick räknas inte (dubbelklicket nollställer).
        if (cellRad >= 0 && ! drog && (cellRadIVy == 1 || cellRadIVy >= 6) && e.getNumberOfClicks() < 2)
        {
            const int r = e.mods.isPopupMenu() || e.mods.isAltDown() ? -1 : 1;
            sattVarde (cellRad, cellRadIVy, startVarde + r * (cellRadIVy == 6 ? 10 : 1));
        }
        if (cellRad >= 0 && ! drog && ! e.mods.isPopupMenu() && cellRadIVy >= 2 && cellRadIVy <= 5)
        {
            auto lr = laten().get (cellRad);
            auto& v = lr.variant[(size_t) (cellRadIVy - 2)];
            v = (v + 1) % 3;
            laten().set (cellRad, lr);
            andrat();
        }
        cellRad = -1;
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int iv = radVid (e.position.y);
        if (e.position.x < etikettB || iv < 1 || (iv >= 2 && iv <= 5)) return;
        const int rad = sida * 16 + juce::jlimit (0, 15, (int) ((e.position.x - etikettB) / cellB()));
        if (rad >= laten().antal()) return;
        auto lr = laten().get (rad);
        if (iv == 1) lr.takter = 4;
        if (iv == 6) lr.spanning = -1;
        if (iv == 7) lr.transponering = 0;
        laten().set (rad, lr);
        andrat();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const int iv = radVid (e.position.y);
        const int riktning = hjul.riktning (w);
        if (riktning == 0) return;
        if (e.position.x < etikettB || iv == 0) { sida = (sida + (riktning > 0 ? 3 : 1)) % 4; repaint(); return; }
        const int rad = sida * 16 + juce::jlimit (0, 15, (int) ((e.position.x - etikettB) / cellB()));
        if (rad >= laten().antal()) return;
        sattVarde (rad, iv, varde (laten().get (rad), iv) + riktning * steg (iv));
    }

    // Panelens mönster, varianter och tystnad som en rad (HÄMTA och första raden).
    LatRad franPanelen() const
    {
        LatRad r;
        for (int k = 0; k < antalKanaler; ++k)
        {
            r.monster[(size_t) k] = lage.monsterFor (k);
            r.variant[(size_t) k] = (int) lage.param (params::id::kanal (k, "variant"));
            r.tyst[(size_t) k] = lage.param (params::id::kanal (k, "tyst")) > 0.5f;
        }
        return r;
    }

    void tick()
    {
        juce::int64 h = lage.proc.latRad() * 7 + valdRad * 131 + sida * 9973 + laten().antal() * 104729;
        for (int c = 0; c < 16; ++c)
        {
            const auto lr = laten().get (sida * 16 + c);
            h = h * 1000003 + lr.takter + 64 * (lr.spanning + 1) + 6464 * (lr.transponering + 24);
            for (int k = 0; k < antalKanaler; ++k) h = h * 131 + lr.monster[(size_t) k] + 16 * lr.variant[(size_t) k] + 64 * (int) lr.tyst[(size_t) k];
        }
        if (h != hash) { hash = h; repaint(); }
        // Sidan följer raden som spelas.
        const int spelar = lage.proc.latRad();
        if (spelar >= 0 && spelar / 16 != sida && spelar != forraSpelar) { sida = spelar / 16; repaint(); }
        forraSpelar = spelar;
    }

private:
    float cellB() const { return ((float) getWidth() - etikettB - 4.0f) / 16.0f; }
    float radH() const { return (float) getHeight() / (float) antalRaderIVy; }
    int radVid (float y) const { return juce::jlimit (0, antalRaderIVy - 1, (int) (y / radH())); }

    static int varde (const LatRad& r, int iv)
    {
        if (iv == 1) return r.takter;
        if (iv >= 2 && iv <= 5) return r.monster[(size_t) (iv - 2)];
        if (iv == 6) return r.spanning;
        if (iv == 7) return r.transponering;
        return 0;
    }
    static int steg (int iv) { return iv == 6 ? 5 : 1; }

    void sattVarde (int rad, int iv, int v)
    {
        auto lr = laten().get (rad);
        if (iv == 1) lr.takter = juce::jlimit (1, 64, v);
        else if (iv >= 2 && iv <= 5) lr.monster[(size_t) (iv - 2)] = juce::jlimit (0, antalMonster - 1, v);
        else if (iv == 6) lr.spanning = juce::jlimit (-1, 100, v < 0 && lr.spanning >= 0 ? -1 : (lr.spanning < 0 && v >= 0 ? 0 : v));
        else if (iv == 7) lr.transponering = juce::jlimit (-24, 24, v);
        else return;
        laten().set (rad, lr);
        andrat();
    }

    void andrat() { repaint(); lage.meddela(); }

    PanelLage& lage;
    Hjul hjul;
    juce::int64 hash = 0;
    int cellRad = -1, cellRadIVy = 0, startVarde = 0, forraSpelar = -1;
    float startY = 0.0f;
    bool drog = false;
};

} // namespace km::vhs
