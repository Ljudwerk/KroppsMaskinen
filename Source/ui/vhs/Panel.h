// VHS-panelen (fas 3): hela gränssnittet i designstorlek 1200 × 840, skalas som helhet av editorn.
//
// Sektionerna följer stilprovet: Bandverket överst (transport, räkneverk, bandhastighet, tempo,
// Varvtalet, Spänningen), kanalväljaren till vänster, bildskärmen i mitten, presets/VU/master till
// höger, rattpanelen med vyerna KANAL · SEKVENS · MIXER · EFFEKTER · MODULATION, stegraden och
// klaviaturen.
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
        lage.andrat = [this] { andrat(); };
        visare.visa = [this] (const juce::String& n, const juce::String& v) { vfdNamn = versaler (n); vfdVarde = versaler (v); vfdTid = nu(); vfd->repaint(); };

        byggBandverket();
        byggKanalvaljaren();
        byggBildskarmen();
        byggPresets();
        byggVyknappar();
        byggLjudvaljaren();
        byggVyerna();
        addAndMakeVisible (stegrad);
        stegrad.setBounds (14, 628, 1172, 92);
        addAndMakeVisible (sekvens);
        sekvens.setBounds (falt.getX() + 222, falt.getY() + 6, falt.getWidth() - 232, falt.getHeight() - 12);
        addAndMakeVisible (latVy);
        latVy.setBounds (sekvens.getBounds());
        byggKlaviaturen();

        layoutVyerna();
        valjVy (Vy::Kanal, 0);
        startTimerHz (30);
    }

    ~Panel() override { stopTimer(); }

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
        for (auto* id : { params::id::spela, params::id::varvtal, params::id::varvLage })
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
    { return nytt<Ratt> (id, stil::sv (text), visare, 12.0f, stil::RattTyp::liten, inv); }
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
                return i == 1 ? spelar : (i == 2 ? ! spelar : (i == 3 ? lage.spelaIn : spolaNed));
            };
            yta (k, [aktiv, i, ik = ikoner[i]] (juce::Graphics& g, Rectangle<float> r)
            {
                const bool a = aktiv();
                const auto kr = r.withTrimmedTop (2.0f).withTrimmedBottom (2.0f);
                auto y = ritaKnapp (g, kr, a && i != 2);
                const auto p = ikon (ik, y.reduced (10.0f, 5.0f));
                if (a && i != 2)
                {
                    g.setColour (farg::lysRod.withAlpha (0.25f));
                    g.strokePath (p, juce::PathStrokeType (3.0f));
                    g.setColour (i == 3 ? farg::lysRod : farg::lysRod);
                }
                else g.setColour (i == 3 ? farg::tryckRott : farg::silver);
                g.fillPath (p);
                if (a && i != 2) ritaLysdiod (g, { kr.getRight() - 6.0f, kr.getY() + 6.0f }, 1.8f, farg::lysRod,
                                              i == 3 && (juce::Time::getMillisecondCounter() / 400) % 2 ? 0.4f : 1.0f);
            },
            [aktiv, i] { return (juce::int64) aktiv() * 2 + (i == 3 && aktiv() ? (juce::int64) ((juce::Time::getMillisecondCounter() / 400) % 2) * 4 : 0); },
            [this, i] (const juce::MouseEvent&)
            {
                if (i == 0) { proc.spola(); lage.markor = 0; spolaNed = true; spolaTid = nu(); }
                else if (i == 1) satt (params::id::spela, 1.0f);
                else if (i == 2) satt (params::id::spela, 0.0f);
                else { lage.spelaIn = ! lage.spelaIn; if (lage.spelaIn) lage.markor = lage.sida * 16; }
                andrat();
            });
        }

        // Räkneverket: speltid.
        yta ({ 612, 24, 120, 32 }, [this] (juce::Graphics& g, Rectangle<float> r) { ritaSjusegment (g, r, tidText (proc.speltid())); },
             [this] { return (juce::int64) proc.speltid(); });

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
    // Kanalväljaren

    void byggKanalvaljaren()
    {
        using namespace km::stil;
        const Rectangle<int> r (14, 96, 178, 352);
        static const char* typ[] { "SEKVENSBAS", "ANDRA BASEN", "TRUMMOR", "STÖTAR OCH LEADS" };
        for (int i = 0; i < 4; ++i)
        {
            const Rectangle<int> k (r.getX() + 10, r.getY() + 26 + i * 60, r.getWidth() - 20, 50);
            yta (k, [this, i] (juce::Graphics& g, Rectangle<float> kr)
            {
                const bool aktiv = i == lage.kanal;
                const bool tyst = param (params::id::kanal (i, "tyst")) > 0.5f;
                kanalMinne[(size_t) i].rita (g, kr, (int) aktiv + 2 * (int) tyst, [aktiv, tyst, i] (juce::Graphics& gk, Rectangle<float> q)
                {
                    auto y = ritaKnapp (gk, q.reduced (1.5f), aktiv, aktiv ? Colour (0xff29191a) : Colour (0xff222227));
                    ritaLysdiod (gk, { y.getX() + 10.0f, y.getY() + 11.0f }, 2.3f, farg::lysRod, aktiv ? 1.0f : 0.0f);
                    tryck (gk, "KANAL " + juce::String (i + 1), { y.getX() + 18.0f, y.getY() + 7.0f, 90.0f, 8.0f }, 5.0f,
                           juce::Justification::centredLeft, farg::silverMatt);
                    if (tyst) tryck (gk, "TYST", { y.getRight() - 60.0f, y.getY() + 7.0f, 44.0f, 8.0f }, 5.0f, juce::Justification::centredRight, farg::tryckRott);
                    rubrik (gk, kanalNamnet (i), { y.getX() + 8.0f, y.getY() + 18.0f, 150.0f, 12.0f }, 11.0f,
                            aktiv ? farg::vitt : farg::silver, juce::Justification::centredLeft, aktiv ? 1.6f : 1.2f, 2.0f);
                    tryck (gk, sv (typ[i]), { y.getX() + 8.0f, y.getY() + 33.0f, 150.0f, 8.0f }, 4.6f, juce::Justification::centredLeft, farg::silverMatt, 0.7f, 2.0f);
                });
                const auto y = kr.reduced (1.5f).translated (0.0f, aktiv ? 0.8f : 0.0f).withTrimmedBottom (aktiv ? 1.8f : 2.5f).reduced (0.6f, 0.4f);
                ritaLedStapel (g, { y.getRight() - 9.0f, y.getY() + 6.0f, 4.0f, y.getHeight() - 12.0f }, kanalVisad[(size_t) i], 8, true);
            },
            [this, i]
            {
                return (juce::int64) (kanalVisad[(size_t) i] * 8.0f) + 16 * (i == lage.kanal) + 32 * (param (params::id::kanal (i, "tyst")) > 0.5f);
            },
            [this, i] (const juce::MouseEvent& e)
            {
                if (e.mods.isPopupMenu()) { satt (params::id::kanal (i, "tyst"), param (params::id::kanal (i, "tyst")) > 0.5f ? 0.0f : 1.0f); return; }
                valjVy (lage.vy, i);
            });
        }

        // Mönster för vald kanal: dra eller rulla för mönstret, klicka för varianten.
        const float y = (float) r.getY() + 280.0f;
        auto& m = yta ({ r.getX() + 12, (int) y + 14, 74, 30 }, [this] (juce::Graphics& g, Rectangle<float> rr)
        {
            const int v = (int) param (params::id::kanal (lage.kanal, "variant"));
            const juce::String bokstav = v == 0 ? "A" : (v == 1 ? "B" : "-");
            ritaSjusegment (g, rr, bokstav + juce::String (lage.monsterFor (lage.kanal) + 1).paddedLeft ('0', 2));
        },
        [this] { return (juce::int64) lage.monsterFor (lage.kanal) * 8 + (int) param (params::id::kanal (lage.kanal, "variant")) + lage.kanal * 1000; });
        m.tryck = [this] (const juce::MouseEvent& e) { monsterStart = lage.monsterFor (lage.kanal); monsterStartY = e.position.y; monsterDrog = false; };
        m.drag = [this] (const juce::MouseEvent& e)
        {
            const int d = (int) ((monsterStartY - e.position.y) / 8.0f);
            if (d == 0 && ! monsterDrog) return;
            monsterDrog = true;
            satt (params::id::kanal (lage.kanal, "monster"), (float) juce::jlimit (1, antalMonster, monsterStart + 1 + d));
        };
        m.slapp = [this] (const juce::MouseEvent&)
        {
            if (! monsterDrog) satt (params::id::kanal (lage.kanal, "variant"), (float) (((int) param (params::id::kanal (lage.kanal, "variant")) + 1) % 3));
        };
        m.addMouseListener (this, false);
        hjulMonster = &m;

        yta ({ r.getX() + 100, (int) y + 12, 74, 40 }, [this] (juce::Graphics& g, Rectangle<float> rr)
        {
            const int v = (int) param (params::id::kanal (lage.kanal, "variant"));
            const char* t[] { "A", "B", "A+B" };
            for (int i = 0; i < 3; ++i) prov::diodMedText (g, { rr.getX() + 4.0f, rr.getY() + 8.0f + 12.0f * (float) i }, t[i], i == v);
        },
        [this] { return (juce::int64) param (params::id::kanal (lage.kanal, "variant")) + lage.kanal * 10; },
        [this] (const juce::MouseEvent& e) { satt (params::id::kanal (lage.kanal, "variant"), (float) juce::jlimit (0, 2, (int) ((e.position.y - 2.0f) / 12.0f))); });
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (e.eventComponent == hjulMonster)
            satt (params::id::kanal (lage.kanal, "monster"),
                  (float) juce::jlimit (1, antalMonster, lage.monsterFor (lage.kanal) + 1 + ((w.deltaY > 0.0f) != w.isReversed ? 1 : -1)));
    }

    // -----------------------------------------------------------------------------------------
    // Bildskärmen

    void byggBildskarmen()
    {
        using namespace km::stil;
        addAndMakeVisible (bild);
        bild.setBounds (glas.toNearestInt());
        // Bandlagret räknas högst i designupplösning (640 × 304, ungefär en VHS-bild) och sträcks ut på
        // Retina och vid förstoring; texten (OSD) ritas skarp ovanpå. Håller gränssnittet under CPU-målet.
        bild.setUpplosning (bildUpplosning, 640, 304);
        yta ({ 892 - 216, 452 - 27, 160, 14 }, [this] (juce::Graphics& g, Rectangle<float> r)
        {
            const int b = (int) bandstyrka();
            const char* t[] { "AV", "LITE", "MYCKET" };
            const float x[] { 0.0f, 32.0f, 72.0f };
            for (int i = 0; i < 3; ++i) prov::diodMedText (g, { r.getX() + 4.0f + x[i], r.getCentreY() }, t[i], i == b, farg::lysOrange);
        },
        [this] { return (juce::int64) bandstyrka(); },
        [this] (const juce::MouseEvent& e) { sattBandstyrka ((Bandstyrka) (e.position.x < 34.0f ? 0 : (e.position.x < 72.0f ? 1 : 2))); });
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
            y += (2.0f - juce::jmin (2.0f, p)) * 3.5f;
            const int max = (int) ((v.getWidth() - 12.0f) / (6.0f * p));
            const auto text = t.substring (0, max);
            g.setColour (farg::vfd.withAlpha (0.25f));
            osd::ritaRaw (g, text, v.getX() + 5.5f, y - 0.5f, p, 1.0f);
            g.setColour (diagnostikVarning && ! param ? farg::lysOrange : farg::vfd);
            osd::ritaRaw (g, text, v.getX() + 6.0f, y, p, 0.75f);
        };
        skriv (rad1, v.getY() + 4.0f, 1.3f);
        skriv (rad2, v.getY() + 15.0f, 2.0f);
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
            else if (r == 2) { visaPrestanda = ! visaPrestanda; cpuProcent = -1.0; cpuStart = 0.0; }
            else if (r >= 10 && r < 13) sattBandstyrka ((Bandstyrka) (r - 10));
            else if (r >= 160 && r <= 260) { proc.panelLage.setProperty ("skala", (float) (r - 100) / 100.0f, nullptr); if (skalaAndrad) skalaAndrad ((float) (r - 100) / 100.0f); }
            else if (r >= 1000) proc.presets.load (r - 1000);
        });
    }

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
            yta ({ 24 + i * 92 - 2, 472, 90, 24 }, [this, i] (juce::Graphics& g, Rectangle<float> r)
            {
                const bool a = (int) lage.vy == i;
                auto y = ritaKnapp (g, r.reduced (2.0f), a);
                ritaLysdiod (g, { y.getX() + 9.0f, y.getCentreY() }, 2.0f, farg::lysRod, a ? 1.0f : 0.0f);
                tryck (g, sv (vyer[i]), y.withTrimmedLeft (14.0f), 5.6f, juce::Justification::centred, a ? farg::vitt : farg::silver);
            },
            [this, i] { return (juce::int64) ((int) lage.vy == i); },
            [this, i] (const juce::MouseEvent&) { valjVy ((Vy) i, lage.kanal); });
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
        pil ({ 616, 474, 28, 20 }, -1);
        auto& namn = yta ({ 646, 473, 200, 22 }, [this] (juce::Graphics& g, Rectangle<float> r)
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
        pil ({ 848, 474, 28, 20 }, 1);
        auto nere = std::make_shared<bool> (false);
        auto& spara = yta ({ 880, 474, 56, 20 }, [nere] (juce::Graphics& g, Rectangle<float> r)
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

    struct Grupp
    {
        juce::String titel;
        std::vector<Reglage*> k;
        std::function<bool()> synlig;     // tom = alltid
        bool sammaX = false;              // börjar där förra gruppen började (alternativ till den)
        int mellanrum = 22;
        float x0 = 0, x1 = 0;
        bool syns() const { return ! synlig || synlig(); }

        Grupp (juce::String t, std::vector<Reglage*> kk, std::function<bool()> s = {}, bool samma = false, int mell = 22)
            : titel (std::move (t)), k (std::move (kk)), synlig (std::move (s)), sammaX (samma), mellanrum (mell) {}
    };
    using Rad = std::vector<Grupp>;
    std::array<std::array<Rad, 2>, antalVyer> vyInnehall;

    int vyIndex() const
    {
        if (lage.vy == Vy::Kanal) return lage.kanal;
        return 3 + (int) lage.vy;
    }

    void byggVyerna()
    {
        using namespace km::params;
        auto& v = vyInnehall;

        // --- Kolvbasen och Kolvbas 2
        for (int k = 0; k < 2; ++k)
        {
            auto kid = [k] (const char* p) { return id::kolv (k, p); };
            v[(size_t) k][0] = {
                { "OSCILLATOR", { &ratt (kid ("vag"), "Vågform"), &ratt (kid ("pulsbredd"), "Pulsbredd"), &ratt (kid ("sub"), "Sub"),
                                  &ratt (kid ("brus"), "Brus"), &knapp (kid ("synk"), "Synk") } },
                { "OSC 2", { &ratt (kid ("osc2Niva"), "Nivå"), &ratt (kid ("osc2Halvtoner"), "Stämning"), &ratt (kid ("osc2Fin"), "Fin") } },
                { "FILTER", { &valj (kid ("filterTyp"), "Typ"), &ratt (kid ("cutoff"), "Brytfrekvens"), &ratt (kid ("resonans"), "Resonans"),
                              &ratt (kid ("envMangd"), "Env-djup"), &ratt (kid ("filterFall"), "Env-fall"), &ratt (kid ("drive"), "Drive"),
                              &ratt (kid ("accent"), "Accent") } } };
            v[(size_t) k][1] = {
                { "ENVELOPE", { &ratt (kid ("fall"), "Fall"), &ratt (kid ("hall"), "Håll"), &ratt (kid ("slapp"), "Släpp"), &ratt (kid ("glid"), "Glid") } } };
            if (k == 1)
                v[1][1].push_back ({ stil::sv ("FÖLJ KOLVBASEN"), { &knapp (id::folj, "Följ"), &ratt (id::forskjutning, "Förskjutning"),
                                                                    &ratt (id::transponering, "Transponering") } });
            laggTillKanal (v[(size_t) k][1], k);
        }

        // --- Stampverket: kit och trumväljaren överst, vald trumma, Drivet och kanalen under
        {
            Grupp trummor { "TRUMMOR", {} };
            for (int t = 0; t < antalSpar; ++t) trummor.k.push_back (&trumKnapp (t));
            v[2][0] = { { "KIT", { &valj (id::kit, "Kit"), &knapp (id::kantslag, "Kantslag") } }, trummor };
            for (int t = 0; t < antalSpar; ++t)
                v[2][1].push_back ({ "TRUMMA " + trumKort (t) + " " + stil::sv ("·") + " " + versaler (stil::sv (trumNamn ((Trumma) t))),
                                     { &ratt (id::trumma (t, "stamning"), "Stämning"), &ratt (id::trumma (t, "klang"), "Klang"),
                                       &ratt (id::trumma (t, "niva"), "Nivå"), &knapp (id::trumma (t, "tyst"), "Tyst") },
                                     [this, t] { return lage.valdTrumma == t; }, t > 0 });
            laggTillKanal (v[2][1], 2);
        }

        // --- Hugget
        v[3][0] = {
            { "HUGGET", { &valj (id::hu ("lage"), "Läge", 76), &valj (id::hu ("ackord"), "Ackord", 76), &ratt (id::hu ("oktav"), "Oktav"),
                          &ratt (id::hu ("klang"), "Klang"), &ratt (id::hu ("sankning"), "Sänkning"), &ratt (id::hu ("bredd"), "Bredd") } },
            { "ENVELOPE", { &ratt (id::hu ("attack"), "Attack"), &ratt (id::hu ("fall"), "Fall"), &ratt (id::hu ("hall"), "Håll"), &ratt (id::hu ("slapp"), "Släpp") } } };
        laggTillKanal (v[3][1], 3);

        // --- Sekvens: mönstret per kanal till vänster, rutnätet (Sekvens) till höger
        for (int k = 0; k < antalKanaler; ++k)
        {
            auto vis = [this, k] { return lage.kanal == k; };
            v[4][0].push_back ({ stil::sv ("MÖNSTER"), { &valj (id::kanal (k, "monster"), "Mönster", 58), &valj (id::kanal (k, "variant"), "Variant", 58),
                                                         &ratt (id::kanal (k, "langd"), "Längd") }, vis, k > 0 });
        }
        v[4][1] = { { "TAKT", { &ratt (id::gung, "Gung"),
                                handling ("Kopiera A→B", [this] { kopieraAB(); }), handling ("Rensa", [this] { rensa(); }) } } };

        // --- Mixern
        for (int k = 0; k < antalKanaler; ++k)
            v[5][(size_t) (k / 2)].push_back ({ kanalNamnet (k), { &ratt (id::kanal (k, "niva"), "Nivå"), &ratt (id::kanal (k, "pan"), "Pan"),
                                                                   &ratt (id::kanal (k, "sandA"), "→ Grindrum"), &ratt (id::kanal (k, "sandB"), "→ Ekot"),
                                                                   &knapp (id::kanal (k, "tyst"), "Tyst"), &knapp (id::kanal (k, "solo"), "Solo") } });
        v[5][0].push_back ({ "TAKTVERKET", { &ratt (id::grundton, "Grundton"), &ratt (id::gung, "Gung") } });

        // --- Effekterna
        v[6][0] = {
            { "GRINDRUMMET", { &ratt (id::fx ("gr", "storlek"), "Storlek"), &ratt (id::fx ("gr", "hallTid"), "Hålltid"),
                               &ratt (id::fx ("gr", "ton"), "Ton"), &ratt (id::fx ("gr", "niva"), "Nivå") } },
            { "EKOT", { &valj (id::fx ("eko", "tid"), "Tid"), &ratt (id::fx ("eko", "aterkoppling"), "Återkoppling"), &ratt (id::fx ("eko", "ton"), "Ton"),
                        &valj (id::fx ("eko", "lage"), "Läge"), &knapp (id::fx ("eko", "pingpong"), "Ping-pong"), &ratt (id::fx ("eko", "niva"), "Nivå") } },
            { "SVAJET", { &valj (id::fx ("svaj", "lage"), "Läge"), &ratt (id::fx ("svaj", "mangd"), "Mängd") } } };
        v[6][1] = {
            { stil::sv ("KLÄMMAN"), { &ratt (id::fx ("klam", "troskel"), "Tröskel"), &ratt (id::fx ("klam", "kvot"), "Kvot"),
                                      &ratt (id::fx ("klam", "attack"), "Attack"), &ratt (id::fx ("klam", "slapp"), "Släpp"),
                                      &ratt (id::fx ("klam", "pump"), "Pump"), &ratt (id::fx ("klam", "efter"), "Efter"), &reduktionsMatare() } },
            { "KASSETTEN", { &valj (id::fx ("kass", "kvalitet"), "Band", 50), &ratt (id::fx ("kass", "slitage"), "Slitage") } } };

        // --- Modulationen
        for (int n = 0; n < 2; ++n)
            v[7][0].push_back ({ "LFO " + juce::String (n + 1), { &valj (id::lfo (n, "form"), "Form"), &ratt (id::lfo (n, "hz"), "Hastighet"),
                                                                 &valj (id::lfo (n, "synk"), "Synk") } });
        for (int n = 0; n < 2; ++n)
            v[7][0].push_back ({ "ENV " + juce::String (n + 1), { &ratt (id::menv (n, "attack"), "Attack"), &ratt (id::menv (n, "fall"), "Fall"),
                                                                 &ratt (id::menv (n, "hall"), "Håll"), &ratt (id::menv (n, "slapp"), "Släpp"),
                                                                 &valj (id::menv (n, "utlosare"), "Utlöses av", 70) } });
        for (int n = 0; n < antalKopplingar; ++n)
        {
            auto& kalla = valj (id::mod (n, "kalla"), "Källa", 58);
            auto& mal = valj (id::mod (n, "mal"), "Mål", 72);
            auto& mangd = ratt (id::mod (n, "mangd"), "Mängd");
            mangd.bredd = 46;
            v[7][1].push_back ({ "KOPPLING " + juce::String (n + 1), { &kalla, &mal, &mangd }, {}, false, 10 });
        }

        // --- Låten: låtläget och raderna (LatVy till höger)
        v[8][0] = { { stil::sv ("LÅTLÄGET"), { &knapp (id::latLage, "Låtläget"), &valj (id::latSlut, "Vid slutet", 62) } } };
        v[8][1] = { { "VALD RAD", { handling ("Kopiera", [this] { latKopiera(); }), handling ("Ta bort", [this] { latTaBort(); }),
                                    handling ("Hämta", [this] { latHamta(); }) } } };
    }

    void laggTillKanal (Rad& rad, int k)
    {
        using namespace km::params;
        rad.push_back ({ "DRIVET", { &valj (id::kanal (k, "drivetLage"), "Läge"), &ratt (id::kanal (k, "drivetMangd"), "Mängd") } });
        rad.push_back ({ "KANAL", { &ratt (id::kanal (k, "niva"), "Nivå"), &ratt (id::kanal (k, "pan"), "Pan"),
                                    &ratt (id::kanal (k, "sandA"), "→ Grindrum"), &ratt (id::kanal (k, "sandB"), "→ Ekot") } });
    }

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

    Tryckyta& reduktionsMatare()
    {
        auto u = std::make_unique<Tryckyta>();
        u->bredd = 30;
        u->etikettText = "RED.";
        auto* p = u.get();
        u->rita = [this, p] (juce::Graphics& g, Rectangle<float> r)
        {
            stil::ritaLedStapel (g, { r.getCentreX() - 2.5f, 4.0f, 5.0f, 32.0f }, juce::jlimit (0.0f, 1.0f, reduktion / 18.0f), 8, true);
            etikett (g, p->etikettText, { 0.0f, 40.0f, r.getWidth(), 8.0f });
        };
        u->lage = [this] { return (juce::int64) (reduktion * 2.0f); };
        addChildComponent (*u);
        ytor.push_back (std::move (u));
        return *p;
    }

    void layoutVyerna()
    {
        for (auto& vy : vyInnehall)
            for (int rad = 0; rad < 2; ++rad)
            {
                const int cy = falt.getY() + (rad == 0 ? 30 : 89);
                float x = (float) falt.getX() + (vy.data() == vyInnehall[4].data() ? 10.0f : 16.0f);
                float forraX0 = x;
                for (auto& gr : vy[(size_t) rad])
                {
                    if (gr.sammaX) x = forraX0;
                    gr.x0 = x;
                    forraX0 = x;
                    for (auto* k : gr.k)
                    {
                        k->setBounds ((int) x, cy - 19, k->cellBredd(), Reglage::hojd);
                        x += (float) k->cellBredd();
                    }
                    gr.x1 = x;
                    x += (float) gr.mellanrum;
                }
            }
    }

    void visaVyn()
    {
        const int aktuell = vyIndex();
        for (int i = 0; i < antalVyer; ++i)
            for (auto& rad : vyInnehall[(size_t) i])
                for (auto& gr : rad)
                {
                    const bool syns = i == aktuell && gr.syns();
                    for (auto* k : gr.k) k->setVisible (syns);
                }
        // En komponent kan finnas i flera grupper (Gung): visa den om någon synlig grupp har den.
        for (auto& rad : vyInnehall[(size_t) aktuell])
            for (auto& gr : rad)
                if (gr.syns()) for (auto* k : gr.k) k->setVisible (true);
        sekvens.setVisible (lage.vy == Vy::Sekvens);
        for (auto* y : ljudYtor) y->setVisible (lage.vy == Vy::Kanal);
        latVy.setVisible (lage.vy == Vy::Laten);
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
        const Rectangle<int> r (14, 724, 1172, 116);
        const Rectangle<int> v (r.getX(), r.getY() + 10, 104, r.getHeight() - 16);
        yta ({ v.getX() + 22, v.getY() + 22, 60, 28 }, [this] (juce::Graphics& g, Rectangle<float> rr)
        {
            ritaSjusegment (g, rr, "C" + juce::String (klaviatur.oktav));
        }, [this] { return (juce::int64) klaviatur.oktav; });
        auto pil = [&] (Rectangle<int> k, bool upp)
        {
            auto& t = yta (k, [upp] (juce::Graphics& g, Rectangle<float> rr)
            {
                auto a = ritaKnapp (g, rr.reduced (1.5f));
                g.setColour (farg::silver);
                g.fillPath (ikon (upp ? Ikon::spela : Ikon::bakat, a.reduced (10.0f, 3.5f)));
            });
            t.tryck = [this, upp] (const juce::MouseEvent&)
            {
                klaviatur.oktav = juce::jlimit (0, 6, klaviatur.oktav + (upp ? 1 : -1));
                oktavPerKanal[(size_t) lage.kanal] = klaviatur.oktav;
                klaviatur.repaint();
            };
        };
        pil ({ v.getX() + 10, v.getY() + 58, 40, 22 }, false);
        pil ({ v.getRight() - 50, v.getY() + 58, 40, 22 }, true);

        addAndMakeVisible (klaviatur);
        klaviatur.setBounds (r.getX() + 116, r.getY() + 12, r.getWidth() - 116, r.getHeight() - 18);
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

        // Kanalväljaren.
        {
            const Rectangle<float> k (14.0f, 96.0f, 178.0f, 352.0f);
            ritaInfalltFalt (g, k, ytorna);
            tryck (g, sv ("KANALVÄLJARE"), { k.getX(), k.getY() + 8.0f, k.getWidth(), 8.0f }, 5.6f);
            const float ky = k.getY() + 26.0f + (float) lage.kanal * 60.0f + 25.0f;
            ritaPil (g, { k.getRight() - 7.0f, ky }, { k.getRight() + 16.0f, ky }, 5.0f, farg::tryckRott);
            const float y = k.getY() + 280.0f;
            tryck (g, sv ("MÖNSTER"), { k.getX() + 12.0f, y, 80.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
            tryck (g, "VARIANT", { k.getX() + 100.0f, y, 70.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
            tryck (g, sv ("DRA · RULLA · KLICKA"), { k.getX() + 12.0f, y + 50.0f, 150.0f, 8.0f }, 4.4f, juce::Justification::centredLeft, farg::silverMatt);
        }

        // Bildskärmens infattning.
        {
            const Rectangle<float> yttre (208.0f, 92.0f, 684.0f, 360.0f);
            skarm::ritaInfattning (g, yttre, glas, ytorna);
            const float y = yttre.getBottom() - 20.0f;
            tryck (g, sv ("BILDSKÄRM"), { yttre.getX() + 22.0f, y - 4.0f, 100.0f, 8.0f }, 5.6f, juce::Justification::centredLeft);
            g.setColour (farg::silverMatt);
            g.fillRect (yttre.getX() + 96.0f, y - 0.5f, 120.0f, 1.0f);
            tryck (g, "BANDSTYRKA", { yttre.getRight() - 300.0f, y - 4.0f, 80.0f, 8.0f }, 5.0f, juce::Justification::centredLeft, farg::silverMatt);
            ritaLysdiod (g, { yttre.getRight() - 30.0f, y }, 2.4f, farg::lysRod, 1.0f);
            tryck (g, "TV", { yttre.getRight() - 58.0f, y - 4.0f, 20.0f, 8.0f }, 5.0f, juce::Justification::centredRight, farg::silverMatt);
        }

        // Presets och master.
        {
            const Rectangle<float> p (906.0f, 96.0f, 280.0f, 352.0f);
            ritaInfalltFalt (g, p, ytorna);
            tryck (g, "PRESETS", { p.getX(), p.getY() + 8.0f, p.getWidth(), 8.0f }, 5.6f);
            prov::skara (g, p.getY() + 212.0f, p.getX() + 10.0f, p.getRight() - 10.0f);
            prov::gruppRubrik (g, "MASTER", p.getX() + 14.0f, p.getRight() - 14.0f, p.getY() + 228.0f);
        }

        prov::skara (g, 462.0f, 0.0f, r.getWidth());

        // Rattpanelen: kanalens (eller vyns) namn och fältet med grupprubrikerna.
        {
            const Rectangle<float> rp (14.0f, 470.0f, 1172.0f, 152.0f);
            const bool kanalVy = lage.vy == Vy::Kanal || lage.vy == Vy::Sekvens;
            const Rectangle<float> blk (rp.getRight() - 236.0f, rp.getY() + 4.0f, 30.0f, 20.0f);
            g.setColour (farg::tryckRott);
            g.fillPath (parallellogram (blk, 6.0f));
            static const char* vyNamn[] { "", "", "MIXER", "EFFEKTER", "MODULATION", "LÅTEN" };
            rubrik (g, kanalVy ? juce::String (lage.kanal + 1) : juce::String (lage.vy == Vy::Laten ? "L" : "M"), blk, 12.0f, farg::vitt, juce::Justification::centred, 1.8f);
            rubrik (g, kanalVy ? kanalNamnet (lage.kanal) : sv (vyNamn[(int) lage.vy]), { blk.getRight() + 8.0f, blk.getY() + 2.0f, 200.0f, 16.0f },
                    14.0f, farg::vitt, juce::Justification::centredLeft, 1.9f, 2.2f);
            if (lage.vy == Vy::Kanal) tryck (g, "LJUD", { 580.0f, 480.0f, 32.0f, 8.0f }, 5.4f, juce::Justification::centredLeft);
            else ritaDiagonaler (g, { rp.getX() + 572.0f, rp.getY() + 9.0f, rp.getRight() - 246.0f - (rp.getX() + 572.0f), 10.0f },
                                 juce::Colours::white.withAlpha (0.06f), 2.0f, 6.0f);
            ritaInfalltFalt (g, falt.toFloat(), ytorna);
            for (int rad = 0; rad < 2; ++rad)
            {
                const float cy = (float) falt.getY() + (rad == 0 ? 30.0f : 89.0f);
                for (auto& gr : vyInnehall[(size_t) vyIndex()][(size_t) rad])
                    if (gr.syns() && gr.titel.isNotEmpty())
                        prov::gruppRubrik (g, gr.titel, gr.x0 + 6.0f, gr.x1 - 6.0f, cy - 24.0f);
            }
        }

        // Stegraden.
        stegrad.ritaTryck (g, stegrad.getPosition().toFloat());

        // Klaviaturen.
        {
            const Rectangle<float> kr (14.0f, 724.0f, 1172.0f, 116.0f);
            prov::skara (g, kr.getY(), 0.0f, r.getWidth());
            const auto v = Rectangle<float> (kr.getX(), kr.getY() + 10.0f, 104.0f, kr.getHeight() - 16.0f);
            ritaInfalltFalt (g, v, ytorna);
            tryck (g, "OKTAV", { v.getX(), v.getY() + 8.0f, v.getWidth(), 8.0f }, 5.6f);
            static const char* spelar[] { "SPELAR KOLVBASEN", "SPELAR KOLVBAS 2", "SPELAR STAMPVERKET", "SPELAR HUGGET" };
            tryck (g, spelar[lage.kanal], { v.getX(), v.getBottom() - 12.0f, v.getWidth(), 8.0f }, 4.0f, juce::Justification::centred, farg::silverMatt, 0.7f, 1.2f);
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
        bild.uppdatera (skarmData (utMax));
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
    const Rectangle<int> falt { 14, 502, 1172, 120 };
    const Rectangle<float> glas { 230.0f, 108.0f, 640.0f, 304.0f };

    std::vector<std::unique_ptr<Reglage>> reglage;
    std::vector<std::unique_ptr<Tryckyta>> ytor;
    Stegrad stegrad;
    Sekvens sekvens;
    LatVy latVy;
    Klaviatur klaviatur;
    Bildskarm bild;
    Tryckyta* vfd = nullptr;
    std::vector<Tryckyta*> ljudYtor;
    static inline float bildUpplosning = 1.0f;   // skärmens interna upplösning (andel av den fysiska)
    Tryckyta* hjulMonster = nullptr;

    stil::Ytor ytorna;
    juce::Image bakgrund;
    float bgSkala = 0.0f;
    juce::int64 bgNyckel = -1;

    juce::String vfdNamn, vfdVarde;
    double vfdTid = -1e9;
    bool spolaNed = false;
    double spolaTid = 0.0;
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
