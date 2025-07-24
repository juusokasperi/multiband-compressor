/*
	==============================================================================

		This file contains the basic framework code for a JUCE plugin editor.

	==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SeventySixCompressorAudioProcessorEditor::SeventySixCompressorAudioProcessorEditor (SeventySixCompressorAudioProcessor& p)
		: AudioProcessorEditor (&p), audioProcessor (p)
{
		// Make sure that before the constructor has finished, you've set the
		// editor's size to whatever you need it to be.
		setSize (757, 141);
		backgroundImg = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
		knobImg = juce::ImageCache::getFromMemory(BinaryData::knob_png, BinaryData::knob_pngSize);
		knob2Img = juce::ImageCache::getFromMemory(BinaryData::knob2_png, BinaryData::knob2_pngSize);
		buttonImg = juce::ImageCache::getFromMemory(BinaryData::button_png, BinaryData::button_pngSize);
		buttonSelectedImg = juce::ImageCache::getFromMemory(BinaryData::buttonSelected_png, BinaryData::buttonSelected_pngSize);

		static KnobLookAndFeel knobLNF(knobImg);
		static KnobLookAndFeel knob2LNF(knob2Img);

		inputKnob.setLookAndFeel(&knobLNF);
		inputKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		inputKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		inputKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.95f, true);
		addAndMakeVisible(inputKnob);

		outputKnob.setLookAndFeel(&knobLNF);
		outputKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		outputKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		outputKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.95f, true);
		addAndMakeVisible(outputKnob);

		attackKnob.setLookAndFeel(&knob2LNF);
		attackKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		attackKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		attackKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.15f, juce::MathConstants<float>::pi * 2.85f, true);
		addAndMakeVisible(attackKnob);

		releaseKnob.setLookAndFeel(&knob2LNF);
		releaseKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
		releaseKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
		releaseKnob.setRotaryParameters(juce::MathConstants<float>::pi * 1.15f, juce::MathConstants<float>::pi * 2.85f, true);
		addAndMakeVisible(releaseKnob);

		ratio4Button.setImages(false, true, true, buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonSelectedImg, 1.0f, juce::Colours::transparentBlack);
		addAndMakeVisible(ratio4Button);

		ratio8Button.setImages(false, true, true, buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonSelectedImg, 1.0f, juce::Colours::transparentBlack);
		addAndMakeVisible(ratio8Button);

		ratio12Button.setImages(false, true, true, buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonSelectedImg, 1.0f, juce::Colours::transparentBlack);
		addAndMakeVisible(ratio12Button);

		ratio20Button.setImages(false, true, true, buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonSelectedImg, 1.0f, juce::Colours::transparentBlack);
		addAndMakeVisible(ratio20Button);

		allButtonsButton.setImages(false, true, true, buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonImg, 1.0f, juce::Colours::transparentBlack,
				buttonSelectedImg, 1.0f, juce::Colours::transparentBlack);
		addAndMakeVisible(allButtonsButton);

		ratio4Button.setClickingTogglesState(true);
		ratio8Button.setClickingTogglesState(true);
		ratio12Button.setClickingTogglesState(true);
		ratio20Button.setClickingTogglesState(true);
		allButtonsButton.setClickingTogglesState(true);

		ratio4Button.setRadioGroupId(1);
		ratio8Button.setRadioGroupId(1);
		ratio12Button.setRadioGroupId(1);
		ratio20Button.setRadioGroupId(1);

		inputAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
			 audioProcessor.apvts, "Input Gain", inputKnob);
		outputAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
				audioProcessor.apvts, "Output Gain", outputKnob);
		attackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
				audioProcessor.apvts, "Attack", attackKnob);
		releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
				audioProcessor.apvts, "Release", releaseKnob);

		allButtonsButton.onClick = [this] {
			auto* param = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
			if (param)
				param->setValueNotifyingHost(param->get() ? 0.0f : 1.0f);
		};
		ratio4Button.onClick  = [this] {
			auto* allParam = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
			if (allParam && allParam->get())
				allParam->setValueNotifyingHost(0.0f);
			audioProcessor.apvts.getParameter("Ratio")->setValueNotifyingHost(0.0f);
		};
		ratio8Button.onClick  = [this] {
			auto* allParam = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
			if (allParam && allParam->get())
				allParam->setValueNotifyingHost(0.0f);
			audioProcessor.apvts.getParameter("Ratio")->setValueNotifyingHost(1.0f / 3.0f);
		};
		ratio12Button.onClick = [this] {
			auto* allParam = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
			if (allParam && allParam->get())
				allParam->setValueNotifyingHost(0.0f);
			audioProcessor.apvts.getParameter("Ratio")->setValueNotifyingHost(2.0f / 3.0f);
		};
		ratio20Button.onClick = [this] {
			auto* allParam = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
			if (allParam && allParam->get())
				allParam->setValueNotifyingHost(0.0f);
			audioProcessor.apvts.getParameter("Ratio")->setValueNotifyingHost(1.0f);
		};

		startTimerHz(30);
		timerCallback();
}

SeventySixCompressorAudioProcessorEditor::~SeventySixCompressorAudioProcessorEditor()
{
}

//==============================================================================
void SeventySixCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
		g.drawImage(backgroundImg, getLocalBounds().toFloat());

		float meterValue = 0.0f;
		if (meterMode != OFF)
		{
			meterValue = audioProcessor.getGainReductionDb();
			if (meterMode == PLUS4)
				meterValue += 4.0f;
			else if (meterMode == PLUS8)
				meterValue += 8.0f;
			float minDb = -20.0f;
			float maxDb = 3.0f;
			meterValue = juce::jlimit(minDb, maxDb, meterValue);
			float norm = (meterValue - minDb) / (maxDb - minDb);

			juce::Point<float> center(565, 85);
			float radius = 40.0f;
			float startAngle = juce::MathConstants<float>::pi * 5.0f / 6.0f;
			float endAngle = juce::MathConstants<float>::pi * 2.0f / 6.0f;
			float angle = startAngle + norm * (endAngle - startAngle);
			juce::Point<float> needleEnd = center + juce::Point<float>(std::cos(angle), -std::sin(angle)) * radius;
			g.setColour(juce::Colours::black);
			g.drawLine(center.x, center.y, needleEnd.x, needleEnd.y, 1.0f);

			g.setColour(juce::Colours::black);
			g.setFont(14.0f);
			g.drawFittedText(juce::String(meterValue, 2) + " dB", center.x - 30, center.y + 20, 60, 20, juce::Justification::centred, 1);
		}
}

void SeventySixCompressorAudioProcessorEditor::resized()
{
	inputKnob.setBounds(80, 40, 60, 60);
	outputKnob.setBounds(226, 40, 60, 60);
	attackKnob.setBounds(363, 25, 30, 30);
	releaseKnob.setBounds(363, 86, 30, 30);

	ratio20Button.setBounds(470, 23, 15, 22);
	ratio12Button.setBounds(470, 45, 15, 22);
	ratio8Button.setBounds(470, 67, 15, 22);
	ratio4Button.setBounds(470, 89, 15, 22);
	allButtonsButton.setBounds(470, 111, 15, 22);
		// This is generally where you'll want to lay out the positions of any
		// subcomponents in your editor..
}

void SeventySixCompressorAudioProcessorEditor::timerCallback()
{
	auto* ratioParam = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("Ratio"));
	auto* allButtonsParam = dynamic_cast<juce::AudioParameterBool*>(audioProcessor.apvts.getParameter("All Buttons"));
	if (!ratioParam || !allButtonsParam)
		return;

	bool allButtonsOn = allButtonsParam->get();
	int idx = ratioParam->getIndex();

	allButtonsButton.setToggleState(allButtonsOn, juce::dontSendNotification);
	if (allButtonsOn)
	{
		ratio4Button.setToggleState(false, juce::dontSendNotification);
		ratio8Button.setToggleState(false, juce::dontSendNotification);
		ratio12Button.setToggleState(false, juce::dontSendNotification);
		ratio20Button.setToggleState(false, juce::dontSendNotification);
	}
	else
	{
		ratio4Button.setToggleState(idx == 0, juce::dontSendNotification);
		ratio8Button.setToggleState(idx == 1, juce::dontSendNotification);
		ratio12Button.setToggleState(idx == 2, juce::dontSendNotification);
		ratio20Button.setToggleState(idx == 3, juce::dontSendNotification);
	}
	repaint();
}
