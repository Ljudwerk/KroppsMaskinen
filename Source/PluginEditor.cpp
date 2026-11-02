// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"
#include "ui/ValueFormat.h"

using namespace km;
using namespace km::params;

namespace
{
    const juce::Colour bakgrund (0xff202020), sektion (0xff2b2b2b), text (0xffd8d8d8), rod (0xffe03020);
    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    // Versaler, även å, ä och ö (toUpperCase klarar dem inte på alla system).
    juce::String versaler (const char* s)
    {
        return u8 (s).toUpperCase().replace (u8 ("å"), u8 ("Å")).replace (u8 ("ä"), u8 ("Ä")).replace (u8 ("ö"), u8 ("Ö"));
    }

    // Sektionernas ytor, delas av paint() och resized().
    struct Ytor
    {
        juce::Rectangle<int> topp, takt, mixer, flikar, ljud, rutnat, keyboard;
        explicit Ytor (juce::Rectangle<int> r)
        {
            r.reduce (10, 10);
            topp = r.removeFromTop (40);
            r.removeFromTop (8);
            auto rad1 = r.removeFromTop (210);
            takt = rad1.removeFromLeft (330);
            rad1.removeFromLeft (8);
            mixer = rad1;
            r.removeFromTop (8);
            flikar = r.removeFromTop (30);
            r.removeFromTop (4);
            ljud = r.removeFromTop (250);
            r.removeFromTop (8);
            keyboard = r.removeFromBottom (60);
            r.removeFromBottom (6);
            rutnat = r;
        }
    };

    juce::Rectangle<int> innehall (juce::Rectangle<int> s) { return s.withTrimmedTop (22).reduced (6, 2); }
}

KroppsmaskinenEditor::KroppsmaskinenEditor (KroppsmaskinenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), rutnat (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    getLookAndFeel().setColour (juce::Slider::rotarySliderFillColourId, rod);
    getLookAndFeel().setColour (juce::Slider::thumbColourId, text);
    getLookAndFeel().setColour (juce::TextButton::buttonOnColourId, rod);
    getLookAndFeel().setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff3a3a3a));

    // --- Taktverket, Varvtalet och Spänningen
    takt.push_back (&ratt (id::tempo, "Tempo"));
    takt.push_back (&ratt (id::gung, "Gung"));
    takt.push_back (&ratt (id::grundton, "Grundton"));
    takt.push_back (&ratt (id::master, "Master"));
    takt.push_back (&ratt (id::spanning, u8 ("Spänningen")));
    takt.push_back (&lista (id::varvtal, "Varvtalet"));
    takt.push_back (&lista (id::varvLage, u8 ("Varvtalets läge")));

    // --- Mixern: en remsa per kanal
    for (int k = 0; k < antalKanaler; ++k)
    {
        auto& m = mixer[(size_t) k];
        m.push_back (&lista (id::kanal (k, "monster"), u8 ("Mönster")));
        m.push_back (&lista (id::kanal (k, "variant"), "Variant"));
        m.push_back (&ratt (id::kanal (k, "langd"), u8 ("Längd"), true));
        m.push_back (&lista (id::kanal (k, "drivetLage"), "Drivet"));
        m.push_back (&ratt (id::kanal (k, "drivetMangd"), u8 ("Mängd"), true));
        m.push_back (&ratt (id::kanal (k, "niva"), u8 ("Nivå"), true));
        m.push_back (&ratt (id::kanal (k, "pan"), "Pan", true));
        m.push_back (&ratt (id::kanal (k, "sandA"), u8 ("→ Grindrum"), true));
        m.push_back (&ratt (id::kanal (k, "sandB"), u8 ("→ Ekot"), true));
        m.push_back (&knapp (id::kanal (k, "tyst"), "Tyst"));
        m.push_back (&knapp (id::kanal (k, "solo"), "Solo"));
    }

    // --- Kolvbasen och Kolvbas 2
    for (int k = 0; k < 2; ++k)
    {
        Grupp g;
        for (auto [pid, namn] : std::initializer_list<std::pair<const char*, const char*>> {
                 { "vag", "Vågform" }, { "pulsbredd", "Pulsbredd" }, { "sub", "Sub" }, { "brus", "Brus" },
                 { "osc2Niva", "Osc 2" }, { "osc2Halvtoner", "Osc 2 stämn." }, { "osc2Fin", "Osc 2 fin" },
                 { "glid", "Glid" }, { "accent", "Accent" }, { "drive", "Filterdrive" },
                 { "cutoff", "Brytfrekvens" }, { "resonans", "Resonans" }, { "envMangd", "Env-mängd" }, { "filterFall", "Filterfall" },
                 { "fall", "Fall" }, { "hall", "Håll" }, { "slapp", "Släpp" } })
            g.kontroller.push_back (&ratt (id::kolv (k, pid), u8 (namn)));
        g.kontroller.push_back (&knapp (id::kolv (k, "synk"), "Synk"));
        g.kontroller.push_back (&lista (id::kolv (k, "filterTyp"), "Filter"));
        g.perRad = 10;
        if (k == 1)
        {
            g.kontroller.push_back (&knapp (id::folj, u8 ("Följ Kolvbasen")));
            g.kontroller.push_back (&ratt (id::forskjutning, u8 ("Förskjutning")));
            g.kontroller.push_back (&ratt (id::transponering, "Transponering"));
            g.perRad = 11;
        }
        flikGrupper[(size_t) k].push_back (std::move (g));
    }

    // --- Stampverket: kit överst, sedan en kolumn per spår
    {
        Grupp g;
        g.kontroller.push_back (&lista (id::kit, "Kit"));
        g.kontroller.push_back (&knapp (id::kantslag, "Kantslag"));
        flikGrupper[(size_t) Flik::Stampverket].push_back (std::move (g));
        for (int s = 0; s < antalSpar; ++s)
        {
            auto& v = stampSpar[(size_t) s];
            v.push_back (&ratt (id::trumma (s, "stamning"), {}, true));
            v.push_back (&ratt (id::trumma (s, "klang"), {}, true));
            v.push_back (&ratt (id::trumma (s, "niva"), {}, true));
            v.push_back (&knapp (id::trumma (s, "tyst"), "Tyst"));
        }
    }

    // --- Hugget
    {
        Grupp g;
        g.kontroller = { &lista (id::hu ("lage"), u8 ("Läge")), &lista (id::hu ("ackord"), "Ackord"), &ratt (id::hu ("oktav"), "Oktav"),
                         &ratt (id::hu ("klang"), "Klang"), &ratt (id::hu ("sankning"), u8 ("Sänkning")),
                         &ratt (id::hu ("attack"), "Attack"), &ratt (id::hu ("fall"), "Fall"), &ratt (id::hu ("hall"), u8 ("Håll")),
                         &ratt (id::hu ("slapp"), u8 ("Släpp")), &ratt (id::hu ("bredd"), "Bredd") };
        g.perRad = 5;
        flikGrupper[(size_t) Flik::Hugget].push_back (std::move (g));
    }

    // --- Effekterna
    {
        auto& v = flikGrupper[(size_t) Flik::Effekter];
        v.push_back ({ "GRINDRUMMET", { &ratt (id::fx ("gr", "storlek"), "Storlek"), &ratt (id::fx ("gr", "hallTid"), u8 ("Hålltid")),
                                        &ratt (id::fx ("gr", "ton"), "Ton"), &ratt (id::fx ("gr", "niva"), u8 ("Nivå")) }, 0.22f, 2, {} });
        v.push_back ({ "EKOT", { &lista (id::fx ("eko", "tid"), "Tid"), &ratt (id::fx ("eko", "aterkoppling"), u8 ("Återkoppling")),
                                 &ratt (id::fx ("eko", "ton"), "Ton"), &lista (id::fx ("eko", "lage"), u8 ("Läge")),
                                 &knapp (id::fx ("eko", "pingpong"), "Ping-pong"), &ratt (id::fx ("eko", "niva"), u8 ("Nivå")) }, 0.3f, 3, {} });
        v.push_back ({ "SVAJET", { &lista (id::fx ("svaj", "lage"), u8 ("Läge")), &ratt (id::fx ("svaj", "mangd"), u8 ("Mängd")) }, 0.14f, 1, {} });
        v.push_back ({ u8 ("KLÄMMAN"), { &ratt (id::fx ("klam", "troskel"), u8 ("Tröskel")), &ratt (id::fx ("klam", "kvot"), "Kvot"),
                                         &ratt (id::fx ("klam", "attack"), "Attack"), &ratt (id::fx ("klam", "slapp"), u8 ("Släpp")),
                                         &ratt (id::fx ("klam", "pump"), "Pump"), &ratt (id::fx ("klam", "efter"), "Efter") }, 0.34f, 3, {} });
    }

    // --- Modulationen
    {
        auto& v = flikGrupper[(size_t) Flik::Modulation];
        for (int n = 0; n < 2; ++n)
            v.push_back ({ "LFO " + juce::String (n + 1), { &lista (id::lfo (n, "form"), "Form"), &ratt (id::lfo (n, "hz"), "Hastighet", true),
                                                           &lista (id::lfo (n, "synk"), "Synk") }, 0.27f, 3, {} });
        for (int n = 0; n < 2; ++n)
            v.push_back ({ "ENV " + juce::String (n + 1), { &ratt (id::menv (n, "attack"), "Attack", true), &ratt (id::menv (n, "fall"), "Fall", true),
                                                           &ratt (id::menv (n, "hall"), u8 ("Håll"), true), &ratt (id::menv (n, "slapp"), u8 ("Släpp"), true),
                                                           &lista (id::menv (n, "utlosare"), u8 ("Utlöses av")) }, 0.27f, 5, {} });
        Grupp k { "KOPPLINGAR", {}, 0.46f, 6, {} };
        for (int n = 0; n < antalKopplingar; ++n)
        {
            k.kontroller.push_back (&lista (id::mod (n, "kalla"), n < 2 ? u8 ("Källa") : juce::String()));
            k.kontroller.push_back (&lista (id::mod (n, "mal"), n < 2 ? u8 ("Mål") : juce::String()));
            k.kontroller.push_back (&ratt (id::mod (n, "mangd"), n < 2 ? u8 ("Mängd") : juce::String(), true));
        }
        v.push_back (std::move (k));
    }

    // --- Toppen
    spela.setButtonText ("SPELA");
    spela.setClickingTogglesState (true);
    spelaKoppling = std::make_unique<APVTS::ButtonAttachment> (proc.state, id::spela, spela);
    addAndMakeVisible (spela);

    addAndMakeVisible (presetLista);
    presetLista.onChange = [this] { if (presetLista.getSelectedItemIndex() >= 0) proc.presets.load (presetLista.getSelectedId() - 1); };
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

    static const char* flikNamn[] = { "KOLVBASEN", "KOLVBAS 2", "STAMPVERKET", "HUGGET", "EFFEKTER", "MODULATION" };
    for (int f = 0; f < antalFlikar; ++f)
    {
        auto& b = flikar[(size_t) f];
        b.setButtonText (flikNamn[f]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (8710);
        b.onClick = [this, f] { if (flikar[(size_t) f].getToggleState()) valjFlik ((Flik) f); };
        addAndMakeVisible (b);
    }
    flikar[0].setToggleState (true, juce::dontSendNotification);

    addAndMakeVisible (rutnat);
    keyboard.setAvailableRange (24, 84);
    keyboard.setLowestVisibleKey (24);
    keyboard.setOctaveForMiddleC (4);
    addAndMakeVisible (keyboard);

    setSize (bredd, hojd);
    valjFlik (Flik::Kolvbasen);
    startTimerHz (20);
}

KroppsmaskinenEditor::~KroppsmaskinenEditor()
{
    for (auto& k : alla) { k->ratt.reset(); k->knapp.reset(); k->lista.reset(); }
}

KroppsmaskinenEditor::Kontroll& KroppsmaskinenEditor::ratt (const juce::String& paramId, const juce::String& etikett, bool liten)
{
    auto k = std::make_unique<Kontroll>();
    auto* s = new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, liten ? 62 : 76, liten ? 15 : 17);
    k->komponent.reset (s);
    k->ratt = std::make_unique<APVTS::SliderAttachment> (proc.state, paramId, *s);
    if (auto* param = proc.state.getParameter (paramId))
        s->textFromValueFunction = [param] (double v) { return km::ui::formatValue (*param, (float) v); };
    s->updateText();
    k->namn.setText (etikett, juce::dontSendNotification);
    k->namn.setJustificationType (juce::Justification::centred);
    k->namn.setFont (juce::FontOptions (liten ? 12.0f : 13.0f));
    k->namn.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (*k->komponent);
    addAndMakeVisible (k->namn);
    alla.push_back (std::move (k));
    return *alla.back();
}

KroppsmaskinenEditor::Kontroll& KroppsmaskinenEditor::knapp (const juce::String& paramId, const juce::String& etikett)
{
    auto k = std::make_unique<Kontroll>();
    auto* b = new juce::TextButton (etikett);
    b->setClickingTogglesState (true);
    k->komponent.reset (b);
    k->knapp = std::make_unique<APVTS::ButtonAttachment> (proc.state, paramId, *b);
    addAndMakeVisible (*b);
    addChildComponent (k->namn);
    alla.push_back (std::move (k));
    return *alla.back();
}

KroppsmaskinenEditor::Kontroll& KroppsmaskinenEditor::lista (const juce::String& paramId, const juce::String& etikett)
{
    auto k = std::make_unique<Kontroll>();
    auto* c = new juce::ComboBox();
    if (auto* p = proc.state.getParameter (paramId))
    {
        if (auto* val = dynamic_cast<juce::AudioParameterChoice*> (p)) c->addItemList (val->choices, 1);
        else if (auto* heltal = dynamic_cast<juce::AudioParameterInt*> (p))
            for (int v = heltal->getRange().getStart(); v <= heltal->getRange().getEnd(); ++v)
                c->addItem (juce::String (v), v - heltal->getRange().getStart() + 1);
    }
    k->komponent.reset (c);
    k->lista = std::make_unique<APVTS::ComboBoxAttachment> (proc.state, paramId, *c);
    k->namn.setText (etikett, juce::dontSendNotification);
    k->namn.setJustificationType (juce::Justification::centred);
    k->namn.setFont (juce::FontOptions (12.0f));
    k->namn.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (*c);
    addAndMakeVisible (k->namn);
    alla.push_back (std::move (k));
    return *alla.back();
}

// Lägger ut kontroller i ett rutnät: namn överst i varje cell, kontrollen under (knappar och listor centreras).
void KroppsmaskinenEditor::rutnatAv (juce::Rectangle<int> yta, const std::vector<Kontroll*>& kontroller, int perRad, int namnHojd)
{
    const int rader = ((int) kontroller.size() + perRad - 1) / perRad;
    const int h = yta.getHeight() / juce::jmax (1, rader);
    const int w = yta.getWidth() / perRad;
    for (int i = 0; i < (int) kontroller.size(); ++i)
    {
        juce::Rectangle<int> cell (yta.getX() + (i % perRad) * w, yta.getY() + (i / perRad) * h, w, h);
        auto* k = kontroller[(size_t) i];
        k->namn.setBounds (cell.removeFromTop (namnHojd));
        if (k->ratt != nullptr) k->komponent->setBounds (cell.reduced (2, 0));
        else k->komponent->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 6, 140), 24));
    }
}

void KroppsmaskinenEditor::valjFlik (Flik f)
{
    vald = f;
    auto visa = [] (const std::vector<Kontroll*>& v, bool syns)
    {
        for (auto* c : v)
        {
            c->komponent->setVisible (syns);
            c->namn.setVisible (syns && c->knapp == nullptr);
        }
    };
    for (int i = 0; i < antalFlikar; ++i)
        for (auto& g : flikGrupper[(size_t) i]) visa (g.kontroller, (int) f == i);
    for (auto& s : stampSpar) visa (s, f == Flik::Stampverket);
    // Rutnätet visar den valda kanalen; under Effekter och Modulation visas den senast valda kanalen.
    if ((int) f < antalKanaler) rutnat.setKanal ((Kanal) (int) f);
    resized();
    repaint();
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
    sektionRuta (y.mixer, "MIXERN");
    static const char* rubriker[] = { "KOLVBASEN", "KOLVBAS 2", "STAMPVERKET", "HUGGET", "EFFEKTER", "MODULATION" };
    sektionRuta (y.ljud, rubriker[(int) vald]);

    // Gruppernas rubriker.
    for (auto& grupp : flikGrupper[(size_t) vald])
        if (grupp.rubrik.isNotEmpty())
        {
            g.setColour (rod);
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (grupp.rubrik, grupp.yta.withHeight (14), juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff3a3a3a));
            g.drawRect (grupp.yta.withTrimmedTop (16).expanded (2, 0), 1);
        }

    // Klämmans reduktion.
    if (vald == Flik::Effekter && ! flikGrupper[(size_t) Flik::Effekter].empty())
    {
        auto r = flikGrupper[(size_t) Flik::Effekter].back().yta;
        auto stapel = juce::Rectangle<int> (r.getRight() - 12, r.getY() + 20, 10, r.getHeight() - 24);
        g.setColour (juce::Colours::black);
        g.fillRect (stapel);
        g.setColour (rod);
        g.fillRect (stapel.withHeight ((int) (juce::jlimit (0.0f, 1.0f, visadReduktion / 18.0f) * (float) stapel.getHeight())));
    }

    // Kanalnamnen framför mixerns remsor.
    auto m = innehall (y.mixer).withTrimmedRight (26);
    const int remsa = m.getHeight() / antalKanaler;
    for (int k = 0; k < antalKanaler; ++k)
    {
        g.setColour (rod);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (versaler (kanalNamn ((Kanal) k)), m.getX(), m.getY() + k * remsa + 8, 100, remsa - 8, juce::Justification::centredLeft);
    }

    // Stampverket: spårnamnen ovanför kolumnerna.
    if (vald == Flik::Stampverket)
    {
        auto s = innehall (y.ljud).withTrimmedTop (30);
        const int kolumn = s.getWidth() / antalSpar;
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        for (int t = 0; t < antalSpar; ++t)
        {
            g.setColour (rod);
            g.drawText (versaler (trumNamn ((Trumma) t)), s.getX() + t * kolumn, s.getY(), kolumn, 14, juce::Justification::centred);
        }
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (u8 ("Varje spår uppifrån: stämning, klang och nivå"), innehall (y.ljud).withHeight (26).withTrimmedLeft (310),
                    juce::Justification::centredLeft);
    }

    // Toppmätare.
    auto mat = innehall (y.mixer).removeFromRight (18);
    g.setColour (juce::Colours::black);
    g.fillRect (mat);
    const float niva = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (visadTopp, -60.0f) + 60.0f) / 60.0f);
    g.setColour (visadTopp > 0.95f ? rod : juce::Colour (0xff60c060));
    g.fillRect (mat.withTop (mat.getBottom() - (int) (niva * (float) mat.getHeight())));

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (u8 ("Klick: av/slag/accent · Skift: ratchet/tonlängd · Alt: chans/oktav · Toner: dra/rulla = ton, högerklick = glid"),
                y.rutnat.withTop (y.rutnat.getBottom() - 16), juce::Justification::centredLeft);

    // Diagnostiken till höger på samma rad; röd när ljudkortet inte går i den takt det uppger.
    g.setColour (diagnostikVarning ? rod : juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f, diagnostikVarning ? juce::Font::bold : juce::Font::plain));
    g.drawText (diagnostik, y.rutnat.withTop (y.rutnat.getBottom() - 16), juce::Justification::centredRight);
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

    rutnatAv (innehall (y.takt), takt, 4);

    // Mixern: en remsa per kanal.
    auto m = innehall (y.mixer).withTrimmedRight (26).withTrimmedLeft (100);
    const int remsa = m.getHeight() / antalKanaler;
    for (int k = 0; k < antalKanaler; ++k)
    {
        auto rad = m.withY (m.getY() + k * remsa).withHeight (remsa);
        auto& v = mixer[(size_t) k];
        const int w = rad.getWidth() / (int) v.size();
        for (int i = 0; i < (int) v.size(); ++i)
        {
            auto cell = rad.withX (rad.getX() + i * w).withWidth (w);
            auto* c = v[(size_t) i];
            c->namn.setBounds (cell.removeFromTop (13));
            if (c->ratt != nullptr)
            {
                auto* s = static_cast<juce::Slider*> (c->komponent.get());
                s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 42, 15);
                s->setScrollWheelEnabled (false);   // de små rattarna ändras annars lätt av misstag när man scrollar
                c->komponent->setBounds (cell);
            }
            else c->komponent->setBounds (cell.withSizeKeepingCentre (w - 6, 22));
            if (k > 0) c->namn.setText ({}, juce::dontSendNotification);
        }
    }

    for (int f = 0; f < antalFlikar; ++f)
        flikar[(size_t) f].setBounds (y.flikar.withX (y.flikar.getX() + f * 156).withWidth (150));

    const auto ljud = innehall (y.ljud);

    // Kanalflikarna och Hugget: en grupp över hela panelen.
    for (auto f : { Flik::Kolvbasen, Flik::Kolvbas2, Flik::Hugget })
        for (auto& g : flikGrupper[(size_t) f])
        {
            g.yta = ljud;
            rutnatAv (ljud, g.kontroller, g.perRad);
        }

    // Effekterna: grupperna bredvid varandra.
    {
        auto r = ljud;
        const int total = r.getWidth();
        for (auto& g : flikGrupper[(size_t) Flik::Effekter])
        {
            g.yta = r.removeFromLeft ((int) (g.bredd * (float) total)).reduced (6, 0);
            rutnatAv (g.yta.withTrimmedTop (18).withTrimmedRight (16), g.kontroller, g.perRad);
        }
    }

    // Modulationen: LFO:er överst och envelopes underst till vänster, kopplingarna till höger.
    {
        auto& v = flikGrupper[(size_t) Flik::Modulation];
        auto r = ljud;
        auto vanster = r.removeFromLeft ((int) (r.getWidth() * 0.54f));
        auto ovre = vanster.removeFromTop (vanster.getHeight() / 2);
        v[0].yta = ovre.removeFromLeft (ovre.getWidth() / 2).reduced (6, 0);
        v[1].yta = ovre.reduced (6, 0);
        v[2].yta = vanster.removeFromLeft (vanster.getWidth() / 2).reduced (6, 0);
        v[3].yta = vanster.reduced (6, 0);
        v[4].yta = r.reduced (6, 0);
        for (auto& g : v)
            rutnatAv (g.yta.withTrimmedTop (18), g.kontroller, g.perRad, &g == &v[4] ? 13 : 14);
    }

    // Stampverket: kit överst, sedan en kolumn per spår.
    {
        auto& h = flikGrupper[(size_t) Flik::Stampverket][0];
        auto huvud = ljud.withHeight (26);
        h.yta = {};
        h.kontroller[0]->namn.setBounds (huvud.withWidth (40));
        h.kontroller[0]->komponent->setBounds (huvud.withX (huvud.getX() + 40).withWidth (140));
        h.kontroller[1]->komponent->setBounds (huvud.withX (huvud.getX() + 200).withWidth (90));
        auto s = ljud.withTrimmedTop (46);
        const int kolumn = s.getWidth() / antalSpar;
        for (int t = 0; t < antalSpar; ++t)
        {
            auto kol = s.withX (s.getX() + t * kolumn).withWidth (kolumn);
            auto& v = stampSpar[(size_t) t];
            v[3]->komponent->setBounds (kol.removeFromBottom (22).reduced (6, 0));
            const int hjd = kol.getHeight() / 3;
            for (int i = 0; i < 3; ++i)
            {
                v[(size_t) i]->namn.setBounds ({});
                v[(size_t) i]->komponent->setBounds (kol.withY (kol.getY() + i * hjd).withHeight (hjd));
            }
        }
    }

    rutnat.setBounds (y.rutnat.withTrimmedBottom (18));
    keyboard.setBounds (y.keyboard);
    keyboard.setKeyWidth ((float) y.keyboard.getWidth() / 36.0f);   // C1–C6: 36 vita tangenter
}

void KroppsmaskinenEditor::timerCallback()
{
    const float topp = proc.outputPeak.exchange (0.0f);
    visadTopp = juce::jmax (topp, visadTopp * 0.85f);
    visadReduktion = juce::jmax (proc.klamReduktionDb(), visadReduktion * 0.85f);
    repaint (Ytor (getLocalBounds()).mixer);
    if (vald == Flik::Effekter) repaint (Ytor (getLocalBounds()).ljud);
    if (presetLista.getText() != proc.presets.getCurrentName()) uppdateraPresets();

    // Var tredje sekund: uppmätt tempo och hur snabbt ljudet behandlas jämfört med realtid.
    const double nu = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (nu - matStart >= 3.0)
    {
        const auto steg = proc.spelade(), sampel = proc.antalSampel.load();
        const double sr = proc.forberedd.load(), dt = nu - matStart;
        if (matStart > 0.0 && sr > 0.0)
        {
            const double bpm = (double) (steg - matSteg) / dt * 15.0;
            const double realtid = (double) (sampel - matSampel) / dt / sr;
            // OBS: text med å, ä, ö eller andra tecken utanför ASCII måste gå via u8(), annars stannar JUCE:s Debug-bygge.
            // Ett ljudkort som hämtar ljud i en annan takt än det uppger (t.ex. Bluetooth-lurar i samtalsläge,
            // 16 kHz) får allt att gå för fort eller för långsamt – både tempo och tonhöjd.
            diagnostikVarning = realtid < 0.95 || realtid > 1.05 || sr < 32000.0;
            if (diagnostikVarning)
                diagnostik = u8 ("Ljudkortet: ") + juce::String (sr / 1000.0, 1).replace (".", ",") + u8 (" kHz, men ljudet hämtas i ")
                           + juce::String (realtid, 2).replace (".", ",") + u8 ("× den takten. Välj 44,1 eller 48 kHz under Options.");
            else
                diagnostik = juce::String (sr / 1000.0, 1).replace (".", ",") + u8 (" kHz · ") + juce::String (proc.senasteBlock.load()) + u8 (" sampel · ")
                           + (steg > matSteg ? juce::String (bpm, 1).replace (".", ",") + " BPM" : u8 ("stoppad"))
                           + u8 (" · ") + juce::String (realtid, 2).replace (".", ",") + u8 ("× realtid");
            repaint (Ytor (getLocalBounds()).rutnat.withTop (Ytor (getLocalBounds()).rutnat.getBottom() - 16));
        }
        matStart = nu; matSteg = steg; matSampel = sampel;
    }
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
