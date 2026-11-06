// PresetManager – fabriksbandet och egna presets.
// Egna presets sparas som .km87-filer (XML) i ~/Library/Application Support/Kroppsmaskinen/Presets.
// Varje fil och varje DAW-läge har ett formatversionsnummer, så att senare versioner kan läsa äldre filer.
// Används bara från meddelandetråden (gränssnittet och värdens programbyte).
//
// Bygger på PresetManager i KrangBox, med eget format.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"
#include "Ljudbanken.h"
#include "../dsp/Motor.h"

namespace km
{

class PresetManager
{
public:
    // 1: fas 0 (ett trummönster och ett tonmönster). 2: fas 1 (banker med 16 mönster × A/B per kanal).
    // 3: fas 4 (låten, barnet "Laten").
    static constexpr int formatVersion = 3;
    static inline const juce::String fileExtension { ".km87" };

    struct Entry
    {
        juce::String name, category;
        bool factory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    PresetManager (juce::AudioProcessorValueTreeState& s, Banker& b) : state (s), banker (b) { refresh(); }

    // Mönstren sparas som barnet "Monster" i läget: ett barn "M" per mönster som inte är tomt,
    // med kanal (kb, kb2, sv, hu), nummer (1–16), variant (A/B) och texten.
    static void writePatterns (juce::ValueTree& tree, const Banker& banker)
    {
        tree.setProperty ("formatVersion", formatVersion, nullptr);
        tree.removeChild (tree.getChildWithName ("Monster"), nullptr);
        juce::ValueTree m ("Monster");
        auto lagg = [&m] (const char* kanal, int nr, int variant, const juce::String& text)
        {
            juce::ValueTree e ("M");
            e.setProperty ("kanal", kanal, nullptr);
            e.setProperty ("nr", nr + 1, nullptr);
            e.setProperty ("variant", variant == 0 ? "A" : "B", nullptr);
            e.setProperty ("text", text, nullptr);
            m.appendChild (e, nullptr);
        };
        for (int nr = 0; nr < antalMonster; ++nr)
            for (int v = 0; v < 2; ++v)
            {
                if (! banker.trummor.get (nr, v).arTomt())   lagg ("sv", nr, v, banker.trummor.get (nr, v).toString());
                if (! banker.kolvbasen.get (nr, v).arTomt()) lagg ("kb", nr, v, banker.kolvbasen.get (nr, v).toString());
                if (! banker.kolvbas2.get (nr, v).arTomt())  lagg ("kb2", nr, v, banker.kolvbas2.get (nr, v).toString());
                if (! banker.hugget.get (nr, v).arTomt())    lagg ("hu", nr, v, banker.hugget.get (nr, v).toString());
            }
        tree.appendChild (m, nullptr);

        tree.removeChild (tree.getChildWithName ("Laten"), nullptr);
        if (! banker.laten.arTom())
        {
            juce::ValueTree l ("Laten");
            l.setProperty ("text", banker.laten.toString(), nullptr);
            tree.appendChild (l, nullptr);
        }
    }

    // Läser och tar bort barnet. Falskt om filen är från en nyare version än vi förstår.
    static bool readPatterns (juce::ValueTree& tree, Banker& banker)
    {
        const int version = (int) tree.getProperty ("formatVersion", 1);
        if (version > formatVersion) return false;
        const auto m = tree.getChildWithName ("Monster");
        if (m.isValid())
        {
            banker.trummor.rensa();
            banker.kolvbasen.rensa();
            banker.kolvbas2.rensa();
            banker.hugget.rensa();
            if (version == 1)
            {
                // Fas 0: ett trummönster och en basgång blir mönster 1A.
                banker.trummor.get (0, 0).fromString (m["trummor"].toString());
                banker.kolvbasen.get (0, 0).fromString (m["kolvbasen"].toString());
            }
            else
            {
                for (const auto& e : m)
                {
                    const int nr = (int) e["nr"] - 1, v = e["variant"].toString() == "B" ? 1 : 0;
                    const auto kanal = e["kanal"].toString();
                    if (kanal == "sv")       banker.trummor.get (nr, v).fromString (e["text"].toString());
                    else if (kanal == "kb")  banker.kolvbasen.get (nr, v).fromString (e["text"].toString());
                    else if (kanal == "kb2") banker.kolvbas2.get (nr, v).fromString (e["text"].toString());
                    else if (kanal == "hu")  banker.hugget.get (nr, v).fromString (e["text"].toString());
                }
            }
            tree.removeChild (m, nullptr);
        }
        banker.laten.rensa();
        const auto l = tree.getChildWithName ("Laten");
        if (l.isValid())
        {
            banker.laten.fromString (l["text"].toString());
            tree.removeChild (l, nullptr);
        }
        tree.setProperty ("formatVersion", formatVersion, nullptr);
        return true;
    }

    static juce::File userFolder()
    {
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        base = base.getChildFile ("Application Support");
       #endif
        return base.getChildFile ("Kroppsmaskinen").getChildFile ("Presets");
    }

    void refresh()
    {
        entries.clear();
        const auto& factory = presets::factoryPresets();
        for (int i = 0; i < (int) factory.size(); ++i)
            entries.push_back ({ juce::String::fromUTF8 (factory[(size_t) i].name),
                                 juce::String::fromUTF8 (factory[(size_t) i].category), true, i, {} });

        auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*" + fileExtension);
        files.sort();
        for (auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), "Egna", false, -1, f });
    }

    const std::vector<Entry>& getEntries() const { return entries; }
    int getNumFactory() const { return (int) presets::factoryPresets().size(); }

    juce::String getCurrentName() const { return state.state.getProperty ("presetName", "Init").toString(); }

    int getCurrentIndex() const
    {
        const auto name = getCurrentName();
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].name == name) return i;
        return -1;
    }

    // Lås mönstren: presets laddar bara ljudet (klang, effekter, modulation, mix) och lämnar mönster,
    // låt, tempo, takt och kanalernas mönsterval som de är. Inställningen sparas inte.
    bool lasMonster = false;

    bool load (int index)
    {
        if (index < 0 || index >= (int) entries.size()) return false;
        const auto& e = entries[(size_t) index];
        if (e.factory) loadFactory (presets::factoryPresets()[(size_t) e.factoryIndex]);
        else if (! loadFile (e.file)) return false;
        state.state.setProperty ("presetName", e.name, nullptr);
        for (int k = 0; k < antalKanaler; ++k) state.state.removeProperty (Ljudbanken::namnNyckel (k), nullptr);
        return true;
    }

    bool loadByName (const juce::String& name)
    {
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].name == name) return load (i);
        return false;
    }

    void loadNext (int delta)
    {
        if (entries.empty()) return;
        const int n = (int) entries.size();
        load (((juce::jmax (0, getCurrentIndex()) + delta) % n + n) % n);
    }

    bool saveUser (const juce::String& rawName)
    {
        auto name = juce::File::createLegalFileName (rawName.trim());
        if (name.isEmpty()) return false;
        for (const auto& e : entries)
            if (e.factory && e.name == name) name << " (egen)";   // fabriksbandet kan inte skrivas över
        userFolder().createDirectory();
        auto copy = state.copyState();
        copy.setProperty ("presetName", name, nullptr);
        writePatterns (copy, banker);
        auto xml = copy.createXml();
        if (xml == nullptr || ! xml->writeTo (userFolder().getChildFile (name + fileExtension))) return false;
        state.state.setProperty ("presetName", name, nullptr);
        refresh();
        return true;
    }

private:
    static void setParam (juce::RangedAudioParameter& p, float normalised)
    {
        p.beginChangeGesture();
        p.setValueNotifyingHost (normalised);
        p.endChangeGesture();
    }

    void loadFactory (const presets::FactoryPreset& preset)
    {
        // Spela-knappen behålls; allt annat till standard först, så att presets inte ärver från det förra ljudet.
        for (auto* param : state.processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (! arUppkoppling (p->getParameterID()) && ! (lasMonster && arMonsterParameter (p->getParameterID())))
                    setParam (*p, p->getDefaultValue());

        for (const auto& [paramId, value] : preset.values)
        {
            if (arUppkoppling (paramId) || (lasMonster && arMonsterParameter (paramId))) continue;
            if (auto* p = state.getParameter (paramId)) setParam (*p, p->convertTo0to1 (value));
            else jassertfalse; // stavfel i FactoryPresets.h
        }
        if (lasMonster) return;

        banker.trummor.rensa();
        banker.kolvbasen.rensa();
        banker.kolvbas2.rensa();
        banker.hugget.rensa();
        for (const auto& t : preset.trummor)
        {
            auto& monster = banker.trummor.get (t.monster, t.variant);
            for (const auto& [spar, rad] : t.rader)
            {
                const auto steg = TrumMonster::lasRad (juce::String::fromUTF8 (rad));
                if (steg.empty()) continue;
                for (int st = 0; st < maxSteg; ++st)
                    monster.set ((int) spar, st, steg[(size_t) st % steg.size()]);
            }
        }
        for (const auto& t : preset.toner)
            banker.ton ((int) t.kanal).get (t.monster, t.variant).fromString (juce::String::fromUTF8 (t.text));

        banker.laten.rensa();
        if (preset.laten != nullptr)
        {
            banker.laten.fromString (juce::String::fromUTF8 (preset.laten));
            if (auto* p = state.getParameter (params::id::latLage)) setParam (*p, 1.0f);
        }
    }

    bool loadFile (const juce::File& f)
    {
        auto xml = juce::XmlDocument::parse (f);
        if (xml == nullptr || ! xml->hasTagName (state.state.getType())) return false;
        auto tree = juce::ValueTree::fromXml (*xml);
        if (lasMonster)
        {
            // Bara parametrarna, utom de som hör till mönstren och takten.
            if ((int) tree.getProperty ("formatVersion", 1) > formatVersion) return false;
            for (const auto& c : tree)
                if (c.hasType ("PARAM"))
                {
                    const auto id = c["id"].toString();
                    if (arUppkoppling (id) || arMonsterParameter (id)) continue;
                    if (auto* p = state.getParameter (id)) setParam (*p, p->convertTo0to1 ((float) c["value"]));
                }
            return true;
        }
        if (! readPatterns (tree, banker)) return false;
        // Uppkopplingen (spela, MIDI-spel) följer inte med presetet: läs av den före och sätt tillbaka den.
        std::vector<std::pair<juce::RangedAudioParameter*, float>> behall;
        for (auto* param : state.processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
                if (arUppkoppling (p->getParameterID())) behall.emplace_back (p, p->getValue());
        state.replaceState (tree);
        for (auto& [p, v] : behall)
            if (p->getValue() != v) setParam (*p, v);
        return true;
    }

    juce::AudioProcessorValueTreeState& state;
    Banker& banker;
    std::vector<Entry> entries;
};

} // namespace km
