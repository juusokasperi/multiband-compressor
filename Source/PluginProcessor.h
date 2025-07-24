#pragma once

#include <JuceHeader.h>
#include "Compressor1176.hpp"

/*
GUI:
1) controls for attack, release, ratio, i/o gain
2) add bypass
3) spectrum analyzer
4) data structs for spectrum analyzer
5) fifo usage in pluginProcessor processBlcok
6)
*/

namespace Params
{
  enum Names
  {
    Attack,
    Release,
    Ratio,
    Bypass,
    All_Buttons,
    Input_Gain,
    Output_Gain
  };

  inline const std::map<Names, juce::String>& GetParams()
  {
    static std::map<Names, juce::String> params = {
      {Attack, "Attack"},
      {Release, "Release"},
      {Ratio, "Ratio"},
      {Bypass, "Bypass"},
      {All_Buttons, "All Buttons"},
      {Input_Gain, "Input Gain"},
      {Output_Gain, "Output Gain"}
    };

    return params;
  }
};

struct CompressorBand {
  private:
    Compressor1176 compressor;
  public:
    juce::AudioParameterFloat* attack { nullptr };
    juce::AudioParameterFloat* release { nullptr };
    juce::AudioParameterChoice* ratio { nullptr };
    juce::AudioParameterFloat* inputGain { nullptr };
    juce::AudioParameterFloat* outputGain { nullptr };
    juce::AudioParameterBool* bypass { nullptr };
    juce::AudioParameterBool* allButtons { nullptr };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
      compressor.prepare(spec);
    }

    void updateCompressorSettings()
    {
      compressor.setAttack(attack->get());
      compressor.setRelease(release->get());
      compressor.setAllButtons(allButtons->get());

      compressor.setRatio(
          ratio->getCurrentChoiceName().getFloatValue());
      compressor.setInputGain(inputGain->get());
      compressor.setOutputGain(outputGain->get());
    }

    float getGainReductionDb() const { return compressor.getGainReductionDb(); }

    void process(juce::AudioBuffer<float>& buffer)
    {
      if (!bypass->get())
        compressor.process(buffer);
    }
};

//==============================================================================
/**
*/
class SeventySixCompressorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SeventySixCompressorAudioProcessor();
    ~SeventySixCompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts;

    float getGainReductionDb() const { return compressor.getGainReductionDb(); }
private:
    std::array<CompressorBand, 1> compressors;
    CompressorBand& compressor = compressors[0];

    void updateState();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SeventySixCompressorAudioProcessor)
};
