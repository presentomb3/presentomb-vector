#include "ScopeBuffer.h"

ScopeBuffer::ScopeBuffer()
{
    reset();
}

void ScopeBuffer::reset() noexcept
{
    for (auto& channel : data)
        channel.fill(0.0f);

    writePosition.store(0, std::memory_order_release);
}

void ScopeBuffer::pushBlock(const juce::AudioBuffer<float>& buffer) noexcept
{
    auto pos = writePosition.load(std::memory_order_relaxed);
    const auto numSamples = buffer.getNumSamples();
    const auto inputs = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        data[0][pos] = inputs > 0 ? buffer.getSample(0, i) : 0.0f;
        data[1][pos] = inputs > 1 ? buffer.getSample(1, i) : data[0][pos];
        pos = (pos + 1) & (capacity - 1);
    }

    writePosition.store(pos, std::memory_order_release);
}

float ScopeBuffer::getSample(int channel, int absoluteIndex) const noexcept
{
    const auto ch = juce::jlimit(0, channelCount - 1, channel);
    return data[static_cast<size_t>(ch)][static_cast<size_t>(absoluteIndex & (capacity - 1))];
}

void ScopeBuffer::copyLatest(juce::AudioBuffer<float>& destination, int samples) const noexcept
{
    const auto count = juce::jlimit(0, destination.getNumSamples(), samples);
    const auto end = getWritePosition();
    const auto start = end - count;

    for (int ch = 0; ch < juce::jmin(channelCount, destination.getNumChannels()); ++ch)
        for (int i = 0; i < count; ++i)
            destination.setSample(ch, i, getSample(ch, start + i));
}
