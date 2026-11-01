#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <TbcTargets.h>
namespace tbc {
inline juce::Array<juce::var> targets() {
    const auto parsed = juce::JSON::parse(juce::String::fromUTF8(TbcTargets::factory_json, TbcTargets::factory_jsonSize));
    juce::Array<juce::var> result;
    if (const auto* array = parsed.getArray())
        for (const auto& item : *array)
            if (item["category"].toString() == (TBC_IS_TRACK ? "track" : "master")) result.add(item);
    return result;
}
inline juce::StringArray targetNames() {
    juce::StringArray result;
    for (const auto& item : targets()) result.add(item["name"].toString());
    return result;
}
}
