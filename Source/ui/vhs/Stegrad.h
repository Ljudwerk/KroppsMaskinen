// Stegraden: sexton stegknappar med lysdioder, sidorna I–IV och A/B.
//
// Stegraden redigerar vald kanals mönster (det som Mönster-väljaren pekar på), variant A eller B.
// I Stampverket redigeras vald trumma (som på en TR-maskin); trummorna väljs i KANAL-vyn.
//
// Trummor: klick växlar av → slag → accent. Skift-klick: ratchet (1–4 slag). Alt-klick: chans.
// Toner: klick växlar av → ton → accent. Dra upp/ned eller rulla: tonen (±12). Högerklick: glid.
// Alt-klick: oktav (0 → upp → ned). Skift-klick: tonlängd.
// SPELA IN: stegskrivning. Tangenterna skriver in toner (eller trumslag) vid markören, som flyttas fram.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Reglage.h"
#include "PanelLage.h"

namespace km::vhs
{

// Ändringar av ett steg, gemensamma för stegraden och sekvensvyn.
namespace redigering
{
    inline void trumKlick (TrumMonster& m, int spar, int steg, const juce::ModifierKeys& mods)
    {
        auto s = m.get (spar, steg);
        if (mods.isShiftDown())     s.ratchet = (s.ratchet + 1) % 4;
        else if (mods.isAltDown())  s.chans = (s.chans + 1) % 4;
        else                        s.slag = (s.slag + 1) % 3;
        m.set (spar, steg, s);
    }

    // Returnerar sant om klicket var färdigt (annars väntar man på att musen släpps eller dras).
    inline bool tonKlick (TonMonster& m, int steg, const juce::ModifierKeys& mods)
    {
        auto s = m.get (steg);
        if (mods.isPopupMenu())       s.glid = ! s.glid;
        else if (mods.isAltDown())    s.oktav = s.oktav == 0 ? 1 : (s.oktav == 1 ? -1 : 0);
        else if (mods.isShiftDown())  s.langd = (s.langd + 1) % 8;
        else return false;
        m.set (steg, s);
        return true;
    }

    inline void tonVaxla (TonMonster& m, int steg)
    {
        auto s = m.get (steg);
        s.slag = (s.slag + 1) % 3;
        m.set (steg, s);
    }

    inline void tonHojd (TonMonster& m, int steg, int halvtoner)
    {
        auto s = m.get (steg);
        s.halvtoner = juce::jlimit (-12, 12, halvtoner);
        if (s.slag == 0) s.slag = 1;
        m.set (steg, s);
    }

    // Kort text på knappen: tonen, oktav och glid, eller ratchet och chans.
    inline juce::String tonText (const TonSteg& s)
    {
        if (s.slag == 0) return s.glid ? "~" : juce::String();
        juce::String t = s.halvtoner > 0 ? "+" + juce::String (s.halvtoner) : (s.halvtoner < 0 ? stil::sv ("−") + juce::String (-s.halvtoner) : juce::String());
        if (s.oktav > 0) t << stil::sv ("↑");
        if (s.oktav < 0) t << stil::sv ("↓");
        if (s.glid) t << "~";
        return t;
    }
    inline juce::String trumText (const TrumSteg& s)
    {
        juce::String t;
        if (s.ratchet > 0) t << "X" << (s.ratchet + 1);
        if (s.chans > 0) t << (t.isEmpty() ? "" : " ") << (int) (chansVarden[(size_t) s.chans] * 100.0f) << "%";
        return t;
    }
}

class Stegrad final : public juce::Component
{
public:
    explicit Stegrad (PanelLage& l) : lage (l) { setOpaque (false); }

    static constexpr float bw = 48.0f, gap = 8.0f, grGap = 18.0f, ledY = 12.0f, ky = 24.0f, kh = 40.0f;

    Rectangle<float> knapp (int i) const
    {
        const float x = 12.0f + (float) i * (bw + gap) + (float) (i / 4) * (grGap - gap);
        return { x, ky, bw, kh };
    }
    Rectangle<float> sidKnapp (int i) const
    {
        const float sx = (float) getWidth() - 5.0f * 36.0f - 4.0f * 8.0f - 12.0f;
        return { sx + (float) i * 44.0f, ky + 6.0f, 36.0f, 28.0f };
    }
    // Ytan som ritas om för steg i (lysdiod, knapp och text).
    juce::Rectangle<int> stegYta (int i) const { return knapp (i).withTop (2.0f).expanded (3.0f, 0.0f).withBottom (ky + kh + 3.0f).getSmallestIntegerContainer(); }

    // Statiskt tryck (siffror, hakparenteser, SIDA) ritas av panelens bakgrund.
    void ritaTryck (juce::Graphics& g, Point<float> o) const
    {
        for (int i = 0; i < 16; ++i)
        {
            const auto k = knapp (i).translated (o.x, o.y);
            stil::tryck (g, juce::String (i + 1), { k.getX(), k.getBottom() + 6.0f, bw, 8.0f }, 6.0f,
                         juce::Justification::centred, i % 4 == 0 ? farg::vitt : farg::silver, i % 4 == 0 ? 1.0f : 0.8f, 1.6f);
            if (i % 4 == 3)
            {
                const float gx0 = knapp (i - 3).getX() + o.x, gx1 = k.getRight();
                const float y = o.y + ky + kh + 20.0f;
                g.setColour (farg::silverMatt);
                g.drawLine (gx0, y, gx1, y, 0.8f);
                g.drawLine (gx0, y - 3.0f, gx0, y, 0.8f);
                g.drawLine (gx1, y - 3.0f, gx1, y, 0.8f);
            }
        }
        const auto s0 = sidKnapp (0).translated (o.x, o.y), s4 = sidKnapp (4).translated (o.x, o.y);
        stil::tryck (g, "SIDA", { s0.getX(), o.y, sidKnapp (3).getRight() - sidKnapp (0).getX(), 8.0f }, 5.0f,
                     juce::Justification::centred, farg::silverMatt);
        stil::tryck (g, "VARIANT", { s4.getX() - 10.0f, o.y, s4.getWidth() + 20.0f, 8.0f }, 5.0f, juce::Justification::centred, farg::silverMatt);
        stil::tryck (g, stil::sv ("MÖNSTERSIDA · REDIGERA"), { s0.getX(), o.y + ky + kh + 4.0f, s4.getRight() - s0.getX(), 8.0f }, 4.8f,
                     juce::Justification::centred, farg::silverMatt);
    }

    void paint (juce::Graphics& g) override
    {
        const int spel = lage.spelsteg();
        const bool blink = (juce::Time::getMillisecondCounter() / 250) % 2 == 0;
        const auto clip = g.getClipBounds();
        for (int i = 0; i < 16; ++i)
        {
            if (! clip.intersects (stegYta (i))) continue;
            const int steg = lage.sida * 16 + i;
            const auto k = knapp (i);
            const bool utanfor = steg >= langd();
            int slag = 0;
            juce::String text;
            if (lage.tonKanal()) { const auto s = lage.ton().get (steg); slag = s.slag; text = redigering::tonText (s); }
            else { const auto s = lage.trum().get (lage.valdTrumma, steg); slag = s.slag; text = redigering::trumText (s); }

            const bool spelhuvud = steg == spel;
            const bool markor = lage.spelaIn && steg == lage.markor && blink;
            float styrka = slag == 2 ? 1.0f : (slag == 1 ? 0.55f : 0.0f);
            if (utanfor) styrka *= 0.3f;
            juce::Colour f = farg::lysRod;
            if (spelhuvud || markor) { f = farg::lysOrange; styrka = 1.0f; }
            stil::ritaLysdiod (g, { k.getCentreX(), ledY }, 2.6f, f, styrka);
            stil::ritaStegknapp (g, k, i / 4, spelhuvud);
            if (slag == 2)   // accent: liten röd trekant i tryck
            {
                juce::Path t;
                t.addTriangle (k.getRight() - 9.0f, ledY - 3.0f, k.getRight() - 3.0f, ledY - 3.0f, k.getRight() - 6.0f, ledY + 2.0f);
                g.setColour (farg::tryckRott);
                g.fillPath (t);
            }
            if (text.isNotEmpty())
                etikett (g, text, k.withTrimmedTop (kh - 15.0f).withTrimmedBottom (5.0f).translated (0.0f, spelhuvud ? 0.8f : 0.0f).reduced (3.0f, 0.0f),
                         5.6f, juce::Colours::black.withAlpha (0.72f));
            if (utanfor || lage.foljer())
            {
                g.setColour (juce::Colours::black.withAlpha (lage.foljer() ? 0.45f : 0.55f));
                g.fillRoundedRectangle (k.expanded (1.0f), 3.0f);
            }
        }

        // Sidknapparna och A/B.
        const int spelSida = spel >= 0 ? spel / 16 : -1;
        const int antalSidor = (langd() + 15) / 16;
        for (int i = 0; i < 5; ++i)
        {
            const auto k = sidKnapp (i);
            if (! clip.intersects (k.withTop (2.0f).getSmallestIntegerContainer())) continue;
            const bool vald = i < 4 ? i == lage.sida : lage.redigera == 1;
            float styrka = vald ? 1.0f : (i == spelSida ? 0.35f : 0.0f);
            stil::ritaLysdiod (g, { k.getCentreX(), ledY }, 2.2f, i == 4 || (! vald && i == spelSida) ? farg::lysOrange : farg::lysRod, styrka);
            auto yta = stil::ritaKnapp (g, k, i < 4 && vald);
            const juce::String t = i < 4 ? juce::StringArray { "I", "II", "III", "IV" }[i] : (lage.redigera == 0 ? "A" : "B");
            stil::tryck (g, t, yta, 7.0f, juce::Justification::centred, vald || i == 4 ? farg::vitt : (i < antalSidor ? farg::silver : farg::silverMatt), 1.0f, 2.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.position;
        for (int i = 0; i < 5; ++i)
            if (sidKnapp (i).contains (p))
            {
                if (i < 4) { lage.sida = i; lage.sidaValdVid = juce::Time::getMillisecondCounterHiRes(); }
                else lage.redigera = 1 - lage.redigera;
                repaint();
                lage.meddela();
                return;
            }
        dragSteg = -1;
        if (lage.foljer()) return;
        for (int i = 0; i < 16; ++i)
            if (knapp (i).expanded (gap * 0.5f, 10.0f).contains (p))
            {
                const int steg = lage.sida * 16 + i;
                if (lage.spelaIn) lage.markor = steg;
                if (! lage.tonKanal()) { redigering::trumKlick (lage.trum(), lage.valdTrumma, steg, e.mods); repaint (stegYta (i)); lage.meddela(); return; }
                dragSteg = steg; dragStartY = e.position.y; dragade = false;
                dragStartTon = lage.ton().get (steg).halvtoner;
                if (redigering::tonKlick (lage.ton(), steg, e.mods)) { dragSteg = -1; repaint (stegYta (i)); lage.meddela(); }
                return;
            }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragSteg < 0) return;
        const int delta = (int) ((dragStartY - e.position.y) / 6.0f);
        if (delta == 0 && ! dragade) return;
        dragade = true;
        redigering::tonHojd (lage.ton(), dragSteg, dragStartTon + delta);
        repaint (stegYta (dragSteg % 16));
        lage.meddela();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragSteg >= 0 && ! dragade) { redigering::tonVaxla (lage.ton(), dragSteg); repaint (stegYta (dragSteg % 16)); lage.meddela(); }
        dragSteg = -1;
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! lage.tonKanal() || lage.foljer()) return;
        for (int i = 0; i < 16; ++i)
            if (knapp (i).contains (e.position))
            {
                const int steg = lage.sida * 16 + i;
                redigering::tonHojd (lage.ton(), steg, lage.ton().get (steg).halvtoner + ((w.deltaY > 0.0f) != w.isReversed ? 1 : -1));
                repaint (stegYta (i));
                lage.meddela();
            }
    }

    // Anropas av panelens timer: ritar om de steg som ändrats (spelhuvud, mönster, markör) och följer
    // spelhuvudet till rätt sida när mönstret är längre än 16 steg.
    void tick()
    {
        const int spel = lage.spelsteg();
        if (spel >= 0 && spel / 16 != lage.sida && juce::Time::getMillisecondCounterHiRes() - lage.sidaValdVid > 4000.0)
        {
            lage.sida = spel / 16;
            repaint();
        }
        std::array<uint32_t, 16> nu {};
        for (int i = 0; i < 16; ++i)
        {
            const int steg = lage.sida * 16 + i;
            uint32_t h = (uint32_t) (steg == spel) | ((uint32_t) (steg >= langd()) << 1)
                       | ((uint32_t) (lage.spelaIn && steg == lage.markor && (juce::Time::getMillisecondCounter() / 250) % 2 == 0) << 2);
            if (lage.tonKanal()) { const auto s = lage.ton().get (steg); h |= (uint32_t) (s.slag | (s.halvtoner + 12) << 2 | (s.oktav + 1) << 7 | s.langd << 9 | (int) s.glid << 12) << 3; }
            else { const auto s = lage.trum().get (lage.valdTrumma, steg); h |= (uint32_t) (s.slag | s.chans << 2 | s.ratchet << 4) << 3; }
            nu[(size_t) i] = h;
        }
        const uint32_t sidH = (uint32_t) lage.sida | (uint32_t) lage.redigera << 2 | (uint32_t) ((spel >= 0 ? spel / 16 : 7) << 3)
                            | (uint32_t) langd() << 6 | (uint32_t) lage.kanal << 13 | (uint32_t) lage.valdTrumma << 15 | (uint32_t) lage.foljer() << 20;
        if (sidH != sidHash) { sidHash = sidH; repaint(); hash = nu; return; }
        for (int i = 0; i < 16; ++i)
            if (nu[(size_t) i] != hash[(size_t) i]) repaint (stegYta (i));
        hash = nu;
    }

private:
    int langd() const { return lage.langdFor (lage.foljer() ? 0 : lage.kanal); }

    PanelLage& lage;
    std::array<uint32_t, 16> hash {};
    uint32_t sidHash = 0xffffffff;
    int dragSteg = -1, dragStartTon = 0;
    float dragStartY = 0.0f;
    bool dragade = false;
};

} // namespace km::vhs
