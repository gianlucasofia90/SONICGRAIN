#pragma once
// SONIC GRAIN DSP (evolved from IDM GRAIN).
// Live granular cloud, beat-synced repeat/roll, bitcrush + downsample, tone, tape wow/flutter, saturation.
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <array>

namespace sonic
{
struct GrainParams
{
    float gpos = 15, gsize = 120, gdens = 15, gpitch = 0, spray = 20, prand = 0, grev = 0, gshape = 30,
          gwidth = 60, feedback = 0;
    bool freeze = false;
    float cloud = 35, glitch = 0, rslice = 0.25f, rptpitch = 0;
    int rhold = 4;
    float crbits = 24, dsamp = 1, tone = 20000, wow = 0, flutter = 0, drive = 0, drywet = 100, outdb = 0;
    float hpf = 20, hpq = 0.707f, lpf = 20000, lreso = 0;
};

class SonicEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        int need = (int) (6.0 * sr);
        NB = 1;
        while (NB < need) NB <<= 1;
        mask = NB - 1;
        bufL.assign ((size_t) NB, 0.0f);
        bufR.assign ((size_t) NB, 0.0f);
        tapeL.assign (TN, 0.0f);
        tapeR.assign (TN, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        std::fill (tapeL.begin(), tapeL.end(), 0.0f);
        std::fill (tapeR.begin(), tapeR.end(), 0.0f);
        for (auto& gr : grains) gr.active = false;
        wpos = 0; gph = 0; stut = false; rp = 0; rrate = 1; sg = 0; remain = 0;
        fbl = fbr = 0; dsc = 0; hl = hr = 0; lpl = lpr = 0; tw = 0; wph = fph = 0; drift = 0;
        smix = 1; sgain = 1; scloud = 0.35f; prevSub = INT64_MIN; spb = sr * 0.5;
        hp = Svf(); lp = Svf();
    }

    /** ppq0 < 0 means no transport info. */
    void process (float* L, float* R, int n, const GrainParams& p, double ppq0, double bpm, bool playing)
    {
        const bool running = playing && ppq0 >= 0.0 && bpm > 1.0;
        const double beatsPerSample = running ? bpm / (60.0 * sr) : 0.0;
        if (running) spb = sr * 60.0 / bpm;
        const double slice = std::max (0.03125, (double) p.rslice);
        const float outGain = std::pow (10.0f, p.outdb / 20.0f);
        const double gdur = std::max (16.0, p.gsize * 0.001 * sr);
        const float edge = 0.5f * (1.0f - p.gshape * 0.009f);
        const float ov = p.gdens * p.gsize * 0.001f;
        const float gn = 1.4f / std::sqrt (std::max (1.0f, ov));
        const float coef = 1.0f - std::exp (-6.2831853f * p.tone / (float) sr);
        const float tmx = clampf ((20000.0f - p.tone) / 2000.0f, 0.0f, 1.0f);
        const float tmix = clampf ((p.wow + p.flutter) * 0.2f, 0.0f, 1.0f);
        const float dv = p.drive * 0.01f;
        const float k = 1.0f + dv * 9.0f, tk = std::tanh (k), sm = std::min (1.0f, dv * 4.0f);
        const float q = std::pow (2.0f, p.crbits - 1.0f);
        const bool useHP = p.hpf > 20.5f, useLP = p.lpf < 19950.0f;
        if (useHP) hp.set (p.hpf, std::max (0.3f, p.hpq), sr);
        if (useLP) lp.set (p.lpf, 0.707f + p.lreso * 11.0f, sr);

        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i], inR = R[i];

            // ---- beat-synced slice boundaries
            bool boundary = false;
            if (running)
            {
                const double ppq = ppq0 + i * beatsPerSample;
                const int64_t cs = (int64_t) std::floor (ppq / slice);
                if (prevSub != INT64_MIN && cs != prevSub) boundary = true;
                prevSub = cs;
            }
            else prevSub = INT64_MIN;

            // ---- record
            const int wi = wpos;
            if (! p.freeze)
            {
                float wl = inL + fbl * p.feedback * 0.01f;
                float wr = inR + fbr * p.feedback * 0.01f;
                if (p.feedback > 0.0f) { wl = std::tanh (wl); wr = std::tanh (wr); }
                bufL[(size_t) wi] = wl;
                bufR[(size_t) wi] = wr;
                wpos = (wi + 1) & mask;
            }

            // ---- beat repeat / roll
            if (boundary)
            {
                if (stut)
                {
                    if (--remain <= 0) stut = false;
                }
                else
                {
                    const float pr = p.glitch * 0.01f * (float) std::min (1.0, slice * 2.0) * 0.5f;
                    if (rnd01() < pr)
                    {
                        stut = true; rstart = wi; rp = 0; rrate = 1;
                        rlen = std::max (64.0, slice * spb);
                        remain = p.rhold + 1;
                    }
                }
            }
            sg += ((stut ? 1.0f : 0.0f) - sg) * 0.004f;
            float stL = 0, stR = 0;
            if (stut || sg > 0.001f)
            {
                const double ridx = rstart + rp;
                stL = read (bufL, ridx);
                stR = read (bufR, ridx);
                const float fe = clampf ((float) (std::min (rp, rlen - rp) / (0.003 * sr)), 0.0f, 1.0f);
                stL *= fe; stR *= fe;
                rp += rrate;
                if (rp >= rlen)
                {
                    rp -= rlen;
                    rrate = std::clamp (rrate * std::pow (2.0, p.rptpitch / 12.0), 0.25, 4.0);
                }
            }
            const float dl = inL + (stL - inL) * sg;
            const float dr = inR + (stR - inR) * sg;

            // ---- grain scheduler
            gph += p.gdens / sr;
            if (gph >= 1.0)
            {
                gph = gph - 1.0 - rnd01() * p.spray * 0.004;
                spawn (p, gdur, wi);
            }

            // ---- grain render
            float gl = 0, grr = 0;
            for (auto& gr : grains)
            {
                if (! gr.active) continue;
                const float x = (float) gr.age / (float) gr.dur;
                float env = 1.0f;
                if (x < edge) env = 0.5f - 0.5f * std::cos (3.14159265f * x / edge);
                else if (x > 1.0f - edge) env = 0.5f - 0.5f * std::cos (3.14159265f * (1.0f - x) / edge);
                gl += read (bufL, gr.pos) * env * gr.gL;
                grr += read (bufR, gr.pos) * env * gr.gR;
                gr.pos += gr.inc;
                if (++gr.age >= gr.dur) gr.active = false;
            }
            const float cl = gl * gn, cr = grr * gn;
            fbl = cl; fbr = cr;

            // ---- blend cloud
            scloud += (p.cloud * 0.01f - scloud) * 0.001f;
            float yl = dl + (cl - dl) * scloud;
            float yr = dr + (cr - dr) * scloud;

            // ---- filters (wet path)
            if (useHP) { yl = hp.hpL (yl); yr = hp.hpR (yr); }
            if (useLP) { yl = lp.lpL (yl); yr = lp.lpR (yr); }

            // ---- crush
            if (p.crbits < 23.5f)
            {
                yl = std::floor (yl * q + 0.5f) / q;
                yr = std::floor (yr * q + 0.5f) / q;
            }
            dsc += 1.0f;
            if (dsc >= p.dsamp) { dsc -= p.dsamp; hl = yl; hr = yr; }
            if (p.dsamp > 1.01f) { yl = hl; yr = hr; }

            // ---- tone
            lpl += (yl - lpl) * coef;
            lpr += (yr - lpr) * coef;
            yl += (lpl - yl) * tmx;
            yr += (lpr - yr) * tmx;

            // ---- tape wow / flutter
            wph += 0.55 / sr; if (wph >= 1.0) wph -= 1.0;
            fph += 6.8 / sr; if (fph >= 1.0) fph -= 1.0;
            drift = clampf (drift * 0.99999f + rndPM() * 0.0003f, -0.3f, 0.3f);
            const double wd = p.wow * 0.01 * 0.004 * sr * std::max (0.0, 1.0 + 0.7 * std::sin (6.283185307 * wph) + drift);
            const double fd = p.flutter * 0.01 * 0.0004 * sr * (1.0 + 0.9 * std::sin (6.283185307 * fph));
            const double dd = wd + fd + 2.0;
            tapeL[(size_t) tw] = yl;
            tapeR[(size_t) tw] = yr;
            const float rl = readTape (tapeL, tw - dd), rr = readTape (tapeR, tw - dd);
            tw = (tw + 1) & (TN - 1);
            yl += (rl - yl) * tmix;
            yr += (rr - yr) * tmix;

            // ---- saturation
            if (dv > 0.0001f)
            {
                yl += ((std::tanh (k * yl) / tk) / (1.0f + dv * 1.5f) - yl) * sm;
                yr += ((std::tanh (k * yr) / tk) / (1.0f + dv * 1.5f) - yr) * sm;
            }

            // ---- out
            smix += (p.drywet * 0.01f - smix) * 0.001f;
            sgain += (outGain - sgain) * 0.001f;
            L[i] = (inL + (yl - inL) * smix) * sgain;
            R[i] = (inR + (yr - inR) * smix) * sgain;
        }
        wposAtomic.store (wpos, std::memory_order_relaxed);
        for (int g = 0; g < NG; ++g)
        {
            const auto& gr = grains[g];
            float ago = -1.0f;
            if (gr.active) { ago = (float) (wpos - gr.pos); while (ago < 0) ago += (float) NB; }
            grainAgo[(size_t) g].store (ago, std::memory_order_relaxed);
        }
    }

    // ---- read-only views for the editor (benign races)
    const float* getBufferL() const { return bufL.data(); }
    int getBufferSize() const { return NB; }
    int getWritePos() const { return wposAtomic.load (std::memory_order_relaxed); }
    double getSampleRate() const { return sr; }
    /** samples-ago of each active grain read head, -1 if inactive */
    float getGrainAgo (int i) const { return grainAgo[(size_t) i].load (std::memory_order_relaxed); }
    static constexpr int numGrains() { return NG; }

private:
    struct Svf
    {
        float a1 = 0, a2 = 0, a3 = 0, k = 1;
        float s1L = 0, s2L = 0, s1R = 0, s2R = 0;
        void set (float fc, float Q, double sr)
        {
            fc = std::min (fc, (float) (0.45 * sr));
            const float g = std::tan (3.14159265f * fc / (float) sr);
            k = 1.0f / Q;
            a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
        }
        inline void run (float x, float& s1, float& s2, float& lo, float& bp)
        {
            const float v3 = x - s2;
            const float v1 = a1 * s1 + a2 * v3;
            const float v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2.0f * v1 - s1; s2 = 2.0f * v2 - s2;
            lo = v2; bp = v1;
        }
        float lpL (float x) { float lo, bp; run (x, s1L, s2L, lo, bp); return lo; }
        float lpR (float x) { float lo, bp; run (x, s1R, s2R, lo, bp); return lo; }
        float hpL (float x) { float lo, bp; run (x, s1L, s2L, lo, bp); return x - k * bp - lo; }
        float hpR (float x) { float lo, bp; run (x, s1R, s2R, lo, bp); return x - k * bp - lo; }
    };
    Svf hp, lp;
    std::atomic<int> wposAtomic { 0 };
    std::array<std::atomic<float>, 32> grainAgo {};

    struct Grain { bool active = false; double pos = 0, inc = 1; int age = 0, dur = 1; float gL = 0, gR = 0; };
    static constexpr int NG = 32;
    static constexpr int TN = 8192;

    static float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

    uint32_t rs = 0x9E3779B9u;
    float rnd01() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (float) (rs >> 8) * (1.0f / 16777216.0f); }
    float rndPM() { return rnd01() * 2.0f - 1.0f; }

    float read (const std::vector<float>& b, double idx) const
    {
        const double fl = std::floor (idx);
        const int i0 = (int) (((int64_t) fl) & (int64_t) mask);
        const int i1 = (i0 + 1) & mask;
        const float fr = (float) (idx - fl);
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }
    static float readTape (const std::vector<float>& b, double idx)
    {
        const double fl = std::floor (idx);
        const int i0 = (int) (((int64_t) fl) & (int64_t) (TN - 1));
        const int i1 = (i0 + 1) & (TN - 1);
        const float fr = (float) (idx - fl);
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * fr;
    }

    void spawn (const GrainParams& p, double gdur, int wi)
    {
        for (auto& gr : grains)
        {
            if (gr.active) continue;
            const double semis = p.gpitch + rndPM() * p.prand;
            const double rate = std::pow (2.0, semis / 12.0);
            double inc = rate;
            if (rnd01() * 100.0f < p.grev) inc = -rate;
            double back = p.gpos * 0.01 * 4.0 * sr + rnd01() * p.spray * 0.01 * sr;
            double need = 64.0;
            if (inc > 1.0) need = gdur * (inc - 1.0) + 64.0;
            back = std::max (back, need);
            back = std::min (back, (double) NB - gdur * 4.0 - 256.0);
            const float pan = 0.5f + rndPM() * 0.5f * p.gwidth * 0.01f;
            gr.active = true;
            gr.pos = wi - back;
            gr.inc = inc;
            gr.age = 0;
            gr.dur = (int) gdur;
            gr.gL = std::cos (pan * 1.5707963f);
            gr.gR = std::sin (pan * 1.5707963f);
            return;
        }
    }

    double sr = 48000.0, spb = 24000.0;
    int NB = 1 << 18, mask = (1 << 18) - 1;
    std::vector<float> bufL, bufR, tapeL, tapeR;
    Grain grains[NG];
    int wpos = 0, tw = 0, rstart = 0, remain = 0;
    double gph = 0, rp = 0, rrate = 1, rlen = 1000, wph = 0, fph = 0;
    int64_t prevSub = INT64_MIN;
    bool stut = false;
    float sg = 0, fbl = 0, fbr = 0, dsc = 0, hl = 0, hr = 0, lpl = 0, lpr = 0, drift = 0;
    float smix = 1, sgain = 1, scloud = 0.35f;
};
} // namespace sonic
