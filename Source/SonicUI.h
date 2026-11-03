#pragma once
// SONIC GRAIN UI parts: look & feel (original blue "speed" style), controls, waveform, LCR scope, LFO view.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SonicProcessor.h"

namespace sonic
{
namespace c
{
    const juce::Colour deep   { 0xff0b3aa8 };
    const juce::Colour royal  { 0xff1f5be8 };
    const juce::Colour sky    { 0xff8fd2f5 };
    const juce::Colour skyLt  { 0xffc4ebfc };
    const juce::Colour ink    { 0xff0a1f5c };
    const juce::Colour red    { 0xffe3262e };
    const juce::Colour gold   { 0xfff7c21b };
    const juce::Colour white  { 0xffffffff };
    const juce::Colour navy   { 0xff071a4a };
}

inline juce::Font boldItalic (float size) { return juce::Font (juce::FontOptions (size, juce::Font::bold | juce::Font::italic)); }
inline juce::Font bold (float size) { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }

/** White text with a dark-blue outline, like a game title card. */
inline void outlinedText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float size,
                          juce::Justification j = juce::Justification::centredLeft, juce::Colour fill = c::white)
{
    juce::GlyphArrangement ga;
    ga.addFittedText (boldItalic (size), t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), j, 1);
    juce::Path p;
    ga.createPath (p);
    g.setColour (c::ink);
    g.strokePath (p, juce::PathStrokeType (size * 0.16f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (fill);
    g.fillPath (p);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, c::ink);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, c::ink);
        setColour (juce::ComboBox::backgroundColourId, c::white);
        setColour (juce::ComboBox::outlineColourId, c::royal);
        setColour (juce::ComboBox::textColourId, c::ink);
        setColour (juce::ComboBox::arrowColourId, c::royal);
        setColour (juce::PopupMenu::backgroundColourId, c::white);
        setColour (juce::PopupMenu::textColourId, c::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, c::royal);
        setColour (juce::PopupMenu::highlightedTextColourId, c::white);
        setColour (juce::TextButton::buttonColourId, c::white);
        setColour (juce::TextButton::buttonOnColourId, c::red);
        setColour (juce::TextButton::textColourOffId, c::deep);
        setColour (juce::TextButton::textColourOnId, c::white);
        setColour (juce::Slider::trackColourId, c::royal);
        setColour (juce::Slider::backgroundColourId, c::skyLt);
        setColour (juce::AlertWindow::backgroundColourId, c::white);
        setColour (juce::AlertWindow::textColourId, c::ink);
        setColour (juce::TextEditor::backgroundColourId, c::skyLt);
        setColour (juce::TextEditor::textColourId, c::ink);
    }

    juce::Font getComboBoxFont (juce::ComboBox& b) override { return bold (b.getWidth() < 62 ? 11.0f : 12.5f); }
    juce::Font getPopupMenuFont() override { return bold (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return boldItalic (juce::jmin (14.0f, h * 0.62f)); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1,
                           juce::Slider&) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto ctr = r.getCentre();
        const float rad = size * 0.5f;
        // track ring
        juce::Path track, val;
        track.addCentredArc (ctr.x, ctr.y, rad - 3, rad - 3, 0, a0, a1, true);
        g.setColour (c::skyLt);
        g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float ang = a0 + pos * (a1 - a0);
        val.addCentredArc (ctr.x, ctr.y, rad - 3, rad - 3, 0, a0, ang, true);
        g.setColour (c::royal);
        g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // face
        const float fr = rad - 9;
        g.setColour (c::ink.withAlpha (0.25f));
        g.fillEllipse (ctr.x - fr + 1.5f, ctr.y - fr + 2.5f, fr * 2, fr * 2);
        g.setGradientFill (juce::ColourGradient (c::white, ctr.x, ctr.y - fr, c::skyLt, ctr.x, ctr.y + fr, false));
        g.fillEllipse (ctr.x - fr, ctr.y - fr, fr * 2, fr * 2);
        g.setColour (c::deep);
        g.drawEllipse (ctr.x - fr, ctr.y - fr, fr * 2, fr * 2, 2.0f);
        // pointer
        juce::Point<float> p0 (ctr.x + fr * 0.25f * std::sin (ang), ctr.y - fr * 0.25f * std::cos (ang));
        juce::Point<float> p1 (ctr.x + (fr - 3) * std::sin (ang), ctr.y - (fr - 3) * std::cos (ang));
        g.setColour (c::red);
        g.drawLine ({ p0, p1 }, 3.0f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        g.setColour (c::white);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (c::royal.withAlpha (0.6f));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);
        const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
        if (style == juce::Slider::LinearBarVertical || style == juce::Slider::LinearVertical)
        {
            const float zero = bip ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
            const float yz = r.getBottom() - zero * r.getHeight();
            const float yv = pos;   // already pixel position
            auto bar = juce::Rectangle<float>::leftTopRightBottom (r.getX() + 3, juce::jmin (yz, yv), r.getRight() - 3,
                                                                    juce::jmax (yz, yv));
            g.setColour (bip && yv < yz ? c::royal : (bip ? c::red : c::sky));
            g.fillRoundedRectangle (bar, 2.0f);
            if (bip) { g.setColour (c::ink.withAlpha (0.5f)); g.fillRect (r.getX() + 2, yz - 0.5f, r.getWidth() - 4, 1.0f); }
        }
        else
        {
            const float zero = bip ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
            const float xz = r.getX() + zero * r.getWidth();
            auto bar = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xz, pos), r.getY() + 3,
                                                                    juce::jmax (xz, pos), r.getBottom() - 3);
            g.setColour (bip && pos < xz ? c::red.withAlpha (0.55f) : c::sky);
            g.fillRoundedRectangle (bar, 2.0f);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        g.setColour (c::ink.withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (0, 2), 6.0f);
        g.setColour (on ? c::red : (down ? c::skyLt : c::white));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? c::gold : (over ? c::royal : c::royal.withAlpha (0.7f)));
        g.drawRoundedRectangle (r, 6.0f, 2.0f);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        if (box.getWidth() < 62) { label.setBounds (1, 1, box.getWidth() - 2, box.getHeight() - 2); label.setJustificationType (juce::Justification::centred); }
        else { label.setBounds (2, 1, box.getWidth() - 22, box.getHeight() - 2); label.setJustificationType (juce::Justification::centredLeft); }
        label.setFont (getComboBoxFont (box));
        label.setMinimumHorizontalScale (0.7f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
        g.setColour (c::white);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (c::royal);
        g.drawRoundedRectangle (r, 5.0f, 1.6f);
        if (w < 62) return;
        juce::Path arr;
        const float ax = (float) w - 12.0f, ay = h * 0.5f;
        arr.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.fillPath (arr);
    }
};

/** Parameter-bound control. Kinds: Knob, Box (number bar), VSlider, HBar, Toggle, Menu. Empty label = no caption. */
class Control : public juce::Component
{
public:
    enum class Kind { Knob, Box, VSlider, HBar, Toggle, Menu };

    Control (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, Kind k)
        : kind (k), paramId (id)
    {
        caption.setText (label, juce::dontSendNotification);
        caption.setJustificationType (juce::Justification::centred);
        caption.setFont (boldItalic (12.0f));
        caption.setColour (juce::Label::textColourId, c::ink);
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
                                 slider.setTextBoxStyle (S::TextBoxBelow, false, 74, 16); break;
                case Kind::Box:  slider.setSliderStyle (S::LinearBarVertical);
                                 slider.setTextBoxStyle (S::NoTextBox, false, 0, 0); break;
                case Kind::VSlider: slider.setSliderStyle (S::LinearBarVertical);
                                 slider.setTextBoxStyle (S::NoTextBox, false, 0, 0); break;
                default:         slider.setSliderStyle (S::LinearBar);
                                 slider.setTextBoxStyle (S::NoTextBox, false, 0, 0); break;
            }
            slider.setColour (S::textBoxTextColourId, c::ink);
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
        if (caption.isVisible()) caption.setBounds (r.removeFromTop (15));
        if (kind == Kind::Menu) { combo.setBounds (r.withHeight (juce::jmin (r.getHeight(), 22))); return; }
        slider.setBounds (r);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        if (kind == Kind::Box || kind == Kind::HBar)
        {
            g.setColour (c::ink);
            g.setFont (bold (11.5f));
            g.drawText (slider.getTextFromValue (slider.getValue()), slider.getBounds(), juce::Justification::centred);
        }
    }

    void setCaptionColour (juce::Colour col) { caption.setColour (juce::Label::textColourId, col); }

    juce::Slider slider;
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

/** Circular-buffer waveform with the read window (Position/Size), spray zone and live grain heads. */
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (SonicProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (c::navy, 0, 0, c::deep, 0, r.getHeight(), false));
        g.fillRoundedRectangle (r, 10.0f);
        const auto& e = proc.getEngine();
        const double sr = e.getSampleRate();
        const int NB = e.getBufferSize();
        const float* buf = e.getBufferL();
        const int wp = e.getWritePos();
        const double span = juce::jmin (4.6 * sr, (double) NB - 1);
        const float W = r.getWidth(), H = r.getHeight(), mid = H * 0.5f;
        auto agoToX = [&] (double ago) { return (float) (W - 8 - (ago / span) * (W - 16)); };

        // grid
        g.setColour (c::white.withAlpha (0.08f));
        for (int s = 0; s <= 4; ++s) { float x = agoToX (s * sr); g.drawVerticalLine ((int) x, 6, H - 6); }
        g.setFont (bold (10.0f));
        g.setColour (c::sky.withAlpha (0.7f));
        for (int s = 0; s <= 4; ++s)
            g.drawText (s == 0 ? juce::String ("NOW") : "-" + juce::String (s) + "s", (int) agoToX (s * sr) - 20,
                        (int) H - 16, 40, 12, juce::Justification::centred);

        // read window
        const float pos = proc.ui.pos.load(), size = proc.ui.size.load(), spray = proc.ui.spray.load();
        const float rate = std::pow (2.0f, proc.ui.pitch.load() / 12.0f);
        const double back = pos * 0.01 * 4.0 * sr;
        const double win = size * 0.001 * sr * juce::jmax (0.25f, rate);
        const double sprayS = spray * 0.01 * sr;
        const float xs = agoToX (back + sprayS), xb = agoToX (back), xe = agoToX (juce::jmax (0.0, back - win));
        g.setColour (c::gold.withAlpha (0.18f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xs, xb), 8, juce::jmax (xs, xb), H - 20));
        g.setColour (c::red.withAlpha (0.35f));
        auto wr = juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xb, xe), 8, juce::jmax (juce::jmax (xb, xe), juce::jmin (xb, xe) + 2), H - 20);
        g.fillRect (wr);
        g.setColour (c::gold);
        g.drawRect (wr, 2.0f);

        // waveform (min/max per column)
        juce::Path wave;
        const int cols = (int) W - 16;
        const double perCol = span / cols;
        for (int x = 0; x < cols; ++x)
        {
            const double agoStart = span - x * perCol;
            float mn = 0, mx = 0;
            const int steps = juce::jmax (1, (int) perCol / 8);
            for (int k = 0; k < steps; ++k)
            {
                int idx = (int) ((wp - (long long) (agoStart - k * perCol / steps)) % NB);
                if (idx < 0) idx += NB;
                const float v = buf[idx];
                mn = juce::jmin (mn, v); mx = juce::jmax (mx, v);
            }
            const float gx = 8.0f + x;
            const float scale = (H - 30) * 0.5f;
            wave.addRectangle (gx, mid - 6 - mx * scale, 1.0f, juce::jmax (1.0f, (mx - mn) * scale));
        }
        g.setColour (c::sky);
        g.fillPath (wave);

        // grain heads
        for (int i = 0; i < SonicEngine::numGrains(); ++i)
        {
            const float ago = e.getGrainAgo (i);
            if (ago < 0 || ago > span) continue;
            const float x = agoToX (ago);
            g.setColour (c::white);
            g.fillEllipse (x - 3, mid - 9, 6, 6);
        }
        // write head
        g.setColour (c::red);
        g.fillRect (W - 9, 6.0f, 2.0f, H - 26);
        g.setColour (c::white.withAlpha (0.8f));
        g.setFont (boldItalic (11.0f));
        g.drawText ("POS " + juce::String (pos, 0) + "%   SIZE " + juce::String (size, 0) + " ms", 12, 8, 260, 14,
                    juce::Justification::left);
    }

private:
    void timerCallback() override { repaint(); }
    SonicProcessor& proc;
};

/** LCR spectroscope: polar half-circle, angle = stereo position (L..C..R) per frequency band,
    radius = level, colour = frequency. Plus correlation bar. */
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
        const float cx = r.getCentreX(), cy = r.getBottom() - 18;
        const float rad = juce::jmin (r.getWidth() * 0.5f - 24, r.getHeight() - 26);
        // dial
        juce::Path half;
        half.addPieSegment (cx - rad, cy - rad, rad * 2, rad * 2, -juce::MathConstants<float>::halfPi,
                            juce::MathConstants<float>::halfPi, 0.0f);
        g.setGradientFill (juce::ColourGradient (c::deep, cx, cy, c::navy, cx, cy - rad, true));
        g.fillPath (half);
        g.setColour (c::white);
        g.strokePath (half, juce::PathStrokeType (3.0f));
        g.setColour (c::white.withAlpha (0.15f));
        for (float f : { 0.33f, 0.66f })
        {
            juce::Path a; a.addCentredArc (cx, cy, rad * f, rad * f, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            g.strokePath (a, juce::PathStrokeType (1.0f));
        }
        for (float a : { -0.785f, 0.0f, 0.785f })
            g.drawLine (cx, cy, cx + rad * std::sin (a), cy - rad * std::cos (a), 1.0f);
        // dots
        for (const auto& d : dots)
        {
            const float ang = d.pan * juce::MathConstants<float>::halfPi;
            const float rr = d.level * (rad - 6);
            const float x = cx + rr * std::sin (ang), y = cy - rr * std::cos (ang);
            g.setColour (d.col.withAlpha (0.85f));
            g.fillEllipse (x - 2.5f, y - 2.5f, 5, 5);
        }
        outlinedText (g, "L", { cx - rad - 4, cy - 14, 20, 16 }, 14.0f, juce::Justification::centred);
        outlinedText (g, "R", { cx + rad - 16, cy - 14, 20, 16 }, 14.0f, juce::Justification::centred);
        outlinedText (g, "C", { cx - 10, cy - rad - 2, 20, 16 }, 14.0f, juce::Justification::centred);
        // correlation
        const float corr = proc.ui.corr.load();
        auto bar = juce::Rectangle<float> (cx - rad, r.getBottom() - 12, rad * 2, 9);
        g.setColour (c::white); g.fillRoundedRectangle (bar, 4);
        g.setColour (corr >= 0 ? c::royal : c::red);
        const float zx = bar.getCentreX(), vx = zx + corr * bar.getWidth() * 0.5f;
        g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (zx, vx), bar.getY() + 1, juce::jmax (zx, vx), bar.getBottom() - 1), 3);
        g.setColour (c::ink); g.setFont (bold (9.0f));
        g.drawText ("-1", bar.withWidth (16).translated (-18, -2), juce::Justification::centredRight);
        g.drawText ("+1", bar.withX (bar.getRight() + 2).withWidth (16).translated (0, -2), juce::Justification::centredLeft);
    }

private:
    static constexpr int kOrder = 11, kN = 1 << kOrder;
    struct Dot { float pan, level; juce::Colour col; };

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
            const float t = (float) b / bands;
            const juce::Colour col = t < 0.5f ? c::sky.interpolatedWith (c::white, t * 2) : c::white.interpolatedWith (c::gold, (t - 0.5f) * 2);
            dots.push_back ({ sm.pan, sm.level, col });
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

/** Mini LFO display: draws the shape with a moving phase dot. */
class LfoView : public juce::Component, private juce::Timer
{
public:
    LfoView (SonicProcessor& p, int index) : proc (p), idx (index) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (c::navy);
        g.fillRoundedRectangle (r, 8.0f);
        const int shape = (int) proc.state.getRawParameterValue ("lfo" + juce::String (idx + 1) + "_shape")->load();
        auto val = [shape] (float p) {
            switch (shape)
            {
                case 0: return std::sin (6.2831853f * p);
                case 1: return 1.0f - 4.0f * std::abs (p - 0.5f);
                case 2: return 2.0f * p - 1.0f;
                case 3: return 1.0f - 2.0f * p;
                case 4: return p < 0.5f ? 1.0f : -1.0f;
                case 5: { static const float v[8] = { 0.6f, -0.3f, 0.9f, -0.8f, 0.2f, -0.5f, 0.7f, -0.1f }; return v[(int) (p * 8) % 8]; }
                default: { static const float v[9] = { 0.6f, -0.3f, 0.9f, -0.8f, 0.2f, -0.5f, 0.7f, -0.1f, 0.6f };
                           const float x = p * 8; const int i = (int) x; const float f = x - i;
                           return v[i] + (v[i + 1] - v[i]) * (0.5f - 0.5f * std::cos (3.14159265f * f)); }
            }
        };
        juce::Path path;
        const float W = r.getWidth() - 16, H = r.getHeight() - 14;
        for (int i = 0; i <= 120; ++i)
        {
            const float p = i / 120.0f;
            const float x = 8 + p * W, y = 7 + (0.5f - 0.5f * val (p)) * H;
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (c::sky);
        g.strokePath (path, juce::PathStrokeType (2.0f));
        const float ph = proc.ui.lfoPhase[idx].load(), v = proc.ui.lfo[idx].load();
        g.setColour (c::gold);
        g.fillEllipse (8 + ph * W - 5, 7 + (0.5f - 0.5f * v) * H - 5, 10, 10);
    }
private:
    void timerCallback() override { repaint(); }
    SonicProcessor& proc;
    int idx;
};
} // namespace sonic
