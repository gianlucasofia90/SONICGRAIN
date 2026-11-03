#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace idm
{
namespace col
{
    const juce::Colour bg       { 0xff17171a };
    const juce::Colour panel    { 0xff222227 };
    const juce::Colour text     { 0xffd6d6da };
    const juce::Colour dim      { 0xff7c7c86 };
    const juce::Colour accent   { 0xffff8c1f };
    const juce::Colour cyan     { 0xff33cce6 };
    const juce::Colour violet   { 0xffbf73ff };
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, col::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::rotarySliderFillColourId, col::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a42));
        setColour (juce::Label::textColourId, col::text);
        setColour (juce::ComboBox::backgroundColourId, col::panel);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3a42));
        setColour (juce::ComboBox::textColourId, col::text);
        setColour (juce::PopupMenu::backgroundColourId, col::panel);
        setColour (juce::TextButton::buttonColourId, col::panel);
        setColour (juce::TextButton::buttonOnColourId, col::accent);
        setColour (juce::TextButton::textColourOffId, col::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider& s) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        auto size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        auto c = r.getCentre();
        auto rad = size * 0.5f - 2.0f;
        juce::Path bgArc, valArc;
        bgArc.addCentredArc (c.x, c.y, rad, rad, 0.0f, a0, a1, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        auto ang = a0 + pos * (a1 - a0);
        valArc.addCentredArc (c.x, c.y, rad, rad, 0.0f, a0, ang, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Point<float> tip (c.x + (rad - 4) * std::sin (ang), c.y - (rad - 4) * std::cos (ang));
        g.setColour (col::text);
        g.drawLine ({ c, tip }, 2.0f);
    }
};

/** A labelled control bound to a parameter (knob / small number box / toggle / menu). */
class ParamControl : public juce::Component
{
public:
    enum class Kind { Knob, Box, Toggle, Menu };

    ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label, Kind k)
        : kind (k)
    {
        caption.setText (label, juce::dontSendNotification);
        caption.setJustificationType (juce::Justification::centred);
        caption.setFont (juce::FontOptions (11.0f));
        caption.setColour (juce::Label::textColourId, col::dim);
        addAndMakeVisible (caption);

        if (k == Kind::Knob || k == Kind::Box)
        {
            slider.setSliderStyle (k == Kind::Knob ? juce::Slider::RotaryHorizontalVerticalDrag
                                                   : juce::Slider::LinearBarVertical);
            slider.setTextBoxStyle (k == Kind::Knob ? juce::Slider::TextBoxBelow : juce::Slider::TextBoxAbove,
                                    false, 60, 16);
            if (k == Kind::Box)
            {
                slider.setColour (juce::Slider::trackColourId, juce::Colour (0xff35353d));
                slider.setColour (juce::Slider::backgroundColourId, col::panel);
            }
            addAndMakeVisible (slider);
            sa = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
        }
        else if (k == Kind::Toggle)
        {
            button.setClickingTogglesState (true);
            button.setButtonText (label);
            caption.setVisible (false);
            addAndMakeVisible (button);
            ba = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, button);
        }
        else
        {
            if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
                combo.addItemList (p->choices, 1);
            addAndMakeVisible (combo);
            ca = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, combo);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds();
        if (kind == Kind::Toggle) { button.setBounds (r.reduced (1)); return; }
        caption.setBounds (r.removeFromTop (13));
        if (kind == Kind::Menu) combo.setBounds (r.reduced (1, 0).withHeight (juce::jmin (r.getHeight(), 20)));
        else slider.setBounds (r);
    }

private:
    Kind kind;
    juce::Label caption;
    juce::Slider slider;
    juce::TextButton button;
    juce::ComboBox combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
};

inline std::unique_ptr<juce::AudioParameterFloat> fparam (const juce::String& id, const juce::String& name,
                                                          float lo, float hi, float def, const juce::String& unit = {},
                                                          float skewCentre = -1.0f)
{
    juce::NormalisableRange<float> r (lo, hi);
    if (skewCentre > lo && skewCentre < hi) r.setSkewForCentre (skewCentre);
    const int dec = (hi - lo) >= 100.0f ? 0 : 1;
    auto toText = [dec, unit] (float v, int) {
        if (unit == "Hz" && v >= 1000.0f) return juce::String (v / 1000.0f, 1) + " kHz";
        return juce::String (v, dec) + (unit.isNotEmpty() ? " " + unit : juce::String());
    };
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                        juce::AudioParameterFloatAttributes().withLabel (unit)
                                                            .withStringFromValueFunction (toText));
}

inline std::unique_ptr<juce::AudioParameterInt> iparam (const juce::String& id, const juce::String& name,
                                                        int lo, int hi, int def, const juce::String& unit = {})
{
    return std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def,
                                                      juce::AudioParameterIntAttributes().withLabel (unit));
}

// On/Off switch as a 2-item choice: unlike AudioParameterBool it always stores a snapped value,
// so state save/restore round-trips exactly (pluginval strictness 8+).
inline std::unique_ptr<juce::AudioParameterChoice> bparam (const juce::String& id, const juce::String& name, bool def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, juce::StringArray { "Off", "On" }, def ? 1 : 0);
}

inline std::unique_ptr<juce::AudioParameterChoice> cparam (const juce::String& id, const juce::String& name,
                                                           juce::StringArray items, int def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
}
} // namespace idm
