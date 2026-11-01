#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <cmath>

namespace fet76
{
    // Perillas 1..7 como en el hardware (7 = mas rapido)
    inline float attackSeconds (float knob)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (knob - 1.0f) / 6.0f);
        return 800.0e-6f * std::pow (20.0f / 800.0f, t);      // 800 us -> 20 us
    }

    inline float releaseSeconds (float knob)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (knob - 1.0f) / 6.0f);
        return 1.1f * std::pow (0.05f / 1.1f, t);              // 1100 ms -> 50 ms
    }
}

class FET76Processor : public juce::AudioProcessor
{
public:
    FET76Processor();
    ~FET76Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FET76"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Reduccion de ganancia (dB, positivo) del ultimo bloque, para el medidor
    std::atomic<float> grMeterDb { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* inputParam   = nullptr;
    std::atomic<float>* outputParam  = nullptr;
    std::atomic<float>* attackParam  = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* ratioParam   = nullptr;
    std::atomic<float>* mixParam     = nullptr;

    juce::SmoothedValue<float> inSmooth, outSmooth, mixSmooth;

    double sampleRate = 44100.0;
    float grState = 0.0f;   // reduccion de ganancia suavizada, en dB

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FET76Processor)
};
