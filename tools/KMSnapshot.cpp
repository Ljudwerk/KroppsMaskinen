// KMSnapshot – ritar panelen till PNG, så att gränssnittet kan granskas utan DAW.
//
//   KMSnapshot fil.png [skala]
//   KMSnapshot fil.png [skala] [flik 1–6] [preset]   (det gråa gränssnittet; flik 1–4 = kanalerna, 5 = effekter, 6 = modulation)
//   KMSnapshot --stilprov fil.png [skala]            (VHS-panelens stilprov, fasta exempelvärden)
//   KMSnapshot --stilprov-detaljer fil.png [skala]   (närbilder: ratt, stegknapp, display, typsnitt, skärm)
//
// Laddar "Kolv 125", låter sekvensern gå en stund så att steglampan syns, och ritar hela panelen.
// Stilproven använder varken processorn eller parametrarna.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/stil/Stilprov.h"

// Skriver en komponent till PNG i given skala.
static int sparaPNG (juce::Component& c, const juce::File& fil, float skala, const char* vad)
{
    const auto bild = c.createComponentSnapshot (c.getLocalBounds(), true, skala);
    fil.deleteFile();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    std::cout << vad << " ritat till " << fil.getFullPathName() << "\n";
    return 0;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: KMSnapshot [--stilprov | --stilprov-detaljer] fil.png [skala]\n";
        return 1;
    }
    const juce::String lage (argv[1]);
    if (lage == "--stilprov" || lage == "--stilprov-detaljer")
    {
        if (argc < 3)
        {
            std::cout << "Användning: KMSnapshot " << lage << " fil.png [skala]\n";
            return 1;
        }
        const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        const float skala = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;
        if (lage == "--stilprov")
        {
            km::stil::prov::Panel panel;
            return sparaPNG (panel, fil, skala, "Stilprovet");
        }
        km::stil::prov::Detaljer detaljer;
        return sparaPNG (detaljer, fil, skala, "Detaljarket");
    }
    // Det gråa gränssnittet: KMSnapshot fil.png [skala] [flik 1–6] [preset]
    const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    const float skala = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
    const int kanal = argc > 3 ? juce::jlimit (1, KroppsmaskinenEditor::antalFlikar, juce::String (argv[3]).getIntValue()) : 1;
    const juce::String preset = argc > 4 ? juce::String::fromUTF8 (argv[4]) : juce::String ("Kolv 125");

    KroppsmaskinenProcessor proc;
    proc.forceraEgenKlocka = true;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    proc.presets.loadByName (preset);
    if (auto* p = proc.state.getParameter (km::params::id::spela)) p->setValueNotifyingHost (1.0f);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int i = 0; i < 60; ++i) { midi.clear(); proc.processBlock (buf, midi); }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (auto* e = dynamic_cast<KroppsmaskinenEditor*> (editor.get())) e->visaFlik ((KroppsmaskinenEditor::Flik) (kanal - 1));
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
