// Editorn: VHS-panelen (Source/ui/vhs/Panel.h) i vald storlek, 60–160 % (MENY → Storlek).
//
// Panelen ritas alltid i designstorlek 1200 × 840 och skalas med en transform; storleken sparas i
// processorns panelläge och följer med projektet.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "ui/vhs/Panel.h"

class KroppsmaskinenEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KroppsmaskinenEditor (KroppsmaskinenProcessor&);

    void paint (juce::Graphics&) override {}
    void resized() override {}

    static constexpr float standardSkala = 0.85f;
    void sattSkala (float s);
    km::vhs::Panel& getPanel() { return panel; }

private:
    KroppsmaskinenProcessor& proc;
    km::vhs::Panel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KroppsmaskinenEditor)
};
