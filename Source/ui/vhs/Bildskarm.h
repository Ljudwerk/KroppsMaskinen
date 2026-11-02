// Bildskarm: den animerade tjock-TV:n i panelens mitt (scen → band → OSD → glas), byggd för låg CPU.
//
// Panelen anropar uppdatera() ungefär 30 gånger i sekunden. Komponenten räknar fram rörelsen
// (vevvinkel, hammarslag, nivåer, historik) vid varje anrop och bestämmer själv om den ska ritas om:
// med bandet på ritas den i 25 bilder/s (bruset lever även när maskinen står), med bandet av bara när
// något synligt har ändrats (högst 25 gånger i sekunden).
//
// Varför den är snabb (stilprovets skärmbild tar ~30 ms, en bildruta här ~1–2 ms):
//   - scenen delas i lager (Scener.h): bakgrunden, det halvstatiska lagret och sprajtarna (kolven i
//     24 vevvinklar, hammaren, bilan, mixertornen) ritas en gång och läggs sedan bara på
//   - bara det levande (spelhuvud, oscilloskop, gnistor, skrivare) ritas per bildruta, med en
//     billigare glödlinje
//   - bandet är ett enda heltalspass per bildruta: radvis darr, färgförskjutning och utsmetad färg
//     (lådfilter via prefixsummor), brus ur en färdig brustabell, svepande ljus zon, spårningsbrus,
//     tappningar, sveplinjerna och glaset (ett förritat lager)
//   - OSD-texten ritas skarpt i fysisk upplösning och cachas per text
//   - bilden räknas i fysisk upplösning upp till ett tak (sträcks ut över taket); ingen
//     tunnbildsbuktning per bildruta
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>
#include "SkarmData.h"
#include "Scener.h"

namespace km::vhs
{

namespace band
{
    // Rutan med synliga pixlar (alfa > 0) i en ARGB-bild.
    inline juce::Rectangle<int> alfaRuta (const juce::Image& bild)
    {
        juce::Image::BitmapData bd (bild, juce::Image::BitmapData::readOnly);
        int x0 = bild.getWidth(), x1 = -1, y0 = bild.getHeight(), y1 = -1;
        for (int y = 0; y < bild.getHeight(); ++y)
        {
            const auto* rad = bd.getLinePointer (y);
            for (int x = 0; x < bild.getWidth(); ++x)
                if (rad[x * bd.pixelStride + juce::PixelARGB::indexA] != 0)
                {
                    x0 = juce::jmin (x0, x); x1 = juce::jmax (x1, x); y0 = juce::jmin (y0, y); y1 = juce::jmax (y1, y);
                }
        }
        return x1 < 0 ? juce::Rectangle<int>() : juce::Rectangle<int> (x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    }

    // Sveplinjernas profil (som stilprovets bildrör), medelvärdet över designraderna [yd0, yd1]:
    // ingen moaré i någon skala.
    inline float sveplinje (float yd0, float yd1, float styrka)
    {
        const float u0 = yd0 + 0.5f, u1 = yd1 + 0.5f;
        const float medelCos = std::abs (u1 - u0) < 1.0e-4f ? std::cos (km::stil::pi * u0)
                                                            : (std::sin (km::stil::pi * u1) - std::sin (km::stil::pi * u0)) / (km::stil::pi * (u1 - u0));
        return 1.0f - styrka * (0.5f - 0.5f * medelCos);
    }

    // --- bandpassets inre slingor: egna funktioner med __restrict, så att de vektoriseras ---------

    // Lägger en rad (RGB eller ARGB) till kanalplanen; forst = skriv i stället för att addera.
    inline void laggTill (int W, const uint8_t* __restrict src, uint16_t* __restrict R, uint16_t* __restrict G,
                          uint16_t* __restrict B, bool forst)
    {
        constexpr int iR = juce::PixelRGB::indexR, iG = juce::PixelRGB::indexG, iB = juce::PixelRGB::indexB;
        if (forst) for (int x = 0; x < W; ++x) { R[x] = src[x * 3 + iR]; G[x] = src[x * 3 + iG]; B[x] = src[x * 3 + iB]; }
        else       for (int x = 0; x < W; ++x) { R[x] = (uint16_t) (R[x] + src[x * 3 + iR]); G[x] = (uint16_t) (G[x] + src[x * 3 + iG]); B[x] = (uint16_t) (B[x] + src[x * 3 + iB]); }
    }

    inline void adderaPlan (int W, const uint16_t* __restrict in, uint16_t* __restrict ut)
    {
        for (int x = 0; x < W; ++x) ut[x] = (uint16_t) (ut[x] + in[x]);
    }

    // Normering, brus, ljus och glas för en linje. Resultatet 0..255 per kanal.
    inline void linjePass (int W, const uint16_t* __restrict sr, const uint16_t* __restrict sg, const uint16_t* __restrict sb,
                           const int16_t* __restrict brus, const uint16_t* __restrict ia, const uint16_t* __restrict gr,
                           const uint16_t* __restrict gg, const uint16_t* __restrict gb,
                           uint16_t* __restrict lr, uint16_t* __restrict lg, uint16_t* __restrict lb,
                           uint16_t invRB, uint16_t invG, int16_t amp, int16_t ljus)
    {
        for (int x = 0; x < W; ++x)
        {
            const auto n = (int16_t) ((int16_t) (brus[x] * amp) >> 8);
            auto r = (int16_t) ((int16_t) (((uint32_t) sr[x] * invRB) >> 16) + n + ljus);
            auto g = (int16_t) ((int16_t) (((uint32_t) sg[x] * invG) >> 16) + n + ljus);
            auto b = (int16_t) ((int16_t) (((uint32_t) sb[x] * invRB) >> 16) + n + ljus);
            r = std::min<int16_t> (std::max<int16_t> (r, 0), 255);
            g = std::min<int16_t> (std::max<int16_t> (g, 0), 255);
            b = std::min<int16_t> (std::max<int16_t> (b, 0), 255);
            const auto r2 = (uint16_t) ((uint16_t) ((uint16_t) r * ia[x]) >> 8) + gr[x];
            const auto g2 = (uint16_t) ((uint16_t) ((uint16_t) g * ia[x]) >> 8) + gg[x];
            const auto b2 = (uint16_t) ((uint16_t) ((uint16_t) b * ia[x]) >> 8) + gb[x];
            lr[x] = std::min<uint16_t> ((uint16_t) r2, 255);
            lg[x] = std::min<uint16_t> ((uint16_t) g2, 255);
            lb[x] = std::min<uint16_t> ((uint16_t) b2, 255);
        }
    }

    // Skriver en färdig linje till en rad i bilden med sveplinjens faktor (0..256).
    inline void skrivRad (int W, uint8_t* __restrict d, int ps, bool argb, const uint16_t* __restrict lr,
                          const uint16_t* __restrict lg, const uint16_t* __restrict lb, uint16_t mul)
    {
        if (ps == 3)
        {
            constexpr int iR = juce::PixelRGB::indexR, iG = juce::PixelRGB::indexG, iB = juce::PixelRGB::indexB;
            for (int x = 0; x < W; ++x)
            {
                d[x * 3 + iR] = (uint8_t) ((uint16_t) (lr[x] * mul) >> 8);
                d[x * 3 + iG] = (uint8_t) ((uint16_t) (lg[x] * mul) >> 8);
                d[x * 3 + iB] = (uint8_t) ((uint16_t) (lb[x] * mul) >> 8);
            }
            return;
        }
        const int iR = argb ? (int) juce::PixelARGB::indexR : (int) juce::PixelRGB::indexR;
        const int iG = argb ? (int) juce::PixelARGB::indexG : (int) juce::PixelRGB::indexG;
        const int iB = argb ? (int) juce::PixelARGB::indexB : (int) juce::PixelRGB::indexB;
        for (int x = 0; x < W; ++x)
        {
            uint8_t* p = d + x * ps;
            p[iR] = (uint8_t) ((uint16_t) (lr[x] * mul) >> 8);
            p[iG] = (uint8_t) ((uint16_t) (lg[x] * mul) >> 8);
            p[iB] = (uint8_t) ((uint16_t) (lb[x] * mul) >> 8);
            if (argb) p[juce::PixelARGB::indexA] = 255;
        }
    }

    // Brustabeller: luminansbrus (−127..127) och snö för spårningszonen (0..255, mest mörk).
    struct Brus
    {
        static constexpr int storlek = 1 << 16, mask = storlek - 1;
        std::vector<int16_t> brus;
        std::vector<uint8_t> sno;
        Brus() : brus ((size_t) storlek), sno ((size_t) storlek)
        {
            km::stil::Slump r (0x87u);
            for (int i = 0; i < storlek; ++i)
            {
                brus[(size_t) i] = (int16_t) juce::jlimit (-127, 127, (int) std::lround (r.bipolar() * 127.0f));
                const float s = r.enhet();
                sno[(size_t) i] = (uint8_t) juce::jlimit (0, 255, (int) (s * s * 255.0f));
            }
        }
        static const Brus& tabell() { static const Brus b; return b; }
    };
}

class Bildskarm final : public juce::Component
{
public:
    Bildskarm()
    {
        // Ogenomskinlig: hörnen utanför glaset ritas här (som infattningens fas), så att en ny
        // bildruta inte tvingar panelen bakom att ritas om.
        setOpaque (true);
        setInterceptsMouseClicks (false, false);
    }

    // Anropas från panelens timer (~30 Hz). Ritar om vid behov.
    void uppdatera (const SkarmData& d) { uppdateraVid (d, juce::Time::getMillisecondCounterHiRes()); }

    // Samma sak med given tid (ms), för provkörningar. Returnerar true om en ny bildruta begärdes.
    bool uppdateraVid (const SkarmData& d, double nuMs)
    {
        const float dt = forraMs < 0.0 ? 1.0f / 30.0f : (float) juce::jlimit (0.0, 0.25, (nuMs - forraMs) * 0.001);
        forraMs = nuMs;
        const bool forsta = ! harData;
        data = d;
        harData = true;
        rora (dt, forsta);

        // 25 bilder/s med en ackumulator, så att en 30 Hz-timer ger 5 bildrutor av 6.
        ack = juce::jmin (ack + dt * 1000.0f, 2.0f * bildtid);
        const bool bandPa = d.bandstyrka != Bandstyrka::Av;
        const uint64_t sig = bandPa ? 0 : signatur();
        const bool vill = bandPa || sig != ritadSignatur || ! harRitat;
        if (vill && ack >= bildtid)
        {
            ack = juce::jmin (ack - bildtid, bildtid);
            ritadSignatur = sig;
            harRitat = true;
            smutsig = true;
            ++fro;
            repaint();
            return true;
        }
        // Mellan bildrutorna: bygg en kolvbild i förväg, så att paint() sällan behöver göra det.
        forbyggEn();
        return false;
    }

    // Taket för den interna upplösningen (fysiska pixlar). Över taket räknas bilden mindre och
    // sträcks ut i paint(); andel < 1 räknar alltid i en del av den fysiska upplösningen.
    void setUpplosning (float andel, int maxBredd = 1024, int maxHojd = 490)
    {
        upplosning = juce::jlimit (0.25f, 1.0f, andel); maxB = maxBredd; maxH = maxHojd;
        storlek = {};
    }

    void paint (juce::Graphics& g) override
    {
        if (getWidth() <= 0 || getHeight() <= 0) return;
        const float s = juce::jlimit (0.25f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        forbered (s);
        if (smutsig || ! ut.isValid()) { rendera(); smutsig = false; }
        const double t1 = juce::Time::getMillisecondCounterHiRes();

        ritaHorn (g);
        {
            juce::Graphics::ScopedSaveState spara (g);
            g.reduceClipRegion (glasForm);
            const bool exakt = W == juce::roundToInt (w * s) && H == juce::roundToInt (h * s);
            g.setImageResamplingQuality (exakt ? juce::Graphics::lowResamplingQuality : juce::Graphics::mediumResamplingQuality);
            g.drawImageTransformed (ut, juce::AffineTransform::scale (w / (float) W, h / (float) H));
        }
        if (data.bandstyrka != Bandstyrka::Av)
            for (auto& f : osd)
                if (f.bild.isValid())
                    g.drawImageTransformed (f.bild, juce::AffineTransform::scale (1.0f / skalaOSD).translated (f.x, f.y));
        tider.paint += juce::Time::getMillisecondCounterHiRes() - t1;
    }

    // Hörnen mellan glasets rundning och komponentens kant: samma fas och svarta kant som
    // skarm::ritaInfattning ritar runt glaset (komponentens gränser = glasrutan).
    void ritaHorn (juce::Graphics& g) const
    {
        const auto glasRuta = getLocalBounds().toFloat();
        const auto fas = glasRuta.expanded (9.0f);
        const float hs = 30.0f;
        for (auto horn : { glasRuta.withSize (hs, hs), glasRuta.withSize (hs, hs).withRightX (glasRuta.getRight()),
                           glasRuta.withSize (hs, hs).withBottomY (glasRuta.getBottom()),
                           glasRuta.withSize (hs, hs).withRightX (glasRuta.getRight()).withBottomY (glasRuta.getBottom()) })
        {
            juce::Graphics::ScopedSaveState spara (g);
            g.reduceClipRegion (horn.toNearestIntEdges());
            g.setColour (km::stil::farg::plast);
            g.fillRect (horn);
            g.setGradientFill ({ juce::Colour (0xff050506), fas.getX(), fas.getY(), juce::Colour (0xff2c2c31), fas.getX(), fas.getBottom(), false });
            g.fillPath (km::stil::skarm::glasForm (fas, 32.0f, 4.0f));
            g.setColour (juce::Colours::black);
            g.fillPath (km::stil::skarm::glasForm (glasRuta.expanded (2.0f), 27.0f, 4.0f));
        }
    }

    // Provkörningarnas statistik (ms, summerat sedan start).
    struct Tider { double lager = 0, scen = 0, band = 0, paint = 0; int rutor = 0; };
    Tider tider;
    int internBredd() const { return W; }
    int internHojd() const  { return H; }

private:
    using Img = juce::Image;
    struct Lager { Img bild; juce::Point<int> pos; };
    struct OSDFalt { juce::String text; Img bild; float x = 0, y = 0; };

    static constexpr float bildtid = 40.0f;      // ms, 25 bilder/s
    static constexpr int vinklar = 24;           // kolvbilder per varv

    // --- tillstånd ------------------------------------------------------------------------------
    SkarmData data;
    bool harData = false, harRitat = false, smutsig = true;
    double forraMs = -1.0;
    float ack = 0.0f, tid = 0.0f;
    uint32_t fro = 1;
    uint64_t ritadSignatur = 0;

    scen::Rorelse rorelse;
    std::array<uint32_t, 4> forraKanalSlag {};
    std::array<uint32_t, 12> forraTrumSlag {};
    std::array<float, 5> toppTid {};
    float sedanSlag = 10.0f;
    int uppdateringar = 0;

    // --- cachar ---------------------------------------------------------------------------------
    float upplosning = 1.0f;
    int maxB = 1024, maxH = 490;
    float w = 640.0f, h = 304.0f;                // designstorlek (komponentens)
    juce::Rectangle<int> storlek;                // W, H som cacharna är byggda för
    int W = 0, H = 0;
    float sx = 1.0f, sy = 1.0f;                  // interna pixlar per designpixel
    juce::Path glasForm;
    Img komp, ut, skiss;
    std::vector<int> radFaktor;                  // sveplinjerna, 0..256 per rad
    // Bandpassets TV-linjer: raderna per linje, glaset per linje (kanalplan), arbetsplan.
    std::vector<juce::Range<int>> linjer;
    std::vector<uint16_t> glasIA, glasR, glasG, glasB;
    std::vector<uint16_t> planR, planG, planB, utR, utG, utB;
    std::array<std::vector<uint16_t>, 3> summa;
    int kant = 0;                                // utfyllnad på var sida av kanalplanen

    int scenNyckel = -1;
    Img bakgrund;
    Lager halv; uint64_t halvNyckel = ~0ull;
    std::array<std::array<Lager, vinklar>, 2> kolv;
    Lager fallvikt;
    std::array<std::array<Lager, 2>, 5> torn;    // mixern: tänt torn, vitt torn (topphållning)

    float skalaOSD = 0.0f;
    std::array<OSDFalt, 4> osd;

    // --- rörelsen -------------------------------------------------------------------------------
    static float dbSkala (float v)
    {
        if (v <= 1.0e-5f) return 0.0f;
        return juce::jlimit (0.0f, 1.0f, (20.0f * std::log10 (v) + 42.0f) / 42.0f);
    }

    void rora (float dt, bool forsta)
    {
        auto& m = rorelse;
        const auto& d = data;
        tid += dt;
        ++uppdateringar;
        if (forsta) { forraKanalSlag = d.kanalSlag; forraTrumSlag = d.trumSlag; }

        // Vevaxeln: ett varv på två steg, övre dödläget på varje jämnt steg. Står still när sekvensern står.
        if (d.steg >= 0)
        {
            const float pos = ((float) d.steg + juce::jlimit (0.0f, 1.0f, d.stegFas)) * 0.5f;
            m.vev = 2.0f * km::stil::pi * (pos - std::floor (pos));
        }

        // Slag: hammaren följer den valda trumman i Stampverket, bilan Huggets slag.
        bool slag = false;
        const auto sc = scen::scenFor (d.vy, d.kanal);
        if (sc == scen::Scen::Stampverket)
        {
            const auto v = (size_t) juce::jlimit (0, 11, d.valdTrumma);
            slag = d.trumSlag[v] != forraTrumSlag[v];
        }
        else if (sc == scen::Scen::Hugget) slag = d.kanalSlag[3] != forraKanalSlag[3];
        for (size_t i = 0; i < 12; ++i)
        {
            m.pelare[i] *= std::exp (-dt / 0.16f);
            if (d.trumSlag[i] != forraTrumSlag[i]) m.pelare[i] = 1.0f;
        }
        forraKanalSlag = d.kanalSlag; forraTrumSlag = d.trumSlag;
        if (slag) { sedanSlag = 0.0f; m.gnistor = 1.0f; }
        else
        {
            sedanSlag += dt;
            m.gnistor *= std::exp (-dt / 0.07f);
        }
        // Ner direkt, kvar en kort stund, sedan upp igen (bilan långsammare än hammaren).
        const float halla = 0.045f, upp = sc == scen::Scen::Hugget ? 0.16f : 0.09f;
        m.fall = sedanSlag < halla ? 1.0f : std::exp (-(sedanSlag - halla) / upp);

        // Mixerns torn: snabb attack, fall 1,4 skala/s, topphållning 0,8 s.
        for (size_t t = 0; t < 5; ++t)
        {
            const float v = dbSkala (t < 4 ? d.kanalNiva[t] : d.niva);
            m.niva[t] = juce::jmax (v, m.niva[t] - dt * 1.4f);
            if (v >= m.topp[t]) { m.topp[t] = v; toppTid[t] = 0.0f; }
            else if ((toppTid[t] += dt) > 0.8f) m.topp[t] = juce::jmax (0.0f, m.topp[t] - dt * 0.9f);
        }

        // Modulationsskrivarna.
        m.historik[0][(size_t) m.skriv] = juce::jlimit (-1.0f, 1.0f, d.lfoVarde[0]);
        m.historik[1][(size_t) m.skriv] = juce::jlimit (-1.0f, 1.0f, d.lfoVarde[1]);
        m.historik[2][(size_t) m.skriv] = juce::jlimit (0.0f, 1.0f, d.menvVarde[0]);
        m.historik[3][(size_t) m.skriv] = juce::jlimit (0.0f, 1.0f, d.menvVarde[1]);
        m.skriv = (m.skriv + 1) % scen::Rorelse::spar;

        // Oscilloskopets ekon: ett nytt svep var tredje uppdatering.
        if (uppdateringar % 3 == 0)
        {
            auto& s = m.svep[(size_t) (m.svepSkriv % scen::Rorelse::ekon)];
            for (size_t i = 0; i < s.size(); ++i) s[i] = (d.scope[i * 2] + d.scope[i * 2 + 1]) * 0.5f;
            m.svepSkriv = (m.svepSkriv + 1) % scen::Rorelse::ekon;
        }
    }

    // Allt som syns, hopslaget till ett tal (för läget Av, där vi bara ritar när något ändrats).
    uint64_t signatur() const
    {
        uint64_t hsh = 1469598103934665603ull;
        auto mix = [&hsh] (uint64_t v) { hsh ^= v + 0x9e3779b97f4a7c15ull; hsh *= 1099511628211ull; };
        auto q = [] (float v, float steg) { return (uint64_t) (int64_t) std::lround (v * steg); };
        const auto& d = data;
        const auto& m = rorelse;
        const auto sc = scen::scenFor (d.vy, d.kanal);
        mix ((uint64_t) sc); mix ((uint64_t) d.bandstyrka);
        mix (scen::halvstatiskNyckel (sc, d));
        if (scen::harSteg (sc)) mix ((uint64_t) (int64_t) d.steg);
        if (scen::harKolvar (sc)) mix ((uint64_t) kolvIndex (m.vev));
        if (scen::harFallvikt (sc)) { mix (q (m.fall, 48.0f)); mix (q (m.gnistor, 16.0f)); }
        if (sc == scen::Scen::Stampverket) for (auto p : m.pelare) mix (q (p, 40.0f));
        if (sc == scen::Scen::Mixer) for (size_t t = 0; t < 5; ++t) { mix (q (m.niva[t], 12.0f)); mix (q (m.topp[t], 12.0f)); }
        if (scen::harKolvar (sc) || sc == scen::Scen::Hugget || sc == scen::Scen::Effekter)
            for (size_t i = 0; i < d.scope.size(); i += 4) mix (q (d.scope[i], 64.0f));
        if (sc == scen::Scen::Effekter) for (auto& s : m.svep) for (size_t i = 0; i < s.size(); i += 4) mix (q (s[i], 64.0f));
        if (sc == scen::Scen::Modulation)
            for (auto& hi : m.historik)
                for (int i = 0; i < scen::Rorelse::spar; ++i) mix (q (hi[(size_t) ((m.skriv + i) % scen::Rorelse::spar)], 96.0f));
        return hsh;
    }

    static int kolvIndex (float vev)
    {
        const int i = (int) std::lround (vev / (2.0f * km::stil::pi) * (float) vinklar);
        return ((i % vinklar) + vinklar) % vinklar;
    }

    float bandStyrka() const
    {
        switch (data.bandstyrka)
        {
            case Bandstyrka::Av:     return 0.0f;
            case Bandstyrka::Lite:   return 0.5f;
            case Bandstyrka::Mycket: return 1.0f;
        }
        return 1.0f;
    }

    scen::Scen aktuellScen() const { return scen::scenFor (data.vy, data.kanal); }

    // --- cacharna -------------------------------------------------------------------------------
    static Img nyBild (Img::PixelFormat f, int bw, int bh) { return Img (f, juce::jmax (1, bw), juce::jmax (1, bh), true, juce::SoftwareImageType()); }

    juce::AffineTransform designTillIntern() const { return juce::AffineTransform::scale (sx, sy); }

    void forbered (float s)
    {
        w = (float) getWidth(); h = (float) getHeight();
        const int pw = juce::roundToInt (w * s), ph = juce::roundToInt (h * s);
        const int nyW = juce::jlimit (16, maxB, juce::roundToInt ((float) pw * upplosning));
        const int nyH = juce::jlimit (16, maxH, juce::roundToInt ((float) ph * upplosning));
        if (nyW != storlek.getWidth() || nyH != storlek.getHeight())
        {
            storlek = { nyW, nyH };
            W = nyW; H = nyH;
            sx = (float) W / w; sy = (float) H / h;
            komp  = nyBild (Img::RGB, W, H);
            ut    = nyBild (Img::RGB, W, H);
            skiss = nyBild (Img::ARGB, W, H);
            kant = (int) std::ceil (52.0f * sx) + 8;
            for (auto* v : { &planR, &planG, &planB }) v->assign ((size_t) (W + 2 * kant), 0);
            for (auto* v : { &utR, &utG, &utB, &summa[0], &summa[1], &summa[2] }) v->assign ((size_t) W, 0);
            byggGlas();
            scenNyckel = -1;
            smutsig = true;
        }
        if (std::abs (s - skalaOSD) > 1.0e-4f) { skalaOSD = s; for (auto& f : osd) f = {}; smutsig = true; }

        const int sn = (int) aktuellScen();
        if (sn != scenNyckel)
        {
            scenNyckel = sn;
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            byggBakgrund();
            halvNyckel = ~0ull; halv = {};
            for (auto& k : kolv) for (auto& l : k) l = {};
            for (auto& t : torn) for (auto& l : t) l = {};
            fallvikt = {};
            smutsig = true;
            tider.lager += juce::Time::getMillisecondCounterHiRes() - t0;
        }
        uppdateraOSD();
    }

    void byggGlas()
    {
        glasForm = km::stil::skarm::glasForm ({ 0.0f, 0.0f, w, h });
        auto glas = nyBild (Img::ARGB, W, H);
        {
            juce::Graphics g (glas);
            g.addTransform (designTillIntern());
            km::stil::skarm::ritaGlas (g, { 0.0f, 0.0f, w, h }, glasForm);
        }
        radFaktor.assign ((size_t) H, 256);
        for (int y = 0; y < H; ++y)
            radFaktor[(size_t) y] = juce::roundToInt (band::sveplinje ((float) y / sy, (float) (y + 1) / sy, 0.30f) * 256.0f);

        // TV-linjerna (två designpixlar var) och glaset vid varje linjes mitt, som kanalplan.
        linjer.clear();
        for (int L = 0; L < (int) std::ceil (h * 0.5f); ++L)
        {
            const int y0 = juce::jmin (H, juce::roundToInt ((float) (2 * L) * sy));
            const int y1 = juce::jmin (H, juce::roundToInt ((float) (2 * L + 2) * sy));
            if (y1 > y0) linjer.push_back ({ y0, y1 });
        }
        for (auto* v : { &glasIA, &glasR, &glasG, &glasB }) v->assign (linjer.size() * (size_t) W, 0);
        juce::Image::BitmapData gd (glas, juce::Image::BitmapData::readOnly);
        for (size_t L = 0; L < linjer.size(); ++L)
        {
            const uint8_t* rad = gd.getLinePointer ((linjer[L].getStart() + linjer[L].getEnd()) / 2);
            for (int x = 0; x < W; ++x)
            {
                const uint8_t* p = rad + x * gd.pixelStride;
                const int a = p[juce::PixelARGB::indexA];
                const size_t i = L * (size_t) W + (size_t) x;
                glasIA[i] = (uint16_t) (256 - a - (a >> 7));
                glasR[i] = p[juce::PixelARGB::indexR]; glasG[i] = p[juce::PixelARGB::indexG]; glasB[i] = p[juce::PixelARGB::indexB];
            }
        }
    }

    void byggBakgrund()
    {
        bakgrund = nyBild (Img::RGB, W, H);
        juce::Graphics g (bakgrund);
        g.addTransform (designTillIntern());
        scen::ritaBakgrund (g, { 0.0f, 0.0f, w, h }, aktuellScen());
    }

    // Ritar ett lager i skissbilden och klipper ut den synliga delen.
    template <typename Rita>
    Lager byggLager (Rita&& rita)
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        {
            juce::Image::BitmapData bd (skiss, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < H; ++y) std::memset (bd.getLinePointer (y), 0, (size_t) (W * bd.pixelStride));
        }
        {
            juce::Graphics g (skiss);
            g.addTransform (designTillIntern());
            rita (g, juce::Rectangle<float> (0.0f, 0.0f, w, h));
        }
        const auto yta = band::alfaRuta (skiss);
        Lager l;
        if (! yta.isEmpty()) l = { skiss.getClippedImage (yta).createCopy(), yta.getPosition() };
        tider.lager += juce::Time::getMillisecondCounterHiRes() - t0;
        return l;
    }

    Lager& kolvLager (int slot, int index)
    {
        auto& l = kolv[(size_t) slot][(size_t) index];
        if (! l.bild.isValid())
        {
            const auto sc = aktuellScen();
            const float vinkel = 2.0f * km::stil::pi * (float) index / (float) vinklar;
            l = byggLager ([&] (juce::Graphics& g, juce::Rectangle<float> r)
            {
                km::stil::skarm::ritaKolv (g, scen::kolvKamera (r, sc), scen::kolvPlacering (sc, slot), vinkel);
            });
        }
        return l;
    }

    // Bygger en saknad kolvbild i förväg (högst en per anrop).
    void forbyggEn()
    {
        const auto sc = aktuellScen();
        if (W <= 0 || scenNyckel != (int) sc || ! scen::harKolvar (sc)) return;
        for (int slot = 0; slot < scen::antalKolvar (sc); ++slot)
            for (int i = 0; i < vinklar; ++i)
                if (! kolv[(size_t) slot][(size_t) i].bild.isValid()) { kolvLager (slot, i); return; }
    }

    void uppdateraOSD()
    {
        const float p = 2.6f, kantX = 26.0f, radH = 7.0f * p;
        const float yl = h * 0.90f - radH - 14.0f;
        const juce::String* texter[4] { &data.osdLage, &data.osdKvalitet, &data.osdKanal, &data.osdRakneverk };
        for (int i = 0; i < 4; ++i)
        {
            auto& f = osd[(size_t) i];
            const auto& t = *texter[i];
            if (f.text == t && (f.bild.isValid() || t.isEmpty())) continue;
            f.text = t;
            f.bild = {};
            if (t.isEmpty()) continue;
            const float b = km::stil::osd::bredd (t, p);
            const float tx = (i % 2 == 0) ? kantX : w - kantX - b;
            const float ty = i < 2 ? 20.0f : yl;
            const float d = juce::jmax (1.0f, p * 0.5f);
            const float s = skalaOSD;
            // Rutan i hela fysiska pixlar, så att texten blir skarp.
            const float x0 = std::floor ((tx - d - 1.0f) * s) / s, y0 = std::floor ((ty - 1.0f) * s) / s;
            const int bw = (int) std::ceil ((b + 3.0f * d + 2.0f) * s), bh = (int) std::ceil ((radH + 2.0f * d + 2.0f) * s);
            f.bild = nyBild (Img::ARGB, bw, bh);
            f.x = x0; f.y = y0;
            {
                juce::Graphics g (f.bild);
                g.addTransform (juce::AffineTransform::translation (-x0, -y0).scaled (s));
                km::stil::osd::rita (g, t, tx, ty, p);
            }
            // Sveplinjerna även i texten (den går genom bildröret).
            juce::Image::BitmapData bd (f.bild, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < bh; ++y)
            {
                const float yd = y0 + (float) y / s;
                const int fk = juce::roundToInt (band::sveplinje (yd, yd + 1.0f / s, 0.30f) * 256.0f);
                auto* rad = bd.getLinePointer (y);
                for (int x = 0; x < bw * 4; ++x)
                    if ((x & 3) != juce::PixelARGB::indexA) rad[x] = (uint8_t) ((rad[x] * fk) >> 8);
            }
        }
    }

    // --- en bildruta ----------------------------------------------------------------------------
    void rendera()
    {
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        const double lager0 = tider.lager;
        const auto sc = aktuellScen();
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, w, h);
        auto& m = rorelse;

        // 1. Bakgrunden.
        {
            juce::Image::BitmapData in (bakgrund, juce::Image::BitmapData::readOnly);
            juce::Image::BitmapData ub (komp, juce::Image::BitmapData::writeOnly);
            for (int y = 0; y < H; ++y) std::memcpy (ub.getLinePointer (y), in.getLinePointer (y), (size_t) (W * in.pixelStride));
        }
        juce::Graphics gk (komp);

        // 2. Halvstatiskt lager och sprajtar.
        const uint64_t hn = scen::halvstatiskNyckel (sc, data);
        if (hn != 0 && hn != halvNyckel)
        {
            halvNyckel = hn;
            halv = byggLager ([&] (juce::Graphics& g, juce::Rectangle<float> rr) { scen::ritaHalvstatiskt (g, rr, sc, data); });
        }
        if (hn != 0 && halv.bild.isValid()) gk.drawImageAt (halv.bild, halv.pos.x, halv.pos.y);

        if (scen::harKolvar (sc))
        {
            const int i0 = kolvIndex (m.vev);
            for (int slot = 0; slot < scen::antalKolvar (sc); ++slot)
            {
                // Den andra kolven går ett halvt varv efter.
                auto& l = kolvLager (slot, (i0 + slot * vinklar / 2) % vinklar);
                if (l.bild.isValid()) gk.drawImageAt (l.bild, l.pos.x, l.pos.y);
            }
        }
        if (scen::harFallvikt (sc))
        {
            if (! fallvikt.bild.isValid())
                fallvikt = byggLager ([&] (juce::Graphics& g, juce::Rectangle<float> rr)
                {
                    if (sc == scen::Scen::Hugget) scen::ritaBila (g, rr); else scen::ritaHammare (g, rr);
                });
            const auto v = scen::fallVektor (r, sc) * m.fall;
            if (fallvikt.bild.isValid())
                gk.drawImageAt (fallvikt.bild, fallvikt.pos.x + juce::roundToInt (v.x * sx), fallvikt.pos.y + juce::roundToInt (v.y * sy));
        }
        if (sc == scen::Scen::Mixer) ritaTornen (gk, r);

        // 3. Det levande, direkt i bilden.
        {
            juce::Graphics::ScopedSaveState spara (gk);
            gk.addTransform (designTillIntern());
            scen::ritaLevande (gk, r, sc, data, m, fro);
        }
        const double t1 = juce::Time::getMillisecondCounterHiRes();

        // 4. Bandet, sveplinjerna och glaset i ett pass.
        bandpass();
        const double t2 = juce::Time::getMillisecondCounterHiRes();
        tider.scen += (t1 - t0) - (tider.lager - lager0);
        tider.band += t2 - t1;
        ++tider.rutor;
    }

    // Mixertornen: det tända tornet klipps vid nivån, det vita vid topphållningens segment.
    void ritaTornen (juce::Graphics& gk, juce::Rectangle<float> r)
    {
        auto snitt = [&] (int t, int seg) { return juce::roundToInt (scen::tornSnitt (r, t, seg) * sy); };
        for (int t = 0; t < 5; ++t)
        {
            for (int v = 0; v < 2; ++v)
                if (! torn[(size_t) t][(size_t) v].bild.isValid())
                    torn[(size_t) t][(size_t) v] = byggLager ([&] (juce::Graphics& g, juce::Rectangle<float> rr) { scen::ritaTorn (g, rr, t, v == 1); });
            const int tanda = juce::jlimit (0, scen::mixSegment, (int) std::lround (rorelse.niva[(size_t) t] * (float) scen::mixSegment));
            const int ts = juce::jmin (scen::mixSegment - 1, (int) std::floor (rorelse.topp[(size_t) t] * (float) scen::mixSegment - 0.001f));
            auto rita = [&] (const Lager& l, int yUpp, int yNer)
            {
                if (! l.bild.isValid() || yNer <= yUpp) return;
                juce::Graphics::ScopedSaveState spara (gk);
                gk.reduceClipRegion (juce::Rectangle<int> (l.pos.x, yUpp, l.bild.getWidth(), yNer - yUpp));
                gk.drawImageAt (l.bild, l.pos.x, l.pos.y);
            };
            if (tanda > 0) rita (torn[(size_t) t][0], snitt (t, tanda), H);
            if (ts >= 0 && ts >= tanda) rita (torn[(size_t) t][1], snitt (t, ts + 1), snitt (t, ts));
        }
    }

    // Bandet: ett heltalspass över bilden, komp → ut.
    //
    // Bilden byggs av TV-linjer som på en riktig bildskärm: en linje är två designpixlar hög (152
    // linjer i 304), räknas en gång (medel av sina rader, darr, färgförskjutning, utsmetning, brus,
    // glas) och skrivs sedan ut på sina fysiska rader med sveplinjernas profil. Halva vertikala
    // upplösningen är äkta 80-tal och halverar arbetet. Linjen räknas i 16-bitars kanalplan, så att
    // kompilatorn kan vektorisera (SSE2/NEON).
    void bandpass()
    {
        const float st = bandStyrka();
        const float sl = st > 0.0f ? juce::jlimit (0.0f, 1.0f, data.slitage) : 0.0f;
        const float slit = 1.0f + 1.5f * sl;                // slitaget förstärker felen
        const auto& tab = band::Brus::tabell();
        km::stil::Slump rnd (fro * 2654435761u + 0x87u);

        juce::Image::BitmapData in (komp, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData ub (ut, juce::Image::BitmapData::writeOnly);
        jassert (in.pixelStride == 3);       // komp är alltid en RGB-bild av mjukvarutyp
        const int skak = rorelse.gnistor > 0.55f ? juce::roundToInt (1.6f * sy * rorelse.gnistor) : 0;

        // Färgförskjutning (rött åt höger, blått åt vänster) och utsmetning per kanal, i internpixlar.
        const int kr = juce::roundToInt (1.7f * st * (1.0f + 0.9f * sl) * sx);
        const int rRB = juce::roundToInt (1.6f * st * (1.0f + 1.2f * sl) * sx);
        const int rG = juce::roundToInt (0.5f * st * (1.0f + sl) * sx);
        const int maxDx = kant - kr - rRB - 2;

        // Bildrutans gemensamma parametrar (designkoordinater).
        const float zonStart = h * (0.90f - 0.04f * sl);
        const float svepH = h * 0.07f;
        const float svepY = h * (std::fmod (tid * 0.085f, 1.35f) - 0.15f);
        const auto brusAmp = (int16_t) juce::roundToInt (10.0f * st * (0.6f + 0.4f * slit) * 256.0f / 127.0f);
        float ryckY = -1.0f, ryckH = 0.0f, ryckDx = 0.0f;              // ett band som hoppar i sidled
        if (st > 0.0f && rnd.enhet() < 0.03f * st * slit * slit)
        {
            ryckY = rnd.enhet() * h * 0.85f;
            ryckH = 4.0f + rnd.enhet() * 18.0f;
            ryckDx = (3.0f + rnd.enhet() * 9.0f) * (rnd.enhet() < 0.5f ? -1.0f : 1.0f);
        }
        uint16_t* R = planR.data() + kant; uint16_t* G = planG.data() + kant; uint16_t* B = planB.data() + kant;
        uint16_t* lr = utR.data(); uint16_t* lg = utG.data(); uint16_t* lb = utB.data();

        float darr = 0.0f;
        for (size_t L = 0; L < linjer.size(); ++L)
        {
            const int y0 = linjer[L].getStart(), y1 = linjer[L].getEnd(), k = y1 - y0;
            const float yd = (float) (2 * L) + 1.0f;      // linjens mitt i design

            // Linjens förskjutning och ljus.
            float dx = 0.0f, ljus = 0.0f;
            int gra = 0;
            const bool iZon = st > 0.0f && yd >= zonStart;
            if (st > 0.0f)
            {
                darr = darr * 0.81f + rnd.bipolar() * 0.17f * slit;
                dx = (darr + 0.35f * std::sin (yd * 0.11f + tid * 1.7f)) * st;
                if (iZon)
                {
                    const float t = (yd - zonStart) / (h - zonStart);
                    dx += (5.0f + 14.0f * t * t + rnd.bipolar() * 3.0f) * st;
                    gra = juce::roundToInt ((0.22f + 0.38f * t) * st * 256.0f);
                    ljus = rnd.enhet() < 0.15f ? 40.0f * st : 0.0f;
                }
                const float ds = (yd - svepY) / svepH;
                if (std::abs (ds) < 3.0f)
                {
                    const float e = std::exp (-ds * ds);
                    ljus += e * 14.0f * st;
                    dx += e * 1.2f * st;
                }
                if (yd >= ryckY && yd < ryckY + ryckH) dx += ryckDx;
            }
            const int xi = juce::jlimit (-maxDx, maxDx, juce::roundToInt (dx * sx));

            // Linjens rader summeras i kanalplanen; kanterna upprepas 'kant' pixlar ut.
            for (int j = 0; j < k; ++j)
                band::laggTill (W, in.getLinePointer (juce::jlimit (0, H - 1, y0 + j - skak)), R, G, B, j == 0);
            for (int i = 1; i <= kant; ++i)
            {
                R[-i] = R[0]; G[-i] = G[0]; B[-i] = B[0];
                R[W - 1 + i] = R[W - 1]; G[W - 1 + i] = G[W - 1]; B[W - 1 + i] = B[W - 1];
            }

            // Lådfilter med färgförskjutning: r från vänster, b från höger.
            std::fill (summa[0].begin(), summa[0].end(), (uint16_t) 0);
            std::fill (summa[1].begin(), summa[1].end(), (uint16_t) 0);
            std::fill (summa[2].begin(), summa[2].end(), (uint16_t) 0);
            for (int j = -rRB; j <= rRB; ++j)
            {
                band::adderaPlan (W, R - xi - kr + j, summa[0].data());
                band::adderaPlan (W, B - xi + kr + j, summa[2].data());
            }
            for (int j = -rG; j <= rG; ++j) band::adderaPlan (W, G - xi + j, summa[1].data());

            const auto invRB = (uint16_t) juce::jmin (65535, 65536 / ((2 * rRB + 1) * k));
            const auto invG  = (uint16_t) juce::jmin (65535, 65536 / ((2 * rG + 1) * k));
            const int off = (int) (rnd.nasta() % (uint32_t) (band::Brus::storlek - W));
            const size_t gi = L * (size_t) W;
            band::linjePass (W, summa[0].data(), summa[1].data(), summa[2].data(), tab.brus.data() + off,
                             glasIA.data() + gi, glasR.data() + gi, glasG.data() + gi, glasB.data() + gi,
                             lr, lg, lb, invRB, invG, brusAmp, (int16_t) juce::roundToInt (ljus));

            // Spårningszonens snö (få linjer, enkel slinga).
            if (gra > 0)
            {
                const int so = (int) (rnd.nasta() % (uint32_t) (band::Brus::storlek - W));
                const uint8_t* sn = tab.sno.data() + so;
                for (int x = 0; x < W; ++x)
                {
                    const int v = sn[x];
                    lr[x] = (uint16_t) juce::jlimit (0, 255, lr[x] + (((v - lr[x]) * gra) >> 8));
                    lg[x] = (uint16_t) juce::jlimit (0, 255, lg[x] + (((v - lg[x]) * gra) >> 8));
                    lb[x] = (uint16_t) juce::jlimit (0, 255, lb[x] + (((v - lb[x]) * gra) >> 8));
                }
            }

            // Vita streck: tappningar i bilden, spårningsbrusets streck i zonen.
            if (st > 0.0f)
            {
                int start = -1, langd = 0, styrka = 0;
                if (! iZon && rnd.enhet() < 0.008f * st * slit * slit)
                {
                    start = (int) (rnd.enhet() * (float) W); langd = (int) ((8.0f + rnd.enhet() * 40.0f) * sx); styrka = 200;
                }
                else if (iZon && rnd.enhet() < 0.6f)
                {
                    start = (int) (rnd.enhet() * (float) W); langd = (int) ((10.0f + rnd.enhet() * 120.0f) * sx); styrka = (int) (230.0f * st);
                }
                if (start >= 0 && langd > 0)
                    for (int x = start; x < juce::jmin (W, start + langd); ++x)
                    {
                        const int a = (int) (std::sin (km::stil::pi * (float) (x - start) / (float) langd) * (float) styrka);
                        const int v = (a * glasIA[gi + (size_t) x]) >> 8;
                        lr[x] = (uint16_t) juce::jmin (255, lr[x] + v);
                        lg[x] = (uint16_t) juce::jmin (255, lg[x] + v);
                        lb[x] = (uint16_t) juce::jmin (255, lb[x] + v);
                    }
            }

            // Linjen ut på sina rader, med sveplinjernas profil.
            for (int y = y0; y < y1; ++y)
                band::skrivRad (W, ub.getLinePointer (y), ub.pixelStride, ub.pixelFormat == juce::Image::ARGB,
                                lr, lg, lb, (uint16_t) radFaktor[(size_t) y]);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bildskarm)
};

} // namespace km::vhs
