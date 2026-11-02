// Mönstren – trummönstret (tolv spår) och tonmönstret (Kolvbasen), upp till 64 steg vardera.
// Gränssnittet skriver och ljudtråden läser, därför är varje steg ett atomiskt ord.
//
// Bygger på Stegverket och Tonstegverket i KrangBox, men formatet är eget (se toString) och har ett
// versionsnummer i preset-filen, så att senare ändringar kan läsa äldre filer.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_core/juce_core.h>
#include "../dsp/Stampverket.h"
#include <array>
#include <atomic>
#include <vector>

namespace km
{

inline constexpr int maxSteg = 64;
inline constexpr std::array<float, 4> chansVarden { 1.0f, 0.75f, 0.5f, 0.25f };

// Kortaste längden (16, 32 eller 64 steg) som mönstret upprepar sig med, så att texten blir kort.
template <typename Lika>
int period (Lika lika)
{
    for (int p : { 16, 32 })
    {
        bool upprepas = true;
        for (int st = p; st < maxSteg && upprepas; ++st) upprepas = lika (st, st % p);
        if (upprepas) return p;
    }
    return maxSteg;
}

// ---------------------------------------------------------------------------------------------
// Trummor

struct TrumSteg
{
    int slag = 0;      // 0 av, 1 slag, 2 accent
    int chans = 0;     // 0 = 100 %, 1 = 75 %, 2 = 50 %, 3 = 25 %
    int ratchet = 0;   // 0..3: 1–4 slag inom steget

    bool harTillagg() const { return chans != 0 || ratchet != 0; }
    bool operator== (const TrumSteg& o) const { return slag == o.slag && chans == o.chans && ratchet == o.ratchet; }
};

class TrumMonster
{
public:
    static constexpr int spar = antalSpar;

    TrumMonster() { rensa(); }

    TrumSteg get (int spr, int steg) const { return unpack (celler[index (spr, steg)].load (std::memory_order_relaxed)); }
    void set (int spr, int steg, const TrumSteg& s) { celler[index (spr, steg)].store (pack (s), std::memory_order_relaxed); }
    void rensa() { for (auto& c : celler) c.store (0, std::memory_order_relaxed); }

    // Text: ett spår per rad, raderna skilda med '/'. Varje steg är en siffra (0 av, 1 slag, 2 accent),
    // följd av {…} när steget har mer: c = chans, r = ratchet. Exempel: "1000{c2}2000/0000100000001000".
    // Ett spår som upprepar sig skrivs bara en gång (16 eller 32 steg); ett tomt spår blir en tom rad.
    juce::String toString() const
    {
        juce::StringArray rader;
        for (int s = 0; s < spar; ++s)
        {
            int sista = -1;
            for (int st = 0; st < maxSteg; ++st)
                if (get (s, st).slag != 0 || get (s, st).harTillagg()) sista = st;
            const int langd = sista < 0 ? 0 : period ([this, s] (int a, int b) { return get (s, a) == get (s, b); });
            juce::String rad;
            for (int st = 0; st < langd; ++st)
            {
                const auto d = get (s, st);
                rad << juce::String (d.slag);
                if (d.harTillagg())
                {
                    rad << "{";
                    if (d.chans != 0)   rad << "c" << d.chans;
                    if (d.ratchet != 0) rad << "r" << d.ratchet;
                    rad << "}";
                }
            }
            rader.add (rad);
        }
        return rader.joinIntoString ("/");
    }

    // Läser text som ovan. En rad som är kortare än 64 steg upprepas (en takt blir fyra).
    void fromString (const juce::String& text)
    {
        rensa();
        const auto rader = juce::StringArray::fromTokens (text, "/", "");
        for (int s = 0; s < juce::jmin (spar, rader.size()); ++s)
        {
            const auto steg = lasRad (rader[s]);
            if (steg.empty()) continue;
            for (int st = 0; st < maxSteg; ++st)
                set (s, st, steg[(size_t) st % steg.size()]);
        }
    }

    static std::vector<TrumSteg> lasRad (const juce::String& rad)
    {
        std::vector<TrumSteg> steg;
        for (int i = 0; i < rad.length() && (int) steg.size() < maxSteg;)
        {
            const auto c = rad[i++];
            if (c < '0' || c > '2') continue;
            TrumSteg d;
            d.slag = (int) (c - '0');
            if (i < rad.length() && rad[i] == '{')
            {
                const int slut = rad.indexOfChar (i, '}');
                const auto inre = rad.substring (i + 1, slut < 0 ? rad.length() : slut);
                i = slut < 0 ? rad.length() : slut + 1;
                for (int j = 0; j + 1 < inre.length(); j += 2)
                {
                    const int v = juce::jlimit (0, 3, (int) (inre[j + 1] - '0'));
                    if (inre[j] == 'c') d.chans = v;
                    else if (inre[j] == 'r') d.ratchet = v;
                }
            }
            steg.push_back (d);
        }
        return steg;
    }

private:
    static uint8_t pack (const TrumSteg& s)
    {
        return (uint8_t) (juce::jlimit (0, 2, s.slag) | (juce::jlimit (0, 3, s.chans) << 2) | (juce::jlimit (0, 3, s.ratchet) << 4));
    }
    static TrumSteg unpack (uint8_t v) { return { v & 3, (v >> 2) & 3, (v >> 4) & 3 }; }
    static size_t index (int spr, int steg)
    {
        return (size_t) (juce::jlimit (0, spar - 1, spr) * maxSteg + juce::jlimit (0, maxSteg - 1, steg));
    }

    std::array<std::atomic<uint8_t>, (size_t) (spar * maxSteg)> celler;
};

// ---------------------------------------------------------------------------------------------
// Toner

// Tonlängder i steg (index 0..7). Index 1 (ett halvt steg) är standard: den studsande sextondelsbasen.
inline constexpr std::array<double, 8> tonLangder { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 };
inline constexpr int standardLangd = 1;

struct TonSteg
{
    int slag = 0;       // 0 av, 1 ton, 2 accent
    int halvtoner = 0;  // −12..+12 från grundtonen
    int oktav = 0;      // −1, 0, +1
    int langd = standardLangd;
    bool glid = false;  // håll tonen och glid in i nästa

    int tonhojd() const { return halvtoner + 12 * oktav; }
    bool operator== (const TonSteg& o) const
    {
        return slag == o.slag && halvtoner == o.halvtoner && oktav == o.oktav && langd == o.langd && glid == o.glid;
    }
};

class TonMonster
{
public:
    TonMonster() { rensa(); }

    TonSteg get (int steg) const { return unpack (celler[index (steg)].load (std::memory_order_relaxed)); }
    void set (int steg, const TonSteg& s) { celler[index (steg)].store (pack (s), std::memory_order_relaxed); }
    void rensa() { for (auto& c : celler) c.store (pack ({}), std::memory_order_relaxed); }

    // Text: ett ord per steg, skilda med mellanslag. "." = paus, annars halvtoner med tillägg:
    // "a" accent, "~" glid, "u"/"d" oktav upp/ned, "/n" längdindex (utelämnas för standardlängden).
    // Exempel: "0a 0 0u . -2a~ 3/3".
    juce::String toString (int antal = -1) const
    {
        if (antal < 0) antal = period ([this] (int a, int b) { return get (a) == get (b); });
        juce::StringArray ord;
        for (int i = 0; i < antal; ++i)
        {
            const auto s = get (i);
            if (s == TonSteg {}) { ord.add ("."); continue; }
            juce::String w = s.slag == 0 ? "_" : "";
            w << s.halvtoner;
            if (s.slag == 2) w << "a";
            if (s.oktav > 0) w << "u";
            if (s.oktav < 0) w << "d";
            if (s.glid) w << "~";
            if (s.langd != standardLangd) w << "/" << s.langd;
            ord.add (w);
        }
        return ord.joinIntoString (" ");
    }

    // Läser text som ovan. Ett kortare mönster upprepas över alla 64 steg.
    void fromString (const juce::String& text)
    {
        rensa();
        juce::Array<TonSteg> steg;
        for (auto w : juce::StringArray::fromTokens (text, " ", ""))
        {
            w = w.trim();
            if (w.isEmpty()) continue;
            TonSteg s;
            if (w == ".") { steg.add (s); continue; }
            bool av = false;
            if (w.startsWithChar ('_')) { av = true; w = w.substring (1); }
            if (w.containsChar ('/'))
            {
                s.langd = juce::jlimit (0, 7, w.fromLastOccurrenceOf ("/", false, false).getIntValue());
                w = w.upToLastOccurrenceOf ("/", false, false);
            }
            s.glid = w.containsChar ('~');
            s.oktav = w.containsChar ('u') ? 1 : (w.containsChar ('d') ? -1 : 0);
            const bool accent = w.containsChar ('a');
            s.halvtoner = juce::jlimit (-12, 12, w.removeCharacters ("a~ud").getIntValue());
            s.slag = av ? 0 : (accent ? 2 : 1);
            steg.add (s);
        }
        if (steg.isEmpty()) return;
        for (int i = 0; i < maxSteg; ++i)
            set (i, steg[i % steg.size()]);
    }

private:
    static uint32_t pack (const TonSteg& s)
    {
        return (uint32_t) juce::jlimit (0, 2, s.slag)
             | ((uint32_t) (juce::jlimit (-12, 12, s.halvtoner) + 12) << 2)
             | ((uint32_t) (juce::jlimit (-1, 1, s.oktav) + 1) << 7)
             | ((uint32_t) juce::jlimit (0, 7, s.langd) << 9)
             | ((s.glid ? 1u : 0u) << 12);
    }
    static TonSteg unpack (uint32_t v)
    {
        TonSteg s;
        s.slag = (int) (v & 3u);
        s.halvtoner = (int) ((v >> 2) & 31u) - 12;
        s.oktav = (int) ((v >> 7) & 3u) - 1;
        s.langd = (int) ((v >> 9) & 7u);
        s.glid = ((v >> 12) & 1u) != 0;
        return s;
    }
    static size_t index (int steg) { return (size_t) juce::jlimit (0, maxSteg - 1, steg); }

    std::array<std::atomic<uint32_t>, (size_t) maxSteg> celler;
};

} // namespace km
