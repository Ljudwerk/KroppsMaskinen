// KMRender – renderingstest och lyssningsfiler, utan DAW.
//
//   KMRender [katalog]
//   KMRender --state "<preset>" fil.xml [param=värde ...] [monster:<kb|kb2|sv|hu>:<nr><A|B>=<text> ...]
//   KMRender --parametrar fil.txt
//   KMRender --projekt spec.json ut.wav
//   KMRender --kanaler katalog [del av presetnamn]
//
// --state skriver pluginets läge (samma XML som DAW:en sparar) efter att ha laddat presetet och ändrat
// parametrar och mönster; används av tools/reaper/gor_reaper_projekt.py. --parametrar skriver alla
// parametrar i den ordning DAW:en ser dem (index, ID och antal steg). --projekt spelar en låt utan DAW:
// en låtsad DAW-transport, automation och MIDI enligt spec.json, och skriver resultatet som WAV.
// --kanaler spelar varje fabrikspresets mönster 1–4 i fyra takter, mixen och varje kanal solo, skriver WAV
// och nivåerna (för att lyssna på och jämföra fabriksbandet).
//
// Kör testerna nedan och skriver WAV-filer till katalogen (standard: renders/). Avslutar med kod 1 om något
// test misslyckas, så att CI blir rött.
//
//    1. Mönsterformatet: alla fabrikspresets överlever toString → fromString, raderna är hela takter,
//       och ett läge i format 1 (fas 0) läses in som mönster 1A.
//    2. Varje ljud för sig – Kolvbasen i båda filtren, med synk, Kolvbas 2, och alla tolv trummor i alla
//       fyra kit: inga NaN, topp högst 0 dBFS, ingen DC, tyst efter släpp. Varje kit skrivs som WAV.
//    3. Taktverket: bastrumman slår exakt på sina steg (±4 samples) vid 125 BPM.
//       Dessutom tio minuter i 128 BPM med egen klocka och med DAW-transport: tempot glider inte.
//    4. Livebyte: ett nytt mönster tar över på nästa takt, inte mitt i.
//    5. Tyst och solo: solo på Stampverket låter som Kolvbasen tyst.
//    6. Följ Kolvbasen: Kolvbas 2 med samma ljud och ingen förskjutning låter exakt som Kolvbasen.
//    7. Drivet: varje läge håller nivån inom ±4 dB av Av.
//    8. Varje fabrikspreset, åtta takter: inga NaN, topp högst 0 dBFS, hörbar nivå. Skrivs som WAV.
//    9. Ytterlägen: 40 slumpade inställningar av alla rattar: inga NaN, topp högst 0 dBFS.
//   10. Som i den fristående appen: genom JUCE:s AudioProcessorPlayer och ett låtsasljudkort, i flera
//       samplingsfrekvenser och blockstorlekar – tempot ska vara detsamma.
//   11. Hugget: alla sju lägen med ackord: inga NaN, topp högst 0 dBFS, ingen DC, tyst efter släpp.
//   12. Grindrummet: rummet hörs medan virveln klingar och grinden stänger efter hålltiden.
//   13. Ekot: ekot kommer exakt en fjärdedel senare vid 120 BPM.
//   14. Klämman: pumpen trycker ned nivån.
//   15. Varvtalet: 33 varv sänker tempo och tonhöjd med 135/100, i båda lägena.
//   16. Spänningen: full spänning håller nivån inom ±4 dB och toppen under 0 dBFS.
//   17. Modulationen: en LFO på Kolvbasens filter ändrar klangen.
//   18. Kassetten: slitage 0 på SP ändrar ingenting; fullt slitage på EP håller nivån och toppen.
//   19. Låtläget: textformatet, raderna byter mönster och tystnad takt för takt, Loopa och Stanna.
//   20. Demolåtarna: varje fabrikspreset med låt spelas hela vägen (alla takter och en till): inga NaN, topp
//       högst 0 dBFS, rimlig nivå, och varje rad låter om inte alla kanalerna är tysta. Skrivs som demo_*.wav.
//   21. Ljudbanken: ett ljud byter bara kanalens klang (mönster, tempo och mix står kvar), alla fabriksljud
//       låter rent, och Lås mönstren laddar ett preset utan att mönster och tempo byts.
//   22. MIDI-spel: en kanal som spelas från MIDI tystnar i sekvensern och spelas av MIDI-tonerna medan sekvensern går.
//   23. Stämningen: tabellerna, Scala-filerna och att Kolvbasen och Hugget hamnar rätt (Rast, en kvartston låg).
//   24. Gjuteriet: modernas frekvenser, stämning, klang, kedjans anslag och fjädern.
//   25. Stämning i cent: Kolvbasen och Hugget i bas, mellanläge och diskant (±3 cent).
//   26. Klick: tyst och solo mitt i en ton, röststöld i Hugget, upprepad ton och programbyte medan sekvensern går.
//   27. CPU: hela maskinen med allt igång under 25 % av en kärna.
//
// Grind 0-loopen hamnar i renders/grind0_kolv125.wav, grind 1-looparna i renders/grind1_*.wav.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "presets/FactoryPresets.h"

void testaLangTakt();   // efter LatsadDaw längre ned

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 512;
int fel = 0;

int antalKontroller = 0;
void kontroll (bool ok, const juce::String& vad)
{
    std::cout << (ok ? "  ok   " : "  FEL  ") << vad << "\n";
    ++antalKontroller;
    if (! ok) ++fel;
}

struct Matning
{
    bool finit = true;
    float topp = 0.0f;
    double medel = 0.0, rms = 0.0;
};

Matning mat (const juce::AudioBuffer<float>& b, int fran = 0, int till = -1)
{
    if (till < 0) till = b.getNumSamples();
    Matning m;
    double sum = 0.0, kv = 0.0;
    for (int i = fran; i < till; ++i)
    {
        const float x = b.getSample (0, i);
        if (! std::isfinite (x)) m.finit = false;
        m.topp = juce::jmax (m.topp, std::abs (x));
        sum += x; kv += (double) x * x;
    }
    const int n = juce::jmax (1, till - fran);
    m.medel = sum / n;
    m.rms = std::sqrt (kv / n);
    return m;
}

void satt (KroppsmaskinenProcessor& p, const juce::String& pid, float varde)
{
    auto* param = p.state.getParameter (pid);
    jassert (param != nullptr);
    param->setValueNotifyingHost (param->convertTo0to1 (varde));
}

std::unique_ptr<KroppsmaskinenProcessor> nyProcessor()
{
    auto p = std::make_unique<KroppsmaskinenProcessor>();
    p->forceraEgenKlocka = true;
    p->setPlayConfigDetails (0, 2, sr, block);
    p->prepareToPlay (sr, block);
    return p;
}

// Renderar 'sekunder' sekunder. 'midi' anropas per block och får lägga till händelser (offset inom blocket).
juce::AudioBuffer<float> rendera (KroppsmaskinenProcessor& p, double sekunder,
                                  std::function<void (juce::MidiBuffer&, int start, int n)> midi = {})
{
    const int total = (int) (sekunder * sr);
    juce::AudioBuffer<float> ut (2, total), buf (2, block);
    juce::MidiBuffer m;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        buf.setSize (2, n, false, false, true);
        m.clear();
        if (midi) midi (m, pos, n);
        p.processBlock (buf, m);
        for (int c = 0; c < 2; ++c) ut.copyFrom (c, pos, buf, c, 0, n);
    }
    return ut;
}

void skrivWav (const juce::AudioBuffer<float>& b, const juce::File& fil)
{
    fil.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream (fil.createOutputStream());
    if (stream == nullptr) { kontroll (false, "kunde inte skriva " + fil.getFullPathName()); return; }
    if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0)))
    {
        stream.release();
        w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

juce::String filnamn (juce::String namn)
{
    return namn.toLowerCase().replace (" ", "_").replace (juce::String::fromUTF8 ("å"), "a")
               .replace (juce::String::fromUTF8 ("ä"), "a").replace (juce::String::fromUTF8 ("ö"), "o");
}

// ---------------------------------------------------------------------------------------------

double rms (const juce::AudioBuffer<float>& b) { return mat (b).rms; }
float dB (double v) { return juce::Decibels::gainToDecibels ((float) v, -120.0f); }
double takter (KroppsmaskinenProcessor& p, int n) { return n * 4 * 60.0 / (double) *p.state.getRawParameterValue (km::params::id::tempo); }

// Som takter(), men med Varvtalet: 33 varv sänker tempot i båda lägena (egen klocka).
double verkligaTakter (KroppsmaskinenProcessor& p, int n)
{
    const auto varv = (km::Varv) (int) *p.state.getRawParameterValue (km::params::id::varvtal);
    return takter (p, n) / km::varvFaktor (varv);
}

// Ansatser: första samplet över 0,004 efter minst 50 ms nära tystnad, om ljudet når 0,02 inom 2 ms.
std::vector<int> ansatser (const juce::AudioBuffer<float>& ut, double takt = sr)
{
    std::vector<int> a;
    int tyst = 100000;
    const int n = ut.getNumSamples();
    for (int i = 0; i < n; ++i)
    {
        const float v = std::abs (ut.getSample (0, i));
        if (v >= 0.004f && tyst > (int) (0.05 * takt))
        {
            bool stark = false;
            for (int j = i; j < juce::jmin (n, i + (int) (0.002 * takt)) && ! stark; ++j) stark = std::abs (ut.getSample (0, j)) > 0.02f;
            if (stark) a.push_back (i);
        }
        tyst = v < 0.004f ? tyst + 1 : 0;
    }
    return a;
}

void testaMonster()
{
    std::cout << "1. Mönsterformatet\n";
    auto p = nyProcessor();
    for (int i = 0; i < p->presets.getNumFactory(); ++i)
    {
        p->presets.load (i);
        bool lika = true;
        for (int m = 0; m < km::antalMonster; ++m)
            for (int v = 0; v < 2; ++v)
            {
                km::TrumMonster t2; km::TonMonster b2, c2;
                t2.fromString (p->banker.trummor.get (m, v).toString());
                b2.fromString (p->banker.kolvbasen.get (m, v).toString());
                c2.fromString (p->banker.kolvbas2.get (m, v).toString());
                km::TonMonster h2;
                h2.fromString (p->banker.hugget.get (m, v).toString());
                for (int st = 0; st < km::maxSteg; ++st) lika = lika && h2.get (st) == p->banker.hugget.get (m, v).get (st);
                for (int s = 0; s < km::maxSteg; ++s)
                {
                    lika = lika && b2.get (s) == p->banker.kolvbasen.get (m, v).get (s) && c2.get (s) == p->banker.kolvbas2.get (m, v).get (s);
                    for (int spr = 0; spr < km::antalSpar; ++spr)
                        lika = lika && t2.get (spr, s) == p->banker.trummor.get (m, v).get (spr, s);
                }
            }
        kontroll (lika, p->presets.getEntries()[(size_t) i].name + juce::String::fromUTF8 (": alla banker tål text fram och tillbaka"));
    }

    // Fabriksbandets rader måste vara hela takter, annars glider mönstret när det upprepas.
    bool hela = true;
    juce::String felrad;
    for (const auto& f : km::presets::factoryPresets())
    {
        for (const auto& t : f.trummor)
            for (const auto& [spar, rad] : t.rader)
                if (const auto n = km::TrumMonster::lasRad (juce::String::fromUTF8 (rad)).size(); n % 16 != 0)
                { hela = false; felrad << f.name << "/" << juce::String::fromUTF8 (km::trumNamn (spar)) << " (" << (int) n << ") "; }
        for (const auto& t : f.toner)
            if (const auto n = juce::StringArray::fromTokens (juce::String::fromUTF8 (t.text), " ", "").size(); n % 16 != 0)
            { hela = false; felrad << f.name << "/" << km::kanalNamn (t.kanal) << " (" << n << ") "; }
    }
    kontroll (hela, juce::String::fromUTF8 ("fabriksbandets rader är hela takter ") + felrad);

    km::TonMonster b;
    b.fromString ("-12d 12u 3a~/0 _5/7 .");
    km::TonMonster b2;
    b2.fromString (b.toString (5));
    bool lika = true;
    for (int s = 0; s < 5; ++s) lika = lika && b.get (s) == b2.get (s);
    kontroll (lika && b.get (0).tonhojd() == -24 && b.get (1).tonhojd() == 24 && b.get (2).glid && b.get (3).slag == 0,
              juce::String::fromUTF8 ("tonsteg med oktav, glid, längd och tyst steg"));

    // Ett DAW-läge från fas 0 (format 1).
    juce::ValueTree gammal ("Kroppsmaskinen");
    gammal.setProperty ("formatVersion", 1, nullptr);
    juce::ValueTree m ("Monster");
    m.setProperty ("trummor", "1000100010001000//0000100000001000", nullptr);
    m.setProperty ("kolvbasen", "0a 0 0u 0", nullptr);
    gammal.appendChild (m, nullptr);
    km::Banker banker;
    const bool last = km::PresetManager::readPatterns (gammal, banker);
    kontroll (last && banker.trummor.get (0, 0).get (0, 0).slag == 1 && banker.trummor.get (0, 0).get (2, 4).slag == 1
                   && banker.kolvbasen.get (0, 0).get (2).oktav == 1 && (int) gammal["formatVersion"] == km::PresetManager::formatVersion,
              juce::String::fromUTF8 ("läge i format 1 blir mönster 1A"));
}

void testaLjud (const juce::File& katalog)
{
    std::cout << "2. Varje ljud\n";
    struct Fall { juce::String namn; int kanal, ton; std::vector<std::pair<juce::String, float>> varden; };
    using namespace km::params;
    std::vector<Fall> fall {
        { "Kolvbasen 24 dB", 1, 33, {} },
        { "Kolvbasen 12 dB", 1, 33, { { id::kolv (0, "filterTyp"), 1.0f } } },
        { "Kolvbasen synk och brus", 1, 45, { { id::kolv (0, "osc2Niva"), 1.0f }, { id::kolv (0, "synk"), 1.0f },
                                             { id::kolv (0, "osc2Halvtoner"), 19.0f }, { id::kolv (0, "brus"), 0.5f } } },
        { juce::String::fromUTF8 ("Kolvbasen hög"), 1, 69, {} },
        { "Kolvbas 2", 2, 45, {} },
    };
    for (int kit = 0; kit < km::antalKit; ++kit)
        for (int t = 0; t < km::antalSpar; ++t)
            fall.push_back ({ juce::String::fromUTF8 (km::trumNamn ((km::Trumma) t)) + " kit " + juce::String (kit + 1), 10,
                              km::Stampverket::notForSpar (t), { { id::kit, (float) kit } } });

    int ok = 0;
    for (const auto& f : fall)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::spela, 0.0f);
        satt (*p, id::kolv (0, "resonans"), 0.9f);
        for (const auto& [pid, v] : f.varden) satt (*p, pid, v);
        auto ut = rendera (*p, 3.5, [&f] (juce::MidiBuffer& m, int pos, int n)
        {
            const int pa = 0, av = (int) (0.6 * sr);
            if (pa >= pos && pa < pos + n) m.addEvent (juce::MidiMessage::noteOn (f.kanal, f.ton, 1.0f), pa - pos);
            if (av >= pos && av < pos + n) m.addEvent (juce::MidiMessage::noteOff (f.kanal, f.ton), av - pos);
        });
        const auto hela = mat (ut);
        const auto slut = mat (ut, ut.getNumSamples() - (int) (0.2 * sr));
        const bool bra = hela.finit && hela.topp <= 1.0f && hela.topp > 0.02f && std::abs (hela.medel) < 0.01 && slut.topp < 1.0e-3f;
        if (bra) ++ok;
        else kontroll (false, f.namn + ": topp " + juce::String (hela.topp, 3) + ", DC " + juce::String (hela.medel, 5)
                                     + ", slut " + juce::String (slut.topp, 6) + (hela.finit ? "" : ", NaN"));
        if (f.kanal != 10) skrivWav (ut, katalog.getChildFile ("ljud_" + filnamn (f.namn) + ".wav"));
    }
    kontroll (ok == (int) fall.size(), juce::String (ok) + " av " + juce::String ((int) fall.size())
                                       + juce::String::fromUTF8 (" ljud: inga NaN, topp ≤ 0 dBFS, ingen DC, tyst efter släpp"));

    // Ett WAV per kit: alla tolv ljud efter varandra, ett varje halvsekund.
    for (int kit = 0; kit < km::antalKit; ++kit)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kit, (float) kit);
        auto ut = rendera (*p, 6.5, [] (juce::MidiBuffer& m, int pos, int n)
        {
            for (int t = 0; t < km::antalSpar; ++t)
                if (const int at = (int) (t * 0.5 * sr); at >= pos && at < pos + n)
                    m.addEvent (juce::MidiMessage::noteOn (10, km::Stampverket::notForSpar (t), 1.0f), at - pos);
        });
        static const char* kitNamn[] = { "attaatta", "niononio", "digital", "fabrik", "gjuteriet" };
        skrivWav (ut, katalog.getChildFile (juce::String ("kit_") + kitNamn[kit] + ".wav"));
    }
}

void testaTakt()
{
    std::cout << "3. Taktverket\n";
    auto p = nyProcessor();
    p->presets.loadByName ("Init");
    p->banker.kolvbasen.rensa();
    satt (*p, km::params::id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);   // kort bastrumma, så att varje slag börjar i tystnad
    satt (*p, km::params::id::kit, (float) km::Kit::Niononio);
    satt (*p, km::params::id::tempo, 125.0f);
    satt (*p, km::params::id::spela, 1.0f);
    const auto ut = rendera (*p, 4.0);
    const auto a = ansatser (ut);
    const double fjardedel = sr * 60.0 / 125.0;
    int vart = 0;
    for (size_t i = 0; i < a.size(); ++i) vart = juce::jmax (vart, (int) std::abs (a[i] - (double) i * fjardedel));
    kontroll (a.size() >= 8 && vart <= 4, juce::String ((int) a.size()) + juce::String::fromUTF8 (" slag, största avvikelse ") + juce::String (vart) + " samples");
    testaLangTakt();
}

void testaLivebyte()
{
    std::cout << "4. Livebyte\n";
    using namespace km::params;
    auto p = nyProcessor();
    p->presets.loadByName ("Init");
    p->banker.kolvbasen.rensa();
    p->banker.trummor.get (1, 0).fromString ("1000000000000000");   // mönster 2: bara ettan
    satt (*p, id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);
    satt (*p, id::kit, (float) km::Kit::Niononio);
    satt (*p, id::tempo, 125.0f);
    satt (*p, id::spela, 1.0f);
    const double takt = 4 * 60.0 / 125.0;
    // Byt till mönster 2 en bit in i första takten: det ska höras först i takt 2.
    auto ut = rendera (*p, 2.0 * takt, [&p, takt] (juce::MidiBuffer&, int pos, int n)
    {
        const int byte = (int) (0.3 * takt * sr);
        if (byte >= pos && byte < pos + n) satt (*p, id::kanal ((int) km::Kanal::Stampverket, "monster"), 2.0f);
    });
    int forsta = 0, andra = 0;
    for (int a : ansatser (ut)) (a < (int) (takt * sr) - 100 ? forsta : andra)++;
    kontroll (forsta == 4 && andra == 1, juce::String::fromUTF8 ("takt 1: ") + juce::String (forsta) + juce::String::fromUTF8 (" slag (väntat 4), takt 2: ")
                                           + juce::String (andra) + juce::String::fromUTF8 (" slag (väntat 1)"));
}

void testaTystSolo()
{
    std::cout << "5. Tyst och solo\n";
    using namespace km::params;
    auto rendera1 = [] (const juce::String& pid)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::spela, 1.0f);
        satt (*p, pid, 1.0f);
        return rendera (*p, 2.0);
    };
    const auto solo = rendera1 (id::kanal ((int) km::Kanal::Stampverket, "solo"));
    const auto tyst = rendera1 (id::kanal ((int) km::Kanal::Kolvbasen, "tyst"));
    const float skillnad = std::abs (dB (rms (solo)) - dB (rms (tyst)));
    kontroll (skillnad < 0.2f, juce::String::fromUTF8 ("solo Stampverket = Kolvbasen tyst (skillnad ") + juce::String (skillnad, 2) + " dB)");
}

void testaFolj()
{
    std::cout << juce::String::fromUTF8 ("6. Följ Kolvbasen\n");
    using namespace km::params;
    auto bas = [] (bool tvaan)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        // Ge Kolvbas 2 exakt Kolvbasens ljud.
        for (auto* param : p->getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param); rp != nullptr && rp->getParameterID().startsWith ("km_kb_"))
                if (auto* motsv = p->state.getParameter (rp->getParameterID().replace ("km_kb_", "km_kb2_")); motsv != nullptr
                        && ! rp->getParameterID().contains ("monster") && ! rp->getParameterID().contains ("variant"))
                    motsv->setValueNotifyingHost (rp->getValue());
        satt (*p, id::folj, 1.0f);
        satt (*p, id::transponering, 0.0f);
        satt (*p, id::forskjutning, 0.0f);
        satt (*p, id::kanal ((int) km::Kanal::Stampverket, "tyst"), 1.0f);
        satt (*p, id::kanal (tvaan ? 0 : 1, "tyst"), 1.0f);
        satt (*p, id::spela, 1.0f);
        return rendera (*p, 2.0);
    };
    const auto ett = bas (false), tva = bas (true);
    float storst = 0.0f;
    for (int i = 0; i < ett.getNumSamples(); ++i) storst = juce::jmax (storst, std::abs (ett.getSample (0, i) - tva.getSample (0, i)));
    kontroll (storst < 1.0e-4f && rms (tva) > 0.01, juce::String::fromUTF8 ("Kolvbas 2 som följer = Kolvbasen (största skillnad ") + juce::String (storst, 6) + ")");
}

void testaDrivet()
{
    std::cout << "7. Drivet\n";
    using namespace km::params;
    static const char* lagen[] = { "Av", "Rör", "Overdrive", "Bitkross" };
    for (int k = 0; k < km::antalKanaler; ++k)
    {
        double av = 0.0;
        juce::String rad = juce::String::fromUTF8 (km::kanalNamn ((km::Kanal) k)) + ":";
        bool ok = true;
        for (int lage = 0; lage < 4; ++lage)
        {
            auto p = nyProcessor();
            p->presets.loadByName ("EBM 128");
            satt (*p, id::folj, 0.0f);
            p->banker.kolvbas2.get (0, 0).fromString ("0a . 0 0 0a . 0 0 0a . 0 0 -2a . 0 3");
            p->banker.kolvbas2.get (0, 1).fromString ("0a . 0 0 0a . 0 0 0a . 0 0 -2a . 0 3");
            for (int j = 0; j < km::antalKanaler; ++j) if (j != k) satt (*p, id::kanal (j, "tyst"), 1.0f);
            satt (*p, id::kanal (k, "drivetLage"), (float) lage);
            satt (*p, id::kanal (k, "drivetMangd"), 0.5f);
            satt (*p, id::master, 0.4f);   // under begränsaren
            satt (*p, id::spela, 1.0f);
            const double r = rms (rendera (*p, 2.0));
            if (lage == 0) av = r;
            const float d = dB (r) - dB (av);
            ok = ok && std::abs (d) <= 4.0f;
            rad << " " << juce::String::fromUTF8 (lagen[lage]) << " " << (d >= 0 ? "+" : "") << juce::String (d, 1) << " dB";
        }
        kontroll (ok, rad);
    }
}

void testaPresets (const juce::File& katalog)
{
    std::cout << "8. Fabriksbandet\n";
    auto mall = nyProcessor();
    for (int i = 0; i < mall->presets.getNumFactory(); ++i)
    {
        auto p = nyProcessor();
        p->presets.load (i);
        satt (*p, km::params::id::spela, 1.0f);
        const auto namn = p->presets.getEntries()[(size_t) i].name;
        auto ut = rendera (*p, takter (*p, 8) + 0.5);
        const auto m = mat (ut);
        kontroll (m.finit && m.topp <= 1.0f && m.rms > 0.02,
                  namn + ": topp " + juce::String (m.topp, 3) + ", RMS " + juce::String (dB (m.rms), 1) + " dB");
        if (km::presets::factoryPresets()[(size_t) i].laten == nullptr)   // låtarna skrivs hela i test 20
            skrivWav (ut, katalog.getChildFile (filnamn (namn) + ".wav"));
        if (namn == "Kolv 125") skrivWav (ut, katalog.getChildFile ("grind0_kolv125.wav"));
        if (namn == "EBM 128" || namn == "Marsch 120" || namn == "New Beat 112")
            skrivWav (ut, katalog.getChildFile ("grind1_" + filnamn (namn) + ".wav"));
    }

    // Grind 0-loopen som separata spår.
    using namespace km::params;
    for (auto [stam, tyst] : { std::pair { "bas", (int) km::Kanal::Stampverket }, std::pair { "trummor", (int) km::Kanal::Kolvbasen } })
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::spela, 1.0f);
        satt (*p, id::kanal (tyst, "tyst"), 1.0f);
        auto ut = rendera (*p, takter (*p, 8) + 0.5);
        const auto m = mat (ut);
        std::cout << "       grind 0, bara " << stam << ": topp " << juce::String (m.topp, 3) << ", RMS " << juce::String (dB (m.rms), 1) << " dB\n";
        skrivWav (ut, katalog.getChildFile (juce::String ("grind0_kolv125_") + stam + ".wav"));
    }
}

void testaYtterlagen()
{
    std::cout << juce::String::fromUTF8 ("9. Ytterlägen\n");
    juce::Random r (87);
    bool ok = true;
    float vartTopp = 0.0f;
    for (int i = 0; i < 40; ++i)
    {
        auto p = nyProcessor();
        p->presets.load (i % p->presets.getNumFactory());
        for (auto* param : p->getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param);
            if (rp == nullptr || rp->getParameterID() == km::params::id::spela || rp->getParameterID().endsWith ("tyst")
                || rp->getParameterID().endsWith ("solo") || rp->getParameterID().endsWith ("monster")) continue;
            const float u = r.nextFloat();
            rp->setValueNotifyingHost (u < 0.3f ? 0.0f : (u > 0.7f ? 1.0f : r.nextFloat()));
        }
        satt (*p, km::params::id::spela, 1.0f);
        satt (*p, km::params::id::tempo, 200.0f);
        const auto m = mat (rendera (*p, 1.5));
        ok = ok && m.finit && m.topp <= 1.0f;
        vartTopp = juce::jmax (vartTopp, m.topp);
    }
    kontroll (ok, juce::String::fromUTF8 ("40 slumpade inställningar, största topp ") + juce::String (vartTopp, 3));
}
// Ett låtsasljudkort: bara det som AudioProcessorPlayer frågar efter.
class Lattsaskort final : public juce::AudioIODevice
{
public:
    Lattsaskort (double r, int b, int in) : AudioIODevice ("Lattsaskort", "Lattsaskort"), rate (r), block (b), ingangar (in) {}
    juce::StringArray getOutputChannelNames() override { return { "V", "H" }; }
    juce::StringArray getInputChannelNames() override { return ingangar > 0 ? juce::StringArray { "1", "2" } : juce::StringArray(); }
    juce::Array<double> getAvailableSampleRates() override { return { rate }; }
    juce::Array<int> getAvailableBufferSizes() override { return { block }; }
    int getDefaultBufferSize() override { return block; }
    juce::String open (const juce::BigInteger&, const juce::BigInteger&, double, int) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start (juce::AudioIODeviceCallback*) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return block; }
    double getCurrentSampleRate() override { return rate; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return 3; }
    juce::BigInteger getActiveInputChannels() const override { return ingangar > 0 ? 3 : 0; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

private:
    double rate;
    int block, ingangar;
};

void testaFristaende()
{
    std::cout << juce::String::fromUTF8 ("10. Som i den fristående appen\n");
    using namespace km::params;
    for (auto [rate, blk, in] : { std::tuple { 44100.0, 512, 0 }, std::tuple { 48000.0, 64, 2 }, std::tuple { 96000.0, 441, 2 } })
    {
        KroppsmaskinenProcessor p;
        p.forceraEgenKlocka = true;   // som wrapperType_Standalone
        p.presets.loadByName ("Init");
        p.banker.kolvbasen.rensa();
        satt (p, id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);
        satt (p, id::kit, (float) km::Kit::Niononio);
        satt (p, id::tempo, 125.0f);
        satt (p, id::spela, 1.0f);

        juce::AudioProcessorPlayer spelare;
        Lattsaskort kort (rate, blk, in);
        spelare.setProcessor (&p);
        spelare.audioDeviceAboutToStart (&kort);

        const int total = (int) (4.0 * rate);
        juce::AudioBuffer<float> ut (2, total), ink (2, blk), utblock (2, blk);
        ink.clear();
        for (int pos = 0; pos < total; pos += blk)
        {
            const int n = juce::jmin (blk, total - pos);
            utblock.clear();
            spelare.audioDeviceIOCallbackWithContext (ink.getArrayOfReadPointers(), in, utblock.getArrayOfWritePointers(), 2, n, {});
            for (int c = 0; c < 2; ++c) ut.copyFrom (c, pos, utblock, c, 0, n);
        }
        spelare.audioDeviceStopped();
        spelare.setProcessor (nullptr);

        const auto a = ansatser (ut, rate);
        const double fjardedel = rate * 60.0 / 125.0;
        int vart = 0;
        for (size_t i = 0; i < a.size(); ++i) vart = juce::jmax (vart, (int) std::abs (a[i] - (double) i * fjardedel));
        kontroll (a.size() >= 8 && a.size() <= 9 && vart <= 4,
                  juce::String (rate / 1000.0, 1) + " kHz, block " + juce::String (blk) + ": " + juce::String ((int) a.size())
                  + juce::String::fromUTF8 (" slag på 4 s (väntat 9), största avvikelse ") + juce::String (vart) + " sampel");
    }
}
// Grundfrekvensen ungefär, genom att räkna uppåtgående nollgenomgångar (med hysteres).
double grundton (const juce::AudioBuffer<float>& b, int fran, int till, double takt = sr)
{
    int antal = 0, forsta = -1, sista = -1;
    bool under = true;
    for (int i = fran; i < till; ++i)
    {
        const float v = b.getSample (0, i);
        if (under && v > 0.02f) { under = false; ++antal; if (forsta < 0) forsta = i; sista = i; }
        else if (! under && v < -0.02f) under = true;
    }
    return antal > 1 ? (antal - 1) * takt / (sista - forsta) : 0.0;
}

// En ton på MIDI-kanal 'kanal' i 'langd' sekunder, och sedan tystnad.
std::function<void (juce::MidiBuffer&, int, int)> ton (int kanal, int notnr, double langd, double vid = 0.0)
{
    return [=] (juce::MidiBuffer& m, int pos, int n)
    {
        const int pa = (int) (vid * sr), av = (int) ((vid + langd) * sr);
        if (pa >= pos && pa < pos + n) m.addEvent (juce::MidiMessage::noteOn (kanal, notnr, 1.0f), pa - pos);
        if (av >= pos && av < pos + n) m.addEvent (juce::MidiMessage::noteOff (kanal, notnr), av - pos);
    };
}

void testaHugget (const juce::File& katalog)
{
    std::cout << "11. Hugget\n";
    using namespace km::params;
    static const char* lagen[] = { "Orkesterstöt", "Blåsstöt", "Körstöt", "Klockstöt", "Synklead", "Pulslead", "FM-lead" };
    for (int l = 0; l < km::antalHuggLagen; ++l)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::hu ("lage"), (float) l);
        satt (*p, id::hu ("hall"), 0.5f);
        auto ut = rendera (*p, 3.0, ton (3, 57, 0.6));
        const auto hela = mat (ut);
        const auto slut = mat (ut, ut.getNumSamples() - (int) (0.2 * sr));
        const auto namn = juce::String::fromUTF8 (lagen[l]);
        kontroll (hela.finit && hela.topp <= 1.0f && hela.topp > 0.05f && std::abs (hela.medel) < 0.01 && slut.topp < 1.0e-3f,
                  namn + ": topp " + juce::String (hela.topp, 3) + ", DC " + juce::String (hela.medel, 5) + ", slut " + juce::String (slut.topp, 6));
        skrivWav (ut, katalog.getChildFile ("hugget_" + filnamn (namn) + ".wav"));
    }
}

void testaGrindrummet()
{
    std::cout << "12. Grindrummet\n";
    using namespace km::params;
    auto virvel = [] (float sand)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal ((int) km::Kanal::Stampverket, "sandA"), sand);
        satt (*p, id::fx ("gr", "hallTid"), 0.2f);
        satt (*p, id::fx ("gr", "niva"), 1.0f);
        satt (*p, id::trumma ((int) km::Trumma::Virvel, "klang"), 0.2f);
        return rendera (*p, 1.5, ton (10, 38, 0.05));
    };
    const auto torr = virvel (0.0f), vat = virvel (1.0f);
    const double rumTidigt = mat (vat, (int) (0.05 * sr), (int) (0.25 * sr)).rms - mat (torr, (int) (0.05 * sr), (int) (0.25 * sr)).rms;
    const auto sent = mat (vat, (int) (0.9 * sr), (int) (1.5 * sr));
    kontroll (rumTidigt > 0.005 && sent.topp < 1.0e-3f, juce::String::fromUTF8 ("rummet hörs (+") + juce::String (rumTidigt, 4)
                                                       + juce::String::fromUTF8 (" RMS) och tystnar efter grinden (") + juce::String (sent.topp, 6) + ")");
}

void testaEkot()
{
    std::cout << "13. Ekot\n";
    using namespace km::params;
    auto p = nyProcessor();
    p->presets.loadByName ("Init");
    satt (*p, id::tempo, 120.0f);
    satt (*p, id::kantslag, 1.0f);
    satt (*p, id::kanal ((int) km::Kanal::Stampverket, "sandB"), 1.0f);
    satt (*p, id::fx ("eko", "tid"), 3.0f);   // 1/4
    satt (*p, id::fx ("eko", "aterkoppling"), 0.0f);
    satt (*p, id::fx ("eko", "pingpong"), 0.0f);
    satt (*p, id::fx ("eko", "niva"), 1.0f);
    satt (*p, id::fx ("eko", "ton"), 1.0f);
    const auto ut = rendera (*p, 1.0, ton (10, 37, 0.05));
    int topp = 0; float storst = 0.0f;
    for (int i = (int) (0.3 * sr); i < (int) (0.8 * sr); ++i)
        if (std::abs (ut.getSample (0, i)) > storst) { storst = std::abs (ut.getSample (0, i)); topp = i; }
    const double ms = topp * 1000.0 / sr;
    kontroll (std::abs (ms - 500.0) < 6.0 && storst > 0.02f, juce::String::fromUTF8 ("ekot kommer efter ") + juce::String (ms, 1) + juce::String::fromUTF8 (" ms (väntat 500)"));
}

void testaKlamman()
{
    std::cout << juce::String::fromUTF8 ("14. Klämman\n");
    using namespace km::params;
    auto mix = [] (float pump)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::fx ("klam", "pump"), pump);
        satt (*p, id::spela, 1.0f);
        return rms (rendera (*p, 2.0));
    };
    const float d = dB (mix (1.0f)) - dB (mix (0.0f));
    kontroll (d < -2.0f, juce::String::fromUTF8 ("pumpen sänker nivån ") + juce::String (d, 1) + " dB");
}

void testaVarvtalet()
{
    std::cout << "15. Varvtalet\n";
    using namespace km::params;
    const double f = 100.0 / 135.0;
    for (int lage = 0; lage < 2; ++lage)
    {
        const juce::String namn = lage == 0 ? "musikaliskt" : "skivspelare";
        // Tempo: bastrumman på varje fjärdedel.
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        p->banker.kolvbasen.rensa();
        satt (*p, id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);
        satt (*p, id::kit, (float) km::Kit::Niononio);
        satt (*p, id::tempo, 125.0f);
        satt (*p, id::varvtal, 1.0f);
        satt (*p, id::varvLage, (float) lage);
        satt (*p, id::spela, 1.0f);
        const auto a = ansatser (rendera (*p, 4.0));
        const double vantat = sr * 60.0 / (125.0 * f);
        double vart = 0.0;
        for (size_t i = 0; i < a.size(); ++i) vart = std::max (vart, std::abs (a[i] - (double) i * vantat));
        kontroll (a.size() >= 6 && vart < 12.0, namn + ": " + juce::String ((int) a.size()) + " slag, avvikelse " + juce::String (vart, 1) + " sampel");

        // Tonhöjd: en hållen sågtand på A2 (110 Hz).
        auto q = nyProcessor();
        q->presets.loadByName ("Init");
        for (auto [pid, v] : { std::pair { id::kolv (0, "sub"), 0.0f }, std::pair { id::kolv (0, "cutoff"), 8000.0f },
                               std::pair { id::kolv (0, "resonans"), 0.0f }, std::pair { id::kolv (0, "envMangd"), 0.0f },
                               std::pair { id::kolv (0, "hall"), 1.0f }, std::pair { id::kolv (0, "drive"), 0.0f } })
            satt (*q, pid, v);
        satt (*q, id::varvtal, 1.0f);
        satt (*q, id::varvLage, (float) lage);
        const auto ut = rendera (*q, 1.5, ton (1, 45, 1.4));
        const double hz = grundton (ut, (int) (0.3 * sr), (int) (1.3 * sr));
        kontroll (std::abs (hz - 110.0 * f) < 1.0, namn + ": " + juce::String (hz, 1) + juce::String::fromUTF8 (" Hz (väntat ") + juce::String (110.0 * f, 1) + ")");
    }
}

void testaSpanningen()
{
    std::cout << juce::String::fromUTF8 ("16. Spänningen\n");
    using namespace km::params;
    for (auto preset : { "Kolv 125", "Fabriksgolv 125" })
    {
        auto mix = [preset] (float s)
        {
            auto p = nyProcessor();
            p->presets.loadByName (preset);
            satt (*p, id::spanning, s);
            satt (*p, id::spela, 1.0f);
            return mat (rendera (*p, 3.0));
        };
        const auto av = mix (0.0f), full = mix (1.0f);
        const float d = dB (full.rms) - dB (av.rms);
        kontroll (std::abs (d) <= 4.0f && full.topp <= 1.0f && full.finit,
                  juce::String (preset) + juce::String::fromUTF8 (": full spänning ") + (d >= 0 ? "+" : "") + juce::String (d, 1) + " dB, topp " + juce::String (full.topp, 3));
    }
}

void testaModulationen()
{
    std::cout << "17. Modulationen\n";
    using namespace km::params;
    auto hf = [] (float mangd)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::kanal ((int) km::Kanal::Stampverket, "tyst"), 1.0f);
        satt (*p, id::lfo (0, "form"), 2.0f);   // fyrkant
        satt (*p, id::lfo (0, "synk"), 4.0f);   // 1/2
        satt (*p, id::mod (0, "kalla"), 1.0f);
        satt (*p, id::mod (0, "mal"), 1.0f);    // Kolvbasen filter
        satt (*p, id::mod (0, "mangd"), mangd);
        satt (*p, id::spela, 1.0f);
        const auto ut = rendera (*p, 2.0);
        double e = 0.0;
        for (int i = 1; i < ut.getNumSamples(); ++i) { const double d = ut.getSample (0, i) - ut.getSample (0, i - 1); e += d * d; }
        return e / rms (ut) / rms (ut);
    };
    const double utan = hf (0.0f), med = hf (1.0f);
    kontroll (med > utan * 1.2, juce::String::fromUTF8 ("LFO på filtret: diskantandel ×") + juce::String (med / utan, 2));
}
void testaKassetten()
{
    std::cout << "18. Kassetten\n";
    using namespace km::params;
    auto spela = [] (float kvalitet, float slitage)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Fabriksgolv 125");
        satt (*p, id::fx ("kass", "kvalitet"), kvalitet);
        satt (*p, id::fx ("kass", "slitage"), slitage);
        satt (*p, id::spela, 1.0f);
        return rendera (*p, 3.0);
    };
    const auto ren = spela (0.0f, 0.0f);
    auto referens = nyProcessor();
    referens->presets.loadByName ("Fabriksgolv 125");
    satt (*referens, id::spela, 1.0f);
    const auto ref = rendera (*referens, 3.0);
    double skillnad = 0.0;
    for (int c = 0; c < ref.getNumChannels(); ++c)
        for (int i = 0; i < ref.getNumSamples(); ++i)
            skillnad = std::max (skillnad, (double) std::abs (ref.getSample (c, i) - ren.getSample (c, i)));
    kontroll (skillnad <= 0.0, "SP utan slitage: ingen skillnad (" + juce::String (skillnad, 6) + ")");

    for (float kv : { 1.0f, 2.0f })
    {
        const auto ut = mat (spela (kv, 1.0f));
        const float d = dB (ut.rms) - dB (mat (ref).rms);
        kontroll (ut.finit && ut.topp <= 1.0f && std::abs (d) <= 3.0f,
                  juce::String (kv < 1.5f ? "LP" : "EP") + juce::String::fromUTF8 (", fullt slitage: ") + (d >= 0 ? "+" : "") + juce::String (d, 1) + " dB, topp " + juce::String (ut.topp, 3));
    }
}
void testaLaten()
{
    std::cout << juce::String::fromUTF8 ("19. Låtläget\n");
    using namespace km::params;
    {
        km::Laten l;
        const juce::String text ("4 kb=1A kb2=2B sv=3AB hu=16A tyst=kb2,hu sp=50 tr=+5; 1 kb=1A kb2=1A sv=1A hu=1A");
        l.fromString (text);
        kontroll (l.antal() == 2 && l.toString() == text && l.totalaTakter() == 5 && l.plats (4, true).rad == 1 && l.plats (5, false).rad < 0
                  && l.plats (6, true).rad == 0 && l.plats (6, true).taktIRad == 1,
                  "textformatet: " + l.toString());
    }
    // Mönster 1: ett slag i början av takten; mönster 2: tomt. Rad 1 spelar mönster 1, rad 2 mönster 2.
    for (int stanna = 0; stanna < 2; ++stanna)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        p->banker.kolvbasen.rensa();
        p->banker.trummor.rensa();
        p->banker.trummor.get (0, 0).fromString ("2000000000000000");
        p->banker.laten.fromString ("1 sv=1A tyst=kb,kb2,hu; 1 sv=2A tyst=kb,kb2,hu");
        satt (*p, id::kit, (float) km::Kit::Niononio);
        satt (*p, id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);
        satt (*p, id::tempo, 200.0f);
        satt (*p, id::latLage, 1.0f);
        satt (*p, id::latSlut, (float) stanna);
        satt (*p, id::spela, 1.0f);
        const double takt = 60.0 / 200.0 * 4.0;
        const auto ut = rendera (*p, takt * 4.0 + 0.1);
        juce::String bild;
        bool ratt = true;
        for (int t = 0; t < 4; ++t)
        {
            juce::AudioBuffer<float> del (1, (int) (0.3 * sr));
            del.copyFrom (0, 0, ut, 0, (int) (t * takt * sr), del.getNumSamples());
            const bool ljud = rms (del) > 0.01;
            const bool vantat = t == 0 || (t == 2 && stanna == 0);
            ratt = ratt && ljud == vantat;
            bild << (ljud ? "X" : ".");
        }
        kontroll (ratt, juce::String (stanna ? "Stanna" : "Loopa") + ": takterna " + bild);
    }
}

void testaDemolatarna (const juce::File& katalog)
{
    std::cout << juce::String::fromUTF8 ("20. Demolåtarna\n");
    const auto& fabrik = km::presets::factoryPresets();
    int antal = 0;
    for (int i = 0; i < (int) fabrik.size(); ++i)
    {
        if (fabrik[(size_t) i].laten == nullptr) continue;
        ++antal;
        auto p = nyProcessor();
        p->presets.load (i);
        satt (*p, km::params::id::spela, 1.0f);
        const auto namn = p->presets.getEntries()[(size_t) i].name;
        const auto& laten = p->banker.laten;
        const int total = laten.totalaTakter();
        const double takt = verkligaTakter (*p, 1);
        const auto ut = rendera (*p, takt * (total + 1));
        const auto m = mat (ut);
        kontroll (m.finit && m.topp <= 1.0f && m.rms > 0.04 && m.rms < 0.32,
                  namn + ": " + juce::String (laten.antal()) + " rader, " + juce::String (total) + " takter ("
                  + juce::String ((int) (takt * total) / 60) + ":" + juce::String ((int) (takt * total) % 60).paddedLeft ('0', 2)
                  + "), topp " + juce::String (m.topp, 3) + ", RMS " + juce::String (dB (m.rms), 1) + " dB");

        // Varje rad ska låta, om inte alla kanalerna är tysta i den.
        juce::String tysta;
        double svagast = 1.0;
        int start = 0;
        for (int r = 0; r < laten.antal(); ++r)
        {
            const auto rad = laten.get (r);
            bool allaTysta = true;
            for (bool t : rad.tyst) allaTysta = allaTysta && t;
            const auto del = mat (ut, (int) (start * takt * sr), (int) ((start + rad.takter) * takt * sr));
            start += rad.takter;
            if (allaTysta) continue;
            svagast = std::min (svagast, del.rms);
            if (del.rms < 0.01) tysta << " " << (r + 1);
        }
        kontroll (tysta.isEmpty(), namn + juce::String::fromUTF8 (": varje rad låter (svagaste rad RMS ") + juce::String (dB (svagast), 1)
                                   + " dB)" + (tysta.isEmpty() ? juce::String() : juce::String::fromUTF8 (", tysta rader:") + tysta));
        skrivWav (ut, katalog.getChildFile ("demo_" + filnamn (namn.fromFirstOccurrenceOf (juce::String::fromUTF8 ("· "), false, false)) + ".wav"));
    }
    kontroll (antal >= 2, juce::String (antal) + juce::String::fromUTF8 (" fabrikspresets med låt"));
}
void testaLjudbanken()
{
    std::cout << juce::String::fromUTF8 ("21. Ljudbanken\n");
    using namespace km::params;
    auto varde = [] (KroppsmaskinenProcessor& p, const juce::String& pid) { return p.state.getRawParameterValue (pid)->load(); };
    auto monsterText = [] (KroppsmaskinenProcessor& p)
    {
        juce::String t;
        for (int m = 0; m < km::antalMonster; ++m)
            for (int v = 0; v < 2; ++v)
                t << p.banker.trummor.get (m, v).toString() << p.banker.kolvbasen.get (m, v).toString() << p.banker.kolvbas2.get (m, v).toString()
                  << p.banker.hugget.get (m, v).toString();
        return t + p.banker.laten.toString();
    };
    {
        auto p = nyProcessor();
        const auto& l = p->ljud;
        kontroll (l.lista (km::LjudTyp::Bas).size() > 10 && l.lista (km::LjudTyp::Trummor).size() > 10 && l.lista (km::LjudTyp::Hugget).size() > 10,
                  juce::String ((int) l.lista (km::LjudTyp::Bas).size()) + " basljud, " + juce::String ((int) l.lista (km::LjudTyp::Trummor).size())
                  + " trumljud, " + juce::String ((int) l.lista (km::LjudTyp::Hugget).size()) + juce::String::fromUTF8 (" ljud för Hugget"));
    }
    // Ett basljud på Kolvbasen: bara Kolvbasens klang ändras.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Valsverk 124");
        const auto fore = monsterText (*p);
        const float tempo = varde (*p, id::tempo), niva = varde (*p, id::kanal (0, "niva")), kb2 = varde (*p, id::kolv (1, "cutoff"));
        const int i = (int) p->ljud.lista (km::LjudTyp::Bas).size() - 1;
        p->ljud.ladda (0, i);
        const float mal = p->ljud.lista (km::LjudTyp::Bas)[(size_t) i].varden.at ("cutoff");
        kontroll (monsterText (*p) == fore && std::abs (varde (*p, id::tempo) - tempo) < 0.01f && std::abs (varde (*p, id::kanal (0, "niva")) - niva) < 0.001f
                  && std::abs (varde (*p, id::kolv (1, "cutoff")) - kb2) < 0.01f && std::abs (varde (*p, id::kolv (0, "cutoff")) - mal) < 0.5f
                  && p->ljud.aktuelltNamn (0) == p->ljud.lista (km::LjudTyp::Bas)[(size_t) i].namn,
                  juce::String::fromUTF8 ("basljudet ändrar bara Kolvbasens klang (") + p->ljud.aktuelltNamn (0) + ")");
    }
    // Alla fabriksljud låter rent, solo på sin kanal.
    int antal = 0, tysta = 0;
    float vartTopp = 0.0f;
    bool rena = true;
    juce::String tystaNamn;
    for (int t = 0; t < 3; ++t)
    {
        const int kanal = t == 0 ? 0 : (t == 1 ? (int) km::Kanal::Stampverket : (int) km::Kanal::Hugget);
        const auto n = (int) nyProcessor()->ljud.lista ((km::LjudTyp) t).size();
        for (int i = 0; i < n; ++i)
        {
            auto p = nyProcessor();
            p->presets.loadByName ("Kolv 125");
            p->banker.hugget.get (0, 0).fromString ("0a . . . 0 . . 3 . . 0 . -2 . . .");
            p->ljud.ladda (kanal, i);
            for (int k = 0; k < km::antalKanaler; ++k) satt (*p, id::kanal (k, "solo"), k == kanal ? 1.0f : 0.0f);
            satt (*p, id::spela, 1.0f);
            const auto m = mat (rendera (*p, takter (*p, 1) + 0.3));
            rena = rena && m.finit && m.topp <= 1.0f;
            vartTopp = juce::jmax (vartTopp, m.topp);
            if (m.rms < 0.003) { ++tysta; tystaNamn << " " << p->ljud.aktuelltNamn (kanal); }
            ++antal;
        }
    }
    kontroll (rena && tysta == 0, juce::String (antal) + " ljud solo: " + (rena ? "alla rena" : "ORENA") + ", " + juce::String (tysta) + " tysta"
              + tystaNamn + juce::String::fromUTF8 (", högsta topp ") + juce::String (vartTopp, 3));
    // Lås mönstren.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Fabriksgolv 125");
        const auto fore = monsterText (*p);
        const float tempo = varde (*p, id::tempo), monster = varde (*p, id::kanal (2, "monster"));
        p->presets.lasMonster = true;
        p->presets.loadByName ("Syrabad 108");
        const bool ok = monsterText (*p) == fore && std::abs (varde (*p, id::tempo) - tempo) < 0.01f && std::abs (varde (*p, id::kanal (2, "monster")) - monster) < 0.01f;
        p->presets.lasMonster = false;
        auto q = nyProcessor();
        q->presets.loadByName ("Syrabad 108");
        const bool ljudet = std::abs (varde (*p, id::kolv (1, "cutoff")) - varde (*q, id::kolv (1, "cutoff"))) < 0.01f
                         && std::abs (varde (*p, id::master) - varde (*q, id::master)) < 0.001f;
        kontroll (ok && ljudet, juce::String::fromUTF8 ("Lås mönstren: Syrabads ljud på Fabriksgolvets mönster och tempo"));
    }
}

void testaMidiSpel()
{
    std::cout << "22. MIDI-spel\n";
    using namespace km::params;
    // Sekvensern går (egen klocka). Kolvbasen spelas från MIDI: mönstret tystnar, tonen på kanal 1 spelar.
    auto spela = [] (bool midi, bool medTon)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::kanal (0, "solo"), 1.0f);
        satt (*p, id::kanal (0, "midi"), midi ? 1.0f : 0.0f);
        satt (*p, id::spela, 1.0f);
        const auto hela = medTon ? rendera (*p, 1.5, ton (1, 45, 1.0, 0.2)) : rendera (*p, 1.5);
        // Från 0,15 s: kanalernas nivåer glider in de första millisekunderna (solo sätts efter första blocket).
        juce::AudioBuffer<float> ut (hela.getNumChannels(), hela.getNumSamples() - (int) (0.15 * sr));
        for (int c = 0; c < ut.getNumChannels(); ++c) ut.copyFrom (c, 0, hela, c, (int) (0.15 * sr), ut.getNumSamples());
        return mat (ut);
    };
    const auto sekvens = spela (false, false), tyst = spela (true, false), medNot = spela (true, true);
    kontroll (sekvens.rms > 0.01 && tyst.rms < 0.001 && medNot.rms > 0.01,
              juce::String::fromUTF8 ("Kolvbasen från MIDI: mönstret ") + juce::String (dB (sekvens.rms), 1) + juce::String::fromUTF8 (" dB, MIDI-läge utan toner ")
              + juce::String (dB (tyst.rms), 1) + juce::String::fromUTF8 (" dB, med en ton ") + juce::String (dB (medNot.rms), 1) + " dB");
    // Stampverket från MIDI: sekvensens trummor tystnar, kanal 10 spelar fortfarande.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::kanal (2, "solo"), 1.0f);
        satt (*p, id::kanal (2, "midi"), 1.0f);
        satt (*p, id::spela, 1.0f);
        auto delAv = [] (const juce::AudioBuffer<float>& b)
        {
            juce::AudioBuffer<float> u (b.getNumChannels(), b.getNumSamples() - (int) (0.15 * sr));
            for (int c = 0; c < u.getNumChannels(); ++c) u.copyFrom (c, 0, b, c, (int) (0.15 * sr), u.getNumSamples());
            return mat (u);
        };
        const auto utan = delAv (rendera (*p, 1.0));
        auto q = nyProcessor();
        q->presets.loadByName ("Kolv 125");
        satt (*q, id::kanal (2, "solo"), 1.0f);
        satt (*q, id::kanal (2, "midi"), 1.0f);
        satt (*q, id::spela, 1.0f);
        const auto med = delAv (rendera (*q, 1.0, ton (10, 36, 0.1, 0.3)));
        kontroll (utan.rms < 0.001 && med.rms > 0.005,
                  juce::String::fromUTF8 ("Stampverket från MIDI: utan toner ") + juce::String (dB (utan.rms), 1) + juce::String::fromUTF8 (" dB, bastrumma på kanal 10 ") + juce::String (dB (med.rms), 1) + " dB");
    }
    // Kolvbas 2 som följer Kolvbasen tystnar också när Kolvbasen spelas från MIDI.
    {
        auto folj = [] (bool midi)
        {
            auto p = nyProcessor();
            p->presets.loadByName ("Kolv 125");
            satt (*p, id::folj, 1.0f);
            satt (*p, id::kanal (1, "solo"), 1.0f);
            satt (*p, id::kanal (0, "midi"), midi ? 1.0f : 0.0f);
            satt (*p, id::spela, 1.0f);
            const auto hela = rendera (*p, 1.5);
            juce::AudioBuffer<float> u (hela.getNumChannels(), hela.getNumSamples() - (int) (0.15 * sr));
            for (int c = 0; c < u.getNumChannels(); ++c) u.copyFrom (c, 0, hela, c, (int) (0.15 * sr), u.getNumSamples());
            return mat (u);
        };
        const auto foljer = folj (false), tyst = folj (true);
        kontroll (foljer.rms > 0.005 && tyst.rms < 0.001,
                  juce::String::fromUTF8 ("Kolvbas 2 följer Kolvbasen: ") + juce::String (dB (foljer.rms), 1)
                  + juce::String::fromUTF8 (" dB, med Kolvbasen från MIDI ") + juce::String (dB (tyst.rms), 1) + " dB");
    }
    // Allt från MIDI: hela sekvensern tyst, och läget (och kanalernas MIDI-spel) behålls när presets byts.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Signalfabrik 126");
        satt (*p, id::midiAlla, 1.0f);
        satt (*p, id::kanal (3, "midi"), 1.0f);
        satt (*p, id::spela, 1.0f);
        p->presets.loadByName ("Kolv 125");
        const bool behalls = p->state.getRawParameterValue (id::midiAlla)->load() > 0.5f
                          && p->state.getRawParameterValue (id::kanal (3, "midi"))->load() > 0.5f;
        const auto hela = rendera (*p, 1.5);
        juce::AudioBuffer<float> u (hela.getNumChannels(), hela.getNumSamples() - (int) (0.15 * sr));
        for (int c = 0; c < u.getNumChannels(); ++c) u.copyFrom (c, 0, hela, c, (int) (0.15 * sr), u.getNumSamples());
        const auto tyst = mat (u);
        satt (*p, id::midiAlla, 0.0f);
        p->presets.loadByName ("Signalfabrik 126");
        const auto igen = mat (rendera (*p, 1.0));
        kontroll (behalls && tyst.rms < 0.001 && igen.rms > 0.01,
                  juce::String::fromUTF8 ("Allt från MIDI: behålls vid presetbyte, sekvensern ") + juce::String (dB (tyst.rms), 1)
                  + juce::String::fromUTF8 (" dB; av igen ") + juce::String (dB (igen.rms), 1) + " dB");
    }
}


// Grundtonhöjden i en ton (autokorrelation på vänster kanal mellan 'fran' och 'fran + 0,4 s').
double tonhojd (const juce::AudioBuffer<float>& b, double fran, double minHz = 30.0, double maxHz = 1000.0)
{
    const int s0 = (int) (fran * sr), n = (int) (0.4 * sr);
    const float* x = b.getReadPointer (0);
    const int minLag = (int) (sr / maxHz), maxLag = (int) (sr / minHz);
    std::vector<double> r ((size_t) maxLag + 2, 0.0);
    for (int lag = minLag; lag <= maxLag + 1; ++lag)
    {
        double a = 0.0;
        for (int i = 0; i < n; ++i) a += (double) x[s0 + i] * (double) x[s0 + i + lag];
        r[(size_t) lag] = a;
    }
    // Första toppen över 90 % av den största (undviker oktavfel).
    double storst = 0.0;
    for (int lag = minLag + 1; lag <= maxLag; ++lag) storst = std::max (storst, r[(size_t) lag]);
    for (int lag = minLag + 1; lag <= maxLag; ++lag)
        if (r[(size_t) lag] > 0.9 * storst && r[(size_t) lag] >= r[(size_t) lag - 1] && r[(size_t) lag] >= r[(size_t) lag + 1])
        {
            const double a = r[(size_t) lag - 1], bb = r[(size_t) lag], c = r[(size_t) lag + 1];
            const double d = 0.5 * (a - c) / (a - 2.0 * bb + c);
            return sr / ((double) lag + d);
        }
    return 0.0;
}

// Tonhöjden nära en väntad frekvens: autokorrelationens topp vid flera perioder (omkring 1 500 sampel),
// så att interpolationsfelet delas på många perioder. Hittar inom ±3 % (±50 cent).
double tonhojdNara (const juce::AudioBuffer<float>& b, double fran, double vantadHz)
{
    const int s0 = (int) (fran * sr), n = (int) (0.4 * sr);
    const float* x = b.getReadPointer (0);
    const double L = sr / vantadHz;
    const int M = std::max (1, (int) std::lround (1500.0 / L));
    const int lo = (int) std::floor (M * L * 0.97), hi = (int) std::ceil (M * L * 1.03);
    auto r = [&] (int lag) { double a = 0.0; for (int i = 0; i < n; ++i) a += (double) x[s0 + i] * x[s0 + i + lag]; return a; };
    int bast = lo; double bastV = -1.0e30;
    for (int lag = lo; lag <= hi; ++lag) if (const double v = r (lag); v > bastV) { bastV = v; bast = lag; }
    const double a = r (bast - 1), c = r (bast + 1);
    const double d = 0.5 * (a - c) / (a - 2.0 * bastV + c);
    return sr * M / ((double) bast + d);
}

void testaStamningen()
{
    std::cout << "23. Stämningen\n";
    using namespace km::params;
    // Tabellerna: liksvävande är exakt, ren ters och kvint från tonikan, 24-TET kvartstoner, Scala med karta.
    {
        km::StamningTabell t {};
        km::raknaTabell (km::Skala::inbyggd (km::StamningTyp::Liksvavande), 33, t);
        bool lika = true;
        for (int n = 0; n < 128; ++n) lika = lika && std::abs (t[(size_t) n] - n) < 1.0e-9;
        km::raknaTabell (km::Skala::inbyggd (km::StamningTyp::Ren), km::tonikaTangent (33, 0), t);
        const double ters = (t[37] - 33.0) * 100.0, kvint = (t[40] - 33.0) * 100.0, oktav = (t[45] - 33.0) * 100.0;
        km::StamningTabell q {};
        km::raknaTabell (km::Skala::inbyggd (km::StamningTyp::Tet24), km::tonikaTangent (33, 0), q);
        kontroll (lika && std::abs (ters - 386.31) < 0.01 && std::abs (kvint - 701.96) < 0.01 && std::abs (oktav - 1200.0) < 1.0e-9
                  && std::abs ((q[34] - q[33]) * 100.0 - 50.0) < 1.0e-9,
                  juce::String::fromUTF8 ("tabeller: liksvävande exakt, ren ters ") + juce::String (ters, 2) + juce::String::fromUTF8 (" cent, kvint ")
                  + juce::String (kvint, 2) + juce::String::fromUTF8 (" cent, 24-TET 50 cent per tangent"));
        km::Skala s = km::Skala::tet (12);
        const bool scl = s.lasScl ("! test.scl\nBohlen-Pierce\n 13\n!\n27/25\n25/21\n9/7\n7/5\n75/49\n5/3\n9/5\n49/25\n15/7\n7/3\n63/25\n25/9\n3/1\n");
        const bool kbm = s.lasKbm ("! test.kbm\n12\n0\n127\n60\n69\n440.0\n13\n0\n1\nx\n2\n3\n4\n5\n6\n7\n8\n9\n10\n");
        km::raknaTabell (s, 60, t);
        kontroll (scl && kbm && s.antal == 13 && std::abs (s.period() - 1901.955) < 0.01 && std::abs (t[69] - 69.0) < 1.0e-9 && std::isnan (t[62]),
                  juce::String::fromUTF8 ("Scala: 13 toner, period ") + juce::String (s.period(), 2) + juce::String::fromUTF8 (" cent, referensen 440 Hz, tangent x tyst"));
        km::Skala fel;
        juce::String varfor;
        const bool avvisad = ! fel.lasScl ("trasig\n3\n1/0\n", &varfor);
        kontroll (avvisad && varfor.isNotEmpty(), juce::String::fromUTF8 ("trasig .scl avvisas: ") + varfor);
    }
    // Ljudet: Kolvbasen och Hugget spelar E med Rast på C (en kvartston låg); trummorna rörs inte.
    auto spela = [] (int kanal, int midiKanal, int notnr, int stamning, int tonika)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal (kanal, "solo"), 1.0f);
        satt (*p, id::kanal (kanal, "midi"), 1.0f);
        satt (*p, id::stamning, (float) stamning);
        satt (*p, id::stamningTon, (float) tonika);
        const auto b = rendera (*p, 1.0, ton (midiKanal, notnr, 0.9, 0.05));
        return tonhojd (b, 0.3);
    };
    const double lik = spela (0, 1, 40, 0, 0), rast = spela (0, 1, 40, (int) km::StamningTyp::Rast, 1);
    const double cent = 1200.0 * std::log2 (rast / lik), forvantad = 440.0 * std::exp2 ((40 - 69) / 12.0);
    const double avOktav = std::remainder (1200.0 * std::log2 (lik / forvantad), 1200.0);   // suboscillatorn kan ge oktaven under
    kontroll (std::abs (avOktav) < 5.0 && std::abs (cent + 50.0) < 3.0,
              juce::String::fromUTF8 ("Kolvbasen E2: liksvävande ") + juce::String (lik, 2) + " Hz, Rast " + juce::String (rast, 2) + " Hz ("
              + juce::String (cent, 1) + " cent)");
    const double hLik = spela (3, 3, 64, 0, 0), hRast = spela (3, 3, 64, (int) km::StamningTyp::Rast, 1);
    const double hCent = 1200.0 * std::log2 (hRast / hLik);
    kontroll (hLik > 0.0 && std::abs (hCent + 50.0) < 3.0,
              juce::String::fromUTF8 ("Hugget E4: liksvävande ") + juce::String (hLik, 2) + " Hz, Rast " + juce::String (hRast, 2) + " Hz (" + juce::String (hCent, 1) + " cent)");
    // Trummorna: samma ljud oavsett stämning.
    auto trummor = [] (int stamning)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::kanal (2, "solo"), 1.0f);
        satt (*p, id::stamning, (float) stamning);
        satt (*p, id::spela, 1.0f);
        return rendera (*p, 1.0);
    };
    const auto t0 = trummor (0), t1 = trummor ((int) km::StamningTyp::Tet19);
    float skillnad = 0.0f;
    for (int i = 0; i < t0.getNumSamples(); ++i) skillnad = std::max (skillnad, std::abs (t0.getSample (0, i) - t1.getSample (0, i)));
    kontroll (skillnad < 1.0e-6f, juce::String::fromUTF8 ("Stampverket oberoende av stämningen (största skillnad ") + juce::String (skillnad, 7) + ")");
}


// Energin vid en frekvens (Goertzel) mellan 'fran' och 'fran + langd' sekunder.
double energiVid (const juce::AudioBuffer<float>& b, double hz, double fran, double langd)
{
    const int s0 = (int) (fran * sr), n = (int) (langd * sr);
    const double w = 2.0 * juce::MathConstants<double>::pi * hz / sr, c = 2.0 * std::cos (w);
    double s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double hann = 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * i / n);
        const double s = b.getSample (0, s0 + i) * hann + c * s1 - s2;
        s2 = s1; s1 = s;
    }
    return std::sqrt (s1 * s1 + s2 * s2 - c * s1 * s2) / n;
}

void testaGjuteriet()
{
    std::cout << "24. Gjuteriet\n";
    using namespace km::params;
    const int kit = (int) km::Kit::Gjuteriet;
    auto slag = [kit] (int spar, std::vector<std::pair<juce::String, float>> varden, double sek = 2.0)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kit, (float) kit);
        for (auto& [pid, v] : varden) satt (*p, pid, v);
        const int notnr = km::Stampverket::notForSpar (spar);
        return rendera (*p, sek, [notnr] (juce::MidiBuffer& m, int pos, int) { if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (10, notnr, 1.0f), 0); });
    };
    // Rör lågt: lägsta moden på 180 Hz och nästa på 2,756 gånger det (en fri stav), en oktav upp med Stämning +12.
    const int ror = (int) km::Trumma::TomLag;
    const auto r0 = slag (ror, {}), r12 = slag (ror, { { id::trumma (ror, "stamning"), 12.0f } });
    const double vid = energiVid (r0, 180.0, 0.05, 0.4), bredvid = energiVid (r0, 205.0, 0.05, 0.4), mod2 = energiVid (r0, 180.0 * 2.756, 0.05, 0.4);
    const double upp = energiVid (r12, 360.0, 0.05, 0.4), uppBredvid = energiVid (r12, 410.0, 0.05, 0.4);
    kontroll (vid > 8.0 * bredvid && mod2 > 4.0 * energiVid (r0, 560.0, 0.05, 0.4) && upp > 8.0 * uppBredvid,
              juce::String::fromUTF8 ("Rör lågt: moden 180 Hz ") + juce::String (20.0 * std::log10 (vid / bredvid), 1) + juce::String::fromUTF8 (" dB över grannen, andra moden (496 Hz) syns, +12 st ger 360 Hz"));
    // Klang: längre klang ger längre ring.
    const int stad = (int) km::Trumma::Metall;
    auto ring = [&] (float klang)
    {
        const auto b = slag (stad, { { id::trumma (stad, "klang"), klang } }, 3.0);
        return mat (b, (int) (1.0 * sr)).rms;
    };
    const double kort = ring (0.0f), lang = ring (1.0f);
    kontroll (lang > 10.0 * kort, juce::String::fromUTF8 ("Städ: efter en sekund ") + juce::String (dB (kort), 1) + juce::String::fromUTF8 (" dB med kort klang, ")
                                    + juce::String (dB (lang), 1) + juce::String::fromUTF8 (" dB med lång"));
    // Kedjan rasslar: många anslag.
    const auto kedja = slag ((int) km::Trumma::Virvel2, {}, 1.0);
    int toppar = 0;
    {
        const int f = (int) (0.004 * sr);
        double forra = 0.0;
        bool uppe = false;
        for (int s0 = 0; s0 + f < (int) (0.4 * sr); s0 += f)
        {
            double e = 0.0;
            for (int i = 0; i < f; ++i) e = std::max (e, (double) std::abs (kedja.getSample (0, s0 + i)));
            if (! uppe && e > forra * 1.6 && e > 0.02) { ++toppar; uppe = true; }
            else if (e < forra) uppe = false;
            forra = e;
        }
    }
    kontroll (toppar >= 6, juce::String::fromUTF8 ("Kedjan: ") + juce::String (toppar) + juce::String::fromUTF8 (" anslag de första 0,4 s"));
    // Fjädern och klockan klingar, och alla tolv slutar av sig själva (testas i 2. Varje ljud).
    const auto fj = slag ((int) km::Trumma::Small, {}, 2.0);
    kontroll (mat (fj, (int) (0.5 * sr)).rms > 1.0e-3, juce::String::fromUTF8 ("Fjädern klingar efter en halv sekund (") + juce::String (dB (mat (fj, (int) (0.5 * sr)).rms), 1) + " dB)");
}


// Stämning i cent: Kolvbasen (utan sub) och Hugget (Blåsstöt utan svävning och sänkning, inget ackord) i bas, mellanläge och diskant.
void testaStamningICent()
{
    std::cout << juce::String::fromUTF8 ("25. Stämning i cent\n");
    using namespace km::params;
    struct Fall { const char* namn; int kanal, midiKanal, notnr; };
    const Fall fall[] { { "Kolvbasen bas (A1)", 0, 1, 33 }, { "Kolvbasen mellan (A3)", 0, 1, 57 }, { "Kolvbasen diskant (A5)", 0, 1, 81 },
                        { "Hugget bas (A2)", 3, 3, 45 }, { "Hugget mellan (A4)", 3, 3, 69 }, { "Hugget diskant (A6)", 3, 3, 93 } };
    for (const auto& f : fall)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal (f.kanal, "solo"), 1.0f);
        satt (*p, id::kanal (f.kanal, "midi"), 1.0f);
        satt (*p, id::kolv (0, "sub"), 0.0f);
        satt (*p, id::kolv (0, "cutoff"), 8000.0f);
        satt (*p, id::hu ("lage"), (float) (int) km::HuggLage::Blas);
        satt (*p, id::hu ("ackord"), 0.0f);
        satt (*p, id::hu ("bredd"), 0.0f);
        satt (*p, id::hu ("hall"), 1.0f);
        satt (*p, id::hu ("sankning"), 0.0f);   // stötarna sjunker annars avsiktligt i anslaget
        const auto b = rendera (*p, 1.0, ton (f.midiKanal, f.notnr, 0.9, 0.05));
        const double vantad = 440.0 * std::exp2 ((f.notnr - 69) / 12.0);
        const double hz = tonhojdNara (b, 0.3, vantad);
        const double cent = hz > 0.0 ? 1200.0 * std::log2 (hz / vantad) : 9999.0;
        kontroll (std::abs (cent) < 3.0, juce::String::fromUTF8 (f.namn) + ": " + juce::String (hz, 2) + " Hz (" + juce::String (cent, 2) + " cent)");
    }
}

// Klick: ett hopp från ett sampel till nästa (ett knäpp) jämfört med de största hoppen i signalen strax före.
// En mjuk övertoning (några millisekunder) ger inga större hopp än signalen själv; ett tvärt avbrott gör det.
double klickKvot (const juce::AudioBuffer<float>& b, int vid)
{
    const float* x = b.getReadPointer (0);
    auto d2 = [x] (int i) { return std::abs ((double) x[i] - x[i - 1]); };
    double fore = 1.0e-9, under = 0.0;
    for (int i = vid - (int) (0.3 * sr); i < vid - (int) (0.01 * sr); ++i) fore = std::max (fore, d2 (i));
    for (int i = vid - (int) (0.001 * sr); i < vid + (int) (0.004 * sr); ++i) under = std::max (under, d2 (i));
    return under / fore;
}

double hoppIDifferens (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b, int fran, int till)
{
    double storst = 0.0;
    for (int i = fran + 1; i < till; ++i)
    {
        const double d0 = (double) a.getSample (0, i - 1) - b.getSample (0, i - 1), d1 = (double) a.getSample (0, i) - b.getSample (0, i);
        storst = std::max (storst, std::abs (d1 - d0));
    }
    return storst;
}

void testaKlick()
{
    std::cout << "26. Klick\n";
    using namespace km::params;
    // Tyst och solo medan Kolvbasen håller en mjuk ton (lågt filter): nivån ska glida, inte knäppa.
    // Solo på Kolvbas 2 tystar Kolvbasen.
    for (int vad = 0; vad < 2; ++vad)
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal (0, "midi"), 1.0f);
        satt (*p, id::kolv (0, "cutoff"), 160.0f);
        satt (*p, id::kolv (0, "hall"), 1.0f);
        const int vid = (int) (0.5 * sr);
        const auto b = rendera (*p, 0.8, [&p, vid, vad] (juce::MidiBuffer& m, int pos, int n)
        {
            if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, 45, 0.8f), 0);
            if (vid >= pos && vid < pos + n) satt (*p, vad == 0 ? id::kanal (0, "tyst") : id::kanal (1, "solo"), 1.0f);
        });
        const double kvot = klickKvot (b, vid - (vid % block));
        kontroll (kvot < 2.0, juce::String::fromUTF8 (vad == 0 ? "Kolvbasen tyst mitt i en ton" : "solo på Kolvbas 2 mitt i Kolvbasens ton")
                              + juce::String::fromUTF8 (": spiken ") + juce::String (kvot, 2) + juce::String::fromUTF8 (" gånger signalens största"));
    }
    // Röststöld: nio toner i Hugget (åtta röster) – den nionde tar den äldsta rösten.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal (3, "midi"), 1.0f);
        satt (*p, id::hu ("lage"), (float) (int) km::HuggLage::Synklead);
        satt (*p, id::hu ("ackord"), 0.0f);
        const int stold = (int) (0.6 * sr);
        const auto b = rendera (*p, 1.0, [stold] (juce::MidiBuffer& m, int pos, int n)
        {
            for (int i = 0; i < 9; ++i)
            {
                const int at = i < 8 ? (int) (0.05 * sr) * (i + 1) : stold;
                if (at >= pos && at < pos + n) m.addEvent (juce::MidiMessage::noteOn (3, 48 + 3 * i, 0.8f), at - pos);
            }
        });
        const double kvot = klickKvot (b, stold);
        kontroll (kvot < 2.0, juce::String::fromUTF8 ("Hugget, röststöld: spiken vid stölden ") + juce::String (kvot, 2) + juce::String::fromUTF8 (" gånger signalens största"));
    }
    // Upprepad ton i Kolvbasen (samma ton igen innan den klingat ut).
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Init");
        satt (*p, id::kanal (0, "midi"), 1.0f);
        satt (*p, id::kolv (0, "sub"), 0.0f);
        const int igen = (int) (0.6 * sr);
        const auto b = rendera (*p, 1.0, [igen] (juce::MidiBuffer& m, int pos, int n)
        {
            for (int at : { (int) (0.05 * sr), igen })
                if (at >= pos && at < pos + n) m.addEvent (juce::MidiMessage::noteOn (1, 45, 0.8f), at - pos);
            for (int at : { igen - 64 })
                if (at >= pos && at < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, 45), at - pos);
        });
        // Anslaget har ett avsiktligt klick: filtret slår upp på ett par sampel (godkänt i grind 1). Den upprepade tonen
        // får inte hoppa mer än vågformens egna flanker strax efter första anslaget (då filtret är öppet).
        auto spik = [&b] (int fran, int till) { double m = 0.0; for (int i = fran; i < till; ++i) m = std::max (m, std::abs ((double) b.getSample (0, i) - b.getSample (0, i - 1))); return m; };
        const int forst = (int) (0.05 * sr);
        const double kvot = spik (igen - 64 - (int) (0.001 * sr), igen + (int) (0.004 * sr)) / std::max (1.0e-9, spik (forst + 2, forst + (int) (0.1 * sr)));
        kontroll (kvot < 1.2, juce::String::fromUTF8 ("Kolvbasen, upprepad ton: största hoppet ") + juce::String (kvot, 2) + juce::String::fromUTF8 (" gånger vågformens egna flanker"));
    }
    // Programbyte medan sekvensern går.
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, id::spela, 1.0f);
        const int byte = (int) (0.75 * sr);
        const auto b = rendera (*p, 1.2, [&p, byte] (juce::MidiBuffer&, int pos, int n)
        {
            if (byte >= pos && byte < pos + n) p->presets.loadByName (juce::String::fromUTF8 ("Syrabad 108"));
        });
        const double kvot = klickKvot (b, byte - (byte % block));
        kontroll (kvot < 2.0, juce::String::fromUTF8 ("programbyte medan sekvensern går: spiken ") + juce::String (kvot, 2) + juce::String::fromUTF8 (" gånger signalens största"));
    }
}

// CPU: hela maskinen med allt igång, som andel av en kärna i realtid.
void testaCpu()
{
    std::cout << "27. CPU\n";
    using namespace km::params;
    auto mat = [] (const juce::String& preset, std::vector<std::pair<juce::String, float>> varden)
    {
        auto p = nyProcessor();
        p->presets.loadByName (preset);
        for (auto& [pid, v] : varden) satt (*p, pid, v);
        satt (*p, id::spela, 1.0f);
        rendera (*p, 1.0);   // uppvärmning
        const double sek = 10.0;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        rendera (*p, sek);
        return (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0 / sek * 100.0;
    };
    const double demo = mat (juce::String::fromUTF8 ("Stålverket 128"), {});
    const double tyngst = mat (juce::String::fromUTF8 ("Stålverket 128"), { { id::kit, (float) (int) km::Kit::Gjuteriet }, { id::spanning, 1.0f },
                                                                              { id::fx ("kass", "slitage"), 1.0f }, { id::stamning, (float) (int) km::StamningTyp::Ren } });
    kontroll (demo < 25.0 && tyngst < 25.0, juce::String::fromUTF8 ("Stålverket 128: ") + juce::String (demo, 1) + juce::String::fromUTF8 (" % av en kärna; med Gjuteriet, Spänningen i topp, slitet band och ren stämning: ")
                                             + juce::String (tyngst, 1) + juce::String::fromUTF8 (" % (mål under 25 %)"));
}

} // namespace

// --trummor katalog [kit]: varje trumma i varje kit (eller ett kit, 1–5) för sig, 2,5 s, som WAV.
int skrivTrummor (const char* katalogNamn, const char* kitArg)
{
    const auto katalog = juce::File::getCurrentWorkingDirectory().getChildFile (katalogNamn);
    katalog.createDirectory();
    static const char* kitFil[] = { "attaatta", "niononio", "digital", "fabrik", "gjuteriet" };
    for (int kit = 0; kit < km::antalKit; ++kit)
    {
        if (kitArg != nullptr && juce::String (kitArg).getIntValue() != kit + 1) continue;
        for (int t = 0; t < km::antalSpar; ++t)
        {
            auto p = nyProcessor();
            p->presets.loadByName ("Init");
            satt (*p, km::params::id::kit, (float) kit);
            const int notnr = km::Stampverket::notForSpar (t);
            auto ut = rendera (*p, 2.5, [notnr] (juce::MidiBuffer& m, int pos, int)
            {
                if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (10, notnr, 1.0f), 0);
            });
            const auto m = mat (ut);
            std::cout << kitFil[kit] << "\t" << t << "\t" << juce::String::fromUTF8 (km::trumNamnFor ((km::Trumma) t, (km::Kit) kit))
                      << "\ttopp " << juce::String (m.topp, 3) << "\tRMS " << juce::String (dB (m.rms), 1) << " dB\n";
            skrivWav (ut, katalog.getChildFile (juce::String (kitFil[kit]) + "_" + juce::String (t + 1).paddedLeft ('0', 2) + ".wav"));
        }
    }
    return 0;
}

// --state: läget efter preset, parametrar och mönster.
int skrivLage (int argc, char* argv[])
{
    auto p = nyProcessor();
    if (! p->presets.loadByName (juce::String::fromUTF8 (argv[2])))
    {
        std::cout << "Okänt preset: " << argv[2] << "\n";
        return 1;
    }
    for (int i = 4; i < argc; ++i)
    {
        const auto arg = juce::String::fromUTF8 (argv[i]);
        const auto nyckel = arg.upToFirstOccurrenceOf ("=", false, false);
        const auto varde = arg.fromFirstOccurrenceOf ("=", false, false);
        if (nyckel.startsWith ("monster:"))
        {
            // monster:kb:2A=text
            const auto delar = juce::StringArray::fromTokens (nyckel, ":", "");
            const auto kanal = delar[1];
            const int nr = delar[2].getIntValue() - 1;
            const int v = delar[2].endsWithChar ('B') ? 1 : 0;
            if (kanal == "sv")       p->banker.trummor.get (nr, v).fromString (varde);
            else if (kanal == "kb")  p->banker.kolvbasen.get (nr, v).fromString (varde);
            else if (kanal == "kb2") p->banker.kolvbas2.get (nr, v).fromString (varde);
            else if (kanal == "hu")  p->banker.hugget.get (nr, v).fromString (varde);
            else { std::cout << "Okänd kanal: " << kanal << "\n"; return 1; }
        }
        else if (auto* param = p->state.getParameter (nyckel))
            param->setValueNotifyingHost (param->convertTo0to1 ((float) varde.getDoubleValue()));
        else
        {
            std::cout << "Okänd parameter: " << nyckel << "\n";
            return 1;
        }
    }
    juce::MemoryBlock mb;
    p->getStateInformation (mb);
    auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
    const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[3]));
    return xml != nullptr && fil.replaceWithText (xml->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader())) ? 0 : 1;
}

// --parametrar: index, ID och antal steg (0 = kontinuerlig) för varje parameter.
int skrivParametrar (const char* filnamn)
{
    auto p = nyProcessor();
    juce::String ut;
    for (auto* param : p->getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            ut << rp->getParameterIndex() << "\t" << rp->getParameterID() << "\t"
               << rp->getNormalisableRange().start << "\t" << rp->getNormalisableRange().end << "\n";
    return juce::File::getCurrentWorkingDirectory().getChildFile (filnamn).replaceWithText (ut) ? 0 : 1;
}

// En låtsad DAW: spelar, med tempo och position i fjärdedelar.
class LatsadDaw final : public juce::AudioPlayHead
{
public:
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setIsPlaying (true);
        info.setBpm (bpm);
        info.setPpqPosition (ppq);
        info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        return info;
    }
    double bpm = 120.0, ppq = 0.0;
};

// Tempot ska stå still även efter lång tid: tio minuter med egen klocka och med DAW-transport.
void testaLangTakt()
{
    for (int medDaw = 0; medDaw < 2; ++medDaw)
    {
        auto p = nyProcessor();
        p->forceraEgenKlocka = medDaw == 0;
        p->presets.loadByName ("Init");
        p->banker.kolvbasen.rensa();
        satt (*p, km::params::id::trumma ((int) km::Trumma::Bastrumma, "klang"), 0.0f);
        satt (*p, km::params::id::kit, (float) km::Kit::Niononio);
        satt (*p, km::params::id::tempo, 128.0f);
        satt (*p, km::params::id::spela, 1.0f);
        LatsadDaw daw;
        daw.bpm = 128.0;
        if (medDaw) p->setPlayHead (&daw);
        const double sek = 600.0, slagPerSampel = 128.0 / 60.0 / sr;
        const auto ut = rendera (*p, sek, [&] (juce::MidiBuffer&, int pos, int) { daw.ppq = pos * slagPerSampel; });
        p->setPlayHead (nullptr);
        const auto a = ansatser (ut);
        const double fjardedel = sr * 60.0 / 128.0;
        int vart = 0, vartSist = 0;
        for (size_t i = 0; i < a.size(); ++i)
        {
            const int d = (int) std::abs (a[i] - (double) i * fjardedel);
            vart = juce::jmax (vart, d);
            if ((double) a[i] > (sek - 60.0) * sr) vartSist = juce::jmax (vartSist, d);
        }
        const double vantat = sek * 128.0 / 60.0;
        kontroll (std::abs ((double) a.size() - vantat) <= 2.0 && vart <= 4,
                  juce::String (medDaw ? "DAW-transport" : "egen klocka") + juce::String::fromUTF8 (", 10 minuter i 128: ") + juce::String ((int) a.size())
                  + juce::String::fromUTF8 (" slag (väntat ") + juce::String ((int) vantat) + juce::String::fromUTF8 ("), största avvikelse ") + juce::String (vart)
                  + juce::String::fromUTF8 (" samples, sista minuten ") + juce::String (vartSist));
    }
}

// --projekt: spelar en låt enligt spec.json (samma format som gor_reaper_projekt.py skriver).
int spelaProjekt (const char* specFil, const char* utFil)
{
    const auto spec = juce::JSON::parse (juce::File::getCurrentWorkingDirectory().getChildFile (specFil));
    if (! spec.isObject()) { std::cout << "Kunde inte läsa " << specFil << "\n"; return 1; }

    auto p = nyProcessor();
    p->forceraEgenKlocka = false;
    // Läget: samma XML som Reaper-projektet laddar.
    const auto xmlText = spec["state"].toString();
    if (auto xml = juce::parseXML (xmlText))
    {
        juce::MemoryBlock mb;
        juce::AudioProcessor::copyXmlToBinary (*xml, mb);
        p->setStateInformation (mb.getData(), (int) mb.getSize());
    }
    LatsadDaw daw;
    daw.bpm = (double) spec["tempo"];
    p->setPlayHead (&daw);

    struct Punkt { double slag; float varde; };
    struct Kurva { juce::RangedAudioParameter* param; std::vector<Punkt> punkter; };
    std::vector<Kurva> kurvor;
    if (auto* a = spec["automation"].getArray())
        for (auto& k : *a)
        {
            auto* param = dynamic_cast<juce::RangedAudioParameter*> (p->state.getParameter (k["param"].toString()));
            if (param == nullptr) { std::cout << "Okänd parameter i automation: " << k["param"].toString() << "\n"; return 1; }
            Kurva kurva { param, {} };
            if (auto* pts = k["punkter"].getArray())
                for (auto& pt : *pts) kurva.punkter.push_back ({ (double) pt[0], (float) (double) pt[1] });
            kurvor.push_back (std::move (kurva));
        }
    struct Ton { double slag, langd; int notnr; float vel; };
    std::vector<Ton> toner;
    if (auto* n = spec["noter"].getArray())
        for (auto& t : *n) toner.push_back ({ (double) t[0], (double) t[1], (int) t[2], (float) (double) t[3] });

    const double slagPerSampel = daw.bpm / 60.0 / sr;
    const int total = (int) ((double) spec["langdSlag"] / slagPerSampel) + (int) sr;
    juce::AudioBuffer<float> ut (2, total), buf (2, block);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        daw.ppq = pos * slagPerSampel;
        const double slut = daw.ppq + n * slagPerSampel;
        // Automation: värdet i blockets början (trappsteg, som DEFSHAPE 1 i Reaper).
        for (auto& k : kurvor)
        {
            float v = k.punkter.empty() ? 0.0f : k.punkter.front().varde;
            for (auto& pt : k.punkter) if (pt.slag <= daw.ppq) v = pt.varde;
            if (std::abs (k.param->convertFrom0to1 (k.param->getValue()) - v) > 1.0e-6f)
                k.param->setValueNotifyingHost (k.param->convertTo0to1 (v));
        }
        midi.clear();
        for (auto& t : toner)
        {
            if (t.slag >= daw.ppq && t.slag < slut) midi.addEvent (juce::MidiMessage::noteOn (1, t.notnr, t.vel), (int) ((t.slag - daw.ppq) / slagPerSampel));
            const double av = t.slag + t.langd;
            if (av >= daw.ppq && av < slut) midi.addEvent (juce::MidiMessage::noteOff (1, t.notnr), (int) ((av - daw.ppq) / slagPerSampel));
        }
        buf.setSize (2, n, false, false, true);
        p->processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) ut.copyFrom (c, pos, buf, c, 0, n);
    }
    p->setPlayHead (nullptr);
    const auto m = mat (ut);
    std::cout << "Låten: " << juce::String (total / sr, 1) << " s, topp " << juce::String (m.topp, 3) << ", RMS "
              << juce::String (dB (m.rms), 1) << " dB" << (m.finit ? "" : ", NaN!") << "\n";
    skrivWav (ut, juce::File::getCurrentWorkingDirectory().getChildFile (utFil));
    return m.finit && m.topp <= 1.0f ? 0 : 1;
}

// --kanaler: varje fabrikspreset (eller bara de vars namn innehåller 'bara'), mönster 1–4 ett i taget i fyra
// takter, låtläget av: mixen och varje kanal solo. Skriver kanaler_<preset>_m<n>[_<kanal>].wav och nivåerna.
int skrivKanaler (const char* katalogNamn, const char* bara)
{
    using namespace km::params;
    const juce::File katalog = juce::File::getCurrentWorkingDirectory().getChildFile (katalogNamn);
    katalog.createDirectory();
    static const char* kort[] = { "kb", "kb2", "sv", "hu" };
    const auto& fabrik = km::presets::factoryPresets();
    for (int i = 0; i < (int) fabrik.size(); ++i)
    {
        const auto namn = juce::String::fromUTF8 (fabrik[(size_t) i].name);
        if (bara != nullptr && ! namn.containsIgnoreCase (juce::String::fromUTF8 (bara))) continue;
        for (int m = 0; m < 4; ++m)
        {
            auto mall = nyProcessor();
            mall->presets.load (i);
            bool nagot = false;
            for (int v = 0; v < 2; ++v)
                nagot = nagot || ! mall->banker.trummor.get (m, v).arTomt() || ! mall->banker.kolvbasen.get (m, v).arTomt()
                              || ! mall->banker.kolvbas2.get (m, v).arTomt() || ! mall->banker.hugget.get (m, v).arTomt();
            if (! nagot) continue;
            juce::String rad = namn + " m" + juce::String (m + 1) + ":";
            for (int solo = -1; solo < km::antalKanaler; ++solo)
            {
                auto p = nyProcessor();
                p->presets.load (i);
                satt (*p, id::latLage, 0.0f);
                for (int k = 0; k < km::antalKanaler; ++k) satt (*p, id::kanal (k, "monster"), (float) (m + 1));
                if (solo >= 0) satt (*p, id::kanal (solo, "solo"), 1.0f);
                satt (*p, id::spela, 1.0f);
                const auto ut = rendera (*p, verkligaTakter (*p, 4) + 0.3);
                const auto r = mat (ut);
                rad << " " << (solo < 0 ? "mix" : kort[solo]) << " " << juce::String (dB (r.rms), 1);
                if (solo < 0) rad << " (topp " << juce::String (r.topp, 2) << ")";
                skrivWav (ut, katalog.getChildFile ("kanaler_" + filnamn (namn) + "_m" + juce::String (m + 1)
                                                    + (solo < 0 ? juce::String() : "_" + juce::String (kort[solo])) + ".wav"));
            }
            std::cout << rad << "\n";
        }
    }
    return 0;
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc > 3 && juce::String (argv[1]) == "--state") return skrivLage (argc, argv);
    if (argc > 2 && juce::String (argv[1]) == "--parametrar") return skrivParametrar (argv[2]);
    if (argc > 3 && juce::String (argv[1]) == "--projekt") return spelaProjekt (argv[2], argv[3]);
    if (argc > 2 && juce::String (argv[1]) == "--trummor") return skrivTrummor (argv[2], argc > 3 ? argv[3] : nullptr);
    if (argc > 2 && juce::String (argv[1]) == "--kanaler") return skrivKanaler (argv[2], argc > 3 ? argv[3] : nullptr);
    if (argc < 2 || juce::String (argv[1]) == "--hjalp" || juce::String (argv[1]) == "--help")
    {
        std::cout << juce::String::fromUTF8 (
            "KMRender – Kroppsmaskinens renderingstest och lyssningsfiler\n\n"
            "  KMRender --test [katalog]                     alla kontroller; lyssningsfilerna till katalog (renders)\n"
            "  KMRender <katalog>                            samma sak (äldre form)\n"
            "  KMRender --trummor <katalog> [kit 1–5]         varje trumma i varje kit som WAV, med nivåer\n"
            "  KMRender --kanaler <katalog> [preset]          varje presets mönster och kanaler för sig\n"
            "  KMRender --parametrar <fil>                   parametrarnas ordning (för Reaper-skripten)\n"
            "  KMRender --state <preset> <fil.xml> [id=värde]  läget som XML\n"
            "  KMRender --projekt <spec.json> <mix.wav>       spelar en låt i en låtsad DAW\n");
        return 0;
    }
    const bool test = juce::String (argv[1]) == "--test";
    const juce::File katalog = juce::File::getCurrentWorkingDirectory().getChildFile (test ? (argc > 2 ? argv[2] : "renders") : argv[1]);
    katalog.createDirectory();

    testaMonster();
    testaLjud (katalog);
    testaTakt();
    testaLivebyte();
    testaTystSolo();
    testaFolj();
    testaDrivet();
    testaPresets (katalog);
    testaYtterlagen();
    testaFristaende();
    testaHugget (katalog);
    testaGrindrummet();
    testaEkot();
    testaKlamman();
    testaVarvtalet();
    testaSpanningen();
    testaModulationen();
    testaKassetten();
    testaLaten();
    testaDemolatarna (katalog);
    testaLjudbanken();
    testaMidiSpel();
    testaStamningen();
    testaGjuteriet();
    testaStamningICent();
    testaKlick();
    testaCpu();

    std::cout << "\n" << antalKontroller << " kontroller, ";
    std::cout << (fel == 0 ? juce::String::fromUTF8 ("alla går igenom.\n") : juce::String (fel) + juce::String::fromUTF8 (" går inte igenom.\n"));
    std::cout << "Filerna ligger i " << katalog.getFullPathName() << "\n";
    return fel == 0 ? 0 : 1;
}
