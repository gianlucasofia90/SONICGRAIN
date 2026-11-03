// SONIC GRAIN headless tests: drive the processor with a fake host transport.
// Run:  SonicGrainTests            -> unit tests (exit code 1 on failure)
//       SonicGrainTests --snap [f] [tab] [scale] -> render the editor to a PNG (default snap_sonic.png);
//                                               tab 0-3 = LFO 1-4, 4 = METRO SEQ; scale = pixel density (default 2)
#include <juce_audio_processors/juce_audio_processors.h>
#include "SonicProcessor.h"
#include "SonicEditor.h"
#include <map>
#include <set>
#include <cstdio>

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 120; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (playing);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

static int failures = 0;
#define CHECK(c, msg) do { if (! (c)) { std::printf ("FAIL: %s\n", msg); ++failures; } else std::printf ("ok   %s\n", msg); } while (0)

static void setP (juce::AudioProcessorValueTreeState& s, const char* id, float v)
{
    auto* p = s.getParameter (id);
    p->setValueNotifyingHost (p->convertTo0to1 (v));
}

static void runSnap (const juce::String& path, int deckTab = 0, float scale = 2.0f);

int main (int argc, char** argv)
{
    if (argc > 1 && juce::String (argv[1]) == "--snap") { runSnap (argc > 2 ? juce::String (argv[2]) : juce::String ("snap_sonic.png"), argc > 3 ? juce::String (argv[3]).getIntValue() : 0,
                                                                       argc > 4 ? juce::String (argv[4]).getFloatValue() : 2.0f); return 0; }
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000.0;

    // ---------------- SONIC GRAIN
    auto feed = [] (SonicProcessor& p, FakeHead& head, double srate, int blocks, double& ph, bool stereoSkew,
                    bool& finite, double& energy, double& maxAbs, double* lrDiff = nullptr) {
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                float x = 0.3f * (float) std::sin (ph) + 0.05f * (juce::Random::getSystemRandom().nextFloat() - 0.5f);
                ph += 2 * juce::MathConstants<double>::pi * 220.0 / srate;
                buf.setSample (0, i, x); buf.setSample (1, i, stereoSkew ? x * 0.2f : x);
            }
            p.processBlock (buf, midi);
            head.ppq += 512 * head.bpm / (60.0 * srate);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i)
            {
                float y = buf.getSample (c, i);
                if (! std::isfinite (y)) finite = false;
                energy += y * y; maxAbs = std::max (maxAbs, (double) std::abs (y));
            }
            if (lrDiff) for (int i = 0; i < 512; ++i) *lrDiff += std::abs (buf.getSample (0, i) - buf.getSample (1, i));
        }
    };
    {
        SonicProcessor gp; FakeHead head; head.bpm = 120;
        gp.setPlayHead (&head);
        gp.setPlayConfigDetails (2, 2, sr, 512);
        gp.prepareToPlay (sr, 512);
        auto& s = gp.state;
        setP (s, "cloud", 80); setP (s, "glitch", 100); setP (s, "slice", 3); setP (s, "rptpitch", 5);
        setP (s, "crbits", 8); setP (s, "dsamp", 4); setP (s, "wow", 60); setP (s, "flutter", 40); setP (s, "drive", 50);
        setP (s, "grev", 50); setP (s, "prand", 12); setP (s, "feedback", 60); setP (s, "tone", 4000);
        setP (s, "hpf", 300); setP (s, "hpq", 8); setP (s, "lpf", 2500); setP (s, "lreso", 1); setP (s, "imgwidth", 200);
        // full modulation: every slot active, seq with ratchets
        const int dsts[8] = { 0, 1, 2, 22, 20, 24, 10, 3 };
        for (int m = 1; m <= 8; ++m)
        {
            juce::String pre = "m" + juce::String (m) + "_";
            setP (s, (pre + "src").toRawUTF8(), (float) (1 + (m - 1) % 4));
            setP (s, (pre + "dst").toRawUTF8(), (float) dsts[m - 1]);
            setP (s, (pre + "amt").toRawUTF8(), m % 2 ? 100.0f : -100.0f);
        }
        for (int st = 1; st <= 8; ++st)
        {
            juce::String pre = "s" + juce::String (st) + "_";
            setP (s, (pre + "ratch").toRawUTF8(), (float) (1 + st % 4));
            setP (s, (pre + "pulses").toRawUTF8(), (float) (1 + st % 3));
            setP (s, (pre + "gate").toRawUTF8(), (float) (st % 4));
        }
        setP (s, "lfo1_shape", 5); setP (s, "lfo2_sync", 0); setP (s, "lfo2_rate", 15); setP (s, "seq_dir", 4);
        double ph = 0, energy = 0, maxAbs = 0; bool finite = true;
        feed (gp, head, sr, 400, ph, false, finite, energy, maxAbs);
        std::printf ("sonic: rms %.4f max %.3f\n", std::sqrt (energy / (400 * 512 * 2)), maxAbs);
        CHECK (finite, "sonic output finite with extreme settings + full mod matrix");
        CHECK (maxAbs < 4.0 && energy > 1.0, "sonic output sane level");
        // seq stage advances with the host clock
        std::set<int> stages;
        for (int b = 0; b < 200; ++b) { double e = 0, m = 0; bool f = true; feed (gp, head, sr, 1, ph, false, f, e, m); stages.insert (gp.ui.seqStage.load()); }
        std::printf ("seq stages visited: %d\n", (int) stages.size());
        CHECK (stages.size() >= 4, "metro sequencer walks through stages");
        // freeze
        setP (s, "freeze", 1); setP (s, "glitch", 0); setP (s, "drywet", 100); setP (s, "cloud", 100);
        for (int m = 1; m <= 8; ++m) setP (s, ("m" + juce::String (m) + "_src").toRawUTF8(), 0);
        setP (s, "hpf", 20); setP (s, "lpf", 20000);
        double e2 = 0;
        { juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
          for (int b = 0; b < 100; ++b) { buf.clear(); gp.processBlock (buf, midi); for (int i = 0; i < 512; ++i) e2 += buf.getSample (0, i) * buf.getSample (0, i); } }
        std::printf ("freeze energy with silent input: %.3f\n", e2);
        CHECK (e2 > 0.5, "freeze keeps the cloud sounding with silent input");
        // state round trip
        juce::MemoryBlock mbk; gp.getStateInformation (mbk);
        SonicProcessor g2; g2.setStateInformation (mbk.getData(), (int) mbk.getSize());
        CHECK (std::abs (g2.state.getRawParameterValue ("cloud")->load() - 100.0f) < 0.01f, "state save/restore");
        CHECK (std::abs (g2.state.getRawParameterValue ("m1_amt")->load() - 100.0f) < 0.01f, "mod matrix in state");
    }
    {   // imager width 0 -> mono; FREE sync runs with stopped host
        SonicProcessor gp; FakeHead head; head.playing = false;
        gp.setPlayHead (&head); gp.setPlayConfigDetails (2, 2, sr, 512); gp.prepareToPlay (sr, 512);
        setP (gp.state, "imgwidth", 0); setP (gp.state, "syncmode", 1); setP (gp.state, "freebpm", 90);
        setP (gp.state, "m1_src", 3); setP (gp.state, "m1_dst", 0); setP (gp.state, "m1_amt", 80);
        double ph = 0, e = 0, m = 0, diff = 0; bool f = true;
        feed (gp, head, sr, 100, ph, true, f, e, m, &diff);
        std::printf ("width0 L-R diff %.6f, bpm %.1f\n", diff, gp.ui.bpm.load());
        CHECK (diff < 1e-3, "imager width 0 collapses to mono");
        CHECK (std::abs (gp.ui.bpm.load() - 90.0f) < 0.01f, "FREE sync uses internal bpm");
        std::set<int> stages;
        for (int b = 0; b < 300; ++b) { double e3 = 0, m3 = 0; bool f3 = true; feed (gp, head, sr, 1, ph, false, f3, e3, m3); stages.insert (gp.ui.seqStage.load()); }
        CHECK (stages.size() >= 4, "sequencer runs in FREE mode with transport stopped");
    }
    {   // presets
        SonicProcessor gp;
        CHECK (gp.getNumPrograms() >= 8, "factory presets exposed as host programs");
        gp.presets.load (2);
        const float g = gp.state.getRawParameterValue ("glitch")->load();
        gp.presets.load (0);
        std::printf ("preset 'Glitch Roll' glitch=%.1f, after Init=%.1f\n", g, gp.state.getRawParameterValue ("glitch")->load());
        CHECK (g > 10.0f && gp.state.getRawParameterValue ("glitch")->load() < 1.0f, "factory preset load + reset");
        setP (gp.state, "cloud", 42);
        CHECK (gp.presets.save ("zz test preset"), "user preset save");
        gp.presets.load (0);
        int idx = gp.presets.getNames().indexOf ("zz test preset");
        CHECK (idx >= gp.presets.numFactory(), "user preset listed");
        gp.presets.load (idx);
        CHECK (std::abs (gp.state.getRawParameterValue ("cloud")->load() - 42.0f) < 0.01f, "user preset recall");
        gp.presets.removeCurrent();
        CHECK (gp.presets.getNames().indexOf ("zz test preset") < 0, "user preset delete");
    }
    {   // LFO 3 / 4 reach the matrix; the UI shows the live value, the parameter stays put
        SonicProcessor gp; FakeHead head;
        gp.setPlayHead (&head); gp.setPlayConfigDetails (2, 2, sr, 512); gp.prepareToPlay (sr, 512);
        auto& s = gp.state;
        CHECK (s.getParameter ("lfo4_rate") != nullptr, "LFO 3 and 4 parameters exist");
        setP (s, "cloud", 50);
        setP (s, "m1_src", 5); setP (s, "m1_dst", 0);  setP (s, "m1_amt", 40);    // LFO 3 -> Position
        setP (s, "m2_src", 6); setP (s, "m2_dst", 22); setP (s, "m2_amt", -60);   // LFO 4 -> LPF
        setP (s, "lfo3_div", 7); setP (s, "lfo4_shape", 1); setP (s, "lfo4_div", 7);
        setP (s, "lpf", 2000);
        std::unique_ptr<juce::AudioProcessorEditor> ed (gp.createEditor());
        auto* se = dynamic_cast<SonicEditor*> (ed.get());
        auto* posCtl = se->getControl ("gpos");
        auto* lpfCtl = se->getControl ("lpf");
        auto* idleCtl = se->getControl ("gsize");
        std::set<juce::String> posTexts, lpfTexts;
        float posLo = 1, posHi = 0, lfoLo = 1, lfoHi = -1;
        double ph = 0;
        for (int b = 0; b < 300; ++b)
        {
            double e = 0, m = 0; bool f = true;
            feed (gp, head, sr, 1, ph, false, f, e, m);
            se->updateModulationDisplay();
            posLo = std::min (posLo, gp.ui.modNorm[0].load()); posHi = std::max (posHi, gp.ui.modNorm[0].load());
            lfoLo = std::min (lfoLo, gp.ui.lfo[3].load()); lfoHi = std::max (lfoHi, gp.ui.lfo[3].load());
            posTexts.insert (posCtl->slider.getTextFromValue (posCtl->slider.getValue()));
            lpfTexts.insert (lpfCtl->slider.getTextFromValue (lpfCtl->slider.getValue()));
        }
        std::printf ("position norm %.3f..%.3f, %d position texts, %d lpf texts\n", posLo, posHi, (int) posTexts.size(), (int) lpfTexts.size());
        CHECK (posHi - posLo > 0.5f, "LFO 3 sweeps its destination");
        CHECK (lfoHi - lfoLo > 1.5f, "LFO 4 runs");
        CHECK (gp.ui.modMask.load() == ((1u << 0) | (1u << 22)), "only the routed destinations are flagged");
        CHECK (posTexts.size() > 10 && lpfTexts.size() > 10, "numeric values follow the modulation");
        CHECK (! posTexts.begin()->containsChar ('.'), "values without decimals are rounded");
        CHECK (posCtl->slider.isModulationShown() && ! idleCtl->slider.isModulationShown(), "only routed knobs show modulation");
        CHECK (std::abs (posCtl->slider.getValue() - 15.0) < 1e-3, "knob value stays the set value");
        CHECK (std::abs (s.getRawParameterValue ("gpos")->load() - 15.0f) < 1e-3f, "parameter is not written by the modulation");
        // audio stops: back to the real value
        for (int k = 0; k < 20; ++k) se->updateModulationDisplay();
        CHECK (! posCtl->slider.isModulationShown(), "knobs return to the set value when audio stops");
        CHECK (posCtl->slider.getTextFromValue (posCtl->slider.getValue()) == "15 %", "text returns to the set value");
        // matrix slot off: back to the real value
        { double e = 0, m = 0; bool f = true; feed (gp, head, sr, 1, ph, false, f, e, m); }
        se->updateModulationDisplay();
        CHECK (posCtl->slider.isModulationShown(), "modulation display resumes with audio");
        setP (s, "m1_src", 0);
        { double e = 0, m = 0; bool f = true; feed (gp, head, sr, 1, ph, false, f, e, m); }
        se->updateModulationDisplay();
        CHECK (! posCtl->slider.isModulationShown(), "knob released when its slot is switched off");
    }
    {   // resizable editor: keeps 16:10, scales the whole view, remembers the size in the state
        SonicProcessor gp;
        std::unique_ptr<juce::AudioProcessorEditor> ed (gp.createEditor());
        CHECK (ed->getWidth() == 1280 && ed->getHeight() == 800, "editor opens at 1280x800");
        CHECK (ed->isResizable(), "editor is resizable");
        ed->setSize (1600, 1000);
        CHECK (ed->getWidth() == 1600 && ed->getHeight() == 1000, "editor resizes at 16:10");
        CHECK ((int) gp.state.state.getProperty ("uiWidth") == 1600, "size stored with the state");
        ed.reset (gp.createEditor());
        CHECK (ed->getWidth() == 1600 && ed->getHeight() == 1000, "editor reopens at the stored size");
        auto* se = dynamic_cast<SonicEditor*> (ed.get());
        auto* knob = se->getControl ("gpos");
        CHECK (knob != nullptr && knob->getBounds().getX() >= 16 && knob->getBoundsInParent().getRight() <= 916,
               "controls laid out inside their panel");
    }
    {   // a session saved before LFO 3/4 existed
        SonicProcessor gp;
        setP (gp.state, "m1_src", 3); setP (gp.state, "m1_dst", 22); setP (gp.state, "m1_amt", 55); setP (gp.state, "m2_src", 2);
        auto tree = gp.state.copyState();
        for (auto id : { "lfo3_shape", "lfo3_sync", "lfo3_rate", "lfo3_div", "lfo4_shape", "lfo4_sync", "lfo4_rate", "lfo4_div" })
            tree.removeChild (tree.getChildWithProperty ("id", id), nullptr);
        juce::MemoryBlock mbk;
        { auto xml = tree.createXml(); juce::AudioProcessor::copyXmlToBinary (*xml, mbk); }
        SonicProcessor g2;
        setP (g2.state, "lfo3_div", 0);
        g2.setStateInformation (mbk.getData(), (int) mbk.getSize());
        CHECK (std::lround (g2.state.getRawParameterValue ("m1_src")->load()) == 3, "old session keeps SEQ as source");
        CHECK (std::lround (g2.state.getRawParameterValue ("m2_src")->load()) == 2, "old session keeps LFO 2 as source");
        CHECK (std::lround (g2.state.getRawParameterValue ("lfo3_div")->load()) == 4, "old session loads LFO 3 defaults");
    }
    std::printf ("\n%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}

// ---- editor snapshot (run with --snap): feeds a drum-like test signal, then renders the 1280x800 editor
static void runSnap (const juce::String& path, int deckTab, float scale)
{
    juce::ScopedJuceInitialiser_GUI init;
    {
        SonicProcessor gp; FakeHead head;
        gp.setPlayHead (&head); gp.setPlayConfigDetails (2, 2, 48000, 512); gp.prepareToPlay (48000, 512);
        gp.presets.load (6);
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; double ph = 0;
        for (int b = 0; b < 400; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                const double t = std::fmod (ph, 0.5), t2 = std::fmod (ph + 0.25, 1.0);
                float x = (float) (0.7 * std::exp (-t * 9.0) * std::sin (2 * juce::MathConstants<double>::pi * (55.0 + 90.0 * std::exp (-t * 30.0)) * t)
                                   + 0.35 * std::exp (-t2 * 18.0) * (juce::Random::getSystemRandom().nextFloat() - 0.5f));
                ph += 1.0 / 48000.0;
                buf.setSample (0, i, x); buf.setSample (1, i, (float) (x * 0.7 + 0.1 * std::sin (ph * 2400.0)));
            }
            gp.processBlock (buf, midi); head.ppq += 512 * 2.0 / 48000.0;
        }
        std::unique_ptr<juce::AudioProcessorEditor> ed (gp.createEditor());
        if (auto* se = dynamic_cast<SonicEditor*> (ed.get()))
        {
            se->showDeckTab (deckTab);
            se->updateModulationDisplay();
            for (int k = 0; k < 20; ++k) se->getScope().refresh();
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (path)); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    }
}
