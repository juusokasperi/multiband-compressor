#pragma once
#include <JuceHeader.h>

class Compressor1176
{
	public:
		Compressor1176();

		void setInputGain(float newInputGain);
		void setRatio(float newRatio);
		void setAttack(float knobValue);
		void setRelease(float knobValue);
		void setOutputGain(float newOutputGain);

		float getSmoothingCoeff(float timeMs);
		float getThreshold();
		float mapAttackMs(float knobValue);
		float mapReleaseMs(float knobValue);

		float processRMS(int ch, float sample);
		void prepare(const juce::dsp::ProcessSpec& spec);
		void reset();

		void process(juce::AudioBuffer<float>& buffer);
	private:
		float inputGain = 0.0f;
		float outputGain = 0.0f;
		float ratio = 4.0f;
		float attackTime = 0.5f;
		float releaseTime = 600.0f;
		float smoothedGainReduction = 1.0f;

		double sampleRate = 44100.0;

		std::vector<float> envelope;
		juce::dsp::IIR::Filter<float> lowShelfFilter;
		juce::dsp::IIR::Filter<float> highShelfFilter;

		float computeGainReduction(float level);
};
