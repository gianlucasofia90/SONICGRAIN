#pragma once
#include "SonicProcessor.h"
#include "SonicUI.h"

class SonicEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SonicEditor (SonicProcessor& p) : AudioProcessorEditor (p), proc (p), wave (p), scope (p)
    {
        setLookAndFeel (&lnf);
        using K = sonic::Control::Kind;
        auto add = [this, &p] (const juce::String& id, const juce::String& label, K k) {
            auto* ctl = controls.add (new sonic::Control (p.state, id, label, k));
            addAndMakeVisible (ctl);
            byId.set (id, ctl);
            return ctl;
        };
        // header
        add ("syncmode", "SYNC", K::Menu);
        add ("freebpm", "BPM", K::Box);
        add ("outdb", "OUTPUT", K::Box);
        // grains
        for (auto pr : { std::pair<const char*, const char*> { "gpos", "Position" }, { "gsize", "Size" },
                          { "gdens", "Density" }, { "gpitch", "Pitch" }, { "spray", "Spray" }, { "prand", "Pitch Rnd" },
                          { "grev", "Reverse" }, { "gshape", "Shape" }, { "gwidth", "Spread" },
                          { "feedback", "Feedback" }, { "cloud", "Cloud" } })
            add (pr.first, pr.second, K::Knob);
        add ("freeze", "FREEZE", K::Toggle);
        // imager
        add ("imgwidth", "Width", K::Knob);
        add ("monofreq", "Bass Mono", K::Knob);
        // filter / glitch / colour
        for (auto pr : { std::pair<const char*, const char*> { "hpf", "HPF" }, { "hpq", "Q" }, { "lpf", "LPF" },
                          { "lreso", "Reso" }, { "glitch", "Glitch" }, { "rptpitch", "Rpt Pitch" },
                          { "crbits", "Bits" }, { "dsamp", "Downsmp" }, { "tone", "Tone" }, { "wow", "Wow" },
                          { "flutter", "Flutter" }, { "drive", "Drive" }, { "drywet", "Dry/Wet" } })
            add (pr.first, pr.second, K::Knob);
        add ("slice", "Slice", K::Menu);
        add ("rhold", "Hold", K::Box);
        // LFOs
        for (int i = 1; i <= sonic::kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i) + "_";
            add (pre + "shape", "Shape", K::Menu);
            add (pre + "sync", "SYNC", K::Toggle);
            add (pre + "rate", "Rate", K::HBar);
            add (pre + "div", "Division", K::Menu);
            addAndMakeVisible (lfoViews.add (new sonic::LfoView (p, i - 1)));
        }
        // sequencer
        add ("seq_len", "Length", K::Box);
        add ("seq_dir", "Direction", K::Menu);
        add ("seq_div", "Clock", K::Menu);
        add ("seq_slide", "Slide", K::Box);
        for (int s = 1; s <= 8; ++s)
        {
            const juce::String pre = "s" + juce::String (s) + "_";
            add (pre + "val", {}, K::VSlider);
            add (pre + "pulses", {}, K::Box);
            add (pre + "ratch", {}, K::Box);
            add (pre + "gate", {}, K::Menu);
            add (pre + "slide", "SL", K::Toggle);
            add (pre + "prob", {}, K::Box);
        }
        // mod matrix
        for (int m = 1; m <= sonic::kNumSlots; ++m)
        {
            const juce::String pre = "m" + juce::String (m) + "_";
            add (pre + "src", {}, K::Menu);
            add (pre + "dst", {}, K::Menu);
            add (pre + "amt", {}, K::HBar);
        }
        addAndMakeVisible (wave);
        addAndMakeVisible (scope);
        for (auto* id : { "syncmode", "freebpm", "outdb" }) byId[id]->setCaptionColour (sonic::c::skyLt);
        for (int d = 0; d < sonic::kNumDests; ++d) destControls[(size_t) d] = byId[sonic::kDestIds[d]];

        // presets
        for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn }) addAndMakeVisible (b);
        prevBtn.setButtonText ("<"); nextBtn.setButtonText (">");
        saveBtn.setButtonText ("SAVE"); delBtn.setButtonText ("DEL");
        addAndMakeVisible (presetBox);
        refreshPresets();
        presetBox.onChange = [this] {
            const int i = presetBox.getSelectedItemIndex();
            if (i >= 0 && i != proc.presets.getCurrent()) proc.presets.load (i);
        };
        prevBtn.onClick = [this] { proc.presets.prev(); refreshPresets(); };
        nextBtn.onClick = [this] { proc.presets.next(); refreshPresets(); };
        delBtn.onClick = [this] { proc.presets.removeCurrent(); refreshPresets(); };
        saveBtn.onClick = [this] {
            auto* w = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon, this);
            w->addTextEditor ("name", proc.presets.getCurrent() >= proc.presets.numFactory()
                                          ? proc.presets.getNames()[proc.presets.getCurrent()] : "My preset");
            w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r) {
                if (r == 1) { proc.presets.save (w->getTextEditorContents ("name")); refreshPresets(); }
            }), true);
        };

        setSize (1280, 800);
        startTimerHz (kDisplayHz);
    }
    ~SonicEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        using namespace sonic;
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffaee0f9), 0, 0, juce::Colour (0xff6fbfee), 0, r.getHeight(), false));
        g.fillAll();
        // checkerboard
        g.setColour (c::white.withAlpha (0.13f));
        for (int y = 64; y < getHeight(); y += 40)
            for (int x = ((y / 40) % 2) * 40; x < getWidth(); x += 80)
                g.fillRect (x, y, 40, 40);
        // decorative rings
        g.setColour (c::white.withAlpha (0.35f));
        g.drawEllipse (1060, -120, 420, 420, 14.0f);
        g.drawEllipse (-160, 520, 380, 380, 10.0f);
        // header band with speed stripes
        g.setColour (c::deep);
        g.fillRect (0.0f, 0.0f, r.getWidth(), 64.0f);
        juce::Path stripe;
        stripe.addQuadrilateral (300, 0, 420, 0, 360, 64, 240, 64);
        g.setColour (c::royal);
        g.fillPath (stripe);
        stripe.applyTransform (juce::AffineTransform::translation (90, 0));
        g.setColour (c::royal.withAlpha (0.6f));
        g.fillPath (stripe);
        g.setColour (c::white);
        g.fillRect (0.0f, 62.0f, r.getWidth(), 3.0f);
        outlinedText (g, "SONIC GRAIN", { 20, 6, 300, 40 }, 34.0f);
        g.setColour (c::skyLt);
        g.setFont (boldItalic (11.0f));
        g.drawText ("GRANULAR  /  GLITCH  /  TAPE  /  MOTION", 22, 44, 300, 14, juce::Justification::left);

        panel (g, { 16, 76, 912, 300 }, "GRAINS");
        panel (g, { 944, 76, 320, 300 }, "IMAGER  L C R");
        panel (g, { 16, 388, 330, 180 }, "FILTER");
        panel (g, { 356, 388, 330, 180 }, "GLITCH");
        panel (g, { 696, 388, 568, 180 }, "COLOUR");
        for (int i = 0; i < sonic::kNumLfos; ++i)
            panel (g, { 16.0f + (float) i * 130.0f, 580, 120, 208 }, "LFO " + juce::String (i + 1));
        panel (g, { 536, 580, 470, 208 }, "METRO SEQ");
        panel (g, { 1016, 580, 248, 208 }, "MOD MATRIX");

        // sequencer row labels + playhead
        g.setFont (bold (10.0f));
        g.setColour (c::ink);
        const char* rows[6] = { "VAL", "PLS", "RAT", "GATE", "SLD", "PRB" };
        const int ry[6] = { 646, 700, 718, 736, 756, 772 };
        for (int i = 0; i < 6; ++i) g.drawText (rows[i], 544, ry[i], 34, 14, juce::Justification::left);
        const int st = proc.ui.seqStage.load();
        auto col = juce::Rectangle<float> (580.0f + st * 52.0f, 640.0f, 50.0f, 146.0f);
        g.setColour (sonic::c::red.withAlpha (0.18f));
        g.fillRoundedRectangle (col, 5.0f);
        g.setColour (proc.ui.seqGate.load() > 0.5f ? sonic::c::red : sonic::c::white);
        g.drawRoundedRectangle (col, 5.0f, 2.0f);
        // matrix header
        g.setColour (c::ink);
        g.drawText ("SOURCE", 1028, 606, 60, 12, juce::Justification::left);
        g.drawText ("DESTINATION", 1092, 606, 90, 12, juce::Justification::left);
        g.drawText ("AMOUNT", 1194, 606, 60, 12, juce::Justification::left);
    }

    void resized() override
    {
        auto at = [this] (const juce::String& id, int x, int y, int w, int h) { if (auto* c = byId[id]) c->setBounds (x, y, w, h); };
        // header
        prevBtn.setBounds (420, 22, 30, 26);
        presetBox.setBounds (454, 22, 270, 26);
        nextBtn.setBounds (728, 22, 30, 26);
        saveBtn.setBounds (766, 22, 60, 26);
        delBtn.setBounds (830, 22, 50, 26);
        at ("syncmode", 920, 8, 80, 44);
        at ("freebpm", 1006, 8, 70, 44);
        at ("outdb", 1150, 8, 110, 44);
        // grains
        wave.setBounds (32, 108, 880, 150);
        at ("freeze", 808, 82, 104, 22);
        const char* g[11] = { "gpos", "gsize", "gdens", "gpitch", "spray", "prand", "grev", "gshape", "gwidth", "feedback", "cloud" };
        for (int i = 0; i < 11; ++i) at (g[i], 32 + i * 80, 264, 80, 106);
        // imager
        scope.setBounds (956, 100, 296, 190);
        at ("imgwidth", 1000, 290, 90, 84);
        at ("monofreq", 1110, 290, 90, 84);
        // filter / glitch / colour
        const char* f[4] = { "hpf", "hpq", "lpf", "lreso" };
        for (int i = 0; i < 4; ++i) at (f[i], 28 + i * 78, 420, 76, 136);
        at ("glitch", 368, 420, 76, 136);
        at ("slice", 450, 450, 70, 40);
        at ("rptpitch", 526, 420, 76, 136);
        at ("rhold", 610, 450, 66, 40);
        const char* co[7] = { "crbits", "dsamp", "tone", "wow", "flutter", "drive", "drywet" };
        for (int i = 0; i < 7; ++i) at (co[i], 708 + i * 79, 420, 78, 136);
        // LFOs
        for (int i = 0; i < sonic::kNumLfos; ++i)
        {
            const int x = 16 + i * 130;
            const juce::String pre = "lfo" + juce::String (i + 1) + "_";
            lfoViews[i]->setBounds (x + 9, 608, 102, 52);
            at (pre + "shape", x + 9, 663, 102, 38);
            at (pre + "div", x + 9, 703, 102, 38);
            at (pre + "rate", x + 9, 703, 102, 38);
            at (pre + "sync", x + 9, 749, 102, 28);
        }
        // sequencer
        at ("seq_len", 584, 604, 60, 32);
        at ("seq_dir", 650, 604, 110, 36);
        at ("seq_div", 766, 604, 90, 36);
        at ("seq_slide", 862, 604, 70, 32);
        for (int s = 0; s < 8; ++s)
        {
            const int x = 582 + s * 52;
            const juce::String pre = "s" + juce::String (s + 1) + "_";
            at (pre + "val", x, 644, 46, 52);
            at (pre + "pulses", x, 699, 46, 16);
            at (pre + "ratch", x, 717, 46, 16);
            at (pre + "gate", x - 2, 735, 50, 20);
            at (pre + "slide", x + 6, 757, 34, 14);
            at (pre + "prob", x, 773, 46, 13);
        }
        // matrix
        for (int m = 0; m < sonic::kNumSlots; ++m)
        {
            const int y = 620 + m * 21;
            const juce::String pre = "m" + juce::String (m + 1) + "_";
            at (pre + "src", 1026, y, 64, 20);
            at (pre + "dst", 1092, y, 98, 20);
            at (pre + "amt", 1194, y + 1, 62, 18);
        }
        updateVisibility();
    }

    sonic::PolarScope& getScope() { return scope; }
    sonic::Control* getControl (const juce::String& id) { return byId[id]; }

    /** Moves the knobs of the matrix destinations to their live values. */
    void updateModulationDisplay()
    {
        const auto count = proc.ui.blockCount.load (std::memory_order_relaxed);
        idleFrames = count != lastBlockCount ? 0 : idleFrames + 1;
        lastBlockCount = count;
        // no audio coming through (bypassed or stopped host): show the real values
        const juce::uint32 mask = idleFrames < kDisplayHz / 2 ? proc.ui.modMask.load() : 0;
        for (int d = 0; d < sonic::kNumDests; ++d)
            if (auto* ctl = destControls[(size_t) d])
                ctl->setModulation (((mask >> d) & 1u) != 0, proc.ui.modNorm[(size_t) d].load (std::memory_order_relaxed));
    }

private:
    void panel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
    {
        using namespace sonic;
        g.setColour (c::ink.withAlpha (0.18f));
        g.fillRoundedRectangle (r.translated (0, 4), 14.0f);
        g.setColour (c::skyLt.withAlpha (0.88f));
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (c::white);
        g.drawRoundedRectangle (r.reduced (1.5f), 14.0f, 3.0f);
        // slanted title ribbon
        const float tw = juce::jmax (90.0f, title.length() * 10.5f + 30);
        juce::Path rib;
        rib.addQuadrilateral (r.getX() + 14, r.getY() - 6, r.getX() + 14 + tw, r.getY() - 6,
                              r.getX() + 4 + tw, r.getY() + 20, r.getX() + 4, r.getY() + 20);
        g.setColour (c::royal);
        g.fillPath (rib);
        g.setColour (c::white);
        g.strokePath (rib, juce::PathStrokeType (2.0f));
        outlinedText (g, title, { r.getX() + 18, r.getY() - 5, tw - 10, 24 }, 15.0f);
    }

    void refreshPresets()
    {
        presetBox.clear (juce::dontSendNotification);
        presetBox.addItemList (proc.presets.getNames(), 1);
        presetBox.setSelectedItemIndex (proc.presets.getCurrent(), juce::dontSendNotification);
    }

    void updateVisibility()
    {
        auto v = [this] (const char* id) { return proc.state.getRawParameterValue (id)->load() > 0.5f; };
        if (auto* c = byId["freebpm"]) c->setVisible (v ("syncmode"));
        for (int i = 1; i <= sonic::kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i) + "_";
            const bool sync = proc.state.getRawParameterValue (pre + "sync")->load() > 0.5f;
            if (auto* c = byId[pre + "rate"]) c->setVisible (! sync);
            if (auto* c = byId[pre + "div"]) c->setVisible (sync);
        }
    }

    void timerCallback() override
    {
        updateModulationDisplay();
        if (++slowTick < kDisplayHz / 10) return;
        slowTick = 0;
        updateVisibility();
        repaint (536, 580, 470, 208);
        if (presetBox.getSelectedItemIndex() != proc.presets.getCurrent()) refreshPresets();
    }

    static constexpr int kDisplayHz = 30;

    SonicProcessor& proc;
    sonic::LookAndFeel lnf;
    sonic::WaveformView wave;
    sonic::PolarScope scope;
    juce::OwnedArray<sonic::LfoView> lfoViews;
    juce::TextButton prevBtn, nextBtn, saveBtn, delBtn;
    juce::ComboBox presetBox;
    juce::OwnedArray<sonic::Control> controls;
    juce::HashMap<juce::String, sonic::Control*> byId;
    std::array<sonic::Control*, sonic::kNumDests> destControls {};
    juce::uint32 lastBlockCount = 0;
    int idleFrames = 0, slowTick = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicEditor)
};
