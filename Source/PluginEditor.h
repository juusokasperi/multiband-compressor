/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <memory>
#include <JuceHeader.h>
#include "PluginProcessor.h"

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    KnobLookAndFeel(juce::Image knobImageToUse) : knobImage(knobImageToUse) {}
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override
    {
      const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
      // const float radius = juce::jmin(width, height) / 2.0f;
      const float centreX = x + width * 0.5f;
      const float centreY = y + height * 0.5f;
      g.saveState();
      g.addTransform(juce::AffineTransform::rotation(angle, centreX, centreY));
      g.drawImage(knobImage, x, y, width, height, 0, 0, knobImage.getWidth(), knobImage.getHeight());
      g.restoreState();
    }
  private:
    juce::Image knobImage;
};

//==============================================================================
/**
*/
class SeventySixCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    SeventySixCompressorAudioProcessorEditor (SeventySixCompressorAudioProcessor&);
    ~SeventySixCompressorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SeventySixCompressorAudioProcessor& audioProcessor;
    void timerCallback() override;

    enum MeterMode { GR, PLUS4, PLUS8, OFF };
    MeterMode meterMode = GR;

    juce::Image backgroundImg;
    juce::Image knobImg;
    juce::Image knob2Img;
     juce::Image buttonImg;
    juce::Image buttonSelectedImg;

     juce::Slider inputKnob, outputKnob, attackKnob, releaseKnob;

     juce::ImageButton ratio4Button, ratio8Button, ratio12Button, ratio20Button, allButtonsButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SeventySixCompressorAudioProcessorEditor)
};
