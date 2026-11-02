// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

KroppsmaskinenEditor::KroppsmaskinenEditor (KroppsmaskinenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), panel (p)
{
    setOpaque (true);
    addAndMakeVisible (panel);
    panel.skalaAndrad = [this] (float s) { sattSkala (s); };
    sattSkala ((float) proc.panelLage.getProperty ("skala", standardSkala));
}

void KroppsmaskinenEditor::sattSkala (float s)
{
    s = juce::jlimit (0.6f, 1.6f, s);
    panel.setTransform (juce::AffineTransform::scale (s));
    setSize (juce::roundToInt ((float) km::vhs::Panel::designBredd * s), juce::roundToInt ((float) km::vhs::Panel::designHojd * s));
}
