// Paletten för VHS-panelen: chassit, silvertrycket, lysdioderna och skärmens tre färger.
// Svart, rött och vitt bär helheten; orange och cyan är accenter. Inga rosa eller lila toner.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace km::stil
{
// All text i koden är UTF-8 (ÅÄÖ, ▶ ■ ●); juce::String (const char*) tar bara ASCII.
inline juce::String sv (const char* utf8) { return juce::String::fromUTF8 (utf8); }
}

namespace km::stil::farg
{
    // Chassit
    inline const juce::Colour chassi       { 0xff141416 };   // borstad aluminium, svartanodiserad
    inline const juce::Colour plast        { 0xff0c0c0e };   // svart plast i infällda fält
    inline const juce::Colour plastKant    { 0xff2a2a2f };   // ljus fas på plastkanter
    inline const juce::Colour skugga       { 0xff000000 };

    // Tryck
    inline const juce::Colour silver       { 0xffc6c9cf };   // silvertryck
    inline const juce::Colour silverMatt   { 0xff7d8086 };   // sekundärt tryck, skalstreck
    inline const juce::Colour vitt         { 0xfff1efe9 };   // stort vitt tryck (modellnamn)
    inline const juce::Colour tryckRott    { 0xffd8191f };   // rött tryck och konstruktivistiska ytor

    // Lysdioder och displayer
    inline const juce::Colour lysRod       { 0xffff2a1c };
    inline const juce::Colour lysOrange    { 0xffff8a1e };
    inline const juce::Colour lysAv        { 0xff3a0f0c };   // släckt röd diod
    inline const juce::Colour segment      { 0xffff3322 };   // sjusegment
    inline const juce::Colour segmentGlas  { 0xff160505 };   // rödtonat displayglas
    inline const juce::Colour vfd          { 0xff5ff5e4 };   // vakuumfluorescens (presetnamnet)

    // Skärmen
    inline const juce::Colour skarmRod     { 0xffff2b33 };
    inline const juce::Colour skarmCyan    { 0xff3ff0ff };
    inline const juce::Colour skarmVit     { 0xfff4f7ff };
    inline const juce::Colour skarmSvart   { 0xff030406 };

    // Stegknapparnas fyra grupper (som 808:ans färgade knappar, men i EBM-skala)
    inline juce::Colour stegGrupp (int grupp)
    {
        static const juce::Colour g[4] { juce::Colour (0xffb51d1f),     // röd
                                         juce::Colour (0xffd2561d),     // orange
                                         juce::Colour (0xffd6d0c0),     // benvit
                                         juce::Colour (0xff57595e) };   // grå
        return g[juce::jlimit (0, 3, grupp)];
    }
}
