// SkarmProv: provbilder och tidsmätning av bildskärmen, anropas från KMSnapshot.
//
//   KMSnapshot --skarm katalog [skala]          en PNG per scen/vy (med infattningen runt)
//   KMSnapshot --skarm-tid [skala] [andel]      ms per bildruta för varje scen (andel = intern upplösning
//                                               av den fysiska, standard 1)
//
// Maskinen simuleras: 125 BPM, ett mönster med accenter, trumslag, nivåer, en filtrerad sågtand i
// oscilloskopet och två LFO:er. Uppdateringarna kommer i 30 Hz, som från panelens timer.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <iostream>
#include <iomanip>
#include "ui/vhs/Bildskarm.h"
#include "ui/stil/Hardvara.h"

namespace skarmprov
{
using km::vhs::SkarmData;
using km::vhs::Vy;
using km::vhs::Bandstyrka;

struct Fall
{
    const char* namn;
    Vy vy;
    int kanal;
    Bandstyrka band = Bandstyrka::Mycket;
    float slitage = 0.0f;
    int uppdateringar = 78;     // provbilden tas efter så många uppdateringar (78 → steg 5, andra varvet)
    bool iTid = true;           // med i tidsmätningen
};

inline const std::vector<Fall>& fallen()
{
    static const std::vector<Fall> f {
        { "kolvbasen",          Vy::Kanal, 0 },
        { "kolvbas2",           Vy::Kanal, 1 },
        { "stampverket",        Vy::Kanal, 2 },
        { "hugget",             Vy::Kanal, 3 },
        { "mixer",              Vy::Mixer, 0 },
        { "effekter",           Vy::Effekter, 0 },
        { "modulation",         Vy::Modulation, 0 },
        { "kolvbasen-band-av",  Vy::Kanal, 0, Bandstyrka::Av },
        { "kolvbasen-lite",     Vy::Kanal, 0, Bandstyrka::Lite },
        { "kolvbasen-slitage1", Vy::Kanal, 0, Bandstyrka::Mycket, 1.0f },
        { "stampverket-slag",   Vy::Kanal, 2, Bandstyrka::Mycket, 0.0f, 73, false },   // just efter ett BD-slag
        { "hugget-slag",        Vy::Kanal, 3, Bandstyrka::Mycket, 0.0f, 87, false },   // just efter ett slag
    };
    return f;
}

inline const char* kanalText (const Fall& f)
{
    switch (f.vy)
    {
        case Vy::Mixer:      return "MIXER";
        case Vy::Effekter:   return "EFFEKTER";
        case Vy::Modulation: return "MODULATION";
        case Vy::Laten:      return "LATEN";
        case Vy::Kanal:
        case Vy::Sekvens:    break;
    }
    static const char* k[4] { "KOLVBASEN", "KOLVBAS 2", "STAMPVERKET", "HUGGET" };
    return k[juce::jlimit (0, 3, f.kanal)];
}

// En simulerad maskin som spelar.
struct Maskin
{
    SkarmData d;
    float env[4] {}, menv[2] {};
    int forraSteg = -1;
    float fas = 0.0f, filter = 0.0f, lp = 0.0f, lp2 = 0.0f;

    explicit Maskin (const Fall& f)
    {
        d.vy = f.vy; d.kanal = f.kanal; d.bandstyrka = f.band; d.slitage = f.slitage;
        d.spelar = true; d.langd = 16; d.valdTrumma = 0;
        const float niva[16] { 1.0f, 0, 0.6f, 0.6f, 0, 0, 0.6f, 0, 1.0f, 0, 0.6f, 0.6f, 0, 0.6f, 1.0f, 0 };
        for (int i = 0; i < 16; ++i) d.stegNiva[(size_t) i] = niva[i];
        d.lfoForm = { 0, 3 };
        d.osdLage = km::stil::sv ("▶ SPELA");
        d.osdKvalitet = "SP";
        d.osdKanal = kanalText (f);
    }

    // Tid t i sekunder; stegar fram till t och fyller i SkarmData.
    void till (double t)
    {
        const double stegTid = 60.0 / 125.0 / 4.0;
        const double pos = t / stegTid;
        const int steg = (int) std::floor (pos);
        d.steg = steg % d.langd;
        d.stegFas = (float) (pos - std::floor (pos));
        const int sek = 12 * 60 + 4 + (int) t;
        d.osdRakneverk = juce::String (sek / 3600) + ":" + juce::String ((sek / 60) % 60).paddedLeft ('0', 2) + ":" + juce::String (sek % 60).paddedLeft ('0', 2);

        while (forraSteg < steg)
        {
            ++forraSteg;
            const int s = forraSteg % 16;
            const float n = d.stegNiva[(size_t) s];
            if (n > 0.0f) { ++d.kanalSlag[0]; env[0] = n; ++d.kanalSlag[1]; env[1] = n * 0.8f; menv[0] = 1.0f; }
            // Trummor: BD på fjärdedelar, SD på 2 och 4, CH på åttondelar, OH på offbeat, CP ibland.
            if (s % 4 == 0)            { ++d.trumSlag[0]; env[2] = 1.0f; }
            if (s == 4 || s == 12)     { ++d.trumSlag[1]; env[2] = 0.9f; }
            if (s % 2 == 0)            ++d.trumSlag[4];
            if (s % 4 == 2)            ++d.trumSlag[5];
            if (s == 7 || s == 15)     ++d.trumSlag[3];
            if (s == 10)               ++d.trumSlag[9];
            if (s % 4 == 0 || s == 6 || s == 14) ++d.trumSlag[(size_t) (11 - s % 3)];
            if (s % 2 == 0)            ++d.kanalSlag[2];
            if (s % 8 == 0 || s == 3 || s == 11) { ++d.kanalSlag[3]; env[3] = 1.0f; menv[1] = 1.0f; }
        }
        for (auto& e : env) e *= 0.8f;
        for (auto& e : menv) e *= 0.86f;
        for (int k = 0; k < 4; ++k) d.kanalNiva[(size_t) k] = juce::jlimit (0.0f, 1.0f, env[k] * (k == 2 ? 0.9f : 0.7f));
        d.niva = juce::jmin (1.0f, 0.5f * (env[0] + env[1] + env[2] + env[3]));
        d.menvVarde = { menv[0], menv[1] };
        d.lfoVarde = { (float) std::sin (2.0 * juce::MathConstants<double>::pi * t * 0.7),
                       (float) (1.0 - 2.0 * std::fmod (t * 0.45, 1.0)) };

        // Oscilloskopet: en filtrerad sågtand som följer envelopet.
        const float e = env[0] + 0.25f;
        for (size_t i = 0; i < d.scope.size(); ++i)
        {
            fas += 0.045f; if (fas >= 1.0f) fas -= 1.0f;
            const float saw = 2.0f * fas - 1.0f;
            const float br = 0.08f + 0.45f * e * e;
            lp += (saw - lp) * br; lp2 += (lp - lp2) * br;
            d.scope[i] = juce::jlimit (-1.0f, 1.0f, lp2 * (0.35f + env[0]) * 5.0f);
        }
    }
};

inline juce::Image ritaMedInfattning (km::vhs::Bildskarm& b, float s)
{
    const float W = 684.0f, H = 360.0f;
    juce::Image bild (juce::Image::RGB, (int) std::ceil (W * s), (int) std::ceil (H * s), true, juce::SoftwareImageType());
    juce::Graphics g (bild);
    g.addTransform (juce::AffineTransform::scale (s));
    km::stil::Ytor ytor;
    ytor.forbered ((int) W, (int) H, s);
    g.setFillType (ytor.metallFyll());
    g.fillRect (0.0f, 0.0f, W, H);
    const juce::Rectangle<float> glas (22.0f, 16.0f, 640.0f, 304.0f);
    km::stil::skarm::ritaInfattning (g, { 0.0f, 0.0f, W, H }, glas, ytor);
    juce::Graphics::ScopedSaveState spara (g);
    g.addTransform (juce::AffineTransform::translation (glas.getX(), glas.getY()));
    b.paint (g);
    return bild;
}

inline int katalog (const juce::File& dir, float s)
{
    dir.createDirectory();
    for (const auto& f : fallen())
    {
        km::vhs::Bildskarm b;
        b.setBounds (0, 0, 640, 304);
        Maskin m (f);
        const int n = f.uppdateringar;
        for (int i = 0; i <= n; ++i)
        {
            m.till (i / 30.0);
            b.uppdateraVid (m.d, i * 1000.0 / 30.0);
        }
        const auto bild = ritaMedInfattning (b, s);
        const auto fil = dir.getChildFile (juce::String ("skarm-") + f.namn + ".png");
        fil.deleteFile();
        juce::FileOutputStream ut (fil);
        juce::PNGImageFormat png;
        if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
        {
            std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
            return 1;
        }
        std::cout << fil.getFullPathName() << "  (intern " << b.internBredd() << " x " << b.internHojd() << ")\n";
    }
    return 0;
}

inline int tid (float s, float upplosning)
{
    std::cout << "skala " << s << ", intern upplösning " << upplosning << " av fysisk. 10 s i 30 Hz per scen; ms per ritad bildruta.\n"
              << "  första = första bildrutan (bygger scenen), snitt/max = alla övriga, varm = de sista 5 s (alla lager byggda),\n"
              << "  CPU = all tid (uppdatera + paint) de sista 5 s / 5 s, rutor = ritade bildrutor av 300 anrop.\n";
    std::cout << std::left << std::setw (20) << "scen" << std::right
              << std::setw (8) << "första" << std::setw (8) << "snitt" << std::setw (8) << "max"
              << std::setw (11) << "varm snitt" << std::setw (9) << "varm max" << std::setw (7) << "CPU %"
              << std::setw (7) << "rutor" << "   [varm: scen + band + paint]\n";
    const int pw = juce::roundToInt (640.0f * s), ph = juce::roundToInt (304.0f * s);
    for (const auto& f : fallen())
    {
        if (! f.iTid) continue;
        km::vhs::Bildskarm b;
        b.setUpplosning (upplosning);
        b.setBounds (0, 0, 640, 304);
        Maskin m (f);
        juce::Image mal (juce::Image::RGB, pw, ph, true, juce::SoftwareImageType());
        const int anrop = 300, varmFran = 150;
        double forsta = -1.0, sum = 0.0, max = 0.0, varmSum = 0.0, varmMax = 0.0, varmTotal = 0.0;
        int rutor = 0, ovriga = 0, varmRutor = 0;
        km::vhs::Bildskarm::Tider fore;
        for (int i = 0; i < anrop; ++i)
        {
            if (i == varmFran) fore = b.tider;
            m.till (i / 30.0);
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            const bool rita = b.uppdateraVid (m.d, i * 1000.0 / 30.0);
            if (rita)
            {
                juce::Graphics g (mal);
                g.addTransform (juce::AffineTransform::scale (s));
                b.paint (g);
            }
            const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
            if (i >= varmFran) varmTotal += ms;
            if (! rita) continue;
            ++rutor;
            if (forsta < 0.0) { forsta = ms; continue; }
            sum += ms; max = juce::jmax (max, ms); ++ovriga;
            if (i >= varmFran) { varmSum += ms; varmMax = juce::jmax (varmMax, ms); ++varmRutor; }
        }
        const auto& T = b.tider;
        const double nr = juce::jmax (1, T.rutor - fore.rutor);
        std::cout << std::left << std::setw (20) << f.namn << std::right << std::fixed << std::setprecision (2)
                  << std::setw (8) << forsta << std::setw (8) << (ovriga > 0 ? sum / ovriga : 0.0) << std::setw (8) << max
                  << std::setw (11) << (varmRutor > 0 ? varmSum / varmRutor : 0.0) << std::setw (9) << varmMax
                  << std::setw (7) << varmTotal / ((anrop - varmFran) * 1000.0 / 30.0) * 100.0
                  << std::setw (7) << rutor
                  << "   [" << (T.scen - fore.scen) / nr << " + " << (T.band - fore.band) / nr << " + " << (T.paint - fore.paint) / nr << "]\n";
    }
    return 0;
}
} // namespace skarmprov

inline int skarmProv (int argc, char* argv[])
{
    const juce::String lage (argv[1]);
    if (lage == "--skarm")
    {
        if (argc < 3) { std::cout << "Användning: KMSnapshot --skarm katalog [skala]\n"; return 1; }
        const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[2]));
        const float s = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;
        return skarmprov::katalog (dir, s);
    }
    const float s = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
    const float u = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;
    return skarmprov::tid (s, u);
}
