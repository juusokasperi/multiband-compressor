#pragma once
#include <JuceHeader.h>

static constexpr int FET_LOOKUP_SIZE = 1024;

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

		// Fet
		float saturateFET(float x);
		void initFETLookup();
		float cubicInterpolate(float y0, float y1, float y2, float y3, float x);
		float lookupFET(float x);
		float softClip(float x);
	private:
		float inputGain = 0.0f;
		float outputGain = 0.0f;
		float ratio = 4.0f;
		float attackTime = 0.5f;
		float releaseTime = 600.0f;
		std::vector<float> smoothedGainReduction;
		int numChannels = 2;

		double sampleRate = 44100.0;

		std::vector<float> envelope;
		std::vector<juce::dsp::IIR::Filter<float>> lowShelfFilter;
		std::vector<juce::dsp::IIR::Filter<float>> highShelfFilter;
		std::vector<float> lastBoostDb;
		std::vector<float> fetLUT;

		juce::dsp::Oversampling<float> overSampling {
			2,
			2,
			juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
		};

		float computeGainReduction(float level);
};
