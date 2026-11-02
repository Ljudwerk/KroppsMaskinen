// Panelens gemensamma tillstånd: vald kanal, vy, trumma, mönstersida, variant att redigera och
// stegskrivningen. Delas av panelen, stegraden, sekvensvyn och klaviaturen.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <functional>
#include "../../PluginProcessor.h"
#include "SkarmData.h"

namespace km::vhs
{

struct PanelLage
{
    explicit PanelLage (KroppsmaskinenProcessor& p) : proc (p) {}

    KroppsmaskinenProcessor& proc;
    int  kanal = 0;            // 0..3
    Vy   vy = Vy::Kanal;
    int  valdTrumma = 0;       // Stampverkets spår för stegraden
    int  sida = 0;             // mönstersida 0..3 (I–IV)
    double sidaValdVid = -1e9; // när sidan valdes för hand (följer spelhuvudet igen efter en stund)
    int  redigera = 0;         // variant som stegraden redigerar: 0 = A, 1 = B
    bool spelaIn = false;      // stegskrivning (SPELA IN)
    int  markor = 0;           // stegskrivningens position 0..63

    std::function<void()> andrat;   // anropas när något ovan ändras från en komponent
    void meddela() { if (andrat) andrat(); }

    // Parametrar och mönster.
    float param (const juce::String& id) const { return proc.state.getRawParameterValue (id)->load(); }
    int  monsterFor (int k) const { return (int) param (params::id::kanal (k, "monster")) - 1; }
    int  langdFor (int k) const { return juce::jlimit (1, maxSteg, (int) param (params::id::kanal (k, "langd"))); }
    bool foljer() const { return kanal == 1 && param (params::id::folj) > 0.5f; }
    bool tonKanal() const { return kanal != (int) Kanal::Stampverket; }

    TrumMonster& trum() { return proc.banker.trummor.get (monsterFor (2), redigera); }
    // Tonmönstret som stegraden visar: Kolvbas 2 som följer visar Kolvbasens.
    TonMonster& ton() { const int k = foljer() ? 0 : kanal; return proc.banker.ton (k).get (monsterFor (k), redigera); }

    // Steget som spelas i vald kanal, eller −1 om det inte är mönstret som visas.
    int spelsteg() const
    {
        const int s = proc.stegNu ((Kanal) kanal);
        const int m = proc.monsterNu ((Kanal) kanal);
        if (s < 0) return -1;
        if (m == -2) return s;                      // Kolvbas 2 följer
        return m == monsterFor (kanal) ? s : -1;
    }
};

} // namespace km::vhs
