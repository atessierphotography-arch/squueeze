#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

SqueezeProcessor::SqueezeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

AudioProcessorValueTreeState::ParameterLayout SqueezeProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;

    auto knob = [&l] (const char* id, const char* name, float def, float hi = 10.0f)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
               NormalisableRange<float> (0.0f, hi, 0.01f), def));
    };
    auto toggle = [&l] (const char* id, const char* name, bool def)
    {
        l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    knob (ParamID::input,   "Input",   4.0f);
    knob (ParamID::attack,  "Attack",  3.0f);
    knob (ParamID::release, "Release", 4.0f);
    knob (ParamID::output,  "Output",  5.0f);
    knob (ParamID::mix,     "Mix",     100.0f, 100.0f);

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::ratio, 1 }, "Ratio",
           StringArray { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "Opto", "20:1", "Nuke" }, 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::audio, 1 }, "Audio",
           StringArray { "Clean", "Dist 2", "Dist 3", "Dist 2+3" }, 0));

    toggle (ParamID::detHP,   "Detector HP",   true);
    toggle (ParamID::detBump, "Detector bump", false);
    toggle (ParamID::link,    "Link",          false);
    toggle (ParamID::audioHP, "Audio HP",      false);
    toggle (ParamID::bypass,  "Bypass",        false);
    toggle (ParamID::power,   "Power",         true);
    toggle (ParamID::hr,      "HR",            false);
    return l;
}

bool SqueezeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void SqueezeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec = dsp::ProcessSpec { sampleRate, (uint32) samplesPerBlock, 2 };

    // Le suréchantillonnage x2 sert à la distorsion : latence fixe, compensée sur le dry et le bypass
    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();
    const int latency = roundToInt (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuf.setSize (2, samplesPerBlock);

    for (int ch = 0; ch < 2; ++ch)
    {
        scHP[(size_t) ch].coefficients     = Coeffs::makeHighPass (sampleRate, 100.0f, 0.707f);
        scBump[(size_t) ch].coefficients   = Coeffs::makePeakFilter (sampleRate, 3000.0f, 0.8f, Decibels::decibelsToGain (6.0f));
        audioHPf[(size_t) ch].coefficients = Coeffs::makeHighPass (sampleRate, 40.0f, 0.707f);
        scHP[(size_t) ch].reset();
        scBump[(size_t) ch].reset();
        audioHPf[(size_t) ch].reset();
        env[ch] = 0.0f;
        dcX[ch] = dcY[ch] = 0.0f;
    }

    inSm.reset (sampleRate, 0.02);   inSm.setCurrentAndTargetValue (1.0f);
    outSm.reset (sampleRate, 0.02);  outSm.setCurrentAndTargetValue (1.0f);
    wetSm.reset (sampleRate, 0.02);  wetSm.setCurrentAndTargetValue (1.0f);
}

void SqueezeProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0) return;
    if (dryBuf.getNumSamples() < n) dryBuf.setSize (2, n, false, false, true);

    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float inK = raw (ParamID::input), atkK = raw (ParamID::attack), relK = raw (ParamID::release);
    const float outK = raw (ParamID::output), mix = raw (ParamID::mix) * 0.01f;
    const int ratioIdx = jlimit (0, 8, roundToInt (raw (ParamID::ratio)));
    const int audioMode = roundToInt (raw (ParamID::audio));
    const bool useHP = raw (ParamID::detHP) > 0.5f, useBump = raw (ParamID::detBump) > 0.5f;
    const bool link = raw (ParamID::link) > 0.5f, audioHP = raw (ParamID::audioHP) > 0.5f;
    const bool bypassed = raw (ParamID::bypass) > 0.5f, powered = raw (ParamID::power) > 0.5f;
    const bool hr = raw (ParamID::hr) > 0.5f;
    const float sr = (float) spec.sampleRate;

    // Signal d'origine, retardé de la latence du suréchantillonnage (pour le mix parallèle et le bypass)
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* src = buffer.getReadPointer (ch);
        auto* dst = dryBuf.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            dryDelay.pushSample (ch, src[i]);
            dst[i] = dryDelay.popSample (ch);
        }
    }

    // ---------- Paramètres du compresseur ----------
    static const float ratios[] = { 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 10.0f, 4.0f, 20.0f, 100.0f };
    const bool opto = (ratioIdx == 6), nuke = (ratioIdx == 8);
    const float R = ratios[ratioIdx];
    const float threshold = nuke ? -18.0f : -12.0f;
    const float knee = opto ? 14.0f : (nuke ? 0.0f : 6.0f);

    // Attack 0.05 ms -> 150 ms, Release 50 ms -> 3.5 s (courbes exponentielles)
    float attackMs = 0.05f * std::pow (3000.0f, atkK / 10.0f);
    if (opto) attackMs = jmax (attackMs, 8.0f);
    const float releaseMs = 50.0f * std::pow (70.0f, relK / 10.0f);
    const float atkCoef = std::exp (-1.0f / (attackMs * 0.001f * sr));
    const float relCoef = std::exp (-1.0f / (releaseMs * 0.001f * sr));

    inSm.setTargetValue (Decibels::decibelsToGain (inK * 3.6f));              // Input : 0 -> +36 dB
    outSm.setTargetValue (Decibels::decibelsToGain ((outK - 5.0f) * 4.0f));   // Output : -20 -> +20 dB
    wetSm.setTargetValue ((powered && ! bypassed) ? mix : 0.0f);

    auto gainComputer = [&] (float levelDb)
    {
        const float over = levelDb - threshold;
        const float slope = 1.0f - 1.0f / R;
        if (knee <= 0.0f) return over > 0.0f ? over * slope : 0.0f;
        if (over <= -knee * 0.5f) return 0.0f;
        if (over >= knee * 0.5f) return over * slope;
        const float t = over + knee * 0.5f;
        return slope * t * t / (2.0f * knee);
    };

    auto updateEnv = [&] (int ch, float gr)
    {
        float& e = env[ch];
        if (gr > e)
            e = atkCoef * e + (1.0f - atkCoef) * gr;
        else
        {
            float rc = relCoef;
            if (opto)   // relâchement dépendant du programme, comme une cellule optique
            {
                const float ms = releaseMs * (0.35f + 0.65f * jmin (1.0f, e / 12.0f));
                rc = std::exp (-1.0f / (ms * 0.001f * sr));
            }
            e = rc * e + (1.0f - rc) * gr;
        }
    };

    // ---------- Compression : détecteur + réduction de gain ----------
    float maxGr = 0.0f;
    float* L = buffer.getWritePointer (0);
    float* Rch = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float gi = inSm.getNextValue();
        const float x0 = L[i] * gi, x1 = Rch[i] * gi;

        const float h0 = scHP[0].processSample (x0), h1 = scHP[1].processSample (x1);
        const float b0 = scBump[0].processSample (useHP ? h0 : x0), b1 = scBump[1].processSample (useHP ? h1 : x1);
        const float d0 = useBump ? b0 : (useHP ? h0 : x0);
        const float d1 = useBump ? b1 : (useHP ? h1 : x1);

        float a0 = std::abs (d0), a1 = std::abs (d1);
        if (link) a0 = a1 = jmax (a0, a1);

        const float gr0 = gainComputer (20.0f * std::log10 (a0 + 1.0e-9f));
        const float gr1 = link ? gr0 : gainComputer (20.0f * std::log10 (a1 + 1.0e-9f));
        updateEnv (0, gr0);
        updateEnv (1, gr1);
        maxGr = jmax (maxGr, env[0], env[1]);

        float y0 = x0 * Decibels::decibelsToGain (-env[0]);
        float y1 = x1 * Decibels::decibelsToGain (-env[1]);
        if (audioHP) { y0 = audioHPf[0].processSample (y0); y1 = audioHPf[1].processSample (y1); }
        L[i] = y0;
        Rch[i] = y1;
    }

    // ---------- Distorsion Dist 2 (harmoniques paires) / Dist 3 (impaires), suréchantillonnée x2 ----------
    dsp::AudioBlock<float> block (buffer);
    auto up = oversampling.processSamplesUp (block);
    const bool d2 = (audioMode & 1) != 0, d3 = (audioMode & 2) != 0;
    if (d2 || d3)
    {
        const float drive = hr ? 2.0f : 4.0f;    // HR : plus de headroom = distorsion plus douce
        for (size_t ch = 0; ch < 2; ++ch)
        {
            auto* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                float x = d[i] * drive;
                if (d2) x += 0.12f * x * x;
                if (d3) { x = jlimit (-1.5f, 1.5f, x); x -= x * x * x / 6.75f; }
                if (d2)
                {
                    const float y = x - dcX[ch] + 0.9995f * dcY[ch];   // enlève la composante continue
                    dcX[ch] = x;  dcY[ch] = y;  x = y;
                }
                d[i] = x / drive;
            }
        }
    }
    oversampling.processSamplesDown (block);

    // ---------- Output, Mix parallèle, Bypass ----------
    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float go = outSm.getNextValue();
        const float w = wetSm.getNextValue();
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            const float wet = d[i] * go;
            peak = jmax (peak, std::abs (wet));
            d[i] = wet * w + dryBuf.getSample (ch, i) * (1.0f - w);
        }
    }

    if (powered && ! bypassed)
    {
        grMeter.store (jmax (maxGr, grMeter.load() * 0.9f));
        outPeak.store (jmax (peak, outPeak.load() * 0.92f));
    }
    else
    {
        grMeter.store (0.0f);
        outPeak.store (0.0f);
    }
}

AudioProcessorEditor* SqueezeProcessor::createEditor()
{
    return new SqueezeEditor (*this);
}

void SqueezeProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SqueezeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SqueezeProcessor();
}
