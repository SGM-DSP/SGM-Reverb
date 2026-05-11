#pragma once

#include <JuceHeader.h>

struct ReverbParameters
{
    float mix = 0.35f;
    float predelayMs = 25.0f;
    float size = 0.72f;
    float decay = 0.62f;
    float diffusion = 0.76f;
    float density = 0.7f;
    float damping = 0.45f;
    float lowCutHz = 120.0f;
    float highCutHz = 12000.0f;
    float width = 1.0f;
    float modulation = 0.25f;
    float ducking = 0.0f;
    float shimmer = 0.0f;
    bool freeze = false;
};

class SGMReverbEngine
{
public:
    void prepare (double newSampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const ReverbParameters& parameters);

private:
    class DelayLine
    {
    public:
        void prepare (int newMaximumSamples);
        void reset();
        void push (float sample);
        float read (float delaySamples) const;

    private:
        juce::AudioBuffer<float> data;
        int writeIndex = 0;
    };

    double sampleRate = 44100.0;
    int blockSize = 512;

    std::array<DelayLine, 2> predelay;
    juce::AudioBuffer<float> wetBuffer;
    juce::Reverb reverb;

    juce::dsp::StateVariableTPTFilter<float> lowCutL;
    juce::dsp::StateVariableTPTFilter<float> lowCutR;
    juce::dsp::StateVariableTPTFilter<float> highCutL;
    juce::dsp::StateVariableTPTFilter<float> highCutR;

    float envelope = 0.0f;
};
