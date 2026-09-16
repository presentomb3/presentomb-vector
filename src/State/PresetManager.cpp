#include "PresetManager.h"

juce::File PresetManager::getGlobalDefaultFile()
{
    auto folder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Presentomb")
        .getChildFile("DSO-2");
    return folder.getChildFile("GlobalDefault.xml");
}

bool PresetManager::saveGlobalDefault(juce::AudioProcessorValueTreeState& state)
{
    const auto file = getGlobalDefaultFile();
    if (!file.getParentDirectory().createDirectory())
        return false;

    auto copy = state.copyState();
    copy.setProperty("presetVersion", 1, nullptr);

    if (auto xml = copy.createXml())
        return xml->writeTo(file);

    return false;
}

bool PresetManager::loadGlobalDefault(juce::AudioProcessorValueTreeState& state)
{
    const auto file = getGlobalDefaultFile();
    if (!file.existsAsFile())
        return false;

    auto xml = juce::parseXML(file);
    if (xml == nullptr || !xml->hasTagName(state.state.getType()))
        return false;

    state.replaceState(juce::ValueTree::fromXml(*xml));
    return true;
}
