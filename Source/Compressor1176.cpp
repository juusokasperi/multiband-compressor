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
	return juce::jmap(clamp(knobValue), 1.0f, 7.0f, 0.8f, 0.02f);
}

float Compressor1176::mapReleaseMs(float knobValue)
{
	return juce::jmap(clamp(knobValue), 1.0f, 7.0f, 1100.0f, 50.0f);
}
void Compressor1176::setInputGain(float newInputGain) { inputGain = newInputGain; }
void Compressor1176::setRatio(float newRatio) { ratio = newRatio; }
void Compressor1176::setAllButtons(bool newValue) { allButtonsMode = newValue; }

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
	overSampledRate = sampleRate * overSamplingFactor;

	envelope.clear();
	envelope.resize(numChannels, 0.0f);
	smoothedGainReduction.clear();
	smoothedGainReduction.resize(numChannels, 1.0f);
	overSampling.initProcessing(static_cast<size_t>(spec.maximumBlockSize));

	juce::dsp::ProcessSpec overSampledSpec;
	overSampledSpec.sampleRate = overSampledRate;
	overSampledSpec.maximumBlockSize = spec.maximumBlockSize * static_cast<int>(overSamplingFactor);
	overSampledSpec.numChannels = spec.numChannels;

	transientDetector.resize(numChannels, 0.0f);
	slowEnvelope.resize(numChannels, 0.0f);

	lowShelfFilter.clear();
	highShelfFilter.clear();
	lowShelfFilter.resize(numChannels);
	highShelfFilter.resize(numChannels);
	lastBoostDb.resize(numChannels, 0.0f);
	compressionHistory.resize(numChannels, 0.0f);
	for (int ch = 0; ch < numChannels; ++ch)
	{
		lowShelfFilter[ch].prepare(overSampledSpec);
		highShelfFilter[ch].prepare(overSampledSpec);
		lowShelfFilter[ch].reset();
		highShelfFilter[ch].reset();
	};
	initFETLookup();
}

void Compressor1176::reset()
{
	overSampling.reset();
	envelope.resize(numChannels, 0.0f);
	smoothedGainReduction.resize(numChannels, 1.0f);
	transientDetector.resize(numChannels, 0.0f);
	slowEnvelope.resize(numChannels, 0.0f);
	ratioModulation = 0.0f;
	lowShelfFilter.clear();
	highShelfFilter.clear();
	for (int ch = 0; ch < numChannels; ++ch)
	{
		lowShelfFilter[ch].reset();
		highShelfFilter[ch].reset();
	}
	lastBoostDb.resize(numChannels, 0.0f);
	lastGainReductionDb = 0.f;
	compressionHistory.resize(numChannels, 0.0f);
}

float Compressor1176::getThreshold()
{
	if (allButtonsMode)
	{
		float baseThreshold = -18.f;
		float modulation = ratioModulation * 2.0f;
		return baseThreshold + modulation;
	}
	if (ratio == 4.0f)	return -15.f;
	if (ratio == 8.0f)	return -10.8f;
	if (ratio == 12.0f)	return -9.6f;
	if (ratio == 20.0f)	return -7.6f;

	return (-15.f);
}

float Compressor1176::getRatio()
{
	if (allButtonsMode)
	{
		float baseRatio = 16.0f;
		float variation = ratioModulation * 4.0f;
		return std::clamp(baseRatio + variation, 12.0f, 20.0f);
	}
	return ratio;
}

float Compressor1176::computeGainReduction(float level)
{
	float threshold = getThreshold();
	float inputLevelDb = juce::Decibels::gainToDecibels(level + 1e-12f);
	if (inputLevelDb <= threshold)
		return 1.0f;

	float effectiveRatio = getRatio();
	float outputLevelDb = threshold + (inputLevelDb - threshold) / effectiveRatio;
	float gainReductionDb = inputLevelDb - outputLevelDb;
	gainReductionDb = std::clamp(gainReductionDb, 0.0f, 60.0f);
	return juce::Decibels::decibelsToGain(-gainReductionDb);
}

float Compressor1176::getSmoothingCoeff(float timeMs)
{
	if (sampleRate <= 0 || timeMs <= 0)
		return 0.0f;
	return 1.0f - std::exp(-1.0f / (0.001f * timeMs * overSampledRate));
}

// Changing the beta to smaller values makes the detection slower
// Currently its quite fast
// float Compressor1176::processRMS(int ch, float sample)
// {
// 	if (ch < 0 || ch >= static_cast<int>(envelope.size()))
// 		return 0.0f;

// 	float beta = 0.5f / overSamplingFactor;
// 	float prevRms = envelope[ch];
// 	float rmsSq = (1.0f - beta) * prevRms * prevRms + beta * sample * sample;
// 	envelope[ch] = std::sqrt(std::max(0.0f, rmsSq));
// 	return envelope[ch];
// }

float Compressor1176::processPeak(int ch, float sample)
{
	if (ch < 0 || ch >= static_cast<int>(envelope.size()))
		return 0.0f;

	float absSample = std::abs(sample);
	float attackCoeff = getSmoothingCoeff(0.02f);
	float releaseCoeff = getSmoothingCoeff(1.5f);
	if (absSample > envelope[ch])
		envelope[ch] = attackCoeff * absSample + (1.0f - attackCoeff) * envelope[ch];
	else
		envelope[ch] = releaseCoeff * absSample + (1.0f - releaseCoeff) * envelope[ch];
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

// The input gain is compensated w/ +12.0f (and later in output gain -12.0f)
void Compressor1176::process(juce::AudioBuffer<float>& buffer)
{
	juce::dsp::AudioBlock<float> inputBlock(buffer);
	juce::dsp::AudioBlock<float> oversampledBlock = overSampling.processSamplesUp(inputBlock);
	float maxGrDb = 0.f;
	for (int ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
	{
		float* data = oversampledBlock.getChannelPointer(ch);
		size_t numSamples = oversampledBlock.getNumSamples();

		for (size_t i = 0; i < numSamples; ++i)
		{
			float sample = data[i] * juce::Decibels::decibelsToGain(inputGain + 12.0f);

			if (allButtonsMode)
			{
				float absSample = std::abs(sample);
				float fastCoeff = getSmoothingCoeff(0.5f);
				float slowCoeff = getSmoothingCoeff(50.0f);
				transientDetector[ch] = fastCoeff * absSample + (1.0f - fastCoeff) * transientDetector[ch];
				slowEnvelope[ch] = slowCoeff * absSample + (1.0f - slowCoeff) * slowEnvelope[ch];
				float transientRatio = transientDetector[ch] / (slowEnvelope[ch] + 1e-6f);
				bool isTransient = transientRatio > 1.3f;
				if (isTransient)
				{
					ratioModulation = std::clamp((transientRatio - 1.5f) * 0.3f, -0.5f, 0.5f);
				}
				else
				{
					ratioModulation *= 0.995f;
				}
				sample = lookupFET(sample * 1.15f);
			}
			sample *= smoothedGainReduction[ch];
			float peakLevel = processPeak(ch, sample);
			sample = lookupFET(sample);
			if (smoothedGainReduction[ch] < 0.95f)
			{
				float maxBoost = 1.0f;
				float boostDb = juce::jmap(1.0f - smoothedGainReduction[ch], 0.0f, 1.0f, 0.0f, maxBoost);
				if (std::abs(boostDb - lastBoostDb[ch]) > 0.1f)
				{
					lowShelfFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
						overSampledRate, 100.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
					highShelfFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
						overSampledRate, 8000.0f, 0.707f, juce::Decibels::decibelsToGain(boostDb));
					lastBoostDb[ch] = boostDb;
				}
				sample = lowShelfFilter[ch].processSample(sample);
				sample = highShelfFilter[ch].processSample(sample);
			}
			float targetGainReduction = computeGainReduction(peakLevel);
			float tempGrDb = juce::Decibels::gainToDecibels(targetGainReduction + 1e-12f);
			maxGrDb = std::min(maxGrDb, tempGrDb);

			float effectiveAttackTime = attackTime;
			float effectiveReleaseTime = releaseTime;
			if (allButtonsMode)
			{
				effectiveAttackTime *= (1.0f + ratioModulation * 0.3f);
				effectiveReleaseTime *= (1.0f - ratioModulation * 0.2f);
				effectiveAttackTime = std::clamp(effectiveAttackTime, 0.005f, 2.0f);
				effectiveReleaseTime = std::clamp(effectiveReleaseTime, 15.0f, 1000.0f);
			}

			float compressionAmount = 1.0f - targetGainReduction;
			if (compressionAmount > 0.05f)
			{
				float buildUpdRate = 2.0f / static_cast<float>(overSampledRate);
				compressionHistory[ch] = std::min(compressionHistory[ch] + buildUpdRate, 1.0f);
			}
			else
				compressionHistory[ch] *= 0.999f;
			float programDependentRelease = effectiveReleaseTime * (1.0f - compressionHistory[ch] * 0.6f);

			float coeff = (targetGainReduction < smoothedGainReduction[ch])
				? getSmoothingCoeff(effectiveAttackTime)
				: getSmoothingCoeff(programDependentRelease);
			smoothedGainReduction[ch] = coeff * targetGainReduction + (1.0f - coeff) * smoothedGainReduction[ch];

			sample *= juce::Decibels::decibelsToGain(outputGain - 12.0f);
			if (std::isnan(sample) || std::isinf(sample))
				sample = 0.0f;
			sample = softClip(sample);
			data[i] = sample;
		}
	}
	lastGainReductionDb.store(maxGrDb);
	overSampling.processSamplesDown(inputBlock);
}

// Smaller asym value equals less colouration, bigger more.
float Compressor1176::saturateFET(float x, float drive)
{
	float threshold = 0.7f;
	float scaledInput = x / threshold;

	float asym = 0.3f;

	float linearPart = scaledInput;
	float saturatedPart = 0;

	if (scaledInput >= 0.0f)
		saturatedPart = std::tanh(drive * scaledInput);
	else
		saturatedPart = std::tanh(drive * (scaledInput + asym * scaledInput));

	float blend = std::min(std::abs(scaledInput) * drive, 1.0f);
	float result = (1.0f - blend) * linearPart + blend * saturatedPart;
	return result * threshold;
}

// Adjust the saturateFET second variable for more/less colouration.
void Compressor1176::initFETLookup()
{
	fetLUT.resize(FET_LOOKUP_SIZE);
	for (int i = 0; i < FET_LOOKUP_SIZE; ++i)
	{
		float x = -2.0f + 4.0f * (i / static_cast<float>(FET_LOOKUP_SIZE - 1));
		fetLUT[i] = saturateFET(x, 0.5f);
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
	if (fetLUT.size() < 4)
		return 0.0f;

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
