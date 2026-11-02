// Taktverket – klockan som driver alla sekvenser.
// Räknar ut var i blocket varje sextondel börjar, med gung (swing). Följer DAW:ens transport när den spelar;
// annars går en egen klocka (i den fristående appen och i KMRender).
//
// Varje kanal har sin egen längd (polymeter): steget för en kanal är k mod kanalens längd, där k är
// sextondelens nummer sedan start.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>

namespace km
{

struct Transport
{
    bool hostSpelar = false;
    double ppq = 0.0;        // position i fjärdedelar
    double bpm = 120.0;
};

struct TaktInstallningar
{
    bool spela = false;      // egen klocka: spela/stopp
    double tempo = 125.0;    // BPM för den egna klockan
    float gung = 0.0f;       // 0..0,5: hur mycket varannan sextondel släpar
};

class Taktverket
{
public:
    static constexpr double stegLangd = 0.25;  // en sextondel i fjärdedelar
    static constexpr int maxHandelser = 128;

    struct Handelse
    {
        juce::int64 k;       // sextondelens nummer sedan start
        int offset;          // sample i blocket
    };

    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { egenPpq = 0.0; gick = false; senastSlut = -1.0; }

    // Räknar ut stegen i blocket. Returnerar antal händelser (i tidsordning i 'ut').
    // 'hopp' blir sant om transporten startade, stannade eller hoppade, så att kanalerna kan släppa sina toner.
    int process (const TaktInstallningar& s, const Transport& t, bool egenKlocka, int numSamples)
    {
        antal = 0;
        hopp = false;
        const bool host = t.hostSpelar;
        const bool egen = ! host && egenKlocka && s.spela;
        const bool gar = host || egen;

        if (! gar)
        {
            if (gick) hopp = true;
            gick = false;
            senastSlut = -1.0;
            return 0;
        }

        if (egen && ! gick) egenPpq = -1.0e-9;   // starta exakt på steg 0
        if (! gick) hopp = true;
        gick = true;

        aktuelltBpm = juce::jlimit (20.0, 400.0, host ? t.bpm : s.tempo);
        samplesPerPpq = sr * 60.0 / aktuelltBpm;
        const double start = host ? t.ppq : egenPpq;
        const double slut = start + numSamples / samplesPerPpq;
        blockStart = start;

        if (senastSlut >= 0.0 && std::abs (start - senastSlut) > 0.01) hopp = true;   // DAW:en hoppade eller loopade
        senastSlut = slut;

        const double gungOffset = juce::jlimit (0.0f, 0.5f, s.gung) * stegLangd;
        const auto forsta = (juce::int64) std::floor (start / stegLangd) - 1;
        const auto sista  = (juce::int64) std::floor (slut / stegLangd);
        for (auto k = juce::jmax ((juce::int64) 0, forsta); k <= sista && antal < maxHandelser; ++k)
        {
            const double vid = (double) k * stegLangd + ((k & 1) ? gungOffset : 0.0);
            if (vid < start || vid >= slut) continue;
            ut[(size_t) antal++] = { k, juce::jlimit (0, numSamples - 1, (int) ((vid - start) * samplesPerPpq)) };
        }

        if (egen) egenPpq = slut;
        return antal;
    }

    const Handelse& handelse (int i) const { return ut[(size_t) i]; }

    // Positionen i fjärdedelar vid ett sample i det senaste blocket, eller −1 om sekvensern står still.
    double ppqVid (int offset) const { return gick ? blockStart + offset / samplesPerPpq : -1.0; }
    bool hoppade() const { return hopp; }
    bool gar() const { return gick; }
    double samplesPerSteg() const { return samplesPerPpq * stegLangd; }
    double bpm() const { return aktuelltBpm; }

    // Ett fast slumptal 0..1 för steg k i ett spår: samma varje gång samma låt spelas (för chans per steg).
    static float chansSlag (juce::int64 k, int spar)
    {
        auto h = (uint32_t) ((uint64_t) k * 2654435761u) ^ ((uint32_t) spar * 40503u + 0x9e3779b9u);
        h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
        return (float) (h & 0xffffu) / 65536.0f;
    }

private:
    double sr = 48000.0, egenPpq = 0.0, senastSlut = -1.0, blockStart = 0.0;
    double samplesPerPpq = 48000.0 * 0.5, aktuelltBpm = 125.0;
    bool gick = false, hopp = false;
    std::array<Handelse, maxHandelser> ut {};
    int antal = 0;
};

} // namespace km
