#include "PluginEditor.h"

//==============================================================================
void FETLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startAngle, float endAngle,
                                       juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float knobR  = radius * 0.70f;
    const float angle  = startAngle + pos * (endAngle - startAngle);

    // Marcas alrededor de la perilla
    g.setColour (juce::Colour (0xff222222));
    for (int i = 0; i <= 10; ++i)
    {
        const float a  = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const bool big = (i % 5 == 0);
        const float r1 = radius * 0.80f;
        const float r2 = radius * (big ? 0.98f : 0.92f);

        g.drawLine (centre.x + std::sin (a) * r1, centre.y - std::cos (a) * r1,
                    centre.x + std::sin (a) * r2, centre.y - std::cos (a) * r2,
                    big ? 2.0f : 1.2f);
    }

    // Sombra
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillEllipse (centre.x - knobR + 1.5f, centre.y - knobR + 3.0f, knobR * 2.0f, knobR * 2.0f);

    // Cuerpo
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4a), centre.x - knobR * 0.5f, centre.y - knobR * 0.7f,
                                             juce::Colour (0xff0d0d0d), centre.x + knobR * 0.6f, centre.y + knobR * 0.8f,
                                             false));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);

    g.setColour (juce::Colour (0xff8a8a8a));
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.2f);

    // Tapa superior
    const float capR = knobR * 0.78f;
    g.setColour (juce::Colour (0xff161616));
    g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

    // Indicador blanco
    juce::Path p;
    p.addRoundedRectangle (-2.0f, -knobR * 0.92f, 4.0f, knobR * 0.45f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
void GRMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Marco
    g.setColour (juce::Colour (0xff141414));
    g.fillRoundedRectangle (b, 8.0f);

    // Cara del medidor (crema)
    const auto face = b.reduced (8.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffdf8e6), face.getCentreX(), face.getBottom(),
                                             juce::Colour (0xffe9dfbd), face.getCentreX(), face.getY(),
                                             false));
    g.fillRoundedRectangle (face, 5.0f);

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    const float radius   = face.getHeight() * 1.20f;
    const float maxAngle = juce::degreesToRadians (60.0f);
    const juce::Point<float> pivot (face.getCentreX(), face.getY() + radius + face.getHeight() * 0.12f);

    // 0 dB de GR a la derecha, 20 dB a la izquierda
    auto angleFor = [maxAngle] (float gr)
    {
        return juce::jmap (juce::jlimit (0.0f, 20.0f, gr), 0.0f, 20.0f, maxAngle, -maxAngle);
    };

    auto pointAt = [pivot] (float angle, float r)
    {
        return juce::Point<float> (pivot.x + std::sin (angle) * r, pivot.y - std::cos (angle) * r);
    };

    // Arco
    juce::Path arc;
    arc.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, -maxAngle, maxAngle, true);
    g.setColour (juce::Colour (0xff111111));
    g.strokePath (arc, juce::PathStrokeType (1.5f));

    // Marcas menores cada 1 dB
    for (int i = 0; i <= 20; ++i)
    {
        const float a = angleFor ((float) i);
        g.setColour (i >= 10 ? juce::Colour (0xffb3261e) : juce::Colour (0xff111111));
        g.drawLine (juce::Line<float> (pointAt (a, radius), pointAt (a, radius * 0.96f)), 1.0f);
    }

    // Marcas mayores con numeros
    const float majors[] = { 0.0f, 3.0f, 5.0f, 7.0f, 10.0f, 20.0f };
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));

    for (const float v : majors)
    {
        const float a = angleFor (v);
        g.setColour (v >= 10.0f ? juce::Colour (0xffb3261e) : juce::Colour (0xff111111));
        g.drawLine (juce::Line<float> (pointAt (a, radius), pointAt (a, radius * 0.92f)), 2.0f);

        const auto t = pointAt (a, radius * 0.83f);
        g.drawText (juce::String ((int) v),
                    juce::Rectangle<float> (28.0f, 16.0f).withCentre (t),
                    juce::Justification::centred);
    }

    // Texto
    g.setColour (juce::Colour (0xcc111111));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("GAIN REDUCTION  (dB)",
                face.withTrimmedTop (face.getHeight() * 0.58f).withHeight (18.0f),
                juce::Justification::centred);

    // Aguja
    g.setColour (juce::Colour (0xff111111));
    g.drawLine (juce::Line<float> (pivot, pointAt (angleFor (value), radius * 1.02f)), 2.0f);

    g.restoreState();
}

//==============================================================================
FET76Editor::FET76Editor (FET76Processor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setSize (900, 380);

    for (auto* s : { &inputKnob, &outputKnob, &attackKnob, &releaseKnob, &mixKnob })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        s->setTextBoxIsEditable (false);
        s->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff111111));
        s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (*s);
    }

    inAtt  = std::make_unique<SliderAtt> (proc.apvts, "input",   inputKnob);
    outAtt = std::make_unique<SliderAtt> (proc.apvts, "output",  outputKnob);
    atkAtt = std::make_unique<SliderAtt> (proc.apvts, "attack",  attackKnob);
    relAtt = std::make_unique<SliderAtt> (proc.apvts, "release", releaseKnob);
    mixAtt = std::make_unique<SliderAtt> (proc.apvts, "mix",     mixKnob);

    addAndMakeVisible (meter);

    // Botones de ratio (grupo excluyente, como los botones del hardware)
    const char* names[] = { "4", "8", "12", "20", "ALL" };

    for (int i = 0; i < 5; ++i)
    {
        auto& b = ratioButtons[(size_t) i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1176);
        b.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff262626));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffc8382c));
        b.setColour (juce::TextButton::textColourOffId,  juce::Colours::white);
        b.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
        b.onClick = [this, i]
        {
            if (ratioButtons[(size_t) i].getToggleState() && ratioAtt != nullptr)
                ratioAtt->setValueAsCompleteGesture ((float) i);
        };
        addAndMakeVisible (b);
    }

    ratioAtt = std::make_unique<juce::ParameterAttachment> (
        *proc.apvts.getParameter ("ratio"),
        [this] (float v)
        {
            const int idx = juce::jlimit (0, 4, (int) std::lround (v));
            ratioButtons[(size_t) idx].setToggleState (true, juce::dontSendNotification);
        });
    ratioAtt->sendInitialUpdate();

    startTimerHz (30);
}

FET76Editor::~FET76Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FET76Editor::timerCallback()
{
    const float target = proc.grMeterDb.load();
    shownGr += (target - shownGr) * 0.35f;   // balistica simple de la aguja
    meter.setValue (shownGr);
}

void FET76Editor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Faceplate blanco / crema
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f3e6), 0.0f, 0.0f,
                                             juce::Colour (0xffe6dfca), 0.0f, b.getBottom(), false));
    g.fillAll();

    g.setColour (juce::Colour (0xff1c1c1c));
    g.drawRect (b, 6.0f);

    // Tornillos
    for (const auto& c : { juce::Point<float> (18.0f, 18.0f), juce::Point<float> (b.getRight() - 18.0f, 18.0f),
                           juce::Point<float> (18.0f, b.getBottom() - 18.0f),
                           juce::Point<float> (b.getRight() - 18.0f, b.getBottom() - 18.0f) })
    {
        g.setColour (juce::Colour (0xff9a9686));
        g.fillEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff4a483f));
        g.drawLine (c.x - 4.0f, c.y - 1.0f, c.x + 4.0f, c.y + 1.0f, 1.5f);
    }

    // Titulo
    g.setColour (juce::Colour (0xff111111));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawText ("FET-76", 40, 8, 120, 30, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("LIMITING AMPLIFIER", 150, 14, 220, 20, juce::Justification::centredLeft);

    // Etiquetas de las perillas
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));

    auto label = [&g] (const juce::Component& c, const juce::String& text)
    {
        g.drawText (text, c.getX(), c.getY() - 18, c.getWidth(), 16, juce::Justification::centred);
    };

    label (inputKnob,   "INPUT");
    label (outputKnob,  "OUTPUT");
    label (attackKnob,  "ATTACK");
    label (releaseKnob, "RELEASE");
    label (mixKnob,     "MIX");

    g.drawText ("RATIO", 380, 244, 260, 16, juce::Justification::centred);
}

void FET76Editor::resized()
{
    inputKnob.setBounds   (30, 60, 150, 170);
    outputKnob.setBounds  (720, 60, 150, 170);
    meter.setBounds       (250, 50, 400, 180);

    attackKnob.setBounds  (90, 262, 120, 105);
    releaseKnob.setBounds (230, 262, 120, 105);
    mixKnob.setBounds     (735, 262, 120, 105);

    for (int i = 0; i < 5; ++i)
        ratioButtons[(size_t) i].setBounds (380 + i * 52, 292, 50, 36);
}
