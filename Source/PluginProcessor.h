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
    struct OscDecimator
    {
        static constexpr int kTaps = 31;

        void reset() { history.fill(0.0f); index = 0; }

        void push(float sample)
        {
            history[(size_t)index] = sample;
            index = (index + 1) % kTaps;
        }

        float read() const
        {
            float out = 0.0f;
            int hi = index;
            for (int t = 0; t < kTaps; ++t)
            {
                hi = (hi - 1 + kTaps) % kTaps;
                out += kCoeffs[(size_t)t] * history[(size_t)hi];
            }
            return out;
        }

        std::array<float, kTaps> history {};
        int index = 0;

        inline static constexpr std::array<float, kTaps> kCoeffs
        {
            -0.0006479604f, -0.0014439791f, -0.0027022580f, -0.0044407449f,
            -0.0061914846f, -0.0069591594f, -0.0053706761f,  0.0000000000f,
             0.0102068186f,  0.0255224468f,  0.0451695882f,  0.0672889128f,
             0.0891803950f,  0.1077806354f,  0.1202713177f,  0.1246722937f,
             0.1202713177f,  0.1077806354f,  0.0891803950f,  0.0672889128f,
             0.0451695882f,  0.0255224468f,  0.0102068186f,  0.0000000000f,
            -0.0053706761f, -0.0069591594f, -0.0061914846f, -0.0044407449f,
            -0.0027022580f, -0.0014439791f, -0.0006479604f
        };
    };

    struct PulseOscillator
    {
        void reset() { phase = 0.0f; }
        float process(float frequency, float duty, float sampleRate);

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

    private:
        float cachedAttackTime = -1.0f;
        float cachedReleaseTime = -1.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
    };

    float readFloat(const char* id) const;
    int readChoice(const char* id) const;
    void handleMidiMessage(const juce::MidiMessage& message);
    static float midiNoteToHz(float noteNumber);
    static float onePoleHighpass(float input, float& x1, float& y1, float coefficient);
    static float polyBlep(float phase, float phaseStep);
    static float fastTanh(float x) noexcept;
    void resetSmoothedValues(double sampleRate);

    double currentSampleRate = 44100.0;
    PulseOscillator osc1;
    PulseOscillator osc2;
    SineOscillator subOsc;
    SineOscillator superSubOsc;
    AttackReleaseEnvelope ampEnvelope;
    juce::dsp::StateVariableTPTFilter<float> lowpass;

    OscDecimator decimator;
    float lfoPhase = 0.0f;
    float superSubFreqMult = 1.0f;
    int sAndHCounter = 0;
    juce::Random random;
    float currentMidiNote = 36.0f;
    bool gate = false;
    float dcX1 = 0.0f;
    float dcY1 = 0.0f;
    float dcCoeff = 0.995f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedRootFrequency;
    juce::SmoothedValue<float> smoothedOsc1PW;
    juce::SmoothedValue<float> smoothedOsc2PW;
    juce::SmoothedValue<float> smoothedOscMix;
    juce::SmoothedValue<float> smoothedSubLevel;
    juce::SmoothedValue<float> smoothedPwmDepth;
    juce::SmoothedValue<float> smoothedPwmRate;
    juce::SmoothedValue<float> smoothedAttack;
    juce::SmoothedValue<float> smoothedRelease;
    juce::SmoothedValue<float> smoothedDrive;
    juce::SmoothedValue<float> smoothedOutputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedCutoff;
    juce::SmoothedValue<float> smoothedResonance;
    juce::SmoothedValue<float> smoothedMadness;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedDetune;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PWMMadnessAudioProcessor)
};
