// Panelen (fas 0–2): ett enkelt grått gränssnitt med standardrattar, bara för att kunna spela.
// VHS-panelen (se Source/ui/stil/ och docs/stilprov_vhs.png) ersätter den i fas 3.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/StegRutnat.h"

class KroppsmaskinenEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KroppsmaskinenEditor (KroppsmaskinenProcessor&);
    ~KroppsmaskinenEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int bredd = 1280, hojd = 900;

    // Flikarna: de fyra kanalerna, effekterna och modulationen.
    enum class Flik { Kolvbasen = 0, Kolvbas2, Stampverket, Hugget, Effekter, Modulation };
    static constexpr int antalFlikar = 6;

    // Väljer flik (som att klicka på den).
    void visaFlik (Flik f) { flikar[(size_t) f].setToggleState (true, juce::sendNotificationSync); }
    void visaKanal (km::Kanal k) { visaFlik ((Flik) (int) k); }

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    // En kontroll med namn och koppling till en parameter: ratt, knapp eller lista.
    struct Kontroll
    {
        std::unique_ptr<juce::Component> komponent;
        juce::Label namn;
        std::unique_ptr<APVTS::SliderAttachment> ratt;
        std::unique_ptr<APVTS::ButtonAttachment> knapp;
        std::unique_ptr<APVTS::ComboBoxAttachment> lista;
    };

    // En grupp kontroller med rubrik i ljudpanelen: andelar av panelens bredd och antal per rad.
    struct Grupp
    {
        juce::String rubrik;
        std::vector<Kontroll*> kontroller;
        float bredd = 1.0f;
        int perRad = 8;
        juce::Rectangle<int> yta;
    };

    Kontroll& ratt (const juce::String& paramId, const juce::String& etikett, bool liten = false);
    Kontroll& knapp (const juce::String& paramId, const juce::String& etikett);
    Kontroll& lista (const juce::String& paramId, const juce::String& etikett);
    void rutnatAv (juce::Rectangle<int> yta, const std::vector<Kontroll*>& k, int perRad, int namnHojd = 16);
    void valjFlik (Flik f);
    void timerCallback() override;
    void uppdateraPresets();

    KroppsmaskinenProcessor& proc;
    std::vector<std::unique_ptr<Kontroll>> alla;
    std::vector<Kontroll*> takt;
    std::array<std::vector<Kontroll*>, km::antalKanaler> mixer;
    std::array<std::vector<Grupp>, antalFlikar> flikGrupper;      // vad varje flik visar i ljudpanelen
    std::array<std::vector<Kontroll*>, km::antalSpar> stampSpar;   // Stampverkets kolumner (läggs ut för sig)

    juce::TextButton spela;
    std::unique_ptr<APVTS::ButtonAttachment> spelaKoppling;
    juce::ComboBox presetLista;
    juce::TextButton forra { juce::String::fromUTF8 ("◀") }, nasta { juce::String::fromUTF8 ("▶") }, spara { "Spara" };
    std::array<juce::TextButton, antalFlikar> flikar;
    Flik vald = Flik::Kolvbasen;

    km::ui::StegRutnat rutnat;
    juce::MidiKeyboardComponent keyboard;
    float visadTopp = 0.0f, visadReduktion = 0.0f;

    // Tempomätaren: jämför spelade sextondelar och behandlade sampel med väggklockan.
    double matStart = 0.0;
    juce::int64 matSteg = 0, matSampel = 0;
    juce::String diagnostik;
    bool diagnostikVarning = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenEditor)
};
