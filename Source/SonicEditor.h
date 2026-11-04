#pragma once
#include "SonicProcessor.h"
#include "SonicUI.h"

namespace sonic
{
/** Layout grid of the 1280x800 view: one margin, one gap, one panel padding everywhere. */
namespace grid
{
    constexpr int W = 1280, H = 800;
    constexpr int M = 16;          // window edge, and padding inside every panel and the deck
    constexpr int G = 12;          // gap between panels
    constexpr int HeaderH = 64;
    constexpr int TitleH = 56;     // kicker + tagged section plate at the top of a panel
    constexpr int KnobH = 86;      // knob + caption + value

    const juce::Rectangle<int> grains  { M, HeaderH + G, 900, 272 };
    const juce::Rectangle<int> imager  { grains.getRight() + G, grains.getY(), W - M - grains.getRight() - G, grains.getHeight() };
    constexpr int row2Y = HeaderH + G + 272 + G, row2H = 158;
    const juce::Rectangle<int> filter  { M, row2Y, 332, row2H };
    const juce::Rectangle<int> glitch  { filter.getRight() + G, row2Y, 332, row2H };
    const juce::Rectangle<int> colour  { glitch.getRight() + G, row2Y, W - M - glitch.getRight() - G, row2H };
    constexpr int tabsY = row2Y + row2H + G, tabW = 170, tabH = 38;   // tab plates are 8 px smaller (shadow)
    constexpr int deckY = tabsY + tabH + 6;
    constexpr int matrixW = 364;
    constexpr int matrixX = W - M - matrixW;
    constexpr int deckLeftR = matrixX - 2 * M;                        // right edge of the page area

    /** Inner content area of a panel (below its title). */
    inline juce::Rectangle<int> content (juce::Rectangle<int> panel)
    {
        return panel.withTrimmedTop (TitleH).reduced (M, 0).withTrimmedBottom (M);
    }
    /** Cell i of n equal cells across r. */
    inline juce::Rectangle<int> cell (juce::Rectangle<int> r, int i, int n)
    {
        const float w = (float) r.getWidth() / (float) n;
        const int x0 = r.getX() + juce::roundToInt (w * (float) i), x1 = r.getX() + juce::roundToInt (w * (float) (i + 1));
        return { x0, r.getY(), x1 - x0, r.getHeight() };
    }
}

/** The whole interface, laid out on a fixed 1280x800 grid; the editor scales it to its size. */
class SonicView : public juce::Component, private juce::Timer
{
public:
    explicit SonicView (SonicProcessor& p) : proc (p), wave (p), scope (p)
    {
        // the static artwork lives in one cached layer, so an animated control never redraws it
        backdrop.draw = [this] (juce::Graphics& g) { paintBackdrop (g); };
        backdrop.setInterceptsMouseClicks (false, false);
        backdrop.setOpaque (true);
        backdrop.setBufferedToImage (true);
        addAndMakeVisible (backdrop);
        playhead.draw = [this] (juce::Graphics& g) { paintPlayhead (g); };
        playhead.setInterceptsMouseClicks (false, false);
        addChildComponent (playhead);

        using K = Control::Kind;
        auto add = [this, &p] (const juce::String& id, const juce::String& label, K k) {
            auto* ctl = controls.add (new Control (p.state, id, label, k));
            if (addingDeck) ctl->getProperties().set ("sgDeck", true);
            addAndMakeVisible (ctl);
            byId.set (id, ctl);
            return ctl;
        };
        // header
        // header: SYNC + TEMPO share one dark window, OUTPUT has its own
        for (auto* id : { "syncmode", "outdb" })
        {
            const juce::String s (id);
            auto* ctl = add (id, s == "syncmode" ? "SYNC" : "OUTPUT", s == "syncmode" ? K::Menu : K::Box);
            ctl->getProperties().set ("sgDark", true);
            ctl->setCaptionColour (c::onPlate);
            ctl->setCaptionJustification (s == "syncmode" ? juce::Justification::centredLeft : juce::Justification::centred);
        }
        addAndMakeVisible (tempo);
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
        for (int i = 1; i <= kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i) + "_";
            add (pre + "shape", "Shape", K::Menu);
            add (pre + "sync", "SYNC", K::Toggle);
            add (pre + "rate", "Rate", K::HBar);
            add (pre + "div", "Division", K::Menu);
            addAndMakeVisible (lfoViews.add (new LfoView (p, i - 1)));
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
        for (int m = 1; m <= kNumSlots; ++m)
        {
            const juce::String pre = "m" + juce::String (m) + "_";
            add (pre + "src", {}, K::Menu);
            add (pre + "dst", {}, K::Menu);
            add (pre + "amt", {}, K::HBar);
        }
        // everything in the deck sits on the green ground
        for (auto* ctl : controls)
            if (ctl->getProperties()["sgDeck"]) { ctl->getProperties().set ("sgDark", true); ctl->setCaptionColour (c::onPlate); }

        addAndMakeVisible (wave);
        addAndMakeVisible (scope);
        for (int d = 0; d < kNumDests; ++d) destControls[(size_t) d] = byId[kDestIds[d]];

        // modulation deck tabs
        for (int t = 0; t < kNumTabs; ++t)
        {
            auto* tab = tabs.add (new ModTab (t < kNumLfos ? "LFO " + juce::String (t + 1) : juce::String ("SEQ")));
            if (t < kNumLfos)
            {
                tab->thumbState = [this, t] {
                    const int shape = (int) proc.state.getRawParameterValue ("lfo" + juce::String (t + 1) + "_shape")->load();
                    if (! isRandomLfoShape (shape)) return shape;
                    // the real random output scrolls: redraw when it moves by a step of the drawing
                    return shape + 16 * (int) (proc.ui.lfoSteps[(size_t) t].load() * 24u + (juce::uint32) (proc.ui.lfoPhase[t].load() * 24.0f));
                };
                tab->drawThumb = [this, t] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col) {
                    traceLfo (g, proc, t, r, 3, 1.5f, col);
                };
            }
            else
            {
                tab->thumbState = [this] {
                    int h = proc.ui.seqStage.load();
                    for (int s = 1; s <= 8; ++s)
                        if (auto* prm = proc.state.getParameter ("s" + juce::String (s) + "_val"))
                            h = h * 31 + juce::roundToInt (prm->getValue() * 64.0f);
                    return h;
                };
                tab->drawThumb = [this] (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col) {
                    const int st = proc.ui.seqStage.load();
                    const float w = r.getWidth() / 8.0f;
                    for (int s = 0; s < 8; ++s)
                    {
                        auto* prm = proc.state.getParameter ("s" + juce::String (s + 1) + "_val");
                        const float n = prm != nullptr ? prm->getValue() : 0.5f;
                        const float h = juce::jmax (2.0f, n * r.getHeight());
                        g.setColour (s == st ? c::signal : col);
                        g.fillRect (r.getX() + (float) s * w + 1, r.getBottom() - h, w - 2, h);
                    }
                };
            }
            tab->onClick = [this] { updateVisibility(); updatePlayhead(); backdrop.repaint(); };
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

        setSize (grid::W, grid::H);
        startTimerHz (kDisplayHz);
    }

    void paintBackdrop (juce::Graphics& g)
    {
        using namespace grid;
        // ground, checkerboard on the lower half, orbit rings
        g.fillAll (c::ground);
        g.setColour (c::checker);
        for (int y = 352; y < deckY; y += 32)
            for (int x = ((y / 32) % 2) * 32; x < W; x += 64)
                g.fillRect (x, y, 32, juce::jmin (32, deckY - y));
        g.setColour (c::orbitB); g.drawEllipse (930 - 470, 430 - 470, 940, 940, 10.0f);
        g.setColour (c::orbitA); g.drawEllipse (930 - 370, 430 - 370, 740, 740, 10.0f);
        g.fillEllipse (668 - 22, 168 - 22, 44, 44);

        // header strip, wordmark, plates behind the preset menu and the global controls
        g.setColour (c::groundAlt);
        g.fillRect (0, 0, W, HeaderH);
        g.setFont (black (38.0f, -0.01f));
        const juce::String name ("SONIC GRAIN");
        const juce::Rectangle<int> wm (M, 0, 300, HeaderH);
        g.setColour (c::go);      g.drawText (name, wm.translated (5, 5), juce::Justification::centredLeft);
        g.setColour (c::signal);  g.drawText (name, wm.translated (3, 3), juce::Justification::centredLeft);
        g.setColour (c::onPlate); g.drawText (name, wm, juce::Justification::centredLeft);
        taggedPlate (g, presetPlate.toFloat(), c::tag);
        headerWindow (g, syncWindow.toFloat());
        g.setColour (c::onPlate.withAlpha (0.2f));
        g.fillRect (tempo.getX() - 8, syncWindow.getY() + 8, 1, syncWindow.getHeight() - 16);
        headerWindow (g, outWindow.toFloat());

        glassPanel (g, grains.toFloat(), "Grain", "Cloud");
        glassPanel (g, imager.toFloat(), "Stereo", "Imager", "L C R");
        glassPanel (g, filter.toFloat(), "Tone", "Filter");
        glassPanel (g, glitch.toFloat(), "Time", "Glitch");
        glassPanel (g, colour.toFloat(), "Tape", "Colour");

        // the matrix has no tab: its tag sits on the tab row, above its column
        {
            const auto f = black (12.0f, 0.04f);
            auto r = juce::Rectangle<float> ((float) matrixX, (float) tabsY, (float) tabW - 8, (float) tabH - 8);
            g.setColour (juce::Colour (0x59000000)); g.fillPath (platePath (r), juce::AffineTransform::translation (3, 3));   // like FREEZE
            g.setColour (c::plate); g.fillPath (platePath (r));
            g.setColour (c::onPlate); g.setFont (f);
            g.drawText ("MOD MATRIX", r, juce::Justification::centred, false);
        }

        // modulation deck
        g.setColour (c::modGround);
        g.fillRect (0, deckY, W, H - deckY);
        g.setColour (c::onPlate.withAlpha (0.18f));
        g.fillRect (matrixX - M, deckY + M, 1, H - deckY - 2 * M);
        g.setFont (bold (10.0f, 0.06f));
        g.setColour (c::onPlate.withAlpha (0.8f));
        g.drawText ("SOURCE", matrixX, deckY + M, 90, 14, juce::Justification::left);
        g.drawText ("DESTINATION", matrixX + 96, deckY + M, 124, 14, juce::Justification::left);
        g.drawText ("AMOUNT", matrixX + 226, deckY + M, 138, 14, juce::Justification::left);

        if (activeTab() == kNumLfos)
        {   // sequencer row labels + playhead
            const char* rows[6] = { "VAL", "PLS", "RAT", "GATE", "SLD", "PRB" };
            for (int i = 0; i < 6; ++i)
                g.drawText (rows[i], kSeqLabelX, seqRow (i).getY(), 40, seqRow (i).getHeight(),
                            i == 0 ? juce::Justification::topLeft : juce::Justification::centredLeft);
        }
    }

    void paintPlayhead (juce::Graphics& g)
    {
        const auto col = playhead.getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (c::signal.withAlpha (0.22f));
        g.fillRoundedRectangle (col, 5.0f);
        g.setColour (shownGate ? c::signal : c::onPlate.withAlpha (0.5f));
        g.drawRoundedRectangle (col, 5.0f, 2.0f);
    }

    /** Moves the sequencer playhead when it has something new to show. */
    void updatePlayhead()
    {
        using namespace grid;
        const bool shown = activeTab() == kNumLfos;
        playhead.setVisible (shown);
        if (! shown) return;
        const int st = proc.ui.seqStage.load();
        const bool gate = proc.ui.seqGate.load() > 0.5f;
        if (st == shownStage && gate == shownGate) return;
        shownStage = st; shownGate = gate;
        playhead.setBounds (seqStage (st).withY (deckY + M - 4).withBottom (H - M + 4).expanded (6, 2));
        playhead.repaint();
    }

    void resized() override
    {
        using namespace grid;
        backdrop.setBounds (getLocalBounds());
        auto at = [this] (const juce::String& id, juce::Rectangle<int> r) { if (auto* c = byId[id]) c->setBounds (r); };
        // header: everything centred on the strip
        const int cy = HeaderH / 2;
        prevBtn.setBounds (300, cy - 15, 40, 30);
        presetPlate = { 348, cy - 17, 270, 34 };
        presetBox.setBounds (presetPlate.getX() + 24, cy - 11, presetPlate.getWidth() - 48, 22);
        nextBtn.setBounds (presetPlate.getRight() + 10, cy - 15, 40, 30);
        saveBtn.setBounds (nextBtn.getRight() + 6, cy - 15, 76, 30);
        delBtn.setBounds (saveBtn.getRight() + 2, cy - 15, 64, 30);
        // right-aligned dark windows, G apart
        // SYNC | TEMPO at the right edge, where the other plugins of the family have it; OUTPUT to its left
        syncWindow = { W - M - 236, cy - 22, 236, 44 };
        outWindow = { syncWindow.getX() - G - 120, cy - 22, 120, 44 };
        at ("outdb", outWindow.reduced (10, 4));
        at ("syncmode", { syncWindow.getX() + 10, syncWindow.getY() + 4, 84, 36 });
        tempo.setBounds (syncWindow.getX() + 112, syncWindow.getY() + 4, syncWindow.getRight() - 10 - (syncWindow.getX() + 112), 36);
        // grains
        auto gc = content (grains);
        wave.setBounds (gc.removeFromTop (gc.getHeight() - KnobH - 8));
        gc.removeFromTop (8);
        const char* g[11] = { "gpos", "gsize", "gdens", "gpitch", "spray", "prand", "grev", "gshape", "gwidth", "feedback", "cloud" };
        for (int i = 0; i < 11; ++i) at (g[i], cell (gc, i, 11));
        // its shadow ends 8 px above the hairline, the same gap the title text leaves
        at ("freeze", { grains.getRight() - M - 112, grains.getY() + 13, 112, 27 });
        // imager
        auto ic = content (imager);
        scope.setBounds (ic.removeFromTop (ic.getHeight() - KnobH - 8));
        ic.removeFromTop (8);
        at ("imgwidth", cell (ic, 0, 2));
        at ("monofreq", cell (ic, 1, 2));
        // filter / glitch / colour: 86 px cells across each panel
        auto fc = content (filter);
        const char* f[4] = { "hpf", "hpq", "lpf", "lreso" };
        for (int i = 0; i < 4; ++i) at (f[i], cell (fc, i, 4));
        auto xc = content (glitch);
        at ("glitch", cell (xc, 0, 4));
        // menu and bar sit with their field on the knobs' centre line (29 px down), caption above
        at ("slice", cell (xc, 1, 4).reduced (6, 0).withHeight (38).translated (0, 2));
        at ("rptpitch", cell (xc, 2, 4));
        at ("rhold", cell (xc, 3, 4).reduced (6, 0).withHeight (34).translated (0, 4));
        auto cc = content (colour);
        const char* co[7] = { "crbits", "dsamp", "tone", "wow", "flutter", "drive", "drywet" };
        for (int i = 0; i < 7; ++i) at (co[i], cell (cc, i, 7));
        // deck tabs
        for (int t = 0; t < kNumTabs; ++t) tabs[t]->setBounds (M + t * tabW, tabsY, tabW, tabH);
        // deck pages: inner area of the deck, left of the matrix
        const juce::Rectangle<int> page (M, deckY + M, deckLeftR - M, H - deckY - 2 * M);
        for (int i = 0; i < kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i + 1) + "_";
            auto r = page;
            lfoViews[i]->setBounds (r.removeFromLeft (r.getWidth() - 252 - M));
            r.removeFromLeft (M);
            at (pre + "shape", r.removeFromTop (40));
            r.removeFromTop (10);
            const auto second = r.removeFromTop (40);
            at (pre + "div", second);
            at (pre + "rate", second.withHeight (36));
            r.removeFromTop (12);
            at (pre + "sync", r.removeFromTop (30).withWidth (124));
        }
        {
            auto r = page.withWidth (140);
            at ("seq_len", r.removeFromTop (34).withWidth (72)); r.removeFromTop (8);
            at ("seq_dir", r.removeFromTop (40)); r.removeFromTop (8);
            at ("seq_div", r.removeFromTop (40)); r.removeFromTop (8);
            at ("seq_slide", r.removeFromTop (34));
        }
        for (int s = 0; s < 8; ++s)
        {
            const auto x = seqStage (s);
            const juce::String pre = "s" + juce::String (s + 1) + "_";
            const char* ids[6] = { "val", "pulses", "ratch", "gate", "slide", "prob" };
            for (int row = 0; row < 6; ++row)
            {
                auto r = seqRow (row).withX (x.getX()).withWidth (x.getWidth());
                if (row == 4) r = r.reduced (8, 0);
                at (pre + ids[row], r);
            }
        }
        // matrix
        for (int m = 0; m < kNumSlots; ++m)
        {
            const int y = deckY + M + 20 + m * 22;
            const juce::String pre = "m" + juce::String (m + 1) + "_";
            at (pre + "src", { matrixX, y, 90, 20 });
            at (pre + "dst", { matrixX + 96, y, 124, 20 });
            at (pre + "amt", { matrixX + 226, y + 1, matrixW - 226, 18 });
        }
        updateVisibility();
    }

    PolarScope& getScope() { return scope; }

    /** Menus on plates and on the deck get light text (the look and feel's default is ink, for glass). */
    void applyDarkColours()
    {
        for (auto* ctl : controls)
            if (ctl->getProperties()["sgDark"]) ctl->combo.setColour (juce::ComboBox::textColourId, c::onPlate);
        presetBox.setColour (juce::ComboBox::textColourId, c::onPlate);
    }
    Control* getControl (const juce::String& id) { return byId[id]; }

    /** Shows one page of the modulation deck: 0..3 = LFO 1..4, 4 = METRO SEQ. */
    void showDeckTab (int t)
    {
        if (auto* tab = tabs[juce::jlimit (0, kNumTabs - 1, t)]) tab->setToggleState (true, juce::dontSendNotification);
        updateVisibility();
        updatePlayhead();
        backdrop.repaint();
    }

    /** Moves the knobs of the matrix destinations to their live values. */
    void updateModulationDisplay()
    {
        const auto count = proc.ui.blockCount.load (std::memory_order_relaxed);
        idleFrames = count != lastBlockCount ? 0 : idleFrames + 1;
        lastBlockCount = count;
        // no audio coming through (bypassed or stopped host): show the real values
        const juce::uint32 mask = idleFrames < kDisplayHz / 2 ? proc.ui.modMask.load() : 0;
        for (int d = 0; d < kNumDests; ++d)
            if (auto* ctl = destControls[(size_t) d])
                ctl->setModulation (((mask >> d) & 1u) != 0, proc.ui.modNorm[(size_t) d].load (std::memory_order_relaxed));
    }

private:
    static constexpr int kDisplayHz = 30, kNumTabs = kNumLfos + 1;
    static constexpr int kSeqLabelX = grid::M + 140 + grid::M;   // after the sequencer's settings column
    static constexpr int kSeqX = kSeqLabelX + 44;

    /** Column of sequencer stage s, spread evenly between the row labels and the page edge. */
    static juce::Rectangle<int> seqStage (int s)
    {
        const auto area = juce::Rectangle<int> (kSeqX, 0, grid::deckLeftR - kSeqX, 1);
        return grid::cell (area, s, 8).reduced (5, 0);
    }
    /** Row r of the sequencer grid (VAL, PLS, RAT, GATE, SLD, PRB), filling the deck's inner height. */
    static juce::Rectangle<int> seqRow (int r)
    {
        static const int h[6] = { 82, 18, 18, 20, 18, 18 };
        int y = grid::deckY + grid::M;
        for (int i = 0; i < r; ++i) y += h[i] + 4;
        return { 0, y, 1, h[r] };
    }

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
        const int tab = activeTab();
        for (int i = 1; i <= kNumLfos; ++i)
        {
            const juce::String pre = "lfo" + juce::String (i) + "_";
            const bool shown = tab == i - 1;
            const bool sync = proc.state.getRawParameterValue (pre + "sync")->load() > 0.5f;
            lfoViews[i - 1]->setVisible (shown);
            for (auto* id : { "shape", "sync" }) if (auto* c = byId[pre + id]) c->setVisible (shown);
            if (auto* c = byId[pre + "rate"]) c->setVisible (shown && ! sync);
            if (auto* c = byId[pre + "div"]) c->setVisible (shown && sync);
        }
        const bool seqShown = tab == kNumLfos;
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
        updatePlayhead();
        if (presetBox.getSelectedItemIndex() != proc.presets.getCurrent()) refreshPresets();
    }

    /** A child that draws through its owner: one for the cached artwork, one for the playhead. */
    struct Layer : juce::Component
    {
        std::function<void (juce::Graphics&)> draw;
        void paint (juce::Graphics& g) override { draw (g); }
    };

    SonicProcessor& proc;
    Layer backdrop, playhead;
    int shownStage = -1;
    bool shownGate = false;
    WaveformView wave;
    PolarScope scope;
    juce::OwnedArray<LfoView> lfoViews;
    juce::OwnedArray<ModTab> tabs;
    juce::TextButton prevBtn, nextBtn, saveBtn, delBtn;
    juce::ComboBox presetBox;
    juce::Rectangle<int> presetPlate, syncWindow, outWindow;
    TempoBox tempo { proc };
    juce::OwnedArray<Control> controls;
    juce::HashMap<juce::String, Control*> byId;
    std::array<Control*, kNumDests> destControls {};
    juce::uint32 lastBlockCount = 0;
    int idleFrames = 0, slowTick = 0;
    bool addingDeck = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicView)
};
} // namespace sonic

/** Resizable editor: the 1280x800 view is scaled as a whole (vector drawing, so it stays sharp at any size and
    DPI) and the window keeps the 16:10 proportions. The last size is stored with the plugin state. */
class SonicEditor : public juce::AudioProcessorEditor
{
public:
    explicit SonicEditor (SonicProcessor& p) : AudioProcessorEditor (p), proc (p), view (p)
    {
        setLookAndFeel (&lnf);
        addAndMakeVisible (view);
        // the controls were built before they had a parent: rebuild their text boxes with this look and feel
        view.sendLookAndFeelChange();
        view.applyDarkColours();

        setResizable (true, true);
        setResizeLimits (kMinW, kMinW * sonic::grid::H / sonic::grid::W, kMaxW, kMaxW * sonic::grid::H / sonic::grid::W);
        if (auto* cons = getConstrainer()) cons->setFixedAspectRatio ((double) sonic::grid::W / (double) sonic::grid::H);
        const int w = juce::jlimit (kMinW, kMaxW, (int) proc.state.state.getProperty (kSizeKey, sonic::grid::W));
        setSize (w, w * sonic::grid::H / sonic::grid::W);
        ready = true;   // from now on a new size is the user's choice: remember it
    }
    ~SonicEditor() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override { g.fillAll (sonic::c::ground); }

    void resized() override
    {
        const float scale = (float) getWidth() / (float) sonic::grid::W;
        view.setTransform (juce::AffineTransform::scale (scale));
        if (ready && getWidth() != (int) proc.state.state.getProperty (kSizeKey, sonic::grid::W))
            proc.state.state.setProperty (kSizeKey, getWidth(), nullptr);
    }

    sonic::PolarScope& getScope() { return view.getScope(); }
    sonic::Control* getControl (const juce::String& id) { return view.getControl (id); }
    void showDeckTab (int t) { view.showDeckTab (t); }
    void updateModulationDisplay() { view.updateModulationDisplay(); }

private:
    static constexpr int kMinW = 800, kMaxW = 2560;
    static inline const juce::Identifier kSizeKey { "uiWidth" };
    SonicProcessor& proc;
    sonic::LookAndFeel lnf;
    sonic::SonicView view;
    bool ready = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicEditor)
};
