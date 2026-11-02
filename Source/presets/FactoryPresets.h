// Fabriksbandet – inbyggda presets med ljud och mönster.
// Värden anges i parameterns egen enhet (Hz, s, steg, valets index …). Parametrar som inte nämns får sitt
// standardvärde. Mönster anges som text (se Monster.h); en rad på 16 steg upprepas till 64.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "../Parameters.h"
#include <utility>
#include <vector>

namespace km::presets
{

struct TrumText
{
    int monster = 0, variant = 0;                            // 0-baserat mönster, 0 = A, 1 = B
    std::vector<std::pair<Trumma, const char*>> rader;
};

struct TonText
{
    Kanal kanal = Kanal::Kolvbasen;
    int monster = 0, variant = 0;
    const char* text = nullptr;
};

struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<juce::String, float>> values;
    std::vector<TrumText> trummor;
    std::vector<TonText> toner;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    using namespace params;
    using T = Trumma;
    const auto kb = [] (const char* vad) { return id::kolv (0, vad); };
    const auto kb2 = [] (const char* vad) { return id::kolv (1, vad); };
    const auto drivet = [] (Kanal k) { return id::kanal ((int) k, "drivetLage"); };
    const auto mangd = [] (Kanal k) { return id::kanal ((int) k, "drivetMangd"); };
    const auto variant = [] (Kanal k) { return id::kanal ((int) k, "variant"); };
    constexpr float ror = 1, overdrive = 2, bitkross = 3, ab = 2;
    const auto hu = [] (const char* vad) { return id::hu (vad); };
    const auto fx = [] (const char* e, const char* vad) { return id::fx (e, vad); };
    const auto k = [] (Kanal kanal, const char* vad) { return id::kanal ((int) kanal, vad); };
    enum { orkester = 0, blas, kor, klocka, synklead, pulslead, fmlead };
    enum { ingen = 0, moll, dur, kvint, oktav, moll7, sus4, moll9 };

    static const std::vector<FactoryPreset> list {
        { "Init", "Grund",
          {},
          { { 0, 0, { { T::Bastrumma, "1000100010001000" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0 . . . 0 . . . 0 . . . 0 . . ." } } },

        // Grind 0: en sextondelsbas med oktavhopp och bastrumma på 125 BPM.
        { "Kolv 125", "EBM",
          { { id::tempo, 125.0f },
            { kb ("vag"), 0.15f }, { kb ("sub"), 0.55f }, { kb ("cutoff"), 170.0f }, { kb ("resonans"), 0.45f },
            { kb ("envMangd"), 0.62f }, { kb ("filterFall"), 0.11f }, { kb ("accent"), 0.75f },
            { kb ("fall"), 0.18f }, { kb ("hall"), 0.45f }, { kb ("drive"), 0.45f },
            { id::trumma ((int) T::Bastrumma, "klang"), 0.4f } },
          { { 0, 0, { { T::Bastrumma, "2000100010001000" }, { T::Virvel, "0000100000001000" }, { T::StangdHihat, "0010002000100020" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a 0 0u 0 0a 0 0u 0 0a 0 0u 0 -2a -2 0u~ 3" } } },

        // Grind 1a: två basar i lager – en mörk botten och samma gång en oktav upp, ljus och överstyrd.
        { "EBM 128", "EBM",
          { { id::tempo, 128.0f }, { id::master, 0.5f }, { id::kit, (float) Kit::Niononio },
            { kb ("vag"), 0.0f }, { kb ("sub"), 0.6f }, { kb ("cutoff"), 150.0f }, { kb ("resonans"), 0.4f },
            { kb ("envMangd"), 0.6f }, { kb ("filterFall"), 0.1f }, { kb ("accent"), 0.7f }, { kb ("fall"), 0.15f },
            { kb ("hall"), 0.4f }, { kb ("drive"), 0.5f },
            { kb2 ("vag"), 0.6f }, { kb2 ("pulsbredd"), 30.0f }, { kb2 ("osc2Niva"), 0.5f }, { kb2 ("osc2Fin"), 9.0f },
            { kb2 ("filterTyp"), 1.0f }, { kb2 ("cutoff"), 500.0f }, { kb2 ("resonans"), 0.45f }, { kb2 ("envMangd"), 0.55f },
            { kb2 ("filterFall"), 0.09f }, { kb2 ("fall"), 0.12f }, { kb2 ("hall"), 0.3f }, { kb2 ("drive"), 0.3f },
            { id::folj, 1.0f }, { id::transponering, 12.0f },
            { drivet (Kanal::Kolvbas2), overdrive }, { mangd (Kanal::Kolvbas2), 0.45f },
            { id::kanal (1, "niva"), 0.55f },
            { drivet (Kanal::Stampverket), ror }, { mangd (Kanal::Stampverket), 0.25f },
            { variant (Kanal::Stampverket), ab }, { variant (Kanal::Kolvbasen), ab } },
          { { 0, 0, { { T::Bastrumma,   "2000100010001000" }, { T::Virvel, "0000100000001000" }, { T::Handklapp, "0000100000001000" },
                      { T::StangdHihat, "1121112111211121" }, { T::OppenHihat, "0000000000000010" } } },
            { 0, 1, { { T::Bastrumma,   "2000100010001010" }, { T::Virvel, "000010000000102{r1}2" }, { T::Handklapp, "0000100000001000" },
                      { T::StangdHihat, "1121112111211121" }, { T::Cymbal, "0000000000000001" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a . 0 0 0a . 0 0 0a . 0 0 -2a . 0 3" },
            { Kanal::Kolvbasen, 0, 1, "0a . 0 0 0a . 0 0 5a . 5 5 3a . 3 -2~" } } },

        // Grind 1b: marschen – rakare, pulsvåg, fabrikskit med virvelvirvlar, städ och en motstämma i Kolvbas 2.
        { "Marsch 120", "EBM",
          { { id::tempo, 120.0f }, { id::master, 0.5f }, { id::kit, (float) Kit::Fabrik },
            { kb ("vag"), 1.0f }, { kb ("pulsbredd"), 28.0f }, { kb ("sub"), 0.4f }, { kb ("cutoff"), 260.0f },
            { kb ("resonans"), 0.3f }, { kb ("envMangd"), 0.5f }, { kb ("filterFall"), 0.16f }, { kb ("accent"), 0.6f },
            { kb ("fall"), 0.25f }, { kb ("hall"), 0.55f }, { kb ("drive"), 0.3f },
            { kb2 ("vag"), 0.0f }, { kb2 ("filterTyp"), 0.0f }, { kb2 ("cutoff"), 900.0f }, { kb2 ("resonans"), 0.25f },
            { kb2 ("envMangd"), 0.3f }, { kb2 ("fall"), 0.4f }, { kb2 ("hall"), 0.5f }, { kb2 ("slapp"), 0.08f }, { kb2 ("glid"), 0.08f },
            { kb2 ("osc2Niva"), 0.7f }, { kb2 ("osc2Halvtoner"), 7.0f }, { kb2 ("osc2Fin"), 0.0f },
            { id::kanal (1, "niva"), 0.45f },
            { drivet (Kanal::Kolvbasen), ror }, { mangd (Kanal::Kolvbasen), 0.35f },
            { id::trumma ((int) T::Virvel, "klang"), 0.55f }, { variant (Kanal::Stampverket), ab } },
          { { 0, 0, { { T::Bastrumma, "2000000020100000" }, { T::Virvel, "00002001{r2}00002011" },
                      { T::StangdHihat, "1010101010101010" }, { T::Metall, "0000000000100000" } } },
            { 0, 1, { { T::Bastrumma, "2000000020100010" }, { T::Virvel, "00002001{r2}00002{r1}01{r2}1{r3}" },
                      { T::StangdHihat, "1010101010101010" }, { T::Metall, "0000000000100000" }, { T::Small, "0000000000000002" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a . 0 0 0a . 0 0 3a . 3 3 -2a . -2 0" },
            { Kanal::Kolvbas2, 0, 0, ". . 0u . . . 0u . . . 3u . . . -2u~ 0u" } } },

        // Grind 1c: New Beat – långsamt och tungt, med gung, glid, digitalt kit och en syrlig andrabas.
        { "New Beat 112", "New Beat",
          { { id::tempo, 112.0f }, { id::master, 0.5f }, { id::gung, 8.0f }, { id::kit, (float) Kit::Digital },
            { kb ("vag"), 0.0f }, { kb ("sub"), 0.8f }, { kb ("cutoff"), 120.0f }, { kb ("resonans"), 0.55f },
            { kb ("envMangd"), 0.55f }, { kb ("filterFall"), 0.25f }, { kb ("accent"), 0.5f },
            { kb ("fall"), 0.6f }, { kb ("hall"), 0.7f }, { kb ("glid"), 0.07f }, { kb ("drive"), 0.6f },
            { kb2 ("vag"), 0.0f }, { kb2 ("filterTyp"), 1.0f }, { kb2 ("cutoff"), 350.0f }, { kb2 ("resonans"), 0.75f },
            { kb2 ("envMangd"), 0.7f }, { kb2 ("filterFall"), 0.2f }, { kb2 ("accent"), 0.8f }, { kb2 ("fall"), 0.3f },
            { kb2 ("hall"), 0.4f }, { kb2 ("glid"), 0.06f },
            { id::folj, 1.0f }, { id::transponering, 12.0f }, { id::kanal (1, "niva"), 0.4f },
            { drivet (Kanal::Kolvbasen), overdrive }, { mangd (Kanal::Kolvbasen), 0.3f },
            { drivet (Kanal::Kolvbas2), bitkross }, { mangd (Kanal::Kolvbas2), 0.35f },
            { id::trumma ((int) T::Bastrumma, "klang"), 0.65f }, { id::trumma ((int) T::Bastrumma, "stamning"), -2.0f } },
          { { 0, 0, { { T::Bastrumma, "1000100010001000" }, { T::Handklapp, "0000100000001000" },
                      { T::StangdHihat, "1000100010001000" }, { T::OppenHihat, "0010001000100010" },
                      { T::Kubjallra, "0000001000000001" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a/3 . . 0 . . 0u~ 0 . . 0 . -5a/3 . -5 -3~" } } },
        // Fas 2: hela maskinen – två basar, gatad virvel, orkesterstötar med eko och en kompressor som pumpar.
        { "Fabriksgolv 125", "EBM",
          { { id::tempo, 125.0f }, { id::master, 0.6f }, { id::kit, (float) Kit::Niononio },
            { kb ("vag"), 0.0f }, { kb ("sub"), 0.6f }, { kb ("cutoff"), 150.0f }, { kb ("resonans"), 0.4f },
            { kb ("envMangd"), 0.6f }, { kb ("filterFall"), 0.1f }, { kb ("accent"), 0.7f }, { kb ("fall"), 0.15f },
            { kb ("hall"), 0.4f }, { kb ("drive"), 0.5f },
            { kb2 ("vag"), 0.6f }, { kb2 ("pulsbredd"), 30.0f }, { kb2 ("filterTyp"), 1.0f }, { kb2 ("cutoff"), 450.0f },
            { kb2 ("resonans"), 0.5f }, { kb2 ("envMangd"), 0.55f }, { kb2 ("filterFall"), 0.09f }, { kb2 ("fall"), 0.12f }, { kb2 ("hall"), 0.3f },
            { id::folj, 1.0f }, { id::transponering, 12.0f }, { k (Kanal::Kolvbas2, "niva"), 0.5f },
            { drivet (Kanal::Kolvbas2), overdrive }, { mangd (Kanal::Kolvbas2), 0.45f },
            { k (Kanal::Kolvbas2, "pan"), 0.15f },
            { k (Kanal::Stampverket, "sandA"), 0.55f }, { variant (Kanal::Stampverket), ab },
            { hu ("lage"), (float) orkester }, { hu ("ackord"), (float) moll }, { hu ("klang"), 0.55f }, { hu ("fall"), 0.4f },
            { hu ("sankning"), 0.5f }, { k (Kanal::Hugget, "niva"), 0.6f }, { k (Kanal::Hugget, "sandB"), 0.4f },
            { k (Kanal::Hugget, "sandA"), 0.2f }, { k (Kanal::Hugget, "pan"), -0.1f },
            { fx ("gr", "storlek"), 0.65f }, { fx ("gr", "hallTid"), 0.18f }, { fx ("gr", "niva"), 0.8f },
            { fx ("eko", "tid"), 2.0f }, { fx ("eko", "aterkoppling"), 0.35f }, { fx ("eko", "niva"), 0.5f },
            { fx ("klam", "troskel"), -12.0f }, { fx ("klam", "kvot"), 3.0f }, { fx ("klam", "pump"), 0.3f }, { fx ("klam", "efter"), 5.0f },
            { id::lfo (0, "form"), 1.0f }, { id::lfo (0, "synk"), 3.0f },
            { id::mod (0, "kalla"), 1.0f }, { id::mod (0, "mal"), 2.0f }, { id::mod (0, "mangd"), 0.35f } },
          { { 0, 0, { { T::Bastrumma, "2000100010001000" }, { T::Virvel, "0000100000001000" },
                      { T::StangdHihat, "1121112111211121" }, { T::OppenHihat, "0000000000000010" } } },
            { 0, 1, { { T::Bastrumma, "2000100010001010" }, { T::Virvel, "000010000000102{r1}2" },
                      { T::StangdHihat, "1121112111211121" }, { T::Metall, "0000000000000001" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a . 0 0 0a . 0 0 0a . 0 0 -2a . 0 3" },
            { Kanal::Hugget, 0, 0, "0a/1 . . . . . . . . . . . . . 0/0 3a/1" } } },

        // Fas 2: en synkad lead med ping-pong-eko, en LFO på filtret och Spänningen en bit upp.
        { "Paroll 132", "EBM",
          { { id::tempo, 132.0f }, { id::master, 0.55f }, { id::kit, (float) Kit::Digital }, { id::spanning, 0.35f },
            { kb ("vag"), 1.0f }, { kb ("pulsbredd"), 25.0f }, { kb ("sub"), 0.5f }, { kb ("cutoff"), 220.0f },
            { kb ("resonans"), 0.35f }, { kb ("envMangd"), 0.5f }, { kb ("filterFall"), 0.12f }, { kb ("fall"), 0.2f },
            { kb ("hall"), 0.5f }, { kb ("drive"), 0.4f },
            { hu ("lage"), (float) synklead }, { hu ("ackord"), (float) ingen }, { hu ("klang"), 0.5f }, { hu ("attack"), 0.003f },
            { hu ("fall"), 0.5f }, { hu ("hall"), 0.6f }, { hu ("slapp"), 0.08f }, { hu ("bredd"), 0.3f },
            { k (Kanal::Hugget, "niva"), 0.45f }, { k (Kanal::Hugget, "sandB"), 0.45f },
            { k (Kanal::Stampverket, "sandA"), 0.3f },
            { fx ("eko", "tid"), 1.0f }, { fx ("eko", "pingpong"), 1.0f }, { fx ("eko", "aterkoppling"), 0.45f }, { fx ("eko", "niva"), 0.55f },
            { fx ("klam", "troskel"), -10.0f }, { fx ("klam", "kvot"), 4.0f }, { fx ("klam", "efter"), 4.0f },
            { id::lfo (0, "form"), 0.0f }, { id::lfo (0, "synk"), 2.0f },
            { id::mod (0, "kalla"), 1.0f }, { id::mod (0, "mal"), 1.0f }, { id::mod (0, "mangd"), 0.3f },
            { id::mod (1, "kalla"), 3.0f }, { id::mod (1, "mal"), 3.0f }, { id::mod (1, "mangd"), 0.4f } },
          { { 0, 0, { { T::Bastrumma, "2000100010001000" }, { T::Handklapp, "0000100000001000" },
                      { T::StangdHihat, "1011101110111011" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a 0 0u 0 0a 0 0u 0 -2a -2 -2u -2 -4a -4 -4u -4" },
            { Kanal::Hugget, 0, 0, "0a . 0 . 3 . 5 . 7a~ . 5 . 3 . 0 ." } } },

        // Fas 2: en 45-varvare spelad på 33 med pitchen uppe – 145 BPM blir 116, och allt sjunker och mörknar.
        { "Svart skiva 33", "New Beat",
          { { id::tempo, 145.0f }, { id::master, 0.8f }, { id::varvtal, 2.0f }, { id::varvLage, 1.0f }, { id::gung, 6.0f },
            { kb ("vag"), 0.2f }, { kb ("sub"), 0.7f }, { kb ("cutoff"), 260.0f }, { kb ("resonans"), 0.6f },
            { kb ("envMangd"), 0.65f }, { kb ("filterFall"), 0.15f }, { kb ("fall"), 0.4f }, { kb ("hall"), 0.6f },
            { kb ("glid"), 0.06f }, { kb ("drive"), 0.5f },
            { hu ("lage"), (float) kor }, { hu ("ackord"), (float) moll7 }, { hu ("attack"), 0.02f }, { hu ("fall"), 0.6f },
            { hu ("hall"), 0.3f }, { hu ("slapp"), 0.4f }, { hu ("bredd"), 0.8f },
            { k (Kanal::Hugget, "niva"), 0.5f }, { k (Kanal::Hugget, "sandA"), 0.3f }, { k (Kanal::Hugget, "sandB"), 0.3f },
            { k (Kanal::Stampverket, "sandA"), 0.35f },
            { fx ("eko", "lage"), 1.0f }, { fx ("eko", "tid"), 3.0f }, { fx ("eko", "aterkoppling"), 0.3f },
            { fx ("gr", "hallTid"), 0.3f },
            { fx ("svaj", "lage"), 1.0f }, { fx ("svaj", "mangd"), 0.35f },
            { fx ("klam", "troskel"), -14.0f }, { fx ("klam", "kvot"), 2.5f }, { fx ("klam", "pump"), 0.45f }, { fx ("klam", "efter"), 5.0f },
            { drivet (Kanal::Kolvbasen), overdrive }, { mangd (Kanal::Kolvbasen), 0.35f } },
          { { 0, 0, { { T::Bastrumma, "1000100010001000" }, { T::Handklapp, "0000100000001000" },
                      { T::StangdHihat, "1000100010001000" }, { T::OppenHihat, "0010001000100010" },
                      { T::Kubjallra, "0000001000000001" } } } },
          { { Kanal::Kolvbasen, 0, 0, "0a/3 . . 0 . . 0u~ 0 . . 0 . -5a/3 . -5 -3~" },
            { Kanal::Hugget, 0, 0, ". . 0a/3 . . . . . . . -2/3 . . . . ." } } },

    };
    return list;
}

} // namespace km::presets
