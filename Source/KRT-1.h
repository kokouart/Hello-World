/*
 
  ==============================================================================

  
  Kart@2019
  
  
  
  ==============================================================================
  
*/




#pragma once

//==============================================================================
class ProcessorBase  : public AudioProcessor
{
public:
    //==============================================================================
    ProcessorBase()  {}
    ~ProcessorBase() {}

    //==============================================================================
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override {}

    //==============================================================================
    AudioProcessorEditor* createEditor() override          { return nullptr; }
    bool hasEditor() const override                        { return false; }

    //==============================================================================
    const String getName() const override                  { return {}; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 0; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const String&) override   {}

    //==============================================================================
    void getStateInformation (MemoryBlock&) override       {}
    void setStateInformation (const void*, int) override   {}

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};

//==============================================================================
class OscillatorProcessor  : public ProcessorBase
{
public:
    OscillatorProcessor()
    {
        oscillator.setFrequency (440.0f);
        oscillator.initialise ([] (float x) { return std::sin (x); });
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock) };
        oscillator.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        oscillator.process (context);
    }

    void reset() override
    {
        oscillator.reset();
    }

    const String getName() const override { return "Oscillator"; }

private:
    dsp::Oscillator<float> oscillator;
};

//==============================================================================
class GainProcessor  : public ProcessorBase
{
public:
    GainProcessor()
    {
        gain.setGainDecibels (-6.0f);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
        gain.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        gain.process (context);
    }

    void reset() override
    {
        gain.reset();
    }

    const String getName() const override { return "Gain"; }

private:
    dsp::Gain<float> gain;
};

//==============================================================================
class FilterProcessor  : public ProcessorBase
{
public:
    FilterProcessor() {}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        *filter.state = *dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 1000.0f);

        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
        filter.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        filter.process (context);
    }

    void reset() override
    {
        filter.reset();
    }

    const String getName() const override { return "Filter"; }

private:
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> filter;
};

//==============================================================================
class MyProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    using AudioGraphIOProcessor = AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = AudioProcessorGraph::Node;

    //==============================================================================
    MyProcessor()
        : AudioProcessor (BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", AudioChannelSet::stereo(), true)),
          mainProcessor  (new AudioProcessorGraph()),
          muteInput      (new AudioParameterBool   ("mute",    "Mute Input", true)),
          processorSlot1 (new AudioParameterChoice ("slot1",   "Slot 1",     processorChoices, 0)),
          processorSlot2 (new AudioParameterChoice ("slot2",   "Slot 2",     processorChoices, 0)),
          processorSlot3 (new AudioParameterChoice ("slot3",   "Slot 3",     processorChoices, 0)),
          bypassSlot1    (new AudioParameterBool   ("bypass1", "Bypass 1",   false)),
          bypassSlot2    (new AudioParameterBool   ("bypass2", "Bypass 2",   false)),
          bypassSlot3    (new AudioParameterBool   ("bypass3", "Bypass 3",   false))
    {
        addParameter (muteInput);

        addParameter (processorSlot1);
        addParameter (processorSlot2);
        addParameter (processorSlot3);

		addParameter (gain = new AudioParameterFloat ("gain", // parameter ID
													  "Gain", // parameter name
													 NormalisableRange<float> (0.0f, 1.0f),
													  0.5f)); // default value
		addParameter (invertPhase = new AudioParameterBool  ("invertPhase", "Invert Phase", false));

        addParameter (bypassSlot1);
        addParameter (bypassSlot2);
        addParameter (bypassSlot3);
    }

    ~MyProcessor() {}

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        if (layouts.getMainInputChannelSet()  == AudioChannelSet::disabled()
         || layouts.getMainOutputChannelSet() == AudioChannelSet::disabled())
            return false;

        if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
            return false;

        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {

		previousGain = *gain;

		mainProcessor->setPlayConfigDetails (getMainBusNumInputChannels(),
                                             getMainBusNumOutputChannels(),
                                             sampleRate, samplesPerBlock);

        mainProcessor->prepareToPlay (sampleRate, samplesPerBlock);

		auto phase = *invertPhase ? -1.0f : 1.0f;
		previousGain = *gain * phase;

        initialiseGraph();
    }

    void releaseResources() override
    {
        mainProcessor->releaseResources();
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) override
    {

		// interact with these parameter objects
		buffer.applyGain (*gain);

		// Smoothing gain changes
        for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
            buffer.clear (i, 0, buffer.getNumSamples());

		auto phase = *invertPhase ? -1.0f : 1.0f;

		// function to perform the gain ramp:
		auto currentGain = *gain * phase;

		if (currentGain == previousGain)
		{
			buffer.applyGain (currentGain);
		}
		else
		{
			buffer.applyGainRamp (0, buffer.getNumSamples(), previousGain, currentGain);
			previousGain = currentGain;
		}

        updateGraph();

        mainProcessor->processBlock (buffer, midiMessages);
    }

    //==============================================================================
    AudioProcessorEditor* createEditor() override          { return new GenericAudioProcessorEditor (this); }
    bool hasEditor() const override                        { return true; }

    //==============================================================================
    const String getName() const override                  { return "Param"; }
    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return true; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const String&) override   {}

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override
	{

		// Using XML to store the processor's state
		std::unique_ptr<XmlElement> xml (new XmlElement ("Param"));
		xml->setAttribute ("gain", (double) *gain);
		xml->setAttribute ("invertPhase", *invertPhase);

		// Storing and retrieving parameters
		MemoryOutputStream (destData, true).writeFloat (*gain);
		copyXmlToBinary (*xml, destData);
		
	}
    void setStateInformation (const void* data, int sizeInBytes) override
	{

		//read data from a memory location and restore the state of the plug-in
		*gain = MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();

		std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

		if (xmlState.get() != nullptr)
		{
			if (xmlState->hasTagName ("Param"))
			{
				*gain = (float) xmlState->getDoubleAttribute ("gain", 1.0);
				*invertPhase = xmlState->getBoolAttribute ("invertPhase", false);
			}
		}
	}

private:
    //==============================================================================
    void initialiseGraph()
    {
        mainProcessor->clear();

        audioInputNode  = mainProcessor->addNode (new AudioGraphIOProcessor (AudioGraphIOProcessor::audioInputNode));
        audioOutputNode = mainProcessor->addNode (new AudioGraphIOProcessor (AudioGraphIOProcessor::audioOutputNode));
        midiInputNode   = mainProcessor->addNode (new AudioGraphIOProcessor (AudioGraphIOProcessor::midiInputNode));
        midiOutputNode  = mainProcessor->addNode (new AudioGraphIOProcessor (AudioGraphIOProcessor::midiOutputNode));

        connectAudioNodes();
        connectMidiNodes();
    }

    void updateGraph()
    {
        bool hasChanged = false;

        Array<AudioParameterChoice*> choices { processorSlot1,
                                               processorSlot2,
                                               processorSlot3 };

        Array<AudioParameterBool*> bypasses { bypassSlot1,
                                              bypassSlot2,
                                              bypassSlot3 };

        ReferenceCountedArray<Node> slots;
        slots.add (slot1Node);
        slots.add (slot2Node);
        slots.add (slot3Node);

        for (int i = 0; i < 3; ++i)
        {
            auto& choice = choices.getReference (i);
            auto  slot   = slots  .getUnchecked (i);

            if (choice->getIndex() == 0)
            {
                if (slot != nullptr)
                {
                    mainProcessor->removeNode (slot);
                    slots.set (i, nullptr);
                    hasChanged = true;
                }
            }
            else if (choice->getIndex() == 1)
            {
                if (slot != nullptr)
                {
                    if (slot->getProcessor()->getName() == "Oscillator")
                        continue;

                    mainProcessor->removeNode (slot);
                }

                slots.set (i, mainProcessor->addNode (new OscillatorProcessor()));
                hasChanged = true;
            }
            else if (choice->getIndex() == 2)
            {
                if (slot != nullptr)
                {
                    if (slot->getProcessor()->getName() == "Gain")
                        continue;

                    mainProcessor->removeNode (slot);
                }

                slots.set (i, mainProcessor->addNode (new GainProcessor()));
                hasChanged = true;
            }
            else if (choice->getIndex() == 3)
            {
                if (slot != nullptr)
                {
                    if (slot->getProcessor()->getName() == "Filter")
                        continue;

                    mainProcessor->removeNode (slot);
                }

                slots.set (i, mainProcessor->addNode (new FilterProcessor()));
                hasChanged = true;
            }
        }

        if (hasChanged)
        {
            for (auto connection : mainProcessor->getConnections())
                mainProcessor->removeConnection (connection);

            ReferenceCountedArray<Node> activeSlots;

            for (auto slot : slots)
            {
                if (slot != nullptr)
                {
                    activeSlots.add (slot);

                    slot->getProcessor()->setPlayConfigDetails (getMainBusNumInputChannels(),
                                                                getMainBusNumOutputChannels(),
                                                                getSampleRate(), getBlockSize());
                }
            }

            if (activeSlots.isEmpty())
            {
                connectAudioNodes();
            }
            else
            {
                for (int i = 0; i < activeSlots.size() - 1; ++i)
                {
                    for (int channel = 0; channel < 2; ++channel)
                        mainProcessor->addConnection ({ { activeSlots.getUnchecked (i)->nodeID,      channel },
                                                        { activeSlots.getUnchecked (i + 1)->nodeID,  channel } });
                }

                for (int channel = 0; channel < 2; ++channel)
                {
                    mainProcessor->addConnection ({ { audioInputNode->nodeID,         channel },
                                                    { activeSlots.getFirst()->nodeID, channel } });
                    mainProcessor->addConnection ({ { activeSlots.getLast()->nodeID,  channel },
                                                    { audioOutputNode->nodeID,        channel } });
                }
            }

            connectMidiNodes();

            for (auto node : mainProcessor->getNodes())
                node->getProcessor()->enableAllBuses();
        }

        for (int i = 0; i < 3; ++i)
        {
            auto  slot   = slots   .getUnchecked (i);
            auto& bypass = bypasses.getReference (i);

            if (slot != nullptr)
                slot->setBypassed (bypass->get());
        }

        audioInputNode->setBypassed (muteInput->get());

        slot1Node = slots.getUnchecked (0);
        slot2Node = slots.getUnchecked (1);
        slot3Node = slots.getUnchecked (2);
    }

    void connectAudioNodes()
    {
        for (int channel = 0; channel < 2; ++channel)
            mainProcessor->addConnection ({ { audioInputNode->nodeID,  channel },
                                            { audioOutputNode->nodeID, channel } });
    }

    void connectMidiNodes()
    {
        mainProcessor->addConnection ({ { midiInputNode->nodeID,  AudioProcessorGraph::midiChannelIndex },
                                        { midiOutputNode->nodeID, AudioProcessorGraph::midiChannelIndex } });
    }

    //==============================================================================
    StringArray processorChoices { "Empty", "Oscillator", "Gain", "Filter" };

    std::unique_ptr<AudioProcessorGraph> mainProcessor;

    AudioParameterBool* muteInput;

    AudioParameterChoice* processorSlot1;
    AudioParameterChoice* processorSlot2;
    AudioParameterChoice* processorSlot3;

	AudioParameterFloat* gain;
	AudioParameterBool* invertPhase;
	float previousGain;

    AudioParameterBool* bypassSlot1;
    AudioParameterBool* bypassSlot2;
    AudioParameterBool* bypassSlot3;

    Node::Ptr audioInputNode;
    Node::Ptr audioOutputNode;
    Node::Ptr midiInputNode;
    Node::Ptr midiOutputNode;

    Node::Ptr slot1Node;
    Node::Ptr slot2Node;
    Node::Ptr slot3Node;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyProcessor)
};
