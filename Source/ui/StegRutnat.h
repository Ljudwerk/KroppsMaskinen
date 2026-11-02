// Stegrutnätet (fas 0, grått): trummornas tre spår och Kolvbasen, sexton steg per sida, sidorna I–IV.
//
// Trummor: klick växlar av → slag → accent. Skift-klick växlar ratchet (1–4 slag), Alt-klick växlar chans.
// Kolvbasen: klick växlar av → ton → accent. Dra upp/ned eller rulla för att ändra tonen (±12 halvtoner),
// högerklick växlar glid, Alt-klick växlar oktav (0 → upp → ned).
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
        for (int i = 0; i < 4; ++i)
        {
            auto& b = sidKnappar[(size_t) i];
            b.setButtonText (juce::StringArray { "I", "II", "III", "IV" }[i]);
            b.setClickingTogglesState (true);
            b.setRadioGroupId (8701);
            b.onClick = [this, i] { if (sidKnappar[(size_t) i].getToggleState()) { sida = i; repaint(); } };
            addAndMakeVisible (b);
        }
        sidKnappar[0].setToggleState (true, juce::dontSendNotification);
        folj.setButtonText (juce::String::fromUTF8 ("Följ"));
        folj.setClickingTogglesState (true);
        addAndMakeVisible (folj);
        startTimerHz (30);
    }

    void resized() override
    {
        auto top = getLocalBounds().removeFromTop (rubrikHojd).reduced (0, 2);
        top.removeFromLeft (etikettBredd);
        for (auto& b : sidKnappar) { b.setBounds (top.removeFromLeft (44)); top.removeFromLeft (4); }
        top.removeFromLeft (8);
        folj.setBounds (top.removeFromLeft (60));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff2b2b2b));
        const int trumSteg = proc.trumStegNu(), basSteg = proc.basStegNu();

        for (int rad = 0; rad < antalRader; ++rad)
        {
            auto r = radYta (rad);
            g.setColour (juce::Colours::lightgrey);
            g.setFont (juce::FontOptions (14.0f));
            g.drawText (radNamn (rad), r.removeFromLeft (etikettBredd).reduced (4, 0), juce::Justification::centredLeft);

            for (int c = 0; c < 16; ++c)
            {
                const int steg = sida * 16 + c;
                auto cell = cellYta (rad, c).toFloat().reduced (2.0f);
                const bool spelas = (rad < 3 ? trumSteg : basSteg) == steg;
                const int langd = (int) *proc.state.getRawParameterValue (rad < 3 ? params::id::trumLangd : params::id::basLangd);
                const bool utanfor = steg >= langd;

                auto bas = juce::Colour (c % 4 == 0 ? 0xff4a4a4a : 0xff3c3c3c);
                if (utanfor) bas = bas.darker (0.6f);
                g.setColour (bas);
                g.fillRoundedRectangle (cell, 3.0f);

                if (rad < 3) ritaTrumSteg (g, cell, proc.trummor.get ((int) byggdaTrummor[(size_t) rad], steg));
                else ritaTonSteg (g, cell, proc.kolvbas.get (steg));

                if (spelas)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.8f));
                    g.drawRoundedRectangle (cell, 3.0f, 2.0f);
                }
            }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto [rad, c] = traff (e.getPosition());
        if (rad < 0) return;
        const int steg = sida * 16 + c;
        dragRad = rad; dragSteg = steg; dragStartY = e.y; dragStartTon = 0; dragade = false;

        if (rad < 3)
        {
            const int spr = (int) byggdaTrummor[(size_t) rad];
            auto s = proc.trummor.get (spr, steg);
            if (e.mods.isShiftDown())     s.ratchet = (s.ratchet + 1) % 4;
            else if (e.mods.isAltDown())  s.chans = (s.chans + 1) % 4;
            else                          s.slag = (s.slag + 1) % 3;
            proc.trummor.set (spr, steg, s);
        }
        else
        {
            auto s = proc.kolvbas.get (steg);
            dragStartTon = s.halvtoner;
            if (e.mods.isPopupMenu())    s.glid = ! s.glid;
            else if (e.mods.isAltDown()) s.oktav = s.oktav == 0 ? 1 : (s.oktav == 1 ? -1 : 0);
            else return;               // vanligt klick: växla när musen släpps (om man inte drog)
            proc.kolvbas.set (steg, s);
        }
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragRad != 3) return;
        const int delta = (dragStartY - e.y) / 6;
        if (delta == 0 && ! dragade) return;
        dragade = true;
        auto s = proc.kolvbas.get (dragSteg);
        s.halvtoner = juce::jlimit (-12, 12, dragStartTon + delta);
        if (s.slag == 0) s.slag = 1;
        proc.kolvbas.set (dragSteg, s);
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragRad == 3 && ! dragade && ! e.mods.isPopupMenu() && ! e.mods.isAltDown())
        {
            auto s = proc.kolvbas.get (dragSteg);
            s.slag = (s.slag + 1) % 3;
            proc.kolvbas.set (dragSteg, s);
            repaint();
        }
        dragRad = -1;
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const auto [rad, c] = traff (e.getPosition());
        if (rad != 3) return;
        const int steg = sida * 16 + c;
        auto s = proc.kolvbas.get (steg);
        s.halvtoner = juce::jlimit (-12, 12, s.halvtoner + (w.deltaY > 0 ? 1 : -1));
        proc.kolvbas.set (steg, s);
        repaint();
    }

    static constexpr int antalRader = 4, rubrikHojd = 30, etikettBredd = 110;

private:
    void timerCallback() override
    {
        if (folj.getToggleState())
        {
            const int s = juce::jmax (proc.trumStegNu(), proc.basStegNu());
            if (s >= 0 && s / 16 != sida)
            {
                sida = s / 16;
                sidKnappar[(size_t) sida].setToggleState (true, juce::dontSendNotification);
            }
        }
        repaint();
    }

    static juce::String radNamn (int rad)
    {
        return rad < 3 ? juce::String::fromUTF8 (trumNamn (byggdaTrummor[(size_t) rad])) : juce::String ("Kolvbasen");
    }

    int radHojd() const { return juce::jlimit (30, 56, (getHeight() - rubrikHojd) / antalRader); }

    juce::Rectangle<int> radYta (int rad) const { return { 0, rubrikHojd + rad * radHojd(), getWidth(), radHojd() }; }

    juce::Rectangle<int> cellYta (int rad, int c) const
    {
        const float w = (float) (getWidth() - etikettBredd) / 16.0f;
        return { etikettBredd + (int) (c * w), rubrikHojd + rad * radHojd(), (int) w, radHojd() };
    }

    std::pair<int, int> traff (juce::Point<int> p) const
    {
        if (p.x < etikettBredd || p.y < rubrikHojd) return { -1, -1 };
        const int rad = (p.y - rubrikHojd) / radHojd();
        const int c = (int) ((p.x - etikettBredd) / ((float) (getWidth() - etikettBredd) / 16.0f));
        if (rad >= antalRader || c < 0 || c >= 16) return { -1, -1 };
        return { rad, c };
    }

    static void ritaTrumSteg (juce::Graphics& g, juce::Rectangle<float> cell, const TrumSteg& s)
    {
        if (s.slag == 0) return;
        g.setColour (s.slag == 2 ? juce::Colour (0xffe03020) : juce::Colour (0xffa04030));
        g.fillRoundedRectangle (cell.reduced (4.0f), 2.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (11.0f));
        juce::String t;
        if (s.ratchet > 0) t << "x" << (s.ratchet + 1);
        if (s.chans > 0) t << (t.isEmpty() ? "" : " ") << (int) (chansVarden[(size_t) s.chans] * 100.0f) << "%";
        if (t.isNotEmpty()) g.drawText (t, cell, juce::Justification::centred);
    }

    static void ritaTonSteg (juce::Graphics& g, juce::Rectangle<float> cell, const TonSteg& s)
    {
        g.setFont (juce::FontOptions (12.0f));
        if (s.slag == 0)
        {
            if (s.glid) { g.setColour (juce::Colours::grey); g.drawText ("~", cell, juce::Justification::centred); }
            return;
        }
        g.setColour (s.slag == 2 ? juce::Colour (0xffe03020) : juce::Colour (0xffa04030));
        g.fillRoundedRectangle (cell.reduced (4.0f), 2.0f);
        g.setColour (juce::Colours::white);
        juce::String t = s.halvtoner > 0 ? "+" + juce::String (s.halvtoner) : juce::String (s.halvtoner);
        if (s.oktav > 0) t << juce::String::fromUTF8 ("↑");
        if (s.oktav < 0) t << juce::String::fromUTF8 ("↓");
        if (s.glid) t << "~";
        g.drawText (t, cell, juce::Justification::centred);
    }

    KroppsmaskinenProcessor& proc;
    std::array<juce::TextButton, 4> sidKnappar;
    juce::TextButton folj;
    int sida = 0;
    int dragRad = -1, dragSteg = 0, dragStartY = 0, dragStartTon = 0;
    bool dragade = false;
};

} // namespace km::ui
