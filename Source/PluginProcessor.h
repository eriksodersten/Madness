#pragma once

#include <JuceHeader.h>

class PWMMadnessAudioProcessor : public juce::AudioProcessor
{
public:
    PWMMadnessAudioProcessor();
    ~PWMMadnessAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PWM Madness"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct PulseOscillator
    {
        void reset() { phase = 0.0f; }
        float process(float frequency, float duty, float sampleRate)
        {
            phase += frequency / sampleRate;
            phase -= std::floor(phase);
            return phase < duty ? 1.0f : -1.0f;
        }
        float phase = 0.0f;
    };

    struct SineOscillator
    {
        void reset() { phase = 0.0f; }
        float process(float frequency, float sampleRate)
        {
            phase += frequency / sampleRate;
            phase -= std::floor(phase);
            return std::sin(phase * juce::MathConstants<float>::twoPi);
        }
        float phase = 0.0f;
    };

    struct AttackReleaseEnvelope
    {
        void prepare(double newSampleRate);
        void reset();
        void noteOn();
        void noteOff();
        float process(float attackSeconds, float releaseSeconds);
        bool isActive() const { return active || value > 0.0001f; }

        double sampleRate = 44100.0;
        float value = 0.0f;
        bool active = false;
    };

    float readFloat(const char* id) const;
    int readChoice(const char* id) const;
    void handleMidi(const juce::MidiBuffer& midiMessages);
    static float midiNoteToHz(float noteNumber);
    static float onePoleHighpass(float input, float& x1, float& y1, float coefficient);

    double currentSampleRate = 44100.0;
    PulseOscillator osc1;
    PulseOscillator osc2;
    SineOscillator subOsc;
    AttackReleaseEnvelope ampEnvelope;
    juce::dsp::StateVariableTPTFilter<float> lowpass;

    float lfoPhase = 0.0f;
    float currentMidiNote = 36.0f;
    bool gate = false;
    float dcX1 = 0.0f;
    float dcY1 = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PWMMadnessAudioProcessor)
};
