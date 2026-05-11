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
    lfoPhase = 0.0f;
    dcX1 = 0.0f;
    dcY1 = 0.0f;

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

void PWMMadnessAudioProcessor::handleMidi(const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            currentMidiNote = static_cast<float>(message.getNoteNumber());
            gate = true;
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
}

void PWMMadnessAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    handleMidi(midiMessages);

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
    const float octaveSemis = static_cast<float>(octaveChoice - 2) * 12.0f;
    const float baseNote = alwaysOn ? 36.0f : currentMidiNote;
    const float rootFrequency = midiNoteToHz(baseNote + tuneSemis + octaveSemis);
    const float detuneRatio = std::pow(2.0f, readFloat("osc2Detune") / 1200.0f);

    const float oscMix = readFloat("oscMix");
    const float subLevel = readFloat("subLevel");
    const float madness = readFloat("madness");
    const float pwmDepth = juce::jlimit(0.0f, 0.46f, readFloat("pwmDepth") + madness * 0.20f);
    const float pwmRate = readFloat("pwmRate") * (1.0f + madness * 0.35f);
    const float attack = readFloat("attack");
    const float release = readFloat("release");
    const float drive = readFloat("drive") * (1.0f + madness * 1.8f);
    const float output = dbToGain(readFloat("outputGain"));

    const float cutoff = juce::jlimit(30.0f, 16000.0f,
        readFloat("filterCutoff") * (1.0f + madness * 0.18f));
    const float resonance = juce::jlimit(0.0f, 0.96f, readFloat("filterResonance") + madness * 0.10f);
    lowpass.setCutoffFrequency(cutoff);
    lowpass.setResonance(0.5f + resonance * 8.0f);

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        lfoPhase += pwmRate / static_cast<float>(currentSampleRate);
        lfoPhase -= std::floor(lfoPhase);
        const float lfo = std::sin(lfoPhase * juce::MathConstants<float>::twoPi);

        const float duty1 = juce::jlimit(kMinPulseWidth, kMaxPulseWidth, readFloat("osc1PW") + lfo * pwmDepth);
        const float duty2 = juce::jlimit(kMinPulseWidth, kMaxPulseWidth, readFloat("osc2PW") - lfo * pwmDepth);

        const float osc1Sample = osc1.process(rootFrequency, duty1, static_cast<float>(currentSampleRate));
        const float osc2Sample = osc2.process(rootFrequency * detuneRatio, duty2, static_cast<float>(currentSampleRate));
        const float subSample = subOsc.process(rootFrequency * 0.5f, static_cast<float>(currentSampleRate));

        float mixed = osc1Sample * (1.0f - oscMix) + osc2Sample * oscMix;
        mixed = (mixed * 0.72f) + (subSample * subLevel * 0.55f);

        float driven = std::tanh(mixed * drive) / std::tanh(drive);
        float filtered = lowpass.processSample(0, driven);
        float env = ampEnvelope.process(attack, release);
        float out = filtered * env;
        out = onePoleHighpass(out, dcX1, dcY1, 0.995f);
        out = std::tanh(out * 1.25f) * output;

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
    const float time = active ? attackSeconds : releaseSeconds;
    const float coefficient = 1.0f - std::exp(-1.0f / (juce::jmax(0.001f, time) * static_cast<float>(sampleRate)));
    value += (target - value) * coefficient;
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
