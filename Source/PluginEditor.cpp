/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//static int   clickRadius = 4;

//==============================================================================
KrplayerAudioProcessorEditor::KrplayerAudioProcessorEditor (KrplayerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.



    xGainSlider      = new Slider (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
    xTimeSlider      = new Slider (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
    xFeedbackSlider  = new Slider (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
    

    
    xGainAttachment      = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), KrplayerAudioProcessor::paramGain, *xGainSlider);
    xTimeAttachment      = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), KrplayerAudioProcessor::paramTime, *xTimeSlider);
    xFeedbackAttachment  = new AudioProcessorValueTreeState::SliderAttachment (p.getValueTreeState(), KrplayerAudioProcessor::paramFeedback, *xFeedbackSlider);
    

	xGainSlider->setPopupDisplayEnabled(true, true, this);
	xTimeSlider->setPopupDisplayEnabled(true, true, this);
	xFeedbackSlider->setPopupDisplayEnabled(true, true, this);

    addAndMakeVisible  (xGainSlider);
	addAndMakeVisible  (xTimeSlider);
	addAndMakeVisible  (xFeedbackSlider);



	setResizable (true, true);
//	setSize (size.x, size.y);
	setResizeLimits (600, 250, 2770, 1400);

    
	setSize (800, 450);
    

}


KrplayerAudioProcessorEditor::~KrplayerAudioProcessorEditor()
{

}

//==============================================================================
void KrplayerAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

	g.fillAll  (Colours::darkkhaki);
    

	g.setColour(Colours::black);
	g.setFont(18.0f);
	Rectangle<int> box (getX(), getBottom() - 40, getWidth() / 3, 40);
	g.drawFittedText(TRANS("Gain"), box, Justification::centred, 1);
	box.setX(box.getRight());
	g.drawFittedText(TRANS("Time"), box, Justification::centred, 1);
	box.setX(box.getRight());
	g.drawFittedText(TRANS("Feedback"), box, Justification::centred, 1);

    

}

void KrplayerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto box = getLocalBounds();
    box.setWidth (getWidth() / 3);
    box.setHeight (getHeight() - 40);
    xGainSlider->setBounds (box);
    box.setX (box.getRight());
    xTimeSlider->setBounds (box);
    box.setX (box.getRight());
    xFeedbackSlider->setBounds (box);



}
