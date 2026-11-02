// Stampverket – trummorna: tolv syntetiska ljud i fyra kit.
//
// Varje ljud byggs av samma delar – en ton med tonhöjdssvep, brus genom ett filter, metall (sex fyrkanter),
// handklappens upprepade brusslag, modala delton (städ och plåt), ett kamfilter (fabrikshallen), ett klick,
// mättnad och bitkross. Ett *recept* säger hur mycket av varje del ett ljud har i ett kit, så att de fyra
// kiten blir en tabell i stället för 48 klasser.
//
// Kiten: Åttaåtta (mjuk, lång, 808-liknande), Niononio (hårdare, mer brus, 909-liknande), Digital (kort och
// torr med 8-bitarskorn, som de tidiga samplade trummaskinerna) och Fabrik (industri: kam, mättnad, metall).
//
// Varje ljud har Stämning (halvtoner), Klang (hur länge det klingar) och Nivå. Stängd hi-hat tystar den öppna.
// Spårens ordning är fast i mönsterformatet och får aldrig ändras.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "Grund.h"
#include <array>
#include <vector>

namespace km
{

enum class Trumma
{
    Bastrumma = 0, Virvel, Virvel2, Handklapp, StangdHihat, OppenHihat,
    Cymbal, TomLag, TomHog, Metall, Small, Kubjallra
};
inline constexpr int antalSpar = 12;

enum class Kit { Attaatta = 0, Niononio, Digital, Fabrik };
inline constexpr int antalKit = 4;

inline const char* trumNamn (Trumma t)
{
    switch (t)
    {
        case Trumma::Bastrumma:   return "Bastrumma";
        case Trumma::Virvel:      return "Virvel";
        case Trumma::Virvel2:     return "Virvel 2";
        case Trumma::Handklapp:   return "Handklapp";
        case Trumma::StangdHihat: return "Hi-hat";
        case Trumma::OppenHihat:  return "Öppen hi-hat";
        case Trumma::Cymbal:      return "Cymbal";
        case Trumma::TomLag:      return "Tom låg";
        case Trumma::TomHog:      return "Tom hög";
        case Trumma::Metall:      return "Metall";
        case Trumma::Small:       return "Smäll";
        case Trumma::Kubjallra:   return "Kubjällra";
    }
    return "";
}

struct TrumParams
{
    float stamning = 0.0f; // halvtoner −12..+12
    float klang    = 0.5f; // 0..1
    float niva     = 0.8f; // 0..1
};

struct StampverketParams
{
    std::array<TrumParams, antalSpar> trummor {};
    Kit kit = Kit::Attaatta;
    bool kantslag = false;   // spår 12 spelar kantslag i stället för kubjällra
    float stamningOffset = 0.0f; // halvtoner för alla spår (Varvtalet)
    float tidFaktor = 1.0f;      // alla fall förlängs med detta (Varvtalet)
};

// Ett ljuds recept. Tider i sekunder, frekvenser i Hz. Noll betyder att delen inte används.
struct Recept
{
    float niva = 1.0f;
    // Ton: sinus (eller fyrkant) med tonhöjdssvep; ton 2 följer med en kvot och kortare fall.
    float tonHz = 0, tonNiva = 0, tonSvep = 0, tonSvepTid = 0.02f, tonFall = 0.2f;
    float ton2Kvot = 0, ton2Niva = 0;
    bool tonFyrkant = false;
    float tonBp = 0;                       // bandpass på tonerna (kubjällran)
    // Brus genom ett filter: 0 lågpass, 1 bandpass, 2 högpass.
    float brusNiva = 0, brusHz = 2000, brusQ = 0.7f, brusFall = 0.1f;
    int brusTyp = 2;
    // Handklapp: bruset slår an flera gånger innan det klingar ut.
    int klappSlag = 0;
    float klappAvstand = 0.011f;
    // Metall: sex fyrkanter i inharmoniska förhållanden, genom bandpass och högpass.
    float metallNiva = 0, metallSkala = 1.7f, metallBp = 10000, metallHp = 7000, metallFall = 0.05f;
    // Modala delton: städ, plåt.
    float modalNiva = 0, modalHz = 0, modalFall = 0.6f;
    // Kamfilter på bruset: fabrikshallen.
    float kamMs = 0, kamAter = 0;
    // Klick i anslaget.
    float klickNiva = 0;
    // Färg: mättnad och bitkross (bitar 0 = av, hall = sample att hålla).
    float drive = 1.0f, bitar = 0;
    int hall = 1;
};

namespace recept
{
    inline Recept bastrumma (Kit k)
    {
        Recept r;
        r.niva = 1.25f;
        switch (k)
        {
            case Kit::Attaatta: r.tonHz = 49; r.tonNiva = 1; r.tonSvep = 2.0f; r.tonSvepTid = 0.03f; r.tonFall = 0.45f; r.klickNiva = 0.05f; r.drive = 1.3f; break;
            case Kit::Niononio: r.tonHz = 55; r.tonNiva = 1; r.tonSvep = 4.0f; r.tonSvepTid = 0.015f; r.tonFall = 0.3f; r.klickNiva = 0.35f;
                                r.brusNiva = 0.15f; r.brusTyp = 0; r.brusHz = 3000; r.brusFall = 0.01f; r.drive = 2.0f; break;
            case Kit::Digital:  r.tonHz = 60; r.tonNiva = 1; r.tonSvep = 3.0f; r.tonSvepTid = 0.012f; r.tonFall = 0.2f; r.klickNiva = 0.25f;
                                r.drive = 1.6f; r.bitar = 8; r.hall = 2; break;
            case Kit::Fabrik:   r.tonHz = 45; r.tonNiva = 1; r.tonSvep = 5.0f; r.tonSvepTid = 0.02f; r.tonFall = 0.35f; r.klickNiva = 0.4f;
                                r.brusNiva = 0.3f; r.brusTyp = 0; r.brusHz = 800; r.brusFall = 0.04f; r.drive = 4.5f; r.niva = 1.05f; break;
        }
        return r;
    }

    inline Recept virvel (Kit k)
    {
        Recept r;
        switch (k)
        {
            case Kit::Attaatta: r.tonHz = 238; r.tonNiva = 0.7f; r.tonSvep = 0.2f; r.tonSvepTid = 0.01f; r.tonFall = 0.1f; r.ton2Kvot = 2.0f; r.ton2Niva = 0.4f;
                                r.brusNiva = 0.6f; r.brusTyp = 2; r.brusHz = 1800; r.brusFall = 0.12f; r.drive = 1.3f; break;
            case Kit::Niononio: r.tonHz = 200; r.tonNiva = 0.6f; r.tonSvep = 0.5f; r.tonSvepTid = 0.006f; r.tonFall = 0.07f; r.ton2Kvot = 1.65f; r.ton2Niva = 0.4f;
                                r.brusNiva = 1.0f; r.brusTyp = 1; r.brusHz = 4500; r.brusQ = 0.6f; r.brusFall = 0.16f; r.drive = 1.8f; break;
            case Kit::Digital:  r.tonHz = 180; r.tonNiva = 0.5f; r.tonSvep = 0.3f; r.tonFall = 0.06f; r.ton2Kvot = 1.6f; r.ton2Niva = 0.3f;
                                r.brusNiva = 0.9f; r.brusTyp = 2; r.brusHz = 2500; r.brusFall = 0.12f; r.bitar = 8; r.hall = 2; r.drive = 1.5f; break;
            case Kit::Fabrik:   r.tonHz = 160; r.tonNiva = 0.6f; r.tonSvep = 0.8f; r.tonFall = 0.08f;
                                r.brusNiva = 1.0f; r.brusTyp = 1; r.brusHz = 1200; r.brusQ = 1.2f; r.brusFall = 0.25f; r.kamMs = 3.3f; r.kamAter = 0.6f; r.drive = 4.0f; r.niva = 0.8f; break;
        }
        return r;
    }

    // Virvel 2: tjockare och längre, gjord för att gatas i Grindrummet (fas 2).
    inline Recept virvel2 (Kit k)
    {
        Recept r;
        switch (k)
        {
            case Kit::Attaatta: r.tonHz = 180; r.tonNiva = 0.5f; r.tonFall = 0.12f; r.brusNiva = 0.8f; r.brusTyp = 2; r.brusHz = 1200; r.brusFall = 0.3f; r.drive = 1.5f; break;
            case Kit::Niononio: r.tonHz = 220; r.tonNiva = 0.5f; r.tonSvep = 0.4f; r.tonFall = 0.1f; r.ton2Kvot = 1.5f; r.ton2Niva = 0.3f;
                                r.brusNiva = 1.0f; r.brusTyp = 1; r.brusHz = 3000; r.brusQ = 0.5f; r.brusFall = 0.35f; r.drive = 2.0f; break;
            case Kit::Digital:  r.tonHz = 150; r.tonNiva = 0.6f; r.tonFall = 0.1f; r.brusNiva = 1.0f; r.brusTyp = 2; r.brusHz = 1500; r.brusFall = 0.3f; r.bitar = 10; r.drive = 1.8f; break;
            case Kit::Fabrik:   r.tonHz = 120; r.tonNiva = 0.5f; r.tonSvep = 1.0f; r.tonFall = 0.1f;
                                r.brusNiva = 1.0f; r.brusTyp = 1; r.brusHz = 900; r.brusQ = 2.0f; r.brusFall = 0.4f; r.kamMs = 7.0f; r.kamAter = 0.7f; r.drive = 5.0f; r.niva = 0.75f; break;
        }
        return r;
    }

    inline Recept handklapp (Kit k)
    {
        Recept r;
        r.brusTyp = 1;
        r.brusNiva = 1.0f;
        switch (k)
        {
            case Kit::Attaatta: r.brusHz = 1100; r.brusQ = 1.5f; r.brusFall = 0.16f; r.klappSlag = 3; r.klappAvstand = 0.011f; r.drive = 1.2f; break;
            case Kit::Niononio: r.brusHz = 1400; r.brusQ = 1.2f; r.brusFall = 0.2f; r.klappSlag = 4; r.klappAvstand = 0.009f; r.drive = 1.6f; break;
            case Kit::Digital:  r.brusHz = 1200; r.brusQ = 1.3f; r.brusFall = 0.14f; r.klappSlag = 3; r.klappAvstand = 0.01f; r.bitar = 8; r.hall = 2; break;
            case Kit::Fabrik:   r.brusHz = 800; r.brusQ = 2.0f; r.brusFall = 0.25f; r.klappSlag = 4; r.klappAvstand = 0.014f; r.kamMs = 2.2f; r.kamAter = 0.5f; r.drive = 3.0f; r.niva = 0.8f; break;
        }
        return r;
    }

    inline Recept hihat (Kit k, bool oppen)
    {
        Recept r;
        switch (k)
        {
            case Kit::Attaatta: r.metallNiva = 0.9f; r.metallSkala = 1.7f; r.metallBp = 10000; r.metallHp = 7000; r.metallFall = 0.04f; break;
            case Kit::Niononio: r.metallNiva = 0.5f; r.metallSkala = 2.0f; r.metallBp = 11000; r.metallHp = 8000; r.metallFall = 0.045f;
                                r.brusNiva = 0.5f; r.brusTyp = 2; r.brusHz = 9000; r.brusFall = 0.035f; break;
            case Kit::Digital:  r.brusNiva = 0.8f; r.brusTyp = 2; r.brusHz = 8000; r.brusFall = 0.03f; r.metallNiva = 0.3f; r.metallFall = 0.03f; r.bitar = 9; break;
            case Kit::Fabrik:   r.metallNiva = 1.0f; r.metallSkala = 1.2f; r.metallBp = 6000; r.metallHp = 4000; r.metallFall = 0.05f; r.drive = 2.5f; r.niva = 0.8f; break;
        }
        if (oppen) { r.metallFall *= 8.0f; r.brusFall *= 8.0f; }
        return r;
    }

    inline Recept cymbal (Kit k)
    {
        Recept r;
        switch (k)
        {
            case Kit::Attaatta: r.metallNiva = 0.7f; r.metallSkala = 1.5f; r.metallBp = 8000; r.metallHp = 5000; r.metallFall = 0.9f;
                                r.brusNiva = 0.3f; r.brusTyp = 2; r.brusHz = 6000; r.brusFall = 0.8f; break;
            case Kit::Niononio: r.brusNiva = 0.6f; r.brusTyp = 2; r.brusHz = 7000; r.brusFall = 1.0f;
                                r.metallNiva = 0.5f; r.metallSkala = 2.2f; r.metallBp = 9000; r.metallHp = 6000; r.metallFall = 1.0f; break;
            case Kit::Digital:  r.brusNiva = 0.8f; r.brusTyp = 2; r.brusHz = 6000; r.brusFall = 0.8f; r.metallNiva = 0.3f; r.metallFall = 0.8f; r.bitar = 9; r.hall = 2; break;
            case Kit::Fabrik:   r.metallNiva = 1.0f; r.metallSkala = 1.0f; r.metallBp = 5000; r.metallHp = 3000; r.metallFall = 1.2f;
                                r.modalNiva = 0.3f; r.modalHz = 600; r.modalFall = 1.0f; r.drive = 2.0f; r.niva = 0.8f; break;
        }
        return r;
    }

    inline Recept tom (Kit k, bool hog)
    {
        Recept r;
        r.tonNiva = 1.0f;
        r.niva = 0.6f;
        switch (k)
        {
            case Kit::Attaatta: r.tonHz = hog ? 150.0f : 90.0f; r.tonSvep = 0.6f; r.tonSvepTid = 0.05f; r.tonFall = hog ? 0.3f : 0.35f; r.klickNiva = 0.05f; break;
            case Kit::Niononio: r.tonHz = hog ? 160.0f : 100.0f; r.tonSvep = 1.0f; r.tonSvepTid = 0.02f; r.tonFall = 0.25f;
                                r.brusNiva = 0.1f; r.brusTyp = 0; r.brusHz = 2000; r.brusFall = 0.05f; break;
            case Kit::Digital:  r.tonHz = hog ? 170.0f : 110.0f; r.tonSvep = 0.5f; r.tonFall = 0.2f; r.bitar = 8; r.hall = 2; break;
            case Kit::Fabrik:   r.tonHz = hog ? 130.0f : 80.0f; r.tonSvep = 1.5f; r.tonFall = 0.3f; r.drive = 3.5f;
                                r.brusNiva = 0.15f; r.brusTyp = 1; r.brusHz = 1500; r.brusFall = 0.08f; r.niva = 0.5f; break;
        }
        return r;
    }

    inline Recept metall (Kit k)
    {
        Recept r;
        r.modalNiva = 1.0f;
        r.niva = 0.6f;
        switch (k)
        {
            case Kit::Attaatta: r.modalHz = 420; r.modalFall = 0.6f; r.klickNiva = 0.3f; break;
            case Kit::Niononio: r.modalHz = 520; r.modalFall = 0.5f; r.brusNiva = 0.1f; r.brusTyp = 2; r.brusHz = 5000; r.brusFall = 0.02f; r.klickNiva = 0.3f; break;
            case Kit::Digital:  r.modalHz = 600; r.modalFall = 0.35f; r.bitar = 8; r.klickNiva = 0.2f; break;
            case Kit::Fabrik:   r.modalHz = 300; r.modalFall = 1.0f; r.drive = 2.5f; r.brusNiva = 0.2f; r.brusTyp = 1; r.brusHz = 2000; r.brusFall = 0.05f; r.niva = 0.5f; break;
        }
        return r;
    }

    // Smällen: en fabriksport som slår igen – brus genom ett kamfilter och en dov stöt.
    inline Recept small (Kit k)
    {
        Recept r;
        r.brusNiva = 1.0f; r.brusTyp = 0; r.brusHz = 3000; r.brusFall = 0.3f;
        r.kamMs = 9.0f; r.kamAter = 0.75f;
        r.tonHz = 60; r.tonNiva = 0.6f; r.tonSvep = 1.5f; r.tonFall = 0.15f;
        r.drive = 3.0f; r.niva = 0.7f;
        switch (k)
        {
            case Kit::Attaatta: r.brusHz = 2000; r.kamMs = 11.0f; break;
            case Kit::Niononio: r.brusHz = 4000; r.kamMs = 8.0f; r.kamAter = 0.7f; break;
            case Kit::Digital:  r.bitar = 7; r.hall = 3; r.brusFall = 0.2f; break;
            case Kit::Fabrik:   r.kamMs = 13.0f; r.kamAter = 0.85f; r.brusFall = 0.45f; r.drive = 5.0f; r.niva = 0.6f; break;
        }
        return r;
    }

    inline Recept kubjallra (Kit k)
    {
        Recept r;
        r.tonHz = 540; r.tonNiva = 0.6f; r.ton2Kvot = 1.48f; r.ton2Niva = 0.6f; r.tonFyrkant = true; r.tonBp = 2600; r.tonFall = 0.25f; r.niva = 0.45f;
        switch (k)
        {
            case Kit::Attaatta: break;
            case Kit::Niononio: r.tonFall = 0.18f; r.drive = 1.5f; break;
            case Kit::Digital:  r.tonFall = 0.15f; r.bitar = 8; r.hall = 2; break;
            case Kit::Fabrik:   r.tonHz = 380; r.tonBp = 1800; r.tonFall = 0.4f; r.drive = 3.0f; r.niva = 0.4f; break;
        }
        return r;
    }

    inline Recept kantslag (Kit k)
    {
        Recept r;
        r.tonHz = 1700; r.tonNiva = 0.4f; r.tonFall = 0.015f; r.ton2Kvot = 0.3f; r.ton2Niva = 0.5f;
        r.brusNiva = 0.4f; r.brusTyp = 2; r.brusHz = 3000; r.brusFall = 0.01f; r.klickNiva = 0.5f; r.drive = 2.0f;
        if (k == Kit::Digital) { r.bitar = 8; r.hall = 2; }
        if (k == Kit::Fabrik)  { r.kamMs = 1.5f; r.kamAter = 0.6f; r.drive = 3.5f; }
        return r;
    }

    inline Recept for_ (Trumma t, Kit k, bool kantslagIStallet)
    {
        switch (t)
        {
            case Trumma::Bastrumma:   return bastrumma (k);
            case Trumma::Virvel:      return virvel (k);
            case Trumma::Virvel2:     return virvel2 (k);
            case Trumma::Handklapp:   return handklapp (k);
            case Trumma::StangdHihat: return hihat (k, false);
            case Trumma::OppenHihat:  return hihat (k, true);
            case Trumma::Cymbal:      return cymbal (k);
            case Trumma::TomLag:      return tom (k, false);
            case Trumma::TomHog:      return tom (k, true);
            case Trumma::Metall:      return metall (k);
            case Trumma::Small:       return small (k);
            case Trumma::Kubjallra:   return kantslagIStallet ? kantslag (k) : kubjallra (k);
        }
        return {};
    }
} // namespace recept

// En trumröst som spelar ett recept.
class TrumRost
{
public:
    void prepare (double s, uint32_t seed)
    {
        sr = s;
        slump = Slump (seed);
        kam.assign ((size_t) (sr * 0.03) + 2, 0.0f);
        reset();
    }

    void reset()
    {
        aktiv = false;
        brusF.reset(); tonF.reset(); metBp.reset(); metHp.reset();
        std::fill (kam.begin(), kam.end(), 0.0f);
    }

    // Receptet läses vid anslaget, så att kit- och klangbyten inte klipper ett ljud som klingar.
    void trigga (const Recept& r, const TrumParams& p, float vel, float stamningOffset = 0.0f, float tidFaktor = 1.0f)
    {
        rec = r;
        skala = std::exp2 ((p.stamning + stamningOffset) / 12.0);
        klangFaktor = (float) std::exp2 ((p.klang - 0.5f) * 3.0f) * tidFaktor;   // ×0,35 .. ×2,8
        amp = vel;
        t = 0;
        ph1 = ph2 = 0.0;
        for (auto& ph : metallFas) ph = slump.unipolar();
        for (auto& ph : modalFas) ph = 0.0;
        dampning = 1.0f;
        dampar = false;
        hallRaknare = 0;
        kamPos = 0;
        kamLangd = rec.kamMs > 0.0f ? klampi ((int) (rec.kamMs * 0.001 * sr / skala), 1, (int) kam.size() - 1) : 0;
        std::fill (kam.begin(), kam.end(), 0.0f);

        const float langsta = std::max ({ rec.tonFall, rec.brusFall + rec.klappSlag * rec.klappAvstand,
                                          rec.metallFall, rec.modalFall, 0.01f }) * klangFaktor;
        slutSek = langsta * 9.0 + (rec.kamAter > 0.0f ? rec.kamMs * 0.001 * 40.0 : 0.0) + 0.01;

        brusF.set (rec.brusHz * skala, rec.brusQ, sr);
        if (rec.tonBp > 0.0f) tonF.set (rec.tonBp * skala, 3.0, sr);
        metBp.set (rec.metallBp * std::sqrt (skala), 1.2, sr);
        metHp.set (rec.metallHp * std::sqrt (skala), 0.7, sr);
        aktiv = true;
    }

    // Tystar ljudet på 5 ms (stängd hi-hat tystar den öppna).
    void dampa() { if (aktiv) dampar = true; }

    float tick (float niva)
    {
        if (! aktiv) return 0.0f;
        const double sek = (double) t++ / sr;
        if (sek > slutSek) { aktiv = false; return 0.0f; }
        const auto fall = [&] (float tid) { return (float) std::exp (-sek / std::max (1.0e-4, (double) tid * klangFaktor * 0.45)); };

        float x = 0.0f;

        // --- Ton
        if (rec.tonNiva > 0.0f)
        {
            const double f = rec.tonHz * skala * (1.0 + rec.tonSvep * std::exp (-sek / rec.tonSvepTid));
            ph1 += f / sr; ph1 -= std::floor (ph1);
            float ton = rec.tonFyrkant ? (ph1 < 0.5 ? 1.0f : -1.0f) : (float) std::sin (twoPi * ph1);
            const float envTon = fall (rec.tonFall);
            float s = ton * rec.tonNiva * envTon;
            if (rec.ton2Niva > 0.0f)
            {
                ph2 += f * rec.ton2Kvot / sr; ph2 -= std::floor (ph2);
                const float t2 = rec.tonFyrkant ? (ph2 < 0.5 ? 1.0f : -1.0f) : (float) std::sin (twoPi * ph2);
                s += t2 * rec.ton2Niva * fall (rec.tonFall * 0.6f);
            }
            if (rec.tonBp > 0.0f) { tonF.tick (s); s = tonF.bp * 2.0f; }
            x += s;
        }

        // --- Brus (med handklappens slag och kamfiltret)
        if (rec.brusNiva > 0.0f)
        {
            float env;
            const double klappSlut = rec.klappSlag * rec.klappAvstand;
            if (rec.klappSlag > 0 && sek < klappSlut)
                env = (float) std::exp (-std::fmod (sek, (double) rec.klappAvstand) / 0.0035);
            else
                env = (float) std::exp (-(sek - (rec.klappSlag > 0 ? klappSlut : 0.0)) / std::max (1.0e-4, (double) rec.brusFall * klangFaktor * 0.45));
            brusF.tick (slump.bipolar());
            float b = rec.brusTyp == 0 ? brusF.lp : (rec.brusTyp == 1 ? brusF.bp * 1.6f : brusF.hp);
            b *= env;
            if (kamLangd > 0)
            {
                const float ut = b + rec.kamAter * kam[(size_t) kamPos];
                kam[(size_t) kamPos] = ut;
                kamPos = (kamPos + 1) % kamLangd;
                b = ut * (1.0f - rec.kamAter * 0.5f);
            }
            x += b * rec.brusNiva;
        }

        // --- Metall
        if (rec.metallNiva > 0.0f)
        {
            static constexpr std::array<double, 6> frekvenser { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
            float m = 0.0f;
            for (size_t i = 0; i < 6; ++i)
            {
                metallFas[i] += frekvenser[i] * rec.metallSkala * skala / sr;
                metallFas[i] -= std::floor (metallFas[i]);
                m += metallFas[i] < 0.5 ? 1.0f : -1.0f;
            }
            metBp.tick (m / 6.0f);
            metHp.tick (metBp.bp);
            x += metHp.hp * 2.2f * rec.metallNiva * fall (rec.metallFall);
        }

        // --- Modala delton
        if (rec.modalNiva > 0.0f)
        {
            static constexpr std::array<double, 5> kvoter { 1.0, 2.76, 5.40, 8.93, 13.34 };
            static constexpr std::array<float, 5> styrka { 1.0f, 0.7f, 0.5f, 0.35f, 0.25f };
            float m = 0.0f;
            for (size_t i = 0; i < 5; ++i)
            {
                const double f = rec.modalHz * skala * kvoter[i];
                if (f > sr * 0.45) continue;
                modalFas[i] += f / sr; modalFas[i] -= std::floor (modalFas[i]);
                m += (float) std::sin (twoPi * modalFas[i]) * styrka[i] * fall (rec.modalFall / (1.0f + (float) i * 0.8f));
            }
            x += m * rec.modalNiva * 0.5f;
        }

        // --- Klick
        if (rec.klickNiva > 0.0f && sek < 0.003)
            x += slump.bipolar() * rec.klickNiva * (float) (1.0 - sek / 0.003);

        // --- Färg
        if (rec.drive > 1.0f) x = mjukKlipp (x * rec.drive) / std::min (rec.drive, 1.6f);
        else x = mjukKlipp (x * 1.3f);
        if (rec.bitar > 0.0f)
        {
            if (hallRaknare++ % std::max (1, rec.hall) == 0)
            {
                const float steg = std::exp2 (rec.bitar - 1.0f);
                hallVarde = std::round (x * steg) / steg;
            }
            x = hallVarde;
        }

        if (dampar)
        {
            dampning *= dampKoef;
            if (dampning < 1.0e-4f) { aktiv = false; return 0.0f; }
        }
        return x * rec.niva * amp * niva * dampning;
    }

    bool arAktiv() const { return aktiv; }

private:
    static int klampi (int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    double sr = 48000.0;
    Recept rec;
    double skala = 1.0, slutSek = 0.0;
    float klangFaktor = 1.0f, amp = 1.0f;
    int64_t t = 0;
    bool aktiv = false, dampar = false;
    float dampning = 1.0f, dampKoef = 0.998f;
    double ph1 = 0.0, ph2 = 0.0;
    std::array<double, 6> metallFas {};
    std::array<double, 5> modalFas {};
    Svf brusF, tonF, metBp, metHp;
    std::vector<float> kam;
    int kamPos = 0, kamLangd = 0;
    int hallRaknare = 0;
    float hallVarde = 0.0f;
    Slump slump;

public:
    void setDampKoef (double s) { dampKoef = (float) std::exp (-1.0 / (0.005 * s)); }
};

class Stampverket
{
public:
    void prepare (double sr)
    {
        for (int i = 0; i < antalSpar; ++i)
        {
            roster[(size_t) i].prepare (sr, 0x53544d50u + (uint32_t) i * 7919u);
            roster[(size_t) i].setDampKoef (sr);
        }
    }

    void reset() { for (auto& r : roster) r.reset(); }

    void setParams (const StampverketParams& p) { params = p; }

    // Slår an ett spår (velocity 0..1).
    void slag (Trumma t, float velocity)
    {
        const auto i = (size_t) t;
        if (t == Trumma::StangdHihat) roster[(size_t) Trumma::OppenHihat].dampa();
        roster[i].trigga (recept::for_ (t, params.kit, params.kantslag), params.trummor[i], velocity,
                          params.stamningOffset, params.tidFaktor);
    }

    float tick()
    {
        float x = 0.0f;
        for (size_t i = 0; i < (size_t) antalSpar; ++i)
        {
            const float v = roster[i].tick (params.trummor[i].niva);
            if (i == 0) bastrummaNu = v;
            x += v;
        }
        return x;
    }

    // Bastrummans senaste sample, för Klämmans sidokedja.
    float bastrumma() const { return bastrummaNu; }

    // MIDI-noter för spåren (GM-trumkartan där det går), i spårens ordning.
    static int notForSpar (int spar)
    {
        static constexpr std::array<int, antalSpar> noter { 36, 38, 40, 39, 42, 46, 49, 41, 48, 56, 54, 37 };
        return noter[(size_t) std::clamp (spar, 0, antalSpar - 1)];
    }
    static int sparForNot (int note)
    {
        for (int i = 0; i < antalSpar; ++i)
            if (notForSpar (i) == note) return i;
        return -1;
    }

private:
    StampverketParams params;
    std::array<TrumRost, antalSpar> roster;
    float bastrummaNu = 0.0f;
};

} // namespace km
