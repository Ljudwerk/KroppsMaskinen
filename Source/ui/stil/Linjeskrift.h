// Linjeskriften: panelens tryckta versaler, ritade som raka linjer i kod (inga typsnittsfiler).
//
// Varje tecken är ett antal polylinjer i en ruta som är 4 enheter bred och 8 hög (y nedåt), med
// fasade hörn i stället för bågar – smala, tekniska versaler som på 80-talets svarta hemelektronik.
// Samma skrift används tunn (silvertryck, 6–9 px) och fet (modellnamnet, rubriker).
// ÅÄÖ har sina prickar och ring ovanför versalhöjden, så lämna lite luft över raden.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <map>
#include "Palett.h"

namespace km::stil::linjeskrift
{

struct Glyf
{
    float bredd = 4.0f;    // i enheter (versalhöjd = 8)
    juce::Path form;       // i enheter
};

namespace detalj
{
    // "x,y x,y|x,y x,y" – '|' börjar en ny linje.
    inline juce::Path tolka (const char* drag)
    {
        juce::Path p;
        for (auto& linje : juce::StringArray::fromTokens (juce::String (drag), "|", ""))
        {
            bool forst = true;
            for (auto& punkt : juce::StringArray::fromTokens (linje.trim(), " ", ""))
            {
                if (punkt.isEmpty()) continue;
                const float x = punkt.upToFirstOccurrenceOf (",", false, false).getFloatValue();
                const float y = punkt.fromFirstOccurrenceOf (",", false, false).getFloatValue();
                if (forst) p.startNewSubPath (x, y); else p.lineTo (x, y);
                forst = false;
            }
        }
        return p;
    }

    inline const std::map<juce::juce_wchar, Glyf>& tabell()
    {
        static const std::map<juce::juce_wchar, Glyf> t = []
        {
            std::map<juce::juce_wchar, Glyf> m;
            auto def = [&m] (juce::juce_wchar c, float b, const char* d) { m[c] = Glyf { b, tolka (d) }; };
            const char* O = "1,0 3,0 4,1 4,7 3,8 1,8 0,7 0,1 1,0";
            const char* A = "0,8 0,1 1,0 3,0 4,1 4,8|0,4.5 4,4.5";
            def ('A', 4, A);
            def ('B', 4, "0,4 3,4 4,5 4,7 3,8 0,8 0,0 3,0 4,1 4,3 3,4");
            def ('C', 4, "4,1.5 4,1 3,0 1,0 0,1 0,7 1,8 3,8 4,7 4,6.5");
            def ('D', 4, "0,0 3,0 4,1 4,7 3,8 0,8 0,0");
            def ('E', 4, "4,0 0,0 0,8 4,8|0,4 3,4");
            def ('F', 4, "4,0 0,0 0,8|0,4 3,4");
            def ('G', 4, "4,1.5 4,1 3,0 1,0 0,1 0,7 1,8 3,8 4,7 4,4.5 2.2,4.5");
            def ('H', 4, "0,0 0,8|4,0 4,8|0,4 4,4");
            def ('I', 0, "0,0 0,8");
            def ('J', 4, "4,0 4,7 3,8 1,8 0,7 0,6");
            def ('K', 4, "0,0 0,8|4,0 0,5|1.7,3.3 4,8");
            def ('L', 4, "0,0 0,8 4,8");
            def ('M', 5, "0,8 0,0 2.5,4.5 5,0 5,8");
            def ('N', 4, "0,8 0,0 4,8 4,0");
            def ('O', 4, O);
            def ('P', 4, "0,8 0,0 3,0 4,1 4,3.5 3,4.5 0,4.5");
            def ('Q', 4, "1,0 3,0 4,1 4,7 3,8 1,8 0,7 0,1 1,0|2.4,6 4,8.6");
            def ('R', 4, "0,8 0,0 3,0 4,1 4,3.5 3,4.5 0,4.5|2,4.5 4,8");
            def ('S', 4, "4,1.5 4,1 3,0 1,0 0,1 0,3 1,4 3,4 4,5 4,7 3,8 1,8 0,7 0,6.5");
            def ('T', 4, "0,0 4,0|2,0 2,8");
            def ('U', 4, "0,0 0,7 1,8 3,8 4,7 4,0");
            def ('V', 4, "0,0 2,8 4,0");
            def ('W', 5, "0,0 1.2,8 2.5,2.5 3.8,8 5,0");
            def ('X', 4, "0,0 4,8|4,0 0,8");
            def ('Y', 4, "0,0 2,4 4,0|2,4 2,8");
            def ('Z', 4, "0,0 4,0 0,8 4,8");
            def (0x00C5, 4, "0,8 0,1 1,0 3,0 4,1 4,8|0,4.5 4,4.5|1.4,-2.6 2.6,-2.6 2.6,-1.2 1.4,-1.2 1.4,-2.6");   // Å
            def (0x00C4, 4, "0,8 0,1 1,0 3,0 4,1 4,8|0,4.5 4,4.5|1,-2.4 1,-1.2|3,-2.4 3,-1.2");                  // Ä
            def (0x00D6, 4, "1,0 3,0 4,1 4,7 3,8 1,8 0,7 0,1 1,0|1,-2.4 1,-1.2|3,-2.4 3,-1.2");                  // Ö
            def (0x00C9, 4, "4,0 0,0 0,8 4,8|0,4 3,4|1.5,-1.2 3,-2.6");                                          // É
            def (0x00DC, 4, "0,0 0,7 1,8 3,8 4,7 4,0|1,-2.4 1,-1.2|3,-2.4 3,-1.2");                              // Ü

            def ('0', 4, "1,0 3,0 4,1 4,7 3,8 1,8 0,7 0,1 1,0|4,1 0,7");
            def ('1', 3, "0,1.5 1.5,0 1.5,8|0,8 3,8");
            def ('2', 4, "0,1.5 0,1 1,0 3,0 4,1 4,3 0,8 4,8");
            def ('3', 4, "0,1 1,0 3,0 4,1 4,3 3,4 1.4,4|3,4 4,5 4,7 3,8 1,8 0,7");
            def ('4', 4, "3,8 3,0 0,5.5 0,6 4,6");
            def ('5', 4, "4,0 0,0 0,3.6 3,3.6 4,4.6 4,7 3,8 1,8 0,7");
            def ('6', 4, "3.6,0 1,0 0,1 0,7 1,8 3,8 4,7 4,5 3,4 0,4");
            def ('7', 4, "0,0 4,0 4,1 1.6,8");
            def ('8', 4, "1,4 0,3 0,1 1,0 3,0 4,1 4,3 3,4 1,4 0,5 0,7 1,8 3,8 4,7 4,5 3,4");
            def ('9', 4, "0.4,8 3,8 4,7 4,1 3,0 1,0 0,1 0,3 1,4 4,4");

            def (' ', 2.4f, "");
            def ('-', 3, "0,4.5 3,4.5");
            def (0x2013, 4, "0,4.5 4,4.5");                       // –
            def (0x2212, 4, "0,4.5 4,4.5");                       // −
            def ('.', 0.6f, "0,7.4 0.6,7.4 0.6,8 0,8 0,7.4");
            def (',', 0.8f, "0.4,7.2 0.4,8 -0.2,9.2");
            def (':', 0.6f, "0,2.4 0.6,2.4 0.6,3 0,3 0,2.4|0,7.4 0.6,7.4 0.6,8 0,8 0,7.4");
            def ('/', 3, "0,8 3,0");
            def ('+', 4, "2,2.5 2,6.5|0,4.5 4,4.5");
            def ('%', 4, "0,8 4,0|0.3,0.3 1.5,0.3 1.5,2.3 0.3,2.3 0.3,0.3|2.5,5.7 3.7,5.7 3.7,7.7 2.5,7.7 2.5,5.7");
            def ('\'', 0.6f, "0.3,0 0.3,2.4");
            def ('(', 2, "2,0 0.8,1.4 0.8,6.6 2,8");
            def (')', 2, "0,0 1.2,1.4 1.2,6.6 0,8");
            def (0x00D7, 4, "0.4,2.9 3.6,6.1|3.6,2.9 0.4,6.1");          // ×
            def ('=', 4, "0,3 4,3|0,6 4,6");
            def ('#', 5, "1.6,0 1,8|4,0 3.4,8|0,2.6 5,2.6|0,5.4 5,5.4");
            def (0x00B7, 0.6f, "0,4.2 0.6,4.2 0.6,4.8 0,4.8 0,4.2");     // ·
            def (0x00B0, 2, "0,0 2,0 2,2 0,2 0,0");                       // °
            def (0x25B6, 4, "0,0.5 4,4 0,7.5 0,0.5");                    // ▶
            def (0x25C0, 4, "4,0.5 0,4 4,7.5 4,0.5");                    // ◀
            def (0x2192, 6, "0,4 6,4|3.6,1.6 6,4 3.6,6.4");              // →
            def (0x2190, 6, "6,4 0,4|2.4,1.6 0,4 2.4,6.4");              // ←
            def (0x2191, 4, "2,8 2,0|0,2.4 2,0 4,2.4");                  // ↑
            def (0x2193, 4, "2,0 2,8|0,5.6 2,8 4,5.6");                  // ↓
            def (0x2160, 0, "0,0 0,8");                                   // Ⅰ (romerska siffror tecknas som I)
            return m;
        }();
        return t;
    }
}

inline const Glyf* glyf (juce::juce_wchar c)
{
    auto& t = detalj::tabell();
    auto it = t.find (c);
    // Versalen för c – också å, ä, ö och de andra Latin-1-gemenerna, som toUpperCase() missar på macOS
    // (samma som krang::ui::versal i krang-libs; Kroppsmaskinen använder inte krang-libs).
    const juce::juce_wchar v = (c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7) ? c - 32 : juce::CharacterFunctions::toUpperCase (c);
    if (it == t.end()) it = t.find (v);
    return it != t.end() ? &it->second : nullptr;
}

// Stil för en textrad.
struct Stil
{
    float hojd = 8.0f;         // versalhöjd i designpixlar
    float tjocklek = 1.0f;     // linjebredd i designpixlar
    float spar = 1.8f;         // spärrning mellan tecken, i enheter
    float smal = 1.0f;         // horisontell skala (mindre än 1 = smalare)
};

inline float bredd (const juce::String& text, const Stil& s)
{
    const float enhet = s.hojd / 8.0f;
    float b = 0.0f; int n = 0;
    for (auto p = text.getCharPointer(); ! p.isEmpty(); ++n)
    {
        const auto c = p.getAndAdvance();
        if (auto* gl = glyf (c)) b += gl->bredd * s.smal;
        else                     b += 4.0f * s.smal;
    }
    if (n > 1) b += (float) (n - 1) * s.spar;
    return b * enhet + s.tjocklek;
}

// Bygger textens kontur (mittlinjer) med versalens överkant på y och vänsterkant på x.
inline juce::Path bana (const juce::String& text, float x, float y, const Stil& s)
{
    const float enhet = s.hojd / 8.0f;
    juce::Path ut;
    float pen = x + s.tjocklek * 0.5f;
    for (auto p = text.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        const auto* gl = glyf (c);
        if (gl != nullptr && ! gl->form.isEmpty())
            ut.addPath (gl->form, juce::AffineTransform::scale (enhet * s.smal, enhet).translated (pen, y));
        pen += ((gl != nullptr ? gl->bredd : 4.0f) * s.smal + s.spar) * enhet;
    }
    return ut;
}

// Ritar texten i rutan med given justering (lodrätt centreras versalhöjden).
inline void rita (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> ruta, const Stil& s,
                  juce::Justification just = juce::Justification::centred)
{
    const float b = bredd (text, s);
    float x = ruta.getX();
    if (just.testFlags (juce::Justification::horizontallyCentred)) x = ruta.getCentreX() - b * 0.5f;
    else if (just.testFlags (juce::Justification::right))          x = ruta.getRight() - b;
    float y = ruta.getCentreY() - s.hojd * 0.5f;
    if (just.testFlags (juce::Justification::top))                 y = ruta.getY();
    else if (just.testFlags (juce::Justification::bottom))         y = ruta.getBottom() - s.hojd;
    g.strokePath (bana (text, x, y, s),
                  juce::PathStrokeType (s.tjocklek, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
}

// Bekvämlighet: rita med vänsterkant/överkant i en punkt.
inline void rita (juce::Graphics& g, const juce::String& text, float x, float y, const Stil& s)
{
    g.strokePath (bana (text, x, y, s),
                  juce::PathStrokeType (s.tjocklek, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
}

} // namespace km::stil::linjeskrift
