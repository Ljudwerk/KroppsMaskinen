// KMSnapshot – ritar panelen till PNG, så att gränssnittet kan granskas utan DAW.
//
//   KMSnapshot fil.png [skala] [vy 1–6] [kanal 1–4] [preset]
//        VHS-panelen; vy 1 KANAL, 2 SEKVENS, 3 MIXER, 4 EFFEKTER, 5 MODULATION, 6 LÅTEN. Skalan är fysisk (2 = Retina).
//   KMSnapshot --panel-tid [skala] [sekunder]       panelens kostnad per sekund när sekvensern går
//   KMSnapshot --panel-parametrar                    listar parametrar som inte nås från panelen
//   KMSnapshot --panel-klick                         klickar på allt i alla vyer (letar krascher och jassert)
//   KMSnapshot --stilprov fil.png [skala]            (VHS-panelens stilprov, fasta exempelvärden)
//   KMSnapshot --stilprov-detaljer fil.png [skala]   (närbilder: ratt, stegknapp, display, typsnitt, skärm)
//
// Laddar "Kolv 125", låter sekvensern gå en stund så att steglampan syns, och ritar hela panelen.
// Stilproven använder varken processorn eller parametrarna.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/stil/Stilprov.h"
#include "SkarmProv.h"
#include <map>

// Skriver en komponent till PNG i given skala.
static int sparaPNG (juce::Component& c, const juce::File& fil, float skala, const char* vad)
{
    const auto bild = c.createComponentSnapshot (c.getLocalBounds(), true, skala);
    fil.deleteFile();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    std::cout << vad << " ritat till " << fil.getFullPathName() << "\n";
    return 0;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: KMSnapshot [--stilprov | --stilprov-detaljer] fil.png [skala]\n";
        return 1;
    }
    const juce::String lage (argv[1]);
    if (lage == "--skarm" || lage == "--skarm-tid") return skarmProv (argc, argv);
    if (lage == "--tid-stilprov")
    {
        using namespace km::stil;
        band::Installning in; in.lage = sv ("▶ SPELA"); in.kvalitet = "SP"; in.kanal = "KOLVBASEN"; in.rakneverk = "0:12:04";
        for (float sk : { 0.75f, 1.0f, 2.0f })
        {
            const int n = 5;
            auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < n; ++i)
            {
                in.fro = (uint32_t) i + 1;
                auto b = band::skarmbild (640.0f, 304.0f, sk, in, [] (juce::Graphics& gs, juce::Rectangle<float> r) { skarm::ritaKolvbasScen (gs, r); });
            }
            auto t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < n; ++i)
            {
                juce::Image b (juce::Image::RGB, (int) (640 * sk), (int) (304 * sk), true);
                juce::Graphics gs (b);
                gs.addTransform (juce::AffineTransform::scale (sk));
                skarm::ritaKolvbasScen (gs, { 0, 0, 640, 304 });
            }
            auto t2 = juce::Time::getMillisecondCounterHiRes();
            std::cout << "skala " << sk << ": hela skärmbilden " << (t1 - t0) / n << " ms, bara scenen " << (t2 - t1) / n << " ms\n";
        }
        return 0;
    }
    if (lage == "--stilprov" || lage == "--stilprov-detaljer")
    {
        if (argc < 3)
        {
            std::cout << "Användning: KMSnapshot " << lage << " fil.png [skala]\n";
            return 1;
        }
        const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        const float skala = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;
        if (lage == "--stilprov")
        {
            km::stil::prov::Panel panel;
            return sparaPNG (panel, fil, skala, "Stilprovet");
        }
        km::stil::prov::Detaljer detaljer;
        return sparaPNG (detaljer, fil, skala, "Detaljarket");
    }
    // VHS-panelen: KMSnapshot fil.png [skala] [vy 1–6] [kanal 1–4] [preset]
    const bool tid = lage == "--panel-tid" || lage == "--panel-parametrar" || lage == "--panel-klick";
    const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (tid ? "x.png" : argv[1]);
    const float skala = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
    const int vy = ! tid && argc > 3 ? juce::jlimit (1, 6, juce::String (argv[3]).getIntValue()) : 1;
    const int kanal = ! tid && argc > 4 ? juce::jlimit (1, 4, juce::String (argv[4]).getIntValue()) : 1;
    const juce::String preset = ! tid && argc > 5 ? juce::String::fromUTF8 (argv[5]) : juce::String ("Kolv 125");

    KroppsmaskinenProcessor proc;
    proc.forceraEgenKlocka = true;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    proc.presets.loadByName (preset);
    if (auto* p = proc.state.getParameter (km::params::id::spela)) p->setValueNotifyingHost (1.0f);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int i = 0; i < 60; ++i) { midi.clear(); proc.processBlock (buf, midi); }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    auto* e = dynamic_cast<KroppsmaskinenEditor*> (editor.get());
    e->sattSkala (1.0f);
    auto& panel = e->getPanel();
    panel.valjVy ((km::vhs::Vy) (vy - 1), kanal - 1);

    if (lage == "--panel-klick")
    {
        // Klickar, drar och rullar på allt i alla vyer och kanaler (utom menyer och sparadialogen),
        // med och utan Skift/Alt, och spelar in med klaviaturen. Fångar krascher och jassert i Debug.
        auto kalla = juce::Desktop::getInstance().getMainMouseSource();
        int antal = 0;
        auto skicka = [&] (juce::Component& c, juce::Point<float> p, juce::ModifierKeys m, float dy)
        {
            const auto t = juce::Time::getCurrentTime();
            juce::MouseEvent ned (kalla, p, m.withFlags (juce::ModifierKeys::leftButtonModifier), 1.0f, 0, 0, 0, 0, &c, &c, t, p, t, 1, false);
            juce::MouseEvent drag (kalla, p.translated (0.0f, dy), m.withFlags (juce::ModifierKeys::leftButtonModifier), 1.0f, 0, 0, 0, 0, &c, &c, t, p, t, 1, true);
            juce::MouseEvent upp (kalla, p.translated (0.0f, dy), m, 1.0f, 0, 0, 0, 0, &c, &c, t, p, t, 1, std::abs (dy) > 0.0f);
            c.mouseDown (ned); c.mouseDrag (drag); c.mouseUp (upp);
            juce::MouseWheelDetails w { 0.0f, 0.2f, false, false, false };
            c.mouseWheelMove (upp, w);
            ++antal;
        };
        const juce::Rectangle<int> undantag (906, 150, 280, 40);   // ◀ ▶ SPARA MENY och presetnumret
        for (int v = 0; v < 6; ++v)
            for (int k = 0; k < 4; ++k)
            {
                panel.valjVy ((km::vhs::Vy) v, k);
                for (auto* c : panel.getChildren())
                {
                    if (! c->isVisible() || c->getBounds().intersects (undantag) || c->getBounds().intersects (juce::Rectangle<int> (918, 118, 60, 40))) continue;
                    if (c->getBounds().getY() < 70 && c->getBounds().getX() >= 408 && c->getBounds().getX() < 440) continue;   // SPOLA
                    if (c->getBounds().intersects (juce::Rectangle<int> (878, 628, 60, 24))) continue;   // SPARA ljud (dialog)
                    for (float fx : { 0.2f, 0.5f, 0.8f })
                        for (auto m : { juce::ModifierKeys(), juce::ModifierKeys (juce::ModifierKeys::shiftModifier), juce::ModifierKeys (juce::ModifierKeys::altModifier) })
                            skicka (*c, { c->getWidth() * fx, c->getHeight() * 0.5f }, m, fx < 0.5f ? -20.0f : 0.0f);
                }
                panel.tickForTest();
                midi.clear(); proc.processBlock (buf, midi);
            }
        std::cout << antal << " klick utan fel\n";
        return 0;
    }
    if (lage == "--panel-parametrar")
    {
        // Grind 3: alla parametrar ska gå att nå från panelen.
        const auto nada = panel.nadaParametrar();
        int saknas = 0;
        for (auto* p : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (! nada.contains (r->getParameterID())) { std::cout << "saknas: " << r->getParameterID() << "\n"; ++saknas; }
        std::cout << proc.getParameters().size() << " parametrar, " << saknas << " saknas i panelen\n";
        return saknas == 0 ? 0 : 1;
    }
    if (tid)
    {
        // Spelar i realtid (block om 512 sampel ≈ 10,7 ms) och kör panelens timer var 33:e ms. Efter varje
        // tick ritas de ytor som timern bad om (Component::repaint fångas inte utan fönster, så panelen
        // samlar dem själv); bildskärmen räknas med när den bett om en ny bild. Allt i fysisk skala.
        const double sek = argc > 3 ? juce::String (argv[3]).getDoubleValue() : 6.0;
        juce::Image yta (juce::Image::RGB, (int) (1200 * skala), (int) (840 * skala), true);
        // Fångar alla repaint()-anrop som når panelen (de bubblar upp från reglagen).
        struct Fangare final : juce::CachedComponentImage
        {
            juce::RectangleList<int> ytor;
            juce::Rectangle<int> hela;
            void paint (juce::Graphics&) override {}
            bool invalidateAll() override { ytor.add (hela); return true; }
            bool invalidate (const juce::Rectangle<int>& r) override { ytor.add (r); return true; }
            void releaseResources() override {}
        };
        auto* fangare = new Fangare();
        fangare->hela = panel.getLocalBounds();
        panel.setCachedComponentImage (fangare);
        double ritTid = 0.0, maxTid = 0.0;
        std::map<juce::String, double> per;
        int ticks = 0;
        const double blockMs = 512.0 / 48.0;
        double klocka = 0.0, nastaTick = 0.0;
        // Förvärm: bakgrund och alla reglage en gång.
        { juce::Graphics g (yta); g.addTransform (juce::AffineTransform::scale (skala)); panel.paintEntireComponent (g, true); }
        while (klocka < sek * 1000.0)
        {
            midi.clear(); proc.processBlock (buf, midi);
            klocka += blockMs;
            if (klocka < nastaTick) continue;
            nastaTick += 1000.0 / 30.0;
            fangare->ytor.clear();
            panel.tickForTest();
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (auto r : fangare->ytor)
            {
                const auto r0 = juce::Time::getMillisecondCounterHiRes();
                {
                    juce::Graphics g (yta);
                    g.addTransform (juce::AffineTransform::scale (skala));
                    g.reduceClipRegion (r);
                    panel.paintEntireComponent (g, true);
                }
                per[r.toString()] += juce::Time::getMillisecondCounterHiRes() - r0;
            }
            const auto dt = juce::Time::getMillisecondCounterHiRes() - t0;
            ritTid += dt; maxTid = juce::jmax (maxTid, dt); ++ticks;
            juce::Thread::sleep ((int) (1000.0 / 30.0));   // väggklockan styr bildskärmens 25 bilder/s
        }
        std::vector<std::pair<double, juce::String>> lista;
        for (auto& [k, v] : per) lista.push_back ({ v, k });
        std::sort (lista.rbegin(), lista.rend());
        for (size_t i = 0; i < juce::jmin ((size_t) 8, lista.size()); ++i)
            std::cout << "  " << lista[i].second << ": " << juce::String (lista[i].first / ticks, 3) << " ms per tick\n";
        {
            const auto& t = panel.getBildskarm().tider;
            const double n = juce::jmax (1, t.rutor);
            std::cout << "  bildskärmen: " << t.rutor << " rutor, lager " << juce::String (t.lager / n, 2) << ", scen " << juce::String (t.scen / n, 2)
                      << ", band " << juce::String (t.band / n, 2) << ", paint " << juce::String (t.paint / n, 2) << " ms per ruta\n";
        }
        std::cout << "panelen, skala " << skala << ": " << juce::String (ritTid / ticks, 2) << " ms per tick (max "
                  << juce::String (maxTid, 1) << " ms), " << juce::String (ritTid / (sek * 1000.0) * 100.0, 2) << " % av en kärna\n";
        return 0;
    }

    // Låt timern gå några bildrutor så att skärmen, lysdioderna och mätarna har något att visa.
    for (int i = 0; i < 8; ++i)
    {
        midi.clear(); proc.processBlock (buf, midi);
        panel.tickForTest();
        juce::Thread::sleep (42);
    }
    const auto bild = panel.createComponentSnapshot (panel.getLocalBounds(), true, skala);
    fil.deleteFile();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    editor.reset();
    std::cout << "Panelen ritad till " << fil.getFullPathName() << "\n";
    return 0;
}
