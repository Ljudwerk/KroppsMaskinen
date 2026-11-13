# Kroppsmaskinen – plan

*Modell KM-87. En groovebox för Electronic Body Music och New Beat, byggd som om den kom ur en svart hemelektroniklåda från 1987 och bara gick att se på genom en VHS-skärm.*

**Status:** Fas 0–3 klara och grind 0–3 godkända. Fas 4, fabriksbandet (1.0), är byggd och väntar på grind 4 (inga externa testare än). Fas 5 pågår (beslut 3 november 2026): mikrotonala stämningar och Gjuteriet är klara, Dimman är godkänd (11 november 2026); MIDI ut från Taktverket är klart i kod och ska provas på Macen. Därmed är fas 5:s planerade kandidater byggda (krang-libs bara om kopiorna glider isär). `KMRender --test`: 178 kontroller, alla går igenom; `KMSnapshot --klicktest` går igenom.

Instrument-plugin (VST3, AU, CLAP) och fristående app för macOS, på samma stack som KrangBox. Allt ljud är syntetiskt. Allt i gränssnittet är på svenska.

## 1. Mål

**Kroppsmaskinen ska räcka till ett helt EBM- eller New Beat-spår på egen hand:** en sekvenserad bas som driver, trummor som slår hårt, stötar och leads ovanpå, och en effektkedja som låter som 1987. Inget mer.

Måttstock: spår av Nitzer Ebb, Front 242, Bigod 20, Pankow, Invincible Spirit, DAF och belgisk New Beat (A Split-Second, The Lords of Acid, Code 61). Vi kopierar inga ljud eller mönster från dem; vi lyssnar på dem för att veta när vi är framme.

Det som kännetecknar genren och som maskinen måste klara:

- **Sextondelsbasen:** monofon, sågtand eller puls, kort filterenvelope, accenter, oktavhopp och glid. Den är motorn i varje låt.
- **Trummor från drumboxar:** 808/909-, DMX- och Linn-liknande, gatad reverb på virveln, handklapp, metallslag och industriella smällar.
- **Stötar:** orkesterstötar, blås- och körstötar i moll, synkleads.
- **Tempo:** EBM ungefär 115–135 BPM, New Beat ungefär 100–118 BPM.
- **New Beat-tricket:** 45-varvsskivor spelade på 33 varv med pitchen uppskruvad. Tyngre, långsammare, mörkare.

### Inte i 1.0

KrangBox växte till femton kammare. Det gör inte Kroppsmaskinen. Det här skjuts upp tills 1.0 är klar och grind 4 passerad:

- Vokoder och röstmodul – finns i Röstverket RV-83, som ligger på rösten bredvid Kroppsmaskinen (beslut 6 november 2026)
- Pads, drönljud, fysisk modellering, mikrotonala stämningar (påbörjade i fas 5 före grind 4, se nedan)
- Sampling av egna filer
- Windows och Linux

## 2. Maskinen

Fyra kanaler, en mixer, en effektkedja och två sekvensrar. Namnen är arbetsnamn.

```
 ┌───────────── Taktverket (klocka, mönster, kedjor, Varvtalet) ─────────────┐
 │                                                                           │
 ▼                          ▼                      ▼                      ▼
Kolvbasen              Stampverket            Hugget                 Kolvbas 2
(sekvensbas)           (trummor, 12 ljud)     (stötar & leads)       (andra basrösten)
 │ Drivet                 │ Drivet               │ Drivet               │ Drivet
 └────────────┬───────────┴──────────┬───────────┴──────────────────────┘
              ▼                      ▼
        Mixern (nivå, panorering, mute/solo, sändningar A/B, sidokedja)
              │                 │ A: Grindrummet (gatad reverb)
              │                 │ B: Ekot (BBD/bandeko, ping-pong)
              ▼
        Masterkedjan: Svajet (chorus) → Klämman (kompressor) → Kassetten (VHS) → limiter
```

### Kolvbasen – sekvensbasen

Monofon bas i andan av SH-101, MC-202 och Pro-One.

- Två oscillatorer (sågtand, puls med PWM) + sub en eller två oktaver ned + brus. Hårdsynk osc 2 → osc 1.
- Filter: 24 dB ladder och 12 dB state-variable (för den tunnare 202-klangen), resonans som kan självsvänga.
- Filterenvelope med **egen accentväg** (accent ökar både nivå och filtersvep, som på 303/202).
- Glid per steg, legato, oktavhopp per steg.
- Gatelängd per steg – det är den som gör EBM-basen "studsande".
- Två kanaler: *Kolvbasen* och *Kolvbas 2* (samma motor, eget mönster, egen klang). Två låsta basar med en sextondels förskjutning, eller en ren botten under en distad bas, är ett klassiskt EBM-grepp.
- **Följ Kolvbasen** (knapp i Kolvbas 2): spelar Kolvbasens mönster, med valbar förskjutning (0–3 sextondelar) och transponering (−24 till +24 halvtoner). Snabbaste vägen till ett tungt lager.

Utgår från `Basmalen.h` i KrangBox, men filter och envelopes skrivs om för snabbare attack och accent.

### Stampverket – trummorna

Tolv syntetiska ljud, alla med stämning, klang (decay), nivå och "tyngd" (drive/ton).

| Spår | Ljud |
| --- | --- |
| 1 | Bastrumma (808-svep, 909-klick, DMX-dunk) |
| 2–3 | Virvel, virvel 2 (med eget gatad-reverb-sänd) |
| 4 | Handklapp |
| 5–6 | Stängd och öppen hi-hat (choke) |
| 7 | Cymbal |
| 8–9 | Tom låg och hög |
| 10 | Metall (stålplåt, städ – modalsyntes) |
| 11 | Smäll (brus + kam, "fabriksport") |
| 12 | Kubjällra/cowbell eller rimshot (väljbart) |

Fyra kit: **Åttaåtta**, **Niononio**, **Digital** (DMX/Linn-liknande, hårdare och torrare) och **Fabrik** (industri). Utgår från `Hammarverket.h`.

### Hugget – stötar och leads

Polyfon, åtta röster.

- **Stötlägen:** Orkesterstöt (lager av brus, sågtandskluster och sjunkande tonhöjd), Blåsstöt, Körstöt (formanter), Klockstöt.
- **Leadlägen:** Synklead, Pulslead, FM-lead (två operatorer).
- Ackordminne: spela en ton, få ett lagrat ackord (moll, kvint, oktav). Viktigt för enfingersstötar från sekvensern.

### Effekterna

- **Drivet** per kanal: rör, overdrive, bitkross (från `KrangDistortion.h`).
- **Grindrummet:** gatad reverb med tröskel, hålltid och fall. Virvelns signaturljud.
- **Ekot:** synkat eko i BBD- eller bandläge, ping-pong, mörknande upprepningar.
- **Svajet:** Juno-liknande chorus, läge I, II och I+II.
- **Klämman:** bussnkompressor med sidokedja från bastrumman (pump när man vill ha det).
- **Kassetten:** VHS-masterbandet – wow, flutter, huvudbyteslack, tappningar, bandbrus, högfrekvensförlust och "SP/LP/EP"-kvalitet. Utgår från `Bandmaskin.h` och `Brusgrind.h`.

### Taktverket – sekvensrarna

- **Trumsekvenser:** `Stegverket.h` rakt av (chans, ratchets, parameterlås). 12 spår i stället för 8.
- **Tonsekvenser:** `TonStegverket.h` för Kolvbasen, Kolvbas 2 och Hugget, utökat med accent, glid, oktav och gatelängd per steg, och olika längd per spår (polymeter).
- **Mönsterbanker:** 16 mönster per kanal, A/B-varianter, och **kedjor** för att bygga en låt.
- **Livespel:** mute/solo per spår, mönsterbyte på nästa takt, transponering av alla tonspår från keyboardet.
- Följer DAW:ens transport; egen klocka i den fristående appen.

### Varvtalet – 45 på 33

En global omkopplare för New Beat: **45**, **33** och **33 + 8 %**. Två lägen:

- **Musikaliskt:** tempo och tonhöjd sänks tillsammans, effekterna går i normal fart.
- **Skivspelare:** hela utgången spelas om i den nya hastigheten, inklusive reverb och eko (varispeed med översampling). Låter mest som originalen.

I DAW:en följer tempot värden; Varvtalet sänker då bara tonhöjden och "sträcker" envelopes, så att låtens tempo inte bråkar med projektet.

### Spänningen – makrospaken

Som Krangspaken: en spak som drar allt mot mer energi. Kortare och mer accentuerade basenvelopes, mer drive, mer gatad reverb, hårdare kompression, mer bandslitage. Nivån kompenseras.

### Modulation

`Modulation.h` från KrangBox: två LFO:er, två envelopes, modhjul, aftertouch, anslag, sex kopplingar. LFO:erna kan synkas till Taktverket.

## 3. Gränssnittet – VHS-panelen

### Idén

**En svart japansk hemelektroniklåda från 1987, kopplad till en tjock-TV.** Chassit är svart borstad aluminium och svart plast med röda och orange lysdioder, sjusegmentsdisplayer och silvertryck. Mitten är en bildskärm som visar den valda kanalen som vektorgrafik i trådram – med VHS-band ovanpå.

Tre visuella lager:

1. **Hårdvaran:** chassit, rattar med silverstreck, TR-liknande stegknappar med lysdioder, faders, VU-mätare med visare.
2. **Skärmen:** trådramsgrafik (rutnät mot horisont, roterande geometri, kanalernas motiv ritade som vektorlinjer), i rött, cyan och vitt på svart.
3. **Bandet:** sveplinjer, kromatisk förskjutning, spårningsbrus i bildens nederkant, vit OSD-text i VHS-typsnitt ("▶ SPELA", "SP", räkneverk 0:00:00), statiskt brus när man byter kanal.

EBM-tonen kommer av färgerna och typografin: svart, rött och vitt, stora smala versaler, konstruktivistiska diagonaler och pilar. Inga synthwave-solnedgångar i rosa.

### Layout (fast designstorlek, skalas 60–160 % som KrangBox)

```
┌─────────────────────────────────────────────────────────────────────────┐
│ KROPPSMASKINEN KM-87   [◀◀][▶][■][●]  0:12:04  SP  ▣ 33/45   SPÄNNINGEN │  ← Bandverket (transport)
├───────────────┬─────────────────────────────────────┬───────────────────┤
│ KANALVÄLJARE  │            BILDSKÄRMEN              │  PRESETS          │
│ ▸ KOLVBASEN   │   trådramsgrafik + OSD + VHS-brus   │  räkneverk-display│
│   KOLVBAS 2   │                                     │  VU-mätare L/R    │
│   STAMPVERKET │                                     │  MASTER           │
│   HUGGET      │                                     │                   │
├───────────────┴─────────────────────────────────────┴───────────────────┤
│ RATTPANELEN: den valda kanalens parametrar                              │
├─────────────────────────────────────────────────────────────────────────┤
│ [1][2][3][4][5][6][7][8][9][10][11][12][13][14][15][16] I II III IV A/B │  ← stegknappar
├─────────────────────────────────────────────────────────────────────────┤
│ KEYBOARD                                                                │
└─────────────────────────────────────────────────────────────────────────┘
```

Knappar över rattpanelen byter vy, som i KrangBox: **KANAL**, **SEKVENS**, **MIXER**, **EFFEKTER**, **MODULATION**.

### Teknik

- Allt ritas i kod, inga bildfiler (som `Textures.h`): borstad aluminium, plastnarv, silvertryck.
- **Eget OSD-typsnitt** som punktmatris i kod (5×7 och 8×12). Inga typsnittslicenser att hålla reda på.
- Skärmens VHS-lager ritas för sig och animeras i högst 30 fps; resten av panelen cachas och ritas bara om när något ändras. Mål: under 3 % CPU för gränssnittet på en M1 i 100 %.
- Ett **"Bandstyrka"-reglage** i inställningarna (av, lite, mycket) – VHS-effekten ska gå att stänga av för den som arbetar länge.
- `KMSnapshot` (från `KrangSnapshot`) ritar panelen och alla kanaler till PNG för granskning utan DAW.
- Värdeformatet från `ValueFormat.h`: procent, ms/s, Hz/kHz, decimalkomma.

## 4. Kod och struktur

Samma stack som KrangBox: **JUCE 8, CMake, clap-juce-extensions, C++20, AGPLv3.** Delar kopieras in och får eget namnrum (`km`). Vi gör inte ett gemensamt bibliotek nu, men vi skriver ned vilka filer som är kopior så att rättningar kan föras över för hand.

| Från KrangBox | Till Kroppsmaskinen | Ändring |
| --- | --- | --- |
| `CMakeLists.txt`, `.github/workflows/` | samma | Nya namn, plugin- och bundle-ID |
| `Stegverket.h` | `Taktverket/Trumsekvens.h` | 12 spår |
| `TonStegverket.h` | `Taktverket/Tonsekvens.h` | Accent, oktav, gatelängd, polymeter |
| `Modulation.h` | `dsp/Modulation.h` | Synk till Taktverket |
| `Basmalen.h` | `dsp/Kolvbasen.h` | Nytt filter (ladder + SVF), accentväg |
| `Hammarverket.h` | `dsp/Stampverket.h` | Nya ljud och kit |
| `KrangDistortion.h` | `dsp/Drivet.h` | Färre lägen |
| `Bandmaskin.h`, `Brusgrind.h` | `dsp/Kassetten.h` | VHS i stället för rullband |
| `PresetManager.h` | `presets/PresetManager.h` | Eget format med versionsnummer, mönster sparas med presets |
| `ValueFormat.h` | `ui/ValueFormat.h` | – |
| `KrangRender.cpp`, `KrangSnapshot.cpp` | `tools/KMRender.cpp`, `tools/KMSnapshot.cpp` | Nya testfall |

Nytt: `Hugget.h`, `Grindrummet.h`, `Ekot.h`, `Svajet.h`, `Klammen.h`, `Varvtalet.h`, `Mixern.h`, hela `ui/`.

Regler från KrangBox som gäller här också:

- **Parameter-ID:n får aldrig döpas om** när de väl finns i ett släppt bygge. Prefix `km_`.
- Ljudtråden allokerar inget, låser inget. Sekvensrarna är sampelexakta.
- Varje ny modul får ett testfall i `KMRender` (NaN, topp, DC, tystnad efter note-off) innan den räknas som klar.

## 5. Faser och grindar

Varje fas slutar med en **grind**: ett lyssnings- eller granskningstest som måste passeras innan nästa fas börjar.

### Fas 0 – Grunden

- Repo, CMake, CI (bygge, `KMRender`, pluginval), tomt plugin som laddar i Reaper och Logic.
- Kopiera in Trumsekvens, Tonsekvens, PresetManager, ValueFormat, verktygen.
- Första Kolvbasen (en oscillator, ladder, filterenvelope, accent) och tre trumljud (bastrumma, virvel, hi-hat).
- Enkelt grått gränssnitt med standardrattar – bara för att kunna spela.

*Klar i kod. Avvikelse från planen: Taktverket och mönstren skrevs som egna filer (`Source/takt/`) i stället för rena kopior, eftersom kanalerna triggas direkt i stället för via MIDI. KrangBox-filerna har varit förlaga.*

**Grind 0:** CI grön, pluginval nivå 5, och en sextondelsbas med bastrumma på 125 BPM som "sitter" vid A/B mot referensspåren.

### Fas 1 – Kroppen

- Kolvbasen komplett, Kolvbas 2.
- Stampverket med alla tolv ljud och fyra kit.
- Taktverket: mönsterbanker, A/B, glid, accent, gatelängd, polymeter, livebyte.
- Drivet per kanal.
- **Stilprov:** VHS-panelen ritad i `KMSnapshot` – chassi, skärm med trådram, OSD-typsnitt, stegknappar. Bara bilder, ingen funktion ännu.

*Klar i kod. Fabriksbandet har grind 1-looparna som presets (*EBM 128*, *Marsch 120*, *New Beat 112*); KMRender skriver dem till `renders/grind1_*.wav`. Stilprovet ligger i `docs/stilprov_vhs.png` och `docs/stilprov_detaljer.png`, ritat av `KMSnapshot --stilprov`. Parameter-ID:n gick över till formen `km_<kanal>_<vad>` (inget bygge var släppt). Spår 12 är kubjällra eller kantslag, valt med en knapp.*

**Grind 1:** Tre korta loopar (EBM 128 BPM, EBM 120 BPM marsch, New Beat 112 BPM) byggda bara med bas och trummor, renderade och genomlyssnade. Stilprovet godkänt.

### Fas 2 – Huggen och rummet

- Hugget med stötar, leads och ackordminne.
- Mixern med sändningar, Grindrummet, Ekot, Svajet, Klämman.
- Varvtalet (båda lägena) och Spänningen.
- Modulationen.

*Klar i kod. Avvikelser från planen: de fyra effekterna ligger i en fil (`Source/dsp/Effekter.h`). Varvtalets skivspelarläge gäller med egen klocka; med DAW-transport blir det automatiskt musikaliskt läge (ingen varning behövs, tempot följer projektet). Grindrummet har ett högpass på ingången så att bastrumman inte mullrar i rummet. Modulationens envelopes startas av en vald kanal (Kolvbasen, Kolvbas 2, Hugget, bastrumman eller alla toner). Nya fabrikspresets som startpunkt för grind 2: Fabriksgolv 125, Paroll 132 och Svart skiva 33.*

*Grind 2 godkänd med testlåten `Reaper/Fabriksgolv.RPP` (3:35, byggd av `tools/reaper/gor_reaper_projekt.py`).*

**Grind 2:** Ett helt spår på 3–4 minuter i Reaper, bara med Kroppsmaskinen. Ingen extern effekt.

### Fas 3 – VHS-panelen

- Hela gränssnittet enligt stilprovet: alla vyer, skalning, animering, Bandstyrka.
- Kassetten på mastern (ljudet och bilden får gärna dela "slitaget").
- Prestandamätning av gränssnittet.

*Klar i kod. Panelen ligger i `Source/ui/vhs/` och bygger på stilprovets ritverktyg. Avvikelser från planen: Kassetten ligger i `Source/dsp/Effekter.h` och är helt av vid slitage 0 och SP; dess två parametrar ligger sist i parameterlistan, så att Reaper-projektets automation står kvar. Bildskärmens bandlager räknas högst i designupplösning (640 × 304, ungefär en VHS-bild) och sträcks ut på Retina och vid förstoring; OSD-texten ovanpå är skarp. Stegradens sida följer spelhuvudet av sig själv (utom fyra sekunder efter att man valt sida) i stället för en Följ-knapp. Stampverkets trummor väljs med tolv knappar som på en TR-maskin, och stegraden redigerar vald trumma. Kanalväljarens mönsterdisplay visar variant A, B eller A+B; kedjorna kommer i fas 4. SPELA IN är stegskrivning från klaviaturen. Prestanda mäts med `KMSnapshot --panel-tid` och i pluginet med MENY → Visa gränssnittets CPU (meddelandetrådens processortid). I molnmaskinen (Xeon, JUCE:s mjukvarurenderare) kostar panelen omkring 8 % av en kärna i 100 % med sekvensern igång, nästan allt i bildskärmen (≈ 2 ms per bild, 25 bilder/s); på Macen ritar CoreGraphics, och siffran för grinden mäts där.*

*Grind 3 godkänd (2 november 2026).*

**Grind 3:** Gränssnittet under CPU-målet, alla parametrar nåbara, hela grind 2-spåret går att bygga utan att öppna DAW:ens generiska vy.

### Fas 4 – Fabriksbandet (1.0)

- 40 presets ("fabriksbandet") med mönster, fördelade på EBM, New Beat och industri.
- Kedjor och låtläge klart.
- Svensk manual i README med bilder från `KMSnapshot`.
- Två demolåtar i Reaper-mappen.
- Signering och notarisering för macOS.

*Klar i kod, väntar på grind 4. Avvikelser från planen: "kedjor och låtläge" blev ett låtläge med rader (`Source/takt/Laten.h`, vyn LÅTEN): varje rad anger mönster, variant och tystnad för alla fyra kanalerna, antal takter och valfritt Spänningen och en transponering; låten följer DAW:ens position och sparas med presetet (formatversion 3). Kedjor per kanal byggdes inte, eftersom låtraderna gör samma sak för alla kanaler samtidigt. Fabriksbandet har 40 presets (Init och 39 i EBM, New Beat och Industri) och dessutom två demolåtar i kategorin Demo (*Stålverket 128* och *Antwerpen 110*), alltså 42 poster; de nya presetsen följer mönsterplanen 1 grundgroove, 2 refräng/kontrast, 3 intro, 4 fill/uppbygge, och ett mönster med bara A får en kopia som B. Demolåtarna ligger i Reaper-mappen som projekt utan automation – låtläget sköter arrangemanget. Manualen finns i README (Ett spår på en timme, Låtläget, Fabriksbandet). Tillägg efter önskemål: **Ljudbanken** – ljud per kanal (bas, trummor, Hugget) som byter bara klangen, fabriksljuden hämtade ur fabriksbandet och egna ljud sparade som `.km87ljud` – och **Lås mönstren**, så att presets kan laddas som rena ljuduppsättningar utan att mönster, låt och tempo byts. Dessutom **MIDI-spel** per kanal (kanalen spelas av inkommande MIDI i stället för av sekvensern) och **ALLT FRÅN MIDI** under räkneverket, som tystar hela sekvensern; MIDI-läget och SPELA följer inte med presets (de räknas som uppkoppling, inte som låt), så att man kan byta ljud i en DAW utan att sekvensern startar och manualerna `docs/Bygga_en_lat.md` och `docs/Kroppsmaskinen_i_Reaper.md`. Sidorna MIXER, EFFEKTER, MODULATION, SEKVENS och LÅTEN är egna stora vyer (som i KrangBox), och vyknapparna ligger i en list under rattarna. **Signering och notarisering är uppskjutna** (beslut 2 november 2026): grind 4 görs med osignerade byggen.*

**Grind 4:** Någon som inte byggt maskinen gör ett spår på en timme med bara fabriksbandet och manualen.

### Fas 5 – Efter 1.0

- **Mikrotonala stämningar** för de melodiska kanalerna, med Scala-filer.
- **Metall i Stampverket:** fysiskt modellerade slagljud.
- **Pads och drön:** en femte, polyfon kanal med MIDI-kanal 4, egen remsa i MIXER, egen KANAL-sida och egen rad i låtläget.
- MIDI ut från Taktverket, så att sekvensrarna kan styra hårdvara.
- Gemensamt bibliotek med KrangBox (krang-libs) om de kopierade filerna börjar glida isär.

*Pågår, påbörjad 3 november 2026 före grind 4 (det finns inga externa testare än; grind 4 görs när det finns). 1.0-ljuden är oförändrade: liksvävande stämning och de fyra första kiten låter som förut. Gjort:*

- *Mikrotonala stämningar (3 november 2026). En global stämning för Kolvbasen, Kolvbas 2 och Hugget (`Source/dsp/Stamning.h`): elva inbyggda (liksvävande, ren, pytagoreisk, medeltonig, Werckmeister III, Rast, Bayati, Slendro, Pelog, 24-TET, 19-TET) och Egen med Scala-filer (`.scl` och `.kbm`). Tonikan är grundtonen eller en vald ton. Motorn räknar en tabell från tangent till liksvävande tonnummer, så glid, transponering och modulation fungerar som förut; trummorna påverkas inte. Den egna skalans text sparas i läget (egenskaperna `scl`, `kbm`, `sclNamn`). Parametrarna* Stämning *och* Stämningens tonika *ligger sist och räknas som låt (Lås mönstren behåller dem). Reglagen sitter i MIXER-vyns* Takt och master. *Mätt: liksvävande är exakt (0,00 cent); Kolvbasen och Hugget stämmer inom 0,1 cent i bas, mellanläge och diskant; Rast sänker E en kvartston (−50 cent).*
- *Metall i Stampverket (3 november 2026). Ett femte kit,* Gjuteriet*, med fysiskt modellerade kroppar: modala resonatorer (upp till 32 per röst) för stav, rör (med svävande par), plåt (rektangulär, fritt upplagd), oljefatets botten (cirkulärt membran), städ och kyrkklocka, anslagna av en klubba vars längd ger hårdheten och vars plats väger moderna; kedjan är många slumpade anslag med varierande länkar, och fjädern en fördröjning med en allpasskedja (dispersion). Spåren, MIDI-tonerna och reglagen (Stämning, Klang, Nivå) är desamma som i de andra kiten. Kitet heter Gjuteriet för att inte förväxlas med presetet* Smedjan 118*. Nivåerna är jämnade mot Fabrik-kitet (RMS per spår inom ungefär 1 dB). Gjuteriet ska lyssnas på (`KMRender --trummor <katalog> 5`).*
- *Pads och drön: **Dimman**, en femte kanal (8 november 2026). Tolv röster i stereo (`Source/dsp/Dimman.h`), tre lägen: Syntmatta (tre svävande sågtänder och en fyrkant en oktav ned genom ett mjukt lågpass), Kör (tre formantfilter, vokalen glider a → o → i) och Drönmaskin (sågtänder ringmodulerade mot en sinus i oharmonisk kvot, subsinus, långsam drift och brus genom ett svepande bandpass). Ackordminnet är Huggets; Rörelse, Bredd, Vokal och en ensemble (tre modulerade fördröjningar) över hela kanalen. Dimman spelar ett eget mönster (glid håller ackordet till nästa ton) eller, i drönläget (Grundton eller Ackord), håller tonen så länge sekvensern går och flyttar med transponeringen och låtens rader. MIDI-kanal 4. Egen remsa i MIXER (remsorna är nu 134 punkter breda), egen KANAL-sida (sida 9), femte rad i kanalväljaren och i LÅTEN, ett torn till i mixerscenen och en egen scen på bildskärmen (skorstenar och dimbankar som driver tätare ju högre Dimman spelar). Dimmans parametrar (24 st, `km_di_*`) ligger sist; i låttexten heter kanalen `di`, och raderna packas i bit 47–53, så äldre låtar läses som förut. Ljudbanken har en fjärde typ, Dimman. Tre nya fabriksprogram läggs sist:* Dimbank 96 *(Industri, drönmaskinen på Gjuteriets metall),* Kalla salar 112 *(New Beat, körmattor) och* Skiftbyte 124 *(EBM, syntmattan). Mätt: de tre lägena är rena (topp 0,49–0,59, RMS −19 till −22 dB), drönet flyttar exakt en kvint (700,0 cent) och tystnar när sekvensern stannar, Rast sänker Dimmans E med −50,0 cent. Godkänd 11 november 2026.*
- *MIDI ut från Taktverket (11 november 2026). Pluginet skickar MIDI (`NEEDS_MIDI_OUTPUT`, `producesMidi`). Parametern* MIDI ut *(Av, Med ljudet, Bara MIDI) ligger sist och hör till uppkopplingen (presets ändrar den inte). Tonerna går ut på MIDI-spels kanaler: 1 Kolvbasen, 2 Kolvbas 2, 3 Hugget, 4 Dimman, 10 Stampverket (GM). Anslag 127 för accent, 91 annars. Hugget och Dimman skickar grundtonen (ackordminnet hör till ljudet). Basarnas glid och drönet går ut som legato; trummorna släpps efter en halv sextondel; allt släpps vid stopp, hopp och när MIDI ut slås av. Kanaler som är tysta (Tyst, solo, låtens tystnad) skickar inget; Bara MIDI tystar de sekvenserade kanalerna men inte dem som spelas från MIDI. Inkommande MIDI ekas inte. I skivspelarläget flyttas händelserna till blockets tid. Ingen MIDI-klocka (VST3 kan inte skicka den från ett plugin). Mätt: tonerna ligger exakt på stegen (0 sampel), inga toner hänger.*
- *STOPP fjädrar ned när man trycker på den, som SPOLA (önskemål 4 november 2026).*
- *Testverktygen följer Ljudwerks styrdokument (6 november 2026): `KMRender --test`, `KMSnapshot --klicktest`, `--prestanda` och `--manual`; de gamla lägena finns kvar. Nya standardkontroller: stämning i cent i tre lägen, klick (tyst och solo mitt i en ton, röststöld, upprepad ton, programbyte) och CPU. Klicktestet hittade att Klämmans sidokedja följde bastrummans måltal i stället för den glidande nivån, så att tyst och solo kunde knäppa i pumpen; rättat.*
- *Mätvärden (11 november 2026, med MIDI ut): `KMRender --test` 178 kontroller, alla går igenom. `KMSnapshot --klicktest`: 235 reglage, 240 parametrar nås från panelen, 10 575 klick utan fel. pluginval nivå 5 går igenom.*
- *Mätvärden (8 november 2026, med Dimman): `KMRender --test` 173 kontroller, alla går igenom. `KMSnapshot --klicktest`: 235 reglage ändrar var och en bara sin egen parameter, 239 parametrar nås från panelen, 10 575 klick utan fel. pluginval nivå 5 går igenom.*
- *Mätvärden (6 november 2026): `KMRender --test` 160 kontroller, alla går igenom. `KMSnapshot --klicktest`: 212 reglage ändrar var och en bara sin egen parameter, 216 parametrar nås från panelen, 8 244 klick utan fel, låtvyns åtta klick. CPU i molnmaskinen (Xeon): hela maskinen med* Stålverket 128 *4 % av en kärna, med Gjuteriet, Spänningen i topp, slitet band och ren stämning också 4 % (mål under 25 %). Att mäta på M1.*

*Förslag (11 november 2026), ur Analogens genomgång av vart ljuden i den tidiga EBM:en hör hemma (Mikkes fråga om
de tidiga Front 242, ungefär 1981–84 – en måttstock som redan står i §1; inga artist-, låt- eller skivnamn i det vi
gör). Tyngdpunkten i det ljudet är sekvenserad analog bas och hårda, torra trummaskiner – Kroppsmaskinens kärna.
Det som ligger runt hör till syskonen: MS-20-skriket och råa monoleads till Analogen (finns delvis: Skrikhalsen,
Synkskriket, Atomkärnan, Stålpressen), radiorösterna och klippen till Montaget, den förvrängda sången till
Röstverket, industrieffekterna till Pannrummet och de kalla digitala basarna på lite senare skivor till Digitalen.
Inget är beslutat för Kroppsmaskinen – tas upp efter grind 4, eller som en del av fas 5 om Mikke beslutar det.*

- **En grupp i fabriksbandet, "Tidig EBM", 6–8 presets med mönster** (sist, så att presetnumren står kvar):
  - *Två låsta basar:* Kolvbasen rå och mörk, Kolvbas 2 med *Följ Kolvbasen* en sextondel efter och en oktav
    upp – det klassiska dubbla basgreppet. Sågtand och puls, kort filterenvelopp, accent på ettorna, lite glid.
  - *Torra trummaskiner:* ett kit som är tunnare och torrare än Åttaåtta – hård, kort bastrumma, en virvel med
    brus och kort gatad reverb, metalliska hi-hats, handklapp; inga långa rum.
  - *Stötar i moll:* Hugget med Blåsstöt och Orkesterstöt på enfingersackord, och en resonant synklead.
  - *Tempo 112–124 BPM*, raka sextondelar, få toner – det kala, mekaniska i början av decenniet; Kassetten på
    lite slitage.
- **Kandidat till en senare fas: ett MS-20-läge i Kolvbasen** – högpass och lågpass i serie, båda med resonans
  som skriker och mättas när den drivs (Kolvbasen har i dag 24 dB-stegen och 12 dB state variable). Nya
  parametrar läggs sist; 1.0-ljuden ska låta som förut (testas sampel för sampel). Analogens högpass
  (`krang-libs`, `Hogpass.h`) och diodstege kan vara utgångspunkter – Kroppsmaskinen har egna kopior av
  krang-libs-filerna tills vidare (se krang-libs README).
- Testfall: nivåfönstret, presetnumren oförändrade, mönstren och låtläget laddas; klicktestet. Lyssning: A/B mot
  fabriksbandets EBM-presets, så att den tidiga gruppen låter kalare och torrare.

## 6. Risker

| Risk | Motåtgärd |
| --- | --- |
| Omfånget växer som i KrangBox | Listan "Inte i 1.0" gäller. Nya kanaler kräver att grind 4 är passerad. |
| Basen låter "mjukvara" – för ren, för långsam | Envelopes med sampelupplösning, accentväg tidigt, A/B mot referenser vid varje grind. |
| VHS-effekten i gränssnittet äter CPU eller tröttar ögonen | Separat skärmlager, 30 fps-tak, cache, Bandstyrka-reglage. |
| Varvtalet krockar med DAW-tempo | Skivspelarläget bara i den fristående appen eller med tydlig varning; musikaliskt läge i DAW. |
| Kopierad kod glider isär från KrangBox | Kopielistan ovan; rättningar förs över för hand (krang-libs, beslut 3 november 2026). |
| Aliasing i stötar, synk och drive | PolyBLEP-oscillatorer, översampling i Drivet och synken, renderingstest med spektrumkontroll. |

## 7. Beslut

- Modellnamnet är **KM-87**.
- **Kolvbas 2 är en egen kanal**, inte ett läge i Kolvbasen. Det tunga ljudet hos Front 242, Nitzer Ebb, DAF och Die Krupps kommer ofta av två basar i lager: en mörk, ren botten och en ljus, distad bas ovanpå, eller två mönster som går i varandra. Det kräver två röster som kan klinga samtidigt, med egen klang, eget Drivet, egen nivå och egen sidokedja i Mixern. Ett läge med en röst klarar inte det.
- **16 stegknappar med sidbyte** (sidorna I–IV, upp till 64 steg). EBM- och New Beat-mönster är nästan alltid en takt sextondelar, så en takt syns i sin helhet utan att bläddra. 16 knappar i fyra grupper om fyra är dessutom 808/909-greppet som genren byggdes på, och knapparna blir tillräckligt stora för att träffa även vid 60 % skala. Fyra sidlampor visar vilken sida som spelas och vilken som redigeras; läget **Följ** låter redigeringssidan hänga med uppspelningen.
- **Låtläget är en låt med rader** (som 909:ans), inte kedjor per kanal. Demolåtarna styrs av låtläget, inte av automation.
- **Signering och notarisering skjuts upp** till efter grind 4.
- **Den femte kanalen heter Dimman**, spelar mönster eller drön och har lägena Syntmatta, Kör och Drönmaskin (beslut 8 november 2026).
- **Ropet stryks; vokoder och talmaskin hör till Röstverket RV-83** (beslut 6 november 2026).
- **Fas 5 börjar före grind 4** (beslut 3 november 2026): det finns inga externa testare än. Pads och drön blir en femte kanal, metallen ett femte kit (Gjuteriet), stämningen global med Scala-filer.
- **MIDI-läget hör till uppkopplingen, inte till presetet.** ALLT FRÅN MIDI, kanalernas MIDI-spel och SPELA behålls när presets byts (beslut 3 november 2026).
- **Presets och mönster får ett eget format**, utan hänsyn till KrangBox. Formatet får ett versionsnummer från första bygget, så att senare ändringar kan läsa äldre filer.
- GitHub-repot heter **`mikkeschiren/Kroppsmaskinen`**.

Inga öppna frågor just nu.
