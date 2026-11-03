// Låten: låtläget. En lista med upp till 64 rader; varje rad anger mönster och variant för alla fyra
// kanalerna, vilka kanaler som är tysta, hur många takter raden spelas, och valfritt Spänningen och
// en transponering. Med låtläget på stegar Taktverket rad för rad (som 909:ans låtläge).
//
// Platsen räknas ur taktnumret sedan start, så att låten följer DAW:ens position exakt (en takt =
// 16 sextondelar). Efter sista raden börjar låten om (Loopa) eller blir tyst (Stanna).
//
// Gränssnittet skriver och ljudtråden läser, därför är varje rad ett atomiskt 64-bitarsord.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_core/juce_core.h>
#include "Monster.h"
#include <array>
#include <atomic>

namespace km
{

struct LatRad
{
    std::array<int, antalKanaler> monster {};    // 0..15
    std::array<int, antalKanaler> variant {};    // 0 A, 1 B, 2 A+B
    std::array<bool, antalKanaler> tyst {};
    int takter = 4;                              // 1..64
    int spanning = -1;                           // −1 = rör inte Spänningen, annars 0..100 %
    int transponering = 0;                       // −24..+24 halvtoner (tonkanalerna)

    bool operator== (const LatRad& o) const
    {
        return monster == o.monster && variant == o.variant && tyst == o.tyst && takter == o.takter
            && spanning == o.spanning && transponering == o.transponering;
    }
};

class Laten
{
public:
    static constexpr int maxRader = 64;

    Laten() { rensa(); }

    LatRad get (int i) const { return unpack (rader[(size_t) juce::jlimit (0, maxRader - 1, i)].load (std::memory_order_relaxed)); }
    void set (int i, const LatRad& r) { rader[(size_t) juce::jlimit (0, maxRader - 1, i)].store (pack (r), std::memory_order_relaxed); }
    int antal() const { return antalRader.load (std::memory_order_relaxed); }
    void sattAntal (int n) { antalRader.store (juce::jlimit (0, maxRader, n), std::memory_order_relaxed); }
    bool arTom() const { return antal() == 0; }
    void rensa() { for (auto& r : rader) r.store (pack ({}), std::memory_order_relaxed); antalRader.store (0, std::memory_order_relaxed); }

    // Lägger in en rad på plats i (flyttar resten nedåt) och tar bort rad i.
    void infoga (int i, const LatRad& r)
    {
        const int n = antal();
        if (n >= maxRader) return;
        i = juce::jlimit (0, n, i);
        for (int j = n; j > i; --j) set (j, get (j - 1));
        set (i, r);
        sattAntal (n + 1);
    }
    void taBort (int i)
    {
        const int n = antal();
        if (i < 0 || i >= n) return;
        for (int j = i; j < n - 1; ++j) set (j, get (j + 1));
        set (n - 1, {});
        sattAntal (n - 1);
    }

    int totalaTakter() const
    {
        int t = 0;
        for (int i = 0; i < antal(); ++i) t += get (i).takter;
        return t;
    }

    // Raden och takten inom raden för takt nummer 'takt' (0 = första). rad = −1 efter slutet (Stanna)
    // eller när låten är tom.
    struct Plats { int rad = -1, taktIRad = 0; };
    Plats plats (juce::int64 takt, bool loopa) const
    {
        const int total = totalaTakter();
        if (total <= 0 || takt < 0) return {};
        if (takt >= total)
        {
            if (! loopa) return {};
            takt %= total;
        }
        int t = (int) takt;
        for (int i = 0; i < antal(); ++i)
        {
            const int n = get (i).takter;
            if (t < n) return { i, t };
            t -= n;
        }
        return {};
    }

    // Text: en rad per låtrad, skilda med ';'. Varje rad: takter och sedan ord för kanalerna,
    // t.ex. "4 kb=1A kb2=2B sv=3AB hu=1A tyst=kb2,hu sp=50 tr=+5". Utelämnade kanaler spelar mönster 1A.
    juce::String toString() const
    {
        static const char* namn[antalKanaler] { "kb", "kb2", "sv", "hu" };
        juce::StringArray ut;
        for (int i = 0; i < antal(); ++i)
        {
            const auto r = get (i);
            juce::String s (r.takter);
            juce::StringArray tysta;
            for (int k = 0; k < antalKanaler; ++k)
            {
                s << " " << namn[k] << "=" << (r.monster[(size_t) k] + 1) << (r.variant[(size_t) k] == 0 ? "A" : (r.variant[(size_t) k] == 1 ? "B" : "AB"));
                if (r.tyst[(size_t) k]) tysta.add (namn[k]);
            }
            if (! tysta.isEmpty()) s << " tyst=" << tysta.joinIntoString (",");
            if (r.spanning >= 0) s << " sp=" << r.spanning;
            if (r.transponering != 0) s << " tr=" << (r.transponering > 0 ? "+" : "") << r.transponering;
            ut.add (s);
        }
        return ut.joinIntoString ("; ");
    }

    void fromString (const juce::String& text)
    {
        rensa();
        const auto rad = juce::StringArray::fromTokens (text, ";", "");
        int n = 0;
        for (const auto& r : rad)
        {
            const auto ord = juce::StringArray::fromTokens (r.trim(), " ", "");
            if (ord.isEmpty() || ord[0].isEmpty() || n >= maxRader) continue;
            LatRad lr;
            lr.takter = juce::jlimit (1, 64, ord[0].getIntValue());
            for (int i = 1; i < ord.size(); ++i)
            {
                const auto nyckel = ord[i].upToFirstOccurrenceOf ("=", false, false);
                const auto varde = ord[i].fromFirstOccurrenceOf ("=", false, false);
                const int k = kanalIndex (nyckel);
                if (k >= 0)
                {
                    lr.monster[(size_t) k] = juce::jlimit (1, antalMonster, varde.getIntValue()) - 1;
                    const auto v = varde.retainCharacters ("ABab").toUpperCase();
                    lr.variant[(size_t) k] = v == "B" ? 1 : (v == "AB" ? 2 : 0);
                }
                else if (nyckel == "tyst")
                {
                    for (const auto& t : juce::StringArray::fromTokens (varde, ",", ""))
                        if (const int kt = kanalIndex (t); kt >= 0) lr.tyst[(size_t) kt] = true;
                }
                else if (nyckel == "sp") lr.spanning = juce::jlimit (0, 100, varde.getIntValue());
                else if (nyckel == "tr") lr.transponering = juce::jlimit (-24, 24, varde.getIntValue());
            }
            set (n++, lr);
        }
        sattAntal (n);
    }

private:
    static int kanalIndex (const juce::String& s)
    {
        if (s == "kb") return 0;
        if (s == "kb2") return 1;
        if (s == "sv") return 2;
        if (s == "hu") return 3;
        return -1;
    }

    // Bitar: mönster 4×4, variant 4×2, tyst 4×1, takter−1 6, Spänningen 7 (127 = rör inte), transponering+24 6.
    static uint64_t pack (const LatRad& r)
    {
        uint64_t v = 0;
        for (int k = 0; k < antalKanaler; ++k)
        {
            v |= (uint64_t) (juce::jlimit (0, 15, r.monster[(size_t) k])) << (4 * k);
            v |= (uint64_t) (juce::jlimit (0, 2, r.variant[(size_t) k])) << (16 + 2 * k);
            v |= (uint64_t) (r.tyst[(size_t) k] ? 1 : 0) << (24 + k);
        }
        v |= (uint64_t) (juce::jlimit (1, 64, r.takter) - 1) << 28;
        v |= (uint64_t) (r.spanning < 0 ? 127 : juce::jlimit (0, 100, r.spanning)) << 34;
        v |= (uint64_t) (juce::jlimit (-24, 24, r.transponering) + 24) << 41;
        return v;
    }
    static LatRad unpack (uint64_t v)
    {
        LatRad r;
        for (int k = 0; k < antalKanaler; ++k)
        {
            r.monster[(size_t) k] = (int) ((v >> (4 * k)) & 15);
            r.variant[(size_t) k] = (int) ((v >> (16 + 2 * k)) & 3);
            r.tyst[(size_t) k] = ((v >> (24 + k)) & 1) != 0;
        }
        r.takter = (int) ((v >> 28) & 63) + 1;
        const int sp = (int) ((v >> 34) & 127);
        r.spanning = sp == 127 ? -1 : sp;
        r.transponering = (int) ((v >> 41) & 63) - 24;
        return r;
    }

    std::array<std::atomic<uint64_t>, maxRader> rader;
    std::atomic<int> antalRader { 0 };
};

} // namespace km
