#include "Editor.h"
#include "../core/Targets.h"
TonalEditor::TonalEditor(TonalProcessor& p)
    : AudioProcessorEditor(p) {
    view.addItemList({"Fine", "Leveled", "Broad"}, 1);
    viewAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, "view", view);
    view.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff142b38));
    view.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff1fb5b5));
    addAndMakeVisible(view);
    target.addItemList(tbc::targetNames(), 1);
    targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, "target", target);
    target.onChange = [this] { repaint(); };
    addAndMakeVisible(target);
    setResizable(true, true);
    setResizeLimits(900, 600, 1800, 1200);
    setSize(1100, 700);
}
void TonalEditor::resized() { view.setBounds(getWidth() - 180, 22, 150, 30); target.setBounds(getWidth()-460, 22, 260, 30); }
void TonalEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0e1621));
    g.setColour(juce::Colour(0xff1fb5b5));
    g.setFont(22.0f);
    g.drawText(getAudioProcessor()->getName().toUpperCase(), 28, 18, 500, 40, juce::Justification::centredLeft);
    auto area = getLocalBounds().reduced(24).withTrimmedTop(60).withTrimmedBottom(55);
    g.setColour(juce::Colour(0xff111e2b)); g.fillRoundedRectangle(area.toFloat(), 8.0f);
    g.setColour(juce::Colour(0xff304453)); g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.0f);
    g.setColour(juce::Colour(0xffa7b8c5)); g.setFont(17.0f);
    g.drawText("Provisional target preview — no live analysis", area.withHeight(40), juce::Justification::centred);
    g.setFont(14.0f);
    auto models = tbc::targets();
    const int index = juce::jlimit(0, models.size()-1, target.getSelectedItemIndex());
    const auto model = models[index];
    auto plot = area.reduced(35).withTrimmedTop(35).toFloat();
    juce::Path band;
    for (int i=0; i<400; ++i) {
        const int bin = i<200 ? i : 399-i;
        const auto values = model[i<200 ? "hi_dB" : "lo_dB"];
        const float x = plot.getX() + plot.getWidth()*float(bin)/199.0f;
        const float y = plot.getCentreY() - float(double(values[bin]))*plot.getHeight()/80.0f;
        if (i==0) band.startNewSubPath(x,y); else band.lineTo(x,y);
    }
    band.closeSubPath();
    g.setColour(juce::Colour(0x551fb5b5)); g.fillPath(band);
    g.setColour(juce::Colour(0xff1fb5b5)); g.strokePath(band, juce::PathStrokeType(1.5f));
    g.drawText("MAX AUDIO  /  Development skeleton  /  Audio passes through unchanged",
               getLocalBounds().removeFromBottom(55), juce::Justification::centred);
}
