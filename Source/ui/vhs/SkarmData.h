// SkarmData: allt bildskärmen behöver veta om maskinen, en ögonblicksbild per bildruta.
//
// Panelen fyller i strukturen från processorn (atomics) och parametrarna i sin timer och lämnar
// den till Bildskarm::uppdatera(). Bildskärmen läser aldrig processorn själv.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

namespace km::vhs
{

// Vad panelen visar. Skärmens scen väljs av vy + kanal.
enum class Vy { Kanal = 0, Sekvens, Mixer, Effekter, Modulation };

// Bandstyrka: hur mycket VHS-fel som läggs på bilden (inställning i MENY, sparas i läget).
enum class Bandstyrka { Av = 0, Lite, Mycket };

struct SkarmData
{
    int  kanal   = 0;                // 0 Kolvbasen, 1 Kolvbas 2, 2 Stampverket, 3 Hugget
    Vy   vy      = Vy::Kanal;
    bool spelar  = false;

    // Sekvensern för vald kanal.
    int   steg    = -1;              // aktuellt steg 0..63, -1 = stoppad
    float stegFas = 0.0f;            // 0..1 inom steget (för mjuk animation mellan stegen)
    int   langd   = 16;              // mönstrets längd i steg
    std::array<float, 64> stegNiva {};   // 0 = tyst, 0,6 = slag/ton, 1 = accent
    int   monster = 0;               // spelande mönster 0..15
    char  variant = 'A';             // 'A' eller 'B'

    // Händelser: ökar med ett för varje slag (jämför med förra värdet för att se nya slag).
    std::array<uint32_t, 4>  kanalSlag {};     // per kanal
    std::array<uint32_t, 12> trumSlag {};      // per trumma i Stampverket (spårordning som i Monster.h)
    int valdTrumma = 0;                        // vald trumma i Stampverket (TR-väljaren)

    // Nivåer 0..1 (topp sedan förra bildrutan).
    std::array<float, 4> kanalNiva {};
    float niva = 0.0f;                         // mastern
    std::array<float, 256> scope {};           // senaste utsignalen (mono, −1..1), äldst först

    // Modulationen.
    std::array<float, 2> lfoVarde {};          // −1..1
    std::array<int, 2>   lfoForm {};           // LfoForm i Modulation.h
    std::array<float, 2> menvVarde {};         // 0..1

    // OSD-texten (bandspelarens, skarp ovanpå bandet).
    juce::String osdLage, osdKvalitet, osdRakneverk, osdKanal;

    Bandstyrka bandstyrka = Bandstyrka::Mycket;
    float slitage = 0.0f;                      // Kassettens slitage 0..1, sliter även bilden
};

} // namespace km::vhs
