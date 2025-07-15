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
	if (spec.sampleRate <= 0 || spec.numChannels == 0 || spec.maximumBlockSize == 0)
	{
		jassertfalse;
		return;
	}
	sampleRate = spec.sampleRate;
	numChannels = static_cast<int>(spec.numChannels);

	envelope.clear();
	envelope.resize(numChannels, 0.0f);
	smoothedGainReduction.clear();
	smoothedGainReduction.resize(numChannels, 1.0f);
	overSampling.initProcessing(static_cast<size_t>(spec.maximumBlockSize));
	// lowShelfFilter.reset();
	// highShelfFilter.reset();
	initFETLookup();
}

void Compressor1176::reset()
{
	overSampling.reset();
	envelope.resize(numChannels, 0.0f);
	smoothedGainReduction.resize(numChannels, 1.0f);
	// lowShelfFilter.reset();
	// highShelfFilter.reset();
}

float Compressor1176::getThreshold()
{
	if (ratio == 4.0f)	return -15.f;
	if (ratio == 8.0f)	return -10.8f;
	if (ratio == 12.0f)	return -9.6f;
	if (ratio == 20.0f)	return -7.6f;

	return (-15.f);
}

float Compressor1176::computeGainReduction(float level)
{
	float threshold = getThreshold();
	float inputLevelDb = juce::Decibels::gainToDecibels(level + 1e-12f);
	float overThreshold = inputLevelDb - threshold;
	if (overThreshold <= 0.0f)
		return 1.0f;

	float compressedDb = overThreshold / ratio;
	float gainReductionDb = overThreshold - compressedDb;
	gainReductionDb = std::clamp(gainReductionDb, 0.0f, 60.0f);
	return juce::Decibels::decibelsToGain(-gainReductionDb);
}

float Compressor1176::getSmoothingCoeff(float timeMs)
{
	if (sampleRate <= 0 || timeMs <= 0)
		return 0.0f;
	return 1.0f - std::exp(-1.0f / (0.001f * timeMs * sampleRate));
}

float Compressor1176::processRMS(int ch, float sample)
{
	if (ch < 0 || ch >= static_cast<int>(envelope.size()))
		return 0.0f;

	float beta = 0.1f;
	float prevRms = envelope[ch];
	float rmsSq = (1.0f - beta) * prevRms * prevRms + beta * sample * sample;
	envelope[ch] = std::sqrt(std::max(0.0f, rmsSq));
	return envelope[ch];
}

float Compressor1176::softClip(float x)
{
	const float threshold = 0.98f;
	if (x > threshold)
		return threshold + std::tanh(x - threshold) * 0.05f;
	else if (x < -threshold)
		return -threshold + std::tanh(x + threshold) * 0.05f;
	else
		return x;
}

void Compressor1176::process(juce::AudioBuffer<float>& buffer)
{
	for (int ch = 0; ch < numChannels; ++ch)
	{
		float* data = buffer.getWritePointer(ch);
		size_t numSamples = buffer.getNumSamples();

		for (size_t i = 0; i < numSamples; ++i)
		{
			float sample = data[i] * juce::Decibels::decibelsToGain(inputGain);
			float rmsLevel = processRMS(ch, sample);

			float targetGainReduction = computeGainReduction(rmsLevel);
			float coeff = (targetGainReduction < smoothedGainReduction[ch])
				? getSmoothingCoeff(attackTime)
				: getSmoothingCoeff(releaseTime);
			smoothedGainReduction[ch] = coeff * targetGainReduction + (1.0f - coeff) * smoothedGainReduction[ch];
			sample *= smoothedGainReduction[ch];
			sample *= juce::Decibels::decibelsToGain(outputGain);
			data[i] = sample;
		}
	}
	// juce::dsp::AudioBlock<float> inputBlock(buffer);
	// juce::dsp::AudioBlock<float> oversampledBlock = overSampling.processSamplesUp(buffer);
	// for (int ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
	// {
	// 	float* data = oversampledBlock.getChannelPointer(ch);
	// 	size_t numSamples = oversampledBlock.getNumSamples();

	// 	for (size_t i = 0; i < numSamples; ++i)
	// 	{
	// 		float sample = data[i] * juce::Decibels::decibelsToGain(inputGain);
	// 		sample *= smoothedGainReduction[ch];

	// 		sample = lookupFET(sample);
	// // 		if (smoothedGainReduction[ch] < 0.95f)
	// // 		{
	// // 			float maxBoost = 2.0f;
	// // 			float boostDb = juce::jmap(1.0f - smoothedGainReduction[ch], 0.0f, 1.0f, 0.0f, maxBoost);
	// // 			static float lastBoostDb = 0.0f;
	// // 			if (std::abs(boostDb - lastBoostDb) > 0.1f)
	// // 			{
	// // 				lowShelfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
	// // 					sampleRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
	// // 				highShelfFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
	// // 					sampleRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
	// // 				lastBoostDb = boostDb;
	// // 			}

	// // 			processed = lowShelfFilter.processSample(processed);
	// // 			processed = highShelfFilter.processSample(processed);
	// // 		}

	// 		sample *= juce::Decibels::decibelsToGain(outputGain);
	// 		if (std::isnan(sample) || std::isinf(sample))
	// 			sample = 0.0f;
	// 		sample = softClip(sample);
	// 		data[i] = sample;
	// 	}
	// }
	// juce::dsp::AudioBlock<float> outputBlock(buffer);
	// overSampling.processSamplesDown(outputBlock);
}

float Compressor1176::saturateFET(float x, float drive)
{
	float threshold = 0.7f;
	float scaledInput = x / threshold;

	float asym = 0.15f;

	float saturated = 0;

	if (scaledInput >= 0.0f)
		saturated = std::tanh(drive * scaledInput);
	else
		saturated = std::tanh(drive * (scaledInput + asym * scaledInput));
	return saturated * threshold;
}

void Compressor1176::initFETLookup()
{
	fetLUT.resize(FET_LOOKUP_SIZE);
	for (int i = 0; i < FET_LOOKUP_SIZE; ++i)
	{
		float x = -2.0f + 4.0f * (i / static_cast<float>(FET_LOOKUP_SIZE - 1));
		fetLUT[i] = saturateFET(x, 1.0f);
	}
}

float Compressor1176::cubicInterpolate(float y0, float y1, float y2, float y3, float x)
{
	float a = (-0.5f * y0) + (1.5f * y1) - (1.5f * y2) + (0.5f * y3);
	float b = y0 - (2.5f * y1) + (2.0f * y2) - (0.5f * y3);
	float c = (-0.5f * y0) + (0.5f * y2);
	float d = y1;
	return a * x * x * x + b * x * x + c * x + d;
}

float Compressor1176::lookupFET(float x)
{
	x = std::clamp(x, -2.0f, 2.0f);
	float norm = (x + 2.0f) / 4.0f;
	float index = norm * (FET_LOOKUP_SIZE - 1);

	int i1 = static_cast<int>(index);
	float frac = index - i1;
	int i0 = std::max(0, i1 - 1);
	int i2 = std::min(i1 + 1, FET_LOOKUP_SIZE - 1);
	int i3 = std::min(i1 + 2, FET_LOOKUP_SIZE - 1);

	return cubicInterpolate(fetLUT[i0], fetLUT[i1], fetLUT[i2], fetLUT[i3], frac);
}
