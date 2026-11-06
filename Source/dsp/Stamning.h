// Stämningen: mikrotonala stämningar för de melodiska kanalerna (Kolvbasen, Kolvbas 2 och Hugget).
//
// En stämning är en tabell från MIDI-tangent till tonhöjd, uttryckt som ett "liksvävande tonnummer"
// (69 = 440 Hz, 70 = en liksvävande halvton högre osv.), så att resten av motorn kan räkna som förut:
// glid, transponering och modulation läggs på i halvtoner ovanpå tabellens värde. Trummorna påverkas inte.
//
// Inbyggda stämningar har tolv tangenter per oktav (utom 24-TET och 19-TET) och utgår från en tonika:
// grundtonen eller en vald ton. Tonikan behåller sin liksvävande tonhöjd. I 24-TET och 19-TET är varje
// tangent ett steg i skalan, med tonikan på den tangent närmast under grundtonen.
//
// Egna stämningar läses in som Scala-filer: .scl (skalan) och valfritt .kbm (tangentkartan). Utan .kbm
// läggs skalan ut från tonikan på samma sätt som de inbyggda. Tangenter som .kbm markerar med x är tysta.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>

namespace km
{

enum class StamningTyp
{
    Liksvavande = 0, Ren, Pytagoreisk, Medeltonig, Werckmeister, Rast, Bayati, Slendro, Pelog, Tet24, Tet19, Egen
};
inline constexpr int antalStamningar = 12;

inline juce::StringArray stamningNamn()
{
    return { juce::String::fromUTF8 ("Liksvävande"), juce::String::fromUTF8 ("Ren stämning"), juce::String::fromUTF8 ("Pytagoreisk"),
             juce::String::fromUTF8 ("Medeltonig"), "Werckmeister III", "Rast (makam)", "Bayati (makam)", "Slendro", "Pelog",
             "24-TET", "19-TET", "Egen (Scala)" };
}

inline juce::StringArray stamningTonNamn()
{
    return { "Grundtonen", "C", juce::String::fromUTF8 ("C♯"), "D", juce::String::fromUTF8 ("D♯"), "E", "F",
             juce::String::fromUTF8 ("F♯"), "G", juce::String::fromUTF8 ("G♯"), "A", juce::String::fromUTF8 ("A♯"), "H" };
}

// En skala: 'antal' steg per period, cent[i] för steg i (cent[0] = 0, cent[antal] = perioden),
// och valfritt en tangentkarta som i Scalas .kbm.
struct Skala
{
    static constexpr int maxSteg = 128;
    int antal = 12;
    std::array<double, maxSteg + 1> cent {};

    bool harKarta = false;
    int kartStorlek = 0, forsta = 0, sista = 127, mitt = 60, referens = 69, oktavSteg = 0;
    double referensHz = 440.0;
    std::array<int, maxSteg> karta {};   // −1 = tyst (x)

    juce::String namn;

    double period() const { return cent[(size_t) antal]; }
    double centFor (int steg) const
    {
        const int q = (int) std::floor ((double) steg / (double) antal);
        return q * period() + cent[(size_t) (steg - q * antal)];
    }

    static Skala tolv (const std::array<double, 12>& c)
    {
        Skala s;
        s.antal = 12;
        for (int i = 0; i < 12; ++i) s.cent[(size_t) i] = c[(size_t) i];
        s.cent[12] = 1200.0;
        return s;
    }
    static Skala tet (int n)
    {
        Skala s;
        s.antal = n;
        for (int i = 0; i <= n; ++i) s.cent[(size_t) i] = 1200.0 * i / n;
        return s;
    }
    // Ett tolvtangentsläge där varje tangent går till närmaste ton i en gles skala (gamelan).
    static Skala vikt (std::initializer_list<double> toner)
    {
        std::array<double, 12> c {};
        for (int k = 0; k < 12; ++k)
        {
            double bast = 0.0, avst = 1.0e9;
            for (double t : toner) for (double o : { 0.0, 1200.0 })
                if (std::abs (t + o - 100.0 * k) < avst) { avst = std::abs (t + o - 100.0 * k); bast = t + o; }
            c[(size_t) k] = bast;
        }
        return tolv (c);
    }
    static double kvot (double a, double b) { return 1200.0 * std::log2 (a / b); }

    static Skala inbyggd (StamningTyp t)
    {
        switch (t)
        {
            case StamningTyp::Ren:
                return tolv ({ 0, kvot (16, 15), kvot (9, 8), kvot (6, 5), kvot (5, 4), kvot (4, 3), kvot (45, 32), kvot (3, 2),
                               kvot (8, 5), kvot (5, 3), kvot (9, 5), kvot (15, 8) });
            case StamningTyp::Pytagoreisk:
                return tolv ({ 0, kvot (256, 243), kvot (9, 8), kvot (32, 27), kvot (81, 64), kvot (4, 3), kvot (729, 512), kvot (3, 2),
                               kvot (128, 81), kvot (27, 16), kvot (16, 9), kvot (243, 128) });
            case StamningTyp::Medeltonig:   // kvartskommatons medeltonig
                return tolv ({ 0, 76.049, 193.157, 310.265, 386.314, 503.422, 579.471, 696.578, 772.627, 889.735, 1006.843, 1082.892 });
            case StamningTyp::Werckmeister:
                return tolv ({ 0, 90.225, 192.180, 294.135, 390.225, 498.045, 588.270, 696.090, 792.180, 888.270, 996.090, 1092.180 });
            case StamningTyp::Rast:         // ters och septima en kvartston låga
                return tolv ({ 0, 100, 200, 300, 350, 500, 600, 700, 800, 900, 1000, 1050 });
            case StamningTyp::Bayati:       // sekunden en kvartston låg
                return tolv ({ 0, 100, 150, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 });
            case StamningTyp::Slendro: return vikt ({ 0, 231, 474, 717, 955 });
            case StamningTyp::Pelog:   return vikt ({ 0, 120, 258, 539, 675, 785, 943 });
            case StamningTyp::Tet24:   return tet (24);
            case StamningTyp::Tet19:   return tet (19);
            case StamningTyp::Liksvavande:
            case StamningTyp::Egen:
            default: return tet (12);
        }
    }

    // --- Scala-filer
    static juce::StringArray rader (const juce::String& text)
    {
        juce::StringArray ut;
        for (auto r : juce::StringArray::fromLines (text))
            if (! r.trimStart().startsWithChar ('!')) ut.add (r.trim());
        return ut;
    }

    // Läser en .scl. Returnerar false (och lämnar skalan orörd) om filen inte går att läsa.
    bool lasScl (const juce::String& text, juce::String* fel = nullptr)
    {
        auto r = rader (text);
        if (r.size() < 2) { if (fel) *fel = juce::String::fromUTF8 ("för kort"); return false; }
        const auto beskrivning = r[0];
        const int n = r[1].getIntValue();
        if (n < 1 || n > maxSteg || r.size() < n + 2) { if (fel) *fel = juce::String::fromUTF8 ("fel antal toner"); return false; }
        Skala s;
        s.antal = n;
        s.cent[0] = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const auto t = r[i + 2].upToFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf ("\t", false, false);
            double c;
            if (t.containsChar ('.')) c = t.getDoubleValue();
            else
            {
                const double a = t.upToFirstOccurrenceOf ("/", false, false).getDoubleValue();
                const double b = t.containsChar ('/') ? t.fromFirstOccurrenceOf ("/", false, false).getDoubleValue() : 1.0;
                if (a <= 0.0 || b <= 0.0) { if (fel) *fel = juce::String::fromUTF8 ("ogiltig kvot på rad ") + juce::String (i + 1); return false; }
                c = kvot (a, b);
            }
            s.cent[(size_t) i + 1] = c;
        }
        if (s.period() <= 0.0) { if (fel) *fel = juce::String::fromUTF8 ("perioden måste vara större än noll"); return false; }
        s.harKarta = harKarta; s.kartStorlek = kartStorlek; s.forsta = forsta; s.sista = sista; s.mitt = mitt;
        s.referens = referens; s.referensHz = referensHz; s.oktavSteg = oktavSteg; s.karta = karta;
        s.namn = beskrivning;
        *this = s;
        return true;
    }

    bool lasKbm (const juce::String& text, juce::String* fel = nullptr)
    {
        auto r = rader (text);
        if (r.size() < 7) { if (fel) *fel = juce::String::fromUTF8 ("för kort"); return false; }
        const int storlek = r[0].getIntValue();
        if (storlek < 0 || storlek > maxSteg || r.size() < 7 + storlek) { if (fel) *fel = juce::String::fromUTF8 ("fel kartstorlek"); return false; }
        kartStorlek = storlek;
        forsta = juce::jlimit (0, 127, r[1].getIntValue());
        sista = juce::jlimit (0, 127, r[2].getIntValue());
        mitt = juce::jlimit (0, 127, r[3].getIntValue());
        referens = juce::jlimit (0, 127, r[4].getIntValue());
        referensHz = r[5].getDoubleValue() > 0.0 ? r[5].getDoubleValue() : 440.0;
        oktavSteg = r[6].getIntValue();
        for (int i = 0; i < storlek; ++i)
            karta[(size_t) i] = r[7 + i].startsWithIgnoreCase ("x") ? -1 : r[7 + i].getIntValue();
        harKarta = true;
        return true;
    }
    void taBortKarta() { harKarta = false; kartStorlek = 0; }

    // Skalsteget för en tangent enligt kartan (eller INT_MIN för en tyst tangent).
    int stegFor (int tangent) const
    {
        if (tangent < forsta || tangent > sista) return std::numeric_limits<int>::min();
        if (kartStorlek == 0) return tangent - mitt;
        const int d = tangent - mitt;
        const int q = (int) std::floor ((double) d / (double) kartStorlek);
        const int g = karta[(size_t) (d - q * kartStorlek)];
        if (g < 0) return std::numeric_limits<int>::min();
        return g + q * (oktavSteg > 0 ? oktavSteg : antal);
    }
};

using StamningTabell = std::array<double, 128>;
inline bool tyst (double tunadNot) { return std::isnan (tunadNot); }

// Räknar ut tabellen. 'ankare' är tonikans tangent (dess liksvävande tonhöjd behålls) när skalan saknar karta.
inline void raknaTabell (const Skala& s, int ankare, StamningTabell& ut)
{
    if (s.harKarta)
    {
        const int refSteg = s.stegFor (s.referens);
        const double refCent = refSteg == std::numeric_limits<int>::min() ? 0.0 : s.centFor (refSteg);
        const double refNot = 69.0 + 12.0 * std::log2 (s.referensHz / 440.0);
        for (int n = 0; n < 128; ++n)
        {
            const int g = s.stegFor (n);
            ut[(size_t) n] = g == std::numeric_limits<int>::min() ? std::numeric_limits<double>::quiet_NaN()
                                                                 : refNot + (s.centFor (g) - refCent) / 100.0;
        }
        return;
    }
    for (int n = 0; n < 128; ++n)
        ut[(size_t) n] = (double) ankare + s.centFor (n - ankare) / 100.0;
}

// Tonikans tangent: tangenten med tonikans tonklass på eller närmast under grundtonen.
// ton 0 = följ grundtonen, 1–12 = C … H.
inline int tonikaTangent (int grundton, int ton)
{
    if (ton <= 0) return grundton;
    const int tk = ton - 1;
    return grundton - (((grundton - tk) % 12) + 12) % 12;
}

// Den egna skalan, inläst på meddelandetråden och läst av ljudtråden. Två platser: skrivaren fyller
// den lediga och byter; versionen talar om för motorn att räkna om sin tabell.
class EgenStamning
{
public:
    void satt (const Skala& s)
    {
        const int ledig = 1 - aktiv.load (std::memory_order_acquire);
        platser[(size_t) ledig] = s;
        aktiv.store (ledig, std::memory_order_release);
        version.fetch_add (1, std::memory_order_acq_rel);
    }
    const Skala& hamta() const { return platser[(size_t) aktiv.load (std::memory_order_acquire)]; }
    uint32_t getVersion() const { return version.load (std::memory_order_acquire); }

private:
    std::array<Skala, 2> platser { Skala::tet (12), Skala::tet (12) };
    std::atomic<int> aktiv { 0 };
    std::atomic<uint32_t> version { 1 };
};

} // namespace km
