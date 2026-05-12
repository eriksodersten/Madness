#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
constexpr int kEditorWidth = 909;
constexpr int kEditorHeight = 675;
constexpr float kReferenceWidth = 1492.0f;
constexpr float kReferenceHeight = 1108.0f;

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
    startTimerHz(30);
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

void PWMMadnessAudioProcessorEditor::timerCallback()
{
    if (modeToggle.getToggleState())
        repaint(ref(880.0f, 276.0f, 52.0f, 52.0f));

    const float madnessAmount = (float)madness.valueToProportionOfLength(madness.getValue());
    if (madnessAmount > 0.01f)
        repaint(ref(1078.0f, 92.0f, 88.0f, 88.0f));
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
        const float innerRadius = slider.large ? 0.08f : 0.12f;
        const float outerRadius = slider.large ? 0.64f : 0.82f;
        g.drawLine((centre + dir * (radius * innerRadius)).x, (centre + dir * (radius * innerRadius)).y,
                   (centre + dir * (radius * outerRadius)).x, (centre + dir * (radius * outerRadius)).y,
                   slider.large ? 3.4f : 2.5f);
    };

    for (auto* slider : { &tune, &attack, &release, &osc1PW, &osc2PW, &osc2Detune, &oscMix,
                         &subLevel, &pwmDepth, &pwmRate, &filterCutoff, &filterResonance,
                         &drive, &outputGain, &madness, &octave })
        drawPointer(*slider);

    const bool alwaysOn = modeToggle.getToggleState();
    {
        const auto led = ref(896.0f, 292.0f, 19.0f, 19.0f).toFloat();
        const auto centre = led.getCentre();
        const auto litCentre = centre.translated(led.getWidth() * 0.05f, led.getHeight() * 0.06f);
        const double blinkPhase = std::sin(juce::Time::getMillisecondCounterHiRes()
                                           * juce::MathConstants<double>::twoPi * 0.002);
        const float blink = alwaysOn ? (float)juce::jmap(blinkPhase, -1.0, 1.0, 0.12, 1.0) : 0.0f;
        const float bloom = blink * blink;

        if (alwaysOn)
        {
            const auto outerGlow = led.expanded(14.0f);
            juce::ColourGradient outer(juce::Colour(0xffff0030).withAlpha(0.14f * bloom), litCentre.x, litCentre.y,
                                       juce::Colours::transparentBlack, outerGlow.getRight(), centre.y, true);
            outer.addColour(0.38, juce::Colour(0xffff1748).withAlpha(0.11f * bloom));
            outer.addColour(0.68, juce::Colour(0xff840012).withAlpha(0.04f * bloom));
            g.setGradientFill(outer);
            g.fillEllipse(outerGlow);

            const auto innerGlow = led.expanded(5.5f);
            juce::ColourGradient glassGlow(juce::Colour(0xffff4d6f).withAlpha(0.36f * bloom), litCentre.x, litCentre.y,
                                           juce::Colours::transparentBlack, innerGlow.getRight(), centre.y, true);
            glassGlow.addColour(0.34, juce::Colour(0xffff002c).withAlpha(0.28f * bloom));
            glassGlow.addColour(0.72, juce::Colour(0xff7b0011).withAlpha(0.10f * bloom));
            g.setGradientFill(glassGlow);
            g.fillEllipse(innerGlow);
        }

        juce::ColourGradient lensBase(alwaysOn ? juce::Colour(0xffcf0029).withMultipliedAlpha(0.72f + 0.28f * blink)
                                               : juce::Colour(0xff33030b),
                                      centre.x - led.getWidth() * 0.32f, led.getY(),
                                      alwaysOn ? juce::Colour(0xff250005) : juce::Colour(0xff0d0103),
                                      centre.x + led.getWidth() * 0.32f, led.getBottom(), false);
        lensBase.addColour(0.20, alwaysOn ? juce::Colour(0xffff4568).withMultipliedAlpha(0.58f + 0.42f * blink)
                                          : juce::Colour(0xff4a0610));
        lensBase.addColour(0.50, alwaysOn ? juce::Colour(0xffb90022).withMultipliedAlpha(0.82f + 0.18f * blink)
                                          : juce::Colour(0xff210207));
        lensBase.addColour(0.86, alwaysOn ? juce::Colour(0xff2c0006) : juce::Colour(0xff090102));
        g.setGradientFill(lensBase);
        g.fillEllipse(led);

        const auto glassDome = led.reduced(1.8f).translated(-led.getWidth() * 0.06f, -led.getHeight() * 0.06f);
        juce::ColourGradient dome(alwaysOn ? juce::Colour(0xffffb8c7).withAlpha(0.30f + 0.14f * blink)
                                           : juce::Colour(0xff7b1724).withAlpha(0.18f),
                                  glassDome.getX(), glassDome.getY(),
                                  juce::Colours::transparentBlack,
                                  glassDome.getRight(), glassDome.getBottom(), false);
        dome.addColour(0.42, alwaysOn ? juce::Colour(0xffff3156).withAlpha(0.10f + 0.08f * blink)
                                      : juce::Colour(0xff2a0308).withAlpha(0.10f));
        g.setGradientFill(dome);
        g.fillEllipse(glassDome);

        if (alwaysOn)
        {
            const auto caustic = led.reduced(5.2f).translated(led.getWidth() * 0.10f, led.getHeight() * 0.08f);
            juce::ColourGradient innerFire(juce::Colour(0xfffff4f7).withAlpha(0.58f * blink), caustic.getCentreX(), caustic.getCentreY(),
                                           juce::Colour(0xffff0030).withAlpha(0.12f * blink),
                                           caustic.getRight(), caustic.getCentreY(), true);
            innerFire.addColour(0.38, juce::Colour(0xffff2d52).withAlpha(0.24f * blink));
            g.setGradientFill(innerFire);
            g.fillEllipse(caustic.expanded(1.2f, 0.7f));
        }

        g.setColour(juce::Colours::black.withAlpha(alwaysOn ? 0.40f : 0.68f));
        g.drawEllipse(led.reduced(0.55f), 1.2f);
        g.setColour(juce::Colours::white.withAlpha(alwaysOn ? 0.20f : 0.08f));
        g.drawEllipse(led.reduced(1.8f), 0.8f);

        if (alwaysOn)
        {
            const auto shine = led.withSizeKeepingCentre(led.getWidth() * 0.44f, led.getHeight() * 0.18f)
                                .translated(-led.getWidth() * 0.14f, -led.getHeight() * 0.24f);
            juce::ColourGradient specular(juce::Colours::white.withAlpha(0.84f),
                                          shine.getX(), shine.getY(),
                                          juce::Colours::white.withAlpha(0.04f),
                                          shine.getRight(), shine.getBottom(), false);
            g.setGradientFill(specular);
            g.fillEllipse(shine);

            g.setColour(juce::Colours::white.withAlpha(0.28f));
            g.fillEllipse(led.withSizeKeepingCentre(led.getWidth() * 0.13f, led.getHeight() * 0.10f)
                              .translated(led.getWidth() * 0.12f, -led.getHeight() * 0.10f));
        }
    }

    {
        const float madnessAmount = (float)madness.valueToProportionOfLength(madness.getValue());
        const float intensity = juce::jlimit(0.0f, 1.0f, madnessAmount);
        const auto led = ref(1110.0f, 125.0f, 24.0f, 24.0f).toFloat();
        const auto centre = led.getCentre();
        const auto litCentre = centre.translated(led.getWidth() * 0.04f, led.getHeight() * 0.08f);
        const double rateHz = 0.8 + (double)intensity * 8.2;
        const double blinkPhase = std::sin(juce::Time::getMillisecondCounterHiRes()
                                           * juce::MathConstants<double>::twoPi * rateHz / 1000.0);
        const float blink = (float)juce::jmap(blinkPhase, -1.0, 1.0, 0.06, 1.0);
        const float energy = std::pow(intensity, 1.35f);
        const float bloom = energy * std::pow(blink, 1.7f);

        if (intensity > 0.01f)
        {
            const auto outerGlow = led.expanded(8.0f + 28.0f * energy);
            juce::ColourGradient outer(juce::Colour(0xffff0030).withAlpha(0.30f * bloom), litCentre.x, litCentre.y,
                                       juce::Colours::transparentBlack, outerGlow.getRight(), centre.y, true);
            outer.addColour(0.24, juce::Colour(0xffff2a55).withAlpha(0.24f * bloom));
            outer.addColour(0.58, juce::Colour(0xffa6001a).withAlpha(0.10f * bloom));
            g.setGradientFill(outer);
            g.fillEllipse(outerGlow);

            const auto innerGlow = led.expanded(3.0f + 12.0f * energy);
            juce::ColourGradient glassGlow(juce::Colour(0xffff6f88).withAlpha(0.46f * bloom), litCentre.x, litCentre.y,
                                           juce::Colours::transparentBlack, innerGlow.getRight(), centre.y, true);
            glassGlow.addColour(0.30, juce::Colour(0xffff0033).withAlpha(0.36f * bloom));
            glassGlow.addColour(0.72, juce::Colour(0xff750010).withAlpha(0.14f * bloom));
            g.setGradientFill(glassGlow);
            g.fillEllipse(innerGlow);
        }

        const float lensLift = 0.16f + 0.84f * energy * (0.30f + 0.70f * blink);
        juce::ColourGradient lensBase(juce::Colour(0xffad001f).withMultipliedAlpha(0.54f + 0.46f * lensLift),
                                      centre.x - led.getWidth() * 0.34f, led.getY(),
                                      juce::Colour(0xff210004),
                                      centre.x + led.getWidth() * 0.34f, led.getBottom(), false);
        lensBase.addColour(0.18, juce::Colour(0xffff5a76).withAlpha(0.14f + 0.50f * lensLift));
        lensBase.addColour(0.48, juce::Colour(0xffd80028).withAlpha(0.44f + 0.44f * lensLift));
        lensBase.addColour(0.84, juce::Colour(0xff270006));
        g.setGradientFill(lensBase);
        g.fillEllipse(led);

        const auto glassDome = led.reduced(2.0f).translated(-led.getWidth() * 0.06f, -led.getHeight() * 0.06f);
        juce::ColourGradient dome(juce::Colour(0xffffd5df).withAlpha(0.20f + 0.24f * lensLift),
                                  glassDome.getX(), glassDome.getY(),
                                  juce::Colours::transparentBlack,
                                  glassDome.getRight(), glassDome.getBottom(), false);
        dome.addColour(0.44, juce::Colour(0xffff3156).withAlpha(0.07f + 0.16f * lensLift));
        g.setGradientFill(dome);
        g.fillEllipse(glassDome);

        if (intensity > 0.01f)
        {
            const auto caustic = led.reduced(5.5f).translated(led.getWidth() * 0.10f, led.getHeight() * 0.10f);
            juce::ColourGradient innerFire(juce::Colour(0xfffffbfd).withAlpha(0.68f * bloom), caustic.getCentreX(), caustic.getCentreY(),
                                           juce::Colour(0xffff0033).withAlpha(0.18f * bloom),
                                           caustic.getRight(), caustic.getCentreY(), true);
            innerFire.addColour(0.38, juce::Colour(0xffff2d52).withAlpha(0.30f * bloom));
            g.setGradientFill(innerFire);
            g.fillEllipse(caustic.expanded(1.4f, 0.8f));
        }

        g.setColour(juce::Colours::black.withAlpha(0.38f));
        g.drawEllipse(led.reduced(0.55f), 1.25f);
        g.setColour(juce::Colours::white.withAlpha(0.12f + 0.16f * lensLift));
        g.drawEllipse(led.reduced(2.0f), 0.8f);

        const auto shine = led.withSizeKeepingCentre(led.getWidth() * 0.44f, led.getHeight() * 0.18f)
                            .translated(-led.getWidth() * 0.14f, -led.getHeight() * 0.24f);
        juce::ColourGradient specular(juce::Colours::white.withAlpha(0.64f + 0.20f * lensLift),
                                      shine.getX(), shine.getY(),
                                      juce::Colours::white.withAlpha(0.04f),
                                      shine.getRight(), shine.getBottom(), false);
        g.setGradientFill(specular);
        g.fillEllipse(shine);
    }

    {
        const auto b = modeToggle.getBounds().toFloat();
        const auto body = b.withSizeKeepingCentre(b.getWidth() * 0.88f, b.getHeight() * 0.58f)
                           .translated(0.0f, b.getHeight() * 0.02f);
        const auto slot = body.reduced(body.getHeight() * 0.18f, body.getHeight() * 0.22f);
        const float bodyRadius = body.getHeight() * 0.42f;
        const float slotRadius = slot.getHeight() * 0.50f;

        g.setColour(juce::Colours::black.withAlpha(0.36f));
        g.fillRoundedRectangle(body.translated(1.0f, 1.9f), bodyRadius);

        juce::ColourGradient rim(kChromeLight, body.getX(), body.getY(),
                                 kChromeDark, body.getRight(), body.getBottom(), false);
        rim.addColour(0.22, juce::Colour(0xff6d716d));
        rim.addColour(0.46, juce::Colour(0xfff4ead7));
        rim.addColour(0.72, juce::Colour(0xff353330));
        g.setGradientFill(rim);
        g.fillRoundedRectangle(body, bodyRadius);

        g.setColour(kInk.withAlpha(0.82f));
        g.drawRoundedRectangle(body.reduced(0.45f), bodyRadius, 1.1f);

        juce::ColourGradient slotFill(juce::Colour(0xff050403), slot.getX(), slot.getY(),
                                      juce::Colour(0xff2b2119), slot.getRight(), slot.getBottom(), false);
        slotFill.addColour(0.55, juce::Colour(0xff0d0a08));
        g.setGradientFill(slotFill);
        g.fillRoundedRectangle(slot, slotRadius);

        const float ballDiam = body.getHeight() * 0.58f;
        const float margin = body.getHeight() * 0.20f;
        const float ballX = alwaysOn
            ? body.getRight() - margin - ballDiam
            : body.getX() + margin;
        const auto ball = juce::Rectangle<float>(ballX,
                                                 body.getCentreY() - ballDiam * 0.5f,
                                                 ballDiam, ballDiam);
        const auto centre = ball.getCentre();

        g.setColour(juce::Colours::black.withAlpha(0.38f));
        g.fillEllipse(ball.expanded(0.8f).translated(0.8f, 1.2f));

        juce::ColourGradient chrome(juce::Colour(0xfffff8e8), centre.x - ballDiam * 0.36f, ball.getY(),
                                    juce::Colour(0xff34312d), centre.x + ballDiam * 0.34f, ball.getBottom(), false);
        chrome.addColour(0.26, kChromeMid);
        chrome.addColour(0.56, juce::Colour(0xfffaf2df));
        chrome.addColour(0.78, juce::Colour(0xff666761));
        g.setGradientFill(chrome);
        g.fillEllipse(ball);

        g.setColour(juce::Colours::white.withAlpha(0.34f));
        g.fillEllipse(ball.withSizeKeepingCentre(ballDiam * 0.30f, ballDiam * 0.18f)
                          .translated(-ballDiam * 0.14f, -ballDiam * 0.17f));

        g.setColour(kInk.withAlpha(0.88f));
        g.drawEllipse(ball, 1.2f);
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
