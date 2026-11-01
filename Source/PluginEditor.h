#pragma once

#include "PluginProcessor.h"

// Perillas negras con indicador blanco y marcas alrededor
class FETLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
};

// Medidor de reduccion de ganancia (aguja, 0 a la derecha)
class GRMeter : public juce::Component
{
public:
    void setValue (float grDb)
    {
        value = grDb;
        repaint();
    }

    void paint (juce::Graphics& g) override;

private:
    float value = 0.0f;
};

class FET76Editor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit FET76Editor (FET76Processor&);
    ~FET76Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    FET76Processor& proc;
    FETLookAndFeel lnf;

    GRMeter meter;
    juce::Slider inputKnob, outputKnob, attackKnob, releaseKnob, mixKnob;
    std::array<juce::TextButton, 5> ratioButtons;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> inAtt, outAtt, atkAtt, relAtt, mixAtt;
    std::unique_ptr<juce::ParameterAttachment> ratioAtt;

    float shownGr = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FET76Editor)
};
