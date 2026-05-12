#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float kMinPulseWidth = 0.08f;
constexpr float kMaxPulseWidth = 0.92f;

juce::NormalisableRange<float> skewedRange(float start, float end, float centre)
{
    juce::NormalisableRange<float> range(start, end);
    range.setSkewForCentre(centre);
    return range;
}

float dbToGain(float db)
{
    return juce::Decibels::decibelsToGain(db);
}

float currentRootFrequency(bool alwaysOn, float currentMidiNote, float tuneSemis, int octaveChoice)
{
    const float octaveSemis = static_cast<float>(octaveChoice - 2) * 12.0f;
    const float baseNote = alwaysOn ? 36.0f : currentMidiNote;
    const float noteNumber = baseNote + tuneSemis + octaveSemis;
    return 440.0f * std::pow(2.0f, (noteNumber - 69.0f) / 12.0f);
}
}

juce::AudioProcessorValueTreeState::ParameterLayout PWMMadnessAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode",
        juce::StringArray { "MIDI", "Always On" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("tune", "Tune", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("octave", "Octave",
        juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
        skewedRange(0.001f, 5.0f, 0.08f), 0.02f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release",
        skewedRange(0.01f, 12.0f, 0.60f), 1.20f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc1PW", "OSC 1 PW", 0.10f, 0.90f, 0.46f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc2PW", "OSC 2 PW", 0.10f, 0.90f, 0.54f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("osc2Detune", "OSC 2 Detune", -25.0f, 25.0f, 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("oscMix", "OSC Mix", 0.0f, 1.0f, 0.50f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("subLevel", "Sub Level", 0.0f, 1.0f, 0.35f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("pwmDepth", "PWM Depth", 0.0f, 0.42f, 0.16f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("pwmRate", "PWM Rate",
        skewedRange(0.01f, 10.0f, 0.35f), 0.18f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterCutoff", "Filter Cutoff",
        skewedRange(35.0f, 12000.0f, 420.0f), 900.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filterResonance", "Filter Resonance", 0.0f, 0.92f, 0.18f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("drive", "Drive",
        skewedRange(1.0f, 12.0f, 2.5f), 1.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output", -24.0f, 6.0f, -6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("madness", "Madness", 0.0f, 1.0f, 0.25f));

    return { params.begin(), params.end() };
}

PWMMadnessAudioProcessor::PWMMadnessAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PWMMadness", createParameterLayout())
{
}

void PWMMadnessAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    osc1.reset();
    osc2.reset();
    subOsc.reset();
    ampEnvelope.prepare(sampleRate);
    ampEnvelope.reset();
    decimator.reset();
    lfoPhase = 0.0f;
    dcX1 = 0.0f;
    dcY1 = 0.0f;
    resetSmoothedValues(sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    lowpass.prepare(spec);
    lowpass.reset();
    lowpass.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

bool PWMMadnessAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

float PWMMadnessAudioProcessor::readFloat(const char* id) const
{
    if (auto* value = apvts.getRawParameterValue(id))
        return value->load();
    return 0.0f;
}

int PWMMadnessAudioProcessor::readChoice(const char* id) const
{
    return static_cast<int>(std::round(readFloat(id)));
}

float PWMMadnessAudioProcessor::midiNoteToHz(float noteNumber)
{
    return 440.0f * std::pow(2.0f, (noteNumber - 69.0f) / 12.0f);
}

float PWMMadnessAudioProcessor::onePoleHighpass(float input, float& x1, float& y1, float coefficient)
{
    const float y = coefficient * (y1 + input - x1);
    x1 = input;
    y1 = y;
    return y;
}

float PWMMadnessAudioProcessor::fastTanh(float x) noexcept
{
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

float PWMMadnessAudioProcessor::polyBlep(float phase, float phaseStep)
{
    if (phaseStep <= 0.0f)
        return 0.0f;

    if (phase < phaseStep)
    {
        phase /= phaseStep;
        return phase + phase - phase * phase - 1.0f;
    }

    if (phase > 1.0f - phaseStep)
    {
        phase = (phase - 1.0f) / phaseStep;
        return phase * phase + phase + phase + 1.0f;
    }

    return 0.0f;
}

float PWMMadnessAudioProcessor::PulseOscillator::process(float frequency, float duty, float sampleRate)
{
    const float phaseStep = juce::jlimit(0.0f, 0.5f, frequency / sampleRate);
    phase += phaseStep;
    phase -= std::floor(phase);

    float sample = phase < duty ? 1.0f : -1.0f;
    sample += PWMMadnessAudioProcessor::polyBlep(phase, phaseStep);

    float fallingEdgePhase = phase - duty;
    if (fallingEdgePhase < 0.0f)
        fallingEdgePhase += 1.0f;
    sample -= PWMMadnessAudioProcessor::polyBlep(fallingEdgePhase, phaseStep);

    return sample;
}

void PWMMadnessAudioProcessor::resetSmoothedValues(double sampleRate)
{
    constexpr double kControlSmoothingSeconds = 0.010;
    constexpr double kPitchSmoothingSeconds = 0.004;

    smoothedRootFrequency.reset(sampleRate, kPitchSmoothingSeconds);
    smoothedOsc1PW.reset(sampleRate, kControlSmoothingSeconds);
    smoothedOsc2PW.reset(sampleRate, kControlSmoothingSeconds);
    smoothedOscMix.reset(sampleRate, kControlSmoothingSeconds);
    smoothedSubLevel.reset(sampleRate, kControlSmoothingSeconds);
    smoothedPwmDepth.reset(sampleRate, kControlSmoothingSeconds);
    smoothedPwmRate.reset(sampleRate, kControlSmoothingSeconds);
    smoothedAttack.reset(sampleRate, kControlSmoothingSeconds);
    smoothedRelease.reset(sampleRate, kControlSmoothingSeconds);
    smoothedDrive.reset(sampleRate, kControlSmoothingSeconds);
    smoothedOutputGain.reset(sampleRate, kControlSmoothingSeconds);
    smoothedCutoff.reset(sampleRate, kControlSmoothingSeconds);
    smoothedResonance.reset(sampleRate, kControlSmoothingSeconds);
    smoothedMadness.reset(sampleRate, kControlSmoothingSeconds);
    smoothedDetune.reset(sampleRate, kControlSmoothingSeconds);

    dcCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 35.0f / (float)sampleRate);

    const bool alwaysOn = readChoice("mode") == 1;
    const float rootFrequency = currentRootFrequency(alwaysOn, currentMidiNote, readFloat("tune"), readChoice("octave"));

    smoothedRootFrequency.setCurrentAndTargetValue(rootFrequency);
    smoothedOsc1PW.setCurrentAndTargetValue(readFloat("osc1PW"));
    smoothedOsc2PW.setCurrentAndTargetValue(readFloat("osc2PW"));
    smoothedOscMix.setCurrentAndTargetValue(readFloat("oscMix"));
    smoothedSubLevel.setCurrentAndTargetValue(readFloat("subLevel"));
    smoothedPwmDepth.setCurrentAndTargetValue(readFloat("pwmDepth"));
    smoothedPwmRate.setCurrentAndTargetValue(readFloat("pwmRate"));
    smoothedAttack.setCurrentAndTargetValue(readFloat("attack"));
    smoothedRelease.setCurrentAndTargetValue(readFloat("release"));
    smoothedDrive.setCurrentAndTargetValue(readFloat("drive"));
    smoothedOutputGain.setCurrentAndTargetValue(dbToGain(readFloat("outputGain")));
    smoothedCutoff.setCurrentAndTargetValue(readFloat("filterCutoff"));
    smoothedResonance.setCurrentAndTargetValue(readFloat("filterResonance"));
    smoothedMadness.setCurrentAndTargetValue(readFloat("madness"));
    smoothedDetune.setCurrentAndTargetValue(std::pow(2.0f, readFloat("osc2Detune") / 1200.0f));
}

void PWMMadnessAudioProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        currentMidiNote = static_cast<float>(message.getNoteNumber());
        gate = true;
        decimator.reset();
        ampEnvelope.noteOn();
    }
    else if (message.isNoteOff() && message.getNoteNumber() == juce::roundToInt(currentMidiNote))
    {
        gate = false;
        ampEnvelope.noteOff();
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        gate = false;
        ampEnvelope.noteOff();
    }
}

void PWMMadnessAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    const bool alwaysOn = readChoice("mode") == 1;
    if (alwaysOn && !ampEnvelope.isActive())
        ampEnvelope.noteOn();
    else if (!alwaysOn && !gate && ampEnvelope.active)
        ampEnvelope.noteOff();

    const float tuneSemis = readFloat("tune");
    const int octaveChoice = readChoice("octave");

    smoothedRootFrequency.setTargetValue(currentRootFrequency(alwaysOn, currentMidiNote, tuneSemis, octaveChoice));
    smoothedOsc1PW.setTargetValue(readFloat("osc1PW"));
    smoothedOsc2PW.setTargetValue(readFloat("osc2PW"));
    smoothedOscMix.setTargetValue(readFloat("oscMix"));
    smoothedSubLevel.setTargetValue(readFloat("subLevel"));
    smoothedPwmDepth.setTargetValue(readFloat("pwmDepth"));
    smoothedPwmRate.setTargetValue(readFloat("pwmRate"));
    smoothedAttack.setTargetValue(readFloat("attack"));
    smoothedRelease.setTargetValue(readFloat("release"));
    smoothedDrive.setTargetValue(readFloat("drive"));
    smoothedOutputGain.setTargetValue(dbToGain(readFloat("outputGain")));
    smoothedCutoff.setTargetValue(readFloat("filterCutoff"));
    smoothedResonance.setTargetValue(readFloat("filterResonance"));
    smoothedMadness.setTargetValue(readFloat("madness"));
    smoothedDetune.setTargetValue(std::pow(2.0f, readFloat("osc2Detune") / 1200.0f));

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    auto midiIterator = midiMessages.cbegin();
    const auto midiEnd = midiMessages.cend();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        while (midiIterator != midiEnd && (*midiIterator).samplePosition <= sample)
        {
            if (!alwaysOn)
            {
                handleMidiMessage((*midiIterator).getMessage());
                smoothedRootFrequency.setTargetValue(currentRootFrequency(false, currentMidiNote, tuneSemis, octaveChoice));
            }
            ++midiIterator;
        }

        const float madness = smoothedMadness.getNextValue();
        const float pwmDepth = juce::jlimit(0.0f, 0.46f, smoothedPwmDepth.getNextValue() + madness * 0.20f);
        const float pwmRate = smoothedPwmRate.getNextValue() * (1.0f + madness * 0.35f);

        lfoPhase += pwmRate / static_cast<float>(currentSampleRate);
        lfoPhase -= std::floor(lfoPhase);
        const float lfo = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);

        const float rootFrequency = smoothedRootFrequency.getNextValue();
        const float detuneRatio = smoothedDetune.getNextValue();
        const float duty1 = juce::jlimit(kMinPulseWidth, kMaxPulseWidth, smoothedOsc1PW.getNextValue() + lfo * pwmDepth);
        const float duty2 = juce::jlimit(kMinPulseWidth, kMaxPulseWidth, smoothedOsc2PW.getNextValue() - lfo * pwmDepth);

        const float oscMix = smoothedOscMix.getNextValue();
        const float subLevel = smoothedSubLevel.getNextValue();
        const float oscSampleRate = static_cast<float>(currentSampleRate) * 8.0f;
        for (int os = 0; os < 8; ++os)
        {
            const float o1 = osc1.process(rootFrequency, duty1, oscSampleRate);
            const float o2 = osc2.process(rootFrequency * detuneRatio, duty2, oscSampleRate);
            decimator.push(o1 * (1.0f - oscMix) + o2 * oscMix);
        }
        const float tone = decimator.read() * 0.72f;

        const float subSample = subOsc.process(rootFrequency * 0.5f, static_cast<float>(currentSampleRate));
        float mixed = tone + subSample * subLevel * 0.55f;

        const float drive = smoothedDrive.getNextValue() * (1.0f + madness * 1.8f);
        float driven = fastTanh(mixed * drive) / fastTanh(drive);

        const float cutoff = juce::jlimit(30.0f, 16000.0f,
            smoothedCutoff.getNextValue() * (1.0f + madness * 0.18f));
        const float resonance = juce::jlimit(0.0f, 0.96f,
            smoothedResonance.getNextValue() + madness * 0.10f);
        if ((sample & 7) == 0)
        {
            lowpass.setCutoffFrequency(cutoff);
            lowpass.setResonance(0.5f + resonance * 8.0f);
        }

        float filtered = lowpass.processSample(0, driven);
        const float attack = smoothedAttack.getNextValue();
        const float release = smoothedRelease.getNextValue();
        float env = ampEnvelope.process(attack, release);
        float out = filtered * env;
        out = onePoleHighpass(out, dcX1, dcY1, dcCoeff);
        out = fastTanh(out * 1.25f) * smoothedOutputGain.getNextValue();

        left[sample] = out;
        if (right != nullptr)
            right[sample] = out;
    }
}

void PWMMadnessAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void PWMMadnessAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

void PWMMadnessAudioProcessor::AttackReleaseEnvelope::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
}

void PWMMadnessAudioProcessor::AttackReleaseEnvelope::reset()
{
    value = 0.0f;
    active = false;
}

void PWMMadnessAudioProcessor::AttackReleaseEnvelope::noteOn()
{
    active = true;
}

void PWMMadnessAudioProcessor::AttackReleaseEnvelope::noteOff()
{
    active = false;
}

float PWMMadnessAudioProcessor::AttackReleaseEnvelope::process(float attackSeconds, float releaseSeconds)
{
    const float target = active ? 1.0f : 0.0f;
    if (attackSeconds != cachedAttackTime)
    {
        cachedAttackTime = attackSeconds;
        attackCoeff = 1.0f - std::exp(-1.0f / (juce::jmax(0.001f, attackSeconds) * static_cast<float>(sampleRate)));
    }
    if (releaseSeconds != cachedReleaseTime)
    {
        cachedReleaseTime = releaseSeconds;
        releaseCoeff = 1.0f - std::exp(-1.0f / (juce::jmax(0.001f, releaseSeconds) * static_cast<float>(sampleRate)));
    }
    const float coeff = active ? attackCoeff : releaseCoeff;
    value += (target - value) * coeff;
    if (!active && value < 0.00001f)
        value = 0.0f;
    return value;
}

juce::AudioProcessorEditor* PWMMadnessAudioProcessor::createEditor()
{
    return new PWMMadnessAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PWMMadnessAudioProcessor();
}
