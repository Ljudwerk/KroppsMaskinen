// Parameterträdet: alla automationsbara parametrar och hur de läses till motorn.
//
// Parameter-ID:n är kontrakt med DAW-projekt och presets: byt ALDRIG namn på ett ID som har funnits
// i ett släppt bygge (från 1.0). Alla börjar med "km_", följt av kanalen: "kb" (Kolvbasen), "kb2"
// (Kolvbas 2), "sv" (Stampverket) och "hu" (Hugget), eller effekten: "gr" (Grindrummet), "eko", "svaj",
// "klam" (Klämman), "lfo1/2", "menv1/2" och "mod1–6" (modulationen).
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Motor.h"

namespace km::params
{

namespace id
{
    // Taktverket och mastern
    inline constexpr auto spela    = "km_spela";
    inline constexpr auto tempo    = "km_tempo";
    inline constexpr auto gung     = "km_gung";
    inline constexpr auto grundton = "km_grundton";
    inline constexpr auto master   = "km_master";

    // Kolvbas 2 följer Kolvbasen
    inline constexpr auto folj          = "km_kb2_folj";
    inline constexpr auto forskjutning  = "km_kb2_forskjutning";
    inline constexpr auto transponering = "km_kb2_transponering";

    // Stampverket
    inline constexpr auto kit      = "km_sv_kit";
    inline constexpr auto kantslag = "km_sv_kantslag";

    // Varvtalet och Spänningen
    inline constexpr auto varvtal  = "km_varvtal";
    inline constexpr auto varvLage = "km_varvLage";
    inline constexpr auto spanning = "km_spanning";

    // Låtläget (fas 4)
    inline constexpr auto latLage = "km_lat_spela";
    inline constexpr auto latSlut = "km_lat_slut";

    // Hugget (ljudet; kanalens gemensamma parametrar heter km_hu_langd osv.)
    inline juce::String hu (const char* vad) { return juce::String ("km_hu_") + vad; }

    // Effekterna: km_gr_*, km_eko_*, km_svaj_*, km_klam_*
    inline juce::String fx (const char* effekt, const char* vad) { return juce::String ("km_") + effekt + "_" + vad; }

    // Modulationen
    inline juce::String lfo (int n, const char* vad) { return "km_lfo" + juce::String (n + 1) + "_" + vad; }
    inline juce::String menv (int n, const char* vad) { return "km_menv" + juce::String (n + 1) + "_" + vad; }
    inline juce::String mod (int n, const char* vad) { return "km_mod" + juce::String (n + 1) + "_" + vad; }

    inline const char* kanalPrefix (int kanal)
    {
        static const char* p[] = { "km_kb_", "km_kb2_", "km_sv_", "km_hu_" };
        return p[juce::jlimit (0, antalKanaler - 1, kanal)];
    }

    // Kanalens gemensamma parametrar: langd, monster, variant, drivetLage, drivetMangd, niva, pan, sandA, sandB, tyst, solo.
    inline juce::String kanal (int k, const char* vad) { return juce::String (kanalPrefix (k)) + vad; }

    // Kolvbasens ljudparametrar (k = 0 Kolvbasen, 1 Kolvbas 2), t.ex. km_kb2_cutoff.
    inline juce::String kolv (int k, const char* vad) { return kanal (k, vad); }

    // Stampverkets spår: km_sv_<spår>_<vad>, t.ex. km_sv_bastrumma_klang.
    inline const char* sparId (int spar)
    {
        static const char* namn[] = { "bastrumma", "virvel", "virvel2", "handklapp", "hihat", "oppenhihat",
                                      "cymbal", "tomlag", "tomhog", "metall", "small", "kubjallra" };
        return namn[juce::jlimit (0, antalSpar - 1, spar)];
    }
    inline juce::String trumma (int spar, const char* vad) { return juce::String ("km_sv_") + sparId (spar) + "_" + vad; }
}

inline juce::String namn (const char* utf8) { return juce::String::fromUTF8 (utf8); }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using F = juce::AudioParameterFloat;
    using I = juce::AudioParameterInt;
    using C = juce::AudioParameterChoice;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;
    using IA = juce::AudioParameterIntAttributes;
    const auto pid = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };
    auto skew = [] (float lo, float hi, float centre) { R r (lo, hi); r.setSkewForCentre (centre); return r; };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Taktverket och mastern
    layout.add (std::make_unique<B> (pid (id::spela), namn ("Spela"), false));
    layout.add (std::make_unique<F> (pid (id::tempo), namn ("Tempo"), R (40.0f, 200.0f, 0.1f), 125.0f, A().withLabel ("BPM")));
    layout.add (std::make_unique<F> (pid (id::gung), namn ("Gung"), R (0.0f, 50.0f, 0.1f), 0.0f, A().withLabel ("%")));
    layout.add (std::make_unique<I> (pid (id::grundton), namn ("Grundton"), 24, 48, 33,
        IA().withStringFromValueFunction ([] (int v, int) { return juce::MidiMessage::getMidiNoteName (v, true, true, 4); })));
    layout.add (std::make_unique<F> (pid (id::master), namn ("Master"), R (0.0f, 1.0f), 0.65f));

    // --- Kanalerna
    for (int k = 0; k < antalKanaler; ++k)
    {
        const auto rubrik = namn (kanalNamn ((Kanal) k)) + " ";
        layout.add (std::make_unique<I> (pid (id::kanal (k, "langd")), rubrik + namn ("längd"), 1, maxSteg, 16, IA().withLabel ("steg")));
        layout.add (std::make_unique<I> (pid (id::kanal (k, "monster")), rubrik + namn ("mönster"), 1, antalMonster, 1));
        layout.add (std::make_unique<C> (pid (id::kanal (k, "variant")), rubrik + "variant", juce::StringArray { "A", "B", "A+B" }, 0));
        layout.add (std::make_unique<C> (pid (id::kanal (k, "drivetLage")), rubrik + "drivet", juce::StringArray { "Av", namn ("Rör"), "Overdrive", "Bitkross" }, 0));
        layout.add (std::make_unique<F> (pid (id::kanal (k, "drivetMangd")), rubrik + namn ("drivet mängd"), R (0.0f, 1.0f), 0.3f));
        layout.add (std::make_unique<F> (pid (id::kanal (k, "niva")), rubrik + namn ("nivå"), R (0.0f, 1.0f), 0.8f));
        layout.add (std::make_unique<F> (pid (id::kanal (k, "pan")), rubrik + "panorering", R (-1.0f, 1.0f), 0.0f,
            A().withStringFromValueFunction ([] (float v, int) { const int p = juce::roundToInt (v * 100.0f);
                return p == 0 ? juce::String ("C") : (p < 0 ? "V " + juce::String (-p) : "H " + juce::String (p)); })));
        layout.add (std::make_unique<F> (pid (id::kanal (k, "sandA")), rubrik + namn ("till Grindrummet"), R (0.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<F> (pid (id::kanal (k, "sandB")), rubrik + "till Ekot", R (0.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<B> (pid (id::kanal (k, "tyst")), rubrik + "tyst", false));
        layout.add (std::make_unique<B> (pid (id::kanal (k, "solo")), rubrik + "solo", false));
    }

    // --- Kolvbasen och Kolvbas 2
    for (int k = 0; k < 2; ++k)
    {
        const auto r = namn (kanalNamn ((Kanal) k)) + " ";
        const bool tva = k == 1;
        layout.add (std::make_unique<F> (pid (id::kolv (k, "vag")), r + namn ("vågform"), R (0.0f, 1.0f), tva ? 1.0f : 0.0f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "pulsbredd")), r + "pulsbredd", R (5.0f, 50.0f, 0.1f), tva ? 30.0f : 50.0f, A().withLabel ("%")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "sub")), r + "sub", R (0.0f, 1.0f), tva ? 0.0f : 0.5f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "osc2Niva")), r + namn ("osc 2 nivå"), R (0.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "osc2Halvtoner")), r + namn ("osc 2 stämning"), R (-24.0f, 24.0f, 1.0f), 0.0f, A().withLabel ("st")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "osc2Fin")), r + namn ("osc 2 fin"), R (-50.0f, 50.0f, 1.0f), 7.0f, A().withLabel ("ct")));
        layout.add (std::make_unique<B> (pid (id::kolv (k, "synk")), r + "synk", false));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "brus")), r + "brus", R (0.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<C> (pid (id::kolv (k, "filterTyp")), r + "filtertyp", juce::StringArray { "24 dB", "12 dB" }, tva ? 1 : 0));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "cutoff")), r + "brytfrekvens", skew (30.0f, 8000.0f, 400.0f), tva ? 600.0f : 220.0f, A().withLabel ("Hz")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "resonans")), r + "resonans", R (0.0f, 1.0f), 0.35f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "envMangd")), r + namn ("envelopemängd"), R (0.0f, 1.0f), 0.6f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "filterFall")), r + "filterfall", skew (0.01f, 2.0f, 0.2f), 0.18f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "accent")), r + "accent", R (0.0f, 1.0f), 0.6f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "fall")), r + "fall", skew (0.01f, 3.0f, 0.3f), 0.3f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "hall")), r + namn ("håll"), R (0.0f, 1.0f), 0.6f));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "slapp")), r + namn ("släpp"), skew (0.002f, 1.0f, 0.05f), 0.012f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "glid")), r + "glid", skew (0.002f, 0.5f, 0.06f), 0.045f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::kolv (k, "drive")), r + "filterdrive", R (0.0f, 1.0f), 0.3f));
    }
    layout.add (std::make_unique<B> (pid (id::folj), namn ("Kolvbas 2 följ Kolvbasen"), false));
    layout.add (std::make_unique<I> (pid (id::forskjutning), namn ("Kolvbas 2 förskjutning"), 0, 3, 0,
        IA().withStringFromValueFunction ([] (int v, int) { return v == 0 ? juce::String ("Ingen") : juce::String (v) + "/16"; })));
    layout.add (std::make_unique<I> (pid (id::transponering), namn ("Kolvbas 2 transponering"), -24, 24, 12, IA().withLabel ("st")));

    // --- Stampverket
    layout.add (std::make_unique<C> (pid (id::kit), "Kit", juce::StringArray { namn ("Åttaåtta"), namn ("Niononio"), "Digital", "Fabrik" }, 0));
    layout.add (std::make_unique<B> (pid (id::kantslag), namn ("Kantslag i stället för kubjällra"), false));
    static const float klangStandard[] = { 0.45f, 0.4f, 0.5f, 0.5f, 0.2f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    for (int s = 0; s < antalSpar; ++s)
    {
        const auto rubrik = namn (trumNamn ((Trumma) s));
        layout.add (std::make_unique<F> (pid (id::trumma (s, "stamning")), rubrik + namn (" stämning"), R (-12.0f, 12.0f, 0.1f), 0.0f, A().withLabel ("st")));
        layout.add (std::make_unique<F> (pid (id::trumma (s, "klang")), rubrik + " klang", R (0.0f, 1.0f), klangStandard[s]));
        layout.add (std::make_unique<F> (pid (id::trumma (s, "niva")), rubrik + namn (" nivå"), R (0.0f, 1.0f), 0.8f));
        layout.add (std::make_unique<B> (pid (id::trumma (s, "tyst")), rubrik + " tyst", false));
    }

    // --- Hugget
    layout.add (std::make_unique<C> (pid (id::hu ("lage")), namn ("Hugget läge"),
        juce::StringArray { namn ("Orkesterstöt"), namn ("Blåsstöt"), namn ("Körstöt"), namn ("Klockstöt"), "Synklead", "Pulslead", "FM-lead" }, 0));
    layout.add (std::make_unique<C> (pid (id::hu ("ackord")), namn ("Hugget ackord"),
        juce::StringArray { "Ingen", "Moll", "Dur", "Kvint", "Oktav", "Moll 7", "Sus 4", "Moll 9" }, 1));
    layout.add (std::make_unique<F> (pid (id::hu ("klang")), "Hugget klang", R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::hu ("attack")), "Hugget attack", skew (0.001f, 1.0f, 0.05f), 0.002f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::hu ("fall")), "Hugget fall", skew (0.02f, 3.0f, 0.4f), 0.35f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::hu ("hall")), namn ("Hugget håll"), R (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (pid (id::hu ("slapp")), namn ("Hugget släpp"), skew (0.005f, 3.0f, 0.3f), 0.15f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::hu ("bredd")), "Hugget bredd", R (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<F> (pid (id::hu ("sankning")), namn ("Hugget sänkning"), R (0.0f, 1.0f), 0.4f));
    layout.add (std::make_unique<I> (pid (id::hu ("oktav")), "Hugget oktav", -2, 2, 0,
        IA().withStringFromValueFunction ([] (int v, int) { return (v > 0 ? "+" : "") + juce::String (v) + " okt"; })));

    // --- Effekterna
    layout.add (std::make_unique<F> (pid (id::fx ("gr", "storlek")), "Grindrummet storlek", R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::fx ("gr", "hallTid")), namn ("Grindrummet hålltid"), skew (0.02f, 1.0f, 0.25f), 0.22f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::fx ("gr", "ton")), "Grindrummet ton", R (0.0f, 1.0f), 0.6f));
    layout.add (std::make_unique<F> (pid (id::fx ("gr", "niva")), namn ("Grindrummet nivå"), R (0.0f, 1.0f), 0.7f));
    layout.add (std::make_unique<C> (pid (id::fx ("eko", "tid")), "Ekot tid", juce::StringArray { "1/16", "1/8", "3/16", "1/4", "3/8", "1/2" }, 2));
    layout.add (std::make_unique<F> (pid (id::fx ("eko", "aterkoppling")), namn ("Ekot återkoppling"), R (0.0f, 1.0f), 0.4f));
    layout.add (std::make_unique<F> (pid (id::fx ("eko", "ton")), "Ekot ton", R (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<B> (pid (id::fx ("eko", "pingpong")), "Ekot ping-pong", true));
    layout.add (std::make_unique<C> (pid (id::fx ("eko", "lage")), namn ("Ekot läge"), juce::StringArray { "BBD", "Band" }, 0));
    layout.add (std::make_unique<F> (pid (id::fx ("eko", "niva")), namn ("Ekot nivå"), R (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<C> (pid (id::fx ("svaj", "lage")), namn ("Svajet läge"), juce::StringArray { "Av", "I", "II", "I+II" }, 0));
    layout.add (std::make_unique<F> (pid (id::fx ("svaj", "mangd")), namn ("Svajet mängd"), R (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "troskel")), namn ("Klämman tröskel"), R (-40.0f, 0.0f, 0.1f), 0.0f, A().withLabel ("dB")));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "kvot")), namn ("Klämman kvot"), skew (1.0f, 20.0f, 4.0f), 3.0f,
        A().withStringFromValueFunction ([] (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0).replace (".", ",") + ":1"; })));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "attack")), namn ("Klämman attack"), skew (0.0005f, 0.1f, 0.01f), 0.01f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "slapp")), namn ("Klämman släpp"), skew (0.02f, 1.0f, 0.15f), 0.15f, A().withLabel ("s")));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "pump")), namn ("Klämman pump"), R (0.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (pid (id::fx ("klam", "efter")), namn ("Klämman efterförstärkning"), R (0.0f, 18.0f, 0.1f), 0.0f, A().withLabel ("dB")));

    // --- Varvtalet och Spänningen
    layout.add (std::make_unique<C> (pid (id::varvtal), "Varvtalet", juce::StringArray { "45", "33", "33 +8 %" }, 0));
    layout.add (std::make_unique<C> (pid (id::varvLage), namn ("Varvtalet läge"), juce::StringArray { "Musikaliskt", "Skivspelare" }, 0));
    layout.add (std::make_unique<F> (pid (id::spanning), namn ("Spänningen"), R (0.0f, 1.0f), 0.0f));

    // --- Modulationen
    const juce::StringArray former { "Sinus", "Triangel", "Fyrkant", namn ("Sågtand"), "Slump", "Glid" };
    const juce::StringArray synk { "Fri", "4 takter", "2 takter", "1 takt", "1/2", "1/4", "1/8", "1/16" };
    const juce::StringArray utlosare { "Kolvbasen", "Kolvbas 2", "Hugget", "Bastrumman", "Alla toner" };
    const juce::StringArray kallor { "Ingen", "LFO 1", "LFO 2", "Env 1", "Env 2", "Modhjul", "Aftertouch", "Anslag" };
    const juce::StringArray malen { "Inget", "Kolvbasen filter", "Kolvbas 2 filter", "Hugget klang", "Kolvbasen puls",
                                    namn ("Hugget tonhöjd"), "Drivet", "Grindrummet", "Ekot", namn ("Spänningen") };
    for (int n = 0; n < 2; ++n)
    {
        const auto r = "LFO " + juce::String (n + 1) + " ";
        layout.add (std::make_unique<C> (pid (id::lfo (n, "form")), r + "form", former, 0));
        layout.add (std::make_unique<F> (pid (id::lfo (n, "hz")), r + "hastighet", skew (0.02f, 20.0f, 2.0f), 2.0f, A().withLabel ("Hz")));
        layout.add (std::make_unique<C> (pid (id::lfo (n, "synk")), r + "synk", synk, n == 0 ? 3 : 0));
    }
    for (int n = 0; n < 2; ++n)
    {
        const auto r = "Env " + juce::String (n + 1) + " ";
        layout.add (std::make_unique<F> (pid (id::menv (n, "attack")), r + "attack", skew (0.001f, 2.0f, 0.1f), 0.005f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::menv (n, "fall")), r + "fall", skew (0.01f, 4.0f, 0.4f), 0.3f, A().withLabel ("s")));
        layout.add (std::make_unique<F> (pid (id::menv (n, "hall")), r + namn ("håll"), R (0.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<F> (pid (id::menv (n, "slapp")), r + namn ("släpp"), skew (0.01f, 4.0f, 0.4f), 0.2f, A().withLabel ("s")));
        layout.add (std::make_unique<C> (pid (id::menv (n, "utlosare")), r + namn ("utlösare"), utlosare, n));
    }
    for (int n = 0; n < antalKopplingar; ++n)
    {
        const auto r = "Koppling " + juce::String (n + 1) + " ";
        layout.add (std::make_unique<C> (pid (id::mod (n, "kalla")), r + namn ("källa"), kallor, 0));
        layout.add (std::make_unique<C> (pid (id::mod (n, "mal")), r + namn ("mål"), malen, 0));
        layout.add (std::make_unique<F> (pid (id::mod (n, "mangd")), r + namn ("mängd"), R (-1.0f, 1.0f), 0.0f));
    }

    // Fas 3: Kassetten sist, så att äldre parameterindex (automation) står kvar.
    layout.add (std::make_unique<C> (pid (id::fx ("kass", "kvalitet")), "Kassetten bandhastighet", juce::StringArray { "SP", "LP", "EP" }, 0));
    layout.add (std::make_unique<F> (pid (id::fx ("kass", "slitage")), "Kassetten slitage", R (0.0f, 1.0f), 0.0f));
    // Fas 4: låtläget.
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (id::latLage), namn ("Låtläget"), false));
    layout.add (std::make_unique<C> (pid (id::latSlut), namn ("Låtens slut"), juce::StringArray { "Loopa", "Stanna" }, 0));
    return layout;
}

// Läser alla parametrar till MotorParams. Pekarna slås upp en gång, så att ljudtråden inte skapar strängar.
class Reader
{
public:
    explicit Reader (juce::AudioProcessorValueTreeState& s)
    {
        auto p = [&s] (const juce::String& pidStr) { auto* v = s.getRawParameterValue (pidStr); jassert (v != nullptr); return v; };
        spela = p (id::spela); tempo = p (id::tempo); gung = p (id::gung); grundton = p (id::grundton); master = p (id::master);
        folj = p (id::folj); forskjutning = p (id::forskjutning); transponering = p (id::transponering);
        kit = p (id::kit); kantslag = p (id::kantslag);
        varvtal = p (id::varvtal); varvLage = p (id::varvLage); spanning = p (id::spanning);
        latLage = p (id::latLage); latSlut = p (id::latSlut);
        {
            size_t i = 0;
            for (auto* v : { "lage", "ackord", "klang", "attack", "fall", "hall", "slapp", "bredd", "sankning", "oktav" })
                hu[i++] = p (id::hu (v));
        }
        size_t fi = 0;
        for (auto [e, v] : std::initializer_list<std::pair<const char*, const char*>> {
                 { "gr", "storlek" }, { "gr", "hallTid" }, { "gr", "ton" }, { "gr", "niva" },
                 { "eko", "tid" }, { "eko", "aterkoppling" }, { "eko", "ton" }, { "eko", "pingpong" }, { "eko", "lage" }, { "eko", "niva" },
                 { "svaj", "lage" }, { "svaj", "mangd" },
                 { "klam", "troskel" }, { "klam", "kvot" }, { "klam", "attack" }, { "klam", "slapp" }, { "klam", "pump" }, { "klam", "efter" },
                 { "kass", "kvalitet" }, { "kass", "slitage" } })
            fx[fi++] = p (id::fx (e, v));
        for (int n = 0; n < 2; ++n)
        {
            lfoP[(size_t) n] = { p (id::lfo (n, "form")), p (id::lfo (n, "hz")), p (id::lfo (n, "synk")) };
            envP[(size_t) n] = { p (id::menv (n, "attack")), p (id::menv (n, "fall")), p (id::menv (n, "hall")),
                                 p (id::menv (n, "slapp")), p (id::menv (n, "utlosare")) };
        }
        for (int n = 0; n < antalKopplingar; ++n)
            koppP[(size_t) n] = { p (id::mod (n, "kalla")), p (id::mod (n, "mal")), p (id::mod (n, "mangd")) };

        for (int k = 0; k < antalKanaler; ++k)
        {
            auto& c = kanal[(size_t) k];
            c.langd = p (id::kanal (k, "langd")); c.monster = p (id::kanal (k, "monster")); c.variant = p (id::kanal (k, "variant"));
            c.drivetLage = p (id::kanal (k, "drivetLage")); c.drivetMangd = p (id::kanal (k, "drivetMangd"));
            c.niva = p (id::kanal (k, "niva")); c.tyst = p (id::kanal (k, "tyst")); c.solo = p (id::kanal (k, "solo"));
            c.pan = p (id::kanal (k, "pan")); c.sandA = p (id::kanal (k, "sandA")); c.sandB = p (id::kanal (k, "sandB"));
        }
        for (int k = 0; k < 2; ++k)
        {
            auto& b = kolv[(size_t) k];
            b.vag = p (id::kolv (k, "vag")); b.pulsbredd = p (id::kolv (k, "pulsbredd")); b.sub = p (id::kolv (k, "sub"));
            b.osc2Niva = p (id::kolv (k, "osc2Niva")); b.osc2Halvtoner = p (id::kolv (k, "osc2Halvtoner")); b.osc2Fin = p (id::kolv (k, "osc2Fin"));
            b.synk = p (id::kolv (k, "synk")); b.brus = p (id::kolv (k, "brus")); b.filterTyp = p (id::kolv (k, "filterTyp"));
            b.cutoff = p (id::kolv (k, "cutoff")); b.resonans = p (id::kolv (k, "resonans")); b.envMangd = p (id::kolv (k, "envMangd"));
            b.filterFall = p (id::kolv (k, "filterFall")); b.accent = p (id::kolv (k, "accent")); b.fall = p (id::kolv (k, "fall"));
            b.hall = p (id::kolv (k, "hall")); b.slapp = p (id::kolv (k, "slapp")); b.glid = p (id::kolv (k, "glid")); b.drive = p (id::kolv (k, "drive"));
        }
        for (int i = 0; i < antalSpar; ++i)
        {
            auto& t = spar[(size_t) i];
            t.stamning = p (id::trumma (i, "stamning")); t.klang = p (id::trumma (i, "klang"));
            t.niva = p (id::trumma (i, "niva")); t.tyst = p (id::trumma (i, "tyst"));
        }
    }

    MotorParams read() const
    {
        MotorParams m;
        m.takt.spela = spela->load() > 0.5f;
        m.takt.tempo = tempo->load();
        m.takt.gung = gung->load() * 0.01f;
        m.grundton = (int) grundton->load();
        m.master = master->load();
        m.foljKolvbasen = folj->load() > 0.5f;
        m.forskjutning = (int) forskjutning->load();
        m.transponering = (int) transponering->load();

        for (size_t k = 0; k < (size_t) antalKanaler; ++k)
        {
            const auto& c = kanal[k];
            auto& o = m.kanaler[k];
            o.langd = (int) c.langd->load();
            o.monster = (int) c.monster->load() - 1;
            o.variant = (Variant) (int) c.variant->load();
            o.drivet = { (DrivLage) (int) c.drivetLage->load(), c.drivetMangd->load() };
            o.niva = c.niva->load();
            o.pan = c.pan->load(); o.sandA = c.sandA->load(); o.sandB = c.sandB->load();
            o.tyst = c.tyst->load() > 0.5f;
            o.solo = c.solo->load() > 0.5f;
        }
        for (size_t k = 0; k < 2; ++k)
        {
            const auto& b = kolv[k];
            auto& o = m.kolvbas[k];
            o.vag = b.vag->load(); o.pulsbredd = b.pulsbredd->load() * 0.01f; o.sub = b.sub->load();
            o.osc2Niva = b.osc2Niva->load(); o.osc2Halvtoner = b.osc2Halvtoner->load(); o.osc2Fin = b.osc2Fin->load();
            o.synk = b.synk->load() > 0.5f; o.brus = b.brus->load(); o.filterTyp = (int) b.filterTyp->load();
            o.cutoffHz = b.cutoff->load(); o.resonans = b.resonans->load(); o.envMangd = b.envMangd->load();
            o.filterFall = b.filterFall->load(); o.accent = b.accent->load(); o.fall = b.fall->load();
            o.hall = b.hall->load(); o.slapp = b.slapp->load(); o.glid = b.glid->load(); o.drive = b.drive->load();
        }
        auto v = [this] (size_t i) { return hu[i]->load(); };   // i samma ordning som i konstruktorn
        m.hugget.lage = (HuggLage) (int) v (0);
        m.hugget.ackord = (int) v (1);
        m.hugget.klang = v (2); m.hugget.attack = v (3); m.hugget.fall = v (4);
        m.hugget.hall = v (5); m.hugget.slapp = v (6); m.hugget.bredd = v (7); m.hugget.sankning = v (8);
        m.huggOktav = (int) v (9);

        auto f = [this] (size_t i) { return fx[i]->load(); };
        m.grindrum = { f (0), f (1), f (2), f (3) };
        m.eko.tid = (int) f (4); m.eko.aterkoppling = f (5); m.eko.ton = f (6);
        m.eko.pingPong = f (7) > 0.5f; m.eko.lage = (EkoLage) (int) f (8); m.eko.niva = f (9);
        m.svaj = { (int) f (10), f (11) };
        m.klam = { f (12), f (13), f (14), f (15), f (16), f (17) };
        m.kassetten = { (Bandkvalitet) (int) f (18), f (19) };

        m.spanning = spanning->load();
        m.latLage = latLage->load() > 0.5f;
        m.latLoopa = latSlut->load() < 0.5f;
        varvFaktorNu = varvFaktor ((Varv) (int) varvtal->load());
        varvLageNu = (VarvLage) (int) varvLage->load();
        m.musikalisktVarv = varvLageNu == VarvLage::Musikaliskt ? varvFaktorNu : 1.0;

        for (size_t n = 0; n < 2; ++n)
        {
            m.mod.lfo[n] = { (LfoForm) (int) lfoP[n].form->load(), lfoP[n].hz->load(), (int) lfoP[n].synk->load() };
            m.mod.env[n] = { envP[n].attack->load(), envP[n].fall->load(), envP[n].hall->load(), envP[n].slapp->load(),
                             (EnvUtlosare) (int) envP[n].utlosare->load() };
        }
        for (size_t n = 0; n < (size_t) antalKopplingar; ++n)
            m.mod.kopplingar[n] = { (ModKalla) (int) koppP[n].kalla->load(), (ModMal) (int) koppP[n].mal->load(), koppP[n].mangd->load() };

        m.stampverket.kit = (Kit) (int) kit->load();
        m.stampverket.kantslag = kantslag->load() > 0.5f;
        for (size_t s = 0; s < (size_t) antalSpar; ++s)
        {
            m.stampverket.trummor[s] = { spar[s].stamning->load(), spar[s].klang->load(), spar[s].niva->load() };
            m.sparTyst[s] = spar[s].tyst->load() > 0.5f;
        }
        return m;
    }

    // Varvtalet efter senaste read(): faktorn och läget (skivspelarläget sköts av processorn).
    mutable double varvFaktorNu = 1.0;
    mutable VarvLage varvLageNu = VarvLage::Musikaliskt;

private:
    using P = std::atomic<float>*;
    P spela, tempo, gung, grundton, master, folj, forskjutning, transponering, kit, kantslag, varvtal, varvLage, spanning, latLage, latSlut;
    struct KanalP { P langd, monster, variant, drivetLage, drivetMangd, niva, tyst, solo, pan, sandA, sandB; };
    std::array<P, 10> hu {};
    std::array<P, 20> fx {};
    struct LfoP { P form, hz, synk; };
    struct EnvP { P attack, fall, hall, slapp, utlosare; };
    struct KoppP { P kalla, mal, mangd; };
    std::array<LfoP, 2> lfoP {};
    std::array<EnvP, 2> envP {};
    std::array<KoppP, antalKopplingar> koppP {};
    struct KolvP { P vag, pulsbredd, sub, osc2Niva, osc2Halvtoner, osc2Fin, synk, brus, filterTyp,
                     cutoff, resonans, envMangd, filterFall, accent, fall, hall, slapp, glid, drive; };
    struct SparP { P stamning, klang, niva, tyst; };
    std::array<KanalP, antalKanaler> kanal {};
    std::array<KolvP, 2> kolv {};
    std::array<SparP, antalSpar> spar {};
};

} // namespace km::params
