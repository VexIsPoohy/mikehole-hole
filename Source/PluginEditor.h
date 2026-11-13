#pragma once

#include "PluginProcessor.h"

#include <array>
#include <memory>

class MikeLAF final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;

    juce::Colour acc { juce::Colours::white };
};

class MikeEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MikeEditor (MikeProcessor&);
    ~MikeEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;
    void selectPreset (int);
    void applyColours();
    void pickTarget();
    float param (const char*) const;

    MikeProcessor& proc;
    MikeLAF laf;
    juce::Image mike;
    int cur = 0;
    std::array<juce::Slider, 10> knobs;
    std::array<std::unique_ptr<SliderAttachment>, 10> att;
    std::array<juce::Label, 10> labels;
    std::array<juce::TextButton, 8> btn;
    std::array<juce::Colour, 4> cc, tc;
    juce::Random rnd;

    float tx = 0.f, ty = 0.f, trot = 0.f, tscl = 1.f;
    float px = 0.f, py = 0.f, rot = 0.f, scl = 1.f;
    int lastBeat = 0, frame = 0, dir = 1;
    float bobT = -1.f, bobAmp = 0.f, flick = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MikeEditor)
};
