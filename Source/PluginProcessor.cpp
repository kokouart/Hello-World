/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


String KrplayerAudioProcessor::paramGain      ("gain");
String KrplayerAudioProcessor::paramTime      ("time");
String KrplayerAudioProcessor::paramFeedback  ("feedback");

//==============================================================================
KrplayerAudioProcessor::KrplayerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
xState (*this, &xUndoManager, "Krplayer",
          {
              std::make_unique<AudioParameterFloat>(paramGain,     TRANS ("Input Gain"),    NormalisableRange<float>(0.0,    2.0, 0.1), xGain.get()),
              std::make_unique<AudioParameterFloat>(paramTime,     TRANS ("Delay TIme"),    NormalisableRange<float>(0.0, 2000.0, 1.0), xTime.get()),
              std::make_unique<AudioParameterFloat>(paramFeedback, TRANS ("Feedback Gain"), NormalisableRange<float>(0.0,    2.0, 0.1), xFeedback.get())
          })
{
    xState.addParameterListener (paramGain, this);
    xState.addParameterListener (paramTime, this);
    xState.addParameterListener (paramFeedback, this);

}

KrplayerAudioProcessor::~KrplayerAudioProcessor()
{
}

//==============================================================================
const String KrplayerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KrplayerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KrplayerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KrplayerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KrplayerAudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int KrplayerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int KrplayerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KrplayerAudioProcessor::setCurrentProgram (int index)
{
}

const String KrplayerAudioProcessor::getProgramName (int index)
{
    return String();
}

void KrplayerAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void KrplayerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    xSampleRate = sampleRate;
    
    const int totalNumInputChannels  = getTotalNumInputChannels();
    
     // sample buffer for 2 seconds + 2 buffers safety
    xDelayBuffer.setSize (totalNumInputChannels, 2.0 * (samplesPerBlock + sampleRate), false, true);
}


void KrplayerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void KrplayerAudioProcessor::parameterChanged (const String &parameterID, float newValue)
{
    if (parameterID == paramGain) {
        xGain = newValue;
    }
    else if (parameterID == paramTime) {
        xTime = newValue;
    }
    else if (parameterID == paramFeedback) {
        xFeedback = newValue;
    }

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KrplayerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void KrplayerAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    if (Bus* inputBus = getBus (true, 0))
    {
        const float gain = xGain.get();
        const float time = xTime.get();
        const float feedback = xFeedback.get();

        for (int i=0; i < inputBus->getNumberOfChannels(); ++i) {
            const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            fillDelayBuffer (buffer, inputChannelNum, i, xWritePos, xLastInputGain, gain);
        }
        xLastInputGain = gain;

        const int64 readPos = static_cast<int64>((xDelayBuffer.getNumSamples() + xWritePos - (xSampleRate * time / 1000.0))) % xDelayBuffer.getNumSamples();

        if (Bus* outputBus = getBus (false, 0)) {
            for (int i=0; i<outputBus->getNumberOfChannels(); ++i) {
                const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer (i);

                fetchFromDelayBuffer (buffer, i % xDelayBuffer.getNumChannels(), outputChannelNum, readPos);
            }
        }

        // feedback
        for (int i=0; i<inputBus->getNumberOfChannels(); ++i) {
            const int outputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer (i);
            feedbackDelayBuffer (buffer, outputChannelNum, i, xWritePos, xLastFeedbackGain, feedback);
        }
        xLastFeedbackGain = feedback;

        xWritePos += buffer.getNumSamples();
        xWritePos %= xDelayBuffer.getNumSamples();
    }
}

void KrplayerAudioProcessor::fillDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut,
                                                const int64 writePos, float startGain, float endGain)
{
    if (xDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
        xDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else {
        const int64 midPos  = xDelayBuffer.getNumSamples() - writePos;
        const float midGain = xLastInputGain +  ((endGain - startGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
		xDelayBuffer.copyFromWithRamp (channelOut, writePos, buffer.getReadPointer (channelIn), midPos, xLastInputGain, midGain);
        xDelayBuffer.copyFromWithRamp (channelOut, 0,        buffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
    }
}

void KrplayerAudioProcessor::fetchFromDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 readPos)
{
    if (xDelayBuffer.getNumSamples() > readPos + buffer.getNumSamples()) {
        buffer.copyFrom (channelOut, 0, xDelayBuffer.getReadPointer (channelIn) + readPos, buffer.getNumSamples());
    }
    else {
        const int64 midPos  = xDelayBuffer.getNumSamples() - readPos;
        buffer.copyFrom (channelOut, 0,      xDelayBuffer.getReadPointer (channelIn) + readPos, midPos);
        buffer.copyFrom (channelOut, midPos, xDelayBuffer.getReadPointer (channelIn), buffer.getNumSamples() - midPos);
    }
}

void KrplayerAudioProcessor::feedbackDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut,
                                                     const int64 writePos, float startGain, float endGain)
                                                     
{
     if (xDelayBuffer.getNumSamples() > writePos + buffer.getNumSamples()) {
         xDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else {
        const int64 midPos  = xDelayBuffer.getNumSamples() - writePos;
        const float midGain = startGain +  ((endGain - startGain) / buffer.getNumSamples()) * (midPos / buffer.getNumSamples());
        xDelayBuffer.addFromWithRamp (channelOut, writePos, buffer.getWritePointer (channelIn), midPos, startGain, midGain);
        xDelayBuffer.addFromWithRamp (channelOut, 0,        buffer.getWritePointer (channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
    }
    
}
AudioProcessorValueTreeState& KrplayerAudioProcessor::getValueTreeState()
{
    return xState;
}

//==============================================================================
bool KrplayerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* KrplayerAudioProcessor::createEditor()
{
    return new KrplayerAudioProcessorEditor (*this);
}

//==============================================================================
void KrplayerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    MemoryOutputStream stream(destData, false);
    xState.state.writeToStream (stream);
    
}

void KrplayerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    ValueTree tree = ValueTree::readFromData (data, sizeInBytes);
    if (tree.isValid()) {
        xState.state = tree;
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KrplayerAudioProcessor();
}
