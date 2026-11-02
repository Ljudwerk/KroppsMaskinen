// Parameterträdet: alla automationsbara parametrar och hur de läses till motorn.
//
// Parameter-ID:n är kontrakt med DAW-projekt och presets: byt ALDRIG namn på ett ID som har funnits
// i ett släppt bygge. Alla börjar med "km_".
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Motor.h"

namespace km::params
{

namespace id
{
    // Taktverket och mixern
    inline constexpr auto spela     = "km_spela";
    inline constexpr auto tempo     = "km_tempo";
    inline constexpr auto gung      = "km_gung";
    inline constexpr auto trumLangd = "km_trumLangd";
    inline constexpr auto basLangd  = "km_basLangd";
    inline constexpr auto grundton  = "km_grundton";
    inline constexpr auto basNiva   = "km_basNiva";
    inline constexpr auto trumNiva  = "km_trumNiva";
    inline constexpr auto master    = "km_master";

    // Kolvbasen
    inline constexpr auto kbVag        = "km_kb_vag";
    inline constexpr auto kbPulsbredd  = "km_kb_pulsbredd";
    inline constexpr auto kbSub        = "km_kb_sub";
    inline constexpr auto kbCutoff     = "km_kb_cutoff";
    inline constexpr auto kbResonans   = "km_kb_resonans";
    inline constexpr auto kbEnvMangd   = "km_kb_envMangd";
    inline constexpr auto kbFilterFall = "km_kb_filterFall";
    inline constexpr auto kbAccent     = "km_kb_accent";
    inline constexpr auto kbFall       = "km_kb_fall";
    inline constexpr auto kbHall       = "km_kb_hall";
    inline constexpr auto kbSlapp      = "km_kb_slapp";
    inline constexpr auto kbGlid       = "km_kb_glid";
    inline constexpr auto kbDrive      = "km_kb_drive";

    // Stampverket: km_sv_<trumma>_<vad>, t.ex. km_sv_bastrumma_klang
    inline juce::String trumma (int byggd, const char* vad)
    {
        static const char* namn[] = { "bastrumma", "virvel", "hihat" };
        return juce::String ("km_sv_") + namn[juce::jlimit (0, antalByggda - 1, byggd)] + "_" + vad;
    }
}

// Ett parameternamn som det visas i DAW:en.
inline juce::String namn (const char* utf8) { return juce::String::fromUTF8 (utf8); }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using F = juce::AudioParameterFloat;
    using I = juce::AudioParameterInt;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;
    const auto pid = [] (const char* s) { return juce::ParameterID { s, 1 }; };
    const auto pidS = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };

    auto skew = [] (float lo, float hi, float centre) { R r (lo, hi); r.setSkewForCentre (centre); return r; };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Taktverket och mixern
    layout.add (std::make_unique<B> (pid (id::spela), namn ("Spela"), false));
    layout.add (std::make_unique<F> (pid (id::tempo), namn ("Tempo"), R (40.0f, 200.0f, 0.1f), 125.0f, A().withLabel ("BPM")));
    layout.add (std::make_unique<F> (pid (id::gung), namn ("Gung"), R (0.0f, 50.0f, 0.1f), 0.0f, A().withLabel ("%")));
    layout.add (std::make_unique<I> (pid (id::trumLangd), namn ("Trummor längd"), 1, maxSteg, 16, juce::AudioParameterIntAttributes().withLabel ("steg")));
    layout.add (std::make_unique<I> (pid (id::basLangd), namn ("Kolvbasen längd"), 1, maxSteg, 16, juce::AudioParameterIntAttributes().withLabel ("steg")));
    layout.add (std::make_unique<I> (pid (id::grundton), namn ("Grundton"), 24, 48, 33,
        juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return juce::MidiMessage::getMidiNoteName (v, true, true, 4); })));
    layout.add (std::make_unique<F> (pid (id::basNiva), namn ("Kolvbasen nivå"), R (0.0f, 1.0f), 0.8f));
    layout.add (std::make_unique<F> (pid (id::trumNiva), namn ("Stampverket nivå"), R (0.0f, 1.0f), 0.8f));
    layout.add (std::make_unique<F> (pid (id::master), namn ("Master"), R (0.0f, 1.0f), 0.65f));

    // --- Kolvbasen
    layout.add (std::make_unique<F> (pid (id::kbVag), namn ("Kolvbasen vågform"), R (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (pid (id::kbPulsbredd), namn ("Kolvbasen pulsbredd"), R (5.0f, 50.0f, 0.1f), 50.0f, A().withLabel ("%")));
    layout.add (std::make_unique<F> (pid (id::kbSub), namn ("Kolvbasen sub"), R (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<F> (pid (id::kbCutoff), namn ("Kolvbasen brytfrekvens"), skew (30.0f, 8000.0f, 400.0f), 220.0f, A().withLabel ("Hz")));
    layout.add (std::make_unique<F> (pid (id::kbResonans), namn ("Kolvbasen resonans"), R (0.0f, 1.0f), 0.35f));
    layout.add (std::make_unique<F> (pid (id::kbEnvMangd), namn ("Kolvbasen envelopemängd"), R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::kbFilterFall), namn ("Kolvbasen filterfall"), skew (0.01f, 2.0f, 0.2f), 0.18f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::kbAccent), namn ("Kolvbasen accent"), R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::kbFall), namn ("Kolvbasen fall"), skew (0.01f, 3.0f, 0.3f), 0.3f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::kbHall), namn ("Kolvbasen håll"), R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::kbSlapp), namn ("Kolvbasen släpp"), skew (0.002f, 1.0f, 0.05f), 0.012f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::kbGlid), namn ("Kolvbasen glid"), skew (0.002f, 0.5f, 0.06f), 0.045f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::kbDrive), namn ("Kolvbasen drive"), R (0.0f, 1.0f), 0.3f));

    // --- Stampverket
    static const char* trumRubrik[] = { "Bastrumma", "Virvel", "Hi-hat" };
    static const float klangStandard[] = { 0.45f, 0.4f, 0.2f };
    for (int d = 0; d < antalByggda; ++d)
    {
        const auto rubrik = namn (trumRubrik[d]);
        layout.add (std::make_unique<F> (pidS (id::trumma (d, "stamning")), rubrik + namn (" stämning"), R (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st")));
        layout.add (std::make_unique<F> (pidS (id::trumma (d, "klang")), rubrik + " klang", R (0.0f, 1.0f), klangStandard[d]));
        layout.add (std::make_unique<F> (pidS (id::trumma (d, "niva")), rubrik + namn (" nivå"), R (0.0f, 1.0f), 0.8f));
    }

    return layout;
}

// Läser alla parametrar till MotorParams. Pekarna slås upp en gång, så att ljudtråden inte skapar strängar.
class Reader
{
public:
    explicit Reader (juce::AudioProcessorValueTreeState& s)
    {
        auto p = [&s] (const juce::String& pidStr) { auto* v = s.getRawParameterValue (pidStr); jassert (v != nullptr); return v; };
        spela = p (id::spela); tempo = p (id::tempo); gung = p (id::gung);
        trumLangd = p (id::trumLangd); basLangd = p (id::basLangd); grundton = p (id::grundton);
        basNiva = p (id::basNiva); trumNiva = p (id::trumNiva); master = p (id::master);

        vag = p (id::kbVag); pulsbredd = p (id::kbPulsbredd); sub = p (id::kbSub); cutoff = p (id::kbCutoff);
        resonans = p (id::kbResonans); envMangd = p (id::kbEnvMangd); filterFall = p (id::kbFilterFall);
        accent = p (id::kbAccent); fall = p (id::kbFall); hall = p (id::kbHall); slapp = p (id::kbSlapp);
        glid = p (id::kbGlid); drive = p (id::kbDrive);

        for (int d = 0; d < antalByggda; ++d)
        {
            trumStamning[(size_t) d] = p (id::trumma (d, "stamning"));
            trumKlang[(size_t) d] = p (id::trumma (d, "klang"));
            trumNivaer[(size_t) d] = p (id::trumma (d, "niva"));
        }
    }

    MotorParams read() const
    {
        MotorParams m;
        m.takt.spela = spela->load() > 0.5f;
        m.takt.tempo = tempo->load();
        m.takt.gung = gung->load() * 0.01f;
        m.trumLangd = (int) trumLangd->load();
        m.basLangd = (int) basLangd->load();
        m.grundton = (int) grundton->load();
        m.basNiva = basNiva->load();
        m.trumNiva = trumNiva->load();
        m.master = master->load();

        auto& k = m.kolvbasen;
        k.vag = vag->load(); k.pulsbredd = pulsbredd->load() * 0.01f; k.sub = sub->load(); k.cutoffHz = cutoff->load();
        k.resonans = resonans->load(); k.envMangd = envMangd->load(); k.filterFall = filterFall->load();
        k.accent = accent->load(); k.fall = fall->load(); k.hall = hall->load(); k.slapp = slapp->load();
        k.glid = glid->load(); k.drive = drive->load();

        for (size_t d = 0; d < (size_t) antalByggda; ++d)
            m.stampverket.trummor[d] = { trumStamning[d]->load(), trumKlang[d]->load(), trumNivaer[d]->load() };
        return m;
    }

private:
    using P = std::atomic<float>*;
    P spela, tempo, gung, trumLangd, basLangd, grundton, basNiva, trumNiva, master;
    P vag, pulsbredd, sub, cutoff, resonans, envMangd, filterFall, accent, fall, hall, slapp, glid, drive;
    std::array<P, antalByggda> trumStamning {}, trumKlang {}, trumNivaer {};
};

} // namespace km::params
