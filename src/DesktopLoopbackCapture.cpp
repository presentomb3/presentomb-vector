#include "DesktopLoopbackCapture.h"

#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <ksmedia.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace
{
float readPcmSample(const BYTE* source, int bitsPerSample) noexcept
{
    if (bitsPerSample == 16)
        return static_cast<float>(*reinterpret_cast<const int16_t*>(source)) / 32768.0f;
    if (bitsPerSample == 24)
    {
        int value = source[0] | (source[1] << 8) | (source[2] << 16);
        if ((value & 0x800000) != 0) value |= ~0xffffff;
        return static_cast<float>(value) / 8388608.0f;
    }
    if (bitsPerSample == 32)
        return static_cast<float>(*reinterpret_cast<const int32_t*>(source)) / 2147483648.0f;
    return 0.0f;
}
}

DesktopLoopbackCapture::DesktopLoopbackCapture(AudioCallback audioCallback)
    : juce::Thread("Desktop audio capture"), callback(std::move(audioCallback))
{
}

DesktopLoopbackCapture::~DesktopLoopbackCapture()
{
    signalThreadShouldExit();
    stopThread(2000);
}

void DesktopLoopbackCapture::start()
{
    if (! isThreadRunning())
        startThread(juce::Thread::Priority::high);
}

void DesktopLoopbackCapture::run()
{
    const auto comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool shouldUninitialise = SUCCEEDED(comResult);

    ComPtr<IMMDeviceEnumerator> enumerator;
    ComPtr<IMMDevice> device;
    ComPtr<IAudioClient> client;
    ComPtr<IAudioCaptureClient> capture;
    WAVEFORMATEX* mixFormat = nullptr;

    auto cleanup = [&]
    {
        capturing.store(false, std::memory_order_release);
        if (client != nullptr) client->Stop();
        if (mixFormat != nullptr) CoTaskMemFree(mixFormat);
        if (shouldUninitialise) CoUninitialize();
    };

    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(enumerator.GetAddressOf())))
        || FAILED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, device.GetAddressOf()))
        || FAILED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                   reinterpret_cast<void**>(client.GetAddressOf())))
        || FAILED(client->GetMixFormat(&mixFormat)))
    {
        cleanup();
        return;
    }

    constexpr REFERENCE_TIME bufferDuration = 1000000; // 100 ms
    if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                                  bufferDuration, 0, mixFormat, nullptr))
        || FAILED(client->GetService(IID_PPV_ARGS(capture.GetAddressOf())))
        || FAILED(client->Start()))
    {
        cleanup();
        return;
    }

    const int channels = static_cast<int>(mixFormat->nChannels);
    const int bytesPerSample = mixFormat->wBitsPerSample / 8;
    bool isFloat = mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
    if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE
        && mixFormat->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        isFloat = IsEqualGUID(reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat)->SubFormat,
                              KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

    if (channels <= 0 || bytesPerSample <= 0 || (! isFloat && mixFormat->wBitsPerSample < 16))
    {
        cleanup();
        return;
    }

    capturing.store(true, std::memory_order_release);
    juce::AudioBuffer<float> converted;

    while (! threadShouldExit())
    {
        UINT32 packetFrames = 0;
        if (FAILED(capture->GetNextPacketSize(&packetFrames))) break;
        if (packetFrames == 0) { wait(5); continue; }

        while (packetFrames > 0 && ! threadShouldExit())
        {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
                break;

            converted.setSize(2, static_cast<int>(frames), false, false, true);
            converted.clear();
            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0 && data != nullptr)
            {
                for (UINT32 frame = 0; frame < frames; ++frame)
                    for (int outputChannel = 0; outputChannel < 2; ++outputChannel)
                    {
                        const int sourceChannel = juce::jmin(outputChannel, channels - 1);
                        const auto* sample = data + (frame * channels + sourceChannel) * bytesPerSample;
                        const float value = isFloat && mixFormat->wBitsPerSample == 32
                            ? *reinterpret_cast<const float*>(sample)
                            : readPcmSample(sample, mixFormat->wBitsPerSample);
                        converted.setSample(outputChannel, static_cast<int>(frame), value);
                    }
            }

            capture->ReleaseBuffer(frames);
            callback(converted, static_cast<double>(mixFormat->nSamplesPerSec));
            if (FAILED(capture->GetNextPacketSize(&packetFrames))) packetFrames = 0;
        }
    }

    cleanup();
}
