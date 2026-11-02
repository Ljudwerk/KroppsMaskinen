// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"
#include "ui/ValueFormat.h"

namespace
{
    const juce::Colour bakgrund (0xff202020), sektion (0xff2b2b2b), text (0xffd8d8d8), rod (0xffe03020);
    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    // Sektionernas ytor, delas av paint() och resized().
    struct Ytor
    {
        juce::Rectangle<int> topp, takt, mix, kolv, stamp, rutnat, keyboard;
        explicit Ytor (juce::Rectangle<int> r)
        {
            r.reduce (10, 10);
            topp = r.removeFromTop (40);
            r.removeFromTop (8);
            auto rad1 = r.removeFromTop (130);
            takt = rad1.removeFromLeft (470);
            rad1.removeFromLeft (8);
            mix = rad1;
            r.removeFromTop (8);
            auto rad2 = r.removeFromTop (220);
            kolv = rad2.removeFromLeft (650);
            rad2.removeFromLeft (8);
            stamp = rad2;
            r.removeFromTop (8);
            keyboard = r.removeFromBottom (70);
            r.removeFromBottom (8);
            rutnat = r;
        }
    };

    juce::Rectangle<int> innehall (juce::Rectangle<int> s) { return s.withTrimmedTop (22).reduced (6, 2); }
}

KroppsmaskinenEditor::KroppsmaskinenEditor (KroppsmaskinenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), rutnat (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace km::params;
    getLookAndFeel().setColour (juce::Slider::rotarySliderFillColourId, rod);
    getLookAndFeel().setColour (juce::Slider::thumbColourId, text);
    getLookAndFeel().setColour (juce::TextButton::buttonOnColourId, rod);

    for (auto [pid, namn] : std::initializer_list<std::pair<const char*, const char*>> {
             { id::tempo, "Tempo" }, { id::gung, "Gung" }, { id::grundton, "Grundton" },
             { id::trumLangd, "Trumlängd" }, { id::basLangd, "Baslängd" } })
        taktRattar.push_back (&ratt (pid, u8 (namn)));

    mixRattar.push_back (&ratt (id::basNiva, "Kolvbasen"));
    mixRattar.push_back (&ratt (id::trumNiva, "Stampverket"));
    mixRattar.push_back (&ratt (id::master, "Master"));

    for (auto [pid, namn] : std::initializer_list<std::pair<const char*, const char*>> {
             { id::kbVag, "Vågform" }, { id::kbPulsbredd, "Pulsbredd" }, { id::kbSub, "Sub" }, { id::kbDrive, "Drive" },
             { id::kbGlid, "Glid" }, { id::kbAccent, "Accent" }, { id::kbCutoff, "Brytfrekvens" },
             { id::kbResonans, "Resonans" }, { id::kbEnvMangd, "Env-mängd" }, { id::kbFilterFall, "Filterfall" },
             { id::kbFall, "Fall" }, { id::kbHall, "Håll" }, { id::kbSlapp, "Släpp" } })
        kolvRattar.push_back (&ratt (pid, u8 (namn)));

    for (int d = 0; d < km::antalByggda; ++d)
    {
        stampRattar.push_back (&ratt (id::trumma (d, "stamning"), d == 0 ? u8 ("Stämning") : juce::String()));
        stampRattar.push_back (&ratt (id::trumma (d, "klang"), d == 0 ? "Klang" : ""));
        stampRattar.push_back (&ratt (id::trumma (d, "niva"), d == 0 ? u8 ("Nivå") : juce::String()));
    }

    spela.setButtonText ("SPELA");
    spela.setClickingTogglesState (true);
    spelaKoppling = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.state, id::spela, spela);
    addAndMakeVisible (spela);

    addAndMakeVisible (presetLista);
    presetLista.onChange = [this] { if (presetLista.getSelectedItemIndex() >= 0) proc.presets.load (presetLista.getSelectedItemIndex()); };
    forra.onClick = [this] { proc.presets.loadNext (-1); uppdateraPresets(); };
    nasta.onClick = [this] { proc.presets.loadNext (1); uppdateraPresets(); };
    spara.onClick = [this]
    {
        auto* w = new juce::AlertWindow ("Spara preset", "Namn:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("namn", proc.presets.getCurrentName());
        w->addButton ("Spara", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Avbryt", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
        {
            if (r == 1) proc.presets.saveUser (w->getTextEditorContents ("namn"));
            uppdateraPresets();
        }), true);
    };
    for (auto* b : { &forra, &nasta, &spara }) addAndMakeVisible (*b);
    uppdateraPresets();

    addAndMakeVisible (rutnat);
    keyboard.setAvailableRange (24, 84);
    keyboard.setLowestVisibleKey (24);
    keyboard.setKeyWidth (16.0f);
    keyboard.setOctaveForMiddleC (4);
    addAndMakeVisible (keyboard);

    setSize (bredd, hojd);
    startTimerHz (20);
}

KroppsmaskinenEditor::~KroppsmaskinenEditor()
{
    for (auto& r : allaRattar) r->koppling.reset();
}

KroppsmaskinenEditor::Ratt& KroppsmaskinenEditor::ratt (const juce::String& paramId, const juce::String& etikett)
{
    auto r = std::make_unique<Ratt>();
    r->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    r->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    r->koppling = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.state, paramId, r->slider);
    if (auto* param = proc.state.getParameter (paramId))
        r->slider.textFromValueFunction = [param] (double v) { return km::ui::formatValue (*param, (float) v); };
    r->slider.updateText();
    r->namn.setText (etikett, juce::dontSendNotification);
    r->namn.setJustificationType (juce::Justification::centred);
    r->namn.setFont (juce::FontOptions (13.0f));
    r->namn.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (r->slider);
    addAndMakeVisible (r->namn);
    allaRattar.push_back (std::move (r));
    return *allaRattar.back();
}

void KroppsmaskinenEditor::layoutRattar (juce::Rectangle<int> yta, std::vector<Ratt*>& rattar, int perRad)
{
    const int rader = ((int) rattar.size() + perRad - 1) / perRad;
    const int h = yta.getHeight() / juce::jmax (1, rader);
    const int w = yta.getWidth() / perRad;
    for (int i = 0; i < (int) rattar.size(); ++i)
    {
        juce::Rectangle<int> cell (yta.getX() + (i % perRad) * w, yta.getY() + (i / perRad) * h, w, h);
        rattar[(size_t) i]->namn.setBounds (cell.removeFromTop (16));
        rattar[(size_t) i]->slider.setBounds (cell.reduced (2));
    }
}

void KroppsmaskinenEditor::paint (juce::Graphics& g)
{
    g.fillAll (bakgrund);
    const Ytor y (getLocalBounds());

    g.setColour (text);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("KROPPSMASKINEN", y.topp.withWidth (260), juce::Justification::centredLeft);
    g.setColour (rod);
    g.drawText ("KM-87", y.topp.withX (y.topp.getX() + 250).withWidth (100), juce::Justification::centredLeft);

    auto sektionRuta = [&] (juce::Rectangle<int> r, const juce::String& rubrik)
    {
        g.setColour (sektion);
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (rubrik, r.reduced (8, 3).removeFromTop (18), juce::Justification::centredLeft);
    };
    sektionRuta (y.takt, "TAKTVERKET");
    sektionRuta (y.mix, "MIXERN");
    sektionRuta (y.kolv, "KOLVBASEN");
    sektionRuta (y.stamp, "STAMPVERKET");

    // Trumnamnen framför Stampverkets tre rader.
    auto s = innehall (y.stamp);
    const int rad = s.getHeight() / km::antalByggda;
    for (int d = 0; d < km::antalByggda; ++d)
    {
        g.setColour (rod);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (u8 (km::trumNamn (km::byggdaTrummor[(size_t) d])).toUpperCase(),
                    s.getX(), s.getY() + d * rad, 80, rad, juce::Justification::centredLeft);
    }

    // Toppmätare i mixern.
    auto m = innehall (y.mix).removeFromRight (18);
    g.setColour (juce::Colours::black);
    g.fillRect (m);
    const float niva = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (visadTopp, -60.0f) + 60.0f) / 60.0f);
    g.setColour (visadTopp > 0.95f ? rod : juce::Colour (0xff60c060));
    g.fillRect (m.withTop (m.getBottom() - (int) (niva * (float) m.getHeight())));

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (u8 ("Klick: av/slag/accent · Skift: ratchet · Alt: chans · Kolvbasen: dra/rulla = ton, högerklick = glid, Alt = oktav"),
                y.rutnat.withTop (y.rutnat.getBottom() - 16), juce::Justification::centredLeft);
}

void KroppsmaskinenEditor::resized()
{
    const Ytor y (getLocalBounds());
    auto topp = y.topp;
    topp.removeFromLeft (360);
    spela.setBounds (topp.removeFromLeft (100).reduced (0, 4));
    topp.removeFromLeft (20);
    forra.setBounds (topp.removeFromLeft (32).reduced (0, 4));
    presetLista.setBounds (topp.removeFromLeft (260).reduced (4, 4));
    nasta.setBounds (topp.removeFromLeft (32).reduced (0, 4));
    topp.removeFromLeft (8);
    spara.setBounds (topp.removeFromLeft (70).reduced (0, 4));

    layoutRattar (innehall (y.takt), taktRattar, 5);
    layoutRattar (innehall (y.mix).withTrimmedRight (26), mixRattar, 3);
    layoutRattar (innehall (y.kolv), kolvRattar, 7);

    // Stampverket: en kolumn per trumma med stämning, klang och nivå.
    auto s = innehall (y.stamp).withTrimmedLeft (80);
    const int rad = s.getHeight() / km::antalByggda;
    for (int d = 0; d < km::antalByggda; ++d)
        for (int i = 0; i < 3; ++i)
        {
            auto* r = stampRattar[(size_t) (d * 3 + i)];
            juce::Rectangle<int> cell (s.getX() + i * s.getWidth() / 3, s.getY() + d * rad, s.getWidth() / 3, rad);
            r->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            r->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 18);
            r->namn.setBounds (cell.removeFromTop (14));
            r->slider.setBounds (cell);
        }

    rutnat.setBounds (y.rutnat.withTrimmedBottom (18));
    keyboard.setBounds (y.keyboard);
    keyboard.setKeyWidth ((float) y.keyboard.getWidth() / 36.0f);   // C1–C6: 36 vita tangenter
}

void KroppsmaskinenEditor::timerCallback()
{
    const float topp = proc.outputPeak.exchange (0.0f);
    visadTopp = juce::jmax (topp, visadTopp * 0.85f);
    repaint (Ytor (getLocalBounds()).mix);
    if (presetLista.getText() != proc.presets.getCurrentName()) uppdateraPresets();
}

void KroppsmaskinenEditor::uppdateraPresets()
{
    presetLista.clear (juce::dontSendNotification);
    juce::String kategori;
    const auto& e = proc.presets.getEntries();
    for (int i = 0; i < (int) e.size(); ++i)
    {
        if (e[(size_t) i].category != kategori) { kategori = e[(size_t) i].category; presetLista.addSectionHeading (kategori); }
        presetLista.addItem (e[(size_t) i].name, i + 1);
    }
    presetLista.setSelectedId (proc.presets.getCurrentIndex() + 1, juce::dontSendNotification);
}
