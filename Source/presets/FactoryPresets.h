// Fabriksbandet – inbyggda presets med ljud och mönster.
// Värden anges i parameterns egen enhet (Hz, s, steg …). Parametrar som inte nämns får sitt standardvärde.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "../Parameters.h"
#include <utility>
#include <vector>

namespace km::presets
{

struct FactoryPreset
{
    const char* name;
    const char* category;
    std::vector<std::pair<juce::String, float>> values;
    std::vector<std::pair<Trumma, const char*>> trummor;   // spår och rad (16 steg upprepas)
    const char* kolvbasen = nullptr;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    using namespace params;
    static const std::vector<FactoryPreset> list {
        { "Init", "Grund",
          {},
          { { Trumma::Bastrumma, "1000100010001000" } },
          "0 . . . 0 . . . 0 . . . 0 . . ." },

        // Grind 0: en sextondelsbas med oktavhopp och bastrumma på 125 BPM.
        { "Kolv 125", "EBM",
          { { id::tempo, 125.0f },
            { id::kbVag, 0.15f }, { id::kbSub, 0.55f }, { id::kbCutoff, 170.0f }, { id::kbResonans, 0.45f },
            { id::kbEnvMangd, 0.62f }, { id::kbFilterFall, 0.11f }, { id::kbAccent, 0.75f },
            { id::kbFall, 0.18f }, { id::kbHall, 0.45f }, { id::kbDrive, 0.45f },
            { id::trumma (0, "klang"), 0.4f } },
          { { Trumma::Bastrumma,   "2000100010001000" },
            { Trumma::Virvel,      "0000100000001000" },
            { Trumma::StangdHihat, "0010002000100020" } },
          "0a 0 0u 0 0a 0 0u 0 0a 0 0u 0 -2a -2 0u~ 3" },

        // Marschen: rakare, pulsvåg och virvel med förslag.
        { "Marsch 120", "EBM",
          { { id::tempo, 120.0f },
            { id::kbVag, 1.0f }, { id::kbPulsbredd, 28.0f }, { id::kbSub, 0.4f }, { id::kbCutoff, 260.0f },
            { id::kbResonans, 0.3f }, { id::kbEnvMangd, 0.5f }, { id::kbFilterFall, 0.16f }, { id::kbAccent, 0.6f },
            { id::kbFall, 0.25f }, { id::kbHall, 0.55f }, { id::kbDrive, 0.3f },
            { id::trumma (1, "klang"), 0.55f } },
          { { Trumma::Bastrumma,   "1000100010001000" },
            { Trumma::Virvel,      "00001{r1}00000001002" },
            { Trumma::StangdHihat, "1010101010101010" } },
          "0a . 0 0 0a . 0 0 3a . 3 3 -2a . -2 0" },

        // New Beat: långsamt och tungt, med lite gung och glid.
        { "New Beat 112", "New Beat",
          { { id::tempo, 112.0f }, { id::gung, 8.0f },
            { id::kbVag, 0.0f }, { id::kbSub, 0.8f }, { id::kbCutoff, 120.0f }, { id::kbResonans, 0.55f },
            { id::kbEnvMangd, 0.55f }, { id::kbFilterFall, 0.25f }, { id::kbAccent, 0.5f },
            { id::kbFall, 0.6f }, { id::kbHall, 0.7f }, { id::kbGlid, 0.07f }, { id::kbDrive, 0.6f },
            { id::trumma (0, "klang"), 0.65f }, { id::trumma (0, "stamning"), -2.0f } },
          { { Trumma::Bastrumma,   "1000100010001000" },
            { Trumma::Virvel,      "0000100000001000" },
            { Trumma::StangdHihat, "0020002000200020" } },
          "0a/3 . . 0 . . 0u~ 0 . . 0 . -5a/3 . -5 -3~" },
    };
    return list;
}

} // namespace km::presets
