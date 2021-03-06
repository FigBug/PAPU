#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

#if ! JUCE_IOS

//==============================================================================
PAPUAudioProcessorEditor::PAPUAudioProcessorEditor (PAPUAudioProcessor& p)
  : ProcessorEditor (p, 60, 100), proc (p)
{
    additionalProgramming = "Shay Green";
    
    logo = juce::ImageFileFormat::loadFrom (BinaryData::logo_png, BinaryData::logo_pngSize);
    
    addAndMakeVisible (&scope);
    
    for (auto pp : p.getPluginParameters())
    {
        gin::ParamComponent* c;
        if (pp->getUid().contains ("tune") || pp->getUid().contains ("fine") || pp->getUid().contains ("sweep"))
            c = new gin::Knob (pp, true);
        else
            c = pp->isOnOff() ? (gin::ParamComponent*)new gin::Switch (pp) : (gin::ParamComponent*)new gin::Knob (pp);
        
        addAndMakeVisible (c);
        controls.add (c);
    }
    
    setGridSize (13, 3);
    
    scope.setNumSamplesPerPixel (2);
    scope.setVerticalZoomFactor (3.0f);
}

PAPUAudioProcessorEditor::~PAPUAudioProcessorEditor()
{
}

//==============================================================================
void PAPUAudioProcessorEditor::paint (juce::Graphics& g)
{
    ProcessorEditor::paint (g);
        
    g.drawImageAt (logo, getWidth() / 2 - logo.getWidth() / 2, 0);
}

void PAPUAudioProcessorEditor::resized()
{
    ProcessorEditor::resized();
    
    for (int i = 0; i < 9; i++)
    {
        auto c = controls[i];
        if (i == 0)
            c->setBounds (getGridArea (0, 0).removeFromTop (cy / 2).translated (0, 7));
        else if (i == 1)
            c->setBounds (getGridArea (0, 0).removeFromBottom (cy / 2));
        else
            c->setBounds (getGridArea (i - 1, 0));
        
    }
    for (int i = 0; i < 7; i++)
    {
        auto c = controls[i + 9];
        if (i == 0)
            c->setBounds (getGridArea (0, 1).removeFromTop (cy / 2).translated (0, 7));
        else if (i == 1)
            c->setBounds (getGridArea (0, 1).removeFromBottom (cy / 2));
        else
            c->setBounds (getGridArea (i - 1, 1));
    }
    for (int i = 0; i < 7; i++)
    {
        auto c = controls[i + 9 + 7];
        if (i == 0)
            c->setBounds (getGridArea (0, 2).removeFromTop (cy / 2).translated (0, 7));
        else if (i == 1)
            c->setBounds (getGridArea (0, 2).removeFromBottom (cy / 2));
        else
            c->setBounds (getGridArea (i - 1, 2));
    }

    int n = controls.size();

    controls[n - 1]->setBounds (getGridArea (7, 1));
    controls[n - 2]->setBounds (getGridArea (7, 2));
    
    scope.setBounds (getGridArea (8, 0, 5, 3).reduced (5));
}

#endif
