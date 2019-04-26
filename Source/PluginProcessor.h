/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/**
*/
class KrplayerAudioProcessor  : public AudioProcessor,
								public AudioProcessorValueTreeState::Listener,
								public ChangeBroadcaster
{
public:
    //==============================================================================
    KrplayerAudioProcessor();
    ~KrplayerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void parameterChanged (const String &parameterID, float newValue) override;
    
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    void fillDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos, float startGain, float endGain);

    void fetchFromDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 readPos);

    void feedbackDelayBuffer (AudioSampleBuffer& buffer, const int channelIn, const int channelOut, const int64 writePos, float startGain, float endGain);
    
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    
    bool hasEditor() const override;
    
    

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    AudioProcessorValueTreeState& getValueTreeState();
    
    
    static String paramGain;
    static String paramTime;
    static String paramFeedback;
    

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrplayerAudioProcessor)
    

    
    Atomic<float>  xGain         {   1.0 };
    Atomic<float>  xTime         { 200.0 };
    Atomic<float>  xFeedback     {   1.0 };
    
    UndoManager                  xUndoManager;
    AudioProcessorValueTreeState xState;
    
    AudioSampleBuffer            xDelayBuffer;
    
    float xLastInputGain    = 0.0f;
    float xLastFeedbackGain = 0.0f;

    int64 xWritePos         = 0;
    double xSampleRate      = 0;

    
    
};
