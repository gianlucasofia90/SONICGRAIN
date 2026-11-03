#pragma once
// SONIC GRAIN modulation sources: LFOs and a Metropolix-style 8-stage sequencer.
#include <cmath>
#include <cstdint>
#include <array>
#include <climits>
#include <algorithm>

namespace sonic
{
// clock divisions shared by LFO sync, sequencer and UI (in beats)
static constexpr int kNumDivs = 12;
static constexpr double kDivBeats[kNumDivs] = { 32.0, 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                                2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
static inline const char* kDivNames[kNumDivs] = { "8 bar", "4 bar", "2 bar", "1 bar", "1/2", "1/4", "1/8", "1/16",
                                                  "1/32", "1/4T", "1/8T", "1/16T" };

struct Rng
{
    uint32_t s = 0x2545F491u;
    float next01() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float nextPM() { return next01() * 2.0f - 1.0f; }
};

class Lfo
{
public:
    enum Shape { Sine, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom, NumShapes };
    static inline const char* kShapeNames[NumShapes] = { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H",
                                                         "Smooth Rnd" };

    explicit Lfo (uint32_t seed = 1) { rng.s ^= seed * 2654435761u; target = rng.nextPM(); prev = rng.nextPM(); }

    /** value (-1..1) at the start of a chunk, then advances by `samples`. */
    float tick (int shape, bool sync, int div, float rateHz, double ppq, int samples, double sr)
    {
        double ph;
        long long cyc;
        if (sync)
        {
            const double c = ppq / kDivBeats[div < 0 ? 0 : (div >= kNumDivs ? kNumDivs - 1 : div)];
            cyc = (long long) std::floor (c);
            ph = c - std::floor (c);
        }
        else
        {
            ph = phase;
            cyc = freeCycle;
            phase += rateHz * samples / sr;
            const double w = std::floor (phase);
            freeCycle += (long long) w;
            phase -= w;
        }
        if (cyc != lastCycle) { prev = target; target = rng.nextPM(); lastCycle = cyc; }
        lastPhase = (float) ph;
        const float p = (float) ph;
        switch (shape)
        {
            case Sine: return std::sin (6.2831853f * p);
            case Triangle: return 1.0f - 4.0f * std::abs (p - 0.5f);
            case SawUp: return 2.0f * p - 1.0f;
            case SawDown: return 1.0f - 2.0f * p;
            case Square: return p < 0.5f ? 1.0f : -1.0f;
            case SampleHold: return target;
            case SmoothRandom: return prev + (target - prev) * (0.5f - 0.5f * std::cos (3.14159265f * p));
            default: return 0.0f;
        }
    }
    float getPhase() const { return lastPhase; }

private:
    Rng rng;
    double phase = 0;
    long long freeCycle = 0, lastCycle = LLONG_MIN;
    float prev = 0, target = 0, lastPhase = 0;
};

/** Metropolix-style sequencer: 8 stages, pulse count per stage, gate modes, ratchets, slide, probability,
    play directions. Outputs a CV value (-1..1, with slide) and a gate (0/1). */
class MetroSeq
{
public:
    enum Gate { Multi, Hold, Single, Rest, NumGates };
    enum Dir { Forward, Reverse, Pendulum, Random, Brownian, NumDirs };
    static inline const char* kGateNames[NumGates] = { "MULTI", "HOLD", "SINGLE", "REST" };
    static inline const char* kDirNames[NumDirs] = { "Forward", "Reverse", "Pendulum", "Random", "Brownian" };

    struct Stage { float value = 0; int pulses = 1, ratchet = 1, gate = Multi, prob = 100; bool slide = false; };
    struct Params { std::array<Stage, 8> st; int length = 8, dir = Forward, div = 7; float slideMs = 80; };

    void reset() { stage = 0; pulse = 0; lastTick = LLONG_MIN; pend = 1; active = true; }

    void update (double ppq, int samples, double sr, const Params& P)
    {
        const int len = P.length < 1 ? 1 : (P.length > 8 ? 8 : P.length);
        if (stage >= len) stage = 0;
        const double c = ppq / kDivBeats[P.div < 0 ? 0 : (P.div >= kNumDivs ? kNumDivs - 1 : P.div)];
        const long long t = (long long) std::floor (c);
        const double frac = c - std::floor (c);
        if (t != lastTick)
        {
            if (lastTick == LLONG_MIN || t < lastTick || t > lastTick + 64)
            {
                stage = 0; pulse = 0; enterStage (P);
            }
            else if (++pulse >= std::max (1, P.st[(size_t) stage].pulses))
            {
                pulse = 0;
                advance (len, P.dir);
                enterStage (P);
            }
            lastTick = t;
        }
        const auto& S = P.st[(size_t) stage];
        // gate
        const int r = std::max (1, S.ratchet);
        const double sub = frac * r;
        const bool ratchOn = (sub - std::floor (sub)) < 0.5;
        bool g = false;
        switch (S.gate)
        {
            case Multi: g = ratchOn; break;
            case Hold: g = true; break;
            case Single: g = pulse == 0 && ratchOn; break;
            default: g = false; break;
        }
        gate = (active && g) ? 1.0f : 0.0f;
        // value with slide
        if (slideNow && P.slideMs > 1.0f)
        {
            const float coef = 1.0f - std::exp (-(float) samples / (P.slideMs * 0.001f * (float) sr));
            value += (target - value) * coef;
        }
        else value = target;
    }

    float getValue() const { return value; }
    float getGate() const { return gate; }
    int getStage() const { return stage; }

private:
    void enterStage (const Params& P)
    {
        const auto& S = P.st[(size_t) stage];
        active = rng.next01() * 100.0f < (float) S.prob;
        target = S.value;
        slideNow = S.slide;
    }
    void advance (int len, int dir)
    {
        switch (dir)
        {
            case Reverse: stage = (stage - 1 + len) % len; break;
            case Pendulum:
                if (len == 1) { stage = 0; break; }
                if (stage + pend >= len || stage + pend < 0) pend = -pend;
                stage += pend;
                break;
            case Random: stage = (int) (rng.next01() * len) % len; break;
            case Brownian: { const float x = rng.next01(); stage = (stage + (x < 0.33f ? -1 : (x < 0.66f ? 0 : 1)) + len) % len; break; }
            default: stage = (stage + 1) % len; break;
        }
    }

    Rng rng;
    int stage = 0, pulse = 0, pend = 1;
    long long lastTick = LLONG_MIN;
    bool active = true, slideNow = false;
    float value = 0, target = 0, gate = 0;
};
} // namespace sonic
