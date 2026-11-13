# Bygga en låt i Kroppsmaskinen

Den här manualen går igenom hur en hel låt byggs i Kroppsmaskinen själv – i den fristående appen eller i pluginet – från ett tomt band till ett färdigt arrangemang i låtläget. Hur låten sedan spelas in, mixas och renderas i en DAW står i [Kroppsmaskinen i Reaper](Kroppsmaskinen_i_Reaper.md).

Innehåll:

1. [Hur maskinen tänker](#1-hur-maskinen-tänker)
2. [Förbered: tempo, grundton och ljud](#2-förbered-tempo-grundton-och-ljud)
3. [Skriv mönstren](#3-skriv-mönstren)
4. [Variation: A och B, längd, Följ Kolvbasen](#4-variation-a-och-b-längd-följ-kolvbasen)
5. [Prova arrangemanget live](#5-prova-arrangemanget-live)
6. [Bygg låten i LÅTEN](#6-bygg-låten-i-låten)
7. [Uppbyggen, break och tonartsbyten](#7-uppbyggen-break-och-tonartsbyten)
8. [Spara](#8-spara)
9. [Ett exempel från början till slut](#9-ett-exempel-från-början-till-slut)
10. [Felsökning](#10-felsökning)

---

## 1. Hur maskinen tänker

Kroppsmaskinen har fem **kanaler** som spelas av en gemensam sekvenser:

| Kanal | Vad | Mönstertyp |
| --- | --- | --- |
| 1 **Kolvbasen** | Sekvensbasen (monofon) | Tonmönster |
| 2 **Kolvbas 2** | Andra basen – eget mönster eller *Följ Kolvbasen* | Tonmönster |
| 3 **Stampverket** | Tolv trummor | Trummönster (tolv spår) |
| 4 **Hugget** | Stötar och leads (åtta röster, ackordminne) | Tonmönster |
| 5 **Dimman** | Mattor och drön (tolv röster, ackordminne, drönläge) | Tonmönster, eller drönet |

Varje kanal har **16 mönster**, och varje mönster har två **varianter**, **A** och **B**. Ett mönster är upp till 64 steg (sextondelar), fyra sidor om 16 (I–IV). Kanalens **Variant** bestämmer vad som spelas: *A*, *B* eller *A+B* (A och B växlar varje varv).

En **låt** är en lista med upp till 64 **rader**. Varje rad säger, för alla fem kanalerna på en gång: vilket mönster och vilken variant som spelas, vilka kanaler som är tysta, hur många takter raden varar – och valfritt hur högt Spänningen står och om tonkanalerna ska transponeras. Med **LÅTLÄGET** på spelar maskinen raderna i tur och ordning, takt för takt.

Arbetsgången blir alltså: först ljuden och mönstren (delarna), sedan låten (ordningen).

> En takt är alltid 16 sextondelar. Mönster med en annan längd (t.ex. 12 steg) går runt i sin egen takt (polymeter), men låtens rader byter alltid på en taktgräns.

---

## 2. Förbered: tempo, grundton och ljud

1. **Välj en startpunkt.** Bläddra med ◀ ▶ under presetdisplayen, eller klicka på presetnumret för listan. *Init* är tomt (en bastrumma och en ton). Presets i **EBM**, **New Beat** och **Industri** har färdiga ljud och mönster; i de nyare är mönster **1** grundgrooven, **2** refrängen eller kontrasten, **3** ett intro och **4** en fill eller ett uppbygge. Se [Fabriksbandet](../README.md#fabriksbandet).
2. **Tempo:** dra i tempodisplayen (Skift = tiondelar), rulla, eller dubbelklicka för 125. EBM ligger oftast på 115–135 BPM, New Beat på 100–118.
3. **Grundton** (MIXER → *Takt och master*): basgångens "0". Alla tonmönster räknas i halvtoner från den, så en hel låt kan flyttas till en annan tonart genom att vrida på grundtonen.
4. **Gung** (MIXER eller SEKVENS): förskjuter varannan sextondel. 0 % är rakt; 10–15 % ger en mjukare New Beat-känsla.
5. **Varvtalet:** *45* är normalt. *33* och *33 + 8 %* är New Beat-tricket – en 45-varvare på fel varvtal. I den fristående appen sänker det tempot och tonhöjden tillsammans (eller hela maskinen i *Skivspelare*-läget).
6. **Ljuden.** Antingen behåller du presetets ljud, eller så byter du per kanal med **LJUD** ◀ namn ▶ i KANAL-vyn (ljudbanken: 79 basljud, 40 trumljud, 37 ljud för Hugget). Ljudbanken byter bara klangen – mönstren och mixen står kvar. Vill du prova hela fabriksbandets ljud på dina mönster: MENY → *Lås mönstren*, och bläddra bland presets.

---

## 3. Skriv mönstren

Välj kanal i kanalväljaren under presets och mönster med **Mönster** i lådan *Kanal* på KANAL-sidan (eller överst på SEKVENS-sidan): klick = nästa, Skift-klick = förra, rulla, högerklick = lista. **Variant** bredvid väljer A, B eller A+B. Stegraden och SEKVENS-sidan visar och ändrar det mönster som Mönster pekar på, i den variant som **A/B**-knappen längst till höger i stegraden visar.

### Trummor (Stampverket)

1. Gå till **KANAL** med Stampverket valt. Välj trumma med de tolv knapparna (BD, SD, SD2, CP, CH, OH, CY, LT, HT, MET, SMÄLL, KO) – som på en TR-maskin.
2. Tryck i stegraden: klick växlar **av → slag → accent** (accent har en liten röd triangel).
3. **Skift-klick**: ratchet – 1, 2, 3 eller 4 slag inom steget (virvelvirvlar, hi-hat-rullar).
4. **Alt-klick**: chans – 100, 75, 50 eller 25 % att steget slår. Ger levande hi-hats.
5. Hela trummönstret syns i **SEKVENS**: tolv rader × sexton steg. Klicka på en trummas namn för att välja den.

Ett klassiskt EBM-grundmönster: bastrumma på 1, 5, 9, 13 (fyra i takten), virvel på 5 och 13, stängd hi-hat på alla udda steg, accent på varje fjärde.

### Basgången (Kolvbasen och Kolvbas 2)

1. Klick i stegraden växlar **av → ton → accent**.
2. **Dra upp/ned** eller **rulla** på ett steg: tonen, ±12 halvtoner från grundtonen. Talet syns på knappen (inget tal = grundtonen).
3. **Högerklick**: glid – tonen hålls och glider in i nästa.
4. **Alt-klick**: oktav upp → ned → av (pil på knappen).
5. **Skift-klick**: tonlängd, ¼–4 steg. Korta toner ger den studsande EBM-basen.
6. I **SEKVENS** syns gången som en tonrulle: klicka på en rad för att sätta tonen, dra för att flytta den.

Tips: EBM-basen är nästan alltid sextondelar med accenter på taktslagen och ett och annat oktavhopp. Börja med alla sexton steg på grundtonen, lägg accent på 1, 5, 9, 13 och ändra sedan två eller tre steg.

### Hugget (stötar och leads)

Samma sätt att skriva som basen. Välj läge och ackord i KANAL-vyn: med ackordminnet (t.ex. *Moll*) blir varje ton i mönstret ett helt ackord – en enda rad ger fulla stötar. Stötar sitter ofta glest: på 1 och "och" före 9, eller bara i refrängen.

### Dimman (mattor och drön)

Dimman spelar långa toner. Skriv ett ackord på första steget med glid (*~*), så hålls det till nästa ton: `0~ . . . . . . . . . . . . . . .` med Längd 32 och `-4~` på steg 17 ger två ackord som byter varannan takt. Med ackordminnet räcker en ton per ackord. Välj läge (*Syntmatta*, *Kör* eller *Drönmaskin*), **Attack** och **Släpp** (mattor vill ha långa), **Rörelse**, **Bredd**, **Vokal** (kören) och **Ensemble** i KANAL-vyn.

**Drönläget** (*Drön*: *Grundton* eller *Ackord*) ersätter mönstret: Dimman håller grundtonen – eller ackordet – så länge sekvensern går, och flyttar med när du transponerar eller när en låtrad har **TRANSP.**. Bra för industri: en drönmaskin under hela låten, tystad på de rader där den inte ska höras.

### Stegskrivning från klaviaturen (SPELA IN)

1. Tryck **SPELA IN** (den röda knappen i bandverket). Markören blinkar orange i stegraden.
2. Spela på klaviaturen – med musen eller datorns tangentbord (**A W S E D F T G Y H U J K O L P** = C … D♯, **Z/X** byter oktav). Varje ton skrivs in där markören står, och markören flyttas ett steg. Hårt anslag (nederkanten av tangenten med musen) ger accent.
3. Klicka på ett steg för att flytta markören. För trummor skriver tangenterna in trumman enligt GM-kartan (C2 bastrumma, D2 virvel, F♯2 hi-hat …).
4. Medan sekvensern går hamnar tonen i det steg som spelas – inspelning i realtid, kvantiserad till sextondelar.
5. Tryck **SPELA IN** igen för att sluta.

### Hjälpmedel

- **Kopiera A→B** (SEKVENS): B blir en kopia av A – utgångspunkt för en variation.
- **Rensa** (SEKVENS): tömmer den variant som visas.
- **Följ** sidan: när mönstret är längre än 16 steg följer stegraden spelhuvudet; väljer du en sida själv stannar den där i fyra sekunder.

---

## 4. Variation: A och B, längd, Följ Kolvbasen

- **A+B**: sätt kanalens Variant till A+B (väljaren *Variant* visar *A+B*). Då spelas A och B varannan takt – A som grund och B med en fill i slutet ger en tvåtaktsfras utan att låten behöver fler rader.
- **Längd** (SEKVENS, 1–64 steg): en Kolvbas 2 på 12 steg mot trummornas 16 ger ett mönster som förskjuts mot takten och kommer tillbaka efter tre takter.
- **Följ Kolvbasen** (Kolvbas 2, KANAL-vyn): Kolvbas 2 spelar Kolvbasens mönster, förskjutet 0–3 sextondelar och transponerat ±24 halvtoner. Det klassiska EBM-lagret: en mörk botten i Kolvbasen och samma gång en oktav upp, överstyrd, i Kolvbas 2.
- **Fyra mönster per kanal räcker långt**: intro (glest), vers, refräng (mer), break (bara bas eller bara trummor).

---

## 5. Prova arrangemanget live

Innan låten skrivs in är det bra att spela den för hand:

- Byt **mönster** medan sekvensern går – bytet sker på nästa takt, så det blir alltid i takt.
- **Högerklick på en kanal** i kanalväljaren tystar den (TYST står på knappen). Solo finns i MIXER.
- Dra i **Spänningen** – hela maskinen drar mot mer energi (hårdare accenter, mer drive, mer rum, mer pump). Bra inför refränger.
- Prova Varvtalet, Ekot och Grindrummet.

När du hittat ordningen: skriv in den i låten.

---

## 6. Bygg låten i LÅTEN

Gå till **LÅTEN**. Överst: **LÅTLÄGET** (på/av), **Vid slutet** (*Loopa* eller *Stanna*), och **KOPIERA**, **TA BORT** och **HÄMTA** för vald rad. Under: raderna som kolumner, sexton per sida.

| Rad i tabellen | Vad | Ändra |
| --- | --- | --- |
| RAD | Radens nummer; orange = raden som spelas | Klicka för att välja raden |
| TAKTER | Hur många takter raden spelas (1–64) | Klick = +1, högerklick/Alt-klick = −1, dra eller rulla; dubbelklick = 4 |
| KOLVB. / KOLVB. 2 / STAMPV. / HUGGET / DIMMAN | Mönster och variant, t.ex. *3AB* = mönster 3, A+B | Dra eller rulla = mönster; klick = variant A → B → A+B; högerklick = tyst |
| SPÄNN. | Spänningen för raden | Klick = +10 %, högerklick/Alt-klick = −10 % (under 0 blir det "–", rör inte spaken); dra eller rulla i steg om 5 %; dubbelklick = "–" |
| TRANSP. | Transponering av Kolvbasen, Kolvbas 2, Hugget och Dimman (även drönet) | Klick = +1, högerklick/Alt-klick = −1, dra eller rulla (±24); dubbelklick = 0 |

Klicka i rubrikkolumnen (eller rulla där) för att byta sida (1/4 … 4/4).

### Arbetsgång

1. **Första raden:** klicka på **+**. Den nya raden får panelens mönster, varianter och tystnad. (Är låten tom tas raden alltid från panelen.)
2. Ställ **TAKTER** – t.ex. 8.
3. **Nästa rad:** ställ in mönstren som du vill ha dem med panelen (kanalväljaren och väljarna *Mönster* och *Variant*), klicka på **+** (raden blir en kopia av den förra) och tryck **HÄMTA** – den valda raden tar då panelens mönster, varianter och tystnad. Eller ändra direkt i rutorna.
4. **KOPIERA** lägger en kopia av vald rad direkt efter den – bra för att upprepa en del och ändra en detalj.
5. **TA BORT** tar bort vald rad.
6. Slå på **LÅTLÄGET**, tryck **SPOLA** och **SPELA**. Bildskärmen visar *RAD 03 2/8* (rad 3, takt 2 av 8).
7. Välj **Vid slutet**: *Stanna* för en färdig låt (tyst efter sista raden), *Loopa* för att låta den gå runt medan du arbetar.

Med LÅTLÄGET av styr panelens Mönster och Variant som vanligt – låten finns kvar och kan slås på igen.

> Lyser **ALLT FRÅN MIDI** under räkneverket är sekvensern tyst – släck den för att höra mönstren.
>
> **SPOLA** börjar om från takt 1 och nollar räkneverket. **STOPP** följt av **SPELA** börjar också från början – det finns ingen paus.

### Bra att veta

- Mönster i **A+B** växlar varannan takt räknat från låtens början. Ge rader med A+B ett jämnt antal takter, så hamnar A och B rätt.
- En kanal som är **tyst** på raden spelar inte, men dess mönster finns kvar.
- Kanalernas egna Tyst och Solo i panelen gäller fortfarande ovanpå låten – praktiskt för att lyssna på en kanal i taget.
- Ändrar du ett mönster medan låten går hörs ändringen nästa gång mönstret spelas.

---

## 7. Uppbyggen, break och tonartsbyten

- **Uppbygge:** lägg två rader om 4 takter före refrängen, med Spänningen 30 % och 60 %, och ett fill-mönster (4) på trummorna på den sista.
- **Break:** en rad där trummorna är tysta (högerklick på STAMPV.) eller bara bastrumman spelar, och Spänningen sjunker till 0.
- **Refräng:** mönster 2 på alla kanaler, Hugget på, Spänningen 40–60 %.
- **Tonartsbyte:** *TRANSP.* +5 (en kvart upp) eller +2 på de sista refrängerna. Trummorna transponeras inte.
- **Slut:** en sista rad om 1 takt med bara stötarna, och *Vid slutet: Stanna*.
- **Spänningen "–"** betyder att raden inte rör spaken, så du kan dra i den för hand.

---

## 8. Spara

**SPARA** under presetdisplayen sparar allt: ljuden, effekterna, alla mönster och hela låten (och att låtläget är på). Egna presets hamnar under *Egna* i listan och sparas som `.km87`-filer i:

- macOS: `~/Library/Application Support/Kroppsmaskinen/Presets/`

Egna ljud från ljudbanken sparas för sig under `Presets/Ljud/`. I en DAW sparas allt dessutom i projektet.

---

## 9. Ett exempel från början till slut

En EBM-låt på 125 BPM, cirka 3½ minut (≈ 108 takter):

1. Ladda *Init*. Tempo 125. Grundton A1.
2. **Stampverket**, kit *Niononio*:
   - mönster 1A: BD 1 5 9 13, SD 5 13, CH alla udda steg, accent på 1 och 9.
   - 1B: som 1A plus en virvelvirvel (ratchet 4) på steg 15–16.
   - 2A: som 1A plus OH på 3 7 11 15 och handklapp med virveln.
   - 3A: bara BD och CH (intro).
3. **Kolvbasen**: mönster 1A sexton toner på 0 med accent på 1 5 9 13, oktav upp på 3 och 11. 2A: samma med −2 på steg 13–16.
4. **Kolvbas 2**: *Följ Kolvbasen* på, transponering +12, Drivet *Overdrive*.
5. **Hugget**: läge *Orkesterstöt*, ackord *Moll*. 1A: tom. 2A: stötar på steg 1 och 11.
6. **LÅTEN**:

| Rad | Takter | Kolvb. | Kolvb. 2 | Stampv. | Hugget | Spänn. | Transp. |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 8 | 1A tyst | 1A tyst | 3A | tyst | 0 % | 0 |
| 2 | 8 | 1A | 1A tyst | 1AB | tyst | 10 % | 0 |
| 3 | 16 | 1A | 1A | 1AB | tyst | 20 % | 0 |
| 4 | 4 | 1A | 1A | 3A | tyst | 40 % | 0 |
| 5 | 16 | 2A | 2A | 2A | 2A | 50 % | 0 |
| 6 | 8 | 1A | 1A tyst | tyst | tyst | 0 % | 0 |
| 7 | 4 | 1A | 1A | 1AB | tyst | 60 % | 0 |
| 8 | 16 | 1A | 1A | 1AB | 1A tyst | 30 % | 0 |
| 9 | 16 | 2A | 2A | 2A | 2A | 65 % | +5 |
| 10 | 8 | 1A | 1A tyst | 3A | tyst | 0 % | 0 |
| 11 | 1 | 1A tyst | 1A tyst | tyst | 2A | – | 0 |

7. LÅTLÄGET på, Vid slutet *Stanna*. SPOLA, SPELA. Justera, och **SPARA** som t.ex. *Min första låt*.

Demolåtarna *Stålverket 128* och *Antwerpen 110* (kategorin Demo) är färdiga exempel med 18 rader vardera – öppna dem och titta i LÅTEN.

---

## 10. Felsökning

| Problem | Orsak och lösning |
| --- | --- |
| Varannan takt är tyst | Kanalen står på A+B men B-varianten är tom. Kopiera A→B, eller ändra varianten till A. |
| Låten spelar inte raderna | LÅTLÄGET är av, eller låten är tom. |
| Låten börjar mitt i | I en DAW räknas låten från projektets början (takt 1 = rad 1). Se Reaper-manualen. |
| Stegraden visar inte spelhuvudet | Stegraden visar mönstret som Mönster pekar på; i låtläget kan en annan rad spela ett annat mönster. |
| Tempo och tonhöjd fel i appen | Ljudkortet går i fel takt (t.ex. Bluetooth-lurar på 16 kHz) – presetdisplayen visar en orange varning. Välj 44,1 eller 48 kHz under *Options*. |
| Datorns tangentbord spelar inte | Klicka i panelen först, så att den har tangentbordet. |
