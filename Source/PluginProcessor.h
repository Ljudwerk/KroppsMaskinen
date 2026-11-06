// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "presets/PresetManager.h"
#include <atomic>

class KroppsmaskinenProcessor final : public juce::AudioProcessor,
                                      private juce::ValueTree::Listener
{
public:
    KroppsmaskinenProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Kroppsmaskinen"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    // DAW:ens programlista = fabriksbandet. Egna presets nås från gränssnittet.
    int getNumPrograms() override { return presets.getNumFactory(); }
    int getCurrentProgram() override { return juce::jlimit (0, presets.getNumFactory() - 1, presets.getCurrentIndex()); }
    void setCurrentProgram (int index) override { presets.load (index); }
    const juce::String getProgramName (int index) override
    {
        const auto& e = presets.getEntries();
        return juce::isPositiveAndBelow (index, (int) e.size()) ? e[(size_t) index].name : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Egen klocka när ingen DAW-transport finns: alltid i den fristående appen och i verktygen.
    bool egenKlocka() const { return forceraEgenKlocka || wrapperType == wrapperType_Standalone; }
    bool forceraEgenKlocka = false;

    juce::AudioProcessorValueTreeState state;

    km::Banker banker;
    km::PresetManager presets { state, banker };
    km::Ljudbanken ljud { state };

    // Det virtuella keyboardet i gränssnittet skriver hit; noterna blandas in i MIDI-flödet.
    juce::MidiKeyboardState keyboardState;

    // För gränssnittet.
    std::atomic<float> outputPeak { 0.0f };
    int stegNu (km::Kanal k) const { return motor.aktuelltSteg[(size_t) k].load (std::memory_order_relaxed); }
    int monsterNu (km::Kanal k) const { return motor.spelarMonster[(size_t) k].load (std::memory_order_relaxed); }
    float klamReduktionDb() const { return motor.getKlamReduktionDb(); }

    // VHS-panelen: slag, nivåer, oscilloskop, modulation och räkneverket.
    std::array<std::atomic<float>, 2> utTopp {};                     // vänster/höger, byts mot 0 när panelen läser
    uint32_t kanalSlag (km::Kanal k) const { return motor.kanalSlag[(size_t) k].load (std::memory_order_relaxed); }
    uint32_t trumSlag (int spar) const { return motor.trumSlag[(size_t) spar].load (std::memory_order_relaxed); }
    float kanalTopp (km::Kanal k) { return motor.kanalTopp[(size_t) k].exchange (0.0f, std::memory_order_relaxed); }
    double ppqNu() const { return motor.ppqNu.load (std::memory_order_relaxed); }
    int latRad() const { return motor.latRadNu.load (std::memory_order_relaxed); }    // låtläget: spelande rad, −1 = ingen
    int latTakt() const { return motor.latTaktNu.load (std::memory_order_relaxed); }
    float modKalla (km::ModKalla k) const { return motor.getModulation().kallVarde (k); }
    // Oscilloskopet: de senaste n värdena, äldst först.
    void lasScope (float* ut, int n) const
    {
        const int pos = motor.scopePos.load (std::memory_order_relaxed);
        const int L = km::Motor::scopeLangd;
        for (int i = 0; i < n; ++i)
            ut[i] = motor.scope[(size_t) ((pos - n + i + 2 * L) % L)].load (std::memory_order_relaxed);
    }
    // Räkneverket: speltid i sekunder (räknas när sekvensern går), nollställs av SPOLA.
    double speltid() const { const double sr = forberedd.load(); return sr > 0.0 ? (double) speltidSampel.load() / sr : 0.0; }
    void spola() { spolaBegaran.store (true); speltidSampel.store (0); }

    // Stämningen "Egen (Scala)": .scl- och .kbm-texten sparas i läget (och i egna presets) som
    // egenskaperna "scl" och "kbm"; den tolkade skalan läses av motorn.
    km::EgenStamning egenStamning;
    bool lasStamningsfil (const juce::File& f, juce::String& fel);
    void taBortKarta() { state.state.removeProperty ("kbm", nullptr); }
    juce::String egenStamningNamn() const;
    bool harKarta() const { return state.state.hasProperty ("kbm"); }

    // Panelens egna inställningar (bandstyrka, skala): sparas i läget men inte i presets.
    juce::ValueTree panelLage { "Panel" };

    // Diagnostik för gränssnittet: samplingsfrekvensen motorn förberetts med, senaste blockstorleken,
    // antal behandlade sampel och antal spelade sextondelar. Jämförs med väggklockan i gränssnittet.
    std::atomic<double> forberedd { 0.0 };
    std::atomic<int> senasteBlock { 0 };
    std::atomic<juce::int64> antalSampel { 0 };
    juce::int64 spelade() const { return motor.antalSpeladeSteg.load (std::memory_order_relaxed); }

private:
    void uppdateraEgenStamning();
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& id) override
    {
        if (id == juce::Identifier ("scl") || id == juce::Identifier ("kbm")) uppdateraEgenStamning();
    }
    void valueTreeRedirected (juce::ValueTree&) override { uppdateraEgenStamning(); }

    km::params::Reader reader { state };
    km::Motor motor;
    juce::AudioBuffer<float> scratchRight;
    std::atomic<bool> spolaBegaran { false };
    std::atomic<juce::int64> speltidSampel { 0 };

    // Varvtalet i skivspelarläge.
    void skivspelare (float* left, float* right, int n, const juce::MidiBuffer& midi, const km::Transport& transport, double f);
    juce::AudioBuffer<float> varvBuffer;
    juce::MidiBuffer varvMidi;
    double varvPos = 0.0;
    std::array<std::array<float, 4>, 2> varvHist {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenProcessor)
};
