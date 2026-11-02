// KMSnapshot – ritar panelen till PNG, så att gränssnittet kan granskas utan DAW.
//
//   KMSnapshot fil.png [skala]
//
// Laddar "Kolv 125", låter sekvensern gå en stund så att steglampan syns, och ritar hela panelen.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: KMSnapshot fil.png [skala]\n";
        return 1;
    }
    const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    const float skala = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;

    KroppsmaskinenProcessor proc;
    proc.forceraEgenKlocka = true;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    proc.presets.loadByName ("Kolv 125");
    if (auto* p = proc.state.getParameter (km::params::id::spela)) p->setValueNotifyingHost (1.0f);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int i = 0; i < 60; ++i) { midi.clear(); proc.processBlock (buf, midi); }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    const auto bild = editor->createComponentSnapshot (editor->getLocalBounds(), true, skala);
    fil.deleteFile();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    editor.reset();
    std::cout << "Panelen ritad till " << fil.getFullPathName() << "\n";
    return 0;
}
