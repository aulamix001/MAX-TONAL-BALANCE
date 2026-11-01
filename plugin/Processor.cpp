#include "Processor.h"
#include "Editor.h"
#include "../core/Targets.h"
TonalProcessor::TonalProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "TBC_STATE", juce::AudioProcessorValueTreeState::ParameterLayout(std::make_unique<juce::AudioParameterChoice>(
          juce::ParameterID{"view", 1}, "View", juce::StringArray{"Fine", "Leveled", "Broad"}, 0), std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"target", 1}, "Target", tbc::targetNames(), 0))) {}
const juce::String TonalProcessor::getName() const { return TBC_IS_TRACK ? "TBC Track" : "TBC Master"; }
bool TonalProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    return b.getMainInputChannelSet() == b.getMainOutputChannelSet()
        && (b.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || b.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void TonalProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals guard;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());
    midi.clear();
}
juce::AudioProcessorEditor* TonalProcessor::createEditor() { return new TonalEditor(*this); }
void TonalProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void TonalProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType()))
        state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TonalProcessor; }
