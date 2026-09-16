#include "PluginEditor.h"
#include "Parameters.h"
#include "State/PresetManager.h"

namespace
{
constexpr int defaultEditorWidth = 430;
constexpr int defaultEditorHeight = 430;
constexpr int scopeRefreshHz = 480;

void setupSlider(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 18);
}

float traceGainScale(float db) noexcept
{
    constexpr auto defaultTraceScale = 2.35f;
    if (db <= -59.95f)
        return 0.06f;
    if (db < 0.0f)
    {
        const auto n = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        return juce::jmap(std::pow(n, 1.65f), 0.0f, 1.0f, 0.06f, defaultTraceScale);
    }
    return juce::jmap(juce::jlimit(0.0f, 6.0f, db), 0.0f, 6.0f, defaultTraceScale, 4.25f);
}

class PreferencesContent final : public juce::Component
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    explicit PreferencesContent(juce::AudioProcessorValueTreeState& state) : apvts(state), tabs(juce::TabbedButtonBar::TabsAtTop)
    {
        addAndMakeVisible(tabs);
        auto* gonio = new juce::Component();
        tabs.addTab("Scope", juce::Colour(0xff2f3d36), gonio, true);

        juce::Component* gonioControls[] { &gainLabel, &gain, &autoGain, &points,
                                            &focusLabel, &focus, &phosphorLabel, &phosphor, &persistenceLabel, &persistence,
                                            &paletteLabel, &preferencesPalette, &inputSourceLabel, &inputSource,
                                            &resetDefaults, &saveDefault, &savedNotice, &copyrightNotice };
        for (auto* c : gonioControls)
            gonio->addAndMakeVisible(c);

        gainLabel.setText("Gain", juce::dontSendNotification);
        focusLabel.setText("Focus", juce::dontSendNotification);
        phosphorLabel.setText("Phosphor", juce::dontSendNotification);
        persistenceLabel.setText("Persistence", juce::dontSendNotification);
        paletteLabel.setText("Phosphor Color", juce::dontSendNotification);
        inputSourceLabel.setText("Standalone Input", juce::dontSendNotification);
        for (auto* l : { &gainLabel, &focusLabel, &phosphorLabel, &persistenceLabel, &paletteLabel, &inputSourceLabel })
            l->setJustificationType(juce::Justification::centred);
        for (auto* s : { &gain, &focus, &phosphor, &persistence })
            setupSlider(*s);

        autoGain.setButtonText("Auto-Gain");
        points.setButtonText("Points");
        resetDefaults.setButtonText("Reset");
        saveDefault.setButtonText("Save State as Global Default");
        savedNotice.setJustificationType(juce::Justification::centred);
        savedNotice.setColour(juce::Label::textColourId, juce::Colour(0xff9dffb8));
        copyrightNotice.setText(juce::String::fromUTF8(u8"\u00a9 presentomb"), juce::dontSendNotification);
        copyrightNotice.setJustificationType(juce::Justification::centred);
        copyrightNotice.setColour(juce::Label::textColourId, juce::Colour(0xff9dffb8).withAlpha(0.54f));
        resetDefaults.onClick = [this]
        {
            resetParameter(Parameters::mode, 0.0f);
            resetParameter(Parameters::timeDiv, 0.001f);
            resetParameter(Parameters::voltsDiv, 0.05f);
            resetParameter(Parameters::trigger, -1.0f);
            resetParameter(Parameters::slope, 0.0f);
            resetParameter(Parameters::intensity, 0.1f);
            resetParameter(Parameters::persistence, 0.0f);
            resetParameter(Parameters::beamWidth, 0.5f);
            resetParameter(Parameters::gain, 0.0f);
            resetParameter(Parameters::autoGain, 0.0f);
            resetParameter(Parameters::points, 0.0f);
            resetParameter(Parameters::phosphor, 0.0f);
            resetParameter(Parameters::hue, 70.0f);
            resetParameter(Parameters::vintage, 0.0f);
            resetParameter(Parameters::palette, 0.0f);
            resetParameter(Parameters::vintageAmount, 0.0f);
            resetParameter(Parameters::effectAmount, 0.0f);
            resetParameter(Parameters::freeze, 0.0f);
            resetParameter(Parameters::performance, 0.0f);
            resetParameter(Parameters::balanceEnabled, 0.0f);
            resetParameter(Parameters::balance, -1.0f);
            resetParameter(Parameters::balanceCenterAttenuationDb, 0.0f);
            resetParameter(Parameters::monitorMode, 0.0f);
            resetParameter(Parameters::inputSource, 0.0f);
            savedNotice.setText("Reset", juce::dontSendNotification);
        };
        saveDefault.onClick = [this]
        {
            PresetManager::saveGlobalDefault(apvts);
            savedNotice.setText("Saved", juce::dontSendNotification);
        };

        gainAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::gain, gain);
        focusAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::beamWidth, focus);
        phosphorAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::phosphor, phosphor);
        persistenceAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::persistence, persistence);
        autoGainAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::autoGain, autoGain);
        pointsAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::points, points);
        preferencesPalette.addItemList({ "Green", "Deep Green", "Amber", "Red", "Blue", "White" }, 1);
        preferencesPaletteAttachment = std::make_unique<ComboAttachment>(apvts, Parameters::palette, preferencesPalette);
        inputSource.addItemList({ "Audio Device", "Desktop Audio (Windows)" }, 1);
        inputSourceAttachment = std::make_unique<ComboAttachment>(apvts, Parameters::inputSource, inputSource);
        setSize(350, 550);
    }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff2f3d36)); }

    void resized() override
    {
        tabs.setBounds(getLocalBounds());
        if (auto* gonio = tabs.getTabContentComponent(0))
        {
            auto r = gonio->getLocalBounds().reduced(16, 18);
            layoutSlider(r, gainLabel, gain);
            auto row = r.removeFromTop(30);
            autoGain.setBounds(row.removeFromLeft(140));
            points.setBounds(row.removeFromRight(100));
            r.removeFromTop(6);
            layoutSlider(r, focusLabel, focus);
            layoutSlider(r, phosphorLabel, phosphor);
            layoutSlider(r, persistenceLabel, persistence);
            paletteLabel.setBounds(r.removeFromTop(22));
            preferencesPalette.setBounds(r.removeFromTop(30));
            r.removeFromTop(6);
            inputSourceLabel.setBounds(r.removeFromTop(22));
            inputSource.setBounds(r.removeFromTop(30));
            r.removeFromTop(6);
            auto actions = r.removeFromTop(34);
            resetDefaults.setBounds(actions.removeFromLeft(94));
            actions.removeFromLeft(8);
            saveDefault.setBounds(actions);
            savedNotice.setBounds(r.removeFromTop(22));
            r.removeFromTop(6);
            copyrightNotice.setBounds(r.removeFromTop(20));
        }
    }

private:
    void resetParameter(const char* id, float value)
    {
        if (auto* parameter = apvts.getParameter(id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
            parameter->endChangeGesture();
        }
    }

    static void layoutSlider(juce::Rectangle<int>& r, juce::Label& label, juce::Slider& slider)
    {
        label.setBounds(r.removeFromTop(22));
        slider.setBounds(r.removeFromTop(40));
        r.removeFromTop(6);
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::TabbedComponent tabs;
    juce::Label gainLabel, focusLabel, phosphorLabel, persistenceLabel, paletteLabel, inputSourceLabel, savedNotice, copyrightNotice;
    juce::Slider gain, focus, phosphor, persistence;
    juce::ComboBox preferencesPalette, inputSource;
    juce::ToggleButton autoGain, points;
    juce::TextButton resetDefaults, saveDefault;
    std::unique_ptr<SliderAttachment> gainAttachment, focusAttachment, phosphorAttachment, persistenceAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment, pointsAttachment;
    std::unique_ptr<ComboAttachment> preferencesPaletteAttachment, inputSourceAttachment;
};
}

DSO2LookAndFeel::DSO2LookAndFeel()
{
    setColour(juce::Slider::thumbColourId, juce::Colour(0xff8dffab));
}

juce::Font DSO2LookAndFeel::lunaFont(float height) const
{
    return juce::Font { juce::FontOptions(height).withStyle("Bold") };
}

void DSO2LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.getTextBoxPosition() == juce::Slider::NoTextBox)
        return;

    juce::LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

void DSO2LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    auto r = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)).reduced(4.0f);
    const auto a = start + pos * (end - start);
    g.setColour(juce::Colour(0xff121713)); g.fillEllipse(r);
    g.setColour(juce::Colour(0xff58735f)); g.drawEllipse(r, 1.2f);
    g.setColour(juce::Colour(0xffdfffe7));
    g.drawLine({ r.getCentre(), r.getCentre().getPointOnCircumference(r.getWidth() * 0.36f, a) }, 2.0f);
}

void DSO2LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    if (b.getButtonText().isEmpty())
        return;

    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const auto on = b.getToggleState();
    g.setGradientFill({ on ? juce::Colour(0xff45694b) : juce::Colour(0xff2b302c), r.getCentreX(), r.getY(),
                        on ? juce::Colour(0xff1e3524) : juce::Colour(0xff121512), r.getCentreX(), r.getBottom(), false });
    g.fillRoundedRectangle(r, 2.0f);
    g.setColour((hi || down || on) ? juce::Colour(0xffb6ffc8) : juce::Colour(0xff5f6860));
    g.drawRoundedRectangle(r, 2.0f, 1.0f);
}

void DSO2LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    if (b.getButtonText().isEmpty())
        return;

    g.setFont(juce::FontOptions(12.0f));
    g.setColour(b.getToggleState() ? juce::Colour(0xffffffff) : juce::Colour(0xffd9ded8));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(2), juce::Justification::centred, 1, 0.75f);
}

void DSO2LookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0, 0, static_cast<float>(w), static_cast<float>(h)).reduced(1.0f);
    g.setColour(juce::Colour(0xff111811)); g.fillRoundedRectangle(r, 3.0f);
    g.setColour(juce::Colour(0xff55705b)); g.drawRoundedRectangle(r, 3.0f, 1.0f);
}

DSO2AudioProcessorEditor::DSO2AudioProcessorEditor(DSO2AudioProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);
    auto& apvts = processor.getParameters();
    for (auto* s : { &timeDivSlider, &voltsDivSlider, &triggerSlider, &intensitySlider, &persistenceSlider,
                     &beamSlider, &gainSlider, &phosphorSlider, &hueSlider, &vintageAmountSlider, &effectAmountSlider })
        setupSlider(*s);

    modeAttachment = std::make_unique<ComboAttachment>(apvts, Parameters::mode, modeBox);
    paletteAttachment = std::make_unique<ComboAttachment>(apvts, Parameters::palette, paletteBox);
    slopeAttachment = std::make_unique<ComboAttachment>(apvts, Parameters::slope, slopeBox);
    timeDivAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::timeDiv, timeDivSlider);
    voltsDivAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::voltsDiv, voltsDivSlider);
    triggerAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::trigger, triggerSlider);
    intensityAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::intensity, intensitySlider);
    persistenceAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::persistence, persistenceSlider);
    beamAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::beamWidth, beamSlider);
    gainAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::gain, gainSlider);
    phosphorAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::phosphor, phosphorSlider);
    hueAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::hue, hueSlider);
    vintageAmountAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::vintageAmount, vintageAmountSlider);
    effectAmountAttachment = std::make_unique<SliderAttachment>(apvts, Parameters::effectAmount, effectAmountSlider);
    freezeAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::freeze, freezeButton);
    vintageAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::vintage, vintageButton);
    autoGainAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::autoGain, autoGainButton);
    pointsAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::points, pointsButton);
    performanceAttachment = std::make_unique<ButtonAttachment>(apvts, Parameters::performance, performanceButton);

    addAndMakeVisible(gearButton);
    gearButton.onClick = [this] { openPreferencesWindow(); };
    gearButton.onStateChange = [this] { repaint(gearButton.getBounds().expanded(3)); };

    updateSettingsVisibility();
    setSize(defaultEditorWidth, defaultEditorHeight);
    setResizable(true, true);
    setResizeLimits(220, 220, 900, 900);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio(1.0);
    startTimerHz(scopeRefreshHz);
}

DSO2AudioProcessorEditor::~DSO2AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void DSO2AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff050805));
    const auto outerPad = 8;
    auto bounds = getLocalBounds().reduced(outerPad);
    drawChrome(g, bounds);

    auto inner = bounds.reduced(10);
    inner.removeFromTop(28);
    const auto side = juce::jmax(24, juce::jmin(inner.getWidth(), inner.getHeight()));
    drawFrame(g, juce::Rectangle<int>(side, side).withCentre(inner.getCentre()));

    drawBranding(g, bounds);
}

juce::Rectangle<int> DSO2AudioProcessorEditor::getMonitorPanelBounds() const { return {}; }
float DSO2AudioProcessorEditor::getMonitorControlsAlpha() const noexcept { return 0.0f; }

void DSO2AudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState state(g);
    drawGearIcon(g, gearButton.getBounds());
}

void DSO2AudioProcessorEditor::resized()
{
    gearButton.setBounds(getWidth() - 38, 10, 28, 28);
}

void DSO2AudioProcessorEditor::timerCallback()
{
    ++frameCounter;
    snapshotAudio();
    if ((frameCounter % 8) == 0)
    {
        const auto target = computeAutoGainScale();
        autoGainScale += ((param(Parameters::autoGain) > 0.5f ? target : 1.0f) - autoGainScale) * 0.04f;
    }
    if ((frameCounter % 16) == 0)
        updateMeasurements();
    const auto targetTraceGain = traceGainScale(param(Parameters::gain)) * (param(Parameters::autoGain) > 0.5f ? autoGainScale : 1.0f);
    displayedTraceGain += (targetTraceGain - displayedTraceGain) * 0.18f;
    repaint();
}

void DSO2AudioProcessorEditor::snapshotAudio()
{
    const auto sr = processor.getCurrentSampleRate();
    activeSamples = juce::jlimit(512, displayBuffer.getNumSamples(), static_cast<int>(param(Parameters::timeDiv) * 10.0f * sr));
    processor.getScopeBuffer().copyLatest(displayBuffer, activeSamples);
}

void DSO2AudioProcessorEditor::updateFft() {}

void DSO2AudioProcessorEditor::updateMeasurements()
{
    const auto samples = juce::jlimit(1, activeSamples, displayBuffer.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
    {
        double sum = 0.0;
        float peak = 0.0f;
        for (int i = 0; i < samples; ++i)
        {
            const auto v = displayBuffer.getSample(ch, i);
            peak = juce::jmax(peak, std::abs(v));
            sum += static_cast<double>(v) * v;
        }
        measurements[ch].peak = peak;
        measurements[ch].rms = std::sqrt(static_cast<float>(sum / samples));
        measurements[ch].vpp = peak * 2.0f;
    }
}

void DSO2AudioProcessorEditor::drawFrame(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat().reduced(6.0f);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(r, 8.0f);
    const auto traceArea = r.reduced(2.0f);
    const auto traceBounds = traceArea.getSmallestIntegerContainer();
    const auto persistence = juce::jlimit(0.0f, 1.0f, param(Parameters::persistence));
    {
        juce::Graphics::ScopedSaveState s(g);
        g.reduceClipRegion(traceBounds);
        g.setColour(juce::Colours::black);

        if (persistence <= 0.001f)
        {
            g.fillRect(traceArea);
            drawActiveTrace(g, traceArea);
        }
        else
        {
            ensurePersistenceImage(traceBounds);
            juce::Graphics pg(persistenceImage);
            const auto fadeAlpha = juce::jmap(persistence, 0.0f, 1.0f, 0.18f, 0.028f);
            pg.setColour(juce::Colours::black.withAlpha(fadeAlpha));
            pg.fillRect(persistenceImage.getBounds());
            drawActiveTrace(pg, persistenceImage.getBounds().toFloat().reduced(1.0f));
            g.fillRect(traceArea);
            g.drawImageAt(persistenceImage, traceBounds.getX(), traceBounds.getY());
        }
    }
    drawGrid(g, r);
    g.setColour(juce::Colour(0xff4a604d));
    g.drawRoundedRectangle(r, 8.0f, 1.35f);
}

void DSO2AudioProcessorEditor::drawActiveTrace(juce::Graphics& g, juce::Rectangle<float> r) { drawXY(g, r); }

void DSO2AudioProcessorEditor::drawGrid(juce::Graphics& g, juce::Rectangle<float> r)
{
    auto grid = r.reduced(1.0f);
    g.setColour(juce::Colour(0xff2f3832).withAlpha(0.34f));
    g.drawLine(grid.getX(), grid.getY(), grid.getRight(), grid.getBottom(), 1.0f);
    g.drawLine(grid.getX(), grid.getBottom(), grid.getRight(), grid.getY(), 1.0f);
    g.setColour(juce::Colour(0xff4b574f).withAlpha(0.55f));
    g.drawLine(grid.getCentreX(), grid.getY(), grid.getCentreX(), grid.getBottom(), 1.1f);
    g.drawLine(grid.getX(), grid.getCentreY(), grid.getRight(), grid.getCentreY(), 1.1f);
}

void DSO2AudioProcessorEditor::drawWaveform(juce::Graphics&, juce::Rectangle<float>) {}

void DSO2AudioProcessorEditor::drawXY(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Path path;
    const auto samples = juce::jlimit(1, activeSamples, displayBuffer.getNumSamples());
    const auto step = juce::jmax(3, samples / 520);
    double energySum = 0.0;
    float peak = 0.0f;
    for (int i = 0; i < samples; i += step)
    {
        const auto l = displayBuffer.getSample(0, i);
        const auto rr = displayBuffer.getSample(1, i);
        const auto mid = (l + rr) * 0.70710678f;
        const auto side = (rr - l) * 0.70710678f;
        energySum += static_cast<double>(mid * mid + side * side);
        peak = juce::jmax(peak, std::abs(mid), std::abs(side));
    }

    const auto scannedSamples = juce::jmax(1, samples / step);
    const auto rms = std::sqrt(static_cast<float>(energySum / static_cast<double>(scannedSamples)));
    const auto audioDrive = juce::jlimit(0.0f, 1.0f, rms * 5.5f + peak * 0.65f);
    const auto reactiveScale = 1.0f + audioDrive * 0.34f;
    const auto span = juce::jmin(r.getWidth(), r.getHeight()) * displayedTraceGain * 0.40f * reactiveScale;
    const auto wobbleDepth = (0.0018f + audioDrive * 0.0115f) * juce::jmin(r.getWidth(), r.getHeight());
    const auto phase = static_cast<float>(frameCounter) * 0.052f;
    auto clamp = [r](juce::Point<float> p) { return juce::Point<float>(juce::jlimit(r.getX(), r.getRight(), p.x), juce::jlimit(r.getY(), r.getBottom(), p.y)); };
    auto project = [=](float l, float rr, int i)
    {
        const auto n = static_cast<float>(i) / static_cast<float>(juce::jmax(1, samples - 1));
        l = juce::jlimit(-1.0f, 1.0f, l);
        rr = juce::jlimit(-1.0f, 1.0f, rr);
        const auto side = (rr - l) * 0.70710678f;
        const auto mid = (l + rr) * 0.70710678f;
        const auto analogBend = std::sin(n * juce::MathConstants<float>::twoPi * 1.7f + phase) * wobbleDepth;
        const auto sweepSag = std::sin(n * juce::MathConstants<float>::twoPi * 3.1f + phase * 0.73f) * wobbleDepth * 0.42f;
        const auto hotSpotPull = audioDrive * std::sin((mid + side) * 5.0f + phase * 1.9f) * wobbleDepth * 0.55f;
        return clamp({ r.getCentreX() + side * span + analogBend + hotSpotPull,
                       r.getCentreY() - mid * span + sweepSag });
    };

    if (param(Parameters::points) < 0.5f)
    {
        for (int i = 0; i < samples; i += step)
        {
            const auto l = displayBuffer.getSample(0, i);
            const auto rr = displayBuffer.getSample(1, i);
            const auto p = project(l, rr, i);
            if (i == 0) path.startNewSubPath(p); else path.lineTo(p);
        }
        strokeLaserTrace(g, path, param(Parameters::beamWidth) * (1.0f + audioDrive * 0.85f));
    }
    else
    {
        g.setColour(traceColour(0, juce::jlimit(0.30f, 1.0f, 0.45f + audioDrive * 0.85f)));
        const auto dot = juce::jmax(1.55f, param(Parameters::beamWidth) * (0.95f + audioDrive * 1.2f));
        for (int i = 0; i < samples; i += step)
        {
            const auto l = displayBuffer.getSample(0, i);
            const auto rr = displayBuffer.getSample(1, i);
            const auto p = project(l, rr, i);
            g.fillEllipse(juce::Rectangle<float>(dot, dot).withCentre(p));
        }
    }
}

void DSO2AudioProcessorEditor::drawFFT(juce::Graphics&, juce::Rectangle<float>) {}

void DSO2AudioProcessorEditor::drawChrome(juce::Graphics& g, juce::Rectangle<int> b)
{
    auto r = b.toFloat();
    g.setColour(juce::Colour(0xff101d13));
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(juce::Colour(0xff3d6948));
    g.drawRoundedRectangle(r, 8.0f, 1.2f);
}

void DSO2AudioProcessorEditor::drawMeters(juce::Graphics&, juce::Rectangle<int>) {}
void DSO2AudioProcessorEditor::drawMeasurementPanel(juce::Graphics&, juce::Rectangle<int>) {}
void DSO2AudioProcessorEditor::drawControlLabels(juce::Graphics&) {}

void DSO2AudioProcessorEditor::drawGearIcon(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat().reduced(5.0f);
    auto c = r.getCentre();
    const auto rad = r.getWidth() * 0.34f;
    const auto highlighted = gearButton.isMouseOverOrDragging();
    if (highlighted)
    {
        g.setColour(juce::Colour(0xff8dffab).withAlpha(gearButton.isDown() ? 0.18f : 0.11f));
        g.fillEllipse(area.toFloat().reduced(3.0f));
    }

    g.setColour(highlighted ? juce::Colour(0xff78d98b) : juce::Colour(0xff5aa66c));
    for (int i = 0; i < 8; ++i)
    {
        const auto a = juce::MathConstants<float>::twoPi * i / 8.0f;
        g.drawLine({ c.getPointOnCircumference(rad * 0.9f, a), c.getPointOnCircumference(rad * 1.3f, a) }, 1.5f);
    }
    g.drawEllipse(juce::Rectangle<float>(rad * 2, rad * 2).withCentre(c), 1.7f);
    g.fillEllipse(juce::Rectangle<float>(rad * 0.6f, rad * 0.6f).withCentre(c));
}

void DSO2AudioProcessorEditor::drawSettingsPanel(juce::Graphics&, juce::Rectangle<int>) {}

void DSO2AudioProcessorEditor::drawBranding(juce::Graphics& g, juce::Rectangle<int> b)
{
    auto r = b.removeFromTop(43).removeFromLeft(250).reduced(16, 4);
    g.setFont(lookAndFeel.lunaFont(17.0f));
    g.setColour(juce::Colour(0xfff2fff4).withAlpha(0.62f));
    auto title = r.removeFromTop(22);
    g.drawText("PRESENTOMB", title.translated(0, 2), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(9.6f));
    g.setColour(juce::Colour(0xff8dffab).withAlpha(0.72f));
    g.drawText("STEREO VECTOR SCOPE", r.translated(0, -2), juce::Justification::centredLeft);
}

void DSO2AudioProcessorEditor::drawTopPanel(juce::Graphics& g, juce::Rectangle<int>)
{
    auto panel = getMonitorPanelBounds();
    auto p = panel.toFloat();
    g.setColour(juce::Colour(0xff18211b));
    g.fillRoundedRectangle(p, 6.0f);
    g.setColour(juce::Colour(0xff314537));
    g.drawRoundedRectangle(p, 6.0f, 0.9f);
    auto title = panel.reduced(10, 6).removeFromTop(25);
    g.setFont(juce::FontOptions(20.0f));
    g.setColour(juce::Colour(0xfff0f3ed));
    g.drawText("Balance", title, juce::Justification::centred);
    auto line = balanceSlider.getBounds().toFloat().reduced(2.0f, 5.0f);
    const auto v = static_cast<float>(balanceSlider.getValue());
    const auto x = juce::jmap(v, -1.0f, 1.0f, line.getX(), line.getRight());
    const auto rail = line.withHeight(3.0f).withCentre(line.getCentre());
    const auto centreX = rail.getCentreX();
    g.setColour(juce::Colour(0xff6fa67a).withAlpha(0.72f));
    g.fillRoundedRectangle(rail, 2.0f);
    if (std::abs(v) > 0.005f)
    {
        const auto activeX = juce::jmin(x, centreX);
        const auto activeW = std::abs(x - centreX);
        g.setColour(juce::Colour(0xff9dffb8).withAlpha(0.96f));
        g.fillRoundedRectangle(juce::Rectangle<float>(activeX, rail.getY(), activeW, rail.getHeight()), 2.0f);
    }
    g.setColour(juce::Colour(0xffe8ffed).withAlpha(0.85f));
    g.fillRoundedRectangle(juce::Rectangle<float>(2.0f, 9.0f).withCentre({ centreX, rail.getCentreY() }), 1.0f);
    auto thumb = juce::Rectangle<float>(24, 22).withCentre({ x, line.getCentreY() });
    g.setColour(juce::Colour(0xff2b302c)); g.fillRoundedRectangle(thumb, 5.0f);
    g.setColour(juce::Colour(0xffeef5ee)); g.fillEllipse(juce::Rectangle<float>(9, 9).withCentre(thumb.getCentre()));
}

void DSO2AudioProcessorEditor::drawBalanceOverlay(juce::Graphics& g)
{
    if (!balanceSlider.isMouseButtonDown() && juce::Time::getMillisecondCounterHiRes() - lastBalanceEditMs > 900.0)
        return;
    const auto v = static_cast<float>(balanceSlider.getValue());
    const auto text = v < -0.005f ? juce::String(static_cast<int>(std::round(v * 100.0f))) : (v > 0.005f ? "+" + juce::String(static_cast<int>(std::round(v * 100.0f))) : "0");
    const auto mp = getLocalPoint(nullptr, juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().roundToInt()).toFloat();
    auto r = juce::Rectangle<float>(54, 19).withCentre(mp.translated(0, -23));
    g.setColour(juce::Colour(0xee101711)); g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff6da977)); g.drawRoundedRectangle(r, 4.0f, 1.0f);
    g.setColour(juce::Colours::white); g.setFont(juce::FontOptions(10.5f));
    g.drawText(text, r.toNearestInt(), juce::Justification::centred);
}

void DSO2AudioProcessorEditor::strokeLaserTrace(juce::Graphics& g, const juce::Path& path, float beamWidth)
{
    const auto seed = static_cast<float>(((static_cast<uint32_t>(frameCounter) * 1103515245u + 12345u) >> 16) & 0xffu) / 255.0f;
    const auto flicker = 0.965f + seed * 0.07f;
    const auto focus = juce::jlimit(0.5f, 5.0f, beamWidth);
    const auto coreWidth = juce::jmax(0.82f, focus * 0.92f);
    g.setColour(glowColour(0.24f * flicker));
    g.strokePath(path, juce::PathStrokeType(focus * 4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(traceColour(0, 0.60f * flicker));
    g.strokePath(path, juce::PathStrokeType(focus * 2.05f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(traceColour(0, 1.0f));
    g.strokePath(path, juce::PathStrokeType(coreWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void DSO2AudioProcessorEditor::drawScreenArtifacts(juce::Graphics&, juce::Rectangle<float>) {}
void DSO2AudioProcessorEditor::autosetFromCurrentBuffer() { autoGainButton.triggerClick(); }

void DSO2AudioProcessorEditor::ensurePersistenceImage(juce::Rectangle<int> area)
{
    const auto w = juce::jmax(1, area.getWidth());
    const auto h = juce::jmax(1, area.getHeight());
    if (!persistenceImage.isValid() || persistenceImage.getWidth() != w || persistenceImage.getHeight() != h)
    {
        persistenceImage = juce::Image(juce::Image::RGB, w, h, true);
        juce::Graphics pg(persistenceImage);
        pg.fillAll(juce::Colours::black);
    }
}

void DSO2AudioProcessorEditor::updateSettingsVisibility()
{
    juce::Component* hiddenControls[] { &modeBox, &paletteBox, &slopeBox, &timeDivSlider, &voltsDivSlider,
                                        &triggerSlider, &intensitySlider, &persistenceSlider, &beamSlider, &gainSlider, &phosphorSlider,
                                        &hueSlider, &vintageAmountSlider, &effectAmountSlider, &freezeButton, &vintageButton, &autosetButton,
                                        &autoGainButton, &pointsButton, &performanceButton };
    for (auto* c : hiddenControls)
        c->setVisible(false);
    gearButton.setVisible(true);
}

void DSO2AudioProcessorEditor::openPreferencesWindow()
{
    if (preferencesWindow != nullptr) { preferencesWindow->toFront(true); return; }
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = "Preferences";
    o.dialogBackgroundColour = juce::Colour(0xff050805);
    o.content.setOwned(new PreferencesContent(processor.getParameters()));
    o.componentToCentreAround = this;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    preferencesWindow = o.launchAsync();
}

void DSO2AudioProcessorEditor::setMonitorMode(int mode)
{
    if (auto* p = processor.getParameters().getParameter(Parameters::monitorMode))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(mode)));
        p->endChangeGesture();
    }
}

void DSO2AudioProcessorEditor::updateMonitorButtons()
{
    const auto mode = static_cast<int>(param(Parameters::monitorMode));
    monoButton.setToggleState(mode == 1, juce::dontSendNotification);
    diffButton.setToggleState(mode == 2, juce::dontSendNotification);
    swapButton.setToggleState(mode == 3, juce::dontSendNotification);
    leftOnlyButton.setToggleState(mode == 4, juce::dontSendNotification);
    rightOnlyButton.setToggleState(mode == 5, juce::dontSendNotification);
    invertLeftButton.setToggleState(mode == 6, juce::dontSendNotification);
    invertBothButton.setToggleState(mode == 7, juce::dontSendNotification);
    invertRightButton.setToggleState(mode == 8, juce::dontSendNotification);
}

juce::Colour DSO2AudioProcessorEditor::traceColour(int, float alpha) const
{
    const auto t = juce::jlimit(0.0f, 1.0f, param(Parameters::phosphor));
    const auto amber = juce::Colour(0xffffc13d);
    const auto green = juce::Colour(0xff5dff95);
    const auto white = juce::Colour(0xffffffff);
    const auto palette = juce::jlimit(0, 5, static_cast<int>(param(Parameters::palette)));

    juce::Colour colour;
    switch (palette)
    {
        case 1: colour = juce::Colour(0xff20d868); break;
        case 2: colour = amber; break;
        case 3: colour = juce::Colour(0xffff3b30); break;
        case 4: colour = juce::Colour(0xff45a7ff); break;
        case 5: colour = white; break;
        default:
            colour = t <= 0.5f ? amber.interpolatedWith(green, t * 2.0f)
                               : green.interpolatedWith(white, (t - 0.5f) * 2.0f);
            break;
    }
    return colour.withAlpha(alpha);
}

juce::Colour DSO2AudioProcessorEditor::glowColour(float alpha) const { return traceColour(0, alpha); }
juce::String DSO2AudioProcessorEditor::modeText() const { return "XY"; }
juce::String DSO2AudioProcessorEditor::formatHz(float hz) const { return juce::String(hz, 1) + " Hz"; }
juce::String DSO2AudioProcessorEditor::formatVolts(float v) const { return juce::String(v, 2) + " V"; }
juce::String DSO2AudioProcessorEditor::formatControlValue(const juce::Slider& s) const { return juce::String(s.getValue(), 2); }

float DSO2AudioProcessorEditor::computeAutoGainScale() const noexcept
{
    const auto samples = juce::jlimit(1, activeSamples, displayBuffer.getNumSamples());
    const auto step = juce::jmax(2, samples / 2048);
    float maxVector = 0.0f;
    for (int i = 0; i < samples; i += step)
    {
        const auto l = displayBuffer.getSample(0, i);
        const auto r = displayBuffer.getSample(1, i);
        maxVector = juce::jmax(maxVector, std::abs((l + r) * 0.70710678f), std::abs((r - l) * 0.70710678f));
    }
    return maxVector < 0.0005f ? 1.0f : juce::jlimit(0.35f, 8.0f, 0.70f / maxVector);
}

int DSO2AudioProcessorEditor::findTriggerOffset(int) const noexcept { return 0; }

float DSO2AudioProcessorEditor::param(const char* id) const noexcept
{
    if (auto* v = processor.getParameters().getRawParameterValue(id))
        return v->load();
    return 0.0f;
}
