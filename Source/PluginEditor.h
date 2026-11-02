// Panelen (fas 0): ett enkelt grått gränssnitt med standardrattar, bara för att kunna spela.
// VHS-panelen ersätter den i fas 3.
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

    static constexpr int bredd = 1120, hojd = 800;

private:
    // En ratt med namn under och värdet i Kroppsmaskinens format.
    struct Ratt
    {
        juce::Slider slider;
        juce::Label namn;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> koppling;
    };

    Ratt& ratt (const juce::String& paramId, const juce::String& etikett);
    void layoutRattar (juce::Rectangle<int> yta, std::vector<Ratt*>& rattar, int perRad);
    void timerCallback() override;
    void uppdateraPresets();

    KroppsmaskinenProcessor& proc;
    std::vector<std::unique_ptr<Ratt>> allaRattar;
    std::vector<Ratt*> taktRattar, mixRattar, kolvRattar, stampRattar;

    juce::TextButton spela;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> spelaKoppling;
    juce::ComboBox presetLista;
    juce::TextButton forra { juce::String::fromUTF8 ("◀") }, nasta { juce::String::fromUTF8 ("▶") }, spara { "Spara" };

    km::ui::StegRutnat rutnat;
    juce::MidiKeyboardComponent keyboard;
    float visadTopp = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenEditor)
};
