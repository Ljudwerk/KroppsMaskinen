// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

KroppsmaskinenProcessor::KroppsmaskinenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "Kroppsmaskinen", km::params::createLayout())
{
    state.state.setProperty ("formatVersion", km::PresetManager::formatVersion, nullptr);
    state.state.addListener (this);
    motor.setEgenStamning (&egenStamning);
    presets.loadByName ("Kolv 125");
}

void KroppsmaskinenProcessor::uppdateraEgenStamning()
{
    km::Skala s = km::Skala::tet (12);
    if (state.state.hasProperty ("scl")) s.lasScl (state.state["scl"].toString());
    if (state.state.hasProperty ("kbm"))
    {
        km::Skala k = s;
        if (k.lasKbm (state.state["kbm"].toString())) s = k;
    }
    egenStamning.satt (s);
}

bool KroppsmaskinenProcessor::lasStamningsfil (const juce::File& f, juce::String& fel)
{
    const auto text = f.loadFileAsString();
    km::Skala prov = km::Skala::tet (12);
    if (f.hasFileExtension ("kbm"))
    {
        if (! prov.lasKbm (text, &fel)) return false;
        state.state.setProperty ("kbm", text, nullptr);
        return true;
    }
    if (! prov.lasScl (text, &fel)) return false;
    state.state.setProperty ("scl", text, nullptr);
    state.state.setProperty ("sclNamn", f.getFileNameWithoutExtension(), nullptr);
    return true;
}

juce::String KroppsmaskinenProcessor::egenStamningNamn() const
{
    return state.state.getProperty ("sclNamn", "").toString();
}

void KroppsmaskinenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    forberedd.store (sampleRate);
    motor.prepare (sampleRate);
    motor.setParams (reader.read());
    scratchRight.setSize (1, juce::jmax (1, samplesPerBlock));
    varvBuffer.setSize (2, juce::jmax (1, samplesPerBlock) + 4);
    varvMidi.ensureSize (4096);
    varvPos = 0.0;
    varvHist = {};
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
    senasteBlock.store (n, std::memory_order_relaxed);
    antalSampel.fetch_add (n, std::memory_order_relaxed);
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
    if (spolaBegaran.exchange (false)) motor.spola();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    if (scratchRight.getNumSamples() < n) scratchRight.setSize (1, n, false, false, true); // nödfall: för stort block
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : scratchRight.getWritePointer (0);

    // Varvtalet i skivspelarläge: hela maskinen (även effekterna) spelas långsammare, som en skiva på fel
    // varvtal. Motorn räknar fram färre sampel som sträcks ut. Med DAW-transport blir det musikaliskt läge
    // i stället, så att tempot följer projektet.
    const double f = reader.varvFaktorNu;
    if (reader.varvLageNu == km::VarvLage::Skivspelare && f < 0.999 && ! transport.hostSpelar)
        skivspelare (left, right, n, midi, transport, f);
    else
        motor.process (left, right, n, midi, transport, egenKlocka(), banker);

    if (buffer.getNumChannels() == 1)   // monobuss: summera
        for (int i = 0; i < n; ++i) left[i] = 0.5f * (left[i] + right[i]);

    const float peak = buffer.getMagnitude (0, n);
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}
    for (int c = 0; c < 2; ++c)
    {
        const float t = buffer.getMagnitude (juce::jmin (c, buffer.getNumChannels() - 1), 0, n);
        float forra = utTopp[(size_t) c].load();
        while (t > forra && ! utTopp[(size_t) c].compare_exchange_weak (forra, t)) {}
    }
    if (motor.ppqNu.load (std::memory_order_relaxed) >= 0.0) speltidSampel.fetch_add (n, std::memory_order_relaxed);
}

void KroppsmaskinenProcessor::skivspelare (float* left, float* right, int n, const juce::MidiBuffer& midi,
                                           const km::Transport& transport, double f)
{
    // Hur många sampel motorn behöver räkna fram i det här blocket.
    const int m = (int) std::floor (varvPos + n * f);
    if (varvBuffer.getNumSamples() < m + 1) varvBuffer.setSize (2, m + 1, false, false, true);   // nödfall

    // MIDI-händelserna hamnar på motsvarande plats i motorns (långsammare) tid.
    varvMidi.clear();
    for (const auto meta : midi)
        varvMidi.addEvent (meta.getMessage(), juce::jlimit (0, juce::jmax (0, m - 1), (int) (meta.samplePosition * f)));

    float* vl = varvBuffer.getWritePointer (0);
    float* vr = varvBuffer.getWritePointer (1);
    if (m > 0) motor.process (vl, vr, m, varvMidi, transport, egenKlocka(), banker);

    // Hermite-interpolation mellan de fyra senaste sampeln.
    int las = 0;
    for (int i = 0; i < n; ++i)
    {
        varvPos += f;
        while (varvPos >= 1.0 && las < m)
        {
            for (size_t c = 0; c < 2; ++c)
            {
                auto& h = varvHist[c];
                h[0] = h[1]; h[1] = h[2]; h[2] = h[3];
                h[3] = (c == 0 ? vl : vr)[las];
            }
            ++las;
            varvPos -= 1.0;
        }
        const float t = (float) varvPos;
        for (size_t c = 0; c < 2; ++c)
        {
            const auto& h = varvHist[c];
            const float c0 = h[1], c1 = 0.5f * (h[2] - h[0]);
            const float c2 = h[0] - 2.5f * h[1] + 2.0f * h[2] - 0.5f * h[3];
            const float c3 = 0.5f * (h[3] - h[0]) + 1.5f * (h[1] - h[2]);
            (c == 0 ? left : right)[i] = juce::jlimit (-1.0f, 1.0f, ((c3 * t + c2) * t + c1) * t + c0);
        }
    }
    varvPos = juce::jlimit (0.0, 0.999999, varvPos);
}

juce::AudioProcessorEditor* KroppsmaskinenProcessor::createEditor()
{
    return new KroppsmaskinenEditor (*this);
}

void KroppsmaskinenProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto copy = state.copyState();
    km::PresetManager::writePatterns (copy, banker);
    copy.removeChild (copy.getChildWithName (panelLage.getType()), nullptr);
    copy.appendChild (panelLage.createCopy(), nullptr);
    if (auto xml = copy.createXml())
        copyXmlToBinary (*xml, destData);
}

void KroppsmaskinenProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto panel = tree.getChildWithName (panelLage.getType());
            if (panel.isValid())
            {
                panelLage.copyPropertiesFrom (panel, nullptr);
                tree.removeChild (panel, nullptr);
            }
            if (km::PresetManager::readPatterns (tree, banker))
                state.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KroppsmaskinenProcessor();
}
