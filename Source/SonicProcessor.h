#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SonicEngine.h"
#include "ModSources.h"
#include "Presets.h"

namespace sonic
{
static constexpr int kNumSlots = 8;
static constexpr int kNumDests = 25;
static inline const char* kDestIds[kNumDests] = { "gpos", "gsize", "gdens", "gpitch", "spray", "prand", "grev",
    "gshape", "gwidth", "feedback", "cloud", "glitch", "rptpitch", "crbits", "dsamp", "tone", "wow", "flutter",
    "drive", "drywet", "hpf", "hpq", "lpf", "lreso", "imgwidth" };
static inline const char* kDestNames[kNumDests] = { "Position", "Size", "Density", "Pitch", "Spray", "Pitch Rnd",
    "Reverse", "Shape", "Spread", "Feedback", "Cloud", "Glitch", "Rpt Pitch", "Bits", "Downsmp", "Tone", "Wow",
    "Flutter", "Drive", "Dry/Wet", "HPF", "HPF Q", "LPF", "LPF Reso", "Width" };
static inline const char* kSrcNames[5] = { "Off", "LFO 1", "LFO 2", "SEQ", "SEQ Gate" };
}

class SonicProcessor : public juce::AudioProcessor
{
public:
    SonicProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SONIC GRAIN"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return (int) sonic::factoryPresets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, presets.getCurrent()); }
    void setCurrentProgram (int i) override { if (i < getNumPrograms()) presets.load (i); }
    const juce::String getProgramName (int i) override
    {
        return i >= 0 && i < getNumPrograms() ? juce::String (sonic::factoryPresets()[(size_t) i].name) : juce::String();
    }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState state;
    sonic::PresetManager presets { state };
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- UI data
    const sonic::SonicEngine& getEngine() const { return engine; }
    struct Ui
    {
        std::atomic<float> lfo[2] { 0.0f, 0.0f }, lfoPhase[2] { 0.0f, 0.0f }, seqVal { 0 }, seqGate { 0 };
        std::atomic<int> seqStage { 0 };
        std::atomic<float> pos { 15 }, size { 120 }, spray { 20 }, pitch { 0 }, bpm { 120 }, corr { 1 };
    } ui;
    static constexpr int kScopeN = 4096;
    void copyScope (float* l, float* r, int n) const;

private:

    sonic::SonicEngine engine;
    sonic::Lfo lfo[2] { sonic::Lfo (11), sonic::Lfo (23) };
    sonic::MetroSeq seq;
    juce::AudioBuffer<float> scratch;
    double sr = 48000.0, internalPpq = 0.0;
    float monoLp = 0;

    std::array<float, kScopeN> scopeL {}, scopeR {};
    std::atomic<int> scopeW { 0 };
    float corrNum = 0, corrDen = 1e-9f;

    // cached parameter pointers
    std::array<juce::RangedAudioParameter*, sonic::kNumDests> destParam {};
    struct Ptrs
    {
        std::atomic<float>* shape[2]; std::atomic<float>* sync[2]; std::atomic<float>* rate[2]; std::atomic<float>* div[2];
        std::atomic<float>* val[8]; std::atomic<float>* pulses[8]; std::atomic<float>* ratch[8];
        std::atomic<float>* gate[8]; std::atomic<float>* slide[8]; std::atomic<float>* prob[8];
        std::atomic<float>* src[sonic::kNumSlots]; std::atomic<float>* dst[sonic::kNumSlots]; std::atomic<float>* amt[sonic::kNumSlots];
    } pl {};
    std::atomic<float>* raw (const juce::String& id) { return state.getRawParameterValue (id); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonicProcessor)
};
