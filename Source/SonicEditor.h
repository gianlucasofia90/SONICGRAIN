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
            if (addingDeck) ctl->getProperties().set ("sgDeck", true);
            addAndMakeVisible (ctl);
            byId.set (id, ctl);
            return ctl;
        };
        // header
        for (auto* id : { "syncmode", "freebpm", "outdb" })
        {
            auto* ctl = add (id, juce::String (id) == "syncmode" ? "SYNC" : (juce::String (id) == "freebpm" ? "BPM" : "OUTPUT"),
                             juce::String (id) == "syncmode" ? K::Menu : K::Box);
            ctl->getProperties().set ("sgDark", true);
            ctl->setCaptionColour (sonic::c::onPlate);
        }
        // grains
        for (auto pr : { std::pair<const char*, const char*> { "gpos", "Position" }, { "gsize", "Size" },
                          { "gdens", "Density" }, { "gpitch", "Pitch" }, { "spray", "Spray" }, { "prand", "Pitch Rnd" },
                          { "grev", "Reverse" }, { "gshape", "Shape" }, { "gwidth", "Spread" },
                          { "feedback", "Feedback" }, { "cloud", "Cloud" } })
            add (pr.first, pr.second, K::Knob);
        add ("freeze", "FREEZE", K::Toggle)->button.getProperties().set ("sgRec", true);
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
        add ("rhold", "Hold", K::HBar);
        // LFOs (one at a time in the modulation deck)
        addingDeck = true;
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
        add ("seq_slide", "Slide", K::HBar);
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
        // everything in the deck sits on the green ground
        for (auto* ctl : controls)
            if (ctl->getProperties()["sgDeck"]) { ctl->getProperties().set ("sgDark", true); ctl->setCaptionColour (sonic::c::onPlate); }

        addAndMakeVisible (wave);
        addAndMakeVisible (scope);
        for (int d = 0; d < sonic::kNumDests; ++d) destControls[(size_t) d] = byId[sonic::kDestIds[d]];

        // modulation deck tabs
        for (int t = 0; t < kNumTabs; ++t)
        {
            auto* tab = tabs.add (new sonic::ModTab (t < sonic::kNumLfos ? "LFO " + juce::String (t + 1) : juce::String ("Metro Seq")));
            if (t < sonic::kNumLfos)
                tab->drawThumb = [this, t] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col) {
                    const int shape = (int) proc.state.getRawParameterValue ("lfo" + juce::String (t + 1) + "_shape")->load();
                    juce::Path path;
                    for (int i = 0; i <= 40; ++i)
                    {
                        const float q = i / 40.0f;
                        const juce::Point<float> pt (r.getX() + q * r.getWidth(), r.getCentreY() - 0.5f * r.getHeight() * sonic::lfoShapeValue (shape, q));
                        if (i == 0) path.startNewSubPath (pt); else path.lineTo (pt);
                    }
                    g.setColour (col);
                    g.strokePath (path, juce::PathStrokeType (1.5f));
                };
            else
                tab->drawThumb = [this] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col) {
                    const int st = proc.ui.seqStage.load();
                    const float w = r.getWidth() / 8.0f;
                    for (int s = 0; s < 8; ++s)
                    {
                        const float v = proc.state.getRawParameterValue ("s" + juce::String (s + 1) + "_val")->load();
                        const float n = (float) proc.state.getParameter ("s" + juce::String (s + 1) + "_val")->convertTo0to1 (v);
                        auto b = juce::Rectangle<float> (r.getX() + s * w + 1, r.getBottom() - juce::jmax (2.0f, n * r.getHeight()), w - 2, juce::jmax (2.0f, n * r.getHeight()));
                        g.setColour (s == st ? sonic::c::signal : col);
                        g.fillRect (b);
                    }
                };
            tab->onClick = [this] { updateVisibility(); repaint(); };
            addAndMakeVisible (tab);
        }
        tabs[0]->setToggleState (true, juce::dontSendNotification);

        // presets
        for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &delBtn }) addAndMakeVisible (b);
        prevBtn.setButtonText ("<"); nextBtn.setButtonText (">");
        saveBtn.setButtonText ("SAVE"); delBtn.setButtonText ("DEL");
        addAndMakeVisible (presetBox);
        presetBox.getProperties().set ("sgDark", true);
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

        // the controls were built before they had a parent: rebuild their text boxes with this look and feel
        sendLookAndFeelChange();
        setSize (1280, 800);
        startTimerHz (kDisplayHz);
    }
    ~SonicEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        using namespace sonic;
        const float W = (float) getWidth();
        // ground, checkerboard on the lower half, orbit rings
        g.fillAll (c::ground);
        g.setColour (c::checker);
        for (int y = 352; y < kDeckY; y += 32)
            for (int x = ((y / 32) % 2) * 32; x < getWidth(); x += 64)
                g.fillRect (x, y, 32, juce::jmin (32, kDeckY - y));
        g.setColour (c::orbitB); g.drawEllipse (930 - 470, 430 - 470, 940, 940, 10.0f);
        g.setColour (c::orbitA); g.drawEllipse (930 - 370, 430 - 370, 740, 740, 10.0f);
        g.fillEllipse (668 - 22, 168 - 22, 44, 44);

        // header strip, wordmark, plates behind the global controls
        g.setColour (c::groundAlt);
        g.fillRect (0.0f, 0.0f, W, 64.0f);
        g.setFont (black (38.0f, -0.01f));
        const juce::String name ("SONIC GRAIN");
        g.setColour (c::go);      g.drawText (name, 21, 10, 330, 44, juce::Justification::centredLeft);
        g.setColour (c::signal);  g.drawText (name, 19, 8, 330, 44, juce::Justification::centredLeft);
        g.setColour (c::onPlate); g.drawText (name, 16, 5, 330, 44, juce::Justification::centredLeft);
        taggedPlate (g, { 446, 14, 296, 34 }, c::tag);
        for (auto* id : { "syncmode", "freebpm", "outdb" })
            if (auto* ctl = byId[id]; ctl != nullptr && ctl->isVisible())
            {
                auto b = ctl->getBounds().toFloat().expanded (14, 4);
                g.setColour (juce::Colour (0x59000000)); g.fillPath (platePath (b), juce::AffineTransform::translation (3, 3));
                g.setColour (c::plate); g.fillPath (platePath (b));
            }

        glassPanel (g, { 16, 76, 900, 284 }, "Grain", "Cloud");
        glassPanel (g, { 928, 76, 336, 284 }, "Stereo", "Imager");
        glassPanel (g, { 16, 372, 330, 140 }, "Tone", "Filter");
        glassPanel (g, { 358, 372, 330, 140 }, "Time", "Glitch");
        glassPanel (g, { 700, 372, 564, 140 }, "Tape", "Colour");

        // modulation deck
        g.setColour (c::modGround);
        g.fillRect (0, kDeckY, getWidth(), getHeight() - kDeckY);
        g.setColour (c::onPlate.withAlpha (0.18f));
        g.fillRect (kMatrixX - 16, kDeckY + 12, 1, getHeight() - kDeckY - 24);
        g.setFont (bold (10.0f, 0.06f));
        g.setColour (c::onPlate);
        g.drawText ("MOD MATRIX", kMatrixX, kDeckY + 10, 120, 12, juce::Justification::left);
        g.setColour (c::onPlate.withAlpha (0.8f));
        g.drawText ("SOURCE", kMatrixX, kDeckY + 26, 80, 12, juce::Justification::left);
        g.drawText ("DESTINATION", kMatrixX + 96, kDeckY + 26, 110, 12, juce::Justification::left);
        g.drawText ("AMOUNT", kMatrixX + 228, kDeckY + 26, 80, 12, juce::Justification::left);

        if (activeTab() == sonic::kNumLfos)
        {   // sequencer row labels + playhead
            const char* rows[6] = { "VAL", "PLS", "RAT", "GATE", "SLD", "PRB" };
            const int ry[6] = { 624, 692, 712, 733, 756, 776 };
            g.setColour (c::onPlate.withAlpha (0.8f));
            for (int i = 0; i < 6; ++i) g.drawText (rows[i], kSeqX - 46, ry[i], 40, 14, juce::Justification::left);
            const int st = proc.ui.seqStage.load();
            auto col = juce::Rectangle<float> ((float) (kSeqX - 4 + st * kSeqStep), 612.0f, (float) kSeqStep - 4, 182.0f);
            g.setColour (c::signal.withAlpha (0.22f));
            g.fillRoundedRectangle (col, 5.0f);
            g.setColour (proc.ui.seqGate.load() > 0.5f ? c::signal : c::onPlate.withAlpha (0.5f));
            g.drawRoundedRectangle (col, 5.0f, 2.0f);
        }
    }

    void resized() override
    {
        auto at = [this] (const juce::String& id, int x, int y, int w, int h) { if (auto* c = byId[id]) c->setBounds (x, y, w, h); };
        // header
        prevBtn.setBounds (404, 16, 40, 30);
        presetBox.setBounds (474, 20, 236, 22);
        nextBtn.setBounds (744, 16, 40, 30);
        saveBtn.setBounds (790, 16, 76, 30);
        delBtn.setBounds (866, 16, 64, 30);
        at ("syncmode", 968, 12, 80, 38);
        at ("freebpm", 1070, 12, 72, 38);
        at ("outdb", 1170, 12, 90, 38);
        // grains
        wave.setBounds (32, 132, 868, 122);
        at ("freeze", 784, 92, 118, 30);
        const char* g[11] = { "gpos", "gsize", "gdens", "gpitch", "spray", "prand", "grev", "gshape", "gwidth", "feedback", "cloud" };
        for (int i = 0; i < 11; ++i) at (g[i], 30 + i * 79, 260, 78, 94);
        // imager
        scope.setBounds (940, 132, 312, 136);
        at ("imgwidth", 990, 270, 90, 86);
        at ("monofreq", 1112, 270, 90, 86);
        // filter / glitch / colour
        const char* f[4] = { "hpf", "hpq", "lpf", "lreso" };
        for (int i = 0; i < 4; ++i) at (f[i], 26 + i * 78, 422, 76, 86);
        at ("glitch", 366, 422, 76, 86);
        at ("slice", 452, 436, 70, 40);
        at ("rptpitch", 528, 422, 76, 86);
        at ("rhold", 612, 436, 66, 36);
        const char* co[7] = { "crbits", "dsamp", "tone", "wow", "flutter", "drive", "drywet" };
        for (int i = 0; i < 7; ++i) at (co[i], 710 + i * 79, 422, 78, 86);
        // deck tabs
        for (int t = 0; t < kNumTabs; ++t) tabs[t]->setBounds (16 + t * 164, 524, 170, 38);
        // LFOs (same place, one shown)
        for (int i = 0; i < sonic::kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i + 1) + "_";
            lfoViews[i]->setBounds (24, kDeckY + 14, 440, 210);
            at (pre + "shape", 488, kDeckY + 16, 200, 40);
            at (pre + "div", 488, kDeckY + 68, 200, 40);
            at (pre + "rate", 488, kDeckY + 68, 200, 38);
            at (pre + "sync", 488, kDeckY + 122, 120, 32);
        }
        // sequencer
        at ("seq_len", 24, kDeckY + 14, 70, 34);
        at ("seq_dir", 24, kDeckY + 58, 130, 40);
        at ("seq_div", 24, kDeckY + 108, 130, 40);
        at ("seq_slide", 24, kDeckY + 158, 130, 34);
        for (int s = 0; s < 8; ++s)
        {
            const int x = kSeqX + s * kSeqStep;
            const juce::String pre = "s" + juce::String (s + 1) + "_";
            at (pre + "val", x, 616, kSeqStep - 12, 66);
            at (pre + "pulses", x, 690, kSeqStep - 12, 17);
            at (pre + "ratch", x, 710, kSeqStep - 12, 17);
            at (pre + "gate", x - 2, 730, kSeqStep - 8, 20);
            at (pre + "slide", x + 4, 753, kSeqStep - 18, 18);
            at (pre + "prob", x, 774, kSeqStep - 12, 16);
        }
        // matrix
        for (int m = 0; m < sonic::kNumSlots; ++m)
        {
            const int y = kDeckY + 42 + m * 24;
            const juce::String pre = "m" + juce::String (m + 1) + "_";
            at (pre + "src", kMatrixX, y, 90, 21);
            at (pre + "dst", kMatrixX + 96, y, 124, 21);
            at (pre + "amt", kMatrixX + 228, y + 1, 128, 19);
        }
        updateVisibility();
    }

    sonic::PolarScope& getScope() { return scope; }
    /** Shows one page of the modulation deck: 0..3 = LFO 1..4, 4 = METRO SEQ. */
    void showDeckTab (int t)
    {
        if (auto* tab = tabs[juce::jlimit (0, kNumTabs - 1, t)]) tab->setToggleState (true, juce::dontSendNotification);
        updateVisibility();
        repaint();
    }
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
    static constexpr int kDisplayHz = 30, kNumTabs = sonic::kNumLfos + 1;
    static constexpr int kDeckY = 568, kMatrixX = 900, kSeqX = 228, kSeqStep = 78;

    int activeTab() const
    {
        for (int t = 0; t < tabs.size(); ++t) if (tabs[t]->getToggleState()) return t;
        return 0;
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
        const int tab = activeTab();
        for (int i = 1; i <= sonic::kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i) + "_";
            const bool shown = tab == i - 1;
            const bool sync = proc.state.getRawParameterValue (pre + "sync")->load() > 0.5f;
            lfoViews[i - 1]->setVisible (shown);
            for (auto* id : { "shape", "sync" }) if (auto* c = byId[pre + id]) c->setVisible (shown);
            if (auto* c = byId[pre + "rate"]) c->setVisible (shown && ! sync);
            if (auto* c = byId[pre + "div"]) c->setVisible (shown && sync);
        }
        const bool seqShown = tab == sonic::kNumLfos;
        for (auto* id : { "seq_len", "seq_dir", "seq_div", "seq_slide" }) if (auto* c = byId[id]) c->setVisible (seqShown);
        for (int s = 1; s <= 8; ++s)
            for (auto* id : { "val", "pulses", "ratch", "gate", "slide", "prob" })
                if (auto* c = byId["s" + juce::String (s) + "_" + id]) c->setVisible (seqShown);
    }

    void timerCallback() override
    {
        updateModulationDisplay();
        if (++slowTick < kDisplayHz / 10) return;
        slowTick = 0;
        updateVisibility();
        repaint (0, kDeckY, getWidth(), getHeight() - kDeckY);
        repaint (940, 0, 340, 64);
        if (presetBox.getSelectedItemIndex() != proc.presets.getCurrent()) refreshPresets();
    }

    SonicProcessor& proc;
    sonic::LookAndFeel lnf;
    sonic::WaveformView wave;
    sonic::PolarScope scope;
    juce::OwnedArray<sonic::LfoView> lfoViews;
    juce::OwnedArray<sonic::ModTab> tabs;
    juce::TextButton prevBtn, nextBtn, saveBtn, delBtn;
    juce::ComboBox presetBox;
    juce::OwnedArray<sonic::Control> controls;
    juce::HashMap<juce::String, sonic::Control*> byId;
    std::array<sonic::Control*, sonic::kNumDests> destControls {};
    juce::uint32 lastBlockCount = 0;
    int idleFrames = 0, slowTick = 0;
    bool addingDeck = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicEditor)
};
