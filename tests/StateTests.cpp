#include "../plugin/Processor.h"
#include <cmath>
#include "../core/Targets.h"
#include <iostream>
class PhaseOneTests final : public juce::UnitTest {
public:
    PhaseOneTests() : UnitTest("Phase 1 processor and editor", "TBC") {}
    void runTest() override {
        TonalProcessor processor;
        processor.prepareToPlay(48000, 512);
        juce::AudioBuffer<float> audio(2, 512);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                audio.setSample(c, i, 0.25f * std::sin(float(i) * 0.1f));
        juce::AudioBuffer<float> expected; expected.makeCopyOf(audio);
        juce::MidiBuffer midi;
        processor.processBlock(audio, midi);
        beginTest("Stereo passthrough: 1024 samples");
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                expectEquals(audio.getSample(c, i), expected.getSample(c, i));
        beginTest("Embedded seed presets");
        expectEquals(tbc::targets().size(), 22);
        for (const auto& model : tbc::targets()) {
            expect(bool(model["provisional"]));
            expectEquals(model["mean_dB"].size(), 200);
            for (int i=0; i<200; ++i) {
                expect(double(model["lo_dB"][i]) < double(model["mean_dB"][i]));
                expect(double(model["hi_dB"][i]) > double(model["mean_dB"][i]));
            }
        }
        beginTest("APVTS state round-trip");
        processor.state.getParameter("target")->setValueNotifyingHost(1.0f);
        processor.state.getParameter("view")->setValueNotifyingHost(1.0f);
        juce::MemoryBlock saved; processor.getStateInformation(saved);
        TonalProcessor restored;
        restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        expectEquals(restored.state.getRawParameterValue("view")->load(), 2.0f);
        expectEquals(restored.state.getRawParameterValue("target")->load(), 21.0f);
        beginTest("Editor creation, resize and attached view selection");
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        expectEquals(editor->getWidth(), 1100);
        expectEquals(editor->getHeight(), 700);
        editor->setSize(900, 600);
        expectEquals(editor->getWidth(), 900);
        auto* combo = dynamic_cast<juce::ComboBox*>(editor->getChildComponent(0));
        expect(combo != nullptr);
        if (combo != nullptr) expectEquals(combo->getSelectedId(), 3);
    }
};
int main() {
    juce::ScopedJuceInitialiser_GUI initialise;
    PhaseOneTests tests;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runTestsInCategory("TBC", 1234);
    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i) {
        failures += runner.getResult(i)->failures;
        passes += runner.getResult(i)->passes;
    }
    std::cout << "TBC assertions: " << passes << " passed, " << failures << " failed\n";
    return failures != 0 || passes == 0 ? 1 : 0;
}
