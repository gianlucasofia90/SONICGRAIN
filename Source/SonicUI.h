#pragma once
// SONIC GRAIN UI parts, v2 look: glass panels on a blue ground, slanted black plates, dark knobs with a
// yellow value arc, pixel-font readouts. Palette and sizes follow the SONIC GRAIN design system.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SonicProcessor.h"
#include "SonicFonts.h"

namespace sonic
{
namespace c
{
    const juce::Colour ground    { 0xff1784e8 };
    const juce::Colour groundAlt { 0xff31aac6 };
    const juce::Colour modGround { 0xff0f7a3c };
    const juce::Colour glass     { 0xccecf8ff };   // rgba(236, 248, 255, 0.80)
    const juce::Colour glassEdge { 0xe6ffffff };
    const juce::Colour ink       { 0xff0b1430 };
    const juce::Colour inkSoft   { 0xff33415f };
    const juce::Colour plate     { 0xff111111 };
    const juce::Colour onPlate   { 0xffffffff };
    const juce::Colour accent    { 0xffffd21f };
    const juce::Colour onAccent  { 0xff0b1430 };
    const juce::Colour go        { 0xff1f9e1a };
    const juce::Colour signal    { 0xffe90101 };
    const juce::Colour tag       { 0xff0123c0 };
    const juce::Colour orbitA    { 0xfff77129 };
    const juce::Colour orbitB    { 0xff52dba5 };
    const juce::Colour knob      { 0xff16212f };
    const juce::Colour knobHi    { 0xff3a4b63 };
    const juce::Colour track     { 0x240b1430 };   // ink at 14 %
    const juce::Colour trackDark { 0x24ffffff };   // white at 14 %, for controls on plates and the deck
    const juce::Colour display   { 0xe0082660 };   // rgba(8, 38, 96, 0.88)
    const juce::Colour wave      { 0xffffffff };
    const juce::Colour checker   { 0x1affffff };
    const juce::Colour grid      { 0x2effffff };
}

/** Embedded faces (Rubik, Silkscreen; SIL Open Font License, see Resources/fonts). */
struct Fonts
{
    static Fonts& get() { static Fonts f; return f; }
    juce::Typeface::Ptr medium, bold, black, pixel;
private:
    Fonts()
    {
        medium = juce::Typeface::createSystemTypefaceFor (SonicFonts::RubikMedium_ttf, SonicFonts::RubikMedium_ttfSize);
        bold   = juce::Typeface::createSystemTypefaceFor (SonicFonts::RubikBold_ttf, SonicFonts::RubikBold_ttfSize);
        black  = juce::Typeface::createSystemTypefaceFor (SonicFonts::RubikBlackItalic_ttf, SonicFonts::RubikBlackItalic_ttfSize);
        pixel  = juce::Typeface::createSystemTypefaceFor (SonicFonts::SilkscreenRegular_ttf, SonicFonts::SilkscreenRegular_ttfSize);
    }
};

inline juce::Font face (const juce::Typeface::Ptr& t, float size, float tracking = 0.0f)
{
    return juce::Font (juce::FontOptions (t).withHeight (size)).withExtraKerningFactor (tracking);
}
inline juce::Font medium (float size, float tracking = 0.0f) { return face (Fonts::get().medium, size, tracking); }
inline juce::Font bold (float size, float tracking = 0.0f)   { return face (Fonts::get().bold, size, tracking); }
inline juce::Font black (float size, float tracking = 0.0f)  { return face (Fonts::get().black, size, tracking); }
inline juce::Font pixel (float size)                         { return face (Fonts::get().pixel, size); }

/** A rectangle skewed by -30 degrees: the plate every button, tab and tag is drawn on. */
inline juce::Path platePath (juce::Rectangle<float> r)
{
    const float lean = juce::jmin (r.getHeight() * 0.577f, r.getWidth() * 0.3f);
    juce::Path p;
    p.addQuadrilateral (r.getX() + lean, r.getY(), r.getRight(), r.getY(),
                        r.getRight() - lean, r.getBottom(), r.getX(), r.getBottom());
    return p;
}

/** Plate with the stacked hard shadow (signal, then go) used by section tags and the preset tag. */
inline void taggedPlate (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill)
{
    g.setColour (c::go);     g.fillPath (platePath (r.translated (7, 7)));
    g.setColour (c::signal); g.fillPath (platePath (r.translated (4, 4)));
    g.setColour (fill);      g.fillPath (platePath (r));
}

/** Glass panel with its two-line title: kicker word, then the section word on a tagged plate. */
inline void glassPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& kicker, const juce::String& title)
{
    juce::Path body;
    body.addRoundedRectangle (r, 10.0f);
    juce::DropShadow (juce::Colour (0x47041e5a), 24, { 0, 10 }).drawForPath (g, body);
    g.setColour (c::glass);
    g.fillPath (body);
    g.setColour (c::glassEdge);
    g.strokePath (body, juce::PathStrokeType (1.0f));

    g.setColour (c::inkSoft);
    g.setFont (medium (10.0f, 0.3f));
    g.drawText (kicker.toUpperCase(), (int) r.getX() + 16, (int) r.getY() + 12, 200, 12, juce::Justification::left);
    const auto f = bold (13.0f, 0.22f);
    const float tw = juce::GlyphArrangement::getStringWidth (f, title.toUpperCase()) + 36.0f;
    auto tr = juce::Rectangle<float> (r.getX() + 16, r.getY() + 26, tw, 22);   // shadow ends 7 px lower, content starts at +56
    taggedPlate (g, tr, c::plate);
    g.setColour (c::onPlate);
    g.setFont (f);
    g.drawText (title.toUpperCase(), tr.reduced (14, 0), juce::Justification::centred);
}

/** The shape of LFO shape index s at phase p, -1..1 (drawing only). */
inline float lfoShapeValue (int s, float p)
{
    static const float v[9] = { 0.6f, -0.3f, 0.9f, -0.8f, 0.2f, -0.5f, 0.7f, -0.1f, 0.6f };
    switch (s)
    {
        case 0: return std::sin (6.2831853f * p);
        case 1: return 1.0f - 4.0f * std::abs (p - 0.5f);
        case 2: return 2.0f * p - 1.0f;
        case 3: return 1.0f - 2.0f * p;
        case 4: return p < 0.5f ? 1.0f : -1.0f;
        case 5: return v[juce::jlimit (0, 7, (int) (p * 8))];
        default:
        {
            const float x = p * 8; const int i = juce::jlimit (0, 7, (int) x); const float f = x - (float) i;
            return v[i] + (v[i + 1] - v[i]) * (0.5f - 0.5f * std::cos (3.14159265f * f));
        }
    }
}

/** Slider that can show a live modulated value without touching its own value (the parameter stays where
    the user left it). While the user holds the control it shows the real value, so it can be set precisely. */
class ModSlider : public juce::Slider
{
public:
    void setModulation (bool active, double value)
    {
        const bool show = active && ! isMouseButtonDown();
        if (show == modShown && (! show || value == modValue)) return;
        modShown = show;
        modValue = value;
        repaint();
        if (! isEditingText()) updateText();
    }

    bool isModulationShown() const { return modShown; }
    double getModulatedValue() const { return modValue; }

    juce::String getTextFromValue (double v) override { return juce::Slider::getTextFromValue (modShown ? modValue : v); }

private:
    bool isEditingText() const
    {
        for (auto* child : getChildren())
            if (auto* l = dynamic_cast<juce::Label*> (child))
                if (l->isBeingEdited()) return true;
        return false;
    }

    bool modShown = false;
    double modValue = 0.0;
};

/** Controls on a dark ground (header plates, modulation deck) carry this property. */
inline bool isOnDark (const juce::Component& comp)
{
    for (auto* p = &comp; p != nullptr; p = p->getParentComponent())
        if (p->getProperties()["sgDark"]) return true;
    return false;
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr int kValueH = 13, kCaptionH = 15;   // knob readout and caption rows

    LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, c::ink);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, c::accent);
        setColour (juce::Label::textColourId, c::ink);
        setColour (juce::ComboBox::textColourId, c::ink);
        setColour (juce::ComboBox::arrowColourId, c::ink);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xffecf8ff));
        setColour (juce::PopupMenu::textColourId, c::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, c::plate);
        setColour (juce::PopupMenu::highlightedTextColourId, c::onPlate);
        setColour (juce::TextButton::buttonColourId, c::plate);
        setColour (juce::TextButton::buttonOnColourId, c::accent);
        setColour (juce::TextButton::textColourOffId, c::onPlate);
        setColour (juce::TextButton::textColourOnId, c::onAccent);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xffecf8ff));
        setColour (juce::AlertWindow::textColourId, c::ink);
        setColour (juce::AlertWindow::outlineColourId, c::plate);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
        setColour (juce::TextEditor::textColourId, c::ink);
        setColour (juce::TextEditor::highlightColourId, c::accent);
        setColour (juce::TextEditor::outlineColourId, c::inkSoft);
        setColour (juce::TextEditor::focusedOutlineColourId, c::ink);
        setColour (juce::CaretComponent::caretColourId, c::ink);
    }

    juce::Font getComboBoxFont (juce::ComboBox& b) override { return medium (b.getWidth() < 62 ? 10.5f : 12.0f); }
    juce::Font getPopupMenuFont() override { return medium (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return black (juce::jmin (12.5f, h * 0.55f), 0.04f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }
    juce::Font getAlertWindowTitleFont() override { return bold (15.0f); }
    juce::Font getAlertWindowMessageFont() override { return medium (13.0f); }
    juce::Font getAlertWindowFont() override { return medium (12.0f); }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (pixel (10.0f));
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, isOnDark (s) ? c::onPlate : c::ink);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colours::white);
        l->setColour (juce::Label::textWhenEditingColourId, c::ink);
        l->setColour (juce::Label::outlineWhenEditingColourId, c::ink);
        return l;
    }

    // rotary knobs: knob on top, then a gap for the caption (a sibling Label), then the value
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& s) override
    {
        if (! s.isRotary()) return juce::LookAndFeel_V4::getSliderLayout (s);
        juce::Slider::SliderLayout l;
        auto r = s.getLocalBounds();
        l.textBoxBounds = r.removeFromBottom (kValueH);
        r.removeFromBottom (kCaptionH);
        l.sliderBounds = r;
        return l;
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1,
                           juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto ctr = r.getCentre();
        const float ar = size * 0.5f - 3.0f, br = size * 0.5f - 9.0f;
        const bool dark = isOnDark (s);
        auto arc = [&] (float rad, float from, float to, juce::Colour col, float width)
        {
            juce::Path p;
            p.addCentredArc (ctr.x, ctr.y, rad, rad, 0, from, to, true);
            g.setColour (col);
            g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        // under modulation the pointer follows the live value; the go arc shows the distance from the set
        // value, which keeps its accent arc and gets a tick
        const float baseAng = a0 + pos * (a1 - a0);
        float ang = baseAng;
        auto* ms = dynamic_cast<ModSlider*> (&s);
        const bool modulated = ms != nullptr && ms->isModulationShown();
        if (modulated) ang = a0 + (float) s.valueToProportionOfLength (ms->getModulatedValue()) * (a1 - a0);

        arc (ar, a0, a1, dark ? c::trackDark : c::track, 4.0f);
        if (pos > 0.002f) arc (ar, a0, baseAng, c::accent, 4.0f);
        if (modulated && std::abs (ang - baseAng) > 0.01f)
            arc (ar + 5.0f, juce::jmin (ang, baseAng), juce::jmax (ang, baseAng), dark ? c::accent.brighter (0.4f) : c::go, 2.5f);

        // body
        juce::Path body;
        body.addEllipse (ctr.x - br, ctr.y - br, br * 2, br * 2);
        juce::DropShadow (juce::Colour (0x73000000), 6, { 0, 3 }).drawForPath (g, body);
        g.setGradientFill (juce::ColourGradient (c::knobHi, ctr.x, ctr.y - br, c::knob, ctr.x, ctr.y + br, false));
        g.fillPath (body);
        // pointer
        g.setColour (c::onPlate);
        g.drawLine ({ { ctr.x + br * 0.3f * std::sin (ang), ctr.y - br * 0.3f * std::cos (ang) },
                      { ctr.x + (br - 3) * std::sin (ang), ctr.y - (br - 3) * std::cos (ang) } }, 2.0f);
        if (modulated)
        {
            g.setColour (dark ? c::onPlate : c::ink);
            g.drawLine (ctr.x + (ar - 4) * std::sin (baseAng), ctr.y - (ar - 4) * std::cos (baseAng),
                        ctr.x + (ar + 3) * std::sin (baseAng), ctr.y - (ar + 3) * std::cos (baseAng), 2.0f);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const bool dark = isOnDark (s);
        g.setColour (dark ? c::trackDark : c::track);
        g.fillRoundedRectangle (r, 4.0f);
        const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
        auto* ms = dynamic_cast<ModSlider*> (&s);
        const bool modulated = ms != nullptr && ms->isModulationShown();
        if (style == juce::Slider::LinearBarVertical || style == juce::Slider::LinearVertical)
        {
            const float zero = bip ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
            const float yz = r.getBottom() - zero * r.getHeight();
            float yv = pos;   // already a pixel position
            if (modulated) yv = r.getBottom() - (float) s.valueToProportionOfLength (ms->getModulatedValue()) * r.getHeight();
            auto bar = juce::Rectangle<float>::leftTopRightBottom (r.getX(), juce::jmin (yz, yv), r.getRight(), juce::jmax (yz, yv));
            g.setColour (bip ? (yv < yz ? (dark ? c::accent : c::go) : c::signal) : c::accent);   // go is lost on the green deck
            g.fillRoundedRectangle (bar, 3.0f);
            if (bip) { g.setColour (dark ? c::onPlate.withAlpha (0.6f) : c::inkSoft); g.fillRect (r.getX() + 3, yz - 0.5f, r.getWidth() - 6, 1.0f); }
            if (modulated) { g.setColour (dark ? c::onPlate : c::ink); g.fillRect (r.getX(), pos - 1.0f, r.getWidth(), 2.0f); }
        }
        else
        {
            const float zero = bip ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
            const float xz = r.getX() + zero * r.getWidth();
            float xv = pos;
            if (modulated) xv = r.getX() + (float) s.valueToProportionOfLength (ms->getModulatedValue()) * r.getWidth();
            auto bar = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xz, xv), r.getY(), juce::jmax (xz, xv), r.getBottom());
            g.setColour (bip ? (xv >= xz ? (dark ? c::accent : c::go) : c::signal) : c::accent);
            g.fillRoundedRectangle (bar, 3.0f);
            if (bip) { g.setColour (dark ? c::onPlate.withAlpha (0.6f) : c::inkSoft); g.fillRect (xz - 0.5f, r.getY() + 3, 1.0f, r.getHeight() - 6); }
            if (modulated) { g.setColour (dark ? c::onPlate : c::ink); g.fillRect (pos - 1.0f, r.getY(), 2.0f, r.getHeight()); }
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f, 2.0f).withTrimmedRight (3.0f).withTrimmedBottom (1.0f);
        const bool on = b.getToggleState();
        const bool rec = b.getProperties()["sgRec"];
        auto p = platePath (r);
        g.setColour (juce::Colour (0x59000000));
        g.fillPath (p, juce::AffineTransform::translation (3, 3));
        juce::Colour fill = c::plate;
        if (on) fill = rec ? c::signal : c::accent;
        else if (down) fill = c::tag.darker (0.3f);
        else if (over) fill = c::tag;
        g.setColour (fill);
        g.fillPath (p);
        if (b.getClickingTogglesState())
        {   // LED: state is never colour alone
            const float d = juce::jmin (8.0f, r.getHeight() * 0.34f);
            auto led = juce::Rectangle<float> (d, d).withCentre ({ r.getX() + r.getHeight() * 0.58f + d * 0.5f + 2, r.getCentreY() });
            g.setColour (on ? (rec ? c::onPlate : c::go) : c::onPlate.withAlpha (0.25f));
            g.fillEllipse (led);
            if (on && ! rec) { g.setColour (c::onAccent); g.drawEllipse (led, 1.2f); }
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f, 2.0f).withTrimmedRight (3.0f).withTrimmedBottom (1.0f);
        const bool on = b.getToggleState(), rec = b.getProperties()["sgRec"];
        if (b.getClickingTogglesState()) r.removeFromLeft (r.getHeight() * 0.58f + 12.0f);
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.setColour (on && ! rec ? c::onAccent : c::onPlate);
        g.drawText (b.getButtonText().toUpperCase(), r, juce::Justification::centred, false);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        if (box.getWidth() < 62) { label.setBounds (1, 1, box.getWidth() - 2, box.getHeight() - 2); label.setJustificationType (juce::Justification::centred); }
        else { label.setBounds (4, 1, box.getWidth() - 22, box.getHeight() - 2); label.setJustificationType (juce::Justification::centredLeft); }
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, isOnDark (box) ? c::onPlate : c::ink);
        label.setMinimumHorizontalScale (0.7f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        const bool dark = isOnDark (box);
        g.setColour (dark ? juce::Colour (0x47000000) : c::track);
        g.fillRoundedRectangle (r, 4.0f);
        if (box.isMouseOver (true)) { g.setColour (dark ? c::onPlate.withAlpha (0.6f) : c::inkSoft); g.drawRoundedRectangle (r, 4.0f, 1.0f); }
        if (w < 62) return;
        juce::Path chev;
        const float ax = (float) w - 11.0f, ay = h * 0.5f;
        chev.startNewSubPath (ax - 3.5f, ay - 2.0f); chev.lineTo (ax, ay + 1.5f); chev.lineTo (ax + 3.5f, ay - 2.0f);
        g.setColour (dark ? c::onPlate : c::ink);
        g.strokePath (chev, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
        g.setColour (c::ink.withAlpha (0.25f));
        g.drawRect (0, 0, w, h, 1);
    }
};

/** Parameter-bound control. Kinds: Knob, Box (number bar), VSlider, HBar, Toggle, Menu. Empty label = no caption.
    Knobs carry their caption between the knob and the value; the other kinds above the control. */
class Control : public juce::Component
{
public:
    enum class Kind { Knob, Box, VSlider, HBar, Toggle, Menu };

    Control (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, Kind k)
        : kind (k), paramId (id)
    {
        caption.setText (label.toUpperCase(), juce::dontSendNotification);
        caption.setJustificationType (k == Kind::Knob ? juce::Justification::centred : juce::Justification::centredLeft);
        caption.setFont (bold (11.0f, 0.06f));
        caption.setColour (juce::Label::textColourId, c::ink);
        caption.setBorderSize ({});
        caption.setMinimumHorizontalScale (0.75f);
        caption.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (caption);
        caption.setVisible (label.isNotEmpty() && k != Kind::Toggle);

        if (k == Kind::Toggle)
        {
            button.setClickingTogglesState (true);
            button.setButtonText (label);
            addAndMakeVisible (button);
            ba = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
        }
        else if (k == Kind::Menu)
        {
            if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id))) combo.addItemList (p->choices, 1);
            addAndMakeVisible (combo);
            ca = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, combo);
        }
        else
        {
            using S = juce::Slider;
            switch (k)
            {
                case Kind::Knob: slider.setSliderStyle (S::RotaryHorizontalVerticalDrag);
                                 slider.setTextBoxStyle (S::TextBoxBelow, false, 80, LookAndFeel::kValueH); break;
                case Kind::Box:
                case Kind::VSlider: slider.setSliderStyle (S::LinearBarVertical);
                                 slider.setTextBoxStyle (S::NoTextBox, false, 0, 0); break;
                default:         slider.setSliderStyle (S::LinearBar);
                                 slider.setTextBoxStyle (S::NoTextBox, false, 0, 0); break;
            }
            slider.setDoubleClickReturnValue (true, 0.0);
            addAndMakeVisible (slider);
            sa = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
            if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
            if (k != Kind::Knob) slider.setTooltip (label);
            slider.onValueChange = [this] { repaint(); };
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        if (kind == Kind::Toggle) { button.setBounds (r); return; }
        if (kind == Kind::Knob)
        {
            slider.setBounds (r);
            caption.setBounds (0, r.getBottom() - LookAndFeel::kValueH - LookAndFeel::kCaptionH, r.getWidth(), LookAndFeel::kCaptionH);
            caption.toFront (false);
            return;
        }
        if (caption.isVisible()) caption.setBounds (r.removeFromTop (16).withTrimmedBottom (2));
        if (kind == Kind::Menu) { combo.setBounds (r.withHeight (juce::jmin (r.getHeight(), 22))); return; }
        slider.setBounds (r);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (kind == Kind::Box || kind == Kind::HBar)
        {
            const auto t = slider.getTextFromValue (slider.getValue());
            const auto f = pixel (10.0f);
            auto sb = slider.getBounds().toFloat();
            const float tw = juce::GlyphArrangement::getStringWidth (f, t) + 8.0f;
            auto patch = juce::Rectangle<float> (tw, 12.0f).withCentre (sb.getCentre());
            if (patch.getHeight() < sb.getHeight() - 2)
            {
                g.setColour (isOnDark (*this) ? c::plate.withAlpha (0.75f) : c::glass.withAlpha (1.0f));
                g.fillRoundedRectangle (patch, 2.0f);
            }
            g.setColour (isOnDark (*this) ? c::onPlate : c::ink);
            g.setFont (f);
            g.drawText (t, sb, juce::Justification::centred);
        }
    }

    void setCaptionColour (juce::Colour col) { caption.setColour (juce::Label::textColourId, col); }
    void setCaptionJustification (juce::Justification j) { caption.setJustificationType (j); }

    /** Shows the live value coming from the mod matrix (norm = 0..1 parameter position). */
    void setModulation (bool active, float norm)
    {
        slider.setModulation (active, slider.proportionOfLengthToValue (norm));
    }

    ModSlider slider;
    juce::TextButton button;
    juce::ComboBox combo;

private:
    Kind kind;
    juce::String paramId;
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
};

/** Display well: the recessed dark-blue rectangle data is drawn in. */
inline void displayWell (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (c::display);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (c::glassEdge.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
}

/** Circular-buffer waveform with the read window (Position/Size), spray zone and live grain heads. */
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (SonicProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        displayWell (g, r);
        const auto& e = proc.getEngine();
        const double sr = e.getSampleRate();
        const int NB = e.getBufferSize();
        const float* buf = e.getBufferL();
        const int wp = e.getWritePos();
        const double span = juce::jmin (4.6 * sr, (double) NB - 1);
        const float W = r.getWidth(), H = r.getHeight(), mid = (H - 12) * 0.5f + 2;
        auto agoToX = [&] (double ago) { return (float) (W - 8 - (ago / span) * (W - 16)); };

        // grid and second marks
        g.setColour (c::grid);
        for (int s = 1; s <= 4; ++s) g.drawVerticalLine ((int) agoToX (s * sr), 6, H - 16);
        g.setFont (pixel (10.0f));
        g.setColour (c::onPlate.withAlpha (0.7f));
        for (int s = 0; s <= 4; ++s)
            g.drawText (s == 0 ? juce::String ("NOW") : "-" + juce::String (s) + "S", (int) agoToX (s * sr) - 22,
                        (int) H - 14, 44, 12, juce::Justification::centred);

        // read window
        const float pos = proc.ui.pos.load(), size = proc.ui.size.load(), spray = proc.ui.spray.load();
        const float rate = std::pow (2.0f, proc.ui.pitch.load() / 12.0f);
        const double back = pos * 0.01 * 4.0 * sr;
        const double win = size * 0.001 * sr * juce::jmax (0.25f, rate);
        const double sprayS = spray * 0.01 * sr;
        const float xs = agoToX (back + sprayS), xb = agoToX (back), xe = agoToX (juce::jmax (0.0, back - win));
        g.setColour (c::accent.withAlpha (0.25f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xs, xb), 4, juce::jmax (xs, xb), H - 16));
        const float wl = juce::jmin (xb, xe), wr = juce::jmax (juce::jmax (xb, xe), wl + 2);

        // waveform (min/max per pair of columns), accent inside the read window
        juce::Path wave, inWin;
        const int cols = (int) W - 16;
        const double perCol = span / cols;
        const float scale = (H - 22) * 0.5f;
        for (int x = 0; x < cols; x += 2)
        {
            const double agoStart = span - x * perCol;
            float mx = 0;
            const int steps = juce::jmax (1, (int) (perCol * 2) / 8);
            for (int k = 0; k < steps; ++k)
            {
                int idx = (int) ((wp - (long long) (agoStart - k * perCol * 2 / steps)) % NB);
                if (idx < 0) idx += NB;
                mx = juce::jmax (mx, std::abs (buf[idx]));
            }
            const float gx = 8.0f + (float) x;
            const float a = juce::jmax (0.5f, juce::jmin (1.0f, mx) * scale);
            (gx >= wl && gx <= wr ? inWin : wave).addRectangle (gx, mid - a, 1.4f, a * 2);
        }
        g.setColour (c::wave);   g.fillPath (wave);
        g.setColour (c::accent); g.fillPath (inWin);
        g.drawRect (juce::Rectangle<float>::leftTopRightBottom (wl, 3, wr, H - 15), 2.0f);

        // grain heads
        g.setColour (c::onPlate);
        for (int i = 0; i < SonicEngine::numGrains(); ++i)
        {
            const float ago = e.getGrainAgo (i);
            if (ago < 0 || ago > span) continue;
            g.fillEllipse (agoToX (ago) - 2.5f, mid - 2.5f, 5, 5);
        }
        // write head
        g.setColour (c::signal);
        g.fillRect (W - 9, 4.0f, 2.0f, H - 20);
        g.setColour (c::onPlate.withAlpha (0.85f));
        g.setFont (pixel (10.0f));
        g.drawText ("POS " + juce::String (juce::roundToInt (pos)) + "%  SIZE " + juce::String (juce::roundToInt (size)) + " MS",
                    10, 6, 260, 12, juce::Justification::left);
    }

private:
    void timerCallback() override { repaint(); }
    SonicProcessor& proc;
};

/** LCR spectroscope: polar half-circle, angle = stereo position (L..C..R) per frequency band,
    radius = level, colour = frequency. Plus correlation line. */
class PolarScope : public juce::Component, private juce::Timer
{
public:
    explicit PolarScope (SonicProcessor& p) : proc (p), fft (kOrder), window (kN, juce::dsp::WindowingFunction<float>::hann)
    {
        startTimerHz (30);
    }

    void refresh() { timerCallback(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        displayWell (g, r);
        const float cx = r.getCentreX(), cy = r.getBottom() - 22;
        const float rad = juce::jmin (r.getWidth() * 0.5f - 24, r.getHeight() - 36);
        const float halfPi = juce::MathConstants<float>::halfPi;
        g.setColour (c::grid);
        for (float f : { 1.0f, 0.66f, 0.33f })
        {
            juce::Path a; a.addCentredArc (cx, cy, rad * f, rad * f, 0, -halfPi, halfPi, true);
            g.strokePath (a, juce::PathStrokeType (1.0f));
        }
        for (float a : { -0.785f, 0.0f, 0.785f })
            g.drawLine (cx, cy, cx + rad * std::sin (a), cy - rad * std::cos (a), 1.0f);
        for (const auto& d : dots)
        {
            const float ang = d.pan * halfPi;
            const float rr = d.level * (rad - 6);
            g.setColour (d.high ? c::accent : c::wave);
            g.fillEllipse (cx + rr * std::sin (ang) - 2.5f, cy - rr * std::cos (ang) - 2.5f, 5, 5);
        }
        g.setColour (c::onPlate);
        g.setFont (pixel (10.0f));
        g.drawText ("L", (int) (cx - rad - 16), (int) cy - 6, 12, 12, juce::Justification::centred);
        g.drawText ("R", (int) (cx + rad + 4), (int) cy - 6, 12, 12, juce::Justification::centred);
        g.drawText ("C", (int) cx - 6, (int) (cy - rad - 14), 12, 12, juce::Justification::centred);
        // correlation
        const float corr = proc.ui.corr.load();
        auto bar = juce::Rectangle<float> (cx - rad, r.getBottom() - 11, rad * 2, 4);
        g.setColour (c::trackDark); g.fillRect (bar);
        g.setColour (corr >= 0 ? c::go.brighter (0.5f) : c::signal);
        const float zx = bar.getCentreX(), vx = zx + corr * bar.getWidth() * 0.5f;
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (zx, vx), bar.getY(), juce::jmax (zx, vx), bar.getBottom()));
        g.setColour (c::onPlate);
        g.fillRect (zx - 0.5f, bar.getY() - 3, 1.0f, 10.0f);
        g.drawText ("-1", (int) bar.getX() - 20, (int) bar.getY() - 4, 16, 12, juce::Justification::centredRight);
        g.drawText ("+1", (int) bar.getRight() + 4, (int) bar.getY() - 4, 16, 12, juce::Justification::centredLeft);
    }

private:
    static constexpr int kOrder = 11, kN = 1 << kOrder;
    struct Dot { float pan, level; bool high; };

    void timerCallback() override
    {
        std::array<float, kN> l {}, rr {};
        proc.copyScope (l.data(), rr.data(), kN);
        std::array<float, kN * 2> fl {}, fr {};
        std::copy (l.begin(), l.end(), fl.begin());
        std::copy (rr.begin(), rr.end(), fr.begin());
        window.multiplyWithWindowingTable (fl.data(), kN);
        window.multiplyWithWindowingTable (fr.data(), kN);
        fft.performFrequencyOnlyForwardTransform (fl.data());
        fft.performFrequencyOnlyForwardTransform (fr.data());
        dots.clear();
        const double sr = juce::jmax (8000.0, proc.getSampleRate());
        const int bands = 96;
        for (int b = 0; b < bands; ++b)
        {
            const double f0 = 30.0 * std::pow (18000.0 / 30.0, (double) b / bands);
            const double f1 = 30.0 * std::pow (18000.0 / 30.0, (double) (b + 1) / bands);
            int k0 = juce::jlimit (1, kN / 2 - 1, (int) (f0 / sr * kN)), k1 = juce::jlimit (k0 + 1, kN / 2, (int) (f1 / sr * kN));
            float ml = 0, mr = 0;
            for (int k = k0; k < k1; ++k) { ml += fl[(size_t) k]; mr += fr[(size_t) k]; }
            ml /= (float) (k1 - k0); mr /= (float) (k1 - k0);
            const float sum = ml + mr;
            const float db = juce::Decibels::gainToDecibels (sum / (float) kN * 4.0f, -90.0f);
            const float level = juce::jlimit (0.0f, 1.0f, (db + 78.0f) / 72.0f);
            if (level < 0.03f) continue;
            const float pan = sum > 1e-9f ? (mr - ml) / sum : 0.0f;
            auto& sm = smooth[(size_t) b];
            sm.pan += (pan - sm.pan) * 0.35f;
            sm.level += (level - sm.level) * 0.35f;
            dots.push_back ({ sm.pan, sm.level, b >= bands * 6 / 10 });
        }
        repaint();
    }

    SonicProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<Dot> dots;
    struct Sm { float pan = 0, level = 0; };
    std::array<Sm, 96> smooth {};
};

/** LFO display: the shape drawn once across the well, with a dot riding it at the current phase. */
class LfoView : public juce::Component, private juce::Timer
{
public:
    LfoView (SonicProcessor& p, int index) : proc (p), idx (index) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        displayWell (g, r);
        const int shape = (int) proc.state.getRawParameterValue ("lfo" + juce::String (idx + 1) + "_shape")->load();
        const float W = r.getWidth() - 24, H = r.getHeight() - 24;
        g.setColour (c::grid);
        g.drawHorizontalLine ((int) r.getCentreY(), 8, r.getWidth() - 8);
        juce::Path path;
        for (int i = 0; i <= 160; ++i)
        {
            const float p = i / 160.0f;
            const float x = 12 + p * W, y = 12 + (0.5f - 0.5f * lfoShapeValue (shape, p)) * H;
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (c::wave);
        g.strokePath (path, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float ph = proc.ui.lfoPhase[idx].load(), v = proc.ui.lfo[idx].load();
        auto dot = juce::Rectangle<float> (12, 12).withCentre ({ 12 + ph * W, 12 + (0.5f - 0.5f * v) * H });
        g.setColour (c::accent);   g.fillEllipse (dot);
        g.setColour (c::onAccent); g.drawEllipse (dot, 1.5f);
    }
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    SonicProcessor& proc;
    int idx;
};

/** Tab of the modulation deck: a slanted plate with a name and a small live drawing of the source. */
class ModTab : public juce::Button, private juce::Timer
{
public:
    std::function<void (juce::Graphics&, juce::Rectangle<float>, juce::Colour)> drawThumb;

    explicit ModTab (const juce::String& name) : juce::Button (name)
    {
        setClickingTogglesState (true);
        setRadioGroupId (1);
        startTimerHz (10);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().withTrimmedRight (8).withTrimmedBottom (8);
        const bool on = getToggleState();
        if (on) taggedPlate (g, r, c::glass.withAlpha (1.0f));
        else
        {
            g.setColour (juce::Colour (0x59000000)); g.fillPath (platePath (r), juce::AffineTransform::translation (3, 3));
            g.setColour (down ? c::tag.darker (0.3f) : (over ? c::tag : c::plate)); g.fillPath (platePath (r));
        }
        const auto ink = on ? c::ink : c::onPlate;
        auto t = r.reduced (r.getHeight() * 0.6f, 0);
        g.setColour (ink);
        g.setFont (black (12.0f, 0.04f));
        g.drawText (getName().toUpperCase(), t.removeFromLeft (t.getWidth() * 0.55f), juce::Justification::centredLeft, false);
        if (drawThumb) drawThumb (g, t.reduced (2, 7), on ? ink : ink.withAlpha (0.7f));
    }

private:
    void timerCallback() override { repaint(); }
};
} // namespace sonic
