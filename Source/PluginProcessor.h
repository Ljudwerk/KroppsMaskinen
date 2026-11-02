// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "presets/PresetManager.h"
#include <atomic>

class KroppsmaskinenProcessor final : public juce::AudioProcessor
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

    // Det virtuella keyboardet i gränssnittet skriver hit; noterna blandas in i MIDI-flödet.
    juce::MidiKeyboardState keyboardState;

    // För gränssnittet.
    std::atomic<float> outputPeak { 0.0f };
    int stegNu (km::Kanal k) const { return motor.aktuelltSteg[(size_t) k].load (std::memory_order_relaxed); }
    int monsterNu (km::Kanal k) const { return motor.spelarMonster[(size_t) k].load (std::memory_order_relaxed); }
    float klamReduktionDb() const { return motor.getKlamReduktionDb(); }

    // Diagnostik för gränssnittet: samplingsfrekvensen motorn förberetts med, senaste blockstorleken,
    // antal behandlade sampel och antal spelade sextondelar. Jämförs med väggklockan i gränssnittet.
    std::atomic<double> forberedd { 0.0 };
    std::atomic<int> senasteBlock { 0 };
    std::atomic<juce::int64> antalSampel { 0 };
    juce::int64 spelade() const { return motor.antalSpeladeSteg.load (std::memory_order_relaxed); }

private:
    km::params::Reader reader { state };
    km::Motor motor;
    juce::AudioBuffer<float> scratchRight;

    // Varvtalet i skivspelarläge.
    void skivspelare (float* left, float* right, int n, const juce::MidiBuffer& midi, const km::Transport& transport, double f);
    juce::AudioBuffer<float> varvBuffer;
    juce::MidiBuffer varvMidi;
    double varvPos = 0.0;
    std::array<std::array<float, 4>, 2> varvHist {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenProcessor)
};
