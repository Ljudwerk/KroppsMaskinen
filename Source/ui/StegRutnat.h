// Stegrutnätet (grått, fas 0–2): den valda kanalens mönster, sexton steg per sida, sidorna I–IV.
// Rutnätet visar och ändrar mönstret som är valt i kanalens Mönster-ratt, variant A eller B.
//
// Trummor: klick växlar av → slag → accent. Skift-klick växlar ratchet (1–4 slag), Alt-klick växlar chans.
// Kolvbasen: klick växlar av → ton → accent. Dra upp/ned eller rulla för att ändra tonen (±12 halvtoner),
// högerklick växlar glid, Alt-klick växlar oktav (0 → upp → ned), Skift-klick växlar tonlängd.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"

namespace km::ui
{

class StegRutnat final : public juce::Component, private juce::Timer
{
public:
    explicit StegRutnat (KroppsmaskinenProcessor& p) : proc (p)
    {
        auto toggle = [this] (juce::TextButton& b, const juce::String& text, int grupp)
        {
            b.setButtonText (text);
            b.setClickingTogglesState (true);
            if (grupp != 0) b.setRadioGroupId (grupp);
            addAndMakeVisible (b);
        };
        for (int i = 0; i < 4; ++i)
        {
            toggle (sidKnappar[(size_t) i], juce::StringArray { "I", "II", "III", "IV" }[i], 8701);
            sidKnappar[(size_t) i].onClick = [this, i] { if (sidKnappar[(size_t) i].getToggleState()) { sida = i; repaint(); } };
        }
        sidKnappar[0].setToggleState (true, juce::dontSendNotification);
        toggle (folj, juce::String::fromUTF8 ("Följ"), 0);
        toggle (variantA, "A", 8702);
        toggle (variantB, "B", 8702);
        variantA.setToggleState (true, juce::dontSendNotification);
        variantA.onClick = variantB.onClick = [this] { repaint(); };
        addAndMakeVisible (rensa);
        rensa.setButtonText ("Rensa");
        rensa.onClick = [this] { if (tonKanal()) ton().rensa(); else trum().rensa(); repaint(); };
        addAndMakeVisible (kopiera);
        kopiera.setButtonText (juce::String::fromUTF8 ("Kopiera A→B"));
        kopiera.onClick = [this] { kopieraAB(); };
        startTimerHz (30);
    }

    void setKanal (Kanal k) { kanal = k; repaint(); }

    void resized() override
    {
        auto top = getLocalBounds().removeFromTop (rubrikHojd).reduced (0, 3);
        top.removeFromLeft (etikettBredd);
        for (auto& b : sidKnappar) { b.setBounds (top.removeFromLeft (40)); top.removeFromLeft (3); }
        top.removeFromLeft (6);
        folj.setBounds (top.removeFromLeft (56));
        top.removeFromLeft (24);
        variantA.setBounds (top.removeFromLeft (32));
        variantB.setBounds (top.removeFromLeft (32));
        top.removeFromLeft (8);
        kopiera.setBounds (top.removeFromLeft (100));
        rensa.setBounds (top.removeFromRight (70));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff2b2b2b));
        const bool foljer = kanal == Kanal::Kolvbas2 && foljerKolvbasen();
        const int stegNu = proc.stegNu (kanal);
        const int spelar = proc.monsterNu (kanal);
        const bool visaSteg = spelar == monsterIndex() || foljer;
        const int langd = langdFor (foljer ? Kanal::Kolvbasen : kanal);

        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (juce::String::fromUTF8 ("Mönster ") + juce::String (monsterIndex() + 1) + (variant() == 0 ? "A" : "B"),
                    getLocalBounds().removeFromTop (rubrikHojd).withWidth (etikettBredd).reduced (4, 0), juce::Justification::centredLeft);

        for (int rad = 0; rad < antalRader(); ++rad)
        {
            auto r = radYta (rad);
            g.setColour (juce::Colours::lightgrey);
            g.setFont (juce::FontOptions (juce::jmin (14.0f, (float) radHojd() * 0.6f)));
            g.drawText (radNamn (rad), r.removeFromLeft (etikettBredd).reduced (4, 0), juce::Justification::centredLeft);

            for (int c = 0; c < 16; ++c)
            {
                const int steg = sida * 16 + c;
                auto cell = cellYta (rad, c).toFloat().reduced (1.5f);
                auto bas = juce::Colour (c % 4 == 0 ? 0xff4a4a4a : 0xff3c3c3c);
                if (steg >= langd) bas = bas.darker (0.6f);
                g.setColour (bas);
                g.fillRoundedRectangle (cell, 3.0f);

                if (tonKanal()) ritaTonSteg (g, cell, (foljer ? proc.banker.kolvbasen.get (monsterFor (Kanal::Kolvbasen), variant()) : ton()).get (steg));
                else ritaTrumSteg (g, cell, trum().get (rad, steg));

                if (visaSteg && stegNu == steg)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.8f));
                    g.drawRoundedRectangle (cell, 3.0f, 2.0f);
                }
            }
        }

        if (foljer)
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (getLocalBounds().withTrimmedTop (rubrikHojd));
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (16.0f));
            g.drawText (juce::String::fromUTF8 ("Kolvbas 2 följer Kolvbasen – stäng av Följ Kolvbasen för att ge den ett eget mönster"),
                        getLocalBounds().withTrimmedTop (rubrikHojd), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (kanal == Kanal::Kolvbas2 && foljerKolvbasen()) return;
        const auto [rad, c] = traff (e.getPosition());
        if (rad < 0) return;
        const int steg = sida * 16 + c;
        dragRad = rad; dragSteg = steg; dragStartY = e.y; dragade = false;

        if (! tonKanal())
        {
            auto s = trum().get (rad, steg);
            if (e.mods.isShiftDown())     s.ratchet = (s.ratchet + 1) % 4;
            else if (e.mods.isAltDown())  s.chans = (s.chans + 1) % 4;
            else                          s.slag = (s.slag + 1) % 3;
            trum().set (rad, steg, s);
        }
        else
        {
            auto s = ton().get (steg);
            dragStartTon = s.halvtoner;
            if (e.mods.isPopupMenu())     s.glid = ! s.glid;
            else if (e.mods.isAltDown())  s.oktav = s.oktav == 0 ? 1 : (s.oktav == 1 ? -1 : 0);
            else if (e.mods.isShiftDown()) s.langd = (s.langd + 1) % 8;
            else return;                  // vanligt klick: växla när musen släpps (om man inte drog)
            ton().set (steg, s);
        }
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! tonKanal() || dragRad < 0) return;
        const int delta = (dragStartY - e.y) / 6;
        if (delta == 0 && ! dragade) return;
        dragade = true;
        auto s = ton().get (dragSteg);
        s.halvtoner = juce::jlimit (-12, 12, dragStartTon + delta);
        if (s.slag == 0) s.slag = 1;
        ton().set (dragSteg, s);
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (tonKanal() && dragRad >= 0 && ! dragade && ! e.mods.isPopupMenu() && ! e.mods.isAltDown() && ! e.mods.isShiftDown())
        {
            auto s = ton().get (dragSteg);
            s.slag = (s.slag + 1) % 3;
            ton().set (dragSteg, s);
            repaint();
        }
        dragRad = -1;
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! tonKanal() || (kanal == Kanal::Kolvbas2 && foljerKolvbasen())) return;
        const auto [rad, c] = traff (e.getPosition());
        if (rad < 0) return;
        auto s = ton().get (sida * 16 + c);
        s.halvtoner = juce::jlimit (-12, 12, s.halvtoner + (w.deltaY > 0 ? 1 : -1));
        ton().set (sida * 16 + c, s);
        repaint();
    }

    static constexpr int rubrikHojd = 30, etikettBredd = 110;

private:
    void timerCallback() override
    {
        if (folj.getToggleState())
        {
            const int s = proc.stegNu (kanal);
            if (s >= 0 && s / 16 != sida)
            {
                sida = s / 16;
                sidKnappar[(size_t) sida].setToggleState (true, juce::dontSendNotification);
            }
        }
        repaint();
    }

    bool tonKanal() const { return kanal != Kanal::Stampverket; }
    int antalRader() const { return tonKanal() ? 1 : antalSpar; }
    int variant() const { return variantB.getToggleState() ? 1 : 0; }

    float param (const juce::String& id) const { return proc.state.getRawParameterValue (id)->load(); }
    int monsterFor (Kanal k) const { return (int) param (params::id::kanal ((int) k, "monster")) - 1; }
    int monsterIndex() const { return monsterFor (kanal); }
    int langdFor (Kanal k) const { return (int) param (params::id::kanal ((int) k, "langd")); }
    bool foljerKolvbasen() const { return param (params::id::folj) > 0.5f; }

    TrumMonster& trum() { return proc.banker.trummor.get (monsterIndex(), variant()); }
    TonMonster& ton() { return proc.banker.ton ((int) kanal).get (monsterIndex(), variant()); }

    void kopieraAB()
    {
        const int m = monsterIndex();
        if (tonKanal())
        {
            auto& bank = proc.banker.ton ((int) kanal);
            for (int s = 0; s < maxSteg; ++s) bank.get (m, 1).set (s, bank.get (m, 0).get (s));
        }
        else
        {
            for (int r = 0; r < antalSpar; ++r)
                for (int s = 0; s < maxSteg; ++s)
                    proc.banker.trummor.get (m, 1).set (r, s, proc.banker.trummor.get (m, 0).get (r, s));
        }
        repaint();
    }

    juce::String radNamn (int rad) const
    {
        return tonKanal() ? juce::String::fromUTF8 (kanalNamn (kanal)) : juce::String::fromUTF8 (trumNamn ((Trumma) rad));
    }

    int radHojd() const
    {
        const int h = (getHeight() - rubrikHojd) / antalRader();
        return tonKanal() ? juce::jmin (90, h) : h;
    }

    juce::Rectangle<int> radYta (int rad) const { return { 0, rubrikHojd + rad * radHojd(), getWidth(), radHojd() }; }

    juce::Rectangle<int> cellYta (int rad, int c) const
    {
        const float w = (float) (getWidth() - etikettBredd) / 16.0f;
        return { etikettBredd + (int) ((float) c * w), rubrikHojd + rad * radHojd(), (int) w, radHojd() };
    }

    std::pair<int, int> traff (juce::Point<int> p) const
    {
        if (p.x < etikettBredd || p.y < rubrikHojd) return { -1, -1 };
        const int rad = (p.y - rubrikHojd) / juce::jmax (1, radHojd());
        const int c = (int) ((float) (p.x - etikettBredd) / ((float) (getWidth() - etikettBredd) / 16.0f));
        if (rad >= antalRader() || c < 0 || c >= 16) return { -1, -1 };
        return { rad, c };
    }

    static void ritaTrumSteg (juce::Graphics& g, juce::Rectangle<float> cell, const TrumSteg& s)
    {
        if (s.slag == 0) return;
        g.setColour (s.slag == 2 ? juce::Colour (0xffe03020) : juce::Colour (0xffa04030));
        g.fillRoundedRectangle (cell.reduced (3.0f), 2.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (10.0f));
        juce::String t;
        if (s.ratchet > 0) t << "x" << (s.ratchet + 1);
        if (s.chans > 0) t << (t.isEmpty() ? "" : " ") << (int) (chansVarden[(size_t) s.chans] * 100.0f) << "%";
        if (t.isNotEmpty()) g.drawText (t, cell, juce::Justification::centred);
    }

    static void ritaTonSteg (juce::Graphics& g, juce::Rectangle<float> cell, const TonSteg& s)
    {
        g.setFont (juce::FontOptions (13.0f));
        if (s.slag == 0)
        {
            if (s.glid) { g.setColour (juce::Colours::grey); g.drawText ("~", cell, juce::Justification::centred); }
            return;
        }
        g.setColour (s.slag == 2 ? juce::Colour (0xffe03020) : juce::Colour (0xffa04030));
        // Tonlängden syns som stapelns bredd (ett halvt steg = halv cell).
        const float andel = (float) juce::jlimit (0.15, 1.0, tonLangder[(size_t) s.langd]);
        auto stapel = cell.reduced (3.0f);
        g.fillRoundedRectangle (stapel.withWidth (s.glid ? stapel.getWidth() : stapel.getWidth() * andel), 2.0f);
        g.setColour (juce::Colours::white);
        juce::String t = s.halvtoner > 0 ? "+" + juce::String (s.halvtoner) : juce::String (s.halvtoner);
        if (s.oktav > 0) t << juce::String::fromUTF8 ("↑");
        if (s.oktav < 0) t << juce::String::fromUTF8 ("↓");
        if (s.glid) t << "~";
        g.drawText (t, cell, juce::Justification::centred);
    }

    KroppsmaskinenProcessor& proc;
    Kanal kanal = Kanal::Kolvbasen;
    std::array<juce::TextButton, 4> sidKnappar;
    juce::TextButton folj, variantA, variantB, rensa, kopiera;
    int sida = 0;
    int dragRad = -1, dragSteg = 0, dragStartY = 0, dragStartTon = 0;
    bool dragade = false;
};

} // namespace km::ui
