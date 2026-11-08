#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace ParamID
{
    inline constexpr const char* input    = "input";
    inline constexpr const char* attack   = "attack";
    inline constexpr const char* release  = "release";
    inline constexpr const char* output   = "output";
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* ratio    = "ratio";     // 1:1 2:1 3:1 4:1 6:1 10:1 OPTO 20:1 NUKE
    inline constexpr const char* detHP    = "detHP";     // filtre passe-haut dans le détecteur
    inline constexpr const char* detBump  = "detBump";   // bosse de présence dans le détecteur
    inline constexpr const char* link     = "link";      // liaison stéréo
    inline constexpr const char* audio    = "audio";     // 0 clean, 1 Dist 2, 2 Dist 3, 3 les deux
    inline constexpr const char* audioHP  = "audioHP";   // passe-haut sur le signal audio
    inline constexpr const char* bypass   = "bypass";
    inline constexpr const char* power    = "power";
    inline constexpr const char* hr       = "hr";        // headroom
}

class SqueezeProcessor : public juce::AudioProcessor
{
public:
    SqueezeProcessor();
    ~SqueezeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MixWaves Squeeze"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grMeter { 0.0f };    // réduction de gain (dB) pour les LED
    std::atomic<float> outPeak { 0.0f };    // crête de sortie (linéaire) pour REDLINE / 1% THD

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Filter = juce::dsp::IIR::Filter<float>;

    std::array<Filter, 2> scHP, scBump, audioHPf;
    float env[2] { 0.0f, 0.0f };           // enveloppe de réduction de gain (dB, positive)
    float dcX[2] { 0.0f, 0.0f }, dcY[2] { 0.0f, 0.0f };

    juce::dsp::ProcessSpec spec {};
    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuf;
    juce::SmoothedValue<float> inSm, outSm, wetSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueezeProcessor)
};
