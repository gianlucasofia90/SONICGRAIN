#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace sonic
{
struct FactoryPreset { const char* name; std::vector<std::pair<const char*, float>> values; };

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> p = {
        { "Init", {} },
        { "Cloud Bank", { { "cloud", 85 }, { "gsize", 320 }, { "gdens", 28 }, { "spray", 45 }, { "gwidth", 90 },
                          { "gshape", 15 }, { "feedback", 25 }, { "lpf", 9000 }, { "imgwidth", 150 } } },
        { "Glitch Roll", { { "cloud", 10 }, { "glitch", 70 }, { "slice", 3 }, { "rptpitch", -3 }, { "rhold", 6 },
                           { "crbits", 12 }, { "dsamp", 3 }, { "drive", 30 } } },
        { "Tape Ghost", { { "cloud", 45 }, { "wow", 55 }, { "flutter", 35 }, { "tone", 6500 }, { "drive", 20 },
                          { "hpf", 120 }, { "gsize", 180 }, { "grev", 25 } } },
        { "Frozen Choir", { { "cloud", 100 }, { "freeze", 1 }, { "gsize", 600 }, { "gdens", 40 }, { "prand", 12 },
                            { "spray", 70 }, { "gwidth", 100 }, { "imgwidth", 170 }, { "lpf", 7000 }, { "lreso", 0.2f } } },
        { "Metro Chop", { { "cloud", 70 }, { "gsize", 60 }, { "gdens", 60 }, { "m1_src", 3 }, { "m1_dst", 0 },
                          { "m1_amt", 60 }, { "m2_src", 4 }, { "m2_dst", 10 }, { "m2_amt", 80 }, { "seq_div", 7 },
                          { "s1_val", -0.8f }, { "s2_val", 0.3f }, { "s3_val", 0.9f }, { "s4_val", -0.2f },
                          { "s5_val", 0.6f }, { "s6_val", -0.5f }, { "s7_val", 0.1f }, { "s8_val", 0.8f },
                          { "s3_ratch", 3 }, { "s6_gate", 1 }, { "s7_slide", 1 } } },
        { "LFO Sweep", { { "cloud", 40 }, { "m1_src", 1 }, { "m1_dst", 22 }, { "m1_amt", 55 }, { "lfo1_div", 3 },
                         { "lfo1_shape", 0 }, { "lpf", 3000 }, { "lreso", 0.45f }, { "m2_src", 2 }, { "m2_dst", 3 },
                         { "m2_amt", 15 }, { "lfo2_shape", 6 }, { "lfo2_div", 4 } } },
        { "Wide Dust", { { "cloud", 55 }, { "gsize", 40 }, { "gdens", 90 }, { "spray", 80 }, { "prand", 24 },
                         { "crbits", 10 }, { "imgwidth", 200 }, { "monofreq", 180 }, { "hpf", 200 } } },
    };
    return p;
}

/** Factory + user presets (XML files in ~/Music/Sonic Grain Presets). */
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& s) : state (s) { refresh(); }

    static juce::File userDir()
    {
        auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Sonic Grain Presets");
        d.createDirectory();
        return d;
    }

    void refresh()
    {
        names.clear();
        for (auto& f : factoryPresets()) names.add (juce::String ("Factory / ") + f.name);
        userFiles = userDir().findChildFiles (juce::File::findFiles, false, "*.sgpreset");
        userFiles.sort();
        for (auto& f : userFiles) names.add (f.getFileNameWithoutExtension());
    }

    const juce::StringArray& getNames() const { return names; }
    int getCurrent() const { return current; }
    int numFactory() const { return (int) factoryPresets().size(); }

    void load (int index)
    {
        if (index < 0 || index >= names.size()) return;
        current = index;
        if (index < numFactory())
        {
            for (auto* p : state.processor.getParameters())
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p)) r->setValueNotifyingHost (r->getDefaultValue());
            for (auto& kv : factoryPresets()[(size_t) index].values)
                if (auto* r = state.getParameter (kv.first)) r->setValueNotifyingHost (r->convertTo0to1 (kv.second));
        }
        else if (auto xml = juce::XmlDocument::parse (userFiles[index - numFactory()]))
        {
            if (xml->hasTagName (state.state.getType())) state.replaceState (juce::ValueTree::fromXml (*xml));
        }
    }

    bool save (const juce::String& name)
    {
        auto clean = juce::File::createLegalFileName (name.trim());
        if (clean.isEmpty()) return false;
        auto f = userDir().getChildFile (clean + ".sgpreset");
        auto xml = state.copyState().createXml();
        const bool ok = xml != nullptr && xml->writeTo (f);
        refresh();
        current = names.indexOf (clean);
        return ok;
    }

    bool removeCurrent()
    {
        if (current < numFactory()) return false;
        const bool ok = userFiles[current - numFactory()].deleteFile();
        refresh();
        current = 0;
        return ok;
    }

    void next() { load ((current + 1) % juce::jmax (1, names.size())); }
    void prev() { load ((current - 1 + names.size()) % juce::jmax (1, names.size())); }

private:
    juce::AudioProcessorValueTreeState& state;
    juce::StringArray names;
    juce::Array<juce::File> userFiles;
    int current = 0;
};
} // namespace sonic
