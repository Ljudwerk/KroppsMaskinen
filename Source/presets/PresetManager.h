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
#include "../takt/Monster.h"

namespace km
{

class PresetManager
{
public:
    static constexpr int formatVersion = 1;
    static inline const juce::String fileExtension { ".km87" };

    struct Entry
    {
        juce::String name, category;
        bool factory = true;
        int factoryIndex = -1;
        juce::File file;
    };

    PresetManager (juce::AudioProcessorValueTreeState& s, TrumMonster& t, TonMonster& b) : state (s), trummor (t), bas (b) { refresh(); }

    // Mönstren sparas som barnet "Monster" i läget.
    static void writePatterns (juce::ValueTree& tree, const TrumMonster& trummor, const TonMonster& bas)
    {
        tree.setProperty ("formatVersion", formatVersion, nullptr);
        tree.removeChild (tree.getChildWithName ("Monster"), nullptr);
        juce::ValueTree m ("Monster");
        m.setProperty ("trummor", trummor.toString(), nullptr);
        m.setProperty ("kolvbasen", bas.toString(), nullptr);
        tree.appendChild (m, nullptr);
    }

    // Läser och tar bort barnet. Falskt om filen är från en nyare version än vi förstår.
    static bool readPatterns (juce::ValueTree& tree, TrumMonster& trummor, TonMonster& bas)
    {
        if ((int) tree.getProperty ("formatVersion", 1) > formatVersion) return false;
        const auto m = tree.getChildWithName ("Monster");
        if (m.isValid())
        {
            trummor.fromString (m["trummor"].toString());
            bas.fromString (m["kolvbasen"].toString());
            tree.removeChild (m, nullptr);
        }
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

    bool load (int index)
    {
        if (index < 0 || index >= (int) entries.size()) return false;
        const auto& e = entries[(size_t) index];
        if (e.factory) loadFactory (presets::factoryPresets()[(size_t) e.factoryIndex]);
        else if (! loadFile (e.file)) return false;
        state.state.setProperty ("presetName", e.name, nullptr);
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
        writePatterns (copy, trummor, bas);
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
                if (p->getParameterID() != params::id::spela)
                    setParam (*p, p->getDefaultValue());

        for (const auto& [paramId, value] : preset.values)
        {
            if (auto* p = state.getParameter (paramId)) setParam (*p, p->convertTo0to1 (value));
            else jassertfalse; // stavfel i FactoryPresets.h
        }

        trummor.rensa();
        for (const auto& [spar, rad] : preset.trummor)
        {
            const auto steg = TrumMonster::lasRad (juce::String::fromUTF8 (rad));
            if (steg.empty()) continue;
            for (int st = 0; st < maxSteg; ++st)
                trummor.set ((int) spar, st, steg[(size_t) st % steg.size()]);
        }
        bas.rensa();
        if (preset.kolvbasen != nullptr) bas.fromString (juce::String::fromUTF8 (preset.kolvbasen));
    }

    bool loadFile (const juce::File& f)
    {
        auto xml = juce::XmlDocument::parse (f);
        if (xml == nullptr || ! xml->hasTagName (state.state.getType())) return false;
        auto tree = juce::ValueTree::fromXml (*xml);
        if (! readPatterns (tree, trummor, bas)) return false;
        state.replaceState (tree);
        return true;
    }

    juce::AudioProcessorValueTreeState& state;
    TrumMonster& trummor;
    TonMonster& bas;
    std::vector<Entry> entries;
};

} // namespace km
