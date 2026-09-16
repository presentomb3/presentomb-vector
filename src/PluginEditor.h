#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class DSO2LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DSO2LookAndFeel();

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;
    juce::Font lunaFont(float height) const;
};

class DSO2AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit DSO2AudioProcessorEditor(DSO2AudioProcessor&);
    ~DSO2AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void snapshotAudio();
    void updateFft();
    void updateMeasurements();
    void drawFrame(juce::Graphics&, juce::Rectangle<int>);
    void drawActiveTrace(juce::Graphics&, juce::Rectangle<float>);
    void drawGrid(juce::Graphics&, juce::Rectangle<float>);
    void drawWaveform(juce::Graphics&, juce::Rectangle<float>);
    void drawXY(juce::Graphics&, juce::Rectangle<float>);
    void drawFFT(juce::Graphics&, juce::Rectangle<float>);
    void drawChrome(juce::Graphics&, juce::Rectangle<int>);
    void drawMeters(juce::Graphics&, juce::Rectangle<int>);
    void drawMeasurementPanel(juce::Graphics&, juce::Rectangle<int>);
    void drawControlLabels(juce::Graphics&);
    void drawGearIcon(juce::Graphics&, juce::Rectangle<int>);
    void drawSettingsPanel(juce::Graphics&, juce::Rectangle<int>);
    void drawBranding(juce::Graphics&, juce::Rectangle<int>);
    void drawTopPanel(juce::Graphics&, juce::Rectangle<int>);
    void drawBalanceOverlay(juce::Graphics&);
    void strokeLaserTrace(juce::Graphics&, const juce::Path&, float beamWidth);
    void drawScreenArtifacts(juce::Graphics&, juce::Rectangle<float>);
    void autosetFromCurrentBuffer();
    void ensurePersistenceImage(juce::Rectangle<int> area);
    void updateSettingsVisibility();
    void openPreferencesWindow();
    void setMonitorMode(int mode);
    void updateMonitorButtons();
    juce::Rectangle<int> getMonitorPanelBounds() const;
    float getMonitorControlsAlpha() const noexcept;
    juce::Colour traceColour(int channel = 0, float alpha = 1.0f) const;
    juce::Colour glowColour(float alpha = 1.0f) const;
    juce::String modeText() const;
    juce::String formatHz(float hz) const;
    juce::String formatVolts(float volts) const;
    juce::String formatControlValue(const juce::Slider& slider) const;
    float computeAutoGainScale() const noexcept;
    int findTriggerOffset(int samples) const noexcept;
    float param(const char* id) const noexcept;

    struct ChannelMeasurements
    {
        float frequency = 0.0f;
        float vpp = 0.0f;
        float rms = 0.0f;
        float peak = 0.0f;
    };

    DSO2AudioProcessor& processor;
    DSO2LookAndFeel lookAndFeel;
    juce::AudioBuffer<float> displayBuffer { 2, ScopeBuffer::capacity };
    juce::dsp::FFT fft { 11 };
    std::array<float, 4096> fftData {};
    std::array<float, 1024> spectrum {};
    juce::Image persistenceImage;
    ChannelMeasurements measurements[2];
    float correlation = 0.0f;
    float autoGainScale = 1.0f;
    float displayedTraceGain = 1.0f;
    float lastBalanceValue = 0.0f;
    double lastBalanceEditMs = -1000.0;
    bool snappingBalanceToCenter = false;
    int activeSamples = 8192;
    int frameCounter = 0;
    uint32_t artifactSeed = 0x1234abcd;
    mutable float smoothedMonitorPanelWidth = 0.0f;
    bool settingsOpen = false;
    juce::Component::SafePointer<juce::DialogWindow> preferencesWindow;

    juce::ComboBox modeBox;
    juce::ComboBox paletteBox;
    juce::ComboBox slopeBox;
    juce::Slider timeDivSlider;
    juce::Slider voltsDivSlider;
    juce::Slider triggerSlider;
    juce::Slider intensitySlider;
    juce::Slider persistenceSlider;
    juce::Slider beamSlider;
    juce::Slider gainSlider;
    juce::Slider phosphorSlider;
    juce::Slider hueSlider;
    juce::Slider vintageAmountSlider;
    juce::Slider effectAmountSlider;
    juce::TextButton freezeButton { "Freeze" };
    juce::TextButton vintageButton { "Vintage" };
    juce::TextButton autosetButton { "Autoset" };
    juce::TextButton gearButton;
    juce::TextButton autoGainButton { "Auto-Gain" };
    juce::TextButton pointsButton { "Points" };
    juce::ToggleButton performanceButton { "Perf" };
    juce::Label titleLabel;
    juce::Slider balanceSlider;
    juce::TextButton balanceButton { "BALANCE" };
    juce::TextButton monoButton { "MONO" };
    juce::TextButton diffButton { "DIFF" };
    juce::TextButton swapButton { "SWAP LR" };
    juce::TextButton leftOnlyButton { "LEFT ONLY" };
    juce::TextButton rightOnlyButton { "RIGHT ONLY" };
    juce::TextButton invertLeftButton { "O LEFT" };
    juce::TextButton invertBothButton { "BOTH" };
    juce::TextButton invertRightButton { "O RIGHT" };

    std::unique_ptr<ComboAttachment> modeAttachment;
    std::unique_ptr<SliderAttachment> timeDivAttachment;
    std::unique_ptr<SliderAttachment> voltsDivAttachment;
    std::unique_ptr<SliderAttachment> triggerAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment;
    std::unique_ptr<SliderAttachment> persistenceAttachment;
    std::unique_ptr<SliderAttachment> beamAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> phosphorAttachment;
    std::unique_ptr<SliderAttachment> hueAttachment;
    std::unique_ptr<SliderAttachment> vintageAmountAttachment;
    std::unique_ptr<SliderAttachment> effectAmountAttachment;
    std::unique_ptr<ComboAttachment> paletteAttachment;
    std::unique_ptr<ComboAttachment> slopeAttachment;
    std::unique_ptr<ButtonAttachment> freezeAttachment;
    std::unique_ptr<ButtonAttachment> vintageAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> pointsAttachment;
    std::unique_ptr<ButtonAttachment> performanceAttachment;
    std::unique_ptr<ButtonAttachment> balanceEnabledAttachment;
    std::unique_ptr<SliderAttachment> balanceAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSO2AudioProcessorEditor)
};
