// Värdeformatet: hur alla rattar och displayen visar sina värden, på ett och samma sätt.
// Kopierad från KrangBox (Source/ui/ValueFormat.h). Rättningar förs över för hand.
//   0..1 utan enhet  -> procent ("30 %")
//   s / ms           -> "250 ms" under en sekund, annars "1,2 s"
//   Hz               -> "2,0 Hz", "400 Hz", "2,4 kHz"
//   dB, okt          -> en decimal ("−6,0 dB")
//   ct, st           -> heltal med tecken ("+7 ct", "−12 st")
// Värden som parametern själv skriver som ord ("Av", "A1") lämnas som de är.
// Decimaltecknet är komma och minustecknet ett riktigt minus, som i svensk text.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace km::ui
{

inline juce::String swedishNumber (double v, int decimals, bool sign = false)
{
    if (decimals == 0) v = std::round (v);
    if (std::abs (v) < 0.5 * std::pow (10.0, -decimals)) v = 0.0;   // aldrig "−0"
    auto s = juce::String (std::abs (v), decimals).replaceCharacter ('.', ',');
    if (v < 0.0) return juce::String::fromUTF8 ("\xe2\x88\x92") + s;
    return (sign && v > 0.0 ? "+" : "") + s;
}

inline bool looksNumeric (const juce::String& t)
{
    return t.isNotEmpty() && t.trim().containsOnly ("0123456789.-+, ");
}

inline juce::String formatValue (juce::RangedAudioParameter& param, float value)
{
    const auto raw = param.getText (param.convertTo0to1 (value), 0);
    const auto unit = param.getLabel();
    if (! looksNumeric (raw)) return unit.isEmpty() ? raw : raw + " " + unit;

    const double v = value;
    const auto range = param.getNormalisableRange();
    const bool isFloat = dynamic_cast<juce::AudioParameterFloat*> (&param) != nullptr;

    if (unit == "s" || unit == "ms")
    {
        const double sec = unit == "ms" ? v / 1000.0 : v;
        if (sec < 0.9995) return swedishNumber (sec * 1000.0, 0) + " ms";
        return swedishNumber (sec, sec < 9.95 ? 1 : 0) + " s";
    }
    if (unit == "Hz")
    {
        if (v >= 1000.0) return swedishNumber (v / 1000.0, 1) + " kHz";
        return swedishNumber (v, v < 10.0 ? 1 : 0) + " Hz";
    }
    if (unit == "dB" || unit == "okt") return swedishNumber (v, 1, unit == "okt") + " " + unit;
    if (unit == "ct" || unit == "cent") return swedishNumber (v, 0, true) + " ct";
    if (unit == "st")                   return swedishNumber (v, 0, true) + " st";
    if (unit == "%")                   return swedishNumber (v, 0) + " %";
    if (unit.isEmpty() && isFloat && juce::exactlyEqual (range.start, 0.0f) && juce::exactlyEqual (range.end, 1.0f))
        return swedishNumber (v * 100.0, 0) + " %";
    if (unit.isEmpty())
        return isFloat && range.interval < 1.0f ? swedishNumber (v, std::abs (v) < 10.0 ? 2 : 1) : swedishNumber (v, 0);
    return raw.replaceCharacter ('.', ',') + " " + unit;
}

} // namespace km::ui
