#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MadnessSlider : public juce::Slider
{
public:
    explicit MadnessSlider(bool largeKnob = false) : large(largeKnob) {}
    bool large = false;
};

class PWMMadnessLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PWMMadnessLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
};

class PWMMadnessAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit PWMMadnessAudioProcessorEditor(PWMMadnessAudioProcessor&);
    ~PWMMadnessAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupSlider(MadnessSlider&);
    juce::Rectangle<int> ref(float x, float y, float w, float h) const;
    void timerCallback() override;

    PWMMadnessAudioProcessor& audioProcessor;
    PWMMadnessLookAndFeel lookAndFeel;
    juce::Image background;

    juce::ToggleButton modeToggle;
    MadnessSlider octave;

    MadnessSlider tune;
    MadnessSlider attack;
    MadnessSlider release;
    MadnessSlider osc1PW;
    MadnessSlider osc2PW;
    MadnessSlider osc2Detune;
    MadnessSlider oscMix;
    MadnessSlider subLevel;
    MadnessSlider pwmDepth;
    MadnessSlider pwmRate;
    MadnessSlider filterCutoff;
    MadnessSlider filterResonance;
    MadnessSlider drive;
    MadnessSlider outputGain;
    MadnessSlider madness { true };

    std::unique_ptr<ButtonAttachment> modeAttachment;
    std::unique_ptr<SliderAttachment> octaveAttachment;
    std::unique_ptr<SliderAttachment> tuneAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> osc1PWAttachment;
    std::unique_ptr<SliderAttachment> osc2PWAttachment;
    std::unique_ptr<SliderAttachment> osc2DetuneAttachment;
    std::unique_ptr<SliderAttachment> oscMixAttachment;
    std::unique_ptr<SliderAttachment> subLevelAttachment;
    std::unique_ptr<SliderAttachment> pwmDepthAttachment;
    std::unique_ptr<SliderAttachment> pwmRateAttachment;
    std::unique_ptr<SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> madnessAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PWMMadnessAudioProcessorEditor)
};
