#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#if JUCE_IOS

//==============================================================================
/**
*/
class PAPUAudioProcessorEditor  : public gin::ProcessorEditor
{
public:
    PAPUAudioProcessorEditor (PAPUAudioProcessor&);
    ~PAPUAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
    void paint (juce::Graphics& g) override;

    PAPUAudioProcessor& proc;
    
    gin::TriggeredScope scope { proc.fifo };

    juce::MidiKeyboardComponent keyboard { proc.state, juce::MidiKeyboardComponent::horizontalKeyboard };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PAPUAudioProcessorEditor)
};

#endif
