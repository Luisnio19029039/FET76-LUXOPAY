#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout FET76Processor::createLayout()
{
    using Attr  = juce::AudioParameterFloatAttributes;
    using Range = juce::NormalisableRange<float>;

    auto dbText  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto pctText = [] (float v, int) { return juce::String (v, 0) + " %"; };
    auto atkText = [] (float v, int) { return juce::String (fet76::attackSeconds (v) * 1.0e6f, 0) + " us"; };
    auto relText = [] (float v, int) { return juce::String (fet76::releaseSeconds (v) * 1000.0f, 0) + " ms"; };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input", Range (0.0f, 40.0f, 0.1f), 12.0f,
        Attr().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output", Range (-24.0f, 24.0f, 0.1f), 0.0f,
        Attr().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack", Range (1.0f, 7.0f, 0.01f), 3.0f,
        Attr().withStringFromValueFunction (atkText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release", Range (1.0f, 7.0f, 0.01f), 5.0f,
        Attr().withStringFromValueFunction (relText)));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ratio", 1 }, "Ratio",
        juce::StringArray { "4:1", "8:1", "12:1", "20:1", "ALL" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", Range (0.0f, 100.0f, 0.1f), 100.0f,
        Attr().withStringFromValueFunction (pctText)));

    return layout;
}

FET76Processor::FET76Processor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    inputParam   = apvts.getRawParameterValue ("input");
    outputParam  = apvts.getRawParameterValue ("output");
    attackParam  = apvts.getRawParameterValue ("attack");
    releaseParam = apvts.getRawParameterValue ("release");
    ratioParam   = apvts.getRawParameterValue ("ratio");
    mixParam     = apvts.getRawParameterValue ("mix");
}

void FET76Processor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    grState = 0.0f;

    inSmooth.reset  (sampleRate, 0.02);
    outSmooth.reset (sampleRate, 0.02);
    mixSmooth.reset (sampleRate, 0.02);

    inSmooth.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (inputParam->load()));
    outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    mixSmooth.setCurrentAndTargetValue (mixParam->load() * 0.01f);
}

bool FET76Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == layouts.getMainInputChannelSet()
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void FET76Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0)
        return;

    inSmooth.setTargetValue  (juce::Decibels::decibelsToGain (inputParam->load()));
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    mixSmooth.setTargetValue (mixParam->load() * 0.01f);

    // --- Ratio / curva ------------------------------------------------------
    static constexpr float ratios[4] = { 4.0f, 8.0f, 12.0f, 20.0f };
    static constexpr float knees[4]  = { 8.0f, 6.0f, 4.0f, 3.0f };   // dB, mas duro a mayor ratio

    const int  ratioIdx   = juce::jlimit (0, 4, (int) std::lround (ratioParam->load()));
    const bool allButtons = (ratioIdx == 4);

    const float ratio     = allButtons ? 20.0f : ratios[ratioIdx];
    const float knee      = allButtons ? 4.0f  : knees[ratioIdx];
    const float threshold = allButtons ? -14.0f : -10.0f;   // dBFS, fijo (la perilla Input empuja la senal)
    const float drive     = allButtons ? 1.1f  : 0.7f;      // saturacion de la etapa de salida
    const float slope     = 1.0f - 1.0f / ratio;

    // --- Tiempos ------------------------------------------------------------
    const float attackSec  = fet76::attackSeconds  (attackParam->load());
    const float releaseSec = fet76::releaseSeconds (releaseParam->load());
    const float attackCoef  = std::exp (-1.0f / (attackSec  * (float) sampleRate));
    const float releaseCoef = std::exp (-1.0f / (releaseSec * (float) sampleRate));

    float* data[2] = { buffer.getWritePointer (0),
                       numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    float maxGr = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float gIn = inSmooth.getNextValue();
        const float gOut = outSmooth.getNextValue();
        const float mix = mixSmooth.getNextValue();

        // Detector de picos enlazado (stereo link)
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = std::max (peak, std::abs (data[ch][n] * gIn));

        const float levelDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        const float over    = levelDb - threshold;

        // Calculo de ganancia con rodilla suave
        float targetGr = 0.0f;
        if (2.0f * over > knee)
        {
            targetGr = slope * over;
        }
        else if (2.0f * over > -knee)
        {
            const float t = over + knee * 0.5f;
            targetGr = slope * t * t / (2.0f * knee);
        }

        // Ataque / release en el dominio de dB
        const float coef = (targetGr > grState) ? attackCoef : releaseCoef;
        grState = targetGr + coef * (grState - targetGr);
        maxGr = std::max (maxGr, grState);

        const float comp = juce::Decibels::decibelsToGain (-grState);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dry = data[ch][n];
            float wet = dry * gIn * comp;
            wet = std::tanh (wet * drive) / drive;   // color tipo FET / transformador
            wet *= gOut;
            data[ch][n] = dry + (wet - dry) * mix;
        }
    }

    grMeterDb.store (maxGr);
}

juce::AudioProcessorEditor* FET76Processor::createEditor()
{
    return new FET76Editor (*this);
}

void FET76Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FET76Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FET76Processor();
}
