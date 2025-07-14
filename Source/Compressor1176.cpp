#include "Compressor1176.hpp"

Compressor1176::Compressor1176() {}

static float clamp(float value)
{
	if (value < 1.0f)
		return 1.0f;
	else if (value > 7.0f)
		return 7.0f;
	else
		return value;
}

float Compressor1176::mapAttackMs(float knobValue)
{
	return juce::jmap(clamp(knobValue), 1.0f, 7.0f, 0.8f, 0.2f);
}

float Compressor1176::mapReleaseMs(float knobValue)
{
	return juce::jmap(clamp(knobValue), 1.0f, 7.0f, 1100.0f, 50.0f);
}
void Compressor1176::setInputGain(float newInputGain) { inputGain = newInputGain; }
void Compressor1176::setRatio(float newRatio) { ratio = newRatio; }

void Compressor1176::setAttack(float knobValue)
{
	attackTime = mapAttackMs(knobValue);
}
void Compressor1176::setRelease(float knobValue)
{
	releaseTime = mapReleaseMs(knobValue);
}
void Compressor1176::setOutputGain(float newOutputGain) { outputGain = newOutputGain; }

void Compressor1176::prepare( const juce::dsp::ProcessSpec& spec)
{
	sampleRate = spec.sampleRate;
	envelope.resize(spec.numChannels, 0.0f);

	lowShelfFilter.reset();
	highShelfFilter.reset();
}

void Compressor1176::reset()
{
	std::fill(envelope.begin(), envelope.end(), 0.0f);
	lowShelfFilter.reset();
	highShelfFilter.reset();
}

float Compressor1176::getThreshold()
{
	if (ratio == 4.0f)	return -17.5f;
	if (ratio == 8.0f)	return -12.8f;
	if (ratio == 12.0f)	return -9.6f;
	if (ratio == 20.0f)	return -8.4f;

	return (-17.5f);
}

float Compressor1176::computeGainReduction(float level)
{
	float threshold = getThreshold();
	float inputLevelDb = juce::Decibels::gainToDecibels(level);
	float overThreshold = inputLevelDb - threshold;
	if (overThreshold <= 0.0f)
		return 1.0f;

	float compressedDb = overThreshold / ratio;
	float gainReductionDb = overThreshold - compressedDb;
	return juce::Decibels::decibelsToGain(-gainReductionDb);
}

float Compressor1176::getSmoothingCoeff(float timeMs)
{
	return 1.0f - std::exp(-1.0f / (0.001f * timeMs * sampleRate));
}

float Compressor1176::processRMS(int ch, float sample)
{
	static constexpr float beta = 0.01f;
	float prevRms = envelope[ch];
	float rmsSq = (1.0f - beta) * prevRms * prevRms + beta * sample * sample;
	envelope[ch] = std::sqrt(rmsSq);
	return 20.0f * std::log10(envelope[ch] + 1e-12f);
}

void Compressor1176::process(juce::AudioBuffer<float>& buffer)
{
	float maxBoost = 2.0f;
	float boostDb = juce::jmap(1.0f - smoothedGainReduction, 0.0f, 1.0f, 0.0f, maxBoost);
	lowShelfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
		sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
	highShelfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
		sampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
	for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
	{
		auto* data = buffer.getWritePointer(ch);
		for (int i = 0; i < buffer.getNumSamples(); ++i)
		{
			float sample = data[i] * juce::Decibels::decibelsToGain(inputGain);

			float inputLevelDb = processRMS(ch, sample);
			float targetGainReduction = computeGainReduction(inputLevelDb);
			float coeff = (targetGainReduction < smoothedGainReduction)
				? getSmoothingCoeff(attackTime)
				: getSmoothingCoeff(releaseTime);
			smoothedGainReduction = coeff * targetGainReduction + (1.0f - coeff) * smoothedGainReduction;
			float processed = sample * smoothedGainReduction * juce::Decibels::decibelsToGain(outputGain);
			if (smoothedGainReduction < 1.0f)
			{
				processed = lowShelfFilter.processSample(processed);
				processed = highShelfFilter.processSample(processed);
			}
			data[i] = processed;
		}
	}
}
