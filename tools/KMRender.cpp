// KMRender – renderingstest och lyssningsfiler, utan DAW.
//
//   KMRender [katalog]
//
// Kör testerna nedan och skriver WAV-filer till katalogen (standard: renders/). Avslutar med kod 1 om något
// test misslyckas, så att CI blir rött.
//
//   1. Mönsterformatet: alla fabrikspresets överlever toString → fromString.
//   2. Varje ljud för sig: inga NaN, topp högst 0 dBFS, ingen DC, tyst efter släpp.
//   3. Taktverket: bastrumman slår exakt på sina steg (±2 samples) vid 125 BPM.
//   4. Varje fabrikspreset, åtta takter: inga NaN, topp högst 0 dBFS, hörbar nivå. Skrivs som WAV.
//   5. Ytterlägen: 40 slumpade inställningar av alla rattar, en takt var: inga NaN, topp högst 0 dBFS.
//
// Grind 0-loopen hamnar i renders/grind0_kolv125.wav.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 512;
int fel = 0;

void kontroll (bool ok, const juce::String& vad)
{
    std::cout << (ok ? "  ok   " : "  FEL  ") << vad << "\n";
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

void testaMonster()
{
    std::cout << "1. Mönsterformatet\n";
    auto p = nyProcessor();
    for (int i = 0; i < p->presets.getNumFactory(); ++i)
    {
        p->presets.load (i);
        km::TrumMonster t2; km::TonMonster b2;
        t2.fromString (p->trummor.toString());
        b2.fromString (p->kolvbas.toString());
        bool lika = true;
        for (int s = 0; s < km::maxSteg; ++s)
        {
            if (! (b2.get (s) == p->kolvbas.get (s))) lika = false;
            for (int spr = 0; spr < km::antalSpar; ++spr)
                if (! (t2.get (spr, s) == p->trummor.get (spr, s))) lika = false;
        }
        kontroll (lika, p->presets.getEntries()[(size_t) i].name);
    }

    // Tonsteg med alla tillägg.
    km::TonMonster b;
    b.fromString ("-12d 12u 3a~/0 _5/7 .");
    km::TonMonster b2;
    b2.fromString (b.toString (5));
    bool lika = true;
    for (int s = 0; s < 5; ++s) lika = lika && b.get (s) == b2.get (s);
    kontroll (lika && b.get (0).tonhojd() == -24 && b.get (1).tonhojd() == 24 && b.get (2).glid && b.get (3).slag == 0,
              juce::String::fromUTF8 ("tonsteg med oktav, glid, längd och tyst steg"));
}

void testaLjud (const juce::File& katalog)
{
    std::cout << "2. Varje ljud\n";
    struct Fall { const char* namn; int kanal, ton; };
    for (auto f : { Fall { "Kolvbasen", 1, 33 }, Fall { "Kolvbasen hög", 1, 69 }, Fall { "Bastrumma", 10, 36 },
                    Fall { "Virvel", 10, 38 }, Fall { "Hi-hat", 10, 42 } })
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, km::params::id::spela, 0.0f);
        satt (*p, km::params::id::kbResonans, 0.9f);
        auto ut = rendera (*p, 2.5, [f] (juce::MidiBuffer& m, int pos, int n)
        {
            const int pa = 0, av = (int) (0.6 * sr);
            if (pa >= pos && pa < pos + n) m.addEvent (juce::MidiMessage::noteOn (f.kanal, f.ton, 1.0f), pa - pos);
            if (av >= pos && av < pos + n) m.addEvent (juce::MidiMessage::noteOff (f.kanal, f.ton), av - pos);
        });
        const auto hela = mat (ut);
        const auto slut = mat (ut, ut.getNumSamples() - (int) (0.2 * sr));
        const juce::String n = juce::String::fromUTF8 (f.namn);
        kontroll (hela.finit, n + ": inga NaN");
        kontroll (hela.topp <= 1.0f && hela.topp > 0.05f, n + ": topp " + juce::String (hela.topp, 3));
        kontroll (std::abs (hela.medel) < 0.01, n + ": DC " + juce::String (hela.medel, 5));
        kontroll (slut.topp < 1.0e-3f, n + ": tyst efter släpp (" + juce::String (slut.topp, 6) + ")");
        skrivWav (ut, katalog.getChildFile ("ljud_" + filnamn (n) + ".wav"));
    }
}

void testaTakt()
{
    std::cout << "3. Taktverket\n";
    auto p = nyProcessor();
    p->presets.loadByName ("Init");
    p->kolvbas.rensa();
    satt (*p, km::params::id::trumma (0, "klang"), 0.0f);   // kort bastrumma, så att varje slag börjar i tystnad
    satt (*p, km::params::id::tempo, 125.0f);
    satt (*p, km::params::id::spela, 1.0f);
    auto ut = rendera (*p, 4.0);

    // Ansatser: första samplet över tröskeln efter minst 50 ms nära tystnad.
    std::vector<int> ansatser;
    int tyst = 100000;
    for (int i = 0; i < ut.getNumSamples(); ++i)
    {
        const float a = std::abs (ut.getSample (0, i));
        if (a > 0.02f && tyst > (int) (0.05 * sr)) ansatser.push_back (i);
        tyst = a < 0.004f ? tyst + 1 : 0;
    }
    // Init: bastrumma på varje fjärdedel. En fjärdedel vid 125 BPM = 0,48 s.
    const double fjardedel = sr * 60.0 / 125.0;
    bool ok = ansatser.size() >= 7;
    int vart = 0;
    for (size_t i = 0; i < ansatser.size(); ++i)
    {
        const double vantad = (double) i * fjardedel;
        vart = juce::jmax (vart, (int) std::abs (ansatser[i] - vantad));
    }
    ok = ok && vart <= 2;
    kontroll (ok, juce::String ((int) ansatser.size()) + " slag, största avvikelse " + juce::String (vart) + " samples");
}

void testaPresets (const juce::File& katalog)
{
    std::cout << "4. Fabriksbandet\n";
    auto mall = nyProcessor();
    for (int i = 0; i < mall->presets.getNumFactory(); ++i)
    {
        auto p = nyProcessor();
        p->presets.load (i);
        satt (*p, km::params::id::spela, 1.0f);
        const auto namn = p->presets.getEntries()[(size_t) i].name;
        const double bpm = (double) *p->state.getRawParameterValue (km::params::id::tempo);
        auto ut = rendera (*p, 8 * 4 * 60.0 / bpm + 0.5);
        const auto m = mat (ut);
        kontroll (m.finit && m.topp <= 1.0f && m.rms > 0.02,
                  namn + ": topp " + juce::String (m.topp, 3) + ", RMS " + juce::String (juce::Decibels::gainToDecibels ((float) m.rms), 1) + " dB");
        skrivWav (ut, katalog.getChildFile (filnamn (namn) + ".wav"));
        if (namn == "Kolv 125") skrivWav (ut, katalog.getChildFile ("grind0_kolv125.wav"));
    }

    // Grind 0-loopen som separata spår, för att kunna lyssna på (och mäta) basen och trummorna var för sig.
    for (auto [stam, tyst] : { std::pair { "bas", km::params::id::trumNiva }, std::pair { "trummor", km::params::id::basNiva } })
    {
        auto p = nyProcessor();
        p->presets.loadByName ("Kolv 125");
        satt (*p, km::params::id::spela, 1.0f);
        satt (*p, tyst, 0.0f);
        auto ut = rendera (*p, 8 * 4 * 60.0 / 125.0 + 0.5);
        const auto m = mat (ut);
        std::cout << "       grind 0, bara " << stam << ": topp " << juce::String (m.topp, 3)
                  << ", RMS " << juce::String (juce::Decibels::gainToDecibels ((float) m.rms), 1) << " dB\n";
        skrivWav (ut, katalog.getChildFile (juce::String ("grind0_kolv125_") + stam + ".wav"));
    }
}

void testaYtterlagen()
{
    std::cout << "5. Ytterlägen\n";
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
            if (rp == nullptr || rp->getParameterID() == km::params::id::spela) continue;
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
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::File katalog = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "renders");
    katalog.createDirectory();

    testaMonster();
    testaLjud (katalog);
    testaTakt();
    testaPresets (katalog);
    testaYtterlagen();

    std::cout << (fel == 0 ? "\nAlla test gick igenom.\n" : "\n" + std::to_string (fel) + " test misslyckades.\n");
    std::cout << "Filerna ligger i " << katalog.getFullPathName() << "\n";
    return fel == 0 ? 0 : 1;
}
