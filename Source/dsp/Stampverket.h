// Stampverket – trummorna: tolv syntetiska ljud i fyra kit.
//
// Varje ljud byggs av samma delar – en ton med tonhöjdssvep, brus genom ett filter, metall (sex fyrkanter),
// handklappens upprepade brusslag, modala delton (städ och plåt), ett kamfilter (fabrikshallen), ett klick,
// mättnad och bitkross. Ett *recept* säger hur mycket av varje del ett ljud har i ett kit, så att de fyra
// kiten blir en tabell i stället för 48 klasser.
//
// Kiten: Åttaåtta (mjuk, lång, 808-liknande), Niononio (hårdare, mer brus, 909-liknande), Digital (kort och
// torr med 8-bitarskorn, som de tidiga samplade trummaskinerna), Fabrik (industri: kam, mättnad, metall) och
// Gjuteriet (fas 5: fysiskt modellerad metall – oljefat, plåt, kedja, städ, rör, fjäder och klocka).
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

enum class Kit { Attaatta = 0, Niononio, Digital, Fabrik, Gjuteriet };
inline constexpr int antalKit = 5;   // kitens ordning är fast (parametern sparas som index)

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

// Gjuteriets namn på spåren (fysiskt modellerad metall, fas 5). Spåren och mönstren är desamma.
inline const char* gjuterietsNamn (Trumma t, bool kantslag = false)
{
    switch (t)
    {
        case Trumma::Bastrumma:   return "Oljefat";
        case Trumma::Virvel:      return "Plåt";
        case Trumma::Virvel2:     return "Kedja";
        case Trumma::Handklapp:   return "Hammare";
        case Trumma::StangdHihat: return "Bult";
        case Trumma::OppenHihat:  return "Ringande rör";
        case Trumma::Cymbal:      return "Åskplåt";
        case Trumma::TomLag:      return "Rör lågt";
        case Trumma::TomHog:      return "Rör högt";
        case Trumma::Metall:      return "Städ";
        case Trumma::Small:       return "Fjäder";
        case Trumma::Kubjallra:   return kantslag ? "Spik" : "Klocka";
    }
    return "";
}
inline const char* trumNamnFor (Trumma t, Kit k, bool kantslag = false)
{
    return k == Kit::Gjuteriet ? gjuterietsNamn (t, kantslag) : (t == Trumma::Kubjallra && kantslag ? "Kantslag" : trumNamn (t));
}

// Fysisk modellering: kroppar som klingar i sina egna svängningsmoder.
enum class Kropp { Ingen = 0, Stav, Plat, Fat, Stad, Ror, Klocka, Kedja, Fjader };

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
    // Fysisk modellering (Gjuteriet): en kropp som slås an av en klubba och klingar i sina moder.
    Kropp kropp = Kropp::Ingen;
    float kroppNiva = 0, kroppHz = 200;  // lägsta modens frekvens
    float kroppFall = 1.0f;              // T60 för lägsta moden, sekunder
    float dampning = 1.0f;               // hur mycket snabbare de höga moderna dör (0 = lika)
    float hardhet = 0.5f;                // 0 mjuk klubba (filt) .. 1 stålhammare
    float slagPlats = 0.3f;              // 0..0,5: var kroppen slås (mitten tystar jämna moder)
    int kroppSlag = 1;                   // antal anslag (kedjan rasslar)
    float kroppSlagTid = 0.0f;           // anslagen sprids över så här lång tid
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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
            case Kit::Gjuteriet: break;   // Gjuteriet har egna recept (gjuteriet())
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

    // Gjuteriet: metall och industri, fysiskt modellerat.
    inline Recept gjuteriet (Trumma t, bool kantslag)
    {
        Recept r;
        r.drive = 1.4f;
        switch (t)
        {
            case Trumma::Bastrumma:   // ett oljefat som sparkas: botten (membran) och ett tungt tonsvep
                r.kropp = Kropp::Fat; r.kroppNiva = 0.8f; r.kroppHz = 96; r.kroppFall = 0.55f; r.dampning = 1.2f; r.hardhet = 0.55f; r.slagPlats = 0.15f;
                r.tonHz = 48; r.tonNiva = 0.9f; r.tonSvep = 3.0f; r.tonSvepTid = 0.02f; r.tonFall = 0.3f; r.klickNiva = 0.25f; r.drive = 2.4f; r.niva = 1.49f; break;
            case Trumma::Virvel:      // tunn plåt med rassel
                r.kropp = Kropp::Plat; r.kroppNiva = 1.0f; r.kroppHz = 210; r.kroppFall = 0.35f; r.dampning = 0.8f; r.hardhet = 0.75f; r.slagPlats = 0.27f;
                r.brusNiva = 0.45f; r.brusTyp = 1; r.brusHz = 4200; r.brusQ = 0.8f; r.brusFall = 0.12f; r.drive = 2.2f; r.niva = 1.52f; break;
            case Trumma::Virvel2:     // en kedja som släpps på en plåt
                r.kropp = Kropp::Kedja; r.kroppNiva = 1.0f; r.kroppHz = 1400; r.kroppFall = 0.12f; r.dampning = 0.6f; r.hardhet = 0.9f;
                r.kroppSlag = 18; r.kroppSlagTid = 0.24f; r.drive = 1.8f; r.niva = 2.0f; break;
            case Trumma::Handklapp:   // hammare mot städet, kort
                r.kropp = Kropp::Stad; r.kroppNiva = 1.0f; r.kroppHz = 820; r.kroppFall = 0.18f; r.dampning = 1.2f; r.hardhet = 1.0f; r.slagPlats = 0.21f;
                r.klickNiva = 0.6f; r.drive = 2.0f; r.niva = 2.10f; break;
            case Trumma::StangdHihat: // liten bult, hårt och kort
                r.kropp = Kropp::Stav; r.kroppNiva = 1.0f; r.kroppHz = 2300; r.kroppFall = 0.05f; r.dampning = 0.5f; r.hardhet = 1.0f; r.slagPlats = 0.11f;
                r.brusNiva = 0.25f; r.brusTyp = 2; r.brusHz = 8000; r.brusFall = 0.015f; r.niva = 1.45f; break;
            case Trumma::OppenHihat:  // ett rör som får ringa (tystas av bulten)
                r.kropp = Kropp::Ror; r.kroppNiva = 1.0f; r.kroppHz = 1650; r.kroppFall = 0.9f; r.dampning = 0.4f; r.hardhet = 0.95f; r.slagPlats = 0.08f;
                r.brusNiva = 0.15f; r.brusTyp = 2; r.brusHz = 7000; r.brusFall = 0.08f; r.niva = 1.17f; break;
            case Trumma::Cymbal:      // åskplåt: stor, tunn, lång
                r.kropp = Kropp::Plat; r.kroppNiva = 1.0f; r.kroppHz = 95; r.kroppFall = 2.4f; r.dampning = 0.25f; r.hardhet = 0.55f; r.slagPlats = 0.19f;
                r.brusNiva = 0.2f; r.brusTyp = 1; r.brusHz = 2500; r.brusFall = 0.6f; r.drive = 1.8f; r.niva = 0.6f; break;
            case Trumma::TomLag:
                r.kropp = Kropp::Ror; r.kroppNiva = 1.0f; r.kroppHz = 180; r.kroppFall = 0.9f; r.dampning = 1.0f; r.hardhet = 0.5f; r.slagPlats = 0.22f;
                r.klickNiva = 0.2f; r.drive = 1.8f; r.niva = 1.44f; break;
            case Trumma::TomHog:
                r.kropp = Kropp::Ror; r.kroppNiva = 1.0f; r.kroppHz = 300; r.kroppFall = 0.75f; r.dampning = 1.0f; r.hardhet = 0.55f; r.slagPlats = 0.22f;
                r.klickNiva = 0.2f; r.drive = 1.8f; r.niva = 1.65f; break;
            case Trumma::Metall:      // städet: höga moder som klingar länge
                r.kropp = Kropp::Stad; r.kroppNiva = 1.0f; r.kroppHz = 640; r.kroppFall = 1.6f; r.dampning = 0.15f; r.hardhet = 1.0f; r.slagPlats = 0.17f;
                r.klickNiva = 0.4f; r.drive = 1.6f; r.niva = 0.99f; break;
            case Trumma::Small:       // en spiralfjäder som slås an: dispersion
                r.kropp = Kropp::Fjader; r.kroppNiva = 1.0f; r.kroppHz = 30; r.kroppFall = 1.2f; r.hardhet = 0.8f;
                r.klickNiva = 0.3f; r.drive = 2.0f; r.niva = 2.40f; break;
            case Trumma::Kubjallra:
                if (kantslag)         // en spik: tunn stav, torr
                {
                    r.kropp = Kropp::Stav; r.kroppNiva = 1.0f; r.kroppHz = 3100; r.kroppFall = 0.04f; r.dampning = 0.3f; r.hardhet = 1.0f; r.slagPlats = 0.05f;
                    r.klickNiva = 0.5f; r.niva = 0.9f;
                }
                else                  // en klocka
                {
                    r.kropp = Kropp::Klocka; r.kroppNiva = 1.0f; r.kroppHz = 520; r.kroppFall = 1.8f; r.dampning = 0.35f; r.hardhet = 0.8f; r.slagPlats = 0.2f;
                    r.niva = 0.80f;
                }
                break;
        }
        return r;
    }

    inline Recept for_ (Trumma t, Kit k, bool kantslagIStallet)
    {
        if (k == Kit::Gjuteriet) return gjuteriet (t, kantslagIStallet);
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

// Kropparnas svängningsmoder: frekvenskvoter mot den lägsta moden och hur starkt varje mod svarar.
struct KroppModer
{
    static constexpr int max = 16;
    int antal = 0;
    std::array<double, max> kvot {};
    std::array<float, max> vikt {};
    bool par = false;   // varje mod finns två gånger, en aning isär: svävning (rör och klockor)
};

inline const KroppModer& moderFor (Kropp k)
{
    static const auto tabeller = []
    {
        std::array<KroppModer, 9> t {};
        auto satt = [&t] (Kropp kropp, std::initializer_list<double> kv, std::initializer_list<float> vi, bool par = false)
        {
            auto& m = t[(size_t) kropp];
            m.antal = 0;
            auto v = vi.begin();
            for (double q : kv) { if (m.antal >= KroppModer::max) break; m.kvot[(size_t) m.antal] = q; m.vikt[(size_t) m.antal] = v != vi.end() ? *v++ : 0.3f; ++m.antal; }
            m.par = par;
        };
        // Fri-fri stav (balk): kvoterna ur Euler–Bernoullis ekvation.
        satt (Kropp::Stav, { 1.0, 2.756, 5.404, 8.933, 13.344, 18.638, 24.814, 31.872 }, { 1.0f, 0.8f, 0.6f, 0.45f, 0.35f, 0.28f, 0.2f, 0.15f });
        // Rör: som staven, med svävande par.
        satt (Kropp::Ror, { 1.0, 2.756, 5.404, 8.933, 13.344, 18.638, 24.814, 31.872 }, { 0.8f, 1.0f, 0.7f, 0.5f, 0.35f, 0.25f, 0.18f, 0.12f }, true);
        // Cirkulärt membran (fatets botten): nollställena i Besselfunktionerna.
        satt (Kropp::Fat, { 1.0, 1.594, 2.136, 2.296, 2.653, 2.918, 3.156, 3.501, 3.600, 3.652, 4.060, 4.154, 4.601, 4.832, 4.903, 5.131 },
              { 1.0f, 0.7f, 0.55f, 0.5f, 0.42f, 0.38f, 0.32f, 0.3f, 0.28f, 0.26f, 0.22f, 0.2f, 0.18f, 0.16f, 0.15f, 0.14f });
        // Städet: ett tjockt block, täta och oharmoniska moder.
        satt (Kropp::Stad, { 1.0, 1.47, 2.09, 2.56, 3.39, 4.12, 4.95, 5.97, 6.81, 7.88, 9.31, 10.6 },
              { 0.7f, 1.0f, 0.85f, 0.6f, 0.75f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.25f, 0.2f });
        // Kyrkklocka: hum, prim, ters, kvint, nominal och de höga deltonerna.
        satt (Kropp::Klocka, { 0.5, 1.0, 1.183, 1.506, 2.0, 2.514, 2.662, 3.011, 4.166, 5.433, 6.796 },
              { 0.6f, 0.8f, 0.7f, 0.4f, 1.0f, 0.5f, 0.45f, 0.35f, 0.3f, 0.2f, 0.15f }, true);
        // Rektangulär plåt (fritt upplagd, sidförhållande 1,37): f ∝ m² + (1,37 n)².
        {
            std::vector<double> f;
            for (int m = 1; m <= 6; ++m) for (int n = 1; n <= 6; ++n) f.push_back (m * m + 1.8769 * n * n);
            std::sort (f.begin(), f.end());
            auto& p = t[(size_t) Kropp::Plat];
            p.antal = KroppModer::max;
            for (int i = 0; i < p.antal; ++i) { p.kvot[(size_t) i] = f[(size_t) i] / f[0]; p.vikt[(size_t) i] = 1.0f / (1.0f + 0.12f * (float) i); }
            t[(size_t) Kropp::Kedja] = p;   // kedjans länkar: små plåtbitar
        }
        return t;
    }();
    return tabeller[(size_t) k];
}

// En trumröst som spelar ett recept.
class TrumRost
{
public:
    void prepare (double s, uint32_t seed)
    {
        sr = s;
        slump = Slump (seed);
        kam.assign ((size_t) (sr * 0.03) + 2, 0.0f);
        fjader.assign ((size_t) (sr * 0.06) + 2, 0.0f);
        reset();
    }

    void reset()
    {
        aktiv = false;
        brusF.reset(); tonF.reset(); metBp.reset(); metHp.reset();
        std::fill (kam.begin(), kam.end(), 0.0f);
        std::fill (fjader.begin(), fjader.end(), 0.0f);
        for (auto& m : mod) m = {};
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

        if (rec.kropp != Kropp::Ingen) startaKroppen (vel);

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

        // --- Kroppen (fysisk modellering)
        if (rec.kropp != Kropp::Ingen) x += kroppTick() * rec.kroppNiva;

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

    // --- Fysisk modellering
    // Varje mod är en dämpad resonator (tvåpolig, impulssvaret r^n·sin(ω(n+1))). Klubban är en halv
    // sinus vars längd ger hårdheten: en mjuk klubba (4 ms) når bara de låga moderna, en stålhammare
    // (0,25 ms) alla. Slagplatsen väger moderna som en sträng (mitten tystar de jämna). Fjädern är en
    // fördröjning med en kedja allpassfilter i återkopplingen: de höga frekvenserna går fortare än de
    // låga, och det ger fjäderns "boing".
    struct Mod { float a1 = 0, a2 = 0, b = 0, y1 = 0, y2 = 0, vikt = 0; };
    static constexpr int maxModer = 2 * KroppModer::max;
    static constexpr int fjaderSteg = 24;

    void startaKroppen (float vel)
    {
        const auto& kr = moderFor (rec.kropp);
        antalModer = 0;
        const double f0 = rec.kroppHz * skala;
        const double t60 = std::max (0.01, (double) rec.kroppFall * klangFaktor);
        for (int i = 0; i < kr.antal; ++i)
            for (int p = 0; p < (kr.par ? 2 : 1); ++p)
            {
                const double f = f0 * kr.kvot[(size_t) i] * (p == 0 ? 1.0 : 1.0023);
                if (f > sr * 0.45 || antalModer >= maxModer) continue;
                const double w = twoPi * f / sr;
                const double tid = t60 / (1.0 + rec.dampning * (kr.kvot[(size_t) i] - 1.0));
                const double r = std::exp (-6.9078 / (tid * sr));
                const float plats = (float) std::abs (std::sin ((0.5 * twoPi) * (i + 1) * (0.5 + rec.slagPlats)));
                auto& m = mod[(size_t) antalModer++];
                m.a1 = (float) (2.0 * r * std::cos (w));
                m.a2 = (float) (r * r);
                m.b = (float) std::sin (w);
                m.y1 = m.y2 = 0.0f;
                m.vikt = kr.vikt[(size_t) i] * (0.35f + 0.65f * plats) * (kr.par ? 0.6f : 1.0f);
            }
        // Klubban.
        const double tc = 0.004 * std::pow (0.0625, (double) rec.hardhet);
        klubbLangd = std::max (1, (int) (tc * sr));
        klubbPos = 0;
        // Anslagen: ett, eller kedjans många.
        antalAnslag = std::clamp (rec.kroppSlag, 1, (int) anslagVid.size());
        for (int i = 0; i < antalAnslag; ++i)
        {
            const float u = i == 0 ? 0.0f : slump.unipolar();
            anslagVid[(size_t) i] = (int) (u * std::sqrt (u) * rec.kroppSlagTid * (float) sr);   // tätast i början
            anslagStyrka[(size_t) i] = i == 0 ? 1.0f : 0.25f + 0.6f * slump.unipolar() * (1.0f - u);
        }
        std::sort (anslagVid.begin() + 1, anslagVid.begin() + antalAnslag);
        nastaAnslag = 0;
        kroppSampel = 0;
        anslagNu = 0.0f;
        // Fjädern.
        if (rec.kropp == Kropp::Fjader)
        {
            fjaderLangd = klampi ((int) (sr / std::max (5.0, f0)), 8, (int) fjader.size() - 1);
            fjaderAter = (float) std::exp (-6.9078 * fjaderLangd / (t60 * sr));
            fjaderPos = 0;
            std::fill (fjader.begin(), fjader.end(), 0.0f);
            for (auto& a : fjaderAp) a = {};
            fjaderLp = 0.0f;
        }
        kroppSlut = t60 * 1.2 + rec.kroppSlagTid + 0.05;
        slutSek = std::max (slutSek, kroppSlut);
        (void) vel;
    }

    float kroppTick()
    {
        // Klubbans kraft just nu (summan av alla anslag som pågår).
        while (nastaAnslag < antalAnslag && anslagVid[(size_t) nastaAnslag] <= kroppSampel)
        {
            anslagNu = anslagStyrka[(size_t) nastaAnslag];
            klubbPos = 0;
            if (rec.kropp == Kropp::Kedja)   // varje länk låter lite olika
                for (int i = 0; i < antalModer; ++i) mod[(size_t) i].vikt = std::abs (mod[(size_t) i].vikt) * (slump.unipolar() < 0.5f ? -1.0f : 1.0f) * (0.6f + 0.8f * slump.unipolar());
            ++nastaAnslag;
        }
        float kraft = 0.0f;
        if (klubbPos < klubbLangd)
        {
            kraft = anslagNu * (float) std::sin ((0.5 * twoPi) * (klubbPos + 0.5) / klubbLangd) * (1.5708f / (float) klubbLangd);
            ++klubbPos;
        }
        ++kroppSampel;

        if (rec.kropp == Kropp::Fjader)
        {
            // Fördröjning → allpasskedja → lågpass → tillbaka.
            float v = kraft * 40.0f + fjaderAter * fjader[(size_t) fjaderPos];
            for (auto& a : fjaderAp)
            {
                const float y = -0.62f * v + a.x1 + 0.62f * a.y1;
                a.x1 = v; a.y1 = y; v = y;
            }
            fjaderLp += 0.5f * (v - fjaderLp);
            fjader[(size_t) fjaderPos] = fjaderLp;
            fjaderPos = (fjaderPos + 1) % fjaderLangd;
            return fjaderLp * 0.5f;
        }

        float y = 0.0f;
        for (int i = 0; i < antalModer; ++i)
        {
            auto& m = mod[(size_t) i];
            const float u = m.a1 * m.y1 - m.a2 * m.y2 + m.b * kraft;
            m.y2 = m.y1; m.y1 = u;
            y += u * m.vikt;
        }
        return y * 0.45f;
    }

    std::array<Mod, maxModer> mod {};
    int antalModer = 0, klubbLangd = 1, klubbPos = 0, antalAnslag = 1, nastaAnslag = 0;
    int64_t kroppSampel = 0;
    float anslagNu = 0.0f;
    double kroppSlut = 0.0;
    std::array<int, 24> anslagVid {};
    std::array<float, 24> anslagStyrka {};
    struct Allpass { float x1 = 0, y1 = 0; };
    std::array<Allpass, fjaderSteg> fjaderAp {};
    std::vector<float> fjader;
    int fjaderPos = 0, fjaderLangd = 8;
    float fjaderAter = 0.0f, fjaderLp = 0.0f;

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
