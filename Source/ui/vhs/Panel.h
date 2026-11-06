// VHS-panelen (fas 3): hela gränssnittet i designstorlek 1200 × 840, skalas som helhet av editorn.
//
// Sektionerna följer stilprovet: Bandverket överst (transport, räkneverk, bandhastighet, tempo,
// Varvtalet, Spänningen), kanalväljaren till vänster, bildskärmen i mitten, presets/VU/master till
// höger, rattpanelen med vyerna KANAL · SEKVENS · MIXER · EFFEKTER · MODULATION · LÅTEN, stegraden och
// klaviaturen.
//
// KANAL-vyn visar kanalväljaren, bildskärmen och rattpanelen. De andra vyerna är egna sidor (som i
// KrangBox): de tar över kanalväljarens, bildskärmens och rattpanelens plats med stora reglage i lådor,
// och en liten kanalväljare flyttar in under presets. Presets, VU och mastern syns alltid.
//
// Prestanda: allt som inte rör sig (chassit, fälten, trycket, grupprubrikerna, infattningen) ritas
// en gång till en bild i fysisk upplösning och byggs om bara när vy, kanal, trumma eller skala
// ändras. Reglagen ritar om sig själva när deras värde ändras; timern (30 Hz) ritar bara om det som
// faktiskt har ändrats (lysdioder, displayer, VU). Bildskärmen sköter sin egen bildtakt.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Reglage.h"
#include "PanelLage.h"
#include "Stegrad.h"
#include "Sekvens.h"
#include "Klaviatur.h"
#include "LatVy.h"
#include "Bildskarm.h"
#include "../stil/Textur.h"
#include "../stil/OSDTypsnitt.h"
#include "../stil/Skarm.h"
#include "../stil/Stilprov.h"

#if JUCE_WINDOWS
 #include <windows.h>
#else
 #include <time.h>
#endif

namespace km::vhs
{

// Processortid (ms) som den anropande tråden har använt. Panelen mäter meddelandetråden med den:
// allt gränssnittet gör (timern, ritningen) sker där.
inline double tradensCpuMs()
{
   #if JUCE_WINDOWS
    FILETIME a, b, k, u;
    if (! GetThreadTimes (GetCurrentThread(), &a, &b, &k, &u)) return 0.0;
    auto ms = [] (FILETIME f) { return (double) (((uint64_t) f.dwHighDateTime << 32) | f.dwLowDateTime) / 10000.0; };
    return ms (k) + ms (u);
   #else
    timespec ts {};
    if (clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return 0.0;
    return (double) ts.tv_sec * 1000.0 + (double) ts.tv_nsec / 1.0e6;
   #endif
}

class Panel final : public juce::Component, private juce::Timer
{
public:
    static constexpr int designBredd = 1200, designHojd = 840;
    static constexpr int antalVyer = 9;   // 0–3 KANAL per kanal, 4 SEKVENS, 5 MIXER, 6 EFFEKTER, 7 MODULATION, 8 LÅTEN

    std::function<void (float)> skalaAndrad;   // MENY → Storlek; editorn ändrar sin storlek

    explicit Panel (KroppsmaskinenProcessor& p) : proc (p), lage (p), stegrad (lage), sekvens (lage), latVy (lage), klaviatur (lage)
    {
        setOpaque (true);
        setSize (designBredd, designHojd);
        setWantsKeyboardFocus (true);
        lage.andrat = [this] { andrat(); };
        visare.visa = [this] (const juce::String& n, const juce::String& v) { vfdNamn = versaler (n); vfdVarde = versaler (v); vfdTid = nu(); vfd->repaint(); };

        byggBandverket();
        byggBildskarmen();
        kanalYtor.push_back (&bild);
        byggPresets();
        byggVyknappar();
        byggLjudvaljaren();
        addAndMakeVisible (stegrad);
        stegrad.setBounds (14, 660, 1172, 92);
        addChildComponent (sekvens);
        addChildComponent (latVy);
        byggSidorna();
        byggKanalsidor();
        byggMinikanaler();
        byggKlaviaturen();

        valjVy (Vy::Kanal, 0);
        startTimerHz (30);
    }

    ~Panel() override { stopTimer(); klaviatur.slappDator(); }

    // Datorns tangentbord spelar klaviaturen (A W S E D … = C C♯ D …), Z och X byter oktav, mellanslag
    // startar och stoppar. Panelen tar tangentbordsfokus när man klickar i den.
    bool keyPressed (const juce::KeyPress& k) override
    {
        const auto c = juce::CharacterFunctions::toLowerCase (k.getTextCharacter());
        if (k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown()) return false;
        if (c == 'z' || c == 'x')
        {
            klaviatur.slappDator();
            klaviatur.oktav = juce::jlimit (0, 6, klaviatur.oktav + (c == 'x' ? 1 : -1));
            oktavPerKanal[(size_t) lage.kanal] = klaviatur.oktav;
            visare ("OKTAV", "C" + juce::String (klaviatur.oktav));
            andrat();
            return true;
        }
        if (k == juce::KeyPress::spaceKey) { satt (params::id::spela, param (params::id::spela) > 0.5f ? 0.0f : 1.0f); return true; }
        if (juce::String (Klaviatur::datorTangenter).containsChar (c)) { klaviatur.datorTangentbord(); return true; }
        return false;
    }
    bool keyStateChanged (bool) override { return klaviatur.datorTangentbord(); }
    void focusLost (FocusChangeType) override { klaviatur.slappDator(); }

    // Väljer vy och kanal (som att trycka på knapparna).
    void valjVy (Vy v, int kanal)
    {
        lage.vy = v;
        lage.kanal = juce::jlimit (0, 3, kanal);
        klaviatur.oktav = oktavPerKanal[(size_t) lage.kanal];
        andrat();
    }

    PanelLage& getLage() { return lage; }

    // Parametrar som nås från panelen: reglagen och de egna ytorna (transport, varvtal, mönster …).
    juce::StringArray nadaParametrar() const
    {
        juce::StringArray ut;
        for (auto& r : reglage) if (auto* p = dynamic_cast<ParamReglage*> (r.get())) ut.addIfNotAlreadyThere (p->paramId());
        for (auto* id : { params::id::spela, params::id::varvtal, params::id::varvLage, params::id::midiAlla })
            ut.addIfNotAlreadyThere (id);
        ut.addIfNotAlreadyThere (params::id::fx ("kass", "kvalitet"));
        for (int k = 0; k < antalKanaler; ++k)
            for (auto* v : { "tyst", "monster", "variant" }) ut.addIfNotAlreadyThere (params::id::kanal (k, v));
        return ut;
    }
    Bildskarm& getBildskarm() { return bild; }

    // Bandstyrkan sparas i processorns panelläge.
    Bandstyrka bandstyrka() const { return (Bandstyrka) juce::jlimit (0, 2, (int) proc.panelLage.getProperty ("bandstyrka", 2)); }
    void sattBandstyrka (Bandstyrka b) { proc.panelLage.setProperty ("bandstyrka", (int) b, nullptr); }

    // Mätning (KMSnapshot --panel-tid): kör timern utan meddelandeslinga.
    void tickForTest() { timerCallback(); }

    void paint (juce::Graphics& g) override
    {
        const float s = stil::fysiskSkala (g);
        const auto nyckel = bakgrundsNyckel();
        if (std::abs (s - bgSkala) > 0.001f || nyckel != bgNyckel || ! bakgrund.isValid())
        {
            bgSkala = s; bgNyckel = nyckel;
            ritaBakgrund (s);
        }
        g.drawImageTransformed (bakgrund, juce::AffineTransform::scale (1.0f / s));
    }

private:
    // -----------------------------------------------------------------------------------------
    // Hjälpare

    static double nu() { return juce::Time::getMillisecondCounterHiRes(); }
    float param (const juce::String& id) const { return proc.state.getRawParameterValue (id)->load(); }

    // Sätter en parameter (verkligt värde) som en hel gest.
    void satt (const juce::String& id, float v)
    {
        if (auto* p = proc.state.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (v));
            p->endChangeGesture();
            visare (p->getName (40), ui::formatValue (*p, v));
        }
    }

    Tryckyta& yta (Rectangle<int> r, std::function<void (juce::Graphics&, Rectangle<float>)> rita,
                   std::function<juce::int64()> lageFn = {}, std::function<void (const juce::MouseEvent&)> tryck = {})
    {
        auto t = std::make_unique<Tryckyta>();
        t->rita = std::move (rita);
        t->lage = std::move (lageFn);
        t->tryck = std::move (tryck);
        t->setBounds (r);
        addAndMakeVisible (*t);
        ytor.push_back (std::move (t));
        return *ytor.back();
    }

    template <typename R, typename... A>
    R& nytt (A&&... a)
    {
        auto r = std::make_unique<R> (proc.state, std::forward<A> (a)...);
        auto& ref = *r;
        addChildComponent (ref);
        reglage.push_back (std::move (r));
        return ref;
    }
    Ratt& ratt (const juce::String& id, const char* text, bool inv = false)
    {
        auto& r = nytt<Ratt> (id, stil::sv (text), visare, 12.0f, stil::RattTyp::liten, inv);
        r.visaVardet = true;
        return r;
    }
    Knapp& knapp (const juce::String& id, const char* text) { return nytt<Knapp> (id, stil::sv (text), visare); }
    Valjare& valj (const juce::String& id, const char* text, int bredd = 66) { return nytt<Valjare> (id, stil::sv (text), visare, bredd); }

    static juce::String kanalNamnet (int k)
    {
        static const char* n[4] { "KOLVBASEN", "KOLVBAS 2", "STAMPVERKET", "HUGGET" };
        return n[juce::jlimit (0, 3, k)];
    }
    static juce::String trumKort (int t)
    {
        static const char* n[antalSpar] { "BD", "SD", "SD2", "CP", "CH", "OH", "CY", "LT", "HT", "MET", "SMÄLL", "KO" };
        return stil::sv (n[juce::jlimit (0, antalSpar - 1, t)]);
    }

    // -----------------------------------------------------------------------------------------
    // Bandverket

    void byggBandverket()
    {
        using namespace km::stil;
        const Ikon ikoner[] { Ikon::spolaBakat, Ikon::spela, Ikon::stopp, Ikon::spelaIn };
        for (int i = 0; i < 4; ++i)
        {
            const Rectangle<int> k (408 + i * 50, 24, 42, 32);
            auto aktiv = [this, i]
            {
                const bool spelar = param (params::id::spela) > 0.5f;
                return i == 1 ? spelar : (i == 2 ? stoppNed : (i == 3 ? lage.spelaIn : spolaNed));   // SPOLA och STOPP fjädrar tillbaka
            };
            yta (k, [aktiv, i, ik = ikoner[i]] (juce::Graphics& g, Rectangle<float> r)
            {
                const bool a = aktiv();
                const auto kr = r.withTrimmedTop (2.0f).withTrimmedBottom (2.0f);
                auto y = ritaKnapp (g, kr, a);
                const auto p = ikon (ik, y.reduced (10.0f, 5.0f));
                if (a && i != 2)
                {
                    g.setColour (farg::lysRod.withAlpha (0.25f));
                    g.strokePath (p, juce::PathStrokeType (3.0f));
                    g.setColour (i == 3 ? farg::lysRod : farg::lysRod);
                }
                else g.setColour (i == 3 ? farg::tryckRott : (a ? farg::vitt : farg::silver));
                g.fillPath (p);
                if (a && i != 2) ritaLysdiod (g, { kr.getRight() - 6.0f, kr.getY() + 6.0f }, 1.8f, farg::lysRod,
                                              i == 3 && (juce::Time::getMillisecondCounter() / 400) % 2 ? 0.4f : 1.0f);
            },
            [aktiv, i] { return (juce::int64) aktiv() * 2 + (i == 3 && aktiv() ? (juce::int64) ((juce::Time::getMillisecondCounter() / 400) % 2) * 4 : 0); },
            [this, i] (const juce::MouseEvent&)
            {
                if (i == 0) { proc.spola(); lage.markor = 0; spolaNed = true; spolaTid = nu(); }
                else if (i == 1) satt (params::id::spela, 1.0f);
                else if (i == 2) { satt (params::id::spela, 0.0f); stoppNed = true; stoppTid = nu(); }
                else { lage.spelaIn = ! lage.spelaIn; if (lage.spelaIn) lage.markor = lage.sida * 16; }
                andrat();
            });
        }

        // Räkneverket: speltid.
        yta ({ 612, 24, 120, 32 }, [this] (juce::Graphics& g, Rectangle<float> r) { ritaSjusegment (g, r, tidText (proc.speltid())); },
             [this] { return (juce::int64) proc.speltid(); });

        // Allt från MIDI: hela sekvensern tyst, alla kanaler spelas av DAW:en. Följer inte med presets.
        yta ({ 612, 62, 120, 14 }, [this] (juce::Graphics& g, Rectangle<float> r)
        {
            prov::diodMedText (g, { r.getX() + 6.0f, r.getCentreY() }, stil::sv ("ALLT FRÅN MIDI"), param (params::id::midiAlla) > 0.5f, farg::lysRod);
        },
        [this] { return (juce::int64) (param (params::id::midiAlla) > 0.5f); },
        [this] (const juce::MouseEvent&) { satt (params::id::midiAlla, param (params::id::midiAlla) > 0.5f ? 0.0f : 1.0f); });

        // Bandhastigheten (Kassettens kvalitet): SP / LP / EP.
        yta ({ 742, 21, 36, 38 }, [this] (juce::Graphics& g, Rectangle<float> r)
        {
            const int k = (int) param (params::id::fx ("kass", "kvalitet"));
            const char* t[] { "SP", "LP", "EP" };
            for (int i = 0; i < 3; ++i) prov::diodMedText (g, { r.getX() + 6.0f, r.getY() + 7.0f + 12.0f * (float) i }, t[i], i == k, farg::lysOrange);
        },
        [this] { return (juce::int64) param (params::id::fx ("kass", "kvalitet")); },
        [this] (const juce::MouseEvent& e) { satt (params::id::fx ("kass", "kvalitet"), (float) juce::jlimit (0, 2, (int) ((e.position.y - 1.0f) / 12.0f))); });

        // Tempo.
        auto& tempo = nytt<SegmentReglage> (params::id::tempo, visare, 1, 4.0f);
        tempo.setBounds (784, 24, 100, 32);
        tempo.setVisible (true);

        // Varvtalet: 45 / 33 / 33 + 8 %, och skivspelarläget.
        for (int i = 0; i < 3; ++i)
        {
            const char* text[] { "45", "33", "+8%" };
            yta ({ 902 + i * 40, 20, 36, 34 }, [this, i, t = text[i]] (juce::Graphics& g, Rectangle<float> r)
            {
                const bool pa = (int) param (params::id::varvtal) == i;
                ritaLysdiod (g, { r.getCentreX(), 6.0f + r.getY() - 20.0f + 20.0f }, 2.2f, farg::lysOrange, pa ? 1.0f : 0.0f);
                auto y = ritaKnapp (g, { r.getX() + 2.0f, r.getY() + 14.0f, 32.0f, 18.0f }, pa);
                tryck (g, t, y, 6.0f, juce::Justification::centred, pa ? farg::vitt : farg::silver, 0.9f, 1.6f);
            },
            [this] { return (juce::int64) param (params::id::varvtal); },
            [this, i] (const juce::MouseEvent&) { satt (params::id::varvtal, (float) i); });
        }
        yta ({ 900, 58, 124, 14 }, [this] (juce::Graphics& g, Rectangle<float> r)
        {
            prov::diodMedText (g, { r.getX() + 6.0f, r.getCentreY() }, "SKIVSPELARE", param (params::id::varvLage) > 0.5f, farg::lysOrange);
        },
        [this] { return (juce::int64) param (params::id::varvLage); },
        [this] (const juce::MouseEvent&) { satt (params::id::varvLage, param (params::id::varvLage) > 0.5f ? 0.0f : 1.0f); });

        // Spänningen.
        auto& spak = nytt<Spak> (params::id::spanning, visare);
        spak.setBounds (1036, 24, 146, 34);
        spak.setVisible (true);
    }

    static juce::String tidText (double sek)
    {
        const int s = (int) sek;
        return juce::String (s / 3600) + ":" + juce::String ((s / 60) % 60).paddedLeft ('0', 2) + ":" + juce::String (s % 60).paddedLeft ('0', 2);
    }

    // -----------------------------------------------------------------------------------------
    // Bildskärmen

    // Skärmen ritas i sin designstorlek (glaset 640 × 304) och skalas ned till KANAL-sidans övre högra hörn.
    static constexpr float skarmSkala = 0.62f;
    static juce::AffineTransform skarmTransform()
    {
        const float b = (640.0f + 44.0f) * skarmSkala;
        return juce::AffineTransform::translation (-208.0f, -92.0f).scaled (skarmSkala).translated (884.0f - b, 96.0f);
    }

    void byggBildskarmen()
    {
        using namespace km::stil;
        addAndMakeVisible (bild);
        bild.setBounds (glas.toNearestInt());
        // Bandlagret räknas högst i designupplösning (640 × 304, ungefär en VHS-bild) och sträcks ut på
        // Retina och vid förstoring; texten (OSD) ritas skarp ovanpå. Håller gränssnittet under CPU-målet.
        bild.setUpplosning (bildUpplosning, 640, 304);
        bild.setTransform (skarmTransform());
    }

    // -----------------------------------------------------------------------------------------
    // Presets, VU och master

    void byggPresets()
    {
        using namespace km::stil;
        const Rectangle<int> r (906, 96, 280, 352);
        yta ({ r.getX() + 12, r.getY() + 24, 54, 34 }, [this] (juce::Graphics& g, Rectangle<float> rr)
        {
            ritaSjusegment (g, rr, juce::String (proc.presets.getCurrentIndex() + 1).paddedLeft ('0', 2));
        },
        [this] { return (juce::int64) proc.presets.getCurrentIndex(); },
        [this] (const juce::MouseEvent&) { visaPresetMeny(); });

        vfd = &yta ({ r.getX() + 72, r.getY() + 22, r.getWidth() - 82, 38 }, [this] (juce::Graphics& g, Rectangle<float> rr) { ritaVfd (g, rr.reduced (2.0f)); },
                    [this] { return (juce::int64) (nu() - vfdTid < 1500.0) + 2 * proc.presets.getCurrentName().hashCode() + 7 * (int) diagnostikVarning
                                     + 11 * (visaPrestanda ? (juce::int64) (cpuProcent * 10.0) + 1 : 0) + 13 * (int) proc.presets.lasMonster; });

        const int y = r.getY() + 68;
        auto liten = [&] (Rectangle<int> k, juce::String text, Ikon ik, bool medIkon, std::function<void()> gor)
        {
            auto nere = std::make_shared<bool> (false);
            auto& t = yta (k, [nere, text, ik, medIkon] (juce::Graphics& g, Rectangle<float> rr)
            {
                auto a = ritaKnapp (g, rr.reduced (1.5f), *nere);
                g.setColour (farg::silver);
                if (medIkon) g.fillPath (ikon (ik, a.reduced (9.0f, 3.5f)));
                else tryck (g, text, a, 5.6f);
            });
            auto* tp = &t;
            t.tryck = [nere, tp] (const juce::MouseEvent&) { *nere = true; tp->repaint(); };
            t.slapp = [nere, tp, gor] (const juce::MouseEvent& e) { *nere = false; tp->repaint(); if (tp->getLocalBounds().contains (e.getPosition())) gor(); };
        };
        liten ({ r.getX() + 10, y - 2, 38, 22 }, {}, Ikon::bakat, true, [this] { proc.presets.loadNext (-1); });
        liten ({ r.getX() + 50, y - 2, 38, 22 }, {}, Ikon::spela, true, [this] { proc.presets.loadNext (1); });
        liten ({ r.getX() + 90, y - 2, 64, 22 }, "SPARA", Ikon::spela, false, [this] { sparaPreset(); });
        liten ({ r.getX() + 156, y - 2, 64, 22 }, "MENY", Ikon::spela, false, [this] { visaMeny(); });

        // VU-mätarna.
        const int vy = r.getY() + 102;
        const int vb = (r.getWidth() - 30) / 2;
        for (int c = 0; c < 2; ++c)
            yta ({ r.getX() + 12 + c * (vb + 6) - 3, vy - 3, vb + 6, 98 }, [this, c] (juce::Graphics& g, Rectangle<float> rr)
            {
                const juce::String lr = c == 0 ? "L" : "R";
                auto& m = vuMinne[(size_t) c];
                m[0].rita (g, rr, 0, [lr] (juce::Graphics& gi, Rectangle<float> q) { ritaVU (gi, q.reduced (3.0f), -40.0f, lr, 1); });
                ritaVU (g, rr.reduced (3.0f), vuDb[(size_t) c], lr, 2);
                m[1].rita (g, rr, 0, [lr] (juce::Graphics& gi, Rectangle<float> q) { ritaVU (gi, q.reduced (3.0f), -40.0f, lr, 3); });
            },
            [this, c] { return (juce::int64) std::round (vuDb[(size_t) c] * 4.0f); });

        // Mastern: nivå, Klämman (tröskeln, medurs = mer) och Kassetten (slitaget).
        auto& niva = nytt<Ratt> (params::id::master, stil::sv ("NIVÅ"), visare, 27.0f, RattTyp::stor);
        niva.setBounds (r.getX() + 60 - 46, r.getY() + 280 - 39, 92, 92);
        niva.setVisible (true);
        auto& klam = nytt<Ratt> (params::id::fx ("klam", "troskel"), stil::sv ("KLÄMMAN"), visare, 15.0f, RattTyp::liten, true);
        klam.setBounds (r.getX() + 152 - 26, r.getY() + 278 - 22, 52, 66);
        klam.setVisible (true);
        auto& kass = nytt<Ratt> (params::id::fx ("kass", "slitage"), "KASSETTEN", visare, 15.0f, RattTyp::liten);
        kass.setBounds (r.getX() + 226 - 26, r.getY() + 278 - 22, 52, 66);
        kass.setVisible (true);
    }

    void ritaVfd (juce::Graphics& g, Rectangle<float> v)
    {
        using namespace km::stil;
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (v.expanded (2.0f), 3.0f);
        g.setColour (Colour (0xff04110f));
        g.fillRoundedRectangle (v, 2.0f);
        const bool param = nu() - vfdTid < 1500.0;
        juce::String rad1, rad2;
        if (param) { rad1 = vfdNamn; rad2 = vfdVarde; }
        else if (diagnostikVarning) { rad1 = diagnostik1; rad2 = diagnostik2; }
        else if (visaPrestanda)
        {
            rad1 = stil::sv ("GRÄNSSNITTET (EN KÄRNA)");
            rad2 = cpuProcent < 0.0 ? stil::sv ("MÄTER …") : juce::String (cpuProcent, 1).replace (".", ",") + " % CPU";
        }
        else
        {
            const auto& e = proc.presets.getEntries();
            const int i = proc.presets.getCurrentIndex();
            rad1 = juce::isPositiveAndBelow (i, (int) e.size()) ? versaler (e[(size_t) i].category) : juce::String();
            if (proc.presets.lasMonster) rad1 << stil::sv ("  (LÅSTA MÖNSTER)");
            rad2 = versaler (proc.presets.getCurrentName());
        }
        auto skriv = [&] (const juce::String& t, float y, float p)
        {
            // Långa namn: mindre punkter (ned till 1,5) innan texten kortas.
            while (p > 1.5f && osd::bredd (t, p) > v.getWidth() - 12.0f) p -= 0.1f;
            y += (1.8f - juce::jmin (1.8f, p)) * 3.5f;   // kortare punkter: centrera i samma rad
            const int max = (int) ((v.getWidth() - 12.0f) / (6.0f * p));
            const auto text = t.substring (0, max);
            g.setColour (farg::vfd.withAlpha (0.25f));
            osd::ritaRaw (g, text, v.getX() + 5.5f, y - 0.5f, p, 1.0f);
            g.setColour (diagnostikVarning && ! param ? farg::lysOrange : farg::vfd);
            osd::ritaRaw (g, text, v.getX() + 6.0f, y, p, 0.75f);
        };
        skriv (rad1, v.getY() + 3.0f, 1.1f);
        skriv (rad2, v.getY() + 16.0f, 1.8f);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.07f), v.getX(), v.getY(), juce::Colours::transparentWhite, v.getX(), v.getCentreY(), false });
        g.fillRect (v.withHeight (v.getHeight() * 0.5f).reduced (1.0f, 0.5f));
    }

    void visaPresetMeny()
    {
        juce::PopupMenu m;
        fyllPresetMeny (m);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (vfd), [this] (int r) { if (r >= 1000) proc.presets.load (r - 1000); });
    }

    void fyllPresetMeny (juce::PopupMenu& m)
    {
        const auto& e = proc.presets.getEntries();
        juce::String kategori;
        juce::PopupMenu under;
        auto avsluta = [&] { if (kategori.isNotEmpty()) m.addSubMenu (kategori, under); under = {}; };
        for (int i = 0; i < (int) e.size(); ++i)
        {
            if (e[(size_t) i].category != kategori) { avsluta(); kategori = e[(size_t) i].category; }
            under.addItem (1000 + i, e[(size_t) i].name, true, i == proc.presets.getCurrentIndex());
        }
        avsluta();
    }

    void visaMeny()
    {
        juce::PopupMenu m, band, storlek;
        juce::PopupMenu presetar;
        fyllPresetMeny (presetar);
        m.addSubMenu ("Presets", presetar);
        m.addItem (1, stil::sv ("Spara preset…"));
        m.addItem (3, stil::sv ("Lås mönstren (presets byter bara ljudet)"), true, proc.presets.lasMonster);
        m.addItem (4, stil::sv ("Allt från MIDI (sekvensern tyst, presets behåller läget)"), true, param (params::id::midiAlla) > 0.5f);
        m.addSeparator();
        const char* bandNamn[] { "Av", "Lite", "Mycket" };
        for (int i = 0; i < 3; ++i) band.addItem (10 + i, bandNamn[i], true, (int) bandstyrka() == i);
        m.addSubMenu ("Bandstyrka", band);
        const float skala = (float) proc.panelLage.getProperty ("skala", 0.85f);
        for (int p : { 60, 70, 85, 100, 120, 140, 160 })
            storlek.addItem (100 + p, juce::String (p) + " %", true, std::abs (skala * 100.0f - (float) p) < 0.5f);
        m.addSubMenu ("Storlek", storlek);
        m.addItem (2, stil::sv ("Visa gränssnittets CPU"), true, visaPrestanda);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (vfd), [this] (int r)
        {
            if (r == 1) sparaPreset();
            else if (r == 3) { proc.presets.lasMonster = ! proc.presets.lasMonster; vfd->repaint(); }
            else if (r == 4) satt (params::id::midiAlla, param (params::id::midiAlla) > 0.5f ? 0.0f : 1.0f);
            else if (r == 2) { visaPrestanda = ! visaPrestanda; cpuProcent = -1.0; cpuStart = 0.0; }
            else if (r >= 10 && r < 13) sattBandstyrka ((Bandstyrka) (r - 10));
            else if (r >= 160 && r <= 260) { proc.panelLage.setProperty ("skala", (float) (r - 100) / 100.0f, nullptr); if (skalaAndrad) skalaAndrad ((float) (r - 100) / 100.0f); }
            else if (r >= 1000) proc.presets.load (r - 1000);
        });
    }

    // Scala-filerna: läs in .scl eller .kbm, ta bort kartan.
    void stamningsMeny()
    {
        juce::PopupMenu m;
        const auto namn = proc.egenStamningNamn();
        m.addSectionHeader (namn.isEmpty() ? juce::String ("Ingen egen skala inläst") : stil::sv ("Egen skala: ") + namn);
        m.addItem (1, stil::sv ("Läs in skala (.scl)…"));
        m.addItem (2, stil::sv ("Läs in tangentkarta (.kbm)…"));
        m.addItem (3, stil::sv ("Ta bort tangentkartan"), proc.harKarta());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this] (int r)
        {
            if (r == 3) { proc.taBortKarta(); return; }
            if (r != 1 && r != 2) return;
            valjare = std::make_unique<juce::FileChooser> (r == 1 ? stil::sv ("Välj en Scala-skala") : stil::sv ("Välj en tangentkarta"),
                                                           juce::File::getSpecialLocation (juce::File::userHomeDirectory), r == 1 ? "*.scl" : "*.kbm");
            valjare->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (! f.existsAsFile()) return;
                juce::String fel;
                if (proc.lasStamningsfil (f, fel))
                {
                    if (f.hasFileExtension ("scl")) satt (params::id::stamning, (float) (int) StamningTyp::Egen);
                    visare (stil::sv ("Stämning"), f.getFileNameWithoutExtension());
                }
                else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, stil::sv ("Kunde inte läsa filen"), f.getFileName() + ": " + fel);
            });
        });
    }
    std::unique_ptr<juce::FileChooser> valjare;

    void sparaPreset()
    {
        auto* w = new juce::AlertWindow ("Spara preset", "Namn:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("namn", proc.presets.getCurrentName());
        w->addButton ("Spara", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Avbryt", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
        {
            if (r == 1) proc.presets.saveUser (w->getTextEditorContents ("namn"));
        }), true);
    }

    // -----------------------------------------------------------------------------------------
    // Rattpanelen: vyknapparna och vyernas reglage

    void byggVyknappar()
    {
        using namespace km::stil;
        static const char* vyer[] { "KANAL", "SEKVENS", "MIXER", "EFFEKTER", "MODULATION", "LÅTEN" };
        for (int i = 0; i < 6; ++i)
            vyKnappar.push_back (&yta ({ 24 + i * 92 - 2, 628, 90, 24 }, [this, i] (juce::Graphics& g, Rectangle<float> r)
            {
                const bool a = (int) lage.vy == i;
                auto y = ritaKnapp (g, r.reduced (2.0f), a);
                ritaLysdiod (g, { y.getX() + 9.0f, y.getCentreY() }, 2.0f, farg::lysRod, a ? 1.0f : 0.0f);
                tryck (g, sv (vyer[i]), y.withTrimmedLeft (14.0f), 5.6f, juce::Justification::centred, a ? farg::vitt : farg::silver);
            },
            [this, i] { return (juce::int64) ((int) lage.vy == i); },
            [this, i] (const juce::MouseEvent&) { valjVy ((Vy) i, lage.kanal); }));
    }

    // Ljudbanken (KANAL-vyn): ◀ ljudets namn ▶ SPARA i rattpanelens list. Klick på namnet ger listan.
    void byggLjudvaljaren()
    {
        using namespace km::stil;
        auto pil = [this] (Rectangle<int> r, int steg)
        {
            auto nere = std::make_shared<bool> (false);
            auto& t = yta (r, [nere, steg] (juce::Graphics& g, Rectangle<float> rr)
            {
                auto a = ritaKnapp (g, rr.reduced (1.5f), *nere);
                g.setColour (farg::silver);
                g.fillPath (ikon (steg > 0 ? Ikon::spela : Ikon::bakat, a.reduced (7.0f, 4.0f)));
            });
            auto* tp = &t;
            t.tryck = [nere, tp] (const juce::MouseEvent&) { *nere = true; tp->repaint(); };
            t.slapp = [this, nere, tp, steg] (const juce::MouseEvent& e)
            {
                *nere = false; tp->repaint();
                if (! tp->getLocalBounds().contains (e.getPosition())) return;
                const int n = (int) proc.ljud.lista (ljudTyp (lage.kanal)).size();
                const int i = proc.ljud.aktuelltIndex (lage.kanal);
                laddaLjud (i < 0 ? (steg > 0 ? 0 : n - 1) : ((i + steg) % n + n) % n);
            };
            ljudYtor.push_back (tp);
        };
        pil ({ 616, 630, 28, 20 }, -1);
        auto& namn = yta ({ 646, 629, 200, 22 }, [this] (juce::Graphics& g, Rectangle<float> r)
        {
            g.setColour (juce::Colours::black);
            g.fillRoundedRectangle (r.reduced (1.0f), 2.5f);
            g.setColour (Colour (0xff04110f));
            g.fillRoundedRectangle (r.reduced (2.0f), 2.0f);
            auto n = proc.ljud.aktuelltNamn (lage.kanal);
            const auto text = versaler (n.isEmpty() ? stil::sv ("Från presetet") : n);
            float p = 1.6f;
            while (p > 1.1f && osd::bredd (text, p) > r.getWidth() - 12.0f) p -= 0.1f;
            g.setColour (n.isEmpty() ? farg::vfd.withAlpha (0.5f) : farg::vfd);
            osd::ritaRaw (g, text.substring (0, (int) ((r.getWidth() - 12.0f) / (6.0f * p))), r.getX() + 6.0f, r.getCentreY() - 3.5f * p, p, 0.75f);
        },
        [this] { return (juce::int64) proc.ljud.aktuelltNamn (lage.kanal).hashCode() * 4 + lage.kanal; },
        [this] (const juce::MouseEvent&) { visaLjudMeny(); });
        ljudYtor.push_back (&namn);
        pil ({ 848, 630, 28, 20 }, 1);
        auto nere = std::make_shared<bool> (false);
        auto& spara = yta ({ 880, 630, 56, 20 }, [nere] (juce::Graphics& g, Rectangle<float> r)
        {
            auto a = ritaKnapp (g, r.reduced (1.5f), *nere);
            tryck (g, "SPARA", a, 5.2f);
        });
        auto* sp = &spara;
        spara.tryck = [nere, sp] (const juce::MouseEvent&) { *nere = true; sp->repaint(); };
        spara.slapp = [this, nere, sp] (const juce::MouseEvent& e) { *nere = false; sp->repaint(); if (sp->getLocalBounds().contains (e.getPosition())) sparaLjud(); };
        ljudYtor.push_back (sp);
    }

    void laddaLjud (int i)
    {
        if (proc.ljud.ladda (lage.kanal, i))
            visare (stil::sv (ljudTyp (lage.kanal) == LjudTyp::Bas ? "BASLJUD" : (ljudTyp (lage.kanal) == LjudTyp::Trummor ? "TRUMLJUD" : "HUGGETS LJUD")),
                    versaler (proc.ljud.aktuelltNamn (lage.kanal)));
        andrat();
    }

    void visaLjudMeny()
    {
        const auto& l = proc.ljud.lista (ljudTyp (lage.kanal));
        juce::PopupMenu m, under;
        juce::String kategori;
        const int aktuell = proc.ljud.aktuelltIndex (lage.kanal);
        auto avsluta = [&] { if (kategori.isNotEmpty()) m.addSubMenu (kategori, under); under = {}; };
        for (int i = 0; i < (int) l.size(); ++i)
        {
            if (l[(size_t) i].kategori != kategori) { avsluta(); kategori = l[(size_t) i].kategori; }
            under.addItem (1 + i, l[(size_t) i].namn, true, i == aktuell);
        }
        avsluta();
        m.addSeparator();
        m.addItem (100000, stil::sv ("Spara kanalens ljud…"));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (ljudYtor[1]), [this] (int r)
        {
            if (r == 100000) sparaLjud();
            else if (r > 0) laddaLjud (r - 1);
        });
    }

    void sparaLjud()
    {
        auto* w = new juce::AlertWindow (stil::sv ("Spara ljud"), stil::sv ("Namn på ljudet för ") + kanalNamnet (lage.kanal).toLowerCase() + ":", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("namn", proc.ljud.aktuelltNamn (lage.kanal));
        w->addButton ("Spara", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Avbryt", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        const int kanal = lage.kanal;
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, kanal] (int r)
        {
            if (r == 1 && proc.ljud.sparaEget (kanal, w->getTextEditorContents ("namn"))) andrat();
        }), true);
    }

    int vyIndex() const
    {
        if (lage.vy == Vy::Kanal) return lage.kanal;
        return 3 + (int) lage.vy;
    }
    bool kanalVyn() const { return lage.vy == Vy::Kanal; }

    // Sidornas nummer: 0 SEKVENS, 1 MIXER, 2 EFFEKTER, 3 MODULATION, 4 LÅTEN, 5–8 KANAL för kanal 1–4.
    int sidIndex() const { return kanalVyn() ? 5 + lage.kanal : (int) lage.vy - 1; }

    Reglage* handling (const char* text, std::function<void()> gor)
    {
        auto t = handlingsknapp (stil::sv (text), std::move (gor));
        auto* p = t.get();
        addChildComponent (*p);
        ytor.push_back (std::move (t));
        return p;
    }

    // Trumväljaren (TR-stil): lysdioden visar vald trumma och blinkar vid slag.
    Tryckyta& trumKnapp (int t)
    {
        auto u = std::make_unique<Tryckyta>();
        u->bredd = 54;
        u->etikettText = trumKort (t);
        auto* p = u.get();
        u->rita = [this, t, p] (juce::Graphics& g, Rectangle<float> r)
        {
            const bool vald = lage.valdTrumma == t;
            const float slag = trumBlink[(size_t) t];
            stil::ritaLysdiod (g, { r.getCentreX(), 7.0f }, 2.3f, slag > 0.0f && ! vald ? farg::lysOrange : farg::lysRod,
                               vald ? 1.0f : slag);
            stil::ritaStegknapp (g, { r.getCentreX() - 17.0f, 13.0f, 34.0f, 22.0f }, t / 3, vald);
            etikett (g, p->etikettText, { 1.0f, 40.0f, r.getWidth() - 2.0f, 8.0f }, 4.8f, vald ? farg::vitt : farg::silver);
        };
        u->lage = [this, t] { return (juce::int64) (lage.valdTrumma == t) + 2 * (juce::int64) (trumBlink[(size_t) t] * 4.0f); };
        u->tryck = [this, t] (const juce::MouseEvent&) { lage.valdTrumma = t; andrat(); };
        addChildComponent (*u);
        ytor.push_back (std::move (u));
        return *p;
    }

    Tryckyta& reduktionsMatare (bool stor = false)
    {
        auto u = std::make_unique<Tryckyta>();
        u->bredd = stor ? 60 : 30;
        u->etikettText = stor ? "REDUKTION" : "RED.";
        if (stor) u->setSize (60, 200);
        auto* p = u.get();
        u->rita = [this, p, stor] (juce::Graphics& g, Rectangle<float> r)
        {
            const float h = stor ? r.getHeight() - 22.0f : 32.0f;
            stil::ritaLedStapel (g, { r.getCentreX() - (stor ? 5.0f : 2.5f), 4.0f, stor ? 10.0f : 5.0f, h }, juce::jlimit (0.0f, 1.0f, reduktion / 18.0f), stor ? 18 : 8, true);
            etikett (g, p->etikettText, { 0.0f, h + 8.0f, r.getWidth(), 8.0f });
            if (stor) etikett (g, juce::String (reduktion, 1).replace (".", ",") + " DB", { 0.0f, h + 18.0f - 8.0f + 8.0f, r.getWidth(), 7.0f }, 4.4f, farg::lysOrange);
        };
        u->lage = [this] { return (juce::int64) (reduktion * 2.0f); };
        addChildComponent (*u);
        ytor.push_back (std::move (u));
        return *p;
    }

    void visaVyn()
    {
        const int sida = sidIndex();
        for (auto& sk : sidKomp) sk.c->setVisible (sk.sida == sida && (! sk.synlig || sk.synlig()));
        for (auto* y : ljudYtor) y->setVisible (kanalVyn());
        for (auto* c : kanalYtor) c->setVisible (kanalVyn());
        for (auto* c : miniYtor) c->setVisible (true);
    }

    // -----------------------------------------------------------------------------------------
    // Sidorna (SEKVENS, MIXER, EFFEKTER, MODULATION, LÅTEN)

    struct Lada { int sida; Rectangle<float> r; juce::String titel; };
    struct SidKomp { int sida; juce::Component* c; std::function<bool()> synlig; };
    std::vector<Lada> lador;
    std::vector<SidKomp> sidKomp;

    void lada (int sida, Rectangle<float> r, const juce::String& titel) { lador.push_back ({ sida, r, titel }); }

    // Lägger en komponent på en sida, centrerad kring cx med överkanten y (designkoordinater).
    template <typename C>
    C& pa (int sida, C& c, float cx, float y, std::function<bool()> synlig = {})
    {
        int w = 0, h = 0;
        if constexpr (std::is_base_of_v<Reglage, C>) { w = c.cellBredd(); h = Reglage::hojd; }
        if constexpr (std::is_same_v<C, Tryckyta>) if (c.getHeight() > 0) { w = c.getWidth(); h = c.getHeight(); }
        if constexpr (std::is_same_v<C, Ratt>) h = c.hojdBehov();
        if (w <= 0) { w = c.getWidth(); h = c.getHeight(); }
        c.setBounds (juce::roundToInt (cx - (float) w * 0.5f), juce::roundToInt (y), w, h);
        sidKomp.push_back ({ sida, &c, std::move (synlig) });
        return c;
    }
    Ratt& stor (const juce::String& id, const char* text, float r = 17.0f)
    {
        auto& ra = nytt<Ratt> (id, stil::sv (text), visare, r, stil::RattTyp::liten);
        ra.visaVardet = true;
        return ra;
    }
    Fader& fader (const juce::String& id, const char* text, float x, float y, float h, int sida)
    {
        auto& f = nytt<Fader> (id, stil::sv (text), visare);
        f.setBounds ((int) x, (int) y, 44, (int) h);
        sidKomp.push_back ({ sida, &f, {} });
        return f;
    }

    void byggSidorna()
    {
        using namespace km::params;
        enum { sSekvens = 0, sMixer, sEffekter, sModulation, sLaten };
        const float x0 = (float) sidYta.getX() + 8.0f, y0 = (float) sidYta.getY() + 4.0f;   // 22, 128

        // --- SEKVENS: verktygsraden och rutnätet
        lada (sSekvens, { x0, y0, 862.0f, 84.0f }, stil::sv ("MÖNSTER"));
        for (int k = 0; k < antalKanaler; ++k)
        {
            auto vis = [this, k] { return lage.kanal == k; };
            pa (sSekvens, valj (id::kanal (k, "monster"), "Mönster", 70), x0 + 60.0f, y0 + 20.0f, vis);
            pa (sSekvens, valj (id::kanal (k, "variant"), "Variant", 70), x0 + 140.0f, y0 + 20.0f, vis);
            pa (sSekvens, ratt (id::kanal (k, "langd"), "Längd"), x0 + 216.0f, y0 + 20.0f, vis);
        }
        pa (sSekvens, ratt (id::gung, "Gung"), x0 + 290.0f, y0 + 20.0f);
        pa (sSekvens, *handling ("Kopiera A→B", [this] { kopieraAB(); }), x0 + 370.0f, y0 + 20.0f);
        pa (sSekvens, *handling ("Rensa", [this] { rensa(); }), x0 + 430.0f, y0 + 20.0f);
        sekvens.setBounds ((int) x0, (int) y0 + 92, 862, 430);
        sidKomp.push_back ({ sSekvens, &sekvens, {} });

        // --- MIXER: en remsa per kanal och Taktverket
        for (int k = 0; k < antalKanaler; ++k)
        {
            const float x = x0 + (float) k * 176.0f, cx = x + 84.0f;
            lada (sMixer, { x, y0, 168.0f, 522.0f }, kanalNamnet (k));
            pa (sMixer, valj (id::kanal (k, "drivetLage"), "Drivet", 76), x + 46.0f, y0 + 26.0f);
            pa (sMixer, ratt (id::kanal (k, "drivetMangd"), "Mängd"), x + 124.0f, y0 + 26.0f);
            pa (sMixer, stor (id::kanal (k, "sandA"), "→ Grindrum", 16.0f), x + 46.0f, y0 + 100.0f);
            pa (sMixer, stor (id::kanal (k, "sandB"), "→ Ekot", 16.0f), x + 124.0f, y0 + 100.0f);
            pa (sMixer, stor (id::kanal (k, "pan"), "Pan", 16.0f), cx, y0 + 190.0f);
            fader (id::kanal (k, "niva"), "Nivå", x + 42.0f, y0 + 282.0f, 180.0f, sMixer);
            pa (sMixer, nivaStapel (k), x + 118.0f, y0 + 290.0f);
            pa (sMixer, knapp (id::kanal (k, "tyst"), "Tyst"), x + 32.0f, y0 + 464.0f);
            pa (sMixer, knapp (id::kanal (k, "solo"), "Solo"), x + 84.0f, y0 + 464.0f);
            pa (sMixer, knapp (id::kanal (k, "midi"), "MIDI"), x + 136.0f, y0 + 464.0f);
        }
        {
            const float x = x0 + 4.0f * 176.0f;
            lada (sMixer, { x, y0, 158.0f, 522.0f }, "TAKT OCH MASTER");
            pa (sMixer, ratt (id::grundton, "Grundton"), x + 42.0f, y0 + 24.0f);
            pa (sMixer, ratt (id::gung, "Gung"), x + 116.0f, y0 + 24.0f);
            // Stämningen (fas 5): skalan, tonikan och Scala-filerna.
            pa (sMixer, valj (id::stamning, "Stämning", 132), x + 79.0f, y0 + 104.0f);
            pa (sMixer, valj (id::stamningTon, "Tonika", 60), x + 42.0f, y0 + 168.0f);
            pa (sMixer, *handling ("Scala…", [this] { stamningsMeny(); }), x + 116.0f, y0 + 168.0f);
            fader (id::master, "Master", x + 57.0f, y0 + 282.0f, 180.0f, sMixer);
        }

        // --- EFFEKTER: rummet, ekot och svajet överst, Klämman och Kassetten under
        {
            const float y2 = y0 + 266.0f;
            lada (sEffekter, { x0, y0, 268.0f, 258.0f }, "GRINDRUMMET");
            pa (sEffekter, stor (id::fx ("gr", "storlek"), "Storlek"), x0 + 72.0f, y0 + 30.0f);
            pa (sEffekter, stor (id::fx ("gr", "hallTid"), "Hålltid"), x0 + 196.0f, y0 + 30.0f);
            pa (sEffekter, stor (id::fx ("gr", "ton"), "Ton"), x0 + 72.0f, y0 + 142.0f);
            pa (sEffekter, stor (id::fx ("gr", "niva"), "Nivå"), x0 + 196.0f, y0 + 142.0f);

            const float ex = x0 + 276.0f;
            lada (sEffekter, { ex, y0, 336.0f, 258.0f }, "EKOT");
            pa (sEffekter, valj (id::fx ("eko", "tid"), "Tid", 90), ex + 60.0f, y0 + 30.0f);
            pa (sEffekter, valj (id::fx ("eko", "lage"), "Läge", 90), ex + 168.0f, y0 + 30.0f);
            pa (sEffekter, knapp (id::fx ("eko", "pingpong"), "Ping-pong"), ex + 276.0f, y0 + 30.0f);
            pa (sEffekter, stor (id::fx ("eko", "aterkoppling"), "Återkoppling"), ex + 60.0f, y0 + 142.0f);
            pa (sEffekter, stor (id::fx ("eko", "ton"), "Ton"), ex + 168.0f, y0 + 142.0f);
            pa (sEffekter, stor (id::fx ("eko", "niva"), "Nivå"), ex + 276.0f, y0 + 142.0f);

            const float sx = x0 + 620.0f;
            lada (sEffekter, { sx, y0, 242.0f, 258.0f }, "SVAJET");
            pa (sEffekter, valj (id::fx ("svaj", "lage"), "Läge", 100), sx + 121.0f, y0 + 30.0f);
            pa (sEffekter, stor (id::fx ("svaj", "mangd"), "Mängd", 22.0f), sx + 121.0f, y0 + 120.0f);

            lada (sEffekter, { x0, y2, 552.0f, 256.0f }, stil::sv ("KLÄMMAN"));
            const char* kl[] { "troskel", "kvot", "attack", "slapp", "pump", "efter" };
            const char* kn[] { "Tröskel", "Kvot", "Attack", "Släpp", "Pump", "Efter" };
            for (int i = 0; i < 6; ++i)
                pa (sEffekter, stor (id::fx ("klam", kl[i]), kn[i]), x0 + 72.0f + (float) (i % 3) * 112.0f, y2 + 28.0f + (float) (i / 3) * 112.0f);
            pa (sEffekter, reduktionsMatare (true), x0 + 470.0f, y2 + 30.0f);

            const float kx = x0 + 560.0f;
            lada (sEffekter, { kx, y2, 302.0f, 256.0f }, "KASSETTEN");
            pa (sEffekter, valj (id::fx ("kass", "kvalitet"), "Bandhastighet", 90), kx + 80.0f, y2 + 40.0f);
            pa (sEffekter, stor (id::fx ("kass", "slitage"), "Slitage", 24.0f), kx + 210.0f, y2 + 28.0f);
        }

        // --- MODULATION: LFO:erna, envelopes med faders och kopplingarna
        for (int n = 0; n < 2; ++n)
        {
            const float y = y0 + (float) n * 266.0f;
            lada (sModulation, { x0, y, 250.0f, 256.0f }, "LFO " + juce::String (n + 1));
            pa (sModulation, valj (id::lfo (n, "form"), "Form", 100), x0 + 66.0f, y + 34.0f);
            pa (sModulation, valj (id::lfo (n, "synk"), "Takt", 100), x0 + 66.0f, y + 120.0f);
            pa (sModulation, stor (id::lfo (n, "hz"), "Hastighet", 24.0f), x0 + 184.0f, y + 40.0f);

            const float ex = x0 + 258.0f;
            lada (sModulation, { ex, y, 284.0f, 256.0f }, "ENVELOPE " + juce::String (n + 1));
            const char* ev[] { "attack", "fall", "hall", "slapp" };
            const char* en[] { "A", "D", "S", "R" };
            for (int i = 0; i < 4; ++i) fader (id::menv (n, ev[i]), en[i], ex + 12.0f + (float) i * 48.0f, y + 26.0f, 210.0f, sModulation);
            pa (sModulation, valj (id::menv (n, "utlosare"), "Utlöses av", 76), ex + 240.0f, y + 90.0f);
        }
        {
            const float mx = x0 + 550.0f;
            lada (sModulation, { mx, y0, 312.0f, 522.0f }, "KOPPLINGARNA");
            for (int n = 0; n < antalKopplingar; ++n)
            {
                const float y = y0 + 24.0f + (float) n * 82.0f;
                pa (sModulation, valj (id::mod (n, "kalla"), "Källa", 92), mx + 58.0f, y);
                pa (sModulation, valj (id::mod (n, "mal"), "Mål", 112), mx + 170.0f, y);
                pa (sModulation, ratt (id::mod (n, "mangd"), "Mängd"), mx + 270.0f, y);
            }
        }

        // --- LÅTEN: verktygsraden och raderna
        lada (sLaten, { x0, y0, 862.0f, 84.0f }, stil::sv ("LÅTEN"));
        pa (sLaten, knapp (id::latLage, "Låtläget"), x0 + 50.0f, y0 + 20.0f);
        pa (sLaten, valj (id::latSlut, "Vid slutet", 76), x0 + 130.0f, y0 + 20.0f);
        pa (sLaten, *handling ("Kopiera", [this] { latKopiera(); }), x0 + 250.0f, y0 + 20.0f);
        pa (sLaten, *handling ("Ta bort", [this] { latTaBort(); }), x0 + 310.0f, y0 + 20.0f);
        pa (sLaten, *handling ("Hämta", [this] { latHamta(); }), x0 + 370.0f, y0 + 20.0f);
        latVy.setBounds ((int) x0, (int) y0 + 92, 862, 430);
        sidKomp.push_back ({ sLaten, &latVy, {} });
    }

    // KANAL-sidorna, en per kanal: skärmen uppe till höger, ljudets lådor runt och under den.
    void byggKanalsidor()
    {
        using namespace km::params;
        const float x0 = 22.0f, y0 = 96.0f, bred = 862.0f, vanster = 430.0f;
        const float yRad3 = 312.0f, yRad4 = 416.0f, yRad5 = 524.0f;
        // Lägger reglagen jämnt fördelade i en låda (överkant y).
        auto rad = [this] (int sida, Rectangle<float> l, std::initializer_list<Reglage*> k, float y)
        {
            const float steg = l.getWidth() / (float) k.size();
            float cx = l.getX() + steg * 0.5f;
            for (auto* r : k)
            {
                if (auto* ra = dynamic_cast<Ratt*> (r)) pa (sida, *ra, cx, y);
                else pa (sida, *r, cx, y);
                cx += steg;
            }
        };
        auto kanalLada = [&] (int sida, int k)
        {
            const Rectangle<float> l (x0, yRad5, bred, 94.0f);
            lada (sida, l, "KANAL");
            rad (sida, l, { &valj (id::kanal (k, "monster"), "Mönster", 70), &valj (id::kanal (k, "variant"), "Variant", 70),
                            &ratt (id::kanal (k, "langd"), "Längd"), &ratt (id::kanal (k, "niva"), "Nivå"), &ratt (id::kanal (k, "pan"), "Pan"),
                            &ratt (id::kanal (k, "sandA"), "→ Grindrum"), &ratt (id::kanal (k, "sandB"), "→ Ekot"),
                            &knapp (id::kanal (k, "midi"), "MIDI-spel") }, yRad5 + 24.0f);
        };
        auto drivetLada = [&] (int sida, int k, Rectangle<float> l)
        {
            lada (sida, l, "DRIVET");
            rad (sida, l, { &valj (id::kanal (k, "drivetLage"), "Läge", 90), &stor (id::kanal (k, "drivetMangd"), "Mängd", 15.0f) }, l.getY() + 22.0f);
        };

        // --- Kolvbasen och Kolvbas 2
        for (int k = 0; k < 2; ++k)
        {
            const int sida = 5 + k;
            auto kid = [k] (const char* p) { return id::kolv (k, p); };
            const Rectangle<float> osc (x0, y0, vanster, 100.0f), osc2 (x0, y0 + 108.0f, vanster, 100.0f);
            lada (sida, osc, "OSCILLATOR");
            rad (sida, osc, { &stor (kid ("vag"), "Vågform", 15.0f), &stor (kid ("pulsbredd"), "Pulsbredd", 15.0f), &stor (kid ("sub"), "Sub", 15.0f),
                              &stor (kid ("brus"), "Brus", 15.0f), &knapp (kid ("synk"), "Synk") }, y0 + 22.0f);
            lada (sida, osc2, "OSC 2");
            rad (sida, osc2, { &stor (kid ("osc2Niva"), "Nivå", 15.0f), &stor (kid ("osc2Halvtoner"), "Stämning", 15.0f), &stor (kid ("osc2Fin"), "Fin", 15.0f) }, osc2.getY() + 22.0f);
            const Rectangle<float> filt (x0, yRad3, bred, 96.0f);
            lada (sida, filt, "FILTER");
            rad (sida, filt, { &valj (kid ("filterTyp"), "Typ", 80), &stor (kid ("cutoff"), "Brytfrekvens", 15.0f), &stor (kid ("resonans"), "Resonans", 15.0f),
                               &stor (kid ("envMangd"), "Env-djup", 15.0f), &stor (kid ("filterFall"), "Env-fall", 15.0f), &stor (kid ("drive"), "Drive", 15.0f),
                               &stor (kid ("accent"), "Accent", 15.0f) }, yRad3 + 22.0f);
            const Rectangle<float> env (x0, yRad4, vanster, 100.0f);
            lada (sida, env, "ENVELOPE");
            rad (sida, env, { &stor (kid ("fall"), "Fall", 15.0f), &stor (kid ("hall"), "Håll", 15.0f), &stor (kid ("slapp"), "Släpp", 15.0f), &stor (kid ("glid"), "Glid", 15.0f) }, yRad4 + 22.0f);
            if (k == 1)
            {
                drivetLada (sida, k, { x0 + 438.0f, yRad4, 180.0f, 100.0f });
                const Rectangle<float> folj (x0 + 626.0f, yRad4, 236.0f, 100.0f);
                lada (sida, folj, stil::sv ("FÖLJ KOLVBASEN"));
                rad (sida, folj, { &knapp (id::folj, "Följ"), &ratt (id::forskjutning, "Förskjutning"), &ratt (id::transponering, "Transponering") }, yRad4 + 24.0f);
            }
            else drivetLada (sida, k, { x0 + 438.0f, yRad4, 424.0f, 100.0f });
            kanalLada (sida, k);
        }

        // --- Stampverket
        {
            const int sida = 5 + (int) Kanal::Stampverket;
            const Rectangle<float> kit (x0, y0, vanster, 100.0f);
            lada (sida, kit, "KIT");
            rad (sida, kit, { &valj (id::kit, "Kit", 100), &knapp (id::kantslag, "Kantslag") }, y0 + 24.0f);
            drivetLada (sida, (int) Kanal::Stampverket, { x0, y0 + 108.0f, vanster, 100.0f });
            const Rectangle<float> tr (x0, yRad3, bred, 96.0f);
            lada (sida, tr, "TRUMMOR");
            const float steg = bred / (float) antalSpar;
            for (int t = 0; t < antalSpar; ++t) pa (sida, trumKnapp (t), x0 + steg * ((float) t + 0.5f), yRad3 + 26.0f);
            const Rectangle<float> vald (x0, yRad4, bred, 100.0f);
            lada (sida, vald, "VALD TRUMMA");
            for (int t = 0; t < antalSpar; ++t)
            {
                auto vis = [this, t] { return lage.valdTrumma == t; };
                const Rectangle<float> l (x0 + 200.0f, yRad4, bred - 200.0f, 100.0f);
                const float st = l.getWidth() / 4.0f;
                pa (sida, stor (id::trumma (t, "stamning"), "Stämning", 15.0f), l.getX() + st * 0.5f, yRad4 + 22.0f, vis);
                pa (sida, stor (id::trumma (t, "klang"), "Klang", 15.0f), l.getX() + st * 1.5f, yRad4 + 22.0f, vis);
                pa (sida, stor (id::trumma (t, "niva"), "Nivå", 15.0f), l.getX() + st * 2.5f, yRad4 + 22.0f, vis);
                pa (sida, knapp (id::trumma (t, "tyst"), "Tyst"), l.getX() + st * 3.5f, yRad4 + 26.0f, vis);
            }
            auto namn = std::make_unique<Tryckyta>();
            namn->setSize (180, 60);
            namn->rita = [this] (juce::Graphics& g, Rectangle<float> r)
            {
                stil::rubrik (g, trumKort (lage.valdTrumma), r.withHeight (26.0f), 22.0f, farg::vitt, juce::Justification::centredLeft, 2.2f, 2.0f);
                stil::tryck (g, versaler (stil::sv (trumNamnFor ((Trumma) lage.valdTrumma, (Kit) (int) param (params::id::kit), param (params::id::kantslag) > 0.5f))), r.withTrimmedTop (34.0f).withHeight (8.0f), 5.6f,
                             juce::Justification::centredLeft, farg::silver);
            };
            namn->lage = [this] { return (juce::int64) lage.valdTrumma + 16 * (int) param (params::id::kit) + 256 * (param (params::id::kantslag) > 0.5f); };
            auto* np = namn.get();
            addChildComponent (*np);
            ytor.push_back (std::move (namn));
            pa (sida, *np, x0 + 104.0f, yRad4 + 28.0f);
            kanalLada (sida, (int) Kanal::Stampverket);
        }

        // --- Hugget
        {
            const int sida = 5 + (int) Kanal::Hugget;
            const Rectangle<float> hu (x0, y0, vanster, 208.0f);
            lada (sida, hu, "HUGGET");
            rad (sida, hu, { &valj (id::hu ("lage"), "Läge", 110), &valj (id::hu ("ackord"), "Ackord", 110), &ratt (id::hu ("oktav"), "Oktav") }, y0 + 26.0f);
            rad (sida, hu, { &stor (id::hu ("klang"), "Klang", 17.0f), &stor (id::hu ("sankning"), "Sänkning", 17.0f), &stor (id::hu ("bredd"), "Bredd", 17.0f) }, y0 + 104.0f);
            const Rectangle<float> env (x0, yRad3, bred, 96.0f);
            lada (sida, env, "ENVELOPE");
            rad (sida, env, { &stor (id::hu ("attack"), "Attack", 15.0f), &stor (id::hu ("fall"), "Fall", 15.0f), &stor (id::hu ("hall"), "Håll", 15.0f),
                              &stor (id::hu ("slapp"), "Släpp", 15.0f) }, yRad3 + 22.0f);
            drivetLada (sida, (int) Kanal::Hugget, { x0, yRad4, bred, 100.0f });
            kanalLada (sida, (int) Kanal::Hugget);
        }
    }

    // Kanalens nivå som lysdiodsstapel (MIXER).
    Tryckyta& nivaStapel (int k)
    {
        auto u = std::make_unique<Tryckyta>();
        u->setSize (16, 160);
        u->rita = [this, k] (juce::Graphics& g, Rectangle<float> r) { stil::ritaLedStapel (g, r.reduced (2.0f, 2.0f), kanalVisad[(size_t) k], 16, true); };
        u->lage = [this, k] { return (juce::int64) (kanalVisad[(size_t) k] * 16.0f); };
        auto* p = u.get();
        addChildComponent (*p);
        ytor.push_back (std::move (u));
        return *p;
    }

    // Liten kanalväljare under presets på sidorna: väljer kanal för stegraden, SEKVENS och klaviaturen.
    void byggMinikanaler()
    {
        using namespace km::stil;
        for (int i = 0; i < antalKanaler; ++i)
        {
            const Rectangle<int> k (916 + (i % 2) * 132, 486 + (i / 2) * 66, 128, 60);
            auto& t = yta (k, [this, i] (juce::Graphics& g, Rectangle<float> kr)
            {
                const bool aktiv = i == lage.kanal;
                const bool tyst = param (params::id::kanal (i, "tyst")) > 0.5f;
                auto y = ritaKnapp (g, kr.reduced (1.5f), aktiv, aktiv ? Colour (0xff29191a) : Colour (0xff222227));
                ritaLysdiod (g, { y.getX() + 10.0f, y.getY() + 11.0f }, 2.3f, farg::lysRod, aktiv ? 1.0f : 0.0f);
                tryck (g, "KANAL " + juce::String (i + 1), { y.getX() + 18.0f, y.getY() + 7.0f, 70.0f, 8.0f }, 4.8f, juce::Justification::centredLeft, farg::silverMatt);
                if (tyst) tryck (g, "TYST", { y.getX() + 8.0f, y.getBottom() - 16.0f, 60.0f, 8.0f }, 4.8f, juce::Justification::centredLeft, farg::tryckRott);
                rubrik (g, kanalNamnet (i), { y.getX() + 8.0f, y.getY() + 24.0f, y.getWidth() - 24.0f, 11.0f }, 9.5f,
                        aktiv ? farg::vitt : farg::silver, juce::Justification::centredLeft, aktiv ? 1.5f : 1.1f, 1.6f);
                ritaLedStapel (g, { y.getRight() - 9.0f, y.getY() + 6.0f, 4.0f, y.getHeight() - 12.0f }, kanalVisad[(size_t) i], 8, true);
            },
            [this, i] { return (juce::int64) (kanalVisad[(size_t) i] * 8.0f) + 16 * (i == lage.kanal) + 32 * (param (params::id::kanal (i, "tyst")) > 0.5f); },
            [this, i] (const juce::MouseEvent& e)
            {
                if (e.mods.isPopupMenu()) { satt (params::id::kanal (i, "tyst"), param (params::id::kanal (i, "tyst")) > 0.5f ? 0.0f : 1.0f); return; }
                valjVy (lage.vy, i);
            });
            miniYtor.push_back (&t);
        }
    }

    // -----------------------------------------------------------------------------------------
    // Mönsteråtgärder (SEKVENS-vyn)

    void kopieraAB()
    {
        const int k = lage.kanal, m = lage.monsterFor (k);
        if (k == (int) Kanal::Stampverket)
        {
            for (int r = 0; r < antalSpar; ++r)
                for (int s = 0; s < maxSteg; ++s) proc.banker.trummor.get (m, 1).set (r, s, proc.banker.trummor.get (m, 0).get (r, s));
        }
        else
        {
            auto& bank = proc.banker.ton (k);
            for (int s = 0; s < maxSteg; ++s) bank.get (m, 1).set (s, bank.get (m, 0).get (s));
        }
        visare (stil::sv ("MÖNSTER ") + juce::String (m + 1), stil::sv ("A KOPIERAT TILL B"));
    }

    void rensa()
    {
        if (lage.foljer()) return;
        if (lage.tonKanal()) lage.ton().rensa(); else lage.trum().rensa();
        visare (stil::sv ("MÖNSTER ") + juce::String (lage.monsterFor (lage.kanal) + 1) + (lage.redigera == 0 ? "A" : "B"), "RENSAT");
    }

    // Låtens rader (LÅTEN-vyn): verkar på vald rad.
    void latKopiera()
    {
        auto& l = proc.banker.laten;
        if (l.antal() == 0 || l.antal() >= Laten::maxRader) return;
        l.infoga (latVy.valdRad + 1, l.get (latVy.valdRad));
        latVy.valdRad = juce::jmin (latVy.valdRad + 1, l.antal() - 1);
        visare (stil::sv ("LÅTEN"), "RAD " + juce::String (latVy.valdRad + 1) + " KOPIERAD");
        andrat();
    }
    void latTaBort()
    {
        auto& l = proc.banker.laten;
        if (l.antal() == 0) return;
        l.taBort (latVy.valdRad);
        visare (stil::sv ("LÅTEN"), "RAD " + juce::String (latVy.valdRad + 1) + " BORTTAGEN");
        latVy.valdRad = juce::jlimit (0, juce::jmax (0, l.antal() - 1), latVy.valdRad);
        andrat();
    }
    void latHamta()
    {
        // Vald rad får panelens mönster, varianter och tystnad (takter, Spänningen och transponeringen står kvar).
        auto& l = proc.banker.laten;
        const auto ny = latVy.franPanelen();
        if (l.antal() == 0) { l.infoga (0, ny); latVy.valdRad = 0; }
        else
        {
            auto r = l.get (latVy.valdRad);
            r.monster = ny.monster; r.variant = ny.variant; r.tyst = ny.tyst;
            l.set (latVy.valdRad, r);
        }
        visare (stil::sv ("LÅTEN"), stil::sv ("RAD ") + juce::String (latVy.valdRad + 1) + stil::sv (" FRÅN PANELEN"));
        andrat();
    }

    // -----------------------------------------------------------------------------------------
    // Klaviaturen

    void byggKlaviaturen()
    {
        using namespace km::stil;
        const Rectangle<int> r (14, 756, 1172, 84);
        const Rectangle<int> v (r.getX(), r.getY() + 6, 104, r.getHeight() - 12);
        yta ({ v.getX() + 30, v.getY() + 18, 44, 26 }, [this] (juce::Graphics& g, Rectangle<float> rr)
        {
            ritaSjusegment (g, rr, "C" + juce::String (klaviatur.oktav));
        }, [this] { return (juce::int64) klaviatur.oktav; });
        auto pil = [&] (Rectangle<int> k, bool upp)
        {
            auto& t = yta (k, [upp] (juce::Graphics& g, Rectangle<float> rr)
            {
                auto a = ritaKnapp (g, rr.reduced (1.5f));
                g.setColour (farg::silver);
                g.fillPath (ikon (upp ? Ikon::spela : Ikon::bakat, a.reduced (5.0f, 4.0f)));
            });
            t.tryck = [this, upp] (const juce::MouseEvent&)
            {
                klaviatur.oktav = juce::jlimit (0, 6, klaviatur.oktav + (upp ? 1 : -1));
                oktavPerKanal[(size_t) lage.kanal] = klaviatur.oktav;
                klaviatur.repaint();
            };
        };
        pil ({ v.getX() + 4, v.getY() + 20, 24, 22 }, false);
        pil ({ v.getRight() - 28, v.getY() + 20, 24, 22 }, true);

        addAndMakeVisible (klaviatur);
        klaviatur.setBounds (r.getX() + 116, r.getY() + 8, r.getWidth() - 116, r.getHeight() - 12);
        klaviatur.nedslag = [this] (int notnr, float styrka) { stegskriv (notnr, styrka); };
    }

    // SPELA IN: tangenten skrivs in vid markören (stilla) eller i steget som spelas.
    void stegskriv (int notnr, float styrka)
    {
        if (! lage.spelaIn || lage.foljer()) return;
        const int spel = lage.spelsteg();
        const bool spelar = spel >= 0;
        const int steg = spelar ? spel : lage.markor;
        if (lage.tonKanal())
        {
            int bas = (int) param (params::id::grundton);
            if (lage.kanal == (int) Kanal::Hugget) bas += 12 * (2 + (int) param (params::id::hu ("oktav")));
            int h = notnr - bas, okt = 0;
            while (h > 12 && okt < 1) { h -= 12; ++okt; }
            while (h < -12 && okt > -1) { h += 12; --okt; }
            TonSteg s;
            s.slag = styrka > 0.85f ? 2 : 1;
            s.halvtoner = juce::jlimit (-12, 12, h);
            s.oktav = okt;
            lage.ton().set (steg, s);
        }
        else
        {
            const int spar = Stampverket::sparForNot (notnr);
            if (spar < 0) return;
            lage.valdTrumma = spar;
            auto s = lage.trum().get (spar, steg);
            s.slag = styrka > 0.85f ? 2 : 1;
            lage.trum().set (spar, steg, s);
        }
        if (! spelar)
        {
            const int langd = lage.langdFor (lage.kanal);
            lage.markor = (lage.markor + 1) % langd;
            lage.sida = lage.markor / 16;
            lage.sidaValdVid = nu();
        }
        andrat();
    }

    // -----------------------------------------------------------------------------------------
    // Bakgrunden (cachad)

    juce::int64 bakgrundsNyckel() const
    {
        return vyIndex() + 16 * lage.kanal + 256 * lage.valdTrumma;
    }

    void ritaBakgrund (float s)
    {
        using namespace km::stil;
        const int W = designBredd, H = designHojd;
        bakgrund = juce::Image (juce::Image::RGB, (int) std::ceil ((float) W * s), (int) std::ceil ((float) H * s), false);
        juce::Graphics g (bakgrund);
        g.addTransform (juce::AffineTransform::scale (s));
        ytorna.forbered (W, H, s);
        const Rectangle<float> r (0.0f, 0.0f, (float) W, (float) H);
        g.setFillType (ytorna.metallFyll());
        g.fillRect (r);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.03f), 0.0f, 0.0f, juce::Colours::black.withAlpha (0.18f), 0.0f, r.getBottom(), false });
        g.fillRect (r);

        // Bandverket.
        {
            const Rectangle<float> b (0.0f, 0.0f, r.getWidth(), 84.0f);
            g.setGradientFill ({ juce::Colours::white.withAlpha (0.045f), 0.0f, 0.0f, juce::Colours::transparentWhite, 0.0f, b.getBottom(), false });
            g.fillRect (b);
            prov::skara (g, b.getBottom(), 0.0f, b.getRight());
            rubrik (g, "KROPPSMASKINEN", { 26.0f, 18.0f, 300.0f, 24.0f }, 24.0f, farg::vitt, juce::Justification::centredLeft, 2.4f, 2.0f);
            tryck (g, sv ("EBM · NEW BEAT · RYTM- OCH BASMASKIN"), { 27.0f, 52.0f, 300.0f, 8.0f }, 5.5f, juce::Justification::centredLeft, farg::silverMatt);
            const Rectangle<float> blk (270.0f, 18.0f, 82.0f, 24.0f);
            g.setColour (farg::tryckRott);
            g.fillPath (parallellogram (blk, 9.0f));
            rubrik (g, "KM-87", blk.translated (1.0f, 0.0f), 14.0f, farg::vitt, juce::Justification::centred, 2.0f, 2.2f);
            ritaDiagonaler (g, { 352.0f, 18.0f, 30.0f, 24.0f }, farg::tryckRott, 3.0f, 8.0f, true);
            g.setColour (farg::tryckRott);
            g.fillRect (270.0f, 48.0f, 112.0f, 1.5f);
            ritaPil (g, { 330.0f, 56.0f }, { 382.0f, 56.0f }, 1.5f, farg::tryckRott);
            const char* etiketter[] { "SPOLA", "SPELA", "STOPP", "SPELA IN" };
            for (int i = 0; i < 4; ++i)
                tryck (g, etiketter[i], { 402.0f + (float) i * 50.0f, 10.0f, 54.0f, 8.0f }, 5.5f);
            tryck (g, sv ("RÄKNEVERK"), { 612.0f, 10.0f, 120.0f, 8.0f }, 5.5f);
            tryck (g, "BAND", { 742.0f, 10.0f, 36.0f, 8.0f }, 5.5f);
            tryck (g, "TEMPO", { 784.0f, 10.0f, 100.0f, 8.0f }, 5.5f);
            tryck (g, "SLAG/MIN", { 784.0f, 62.0f, 100.0f, 8.0f }, 5.0f, juce::Justification::centred, farg::silverMatt);
            tryck (g, "VARVTALET", { 900.0f, 10.0f, 124.0f, 8.0f }, 5.5f);
            g.setColour (farg::tryckRott);
            linjeskrift::rita (g, sv ("SPÄNNINGEN"), { 1036.0f, 10.0f, 146.0f, 8.0f }, { 6.0f, 1.1f, 2.4f, 1.0f }, juce::Justification::centred);
            tryck (g, "LUGN", { 1036.0f, 62.0f, 60.0f, 8.0f }, 5.0f, juce::Justification::centredLeft, farg::silverMatt);
            tryck (g, "HELVETE", { 1122.0f, 62.0f, 60.0f, 8.0f }, 5.0f, juce::Justification::centredRight, farg::silverMatt);
        }

        // Presets och master.
        {
            const Rectangle<float> p (906.0f, 96.0f, 280.0f, 352.0f);
            ritaInfalltFalt (g, p, ytorna);
            tryck (g, "PRESETS", { p.getX(), p.getY() + 8.0f, p.getWidth(), 8.0f }, 5.6f);
            prov::skara (g, p.getY() + 212.0f, p.getX() + 10.0f, p.getRight() - 10.0f);
            prov::gruppRubrik (g, "MASTER", p.getX() + 14.0f, p.getRight() - 14.0f, p.getY() + 228.0f);
        }

        prov::skara (g, 462.0f, 900.0f, r.getWidth());

        // Sidorna: fältet, lådorna och den lilla kanalväljaren. I KANAL-vyn också skärmens infattning.
        {
            ritaInfalltFalt (g, sidYta.toFloat(), ytorna);
            for (const auto& l : lador)
                if (l.sida == sidIndex())
                {
                    g.setColour (juce::Colours::white.withAlpha (0.035f));
                    g.fillRoundedRectangle (l.r, 3.0f);
                    g.setColour (juce::Colours::black.withAlpha (0.6f));
                    g.drawRoundedRectangle (l.r, 3.0f, 1.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.06f));
                    g.drawRoundedRectangle (l.r.translated (0.0f, 1.0f), 3.0f, 0.6f);
                    tryck (g, l.titel, { l.r.getX() + 10.0f, l.r.getY() + 6.0f, l.r.getWidth() - 20.0f, 8.0f }, 5.8f, juce::Justification::centredLeft, farg::silver, 0.9f, 2.2f);
                    g.setColour (farg::silverMatt.withAlpha (0.6f));
                    g.fillRect (l.r.getX() + 10.0f, l.r.getY() + 18.0f, l.r.getWidth() - 20.0f, 0.8f);
                }
            if (kanalVyn())
            {
                juce::Graphics::ScopedSaveState spara (g);
                g.addTransform (skarmTransform());
                const Rectangle<float> yttre (208.0f, 92.0f, 684.0f, 336.0f);
                skarm::ritaInfattning (g, yttre, glas, ytorna);
            }
            const Rectangle<float> mk (906.0f, 466.0f, 280.0f, 156.0f);
            ritaInfalltFalt (g, mk, ytorna);
            tryck (g, "KANAL", { mk.getX(), mk.getY() + 8.0f, mk.getWidth(), 8.0f }, 5.6f);
        }

        // Vylisten under rattarna (samma plats i alla vyer): vyknapparna, LJUD i KANAL-vyn och kanalens
        // eller vyns namn.
        {
            const Rectangle<float> blk (950.0f, 630.0f, 30.0f, 20.0f);
            const bool kanalVy = lage.vy == Vy::Kanal || lage.vy == Vy::Sekvens;
            g.setColour (farg::tryckRott);
            g.fillPath (parallellogram (blk, 6.0f));
            static const char* vyNamn[] { "", "", "MIXER", "EFFEKTER", "MODULATION", "LÅTEN" };
            rubrik (g, kanalVy ? juce::String (lage.kanal + 1) : juce::String (lage.vy == Vy::Laten ? "L" : "M"), blk, 12.0f, farg::vitt, juce::Justification::centred, 1.8f);
            rubrik (g, kanalVy ? kanalNamnet (lage.kanal) : sv (vyNamn[(int) lage.vy]), { blk.getRight() + 8.0f, blk.getY() + 2.0f, 200.0f, 16.0f },
                    14.0f, farg::vitt, juce::Justification::centredLeft, 1.9f, 2.2f);
            if (lage.vy == Vy::Kanal) tryck (g, "LJUD", { 580.0f, 636.0f, 32.0f, 8.0f }, 5.4f, juce::Justification::centredLeft);
            else ritaDiagonaler (g, { 586.0f, 635.0f, 354.0f, 10.0f }, juce::Colours::white.withAlpha (0.06f), 2.0f, 6.0f);
        }

        // Stegraden.
        stegrad.ritaTryck (g, stegrad.getPosition().toFloat());

        // Klaviaturen.
        {
            const Rectangle<float> kr (14.0f, 756.0f, 1172.0f, 84.0f);
            prov::skara (g, kr.getY(), 0.0f, r.getWidth());
            const auto v = Rectangle<float> (kr.getX(), kr.getY() + 6.0f, 104.0f, kr.getHeight() - 12.0f);
            ritaInfalltFalt (g, v, ytorna);
            tryck (g, "OKTAV", { v.getX(), v.getY() + 6.0f, v.getWidth(), 8.0f }, 5.6f);
            static const char* spelar[] { "SPELAR KOLVBASEN", "SPELAR KOLVBAS 2", "SPELAR STAMPVERKET", "SPELAR HUGGET" };
            tryck (g, spelar[lage.kanal], { v.getX(), v.getBottom() - 11.0f, v.getWidth(), 8.0f }, 4.0f, juce::Justification::centred, farg::silverMatt, 0.7f, 1.2f);
        }

        for (auto p : { Point<float> (7.0f, 7.0f), { r.getRight() - 7.0f, 7.0f }, { 7.0f, r.getBottom() - 7.0f }, { r.getRight() - 7.0f, r.getBottom() - 7.0f } })
            ritaSkruv (g, p, 3.6f, 0.3f + p.x * 0.01f);
        g.setColour (juce::Colours::black);
        g.drawRect (r, 1.0f);
    }

    // -----------------------------------------------------------------------------------------
    // Ändringar och timern

    void andrat()
    {
        visaVyn();
        for (auto& t : ytor) if (t->isVisible()) t->repaint();
        stegrad.repaint();
        sekvens.repaint();
        klaviatur.repaint();
        repaint();   // bakgrunden byggs om om nyckeln ändrats
    }

    void timerCallback() override
    {
        const double t = nu();
        if (spolaNed && t - spolaTid > 150.0) spolaNed = false;
        if (stoppNed && t - stoppTid > 150.0) stoppNed = false;

        // Nivåer: kanalerna och VU (enkel ballistik).
        for (int k = 0; k < antalKanaler; ++k)
        {
            const float topp = proc.kanalTopp ((Kanal) k);
            const float niva = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (topp, -48.0f) + 48.0f) / 48.0f);
            kanalVisad[(size_t) k] = juce::jmax (niva, kanalVisad[(size_t) k] - 0.06f);
            kanalNiva[(size_t) k] = topp;
        }
        float utMax = 0.0f;
        for (int c = 0; c < 2; ++c)
        {
            const float topp = proc.utTopp[(size_t) c].exchange (0.0f);
            utMax = juce::jmax (utMax, topp);
            const float mal = juce::Decibels::gainToDecibels (topp, -40.0f);
            auto& v = vuDb[(size_t) c];
            v += (mal - v) * (mal > v ? 0.45f : 0.12f);
        }
        reduktion = juce::jmax (proc.klamReduktionDb(), reduktion * 0.85f);

        // Trumslagen blinkar trumväljarens lysdioder.
        for (int s = 0; s < antalSpar; ++s)
        {
            const auto n = proc.trumSlag (s);
            if (n != trumForra[(size_t) s]) { trumForra[(size_t) s] = n; trumBlink[(size_t) s] = 1.0f; }
            else trumBlink[(size_t) s] = juce::jmax (0.0f, trumBlink[(size_t) s] - 0.25f);
        }

        // Diagnostik var tredje sekund: ljudkortet som går i fel takt (t.ex. Bluetooth-lurar på 16 kHz).
        if (t - matStart >= 3000.0)
        {
            const auto sampel = proc.antalSampel.load();
            const double sr = proc.forberedd.load(), dt = (t - matStart) * 0.001;
            if (matStart > 0.0 && sr > 0.0 && sampel > matSampel)
            {
                const double realtid = (double) (sampel - matSampel) / dt / sr;
                diagnostikVarning = realtid < 0.95 || realtid > 1.05 || sr < 32000.0;
                diagnostik1 = stil::sv ("LJUDKORTET ") + juce::String (sr / 1000.0, 1).replace (".", ",") + " KHZ";
                diagnostik2 = juce::String (realtid, 2).replace (".", ",") + stil::sv ("× TAKTEN – VÄLJ 48 KHZ");
            }
            matStart = t; matSampel = sampel;
        }

        // Gränssnittets processortid (MENY → Visa gränssnittets CPU), medel över två sekunder.
        if (visaPrestanda && t - cpuStart >= 2000.0)
        {
            const double c = tradensCpuMs();
            if (cpuStart > 0.0) cpuProcent = (c - cpuForra) / (t - cpuStart) * 100.0;
            cpuStart = t; cpuForra = c;
        }

        for (auto& y : ytor) y->kolla();
        stegrad.tick();
        if (sekvens.isVisible()) sekvens.tick();
        if (latVy.isVisible()) latVy.tick();
        klaviatur.tick();
        if (bild.isVisible()) bild.uppdatera (skarmData (utMax));
    }

    SkarmData skarmData (float utMax)
    {
        SkarmData d;
        d.kanal = lage.kanal;
        d.vy = lage.vy;
        const double ppq = proc.ppqNu();
        d.spelar = ppq >= 0.0;
        const int kalla = lage.foljer() ? 0 : lage.kanal;
        d.steg = proc.stegNu ((Kanal) lage.kanal);
        d.stegFas = d.spelar ? (float) (ppq * 4.0 - std::floor (ppq * 4.0)) : 0.0f;
        d.langd = lage.langdFor (kalla);
        const int v = (int) param (params::id::kanal (kalla, "variant"));
        const int m = lage.monsterFor (kalla);
        d.monster = m;
        d.variant = v == 1 ? 'B' : 'A';
        const int variant = v == 1 ? 1 : 0;
        for (int s = 0; s < maxSteg; ++s)
        {
            int slag;
            if (lage.kanal == (int) Kanal::Stampverket) slag = proc.banker.trummor.get (m, variant).get (lage.valdTrumma, s).slag;
            else slag = proc.banker.ton (kalla).get (m, variant).get (s).slag;
            d.stegNiva[(size_t) s] = slag == 2 ? 1.0f : (slag == 1 ? 0.6f : 0.0f);
        }
        for (int k = 0; k < antalKanaler; ++k)
        {
            d.kanalSlag[(size_t) k] = proc.kanalSlag ((Kanal) k);
            d.kanalNiva[(size_t) k] = kanalNiva[(size_t) k];
        }
        for (int s = 0; s < antalSpar; ++s) d.trumSlag[(size_t) s] = trumForra[(size_t) s];
        d.valdTrumma = lage.valdTrumma;
        d.niva = utMax;
        proc.lasScope (d.scope.data(), (int) d.scope.size());
        for (int n = 0; n < 2; ++n)
        {
            d.lfoVarde[(size_t) n] = proc.modKalla (n == 0 ? ModKalla::Lfo1 : ModKalla::Lfo2);
            d.lfoForm[(size_t) n] = (int) param (params::id::lfo (n, "form"));
            d.menvVarde[(size_t) n] = proc.modKalla (n == 0 ? ModKalla::Env1 : ModKalla::Env2);
        }
        d.osdLage = lage.spelaIn ? stil::sv ("● INSP") : (d.spelar ? stil::sv ("▶ SPELA") : stil::sv ("■ STOPP"));
        static const char* kv[] { "SP", "LP", "EP" };
        d.osdKvalitet = kv[juce::jlimit (0, 2, (int) param (params::id::fx ("kass", "kvalitet")))];
        d.osdRakneverk = tidText (proc.speltid());
        static const char* vyNamn[] { "", "SEKVENS", "MIXER", "EFFEKTER", "MODULATION", "LÅTEN" };
        d.osdKanal = lage.vy == Vy::Kanal ? kanalNamnet (lage.kanal) : stil::sv (vyNamn[(int) lage.vy]);
        if (const int rad = proc.latRad(); rad >= 0 && lage.vy == Vy::Laten)
            d.osdKanal = "RAD " + juce::String (rad + 1).paddedLeft ('0', 2) + " " + juce::String (proc.latTakt() + 1) + "/"
                       + juce::String (proc.banker.laten.get (rad).takter);
        d.bandstyrka = bandstyrka();
        d.slitage = param (params::id::fx ("kass", "slitage"));
        return d;
    }

    // -----------------------------------------------------------------------------------------

    KroppsmaskinenProcessor& proc;
    PanelLage lage;
    Visare visare;
    const Rectangle<int> falt { 14, 470, 1172, 152 };      // KANAL-vyns rattpanel
    const Rectangle<int> sidYta { 14, 92, 878, 530 };      // sidornas yta (vyknapparna ligger under)
    const Rectangle<float> glas { 230.0f, 108.0f, 640.0f, 304.0f };

    std::vector<std::unique_ptr<Reglage>> reglage;
    std::vector<std::unique_ptr<Tryckyta>> ytor;
    Stegrad stegrad;
    Sekvens sekvens;
    LatVy latVy;
    Klaviatur klaviatur;
    Bildskarm bild;
    Tryckyta* vfd = nullptr;
    std::vector<Tryckyta*> ljudYtor, miniYtor, vyKnappar;
    std::vector<juce::Component*> kanalYtor;   // kanalväljaren och bildskärmen: bara i KANAL-vyn
    static inline float bildUpplosning = 1.0f;   // skärmens interna upplösning (andel av den fysiska)
    Tryckyta* hjulMonster = nullptr;

    stil::Ytor ytorna;
    juce::Image bakgrund;
    float bgSkala = 0.0f;
    juce::int64 bgNyckel = -1;

    juce::String vfdNamn, vfdVarde;
    double vfdTid = -1e9;
    bool spolaNed = false, stoppNed = false;
    double spolaTid = 0.0, stoppTid = 0.0;
    int monsterStart = 0;
    float monsterStartY = 0.0f;
    bool monsterDrog = false;
    std::array<int, 4> oktavPerKanal { 1, 1, 2, 3 };

    std::array<float, antalKanaler> kanalVisad {}, kanalNiva {};
    std::array<std::array<Bildminne, 2>, 2> vuMinne;
    std::array<Bildminne, 4> kanalMinne;
    std::array<float, 2> vuDb { -40.0f, -40.0f };
    float reduktion = 0.0f;
    std::array<uint32_t, antalSpar> trumForra {};
    std::array<float, antalSpar> trumBlink {};

    double matStart = 0.0;
    juce::int64 matSampel = 0;
    bool diagnostikVarning = false;
    bool visaPrestanda = false;
    double cpuProcent = -1.0, cpuStart = 0.0, cpuForra = 0.0;
    juce::String diagnostik1, diagnostik2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Panel)
};

} // namespace km::vhs
