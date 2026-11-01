#pragma once
#include "Processor.h"
class TonalEditor final : public juce::AudioProcessorEditor {
public:
    explicit TonalEditor(TonalProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::ComboBox view, target;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> viewAttachment;
};
