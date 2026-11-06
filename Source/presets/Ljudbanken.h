// Ljudbanken: ljud per kanal, som byter bara kanalens klang och lämnar mönster, låt, tempo och mix i fred.
//
// Tre sorter: basljud (för både Kolvbasen och Kolvbas 2), trumljud (Stampverkets kit och de tolv
// trummornas stämning, klang och nivå) och Huggets ljud. Varje ljud tar också med kanalens Drivet.
// Ett ljud sparas som parametrarnas ändelser (t.ex. "cutoff" för km_kb_cutoff), så att samma basljud
// kan läggas på Kolvbasen och Kolvbas 2.
//
// Fabriksljuden tas ur fabriksbandet: varje preset bidrar med de kanaler den använder (dubbletter tas
// bort). Egna ljud sparas i Presets/Ljud/<Bas|Trummor|Hugget>/ som .km87ljud.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FactoryPresets.h"
#include <map>

namespace km
{

enum class LjudTyp { Bas = 0, Trummor, Hugget };
inline LjudTyp ljudTyp (int kanal) { return kanal == (int) Kanal::Stampverket ? LjudTyp::Trummor : (kanal == (int) Kanal::Hugget ? LjudTyp::Hugget : LjudTyp::Bas); }
inline const char* ljudTypNamn (LjudTyp t) { return t == LjudTyp::Bas ? "Bas" : (t == LjudTyp::Trummor ? "Trummor" : "Hugget"); }

// Parametrar som hör till takten, mönstren och mixen – inte till ljudet. Används av ljudbanken och av
// "Lås mönstren" (presets som laddas utan att mönster, låt och tempo byts).
inline bool arMonsterParameter (const juce::String& id)
{
    using namespace params;
    for (auto* g : { id::spela, id::tempo, id::gung, id::grundton, id::varvtal, id::varvLage, id::latLage, id::latSlut, id::stamning, id::stamningTon,
                     id::folj, id::forskjutning, id::transponering })
        if (id == g) return true;
    for (int k = 0; k < antalKanaler; ++k)
        for (auto* v : { "langd", "monster", "variant", "tyst", "solo", "midi" })
            if (id == id::kanal (k, v)) return true;
    return false;
}

// Uppkopplingen: spela-knappen och MIDI-spel (per kanal och Allt från MIDI). Hör till hur pluginet
// används i studion, inte till låten, så presets lämnar dem som de är.
inline bool arUppkoppling (const juce::String& id)
{
    using namespace params;
    if (id == id::spela || id == id::midiAlla) return true;
    for (int k = 0; k < antalKanaler; ++k)
        if (id == id::kanal (k, "midi")) return true;
    return false;
}

struct Ljud
{
    juce::String namn, kategori;
    std::map<juce::String, float> varden;   // ändelse → värde (parameterns egen enhet)
    bool fabrik = true;
    juce::File fil;
};

class Ljudbanken
{
public:
    explicit Ljudbanken (juce::AudioProcessorValueTreeState& s) : state (s) { refresh(); }

    static constexpr int antalTyper = 3;
    static inline const juce::String filandelse { ".km87ljud" };

    // Ljudets parametrar för en kanal: ändelserna efter kanalens prefix, utan mix (nivå, panorering,
    // sändningar) och mönster. Trummornas tyst-knappar räknas inte heller.
    juce::StringArray andelser (int kanal) const
    {
        const juce::String prefix (params::id::kanalPrefix (kanal));
        juce::StringArray ut;
        for (auto* p : state.processor.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
            {
                const auto id = r->getParameterID();
                if (! id.startsWith (prefix) || arMonsterParameter (id)) continue;
                const auto a = id.substring (prefix.length());
                if (a == "niva" || a == "pan" || a == "sandA" || a == "sandB" || a.endsWith ("_tyst")) continue;
                ut.add (a);
            }
        return ut;
    }

    const std::vector<Ljud>& lista (LjudTyp t) const { return listor[(size_t) t]; }

    // Lägger ljud nummer i på kanalen (som en gest per parameter, så att DAW:en ser ändringen).
    bool ladda (int kanal, int i)
    {
        const auto& l = lista (ljudTyp (kanal));
        if (! juce::isPositiveAndBelow (i, (int) l.size())) return false;
        const juce::String prefix (params::id::kanalPrefix (kanal));
        for (const auto& a : andelser (kanal))
            if (auto* p = state.getParameter (prefix + a))
            {
                const auto it = l[(size_t) i].varden.find (a);
                const float v = it != l[(size_t) i].varden.end() ? it->second : p->convertFrom0to1 (p->getDefaultValue());
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 (v));
                p->endChangeGesture();
            }
        state.state.setProperty (namnNyckel (kanal), l[(size_t) i].namn, nullptr);
        return true;
    }

    bool sparaEget (int kanal, const juce::String& raNamn)
    {
        const auto namn = juce::File::createLegalFileName (raNamn.trim());
        if (namn.isEmpty()) return false;
        const juce::String prefix (params::id::kanalPrefix (kanal));
        juce::XmlElement xml ("KM87Ljud");
        xml.setAttribute ("typ", ljudTypNamn (ljudTyp (kanal)));
        xml.setAttribute ("version", 1);
        for (const auto& a : andelser (kanal))
            if (auto* p = state.getParameter (prefix + a))
            {
                auto* e = xml.createNewChildElement ("P");
                e->setAttribute ("id", a);
                e->setAttribute ("v", (double) p->convertFrom0to1 (p->getValue()));
            }
        auto mapp = mappFor (ljudTyp (kanal));
        mapp.createDirectory();
        if (! xml.writeTo (mapp.getChildFile (namn + filandelse))) return false;
        state.state.setProperty (namnNyckel (kanal), namn, nullptr);
        refresh();
        return true;
    }

    // Namnet på kanalens senast laddade ljud (tomt = ljudet kommer från presetet).
    juce::String aktuelltNamn (int kanal) const { return state.state.getProperty (namnNyckel (kanal)).toString(); }
    static juce::Identifier namnNyckel (int kanal) { return juce::Identifier ("ljud" + juce::String (kanal)); }

    int aktuelltIndex (int kanal) const
    {
        const auto namn = aktuelltNamn (kanal);
        const auto& l = lista (ljudTyp (kanal));
        for (int i = 0; i < (int) l.size(); ++i) if (l[(size_t) i].namn == namn) return i;
        return -1;
    }

    static juce::File mappFor (LjudTyp t)
    {
        auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        base = base.getChildFile ("Application Support");
       #endif
        return base.getChildFile ("Kroppsmaskinen").getChildFile ("Presets").getChildFile ("Ljud").getChildFile (ljudTypNamn (t));
    }

    void refresh()
    {
        for (auto& l : listor) l.clear();
        byggFabriksljud();
        for (int t = 0; t < antalTyper; ++t)
        {
            auto filer = mappFor ((LjudTyp) t).findChildFiles (juce::File::findFiles, false, "*" + filandelse);
            filer.sort();
            for (auto& f : filer)
                if (auto xml = juce::XmlDocument::parse (f))
                {
                    Ljud l { f.getFileNameWithoutExtension(), "Egna", {}, false, f };
                    for (auto* e : xml->getChildWithTagNameIterator ("P"))
                        l.varden[e->getStringAttribute ("id")] = (float) e->getDoubleAttribute ("v");
                    listor[(size_t) t].push_back (std::move (l));
                }
        }
    }

private:
    // Fabriksljuden: Init (grundvärdena) och varje fabrikspresets använda kanaler.
    void byggFabriksljud()
    {
        const int kanalFor[antalTyper] { 0, (int) Kanal::Stampverket, (int) Kanal::Hugget };
        for (int t = 0; t < antalTyper; ++t)
        {
            Ljud init { "Init", "Grund", {}, true, {} };
            for (const auto& a : andelser (kanalFor[t]))
                if (auto* p = state.getParameter (juce::String (params::id::kanalPrefix (kanalFor[t])) + a))
                    init.varden[a] = p->convertFrom0to1 (p->getDefaultValue());
            listor[(size_t) t].push_back (init);
        }

        for (const auto& fp : presets::factoryPresets())
        {
            const juce::String presetNamn = juce::String::fromUTF8 (fp.name), kategori = juce::String::fromUTF8 (fp.category);
            if (kategori == "Grund") continue;
            std::map<juce::String, float> satta;
            for (const auto& [id, v] : fp.values) satta[id] = v;

            auto anvands = [&] (int k)
            {
                if (k == (int) Kanal::Stampverket) return ! fp.trummor.empty();
                for (const auto& t : fp.toner) if ((int) t.kanal == k) return true;
                if (k == 1) { const auto it = satta.find (params::id::folj); return it != satta.end() && it->second > 0.5f; }
                return false;
            };
            for (int k = 0; k < antalKanaler; ++k)
            {
                if (! anvands (k)) continue;
                const auto typ = ljudTyp (k);
                Ljud l;
                l.kategori = kategori;
                l.namn = presetNamn + (typ == LjudTyp::Bas ? juce::String (k == 0 ? " (bas 1)" : " (bas 2)") : juce::String());
                const juce::String prefix (params::id::kanalPrefix (k));
                for (const auto& a : andelser (k))
                    if (auto* p = state.getParameter (prefix + a))
                    {
                        const auto it = satta.find (prefix + a);
                        l.varden[a] = it != satta.end() ? it->second : p->convertFrom0to1 (p->getDefaultValue());
                    }
                auto& lista = listor[(size_t) typ];
                bool dubblett = false;
                for (const auto& f : lista) dubblett = dubblett || f.varden == l.varden;
                if (! dubblett) lista.push_back (std::move (l));
            }
        }
    }

    juce::AudioProcessorValueTreeState& state;
    std::array<std::vector<Ljud>, antalTyper> listor;
};

} // namespace km
