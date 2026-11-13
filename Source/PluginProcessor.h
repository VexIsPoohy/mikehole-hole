#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MikeProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    MikeProcessor();
    ~MikeProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Mike Holes Hole"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int index);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int>   beatCount { 0 };      // editor watches this to bob Mike's head
    std::atomic<float> beatStrength { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void run() override;   // IR builder thread
    juce::AudioBuffer<float> buildIR (double sr, float size, float dens, float grain, float scat, float width);

    std::atomic<float>* P[10] {};
    std::atomic<bool> irDirty { true };
    std::atomic<double> sampleRateAtomic { 44100.0 };
    float lastIR[5] { -1.f, -1.f, -1.f, -1.f, -1.f };

    juce::dsp::Convolution conv;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
    juce::dsp::StateVariableTPTFilter<float> damp, lowCut, highCut;
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float> dryG, wetG, preSm;
    double sr = 44100.0;

    float beatAvg = 0.f; int64_t sampleCounter = 0, lastBeatSample = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MikeProcessor)
};
