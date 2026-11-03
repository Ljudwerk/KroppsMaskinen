// Reglagen på VHS-panelen: ratt, knapp, väljare, spak, tempodisplay och en fri tryckyta.
//
// Alla är vanliga komponenter i designkoordinater (panelen skalas som helhet) och kopplas till
// parametrarna med juce::ParameterAttachment, så att DAW:en ser gesterna (automation, ångra).
// Varje reglage ritar sig själv med etiketten under; bakgrunden (chassit, fälten, grupprubrikerna)
// ligger i panelens cachade bild. Rör man ett reglage visas namnet och värdet i presetdisplayen.
//
// Copyright (C) 2026 Kroppsmaskinen-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../stil/Palett.h"
#include "../stil/Hardvara.h"
#include "../stil/Linjeskrift.h"
#include "../ValueFormat.h"

namespace km::vhs
{

using juce::Rectangle;
using juce::Point;
namespace farg = km::stil::farg;

// Versaler, även å, ä och ö (toUpperCase klarar dem inte på alla system).
inline juce::String versaler (const juce::String& s)
{
    return s.toUpperCase().replace (stil::sv ("å"), stil::sv ("Å")).replace (stil::sv ("ä"), stil::sv ("Ä"))
            .replace (stil::sv ("ö"), stil::sv ("Ö")).replace (stil::sv ("é"), stil::sv ("É"));
}

// Silvertryck som krymper spärrningen (och till sist höjden) tills texten får plats i rutan.
inline void etikett (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, float hojd = 4.8f,
                     juce::Colour f = farg::silver, juce::Justification just = juce::Justification::centred)
{
    stil::linjeskrift::Stil s { hojd, 0.75f, 1.6f, 1.0f };
    while (stil::linjeskrift::bredd (text, s) > ruta.getWidth() && s.spar > 0.6f) s.spar -= 0.2f;
    while (stil::linjeskrift::bredd (text, s) > ruta.getWidth() && s.smal > 0.7f) s.smal -= 0.05f;
    g.setColour (f);
    stil::linjeskrift::rita (g, text, ruta, s, just);
}

// En sparad bild av något som sällan ändras (VU-tavlan, kanalknapparna), i fysisk upplösning.
// Ritas om bara när nyckeln eller skalan ändras.
struct Bildminne
{
    juce::Image bild;
    juce::int64 nyckel = -1;
    float skala = 0.0f;

    void rita (juce::Graphics& g, Rectangle<float> r, juce::int64 k, const std::function<void (juce::Graphics&, Rectangle<float>)>& f)
    {
        const float s = juce::jmax (0.25f, g.getInternalContext().getPhysicalPixelScaleFactor());
        if (k != nyckel || std::abs (s - skala) > 0.001f || ! bild.isValid())
        {
            nyckel = k; skala = s;
            bild = juce::Image (juce::Image::ARGB, juce::jmax (1, (int) std::ceil (r.getWidth() * s)), juce::jmax (1, (int) std::ceil (r.getHeight() * s)), true);
            juce::Graphics gi (bild);
            gi.addTransform (juce::AffineTransform::translation (-r.getX(), -r.getY()).scaled (s));
            f (gi, r);
        }
        juce::Graphics::ScopedSaveState spara (g);
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImageTransformed (bild, juce::AffineTransform::scale (1.0f / s).translated (r.getX(), r.getY()));
    }
};

// Det som reglagen delar: vart "senast rörda" ska visas, och att panelen ska veta att något ändrats.
struct Visare
{
    std::function<void (const juce::String& namn, const juce::String& varde)> visa;
    void operator() (const juce::String& namn, const juce::String& varde) const { if (visa) visa (namn, varde); }
};

// Basklass: ett reglage i rattpanelen. Höjden är alltid 60 (med värdet under etiketten); cellbredden varierar.
class Reglage : public juce::Component
{
public:
    static constexpr int hojd = 60;
    int bredd = 0;                                   // > 0: cellbredd bestämd av layouten
    int cellBredd() const { return bredd > 0 ? bredd : naturligBredd(); }
    virtual int naturligBredd() const { return 58; }
    juce::String etikettText;

protected:
    void ritaEtikett (juce::Graphics& g) const
    {
        if (etikettText.isNotEmpty())
            etikett (g, etikettText, { 1.0f, 40.0f, (float) getWidth() - 2.0f, 8.0f });
    }
};

// Gemensamt för reglage som styr en parameter.
class ParamReglage : public Reglage
{
public:
    ParamReglage (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& text, const Visare& v)
        : param (*state.getParameter (id)), visare (v),
          koppling (param, [this] (float x) { varde = x; repaint(); }, state.undoManager)
    {
        etikettText = versaler (text);
        setRepaintsOnMouseActivity (false);
        koppling.sendInitialUpdate();
    }

    float norm() const { return param.convertTo0to1 (varde); }
    juce::String paramId() const { return param.getParameterID(); }
    void visaVarde() const { visare (param.getName (40), ui::formatValue (param, varde)); }

protected:
    void sattNorm (float n, bool helGest)
    {
        const float v = param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n));
        if (helGest) koppling.setValueAsCompleteGesture (v);
        else         koppling.setValueAsPartOfGesture (v);
        visaVarde();
    }

    juce::RangedAudioParameter& param;
    const Visare& visare;
    juce::ParameterAttachment koppling;
    float varde = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Ratt: dra upp/ned (Skift = fint), rulla, dubbelklicka för grundvärdet.

class Ratt final : public ParamReglage
{
public:
    Ratt (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, const Visare& v,
          float radie = 12.0f, stil::RattTyp typ = stil::RattTyp::liten, bool inverterad = false)
        : ParamReglage (s, id, text, v), r (radie), rattTyp (typ), inv (inverterad) {}

    int naturligBredd() const override { return r > 14.0f ? (int) (r * 3.4f) : 58; }

    bool visaVardet = false;   // värdet under etiketten (rattpanelen och vyerna)

    // Höjden som ratten behöver: ratt, etikett och värde.
    int hojdBehov() const { return r > 14.0f ? (int) (r * 2.9f + 22.0f + (visaVardet ? 10.0f : 0.0f)) : hojd; }

    void paint (juce::Graphics& g) override
    {
        const float n = inv ? 1.0f - norm() : norm();
        stil::ritaRatt (g, centrum(), r, n, rattTyp);
        float y = 40.0f;
        if (r > 14.0f) { y = centrum().y + r * 1.42f + 5.0f; etikett (g, etikettText, { 0.0f, y, (float) getWidth(), 8.0f }, 5.4f); }
        else ritaEtikett (g);
        if (visaVardet)
            etikett (g, versaler (ui::formatValue (param, varde)), { 1.0f, y + 10.0f, (float) getWidth() - 2.0f, 7.0f }, 5.0f, farg::lysOrange.withAlpha (0.85f));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        startNorm = norm();
        startY = e.position.y;
        koppling.beginGesture();
        visaVarde();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float d = (startY - e.position.y) / 180.0f * (e.mods.isShiftDown() ? 0.15f : 1.0f);
        sattNorm (startNorm + (inv ? -d : d), false);
    }
    void mouseUp (const juce::MouseEvent&) override { koppling.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { sattNorm (param.getDefaultValue(), true); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const int steg = param.getNumSteps();
        float d = steg > 1 && steg < 200 ? 1.0f / (float) (steg - 1) : 0.02f;
        if (e.mods.isShiftDown()) d *= 0.2f;
        const float riktning = (std::abs (w.deltaY) > 0.0f ? w.deltaY : w.deltaX) > 0.0f ? 1.0f : -1.0f;
        sattNorm (norm() + riktning * d * (inv ? -1.0f : 1.0f) * (w.isReversed ? -1.0f : 1.0f), true);
    }

private:
    Point<float> centrum() const { return { (float) getWidth() * 0.5f, r > 14.0f ? r * 1.45f : 19.0f }; }
    float r;
    stil::RattTyp rattTyp;
    bool inv;
    float startNorm = 0.0f, startY = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Fader: lodrät, dra (Skift = fint), rulla, dubbelklicka för grundvärdet. Etikett och värde under.

class Fader final : public ParamReglage
{
public:
    Fader (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, const Visare& v)
        : ParamReglage (s, id, text, v) {}

    int naturligBredd() const override { return 44; }
    Rectangle<float> spar() const { return { 0.0f, 0.0f, (float) getWidth(), (float) getHeight() - 22.0f }; }

    void paint (juce::Graphics& g) override
    {
        const auto r = spar();
        stil::ritaFader (g, r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 34.0f), r.getHeight()), norm());
        const float y = r.getBottom() + 3.0f;
        etikett (g, etikettText, { 0.0f, y, (float) getWidth(), 8.0f }, 5.2f);
        etikett (g, versaler (ui::formatValue (param, varde)), { 0.0f, y + 10.0f, (float) getWidth(), 7.0f }, 5.0f, farg::lysOrange.withAlpha (0.85f));
    }
    void mouseDown (const juce::MouseEvent& e) override { startNorm = norm(); startY = e.position.y; koppling.beginGesture(); visaVarde(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float h = juce::jmax (20.0f, spar().getHeight() - 18.0f);
        sattNorm (startNorm + (startY - e.position.y) / h * (e.mods.isShiftDown() ? 0.15f : 1.0f), false);
    }
    void mouseUp (const juce::MouseEvent&) override { koppling.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { sattNorm (param.getDefaultValue(), true); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        sattNorm (norm() + ((w.deltaY > 0.0f) != w.isReversed ? 0.02f : -0.02f), true);
    }

private:
    float startNorm = 0.0f, startY = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Knapp: av/på för en parameter, med lysdiod ovanför.

class Knapp final : public ParamReglage
{
public:
    Knapp (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, const Visare& v,
           juce::Colour diod = farg::lysRod)
        : ParamReglage (s, id, text, v), diodFarg (diod) {}

    int naturligBredd() const override { return 50; }

    void paint (juce::Graphics& g) override
    {
        const bool pa = norm() > 0.5f;
        const float cx = (float) getWidth() * 0.5f;
        stil::ritaLysdiod (g, { cx, 7.0f }, 2.3f, diodFarg, pa ? 1.0f : 0.0f);
        stil::ritaKnapp (g, { cx - 16.0f, 14.0f, 32.0f, 14.0f }, pa);
        ritaEtikett (g);
    }
    void mouseDown (const juce::MouseEvent&) override { sattNorm (norm() > 0.5f ? 0.0f : 1.0f, true); }

private:
    juce::Colour diodFarg;
};

// ---------------------------------------------------------------------------------------------
// Väljare: ett litet fönster med valt värde. Klick = nästa (Skift = förra), rulla, högerklick = lista.

class Valjare final : public ParamReglage
{
public:
    Valjare (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text, const Visare& v, int cellB = 66)
        : ParamReglage (s, id, text, v), cell (cellB) {}

    int naturligBredd() const override { return cell; }

    int antal() const { return juce::jmax (2, param.getNumSteps()); }
    int index() const { return juce::roundToInt (norm() * (float) (antal() - 1)); }
    void sattIndex (int i) { sattNorm ((float) juce::jlimit (0, antal() - 1, i) / (float) (antal() - 1), true); }

    void paint (juce::Graphics& g) override
    {
        const auto f = Rectangle<float> (3.0f, 10.0f, (float) getWidth() - 6.0f, 17.0f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (f.expanded (1.5f), 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (f.expanded (2.0f).translated (0.0f, 0.5f), 3.0f, 0.6f);
        g.setColour (juce::Colour (0xff120604));
        g.fillRoundedRectangle (f, 2.0f);
        etikett (g, versaler (param.getText (norm(), 0)), f.reduced (4.0f, 0.0f), 5.0f, farg::lysOrange);
        // Små pilar: det finns fler värden.
        g.setColour (farg::lysOrange.withAlpha (0.35f));
        juce::Path p;
        p.addTriangle (f.getRight() - 4.0f, f.getY() + 3.0f, f.getRight() - 1.5f, f.getY() + 3.0f, f.getRight() - 2.75f, f.getY() + 1.0f);
        p.addTriangle (f.getRight() - 4.0f, f.getBottom() - 3.0f, f.getRight() - 1.5f, f.getBottom() - 3.0f, f.getRight() - 2.75f, f.getBottom() - 1.0f);
        g.fillPath (p);
        ritaEtikett (g);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu m;
            for (int i = 0; i < antal(); ++i)
                m.addItem (i + 1, param.getText ((float) i / (float) (antal() - 1), 0), true, i == index());
            juce::Component::SafePointer<Valjare> sp (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                             [sp] (int r) { if (sp != nullptr && r > 0) sp->sattIndex (r - 1); });
            return;
        }
        const int n = antal();
        sattIndex ((index() + (e.mods.isShiftDown() ? n - 1 : 1)) % n);
    }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        sattIndex (index() + ((w.deltaY > 0.0f) != w.isReversed ? 1 : -1));
    }

private:
    int cell;
};

// ---------------------------------------------------------------------------------------------
// Spak (Spänningen): vågrät, dra i sidled.

class Spak final : public ParamReglage
{
public:
    Spak (juce::AudioProcessorValueTreeState& s, const juce::String& id, const Visare& v) : ParamReglage (s, id, {}, v) {}

    void paint (juce::Graphics& g) override { stil::ritaSpak (g, getLocalBounds().toFloat(), norm()); }
    void mouseDown (const juce::MouseEvent& e) override { koppling.beginGesture(); flytta (e); }
    void mouseDrag (const juce::MouseEvent& e) override { flytta (e); }
    void mouseUp (const juce::MouseEvent&) override { koppling.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { sattNorm (param.getDefaultValue(), true); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        sattNorm (norm() + ((w.deltaY > 0.0f) != w.isReversed ? 0.02f : -0.02f), true);
    }

private:
    void flytta (const juce::MouseEvent& e)
    {
        const float marg = 14.0f;
        sattNorm ((e.position.x - marg) / ((float) getWidth() - 2.0f * marg), false);
    }
};

// ---------------------------------------------------------------------------------------------
// Sjusegmentsdisplay som styr en parameter: dra upp/ned (Skift = fint), rulla, dubbelklicka.
// Tempo med en decimal; andra heltal.

class SegmentReglage final : public ParamReglage
{
public:
    SegmentReglage (juce::AudioProcessorValueTreeState& s, const juce::String& id, const Visare& v, int decimaler, float pixlarPerEnhet)
        : ParamReglage (s, id, {}, v), dec (decimaler), ppe (pixlarPerEnhet) {}

    std::function<juce::String (float)> text;   // valfri egen formatering

    void paint (juce::Graphics& g) override
    {
        stil::ritaSjusegment (g, getLocalBounds().toFloat(), text ? text (varde) : juce::String (varde, dec));
    }
    void mouseDown (const juce::MouseEvent& e) override { startV = varde; startY = e.position.y; koppling.beginGesture(); visaVarde(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float d = (startY - e.position.y) / ppe * (e.mods.isShiftDown() ? (dec > 0 ? 0.1f : 0.25f) : 1.0f);
        const float v = param.getNormalisableRange().snapToLegalValue (startV + (dec > 0 ? d : std::round (d)));
        koppling.setValueAsPartOfGesture (v);
        visaVarde();
    }
    void mouseUp (const juce::MouseEvent&) override { koppling.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { sattNorm (param.getDefaultValue(), true); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const float steg = (dec > 0 && e.mods.isShiftDown()) ? 0.1f : 1.0f;
        koppling.setValueAsCompleteGesture (param.getNormalisableRange().snapToLegalValue (varde + ((w.deltaY > 0.0f) != w.isReversed ? steg : -steg)));
        visaVarde();
    }

private:
    int dec;
    float ppe, startV = 0.0f, startY = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Tryckyta: valfritt utseende och beteende. 'lage' returnerar ett tal som ändras när ytan ska ritas
// om; panelens timer anropar kolla() och ritar bara om när talet har ändrats.

class Tryckyta final : public Reglage
{
public:
    std::function<void (juce::Graphics&, Rectangle<float>)> rita;
    std::function<void (const juce::MouseEvent&)> tryck, slapp, drag;
    std::function<juce::int64()> lage;
    void paint (juce::Graphics& g) override { if (rita) rita (g, getLocalBounds().toFloat()); }
    void mouseDown (const juce::MouseEvent& e) override { if (tryck) tryck (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (slapp) slapp (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (drag) drag (e); }

    void kolla()
    {
        if (! lage || ! isVisible()) return;
        const auto nu = lage();
        if (nu != senast) { senast = nu; repaint(); }
    }

private:
    juce::int64 senast = -0x7fffffff;
};

// En knapp i rattpanelen som utför något (KOPIERA, RENSA): lysdiod, knapp och etikett.
inline std::unique_ptr<Tryckyta> handlingsknapp (const juce::String& text, std::function<void()> gor, std::function<bool()> lyser = {})
{
    auto t = std::make_unique<Tryckyta>();
    t->etikettText = versaler (text);
    t->bredd = 50;
    auto* p = t.get();
    auto nere = std::make_shared<bool> (false);
    t->rita = [p, nere, lyser] (juce::Graphics& g, Rectangle<float> r)
    {
        const float cx = r.getCentreX();
        stil::ritaLysdiod (g, { cx, 7.0f }, 2.3f, farg::lysOrange, lyser && lyser() ? 1.0f : 0.0f);
        stil::ritaKnapp (g, { cx - 16.0f, 14.0f, 32.0f, 14.0f }, *nere);
        etikett (g, p->etikettText, { 1.0f, 40.0f, r.getWidth() - 2.0f, 8.0f });
    };
    t->tryck = [p, nere] (const juce::MouseEvent&) { *nere = true; p->repaint(); };
    t->slapp = [p, nere, gor] (const juce::MouseEvent& e)
    {
        *nere = false;
        p->repaint();
        if (p->getLocalBounds().contains (e.getPosition()) && gor) gor();
    };
    if (lyser) t->lage = [lyser] { return (juce::int64) lyser(); };
    return t;
}

} // namespace km::vhs
