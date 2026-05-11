#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SGMReverbAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SGMReverbAudioProcessorEditor (SGMReverbAudioProcessor&);
    ~SGMReverbAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    enum class Group
    {
        space = 0,
        tone,
        motion,
        output,
        count
    };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
        Group group = Group::space;
        bool primary = false;
    };

    SGMReverbAudioProcessor& processor;
    juce::OwnedArray<Knob> knobs;
    juce::ToggleButton freezeButton;
    std::unique_ptr<ButtonAttachment> freezeAttachment;

    juce::Label title;
    juce::Label subtitle;
    juce::Label freezeLabel;
    std::array<juce::Label, static_cast<size_t> (Group::count)> groupLabels;
    std::array<juce::Rectangle<int>, static_cast<size_t> (Group::count)> groupBounds;

    void addKnob (const juce::String& parameterId, const juce::String& text, Group group, bool primary = false);
    void layoutGroup (Group group, juce::Rectangle<int> bounds, int columns);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SGMReverbAudioProcessorEditor)
};
