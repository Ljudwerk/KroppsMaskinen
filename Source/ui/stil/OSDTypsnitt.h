// OSD-typsnittet: en punktmatris på 5×7, som texten en videobandspelare lägger över bilden.
//
// Versaler, siffror, ÅÄÖ, transportsymbolerna ▶ ◀ ■ ● ‖ och några skiljetecken. Ritas med fyllda
// rutor (ingen typsnittsfil) och kan användas både på skärmen och för märkningar på panelen.
// Okända tecken ritas som en tom ruta, så att de syns vid granskning.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>
#include <map>
#include "Palett.h"

namespace km::stil::osd
{

constexpr int kolumner = 5, rader = 7;
using Matris = std::array<uint8_t, rader>;     // en byte per rad, bit 4 = vänstra kolumnen

namespace detalj
{
    // Varje rad skrivs som fem tecken '#' / '.'.
    inline Matris rad (std::initializer_list<const char*> r)
    {
        Matris m {};
        int i = 0;
        for (auto* s : r)
        {
            uint8_t b = 0;
            for (int k = 0; k < kolumner && s[k] != 0; ++k)
                if (s[k] == '#') b |= (uint8_t) (1u << (kolumner - 1 - k));
            m[(size_t) i++] = b;
        }
        return m;
    }

    inline const std::map<juce::juce_wchar, Matris>& tabell()
    {
        static const std::map<juce::juce_wchar, Matris> t = []
        {
            std::map<juce::juce_wchar, Matris> m;
            auto d = [&m] (juce::juce_wchar c, std::initializer_list<const char*> r) { m[c] = rad (r); };
            d ('A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" });
            d ('B', { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." });
            d ('C', { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." });
            d ('D', { "###..", "#..#.", "#...#", "#...#", "#...#", "#..#.", "###.." });
            d ('E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" });
            d ('F', { "#####", "#....", "#....", "####.", "#....", "#....", "#...." });
            d ('G', { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####" });
            d ('H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" });
            d ('I', { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." });
            d ('J', { "..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.." });
            d ('K', { "#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#" });
            d ('L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" });
            d ('M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" });
            d ('N', { "#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#" });
            d ('O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." });
            d ('P', { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." });
            d ('Q', { ".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#" });
            d ('R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" });
            d ('S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." });
            d ('T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." });
            d ('U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." });
            d ('V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." });
            d ('W', { "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#." });
            d ('X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" });
            d ('Y', { "#...#", "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#.." });
            d ('Z', { "#####", "....#", "...#.", "..#..", ".#...", "#....", "#####" });
            d (0x00C5, { "..#..", ".#.#.", "..#..", ".###.", "#...#", "#####", "#...#" });   // Å
            d (0x00C4, { ".#.#.", ".....", ".###.", "#...#", "#####", "#...#", "#...#" });   // Ä
            d (0x00D6, { ".#.#.", ".....", ".###.", "#...#", "#...#", "#...#", ".###." });   // Ö
            d (0x00DC, { ".#.#.", ".....", "#...#", "#...#", "#...#", "#...#", ".###." });   // Ü
            d (0x00C9, { "...#.", "..#..", "#####", "#....", "####.", "#....", "#####" });   // É

            d ('0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." });
            d ('1', { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." });
            d ('2', { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" });
            d ('3', { "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." });
            d ('4', { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." });
            d ('5', { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." });
            d ('6', { "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." });
            d ('7', { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." });
            d ('8', { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." });
            d ('9', { ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." });

            d (' ', { ".....", ".....", ".....", ".....", ".....", ".....", "....." });
            d (':', { ".....", ".##..", ".##..", ".....", ".##..", ".##..", "....." });
            d ('.', { ".....", ".....", ".....", ".....", ".....", ".##..", ".##.." });
            d (',', { ".....", ".....", ".....", ".....", ".##..", "..#..", ".#..." });
            d ('-', { ".....", ".....", ".....", "#####", ".....", ".....", "....." });
            d ('+', { ".....", "..#..", "..#..", "#####", "..#..", "..#..", "....." });
            d ('/', { ".....", "....#", "...#.", "..#..", ".#...", "#....", "....." });
            d ('%', { "##...", "##..#", "...#.", "..#..", ".#...", "#..##", "...##" });
            d ('!', { "..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.." });
            d ('?', { ".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.." });
            d ('<', { "...#.", "..#..", ".#...", "#....", ".#...", "..#..", "...#." });
            d ('>', { ".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..." });
            d ('=', { ".....", ".....", "#####", ".....", "#####", ".....", "....." });
            d ('_', { ".....", ".....", ".....", ".....", ".....", ".....", "#####" });
            d ('(', { "...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#." });
            d (')', { ".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..." });
            d ('\'',{ ".##..", "..#..", ".#...", ".....", ".....", ".....", "....." });
            d ('#', { ".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#." });
            d ('*', { ".....", "..#..", "#.#.#", ".###.", "#.#.#", "..#..", "....." });
            d (0x00B0, { ".##..", "#..#.", ".##..", ".....", ".....", ".....", "....." });   // °
            d (0x2212, { ".....", ".....", ".....", "#####", ".....", ".....", "....." });   // −
            d (0x2013, { ".....", ".....", ".....", "#####", ".....", ".....", "....." });   // –
            d (0x25B6, { "#....", "##...", "###..", "####.", "###..", "##...", "#...." });   // ▶
            d (0x25C0, { "....#", "...##", "..###", ".####", "..###", "...##", "....#" });   // ◀
            d (0x25A0, { ".....", "#####", "#####", "#####", "#####", "#####", "....." });   // ■
            d (0x25CF, { ".....", ".###.", "#####", "#####", "#####", ".###.", "....." });   // ●
            d (0x2016, { ".....", "##.##", "##.##", "##.##", "##.##", "##.##", "....." });   // ‖ paus
            d (0x25B2, { ".....", "..#..", ".###.", "#####", ".....", ".....", "....." });   // ▲
            d (0x25BC, { ".....", "#####", ".###.", "..#..", ".....", ".....", "....." });   // ▼
            d (0x2192, { ".....", "..#..", "...#.", "#####", "...#.", "..#..", "....." });   // →
            return m;
        }();
        return t;
    }
}

// Alla tecken som typsnittet har, i tabellordning (för provarket).
inline juce::String allaTecken()
{
    juce::String s;
    for (auto& [c, m] : detalj::tabell()) { juce::ignoreUnused (m); s += juce::String::charToString (c); }
    return s;
}

inline const Matris* matris (juce::juce_wchar c)
{
    auto& t = detalj::tabell();
    auto it = t.find (c);
    if (it == t.end()) it = t.find ((juce::juce_wchar) juce::CharacterFunctions::toUpperCase (c));
    return it != t.end() ? &it->second : nullptr;
}

// Teckenbredden är 5 punkter plus en punkts mellanrum.
inline float bredd (const juce::String& text, float punkt) { return (float) text.length() * 6.0f * punkt - punkt; }

// Ritar texten med övre vänstra hörnet i (x, y). 'fyllnad' < 1 ger synliga punkter (matrisdisplay).
inline void ritaRaw (juce::Graphics& g, const juce::String& text, float x, float y, float punkt, float fyllnad = 1.0f)
{
    juce::Path rutor;     // en bana i stället för rektangellista: inga kantutjämnade skarvar mellan punkterna
    const float sida = punkt * fyllnad, kant = (punkt - sida) * 0.5f;
    float pen = x;
    for (auto p = text.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        if (const auto* m = matris (c))
        {
            for (int r = 0; r < rader; ++r)
            {
                // Sammanhängande punkter på en rad slås ihop, så att det inte blir glipor i 100 %.
                int k = 0;
                while (k < kolumner)
                {
                    if (((*m)[(size_t) r] >> (kolumner - 1 - k)) & 1u)
                    {
                        int slut = k;
                        while (slut + 1 < kolumner && (((*m)[(size_t) r] >> (kolumner - 2 - slut)) & 1u)) ++slut;
                        if (fyllnad >= 1.0f)
                            rutor.addRectangle (pen + (float) k * punkt, y + (float) r * punkt, (float) (slut - k + 1) * punkt, punkt);
                        else
                            for (int i = k; i <= slut; ++i)
                                rutor.addRectangle (pen + (float) i * punkt + kant, y + (float) r * punkt + kant, sida, sida);
                        k = slut + 1;
                    }
                    else ++k;
                }
            }
        }
        else
        {
            rutor.addRectangle (pen, y, punkt * 5.0f, punkt * 0.6f);       // okänt tecken: syns som ett streck
        }
        pen += punkt * 6.0f;
    }
    g.fillPath (rutor);
}

// OSD-text som en bandspelare lägger den: vitt med en mörk kant, så att den syns mot all bild.
inline void rita (juce::Graphics& g, const juce::String& text, float x, float y, float punkt,
                  juce::Colour farg = farg::skarmVit, float kantStyrka = 0.85f)
{
    if (kantStyrka > 0.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (kantStyrka));
        const float d = juce::jmax (1.0f, punkt * 0.5f);
        ritaRaw (g, text, x + d, y + d, punkt);
        ritaRaw (g, text, x - d * 0.5f, y, punkt);
        ritaRaw (g, text, x, y + d * 1.5f, punkt);
    }
    g.setColour (farg);
    ritaRaw (g, text, x, y, punkt);
}

} // namespace km::stil::osd
