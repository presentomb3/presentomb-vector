#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class ScopeBuffer final
{
public:
    static constexpr int capacity = 262144;
    static constexpr int channelCount = 2;

    ScopeBuffer();

    void reset() noexcept;
    void pushBlock(const juce::AudioBuffer<float>& buffer) noexcept;
    int getWritePosition() const noexcept { return writePosition.load(std::memory_order_acquire); }
    float getSample(int channel, int absoluteIndex) const noexcept;
    void copyLatest(juce::AudioBuffer<float>& destination, int samples) const noexcept;

private:
    std::array<std::array<float, capacity>, channelCount> data {};
    std::atomic<int> writePosition { 0 };
};
