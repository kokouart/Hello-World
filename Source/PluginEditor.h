/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class KrplayerAudioProcessorEditor  : public AudioProcessorEditor
//                                      public ChangeListener,
//                                      public Timer
{
public:
    KrplayerAudioProcessorEditor (KrplayerAudioProcessor&);
    ~KrplayerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
//    void changeListenerCallback (ChangeBroadcaster* sender) override;
//    void timerCallback() override;

//    void mouseDown (const MouseEvent& e) override;

//    void mouseMove (const MouseEvent& e) override;
//    void mouseDrag (const MouseEvent& e) override;

//    void mouseDoubleClick (const MouseEvent& e) override;

    

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    KrplayerAudioProcessor& processor;
    
    #ifdef JUCE_OPENGL
    OpenGLContext           openGLContext;
    #endif
    
    ScopedPointer<Slider> xGainSlider;
    ScopedPointer<Slider> xTimeSlider;
    ScopedPointer<Slider> xFeedbackSlider;

    
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> xGainAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> xTimeAttachment;
    ScopedPointer<AudioProcessorValueTreeState::SliderAttachment> xFeedbackAttachment;
    


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KrplayerAudioProcessorEditor)
};
