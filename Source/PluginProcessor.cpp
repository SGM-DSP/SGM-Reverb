#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto mixId = "mix";
constexpr auto predelayId = "predelay";
constexpr auto sizeId = "size";
constexpr auto decayId = "decay";
constexpr auto diffusionId = "diffusion";
constexpr auto densityId = "density";
constexpr auto dampingId = "damping";
constexpr auto lowCutId = "lowCut";
constexpr auto highCutId = "highCut";
constexpr auto widthId = "width";
constexpr auto modulationId = "modulation";
constexpr auto duckingId = "ducking";
constexpr auto shimmerId = "shimmer";
constexpr auto freezeId = "freeze";

float value (const juce::AudioProcessorValueTreeState& state, const char* id)
{
    return state.getRawParameterValue (id)->load();
}
}

SGMReverbAudioProcessor::SGMReverbAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SGMReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto percent = [] (float value, int) { return juce::String (juce::roundToInt (value * 100.0f)) + "%"; };
    auto ms = [] (float value, int) { return juce::String (value, 1) + " ms"; };
    auto hz = [] (float value, int) { return value >= 1000.0f ? juce::String (value / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (value)) + " Hz"; };

    params.push_back (std::make_unique<juce::AudioParameterFloat> (mixId, "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (predelayId, "Predelay", juce::NormalisableRange<float> (0.0f, 250.0f, 0.1f), 25.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, ms));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (sizeId, "Size", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.72f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (decayId, "Decay", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.72f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (diffusionId, "Diffusion", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.76f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (densityId, "Density", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.78f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (dampingId, "Damping", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (lowCutId, "Low Cut", juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.35f), 120.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, hz));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (highCutId, "High Cut", juce::NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.45f), 12000.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, hz));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (widthId, "Width", juce::NormalisableRange<float> (0.0f, 1.8f, 0.001f), 1.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (modulationId, "Modulation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (duckingId, "Ducking", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (shimmerId, "Shimmer", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, juce::String(), juce::AudioProcessorParameter::genericParameter, percent));
    params.push_back (std::make_unique<juce::AudioParameterBool> (freezeId, "Freeze", false));

    return { params.begin(), params.end() };
}

void SGMReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    lastTransportSample.reset();
    wasPlaying = false;
}

void SGMReverbAudioProcessor::releaseResources()
{
}

bool SGMReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainIn == mainOut && (mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo());
}

void SGMReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (shouldResetReverbForTransport (buffer.getNumSamples()))
        engine.reset();

    engine.process (buffer, readParameters());
}

bool SGMReverbAudioProcessor::shouldResetReverbForTransport (int numSamples)
{
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            const auto isPlaying = position->getIsPlaying();

            if (auto timeInSamples = position->getTimeInSamples())
            {
                bool shouldReset = false;

                if (isPlaying && ! wasPlaying)
                    shouldReset = true;

                if (isPlaying && lastTransportSample.has_value())
                {
                    const auto expected = *lastTransportSample + static_cast<juce::int64> (numSamples);
                    const auto tolerance = juce::jmax<juce::int64> (static_cast<juce::int64> (numSamples * 4),
                                                                    static_cast<juce::int64> (getSampleRate() * 0.05));

                    if (std::abs (*timeInSamples - expected) > tolerance)
                        shouldReset = true;
                }

                lastTransportSample = *timeInSamples;
                wasPlaying = isPlaying;
                return shouldReset;
            }

            if (! isPlaying)
                lastTransportSample.reset();

            wasPlaying = isPlaying;
        }
    }

    return false;
}

ReverbParameters SGMReverbAudioProcessor::readParameters() const
{
    ReverbParameters p;
    p.mix = value (apvts, mixId);
    p.predelayMs = value (apvts, predelayId);
    p.size = value (apvts, sizeId);
    p.decay = value (apvts, decayId);
    p.diffusion = value (apvts, diffusionId);
    p.density = value (apvts, densityId);
    p.damping = value (apvts, dampingId);
    p.lowCutHz = value (apvts, lowCutId);
    p.highCutHz = value (apvts, highCutId);
    p.width = value (apvts, widthId);
    p.modulation = value (apvts, modulationId);
    p.ducking = value (apvts, duckingId);
    p.shimmer = value (apvts, shimmerId);
    p.freeze = value (apvts, freezeId) > 0.5f;
    return p;
}

void SGMReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SGMReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SGMReverbAudioProcessor::createEditor()
{
    return new SGMReverbAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SGMReverbAudioProcessor();
}
