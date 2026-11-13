// Klaviaturen: tre oktaver (22 vita tangenter) som spelar vald kanal.
//
// Kolvbasen MIDI-kanal 1, Kolvbas 2 kanal 2, Hugget kanal 3 och Stampverket kanal 10 (GM-kartan).
// När sekvensern går transponerar tangenterna i stället (se README). Med SPELA IN skriver tangenterna
// in toner i mönstret (panelen tar hand om det via 'nedslag').
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Reglage.h"
#include "PanelLage.h"
#include <set>

namespace km::vhs
{

class Klaviatur final : public juce::Component
{
public:
    explicit Klaviatur (PanelLage& l) : lage (l) { setOpaque (false); }

    static constexpr int antalVita = 22;
    int oktav = 1;                                    // lägsta tangenten är C<oktav> (C1 = MIDI 24)
    std::function<void (int notnr, float styrka)> nedslag;

    int basNot() const { return 12 * (oktav + 1); }
    int midiKanal() const { static const int k[antalKanaler] { 1, 2, 10, 3, 4 }; return k[juce::jlimit (0, antalKanaler - 1, lage.kanal)]; }

    void paint (juce::Graphics& g) override
    {
        std::set<int> ned;
        for (int i = 0; i < 37; ++i)
            if (lage.proc.keyboardState.isNoteOn (midiKanal(), basNot() + i)) ned.insert (i);
        stil::ritaTangentbord (g, getLocalBounds().toFloat(), antalVita, ned);
    }

    void mouseDown (const juce::MouseEvent& e) override { spela (tangent (e.position), e.position.y); }
    void mouseDrag (const juce::MouseEvent& e) override { const int t = tangent (e.position); if (t != aktiv) spela (t, e.position.y); }
    void mouseUp (const juce::MouseEvent&) override { slapp(); }

    // Datorns tangentbord (som i KrangBox): A W S E D F T G Y H U J K O L P = C … D♯ en oktav upp,
    // Z och X byter oktav. Panelen anropar detta när en tangent trycks ned eller släpps.
    static constexpr const char* datorTangenter = "awsedftgyhujkolp";

    // Returnerar sant om tangentbordets läge ändrade något.
    bool datorTangentbord()
    {
        bool andrat = false;
        const juce::String tangenter (datorTangenter);
        for (int i = 0; i < tangenter.length(); ++i)
        {
            const bool ned = juce::KeyPress::isKeyCurrentlyDown (tangenter[i]) || juce::KeyPress::isKeyCurrentlyDown (juce::CharacterFunctions::toUpperCase (tangenter[i]));
            auto& var = datorNed[(size_t) i];
            if (ned == (var >= 0)) continue;
            andrat = true;
            if (ned)
            {
                var = juce::jlimit (0, 127, basNot() + i);
                lage.proc.keyboardState.noteOn (midiKanal(), var, 0.8f);
                if (nedslag) nedslag (var, 0.8f);
            }
            else
            {
                lage.proc.keyboardState.noteOff (midiKanal(), var, 0.0f);
                var = -1;
            }
        }
        if (andrat) repaint();
        return andrat;
    }
    void slappDator()
    {
        for (auto& v : datorNed) if (v >= 0) { lage.proc.keyboardState.noteOff (midiKanal(), v, 0.0f); v = -1; }
    }

    void tick()
    {
        juce::int64 h = midiKanal() * 131 + basNot();
        for (int i = 0; i < 37; ++i) h = h * 3 + (lage.proc.keyboardState.isNoteOn (midiKanal(), basNot() + i) ? 1 : 0);
        if (h != hash) { hash = h; repaint(); }
    }

private:
    // Tangenten (halvtoner från första C) under punkten, svarta först.
    int tangent (Point<float> p) const
    {
        const float vb = (float) getWidth() / (float) antalVita, h = (float) getHeight();
        static const int svartEfter[5] { 0, 1, 3, 4, 5 }, svartTon[5] { 1, 3, 6, 8, 10 }, vitTon[7] { 0, 2, 4, 5, 7, 9, 11 };
        if (p.y < h * 0.62f)
            for (int okt = 0; okt * 7 < antalVita; ++okt)
                for (int j = 0; j < 5; ++j)
                {
                    const int vi = okt * 7 + svartEfter[j];
                    if (vi + 1 >= antalVita) continue;
                    const float x = (float) (vi + 1) * vb;
                    if (std::abs (p.x - x) < vb * 0.3f) return okt * 12 + svartTon[j];
                }
        const int i = juce::jlimit (0, antalVita - 1, (int) (p.x / vb));
        return (i / 7) * 12 + vitTon[i % 7];
    }

    void spela (int t, float y)
    {
        slapp();
        aktiv = t;
        const float styrka = juce::jlimit (0.3f, 1.0f, 0.4f + 0.6f * y / (float) getHeight());
        lage.proc.keyboardState.noteOn (midiKanal(), basNot() + t, styrka);
        if (nedslag) nedslag (basNot() + t, styrka);
        repaint();
    }
    void slapp()
    {
        if (aktiv >= 0) lage.proc.keyboardState.noteOff (midiKanal(), basNot() + aktiv, 0.0f);
        aktiv = -1;
        repaint();
    }

    PanelLage& lage;
    int aktiv = -1;
    std::array<int, 16> datorNed { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    juce::int64 hash = 0;
};

} // namespace km::vhs
