#include "ReverbEngine.h"

void SGMReverbEngine::DelayLine::prepare (int newMaximumSamples)
{
    data.setSize (1, juce::jmax (4, newMaximumSamples + 4));
    reset();
}

void SGMReverbEngine::DelayLine::reset()
{
    data.clear();
    writeIndex = 0;
}

void SGMReverbEngine::DelayLine::push (float sample)
{
    data.setSample (0, writeIndex, sample);
    writeIndex = (writeIndex + 1) % data.getNumSamples();
}

float SGMReverbEngine::DelayLine::read (float delaySamples) const
{
    const auto size = data.getNumSamples();
    const auto clampedDelay = juce::jlimit (1.0f, static_cast<float> (size - 3), delaySamples);
    auto readPosition = static_cast<float> (writeIndex) - clampedDelay;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (size);

    const auto index0 = static_cast<int> (readPosition) % size;
    const auto index1 = (index0 + 1) % size;
    const auto fraction = readPosition - std::floor (readPosition);

    return juce::jmap (fraction, data.getSample (0, index0), data.getSample (0, index1));
}

void SGMReverbEngine::prepare (double newSampleRate, int maxBlockSize, int)
{
    sampleRate = newSampleRate;
    blockSize = maxBlockSize;

    const auto maxPredelaySamples = static_cast<int> (sampleRate * 0.55);
    for (auto& delay : predelay)
        delay.prepare (maxPredelaySamples);

    wetBuffer.setSize (2, juce::jmax (1, blockSize));
    reverb.setSampleRate (sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 1 };
    lowCutL.prepare (spec);
    lowCutR.prepare (spec);
    highCutL.prepare (spec);
    highCutR.prepare (spec);

    lowCutL.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lowCutR.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highCutL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    highCutR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    reset();
}

void SGMReverbEngine::reset()
{
    for (auto& delay : predelay)
        delay.reset();

    wetBuffer.clear();
    reverb.reset();
    envelope = 0.0f;
    lowCutL.reset();
    lowCutR.reset();
    highCutL.reset();
    highCutR.reset();
}

void SGMReverbEngine::process (juce::AudioBuffer<float>& buffer, const ReverbParameters& parameters)
{
    const auto numSamples = buffer.getNumSamples();
    const auto hasRight = buffer.getNumChannels() > 1;

    if (wetBuffer.getNumSamples() < numSamples)
        wetBuffer.setSize (2, numSamples, false, false, true);

    juce::Reverb::Parameters reverbParameters;
    reverbParameters.roomSize = juce::jlimit (0.0f, 1.0f, 0.18f + parameters.size * 0.46f + parameters.decay * 0.36f);
    reverbParameters.damping = juce::jlimit (0.0f, 1.0f, parameters.damping);
    const auto density = juce::jlimit (0.0f, 1.0f, parameters.density);
    const auto diffusion = juce::jlimit (0.0f, 1.0f, parameters.diffusion);
    reverbParameters.wetLevel = juce::jmap (diffusion * (0.65f + density * 0.35f), 0.10f, 0.24f);
    reverbParameters.dryLevel = 0.0f;
    reverbParameters.width = juce::jlimit (0.0f, 1.0f, parameters.width / 1.4f);
    reverbParameters.freezeMode = parameters.freeze ? 1.0f : 0.0f;
    reverb.setParameters (reverbParameters);

    lowCutL.setCutoffFrequency (parameters.lowCutHz);
    lowCutR.setCutoffFrequency (parameters.lowCutHz);
    highCutL.setCutoffFrequency (parameters.highCutHz);
    highCutR.setCutoffFrequency (parameters.highCutHz);

    const auto predelaySamples = parameters.predelayMs * 0.001f * static_cast<float> (sampleRate);
    const auto shimmerAir = juce::jlimit (0.0f, 1.0f, parameters.shimmer) * 0.18f;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto dryL = buffer.getSample (0, i);
        const auto dryR = hasRight ? buffer.getSample (1, i) : dryL;
        const auto input = 0.5f * (std::abs (dryL) + std::abs (dryR));
        envelope += (input - envelope) * (input > envelope ? 0.004f : 0.0007f);

        const auto wetInL = predelay[0].read (predelaySamples);
        const auto wetInR = hasRight ? predelay[1].read (predelaySamples) : wetInL;
        predelay[0].push (dryL + (dryL - dryR) * shimmerAir);
        predelay[1].push (dryR + (dryR - dryL) * shimmerAir);

        wetBuffer.setSample (0, i, wetInL);
        wetBuffer.setSample (1, i, wetInR);
    }

    if (hasRight)
        reverb.processStereo (wetBuffer.getWritePointer (0), wetBuffer.getWritePointer (1), numSamples);
    else
        reverb.processMono (wetBuffer.getWritePointer (0), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto dryL = buffer.getSample (0, i);
        const auto dryR = hasRight ? buffer.getSample (1, i) : dryL;

        auto wetL = wetBuffer.getSample (0, i);
        auto wetR = hasRight ? wetBuffer.getSample (1, i) : wetL;

        wetL = highCutL.processSample (0, lowCutL.processSample (0, wetL));
        wetR = highCutR.processSample (0, lowCutR.processSample (0, wetR));

        const auto mid = 0.5f * (wetL + wetR);
        const auto side = 0.5f * (wetL - wetR) * juce::jlimit (0.0f, 1.8f, parameters.width);
        wetL = mid + side;
        wetR = mid - side;

        const auto duckAmount = juce::jlimit (0.0f, 1.0f, parameters.ducking);
        const auto duckGain = 1.0f - duckAmount * juce::jlimit (0.0f, 0.85f, envelope * 3.0f);
        const auto mix = juce::jlimit (0.0f, 1.0f, parameters.mix);
        constexpr auto wetTrim = 0.82f;

        buffer.setSample (0, i, dryL * (1.0f - mix) + wetL * wetTrim * mix * duckGain);
        if (hasRight)
            buffer.setSample (1, i, dryR * (1.0f - mix) + wetR * wetTrim * mix * duckGain);
    }
}
