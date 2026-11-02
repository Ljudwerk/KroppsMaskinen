// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

KroppsmaskinenProcessor::KroppsmaskinenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Kroppsmaskinen", km::params::createLayout())
{
    state.state.setProperty ("formatVersion", km::PresetManager::formatVersion, nullptr);
    presets.loadByName ("Kolv 125");
}

void KroppsmaskinenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    motor.prepare (sampleRate);
    motor.setParams (reader.read());
    scratchRight.setSize (1, juce::jmax (1, samplesPerBlock));
}

bool KroppsmaskinenProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void KroppsmaskinenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    km::Transport transport;
    if (auto* ph = getPlayHead())
        if (const auto pos = ph->getPosition())
        {
            transport.hostSpelar = pos->getIsPlaying();
            transport.ppq = pos->getPpqPosition().orFallback (0.0);
            transport.bpm = pos->getBpm().orFallback (120.0);
        }

    motor.setParams (reader.read());
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    if (scratchRight.getNumSamples() < n) scratchRight.setSize (1, n, false, false, true); // nödfall: för stort block
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    motor.process (left, right, n, midi, transport, egenKlocka(), trummor, kolvbas);

    const float peak = buffer.getMagnitude (0, n);
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
}

juce::AudioProcessorEditor* KroppsmaskinenProcessor::createEditor()
{
    return new KroppsmaskinenEditor (*this);
}

void KroppsmaskinenProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto copy = state.copyState();
    km::PresetManager::writePatterns (copy, trummor, kolvbas);
    if (auto xml = copy.createXml())
        copyXmlToBinary (*xml, destData);
}

void KroppsmaskinenProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            if (km::PresetManager::readPatterns (tree, trummor, kolvbas))
                state.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KroppsmaskinenProcessor();
}
