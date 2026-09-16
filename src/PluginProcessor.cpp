#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "State/PresetManager.h"
#if JUCE_WINDOWS
#include "DesktopLoopbackCapture.h"
#endif

DSO2AudioProcessor::DSO2AudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    peaks[0].store(0.0f);
    peaks[1].store(0.0f);
    PresetManager::loadGlobalDefault(apvts);
}

DSO2AudioProcessor::~DSO2AudioProcessor() = default;

void DSO2AudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate.store(sampleRate > 0.0 ? sampleRate : 48000.0, std::memory_order_release);
    balanceSmoothed.reset(currentSampleRate.load(std::memory_order_acquire), 0.02);
    balanceTrimSmoothed.reset(currentSampleRate.load(std::memory_order_acquire), 0.02);
    balanceSmoothed.setCurrentAndTargetValue(apvts.getRawParameterValue(Parameters::balance)->load());
    balanceTrimSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(-apvts.getRawParameterValue(Parameters::balanceCenterAttenuationDb)->load()));
    scopeBuffer.reset();

#if JUCE_WINDOWS
    if (wrapperType == wrapperType_Standalone && desktopCapture == nullptr)
    {
        desktopCapture = std::make_unique<DesktopLoopbackCapture>(
            [this] (const juce::AudioBuffer<float>& audio, double sourceRate)
            {
                pushExternalAudio(audio, sourceRate);
            });
        desktopCapture->start();
    }
#endif
}

void DSO2AudioProcessor::reset()
{
    scopeBuffer.reset();
    balanceSmoothed.setCurrentAndTargetValue(apvts.getRawParameterValue(Parameters::balance)->load());
    balanceTrimSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(-apvts.getRawParameterValue(Parameters::balanceCenterAttenuationDb)->load()));
    peaks[0].store(0.0f, std::memory_order_release);
    peaks[1].store(0.0f, std::memory_order_release);
}

bool DSO2AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && in.size() <= out.size();
}

void DSO2AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (! usesDesktopAudio() && apvts.getRawParameterValue(Parameters::freeze)->load() < 0.5f)
        scopeBuffer.pushBlock(buffer);

    if (! usesDesktopAudio())
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* read = buffer.getReadPointer(juce::jmin(ch, buffer.getNumChannels() - 1));
        float peak = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = juce::jmax(peak, std::abs(read[i]));
        peaks[static_cast<size_t>(ch)].store(peak, std::memory_order_release);
    }
}

bool DSO2AudioProcessor::usesDesktopAudio() const noexcept
{
#if JUCE_WINDOWS
    return wrapperType == wrapperType_Standalone
        && apvts.getRawParameterValue(Parameters::inputSource)->load() >= 0.5f
        && desktopCapture != nullptr
        && desktopCapture->isCapturing();
#else
    return false;
#endif
}

void DSO2AudioProcessor::pushExternalAudio(const juce::AudioBuffer<float>& audio, double sampleRate) noexcept
{
    if (! usesDesktopAudio() || apvts.getRawParameterValue(Parameters::freeze)->load() >= 0.5f)
        return;

    currentSampleRate.store(sampleRate, std::memory_order_release);
    scopeBuffer.pushBlock(audio);
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto sourceChannel = juce::jmin(ch, audio.getNumChannels() - 1);
        peaks[static_cast<size_t>(ch)].store(
            audio.getMagnitude(sourceChannel, 0, audio.getNumSamples()), std::memory_order_release);
    }
}

juce::AudioProcessorEditor* DSO2AudioProcessor::createEditor()
{
    return new DSO2AudioProcessorEditor(*this);
}

void DSO2AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void DSO2AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

float DSO2AudioProcessor::getPeakLevel(int channel) const noexcept
{
    return peaks[static_cast<size_t>(juce::jlimit(0, 1, channel))].load(std::memory_order_acquire);
}

juce::AudioProcessorValueTreeState::ParameterLayout DSO2AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        Parameters::mode, "Mode", juce::StringArray { "Waveform", "XY", "FFT" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::timeDiv, 1 }, "Time / Div",
        juce::NormalisableRange<float>(0.001f, 0.5f, 0.001f, 0.45f), 0.001f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::voltsDiv, 1 }, "Volts / Div",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.55f), 0.05f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::trigger, 1 }, "Trigger Level",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), -1.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        Parameters::slope, "Trigger Slope", juce::StringArray { "Rising", "Falling" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::intensity, 1 }, "Intensity",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f), 0.1f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::persistence, 1 }, "Persistence",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::beamWidth, 1 }, "Beam Width",
        juce::NormalisableRange<float>(0.5f, 5.0f, 0.01f), 0.5f,
        juce::AudioParameterFloatAttributes()));
    juce::NormalisableRange<float> gainRange(
        -60.0f, 6.0f,
        [] (float, float, float normalised)
        {
            return normalised <= 0.5f
                ? juce::jmap(normalised, 0.0f, 0.5f, -60.0f, 0.0f)
                : juce::jmap(normalised, 0.5f, 1.0f, 0.0f, 6.0f);
        },
        [] (float, float, float value)
        {
            return value <= 0.0f
                ? juce::jmap(value, -60.0f, 0.0f, 0.0f, 0.5f)
                : juce::jmap(value, 0.0f, 6.0f, 0.5f, 1.0f);
        },
        [] (float, float, float value)
        {
            return juce::jlimit(-60.0f, 6.0f, std::round(value * 100.0f) / 100.0f);
        });
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::gain, 1 }, "Input Gain",
        gainRange, 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::autoGain, "Auto-Gain", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::points, "Points Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::phosphor, 1 }, "Phosphor",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::hue, 1 }, "Trace Hue",
        juce::NormalisableRange<float>(70.0f, 170.0f, 1.0f), 70.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::vintage, "Vintage Display", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        Parameters::palette, "Phosphor Palette", juce::StringArray { "Green", "Deep Green", "Amber", "Red", "Blue", "White" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::vintageAmount, 1 }, "Vintage Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::effectAmount, 1 }, "Screen Artifacts",
        juce::NormalisableRange<float>(0.0f, 1.6f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::freeze, "Freeze", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::performance, "Performance Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(Parameters::balanceEnabled, "Balance In", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::balance, 1 }, "Balance",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), -1.0f,
        juce::AudioParameterFloatAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Parameters::balanceCenterAttenuationDb, 1 }, "Balance Center Attenuation",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        Parameters::monitorMode, "Monitor Mode",
        juce::StringArray { "Normal", "Mono", "Diff", "Swap LR", "Left Only", "Right Only", "Invert Left", "Invert Both", "Invert Right" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        Parameters::inputSource, "Standalone Input",
        juce::StringArray { "Audio Device", "Desktop Audio (Windows)" }, 0));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSO2AudioProcessor();
}
