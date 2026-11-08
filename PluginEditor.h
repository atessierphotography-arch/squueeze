#pragma once
#include "PluginProcessor.h"

// Gros bouton argenté moleté avec échelle 0-10 imprimée autour ; petit bouton noir pour MIX
class SilverKnobLnF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

// Bouton-poussoir argenté (BY PASS, RATIO, DETECTOR, AUDIO)
class SilverButton : public juce::Button
{
public:
    SilverButton() : juce::Button ("silver") {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Interrupteur à bascule POWER
class RockerButton : public juce::Button
{
public:
    RockerButton() : juce::Button ("power") {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Petit interrupteur HR
class HrSwitch : public juce::Button
{
public:
    HrSwitch() : juce::Button ("hr") {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

class SqueezeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SqueezeEditor (SqueezeProcessor&);
    ~SqueezeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void buildBackground();
    float getParam (const char* id) const;
    void setParam (const char* id, float realValue);
    void cycleParam (const char* id, int count);

    SqueezeProcessor& processor;
    SilverKnobLnF laf;
    juce::Image background;

    juce::Slider inputKnob, attackKnob, releaseKnob, outputKnob, mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inA, atA, reA, ouA, miA;

    SilverButton bypassBtn, ratioBtn, detectorBtn, audioBtn;
    RockerButton powerBtn;
    HrSwitch hrBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> byA, poA, hrA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SqueezeEditor)
};
