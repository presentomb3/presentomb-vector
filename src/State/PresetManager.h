#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager final
{
public:
    static bool saveGlobalDefault(juce::AudioProcessorValueTreeState& state);
    static bool loadGlobalDefault(juce::AudioProcessorValueTreeState& state);

private:
    static juce::File getGlobalDefaultFile();
};
