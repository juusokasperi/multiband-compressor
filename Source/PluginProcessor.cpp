/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SeventySixCompressorAudioProcessor::SeventySixCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    using namespace Params;
    const auto& params = GetParams();

    auto floatHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
      param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(params.at(paramName)));
      jassert(param != nullptr);
    };

    floatHelper(compressor.attack, Names::Attack);
    floatHelper(compressor.release, Names::Release);

    auto choiceHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        DBG("Trying to get parameter: " << params.at(paramName));
        param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(params.at(paramName)));
        DBG("Result: " << ((param == nullptr) ? "nullptr" : "not null!"));
        jassert(param != nullptr);
    };

    choiceHelper(compressor.ratio, Names::Ratio);

    auto boolHelper = [&apvts = this->apvts, &params](auto& param, const auto& paramName)
    {
        param = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter(params.at(paramName)));
        jassert(param != nullptr);
    };

    boolHelper(compressor.bypass, Names::Bypass);
    boolHelper(compressor.allButtons, Names::All_Buttons);

    floatHelper(compressor.inputGain, Names::Input_Gain);
    floatHelper(compressor.outputGain, Names::Output_Gain);
}

SeventySixCompressorAudioProcessor::~SeventySixCompressorAudioProcessor()
{
}

//==============================================================================
const juce::String SeventySixCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SeventySixCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SeventySixCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SeventySixCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SeventySixCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SeventySixCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SeventySixCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SeventySixCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SeventySixCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SeventySixCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SeventySixCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;

    for (auto &comp : compressors)
        comp.prepare(spec);

}

void SeventySixCompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SeventySixCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SeventySixCompressorAudioProcessor::updateState()
{
    for (auto& compressor : compressors)
        compressor.updateCompressorSettings();
}

void SeventySixCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    updateState();
    compressor.process(buffer);
}

//==============================================================================
bool SeventySixCompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SeventySixCompressorAudioProcessor::createEditor()
{
    return new SeventySixCompressorAudioProcessorEditor (*this);
}

//==============================================================================
void SeventySixCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SeventySixCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessorValueTreeState::ParameterLayout SeventySixCompressorAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    using namespace juce;
    using namespace Params;

    const auto& params = GetParams();

    auto gainRange = NormalisableRange<float>(-40.f, 40.f, 0.5f, 1);
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Input_Gain), params.at(Names::Input_Gain), gainRange, 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Output_Gain), params.at(Names::Output_Gain), gainRange, 0.f));

    auto attackReleaseRange = NormalisableRange<float>(1, 7, 1, 1);
    layout.add(std::make_unique<AudioParameterFloat>("Attack", "Attack", attackReleaseRange, 4));
    layout.add(std::make_unique<AudioParameterFloat>(params.at(Names::Release), params.at(Names::Release), attackReleaseRange, 4));

    auto choices = std::vector<double>{ 4, 8, 12, 20 };
    juce::StringArray sa;
    for (auto choice : choices)
        sa.add(juce::String(choice, 1));
    layout.add(std::make_unique<AudioParameterChoice>(params.at(Names::Ratio), params.at(Names::Ratio), sa, 0));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::Bypass), params.at(Names::Bypass), false));
    layout.add(std::make_unique<AudioParameterBool>(params.at(Names::All_Buttons), params.at(Names::All_Buttons), false));
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SeventySixCompressorAudioProcessor();
}
