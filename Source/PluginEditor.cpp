#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
constexpr int kEditorWidth = 900;
constexpr int kEditorHeight = 675;
constexpr float kReferenceWidth = 1448.0f;
constexpr float kReferenceHeight = 1086.0f;

const juce::Colour kInk(0xff18110e);
const juce::Colour kCream(0xffefe0bf);
const juce::Colour kRed(0xffb9221e);
const juce::Colour kBlue(0xff2f94a0);
const juce::Colour kChromeLight(0xfff4eee0);
const juce::Colour kChromeMid(0xff8f928e);
const juce::Colour kChromeDark(0xff1d1d1b);

bool isLarge(const juce::Slider& slider)
{
    if (const auto* madnessSlider = dynamic_cast<const MadnessSlider*>(&slider))
        return madnessSlider->large;
    return false;
}
}

PWMMadnessLookAndFeel::PWMMadnessLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff120d0b));
    setColour(juce::ComboBox::outlineColourId, kInk);
    setColour(juce::ComboBox::textColourId, kCream);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1d1813));
    setColour(juce::PopupMenu::textColourId, kCream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kRed);
}

void PWMMadnessLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float, float, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto knob = juce::Rectangle<float>(bounds.getCentreX() - size * 0.5f,
                                            bounds.getCentreY() - size * 0.5f, size, size);
    const auto centre = knob.getCentre();

    g.setColour(juce::Colours::black.withAlpha(0.34f));
    g.fillEllipse(knob.translated(2.0f, 4.0f));

    juce::ColourGradient chrome(kChromeLight, centre.x - size * 0.25f, knob.getY(),
                                kChromeDark, centre.x + size * 0.25f, knob.getBottom(), false);
    chrome.addColour(0.48, kChromeMid);
    chrome.addColour(0.62, juce::Colour(0xfff7f4ea));
    g.setGradientFill(chrome);
    g.fillEllipse(knob);

    g.setColour(kInk);
    g.drawEllipse(knob, isLarge(slider) ? 3.0f : 2.0f);

    const float capInset = size * 0.22f;
    g.setColour(juce::Colour(0xff29241f));
    g.fillEllipse(knob.reduced(capInset));
    g.setColour(juce::Colour(0x66ffffff));
    g.drawEllipse(knob.reduced(capInset + 1.0f), 1.0f);

    const float angle = juce::jmap(sliderPos, 0.0f, 1.0f,
        -juce::MathConstants<float>::pi * 0.74f,
         juce::MathConstants<float>::pi * 0.74f);
    const auto dir = juce::Point<float>(std::cos(angle - juce::MathConstants<float>::halfPi),
                                        std::sin(angle - juce::MathConstants<float>::halfPi));
    const float radius = size * 0.5f;
    const auto a = centre + dir * (radius * 0.12f);
    const auto b = centre + dir * (radius * 0.82f);
    g.setColour(kCream);
    g.drawLine(a.x, a.y, b.x, b.y, isLarge(slider) ? 4.0f : 2.5f);
}

void PWMMadnessLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);
    g.setColour(juce::Colour(0xff15110f));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(kInk);
    g.drawRoundedRectangle(bounds, 4.0f, 1.4f);

    juce::Path arrow;
    arrow.startNewSubPath((float)width - 18.0f, (float)height * 0.42f);
    arrow.lineTo((float)width - 12.0f, (float)height * 0.60f);
    arrow.lineTo((float)width - 6.0f, (float)height * 0.42f);
    g.setColour(kCream);
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

juce::Font PWMMadnessLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return juce::Font(juce::FontOptions((float)juce::jmin(15, box.getHeight() - 4)).withStyle("Bold"));
}

void PWMMadnessLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(6, 0, box.getWidth() - 24, box.getHeight());
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, kCream);
    label.setFont(getComboBoxFont(box));
}

PWMMadnessAudioProcessorEditor::PWMMadnessAudioProcessorEditor(PWMMadnessAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      background(juce::ImageFileFormat::loadFrom(BinaryData::vintage_synth_madness_and_beach_vibes_png,
                                                 BinaryData::vintage_synth_madness_and_beach_vibes_pngSize))
{
    setResizable(false, false);
    setSize(kEditorWidth, kEditorHeight);
    setLookAndFeel(&lookAndFeel);

    modeToggle.setAlpha(0.0f);
    modeToggle.setClickingTogglesState(true);
    modeToggle.onStateChange = [this] { repaint(); };
    addAndMakeVisible(modeToggle);

    for (auto* slider : { &tune, &attack, &release, &osc1PW, &osc2PW, &osc2Detune, &oscMix,
                         &subLevel, &pwmDepth, &pwmRate, &filterCutoff, &filterResonance,
                         &drive, &outputGain, &madness, &octave })
    {
        setupSlider(*slider);
        addAndMakeVisible(*slider);
    }

    auto& state = audioProcessor.apvts;
    modeAttachment = std::make_unique<ButtonAttachment>(state, "mode", modeToggle);
    octaveAttachment = std::make_unique<SliderAttachment>(state, "octave", octave);
    tuneAttachment = std::make_unique<SliderAttachment>(state, "tune", tune);
    attackAttachment = std::make_unique<SliderAttachment>(state, "attack", attack);
    releaseAttachment = std::make_unique<SliderAttachment>(state, "release", release);
    osc1PWAttachment = std::make_unique<SliderAttachment>(state, "osc1PW", osc1PW);
    osc2PWAttachment = std::make_unique<SliderAttachment>(state, "osc2PW", osc2PW);
    osc2DetuneAttachment = std::make_unique<SliderAttachment>(state, "osc2Detune", osc2Detune);
    oscMixAttachment = std::make_unique<SliderAttachment>(state, "oscMix", oscMix);
    subLevelAttachment = std::make_unique<SliderAttachment>(state, "subLevel", subLevel);
    pwmDepthAttachment = std::make_unique<SliderAttachment>(state, "pwmDepth", pwmDepth);
    pwmRateAttachment = std::make_unique<SliderAttachment>(state, "pwmRate", pwmRate);
    filterCutoffAttachment = std::make_unique<SliderAttachment>(state, "filterCutoff", filterCutoff);
    filterResonanceAttachment = std::make_unique<SliderAttachment>(state, "filterResonance", filterResonance);
    driveAttachment = std::make_unique<SliderAttachment>(state, "drive", drive);
    outputGainAttachment = std::make_unique<SliderAttachment>(state, "outputGain", outputGain);
    madnessAttachment = std::make_unique<SliderAttachment>(state, "madness", madness);

    resized();
}

PWMMadnessAudioProcessorEditor::~PWMMadnessAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PWMMadnessAudioProcessorEditor::setupSlider(MadnessSlider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setAlpha(0.0f);
    slider.onValueChange = [this] { repaint(); };
}

juce::Rectangle<int> PWMMadnessAudioProcessorEditor::ref(float x, float y, float w, float h) const
{
    const auto bounds = getLocalBounds();
    const float sx = (float)bounds.getWidth() / kReferenceWidth;
    const float sy = (float)bounds.getHeight() / kReferenceHeight;
    return {
        bounds.getX() + juce::roundToInt(x * sx),
        bounds.getY() + juce::roundToInt(y * sy),
        juce::roundToInt(w * sx),
        juce::roundToInt(h * sy)
    };
}

void PWMMadnessAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080604));

    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    else
    {
        auto b = getLocalBounds().toFloat();
        g.setColour(kCream);
        g.fillRect(b);
        g.setColour(kBlue);
        g.fillRect(ref(0, 0, kReferenceWidth, 92).toFloat());
        g.setColour(kRed);
        g.fillRect(ref(1210, 570, 360, 270).toFloat());
        g.setColour(kInk);
        g.setFont(juce::Font(juce::FontOptions(42.0f).withStyle("Bold")));
        g.drawText("PWM MADNESS - WAVE SHAPING UNIT", ref(470, 22, 760, 54), juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(58.0f).withStyle("Bold")));
        g.drawText("MADNESS", ref(1240, 596, 300, 68), juce::Justification::centred);
        g.setFont(juce::Font(juce::FontOptions(24.0f).withStyle("Bold")));
        g.drawText("More Is More", ref(1260, 790, 260, 40), juce::Justification::centred);
    }
}

void PWMMadnessAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    auto drawPointer = [&](MadnessSlider& slider)
    {
        if (!slider.isVisible())
            return;

        const auto bounds = slider.getBounds().toFloat();
        const float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 6.0f;
        const auto centre = bounds.getCentre();
        const float radius = size * 0.5f;
        const float normalized = (float)slider.valueToProportionOfLength(slider.getValue());
        const float angle = juce::jmap(normalized, 0.0f, 1.0f,
            -juce::MathConstants<float>::pi * 0.74f,
             juce::MathConstants<float>::pi * 0.74f);
        const auto dir = juce::Point<float>(std::cos(angle - juce::MathConstants<float>::halfPi),
                                            std::sin(angle - juce::MathConstants<float>::halfPi));
        g.setColour(kCream);
        const float innerRadius = slider.large ? 0.0f : 0.12f;
        g.drawLine((centre + dir * (radius * innerRadius)).x, (centre + dir * (radius * innerRadius)).y,
                   (centre + dir * (radius * 0.82f)).x, (centre + dir * (radius * 0.82f)).y,
                   slider.large ? 4.0f : 2.5f);
    };

    for (auto* slider : { &tune, &attack, &release, &osc1PW, &osc2PW, &osc2Detune, &oscMix,
                         &subLevel, &pwmDepth, &pwmRate, &filterCutoff, &filterResonance,
                         &drive, &outputGain, &madness, &octave })
        drawPointer(*slider);

    const bool alwaysOn = modeToggle.getToggleState();
    {
        const auto led = ref(896.0f, 292.0f, 19.0f, 19.0f).toFloat();
        const auto centre = led.getCentre();

        if (alwaysOn)
        {
            const auto glow = led.expanded(9.0f);
            juce::ColourGradient amberGlow(juce::Colour(0xffffb037).withAlpha(0.56f), centre.x, centre.y,
                                           juce::Colours::transparentBlack, glow.getRight(), centre.y, true);
            amberGlow.addColour(0.34, juce::Colour(0xffff6f19).withAlpha(0.28f));
            g.setGradientFill(amberGlow);
            g.fillEllipse(glow);
        }

        juce::ColourGradient lens(alwaysOn ? juce::Colour(0xffffd162) : juce::Colour(0xff4b140d),
                                  centre.x - led.getWidth() * 0.24f, led.getY(),
                                  alwaysOn ? juce::Colour(0xffb93110) : juce::Colour(0xff170806),
                                  centre.x + led.getWidth() * 0.32f, led.getBottom(), false);
        lens.addColour(0.55, alwaysOn ? juce::Colour(0xffff761e) : juce::Colour(0xff260906));
        g.setGradientFill(lens);
        g.fillEllipse(led);

        g.setColour(kInk.withAlpha(0.72f));
        g.drawEllipse(led, 1.2f);

        if (alwaysOn)
        {
            g.setColour(juce::Colours::white.withAlpha(0.72f));
            g.fillEllipse(led.withSizeKeepingCentre(led.getWidth() * 0.28f, led.getHeight() * 0.28f)
                              .translated(-led.getWidth() * 0.12f, -led.getHeight() * 0.16f));
        }
    }

    {
        const auto b = modeToggle.getBounds().toFloat();
        const float rx = 6.0f;
        g.setColour(juce::Colour(0xff1a1410));
        g.fillRoundedRectangle(b, rx);
        g.setColour(kInk);
        g.drawRoundedRectangle(b.reduced(0.5f), rx, 1.2f);

        const float ballDiam = b.getHeight() * 0.72f;
        const float margin = (b.getHeight() - ballDiam) * 0.5f;
        const float ballX = alwaysOn
            ? b.getRight() - margin - ballDiam
            : b.getX() + margin;
        const auto ball = juce::Rectangle<float>(ballX, b.getY() + margin, ballDiam, ballDiam);
        const auto centre = ball.getCentre();

        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillEllipse(ball.translated(1.0f, 2.0f));

        juce::ColourGradient chrome(kChromeLight, centre.x - ballDiam * 0.25f, ball.getY(),
                                    kChromeDark, centre.x + ballDiam * 0.25f, ball.getBottom(), false);
        chrome.addColour(0.48, kChromeMid);
        chrome.addColour(0.62, juce::Colour(0xfff7f4ea));
        g.setGradientFill(chrome);
        g.fillEllipse(ball);
        g.setColour(kInk);
        g.drawEllipse(ball, 1.5f);
    }

}

void PWMMadnessAudioProcessorEditor::resized()
{
    modeToggle.setBounds(ref(824, 226, 62, 43));
    octave.setBounds(ref(1077, 216, 82, 82));

    tune.setBounds(ref(955, 216, 82, 82));
    attack.setBounds(ref(1212, 216, 82, 82));
    release.setBounds(ref(1340, 216, 82, 82));

    osc1PW.setBounds(ref(832, 400, 82, 82));
    osc2PW.setBounds(ref(963, 400, 82, 82));
    osc2Detune.setBounds(ref(1085, 400, 82, 82));
    oscMix.setBounds(ref(1213, 400, 82, 82));
    subLevel.setBounds(ref(1336, 400, 82, 82));

    pwmDepth.setBounds(ref(832, 576, 82, 82));
    pwmRate.setBounds(ref(963, 576, 82, 82));

    filterCutoff.setBounds(ref(1153, 576, 82, 82));
    filterResonance.setBounds(ref(1320, 576, 82, 82));

    drive.setBounds(ref(831, 744, 82, 82));
    outputGain.setBounds(ref(963, 744, 82, 82));

    madness.setBounds(ref(1138, 785, 244, 244));

}
