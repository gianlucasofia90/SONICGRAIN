#include "SonicProcessor.h"
#include "SonicEditor.h"
#include "IDMLookAndFeel.h"

using namespace idm;
using namespace sonic;

juce::AudioProcessorValueTreeState::ParameterLayout SonicProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    // grains
    l.add (fparam ("gpos", "Position", 0, 100, 15, "%"));
    l.add (fparam ("gsize", "Size", 5, 2000, 120, "ms", 200));
    l.add (fparam ("gdens", "Density", 0.5f, 200, 15, "/s", 20));
    l.add (fparam ("gpitch", "Pitch", -24, 24, 0, "st"));
    l.add (fparam ("spray", "Spray", 0, 100, 20, "%"));
    l.add (fparam ("prand", "Pitch Rnd", 0, 24, 0, "st"));
    l.add (fparam ("grev", "Reverse", 0, 100, 0, "%"));
    l.add (fparam ("gshape", "Shape", 0, 100, 30, "%"));
    l.add (fparam ("gwidth", "Spread", 0, 100, 60, "%"));
    l.add (fparam ("feedback", "Feedback", 0, 95, 0, "%"));
    l.add (fparam ("cloud", "Cloud", 0, 100, 35, "%"));
    l.add (bparam ("freeze", "Freeze", false));
    // glitch
    l.add (fparam ("glitch", "Glitch", 0, 100, 0, "%"));
    l.add (cparam ("slice", "Slice", { "1/4", "1/8", "1/16", "1/32", "1/64" }, 2));
    l.add (fparam ("rptpitch", "Rpt Pitch", -12, 12, 0, "st"));
    l.add (iparam ("rhold", "Hold", 1, 16, 4));
    // colour
    l.add (fparam ("crbits", "Bits", 1, 24, 24));
    l.add (fparam ("dsamp", "Downsmp", 1, 64, 1, "x", 8));
    l.add (fparam ("tone", "Tone", 100, 20000, 20000, "Hz", 2000));
    l.add (fparam ("wow", "Wow", 0, 100, 0, "%"));
    l.add (fparam ("flutter", "Flutter", 0, 100, 0, "%"));
    l.add (fparam ("drive", "Drive", 0, 100, 0, "%"));
    l.add (fparam ("drywet", "Dry/Wet", 0, 100, 100, "%"));
    l.add (fparam ("outdb", "Output", -36, 12, 0, "dB"));
    // filters
    l.add (fparam ("hpf", "HPF", 20, 2000, 20, "Hz", 200));
    l.add (fparam ("hpq", "HPF Q", 0.3f, 10, 0.707f, {}, 1.5f));
    l.add (fparam ("lpf", "LPF", 200, 20000, 20000, "Hz", 2000));
    l.add (fparam ("lreso", "LPF Reso", 0, 1, 0));
    // imager
    l.add (fparam ("imgwidth", "Width", 0, 200, 100, "%"));
    l.add (fparam ("monofreq", "Bass Mono", 0, 400, 0, "Hz"));
    // sync
    l.add (cparam ("syncmode", "Sync", { "DAW", "FREE" }, 0));
    l.add (fparam ("freebpm", "Free BPM", 40, 240, 120, "bpm"));
    // LFOs
    juce::StringArray shapes, divs;
    for (auto* s : Lfo::kShapeNames) shapes.add (s);
    for (auto* d : kDivNames) divs.add (d);
    auto addLfo = [&] (int i, int defaultDiv)
    {
        juce::String pre = "lfo" + juce::String (i) + "_";
        l.add (cparam (pre + "shape", "LFO" + juce::String (i) + " Shape", shapes, 0));
        l.add (bparam (pre + "sync", "LFO" + juce::String (i) + " Sync", true));
        l.add (fparam (pre + "rate", "LFO" + juce::String (i) + " Rate", 0.02f, 20, 1, "Hz", 1));
        l.add (cparam (pre + "div", "LFO" + juce::String (i) + " Div", divs, defaultDiv));
    };
    addLfo (1, 3);
    addLfo (2, 5);
    // sequencer
    juce::StringArray gates, dirs;
    for (auto* g : MetroSeq::kGateNames) gates.add (g);
    for (auto* d : MetroSeq::kDirNames) dirs.add (d);
    l.add (iparam ("seq_len", "Seq Length", 1, 8, 8));
    l.add (cparam ("seq_dir", "Seq Direction", dirs, 0));
    l.add (cparam ("seq_div", "Seq Clock", divs, 7));
    l.add (fparam ("seq_slide", "Seq Slide", 1, 1000, 80, "ms", 150));
    for (int s = 1; s <= 8; ++s)
    {
        juce::String pre = "s" + juce::String (s) + "_", nm = "Stage " + juce::String (s) + " ";
        static const float kValDef[8] = { 0.0f, 0.5f, -0.3f, 0.8f, -0.6f, 0.25f, -0.1f, 0.6f };
        l.add (fparam (pre + "val", nm + "Value", -1, 1, kValDef[s - 1]));
        l.add (iparam (pre + "pulses", nm + "Pulses", 1, 8, 1));
        l.add (iparam (pre + "ratch", nm + "Ratchet", 1, 4, 1));
        l.add (cparam (pre + "gate", nm + "Gate", gates, 0));
        l.add (bparam (pre + "slide", nm + "Slide", false));
        l.add (iparam (pre + "prob", nm + "Prob", 0, 100, 100, "%"));
    }
    // modulation matrix
    juce::StringArray srcs, dsts;
    for (auto* s : kSrcNames) srcs.add (s);
    for (auto* d : kDestNames) dsts.add (d);
    for (int m = 1; m <= kNumSlots; ++m)
    {
        juce::String pre = "m" + juce::String (m) + "_";
        l.add (cparam (pre + "src", "Mod " + juce::String (m) + " Source", srcs, 0));
        l.add (cparam (pre + "dst", "Mod " + juce::String (m) + " Dest", dsts, 0));
        l.add (fparam (pre + "amt", "Mod " + juce::String (m) + " Amount", -100, 100, 0, "%"));
    }
    // LFO 3 and 4 live here because new parameters go at the end
    addLfo (3, 4);
    addLfo (4, 6);
    return l;
}

SonicProcessor::SonicProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "SONICGRAIN", createLayout())
{
    for (int d = 0; d < kNumDests; ++d) destParam[(size_t) d] = state.getParameter (kDestIds[d]);
    for (int i = 0; i < kNumLfos; ++i)
    {
        const juce::String pre = "lfo" + juce::String (i + 1) + "_";
        pl.shape[i] = raw (pre + "shape"); pl.sync[i] = raw (pre + "sync"); pl.rate[i] = raw (pre + "rate"); pl.div[i] = raw (pre + "div");
    }
    for (int s = 0; s < 8; ++s)
    {
        const juce::String pre = "s" + juce::String (s + 1) + "_";
        pl.val[s] = raw (pre + "val"); pl.pulses[s] = raw (pre + "pulses"); pl.ratch[s] = raw (pre + "ratch");
        pl.gate[s] = raw (pre + "gate"); pl.slide[s] = raw (pre + "slide"); pl.prob[s] = raw (pre + "prob");
    }
    for (int m = 0; m < kNumSlots; ++m)
    {
        const juce::String pre = "m" + juce::String (m + 1) + "_";
        pl.src[m] = raw (pre + "src"); pl.dst[m] = raw (pre + "dst"); pl.amt[m] = raw (pre + "amt");
    }
}

bool SonicProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void SonicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate);
    seq.reset();
    scratch.setSize (2, juce::jmax (1, samplesPerBlock));
}

void SonicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    const int nch = buffer.getNumChannels();
    if (n == 0 || nch == 0) return;

    auto F = [this] (const char* id) { return raw (id)->load(); };
    auto I = [this] (const char* id) { return (int) std::lround (raw (id)->load()); };

    // ---- clock: host (DAW) or internal (FREE)
    double hostPpq = -1, bpm = 120;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) hostPpq = *q;
            hostPlaying = pos->getIsPlaying();
        }
    const bool freeMode = I ("syncmode") == 1;
    if (freeMode) bpm = F ("freebpm");
    bpm = juce::jlimit (20.0, 400.0, bpm);
    const double bps = bpm / (60.0 * sr);
    double ppq0 = internalPpq;
    if (! freeMode && hostPlaying && hostPpq >= 0) ppq0 = hostPpq;
    internalPpq = ppq0 + n * bps;
    ui.bpm.store ((float) bpm);

    // ---- static params
    GrainParams base;
    base.gpos = F ("gpos"); base.gsize = F ("gsize"); base.gdens = F ("gdens"); base.gpitch = F ("gpitch");
    base.spray = F ("spray"); base.prand = F ("prand"); base.grev = F ("grev"); base.gshape = F ("gshape");
    base.gwidth = F ("gwidth"); base.feedback = F ("feedback"); base.cloud = F ("cloud");
    base.freeze = F ("freeze") > 0.5f; base.glitch = F ("glitch");
    base.rslice = (float) std::pow (2.0, -(double) I ("slice"));
    base.rptpitch = F ("rptpitch"); base.rhold = I ("rhold"); base.crbits = F ("crbits");
    base.dsamp = F ("dsamp"); base.tone = F ("tone"); base.wow = F ("wow"); base.flutter = F ("flutter");
    base.drive = F ("drive"); base.drywet = F ("drywet"); base.outdb = F ("outdb");
    base.hpf = F ("hpf"); base.hpq = F ("hpq"); base.lpf = F ("lpf"); base.lreso = F ("lreso");
    const float baseWidth = F ("imgwidth"), monoFreq = F ("monofreq");

    int lShape[kNumLfos], lDiv[kNumLfos]; bool lSync[kNumLfos]; float lRate[kNumLfos];
    auto R0 = [] (std::atomic<float>* a) { return (int) std::lround (a->load()); };
    for (int i = 0; i < kNumLfos; ++i)
    {
        lShape[i] = R0 (pl.shape[i]); lSync[i] = pl.sync[i]->load() > 0.5f;
        lRate[i] = pl.rate[i]->load(); lDiv[i] = R0 (pl.div[i]);
    }
    MetroSeq::Params sp;
    sp.length = I ("seq_len"); sp.dir = I ("seq_dir"); sp.div = I ("seq_div"); sp.slideMs = F ("seq_slide");
    for (int s = 0; s < 8; ++s)
    {
        auto& st = sp.st[(size_t) s];
        st.value = pl.val[s]->load(); st.pulses = R0 (pl.pulses[s]); st.ratchet = R0 (pl.ratch[s]);
        st.gate = R0 (pl.gate[s]); st.slide = pl.slide[s]->load() > 0.5f; st.prob = R0 (pl.prob[s]);
    }
    int mSrc[kNumSlots], mDst[kNumSlots]; float mAmt[kNumSlots];
    bool anyMod = false;
    juce::uint32 modMask = 0;
    for (int m = 0; m < kNumSlots; ++m)
    {
        mSrc[m] = R0 (pl.src[m]); mDst[m] = juce::jlimit (0, kNumDests - 1, R0 (pl.dst[m]));
        mAmt[m] = pl.amt[m]->load() * 0.01f;
        if (mSrc[m] != 0 && mAmt[m] != 0.0f) { anyMod = true; modMask |= (juce::uint32) 1 << mDst[m]; }
    }

    if (scratch.getNumSamples() < n) scratch.setSize (2, n, false, false, true);
    float* L = buffer.getWritePointer (0);
    float* R = nch > 1 ? buffer.getWritePointer (1) : scratch.getWritePointer (1);
    if (nch == 1) juce::FloatVectorOperations::copy (R, L, n);

    const float monoCoef = monoFreq > 1.0f ? 1.0f - std::exp (-6.2831853f * monoFreq / (float) sr) : 0.0f;
    constexpr int kChunk = 32;
    GrainParams gp = base;
    float width = baseWidth;
    for (int off = 0; off < n; off += kChunk)
    {
        const int len = juce::jmin (kChunk, n - off);
        const double ppq = ppq0 + off * bps;
        float srcVal[kNumSrcs] = {};
        for (int i = 0; i < kNumLfos; ++i) srcVal[kLfoSrc[i]] = lfo[i].tick (lShape[i], lSync[i], lDiv[i], lRate[i], ppq, len, sr);
        seq.update (ppq, len, sr, sp);
        srcVal[3] = seq.getValue();
        srcVal[4] = seq.getGate();

        gp = base;
        width = baseWidth;
        if (anyMod)
        {
            float off01[kNumDests] = {};
            for (int m = 0; m < kNumSlots; ++m)
                if (mSrc[m] > 0) off01[mDst[m]] += mAmt[m] * srcVal[mSrc[m]];
            float* fields[kNumDests] = { &gp.gpos, &gp.gsize, &gp.gdens, &gp.gpitch, &gp.spray, &gp.prand, &gp.grev,
                                         &gp.gshape, &gp.gwidth, &gp.feedback, &gp.cloud, &gp.glitch, &gp.rptpitch,
                                         &gp.crbits, &gp.dsamp, &gp.tone, &gp.wow, &gp.flutter, &gp.drive, &gp.drywet,
                                         &gp.hpf, &gp.hpq, &gp.lpf, &gp.lreso, &width };
            for (int d = 0; d < kNumDests; ++d)
                if (off01[d] != 0.0f)
                {
                    auto* p = destParam[(size_t) d];
                    *fields[d] = p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, p->convertTo0to1 (*fields[d]) + off01[d]));
                }
            if (off == 0)
                for (int d = 0; d < kNumDests; ++d)
                    if ((modMask >> d) & 1u)
                        ui.modNorm[(size_t) d].store (destParam[(size_t) d]->convertTo0to1 (*fields[d]), std::memory_order_relaxed);
        }

        engine.process (L + off, R + off, len, gp, juce::jmax (0.0, ppq), bpm, true);

        // ---- imager: M/S width + bass mono
        const float w = width * 0.01f;
        for (int i = off; i < off + len; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            float s = 0.5f * (L[i] - R[i]);
            if (monoCoef > 0.0f) { monoLp += (s - monoLp) * monoCoef; s -= monoLp; }
            s *= w;
            L[i] = m + s; R[i] = m - s;
        }
        if (off == 0)
        {
            for (int i = 0; i < kNumLfos; ++i) { ui.lfo[i].store (srcVal[kLfoSrc[i]]); ui.lfoPhase[i].store (lfo[i].getPhase()); }
            ui.seqVal.store (srcVal[3]); ui.seqGate.store (srcVal[4]); ui.seqStage.store (seq.getStage());
            ui.pos.store (gp.gpos); ui.size.store (gp.gsize); ui.spray.store (gp.spray); ui.pitch.store (gp.gpitch);
            ui.modMask.store (modMask);
            ui.blockCount.fetch_add (1, std::memory_order_relaxed);
        }
    }

    // ---- scope + correlation
    int w = scopeW.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        scopeL[(size_t) w] = L[i]; scopeR[(size_t) w] = R[i];
        w = (w + 1) & (kScopeN - 1);
        corrNum = corrNum * 0.9995f + L[i] * R[i];
        corrDen = corrDen * 0.9995f + 0.5f * (L[i] * L[i] + R[i] * R[i]);
    }
    scopeW.store (w, std::memory_order_relaxed);
    ui.corr.store (corrDen > 1e-7f ? juce::jlimit (-1.0f, 1.0f, corrNum / corrDen) : 1.0f);

    if (nch == 1) { juce::FloatVectorOperations::add (L, R, n); juce::FloatVectorOperations::multiply (L, 0.5f, n); }
    for (int c = 2; c < nch; ++c) buffer.clear (c, 0, n);
}

void SonicProcessor::copyScope (float* l, float* r, int count) const
{
    int w = scopeW.load (std::memory_order_relaxed);
    int start = (w - count + kScopeN) & (kScopeN - 1);
    for (int i = 0; i < count; ++i)
    {
        const int k = (start + i) & (kScopeN - 1);
        l[i] = scopeL[(size_t) k]; r[i] = scopeR[(size_t) k];
    }
}

void SonicProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = state.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void SonicProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (state.state.getType())) state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SonicProcessor::createEditor() { return new SonicEditor (*this); }

#ifndef IDM_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SonicProcessor(); }
#endif
