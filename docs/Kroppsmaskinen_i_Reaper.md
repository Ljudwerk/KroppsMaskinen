# Kroppsmaskinen i Reaper

Kroppsmaskinen fungerar på två sätt i en DAW:

- **A. Som en hel maskin:** låten byggs i Kroppsmaskinen (mönster och låtläge), och Reaper spelar in, mixar och renderar. Se [del A](#a-bygga-låten-i-kroppsmaskinen-och-använda-reaper).
- **B. Som instrument:** Kroppsmaskinens bas, trummor och stötar spelas av MIDI från Reaper i en låt som byggs i Reaper. Se [del B](#b-kroppsmaskinens-ljud-i-låtar-som-görs-i-reaper).

Det går också att blanda: trummorna från Kroppsmaskinens sekvenser, basen från Reapers MIDI.

Manualen utgår från Reaper 7 på macOS, men allt utom installationen gäller också andra DAW:ar (Logic, Ableton Live, Bitwig, Cubase) – menyerna heter bara annorlunda.

Innehåll:

- [Installera och lägga in pluginet](#installera-och-lägga-in-pluginet)
- [Hur Kroppsmaskinen följer Reaper](#hur-kroppsmaskinen-följer-reaper)
- [A. Bygga låten i Kroppsmaskinen och använda Reaper](#a-bygga-låten-i-kroppsmaskinen-och-använda-reaper)
- [B. Kroppsmaskinens ljud i låtar som görs i Reaper](#b-kroppsmaskinens-ljud-i-låtar-som-görs-i-reaper)
- [Mixa, separata spår och rendering](#mixa-separata-spår-och-rendering)
- [Referens: MIDI](#referens-midi)
- [Felsökning](#felsökning)

---

## Installera och lägga in pluginet

1. Bygg (se README, *Bygga lokalt*) och kopiera pluginet:

   ```sh
   cp -R build/Kroppsmaskinen_artefacts/Release/VST3/Kroppsmaskinen.vst3 ~/Library/Audio/Plug-Ins/VST3/
   cp -R build/Kroppsmaskinen_artefacts/Release/CLAP/Kroppsmaskinen.clap ~/Library/Audio/Plug-Ins/CLAP/
   ```

   (AU-versionen behövs för Logic, inte för Reaper.)
2. Starta Reaper. Om pluginet inte syns: *Options → Preferences → Plug-ins → VST → Re-scan* (och *CLAP* för CLAP-versionen).
3. Skapa ett spår, klicka på **FX** och välj *VST3i: Kroppsmaskinen* (eller *CLAPi*). Det är ett instrument med en stereoutgång.
4. **Armera spåret för inspelning** och sätt ingången till *Input: MIDI → All MIDI inputs → All channels* om du vill spela på ett MIDI-keyboard. Slå på *Record monitoring*, annars hörs inte keyboardet.

Färdiga projekt finns i `Reaper/`: `Fabriksgolv.RPP` (grind 2-låten, arrangerad med automation), `Stalverket.RPP` och `Antwerpen.RPP` (demolåtarna, arrangerade i låtläget).

---

## Hur Kroppsmaskinen följer Reaper

Det här är det viktigaste att förstå:

| | Fristående app | Plugin i Reaper |
| --- | --- | --- |
| **Klocka** | Egen; SPELA och STOPP i bandverket | Reapers transport: när Reaper spelar, spelar sekvensern. Pluginets SPELA/STOPP gör ingenting. |
| **Tempo** | Tempodisplayen | Projektets tempo (även tempoförändringar). Tempodisplayen används inte. |
| **Position** | Börjar alltid från början | Följer Reapers position exakt. Hoppar du, loopar eller spolar, hänger sekvensern med. |
| **Låtläget** | Från rad 1 när du trycker SPELA | Takt 1 i projektet = rad 1 i låten. Låten räknas alltid från projektets början. |
| **Varvtalet** | Sänker tempo och tonhöjd | Bara tonhöjden och klangtiderna – tempot följer projektet. Skivspelarläget blir musikaliskt läge. |
| **MIDI-toner när sekvensern går** | Transponerar tongångarna | Samma: en ton på kanal 1–3 flyttar alla tongångar (räknat från Grundton), om inte kanalen spelas från MIDI (se del B). |

Ekot, LFO:er i takt och Gung följer projektets tempo. Projektet ska gå i **4/4** – en takt är alltid 16 sextondelar för Kroppsmaskinen.

Allt sparas i Reaper-projektet: ljud, mönster, låten, panelens storlek och bandstyrka.

---

## A. Bygga låten i Kroppsmaskinen och använda Reaper

### A1. Bygg låten

Bygg låten som i [Bygga en låt i Kroppsmaskinen](Bygga_en_lat.md): mönster per kanal och rader i **LÅTEN** med **LÅTLÄGET** på. Det går lika bra att göra det i Reaper som i den fristående appen – låten följer med pluginets läge. (En låt sparad som preset i appen kan laddas i Reaper med ◀ ▶ eller presetlistan.)

### A2. Spela låten i Reaper

1. Sätt projektets tempo (*Project Settings* eller tempofältet i transporten) till låtens tempo, t.ex. 128.
2. Ställ markören på **takt 1** och tryck play. Låten börjar på rad 1.
3. Startar du mitt i projektet börjar låten på den rad som hör till den takten – precis som om den hade spelat från början.
4. **Vid slutet: Stanna** gör maskinen tyst efter sista raden. *Loopa* låter den börja om.

> Vill du ha en tom inledning innan låten börjar (t.ex. en sång som ska ligga före)? Lägg en första rad i låten där alla kanaler är tysta, med så många takter som behövs.

### A3. Göra ändringar under arbetet

- **Ändra i låten:** stoppa (eller låt Reaper loopa en del), ändra rader eller mönster i pluginet. Ändringen hörs nästa gång raden eller mönstret spelas.
- **Loopa en del:** sätt en loop i Reapers tidslinje över de takter du arbetar med. Kroppsmaskinen hoppar tillbaka med Reaper och spelar rätt rad.
- **Spela över låten:** spela in sång, gitarr eller andra instrument på egna spår som vanligt.

### A4. Automation ovanpå låten

Alla 213 parametrar kan automatiseras, även när låtläget sköter mönstren:

1. Klicka på spårets **automationsknapp** (*Trim/Read*) och välj *Latch* eller *Touch*.
2. Spela, och rör reglagen i Kroppsmaskinens panel – rörelserna spelas in som automation (filtersvep, Spänningen, ekots nivå …).
3. Ställ tillbaka till *Read*. Kurvorna syns under spåret och kan ritas om för hand (*Envelopes* → välj parameter).

Spänningen per rad i låten och Spänningen som automation krockar: en rad med ett eget Spänningsvärde vinner. Sätt raderna till "–" om du hellre vill automatisera spaken i Reaper.

### A5. Arrangera med automation i stället för låtläget

Det går också att arrangera helt i Reaper, som i `Reaper/Fabriksgolv.RPP`: låtläget av, och automation på kanalernas **Mönster**, **Variant** och **Tyst** (t.ex. *Kolvbasen mönster*, *Stampverket tyst*). Mönsterbyten sker alltid på nästa takt, så automationspunkterna behöver inte ligga exakt. Fördelen är att arrangemanget syns i Reapers tidslinje; nackdelen att det inte följer med presetet.

### A6. Transponera från Reaper

En MIDI-region på spåret med toner på kanal 1 transponerar alla tongångar medan sekvensern går: grundtonen = ingen transponering, en ton en kvart upp = +5 halvtoner, osv. (±24). `Fabriksgolv.RPP` gör så i takt 57–64.

---

## B. Kroppsmaskinens ljud i låtar som görs i Reaper

Här används Kroppsmaskinen som synt och trummaskin: Reapers MIDI spelar ljuden, och Kroppsmaskinens egen sekvenser används inte (eller bara för vissa kanaler).

### B1. MIDI-spel per kanal

Varje kanal har en knapp **MIDI-spel** (i KANAL-vyn, gruppen *Kanal*, och **MIDI** i MIXER). Med den på:

- spelar sekvensern **inte** kanalen (dess mönster tystnar), och
- spelar MIDI-toner på kanalens MIDI-kanal den **alltid** – även när Reaper spelar.

| Kanal | MIDI-kanal | Spelas som |
| --- | --- | --- |
| Kolvbasen | 1 | Monofon bas |
| Kolvbas 2 | 2 | Monofon bas |
| Hugget | 3 | Åtta röster; med ackordminnet blir varje ton ett ackord |
| Stampverket | 10 | Trummor enligt GM-kartan (se [Referens](#referens-midi)) |

Utan MIDI-spel gäller det vanliga: medan sekvensern går transponerar tonerna på kanal 1–3 i stället för att spela. Trummorna på kanal 10 spelar alltid, men med MIDI-spel av spelar sekvensens trummor dessutom.

### B2. Basen som synt

1. Lägg Kroppsmaskinen på ett spår. Välj ett basljud: presetlistan (t.ex. *Valsverk 124*) eller **LJUD** i KANAL-vyn med Kolvbasen vald.
2. Slå på **MIDI-spel** för Kolvbasen, och **Tyst** på de kanaler du inte använder (eller MIDI-spel på dem också, så att sekvensern inte spelar dem).
3. Rita eller spela in en MIDI-region på kanal 1. Kolvbasen är monofon: spela en ton i taget. Korta toner ger den studsande EBM-basen; sextondelar med accent kommer bäst fram med ett kort *Fall* och lite *Accent*.
4. Anslaget och modhjulet kan styra ljudet: i **MODULATION** finns källorna *Anslag*, *Modhjul* (CC 1) och *Aftertouch* – koppla dem till t.ex. *Kolvbasen filter*.

**Två basar:** sätt även Kolvbas 2 på MIDI-spel och skicka samma toner på kanal 2 (eller slå på *Följ Kolvbasen* – den följer bara sekvensern, inte MIDI). Alternativt två instanser av pluginet.

### B3. Stampverket som trummaskin

1. Ladda ett preset med ett trumkit du gillar, eller välj ett trumljud i **LJUD** med Stampverket valt. Kit, stämning, klang och nivå per trumma finns i KANAL-vyn.
2. Slå på **MIDI-spel** för Stampverket (annars spelar presetets trummönster också) och tysta de andra kanalerna.
3. Rita trummor i en MIDI-region på **kanal 10**. I Reapers MIDI-editor: välj kanal 10 i kanalväljaren nederst innan du ritar, eller markera noterna och ändra kanal under *Note properties*. Använd gärna *View → Mode: Named notes* och mappningen nedan (C2 bastrumma, D2 virvel, F♯2 hi-hat …).
4. Accent ges av anslaget (hårdare slag = starkare).

**Eller låt Stampverket själv spela:** stäng av MIDI-spel, skriv trummönstret i Kroppsmaskinen och låt det följa Reapers transport, medan basen spelas från Reapers MIDI. Mönsterbyten automatiseras med *Stampverket mönster* (A5 ovan).

### B4. Hugget för stötar och leads

1. Välj ett ljud för Hugget (presets eller **LJUD**). Läget (*Orkesterstöt*, *Körstöt*, *Synklead* …) och ackordet väljs i KANAL-vyn.
2. MIDI-spel på för Hugget, toner på **kanal 3**.
3. Med ett ackord i ackordminnet (t.ex. *Moll*) räcker en ton per stöt. Utan ackord spelar Hugget upp till åtta toner samtidigt.

### B5. En instans per ljud eller en för allt?

- **En instans för allt:** en Kroppsmaskinen, MIDI på kanal 1, 2, 3 och 10 från en eller flera MIDI-regioner på samma spår (eller skickade till spåret, se nedan). Allt delar effekter (Grindrummet, Ekot, Klämman) – det låter sammanhållet, som en maskin.
- **En instans per ljud:** ett spår med Kroppsmaskinen för basen, ett för trummorna osv. Varje spår får egna effekter i Reaper och egen fader. Mer flexibelt i mixen, mer processorkraft.

Skicka MIDI från flera spår till en instans: på MIDI-spåret, öppna **Route** → *Add new send* till instrumentspåret, sätt *Audio* till *None* och *MIDI* till rätt kanal (t.ex. *All → 10*).

### B6. Blanda: sekvensern och Reaper tillsammans

Ett bra arbetssätt för EBM: låt Kroppsmaskinens sekvenser spela basen och trummorna (mönster eller låtläget), och spela stötarna i Hugget från Reaper (MIDI-spel på Hugget). Eller tvärtom: trummor från sekvensern och basen inspelad från ett keyboard.

---

## Mixa, separata spår och rendering

Kroppsmaskinen har en stereoutgång. För att mixa kanalerna var för sig i Reaper:

- **Rendera stems:** sätt Solo på en kanal i MIXER, rendera (*File → Render*, *Source: Master mix* eller det valda spåret), byt kanal, rendera igen. Lägg tillbaka filerna på egna spår.
- **Flera instanser:** kopiera spåret (alla instanser följer samma transport och spelar i takt), och sätt Solo på en kanal i varje kopia. Kom ihåg att alla instanser räknar hela maskinen.
- **Effekter:** Kroppsmaskinens egna effekter (Grindrummet, Ekot, Svajet, Klämman, Kassetten) ligger efter mixen. Vill du ha egna effekter i Reaper på en kanal: sätt kanalens sändningar till 0, eller använd en egen instans för den kanalen.

**Rendera låten:** *File → Render*, välj *Entire project* (eller *Time selection*), *WAV 24 bit* eller *MP3*. Räkna med en takt extra i slutet för ekot och rummet.

**Spara processorkraft:** *Freeze* spåret (högerklick på spåret → *Freeze track*) när delen är klar.

---

## Referens: MIDI

**Kanaler:** 1 Kolvbasen, 2 Kolvbas 2, 3 Hugget, 10 Stampverket. **CC 1** (modhjul) och **aftertouch** går till modulationen (källorna *Modhjul* och *Aftertouch*); anslaget till källan *Anslag*.

**Trummor på kanal 10 (GM):**

| Spår | Ton | Spår | Ton |
| --- | --- | --- | --- |
| Bastrumma | 36 C2 | Cymbal | 49 C♯3 |
| Virvel | 38 D2 | Tom låg | 41 F2 |
| Virvel 2 | 40 E2 | Tom hög | 48 C3 |
| Handklapp | 39 D♯2 | Metall | 56 G♯3 |
| Hi-hat | 42 F♯2 | Smäll | 54 F♯3 |
| Öppen hi-hat | 46 A♯2 | Kubjällra / kantslag | 37 C♯2 |

(Reaper numrerar ibland oktaverna annorlunda: C2 kan heta C3 i Reapers MIDI-editor. Ton 36 är alltid bastrumman.)

**Datorns tangentbord** i pluginet: A W S E D F T G Y H U J K O L P, Z/X för oktav. I Reaper behöver pluginfönstret få tangenterna: i FX-fönstret, välj *Send all keyboard input to plug-in* (under *Options* eller i högerklicksmenyn). Annars använder Reaper tangenterna för sina kortkommandon.

---

## Felsökning

| Problem | Orsak och lösning |
| --- | --- |
| Inget ljud när Reaper spelar | Kanalerna är tysta, låten är på *Stanna* och slut, eller alla kanaler står på MIDI-spel utan MIDI. Kontrollera MIXER. |
| MIDI-tonerna låter inte, utan basgången flyttas | Kanalen står inte på MIDI-spel, så tonerna transponerar sekvensen. Slå på **MIDI-spel** för kanalen. |
| Trummorna spelar dubbelt | Både Stampverkets mönster och MIDI på kanal 10 spelar. Slå på MIDI-spel för Stampverket. |
| Trumnoterna låter inte | Noterna ligger på kanal 1. Ändra till kanal 10. |
| Låten börjar på fel rad | Låten räknas från projektets början; takt 1 = rad 1. Lägg en tyst rad först, eller flytta allt annat. |
| Tempot är fel | I Reaper gäller projektets tempo, inte tempodisplayen. |
| Pluginet hittas inte | Kopiera `.vst3`/`.clap` till rätt mapp och gör *Re-scan* i Reapers inställningar. |
| `Fabriksgolv.RPP` säger att pluginet saknas | Kör `python3 tools/reaper/gor_reaper_projekt.py build/KMRender_artefacts/Release/KMRender Reaper` på Macen efter att Reaper har hittat pluginet; skriptet läser Reapers plugin-id. |
