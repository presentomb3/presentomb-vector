#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ScopeBuffer.h"

#if JUCE_WINDOWS
class DesktopLoopbackCapture;
#endif

class DSO2AudioProcessor final : public juce::AudioProcessor
{
public:
    DSO2AudioProcessor();
    ~DSO2AudioProcessor() override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept { return apvts; }
    const ScopeBuffer& getScopeBuffer() const noexcept { return scopeBuffer; }
    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(std::memory_order_acquire); }
    float getPeakLevel(int channel) const noexcept;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void pushExternalAudio(const juce::AudioBuffer<float>&, double sampleRate) noexcept;
    bool usesDesktopAudio() const noexcept;

    ScopeBuffer scopeBuffer;
    juce::AudioProcessorValueTreeState apvts;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> balanceSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> balanceTrimSmoothed;
    std::atomic<double> currentSampleRate { 48000.0 };
    std::array<std::atomic<float>, 2> peaks {};
#if JUCE_WINDOWS
    std::unique_ptr<DesktopLoopbackCapture> desktopCapture;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSO2AudioProcessor)
};
