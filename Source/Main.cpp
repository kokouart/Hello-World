/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "KRT-1.h"

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyProcessor();
}
