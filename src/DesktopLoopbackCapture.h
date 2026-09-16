#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

class DesktopLoopbackCapture final : private juce::Thread
{
public:
    using AudioCallback = std::function<void(const juce::AudioBuffer<float>&, double)>;

    explicit DesktopLoopbackCapture(AudioCallback);
    ~DesktopLoopbackCapture() override;

    void start();
    bool isCapturing() const noexcept { return capturing.load(std::memory_order_acquire); }

private:
    void run() override;

    AudioCallback callback;
    std::atomic<bool> capturing { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesktopLoopbackCapture)
};
