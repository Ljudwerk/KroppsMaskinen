# Kroppsmaskinen – plan

*Modell KM-87. En groovebox för Electronic Body Music och New Beat, byggd som om den kom ur en svart hemelektroniklåda från 1987 och bara gick att se på genom en VHS-skärm.*

**Status:** Fas 0–3 klara och grind 0–3 godkända. Fas 4, fabriksbandet (1.0), är byggd och väntar på grind 4. Se README.md. Stilprovet och loggan godkändes i fas 1 (2 november 2026).

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

- Vokoder och röstmodul (*Ropet*, se fas 5)
- Pads, drönljud, fysisk modellering, mikrotonala stämningar
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

*Klar i kod, väntar på grind 4. Avvikelser från planen: "kedjor och låtläge" blev ett låtläge med rader (`Source/takt/Laten.h`, vyn LÅTEN): varje rad anger mönster, variant och tystnad för alla fyra kanalerna, antal takter och valfritt Spänningen och en transponering; låten följer DAW:ens position och sparas med presetet (formatversion 3). Kedjor per kanal byggdes inte, eftersom låtraderna gör samma sak för alla kanaler samtidigt. Fabriksbandet har 40 presets (Init och 39 i EBM, New Beat och Industri) och dessutom två demolåtar i kategorin Demo (*Stålverket 128* och *Antwerpen 110*), alltså 42 poster; de nya presetsen följer mönsterplanen 1 grundgroove, 2 refräng/kontrast, 3 intro, 4 fill/uppbygge, och ett mönster med bara A får en kopia som B. Demolåtarna ligger i Reaper-mappen som projekt utan automation – låtläget sköter arrangemanget. Manualen finns i README (Ett spår på en timme, Låtläget, Fabriksbandet). Tillägg efter önskemål: **Ljudbanken** – ljud per kanal (bas, trummor, Hugget) som byter bara klangen, fabriksljuden hämtade ur fabriksbandet och egna ljud sparade som `.km87ljud` – och **Lås mönstren**, så att presets kan laddas som rena ljuduppsättningar utan att mönster, låt och tempo byts. Dessutom **MIDI-spel** per kanal (kanalen spelas av inkommande MIDI i stället för av sekvensern) och manualerna `docs/Bygga_en_lat.md` och `docs/Kroppsmaskinen_i_Reaper.md`. Sidorna MIXER, EFFEKTER, MODULATION, SEKVENS och LÅTEN är egna stora vyer (som i KrangBox), och vyknapparna ligger i en list under rattarna. **Signering och notarisering är uppskjutna** (beslut 2 november 2026): grind 4 görs med osignerade byggen.*

**Grind 4:** Någon som inte byggt maskinen gör ett spår på en timme med bara fabriksbandet och manualen.

### Fas 5 – Efter 1.0 (kandidater)

- **Ropet:** vokoder med sidokedjeingång och inbyggd talmaskin (formantsyntesen från Automaten i KrangBox) för robotparoller.
- MIDI ut från Taktverket, så att sekvensrarna kan styra hårdvara.
- Gemensamt bibliotek med KrangBox om de kopierade filerna börjar glida isär.

## 6. Risker

| Risk | Motåtgärd |
| --- | --- |
| Omfånget växer som i KrangBox | Listan "Inte i 1.0" gäller. Nya kanaler kräver att grind 4 är passerad. |
| Basen låter "mjukvara" – för ren, för långsam | Envelopes med sampelupplösning, accentväg tidigt, A/B mot referenser vid varje grind. |
| VHS-effekten i gränssnittet äter CPU eller tröttar ögonen | Separat skärmlager, 30 fps-tak, cache, Bandstyrka-reglage. |
| Varvtalet krockar med DAW-tempo | Skivspelarläget bara i den fristående appen eller med tydlig varning; musikaliskt läge i DAW. |
| Kopierad kod glider isär från KrangBox | Kopielistan ovan; rättningar förs över för hand tills fas 5. |
| Aliasing i stötar, synk och drive | PolyBLEP-oscillatorer, översampling i Drivet och synken, renderingstest med spektrumkontroll. |

## 7. Beslut

- Modellnamnet är **KM-87**.
- **Kolvbas 2 är en egen kanal**, inte ett läge i Kolvbasen. Det tunga ljudet hos Front 242, Nitzer Ebb, DAF och Die Krupps kommer ofta av två basar i lager: en mörk, ren botten och en ljus, distad bas ovanpå, eller två mönster som går i varandra. Det kräver två röster som kan klinga samtidigt, med egen klang, eget Drivet, egen nivå och egen sidokedja i Mixern. Ett läge med en röst klarar inte det.
- **16 stegknappar med sidbyte** (sidorna I–IV, upp till 64 steg). EBM- och New Beat-mönster är nästan alltid en takt sextondelar, så en takt syns i sin helhet utan att bläddra. 16 knappar i fyra grupper om fyra är dessutom 808/909-greppet som genren byggdes på, och knapparna blir tillräckligt stora för att träffa även vid 60 % skala. Fyra sidlampor visar vilken sida som spelas och vilken som redigeras; läget **Följ** låter redigeringssidan hänga med uppspelningen.
- **Låtläget är en låt med rader** (som 909:ans), inte kedjor per kanal. Demolåtarna styrs av låtläget, inte av automation.
- **Signering och notarisering skjuts upp** till efter grind 4.
- **Presets och mönster får ett eget format**, utan hänsyn till KrangBox. Formatet får ett versionsnummer från första bygget, så att senare ändringar kan läsa äldre filer.
- GitHub-repot heter **`mikkeschiren/Kroppsmaskinen`**.

Inga öppna frågor just nu.
