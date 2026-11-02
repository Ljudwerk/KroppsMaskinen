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

    km::TrumMonster trummor;
    km::TonMonster kolvbas;
    km::PresetManager presets { state, trummor, kolvbas };

    // Det virtuella keyboardet i gränssnittet skriver hit; noterna blandas in i MIDI-flödet.
    juce::MidiKeyboardState keyboardState;

    // För gränssnittet.
    std::atomic<float> outputPeak { 0.0f };
    int trumStegNu() const { return motor.aktuelltTrumSteg.load (std::memory_order_relaxed); }
    int basStegNu() const  { return motor.aktuelltBasSteg.load (std::memory_order_relaxed); }

private:
    km::params::Reader reader { state };
    km::Motor motor;
    juce::AudioBuffer<float> scratchRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenProcessor)
};
