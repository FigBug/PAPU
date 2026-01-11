#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PadPluginEditor.h"

juce::String PAPUAudioProcessor::paramPulse1Sweep      = "sweep1";
juce::String PAPUAudioProcessor::paramPulse1Shift      = "shift1";
juce::String PAPUAudioProcessor::paramPulse1Duty       = "duty1";
juce::String PAPUAudioProcessor::paramPulse1A          = "A1";
juce::String PAPUAudioProcessor::paramPulse1R          = "R1";
juce::String PAPUAudioProcessor::paramPulse1OL         = "OL1";
juce::String PAPUAudioProcessor::paramPulse1OR         = "OR1";
juce::String PAPUAudioProcessor::paramPulse1Tune       = "tune1";
juce::String PAPUAudioProcessor::paramPulse1Fine       = "fine1";
juce::String PAPUAudioProcessor::paramPulse1VibRate    = "rate1";
juce::String PAPUAudioProcessor::paramPulse1VibAmt     = "amt1";
juce::String PAPUAudioProcessor::paramPulse2Duty       = "duty2";
juce::String PAPUAudioProcessor::paramPulse2A          = "A2";
juce::String PAPUAudioProcessor::paramPulse2R          = "R2";
juce::String PAPUAudioProcessor::paramPulse2OL         = "OL2";
juce::String PAPUAudioProcessor::paramPulse2OR         = "OR2";
juce::String PAPUAudioProcessor::paramPulse2Tune       = "tune2";
juce::String PAPUAudioProcessor::paramPulse2Fine       = "fine2";
juce::String PAPUAudioProcessor::paramPulse2VibRate    = "rate2";
juce::String PAPUAudioProcessor::paramPulse2VibAmt     = "amt2";
juce::String PAPUAudioProcessor::paramNoiseOL          = "OLN";
juce::String PAPUAudioProcessor::paramNoiseOR          = "ORL";
juce::String PAPUAudioProcessor::paramNoiseShift       = "shiftN";
juce::String PAPUAudioProcessor::paramNoiseStep        = "stepN";
juce::String PAPUAudioProcessor::paramNoiseRatio       = "ratioN";
juce::String PAPUAudioProcessor::paramNoiseA           = "AN";
juce::String PAPUAudioProcessor::paramNoiseR           = "AR";
juce::String PAPUAudioProcessor::paramWaveOL           = "OLW";
juce::String PAPUAudioProcessor::paramWaveOR           = "ORW";
juce::String PAPUAudioProcessor::paramWaveWfm          = "waveform";
juce::String PAPUAudioProcessor::paramWaveTune         = "tunewave";
juce::String PAPUAudioProcessor::paramWaveFine         = "finewave";
juce::String PAPUAudioProcessor::paramWaveVibRate      = "ratewave";
juce::String PAPUAudioProcessor::paramWaveVibAmt       = "amtwave";
juce::String PAPUAudioProcessor::paramChannelSplit     = "channelsplit";
juce::String PAPUAudioProcessor::paramTreble           = "trebeq";
juce::String PAPUAudioProcessor::paramBass             = "bassf";
juce::String PAPUAudioProcessor::paramOutput           = "output";
juce::String PAPUAudioProcessor::paramVoices           = "param";

//==============================================================================
PAPUEngine::PAPUEngine (PAPUAudioProcessor& p)
    : processor (p)
{
}

void PAPUEngine::prepareToPlay (double sampleRate)
{
    apu.treble_eq( -20.0 ); // lower values muffle it more
    buf.bass_freq( 461 ); // higher values simulate smaller speaker

    buf.clock_rate (4194304);
    buf.set_sample_rate (long (sampleRate));

    apu.output (buf.center(), buf.left(), buf.right());

    writeReg (0xff1A, 0x00, true); // reset
    // set pattern
    for (uint8_t s = 0; s < 16; s++) {
        uint8_t high = (wave_samples[waveIndex][s * 2]) & 0xff;
        uint8_t low = (wave_samples[waveIndex][(s * 2) + 1]) & 0xff;
    	writeReg (0xff30 + s, (low | (high << 4)), true);
    }
    writeReg (0xff1A, 0x80, true); // enable

    writeReg (0xff26, 0x8f, true);
    
    vib1.setSampleRate(sampleRate);
    vib1Parameters.waveShape = gin::LFO::WaveShape::sine;
    vib1Parameters.frequency = 5.0;
    vib1Parameters.phase     = 0.0;
    vib1Parameters.offset    = 0.0;
    vib1Parameters.depth     = 0.0;
    vib1Parameters.fade     = 0.0;
    vib1Parameters.delay     = 0.0;

    vib2.setParameters (vib2Parameters);
    vib2.reset();
    vib2.setSampleRate(sampleRate);
    vib2Parameters.waveShape = gin::LFO::WaveShape::sine;
    vib2Parameters.frequency = 5.0;
    vib2Parameters.phase     = 0.0;
    vib2Parameters.offset    = 0.0;
    vib2Parameters.depth     = 0.0;
    vib2Parameters.fade     = 0.0;
    vib2Parameters.delay     = 0.0;

    vib2.setParameters (vib2Parameters);
    vib2.reset();
    
    vib3.setSampleRate(sampleRate);
    vib3Parameters.waveShape = gin::LFO::WaveShape::sine;
    vib3Parameters.frequency = 5.0;
    vib3Parameters.phase     = 0.0;
    vib3Parameters.offset    = 0.0;
    vib3Parameters.depth     = 0.0;
    vib3Parameters.fade     = 0.0;
    vib3Parameters.delay     = 0.0;

    vib3.setParameters (vib3Parameters);
    vib3.reset();
}

int PAPUEngine::parameterIntValue (const juce::String& uid)
{
    return processor.parameterIntValue (uid);
}

void PAPUEngine::runVibrato(int todo)
{
    vib1.process(todo);
    vib2.process(todo);
    vib3.process(todo);
    
    bool trigger1 = regCache[0xff14] & 0x80;
    freq1 = float (gin::getMidiNoteInHertz (vibNote1 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse1Tune) + (parameterIntValue (PAPUAudioProcessor::paramPulse1Fine) / 100.0f) + (vib1.getOutput() * 12.0)));
    uint16_t period1 = uint16_t (((4194304 / freq1) - 65536) / -32);
    writeReg (0xff13, period1 & 0xff, false);
    writeReg (0xff14, (trigger1 ? 0x80 : 0x00) | ((period1 >> 8) & 0x07), false);
    
    bool trigger2 = regCache[0xff19] & 0x80;
    freq2 = float (gin::getMidiNoteInHertz (vibNote2 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse2Tune) + (parameterIntValue (PAPUAudioProcessor::paramPulse2Fine) / 100.0f) + (vib2.getOutput() * 12.0)));
    uint16_t period2 = uint16_t (((4194304 / freq2) - 65536) / -32);
    writeReg (0xff18, period2 & 0xff, false);
    writeReg (0xff19, (trigger2 ? 0x80 : 0x00) | ((period2 >> 8) & 0x07), false);
    
    bool trigger3 = regCache[0xff1E] & 0x80;
    freq3 = float (gin::getMidiNoteInHertz (vibNote3 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramWaveTune) + (parameterIntValue (PAPUAudioProcessor::paramWaveFine) / 100.0f) + (vib3.getOutput() * 12.0)));
    uint16_t period3 = uint16_t (((4194304 / freq3) - 65536) / -32);
    writeReg (0xff1D, period3 & 0xff, false);
    writeReg (0xff1E, (trigger3 ? 0x80 : 0x00) | ((period3 >> 8) & 0x07), false);
}

void PAPUEngine::runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos)
{
    int todo = juce::jmin (pos, buffer.getNumSamples()) - done;
    
    runVibrato(todo);

    while (todo > 0)
    {
        if (buf.samples_avail() > 0)
        {
            blip_sample_t out[1024];

            int count = int (buf.read_samples (out, juce::jmin (todo, 1024 / 2, (int) buf.samples_avail())));

            auto data0 = buffer.getWritePointer (0, done);
            auto data1 = buffer.getWritePointer (1, done);

            for (int i = 0; i < count; i++)
            {
                data0[i] += out[i * 2 + 0] / 32768.0f;
                data1[i] += out[i * 2 + 1] / 32768.0f;
            }

            done += count;
            todo -= count;
        }
        else
        {
            time = 0;
            

            bool stereo = apu.end_frame (1024);
            buf.end_frame (1024, stereo);
        }
    }
}

void PAPUEngine::runOscs (int curNote1, int curNote2, int curNote3, int curNote4, bool trigger1, bool trigger2, bool trigger3, bool trigger4)
{
    // Ch 1
    if (curNote1 != -1)
    {   
        vibNote1 = curNote1;
        
        uint8_t sweep = uint8_t (std::abs (parameterIntValue (PAPUAudioProcessor::paramPulse1Sweep)));
        uint8_t neg   = parameterIntValue (PAPUAudioProcessor::paramPulse1Sweep) < 0;
        uint8_t shift = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1Shift));

        writeReg (0xff10, (sweep << 4) | ((neg ? 1 : 0) << 3) | shift, trigger1);
        writeReg (0xff11, (parameterIntValue (PAPUAudioProcessor::paramPulse1Duty) << 6), trigger1);

        freq1 = float (gin::getMidiNoteInHertz (curNote1 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse1Tune) + parameterIntValue (PAPUAudioProcessor::paramPulse1Fine) / 100.0f));
        uint16_t period1 = uint16_t (((4194304 / freq1) - 65536) / -32);
        writeReg (0xff13, period1 & 0xff, trigger1);
        uint8_t a1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1A));
        writeReg (0xff12, a1 ? (0x00 | (1 << 3) | a1) : 0xf0, trigger1);
        writeReg (0xff14, (trigger1 ? 0x80 : 0x00) | ((period1 >> 8) & 0x07), trigger1);
    }
    else if (trigger1)
    {
        uint8_t r1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1R));
        uint8_t a1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1A));
        
        if (a1 == 0 && r1 != 0)
        {
            uint16_t period1 = uint16_t (((4194304 / freq1) - 65536) / -32);
            
            writeReg (0xff13, period1 & 0xff, trigger1);
            writeReg (0xff12, r1 ? (0xf0 | (0 << 3) | r1) : 0, trigger1);
            writeReg (0xff14, (trigger1 ? 0x80 : 0x00) | ((period1 >> 8) & 0x07), trigger1);
        }
        else
        {
            writeReg (0xff12, r1 ? (0xf0 | (0 << 3) | r1) : 0, trigger1);
        }
    }

    // Ch 2
    if (curNote2 != -1)
    {
        vibNote2 = curNote2;
        
        writeReg (0xff16, (parameterIntValue (PAPUAudioProcessor::paramPulse2Duty) << 6), trigger2);

        freq2 = float (gin::getMidiNoteInHertz (curNote2 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse2Tune) + parameterIntValue (PAPUAudioProcessor::paramPulse2Fine) / 100.0f));
        uint16_t period2 = uint16_t (((4194304 / freq2) - 65536) / -32);
        writeReg (0xff18, period2 & 0xff, trigger2);
        uint8_t a2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2A));
        writeReg (0xff17, a2 ? (0x00 | (1 << 3) | a2) : 0xf0, trigger2);
        writeReg (0xff19, (trigger2 ? 0x80 : 0x00) | ((period2 >> 8) & 0x07), trigger2);
    }
    else if (trigger2)
    {
        uint8_t r2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2R));
        uint8_t a2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2A));
        
        if (a2 == 0 && r2 != 0)
        {
            uint16_t period2 = uint16_t (((4194304 / freq2) - 65536) / -32);
            
            writeReg (0xff18, period2 & 0xff, trigger2);
            writeReg (0xff17, r2 ? (0xf0 | (0 << 3) | r2) : 0, trigger2);
            writeReg (0xff19, (trigger2 ? 0x80 : 0x00) | ((period2 >> 8) & 0x07), trigger2);
        }
        else
        {
            writeReg (0xff17, r2 ? (0xf0 | (0 << 3) | r2) : 0, trigger2);
        }
    }

    // Ch 3
    if (curNote3 != -1)
    {
        // printf("Trigger! chan2\n");
        vibNote3 = curNote3;
        
        apu.resetStopWave();
        freq3 = float (gin::getMidiNoteInHertz (curNote3 + pitchBend + parameterIntValue (PAPUAudioProcessor::paramWaveTune) + parameterIntValue (PAPUAudioProcessor::paramWaveFine) / 100.0f));
        uint16_t period3 = uint16_t (-((65536 - 2048 * freq3)/freq3));
        writeReg ( 0xff1D, period3 & 0xff, trigger3); // lower freq bits
        writeReg ( 0xff1C, 0x20, trigger3);
        writeReg ( 0xff1E, (trigger3 ? 0x80 : 0x00) | ((period3 >> 8) & 0x07), trigger3); // trigger, high freq bits
    }
    else if (trigger3)
    {
        apu.stopWave();
    }
        
    // Noise
    if (curNote4 != -1)
    {
        uint8_t aN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseA));
        writeReg (0xff21, aN ? (0x00 | (1 << 3) | aN) : 0xf0, trigger4);
        writeReg (0xff22, (parameterIntValue (PAPUAudioProcessor::paramNoiseShift) << 4) |
                  (parameterIntValue (PAPUAudioProcessor::paramNoiseStep)  << 3) |
                  (parameterIntValue (PAPUAudioProcessor::paramNoiseRatio)), trigger4);
        writeReg (0xff23, trigger4 ? 0x80 : 0x00, trigger4);
    }
    else if (trigger4)
    {
        uint8_t rN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseR));
        uint8_t aN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseA));
        
        if (aN == 0 && rN != 0)
        {
            writeReg (0xff21, rN ? (0xf0 | (0 << 3) | rN) : 0, trigger4);
            writeReg (0xff23, trigger4 ? 0x80 : 0x00, trigger4);
        }
        else
        {
            writeReg (0xff21, rN ? (0xf0 | (0 << 3) | rN) : 0, trigger4);
        }
    }
}

void PAPUEngine::writeReg (int reg, int value, bool force)
{
    auto itr = regCache.find (reg);
    if (force || itr == regCache.end() || itr->second != value)
    {
        regCache[reg] = value;
        apu.write_register (clock(), gb_addr_t (reg), value);
    }
}

void PAPUEngine::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi)
{
    uint16_t reg;

    reg = uint16_t (0x08 | parameterIntValue (PAPUAudioProcessor::paramOutput));
    writeReg (0xff24, reg, false);

    reg = (parameterIntValue (PAPUAudioProcessor::paramPulse1OL) ? 0x10 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse1OR) ? 0x01 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse2OL) ? 0x20 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse2OR) ? 0x02 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramWaveOL)   ? 0x40 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramWaveOR)   ? 0x04 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramNoiseOL)  ? 0x80 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramNoiseOR)  ? 0x08 : 0x00);

    writeReg (0xff25, reg, false);
    
    bool new_channelsplit = parameterIntValue (PAPUAudioProcessor::paramChannelSplit) ? true : false;
    if (new_channelsplit != channelsplit)
    {
        channelsplit = new_channelsplit;
        noteQueue1.clear();
        noteQueue2.clear();
        noteQueue3.clear();
        noteQueue4.clear();
    }
    
    int done = 0;
    runOscs (lastNote1, lastNote2, lastNote3, lastNote4, false, false, false, false);
    runUntil (done, buffer, 0);

    for (auto itr : midi)
    {
        auto msg = itr.getMessage();
        int pos = itr.samplePosition;

        bool updateBend = false;
        runUntil (done, buffer, pos);
        
        if (msg.isNoteOn())
        {   
            if (msg.getChannel() == 1 || !channelsplit) noteQueue1.add (msg.getNoteNumber());
            else if (msg.getChannel() == 2) noteQueue2.add (msg.getNoteNumber());
            else if (msg.getChannel() == 3) noteQueue3.add (msg.getNoteNumber());
            else if (msg.getChannel() == 4) noteQueue4.add (msg.getNoteNumber());
        }
        else if (msg.isNoteOff())
        {
            if (msg.getChannel() == 1 || !channelsplit) noteQueue1.removeFirstMatchingValue (msg.getNoteNumber());
            else if (msg.getChannel() == 2) noteQueue2.removeFirstMatchingValue (msg.getNoteNumber());
            else if (msg.getChannel() == 3) noteQueue3.removeFirstMatchingValue (msg.getNoteNumber());
            else if (msg.getChannel() == 4) noteQueue4.removeFirstMatchingValue (msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff())
        {
            noteQueue1.clear();
            noteQueue2.clear();
            noteQueue3.clear();
            noteQueue4.clear();
        }
        else if (msg.isPitchWheel())
        {
            updateBend = true;
            pitchBend = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2;
        }
        const int curNote1 = noteQueue1.size() > 0 ? noteQueue1.getLast() : -1;
        const int curNote2 = !channelsplit ? curNote1 : noteQueue2.size() > 0 ? noteQueue2.getLast() : -1;
        const int curNote3 = !channelsplit ? curNote1 : noteQueue3.size() > 0 ? noteQueue3.getLast() : -1;
        const int curNote4 = !channelsplit ? curNote1 : noteQueue4.size() > 0 ? noteQueue4.getLast() : -1;
        
        if (updateBend ||
            lastNote1 != curNote1 ||
            lastNote2 != curNote2 ||
            lastNote3 != curNote3 ||
            lastNote4 != curNote4)
        {
            if (!updateBend && (curNote1 != -1)) vib1.reset();
            if (!updateBend && (curNote2 != -1)) vib2.reset();
            if (!updateBend && (curNote3 != -1)) vib3.reset();
            
            runOscs (curNote1, curNote2, curNote3, curNote4,
                     lastNote1 != curNote1,
                     lastNote2 != curNote2,
                     lastNote3 != curNote3,
                     lastNote4 != curNote4);
            lastNote1 = curNote1;
            lastNote2 = curNote2;
            lastNote3 = curNote3;
            lastNote4 = curNote4;
        }
    }

    int numSamples = buffer.getNumSamples();
    runUntil (done, buffer, numSamples);
}

void PAPUEngine::prepareBlock (juce::AudioSampleBuffer& buffer)
{
    uint16_t reg;

    reg = uint16_t (0x08 | parameterIntValue (PAPUAudioProcessor::paramOutput));
    writeReg (0xff24, reg, false);

    reg = (parameterIntValue (PAPUAudioProcessor::paramPulse1OL) ? 0x10 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse1OR) ? 0x01 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse2OL) ? 0x20 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramPulse2OR) ? 0x02 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramWaveOL)   ? 0x40 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramWaveOR)   ? 0x04 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramNoiseOL)  ? 0x80 : 0x00) |
          (parameterIntValue (PAPUAudioProcessor::paramNoiseOR)  ? 0x08 : 0x00);

    writeReg (0xff25, reg, false);

    int done = 0;
    runOscs (lastNote1, lastNote2, lastNote3, lastNote4, false, false, false, false);
    runUntil (done, buffer, 0);

    jassert (done == 0);
}

void PAPUEngine::handleMessage (const juce::MidiMessage& msg)
{
    bool updateBend = false;
    
    bool new_channelsplit = parameterIntValue (PAPUAudioProcessor::paramChannelSplit) ? true : false;
    if (new_channelsplit != channelsplit)
    {
        channelsplit = new_channelsplit;
        noteQueue1.clear();
        noteQueue2.clear();
        noteQueue3.clear();
        noteQueue4.clear();
    }    
    
    if (msg.isNoteOn())
    {
        if (msg.getChannel() == 1 || !channelsplit) noteQueue1.add (msg.getNoteNumber());
        else if (msg.getChannel() == 2) noteQueue2.add (msg.getNoteNumber());
        else if (msg.getChannel() == 3) noteQueue3.add (msg.getNoteNumber());
        else if (msg.getChannel() == 4) noteQueue4.add (msg.getNoteNumber());
    }
    else if (msg.isNoteOff())
    {
        if (msg.getChannel() == 1 || !channelsplit) noteQueue1.removeFirstMatchingValue (msg.getNoteNumber());
        else if (msg.getChannel() == 2) noteQueue2.removeFirstMatchingValue (msg.getNoteNumber());
        else if (msg.getChannel() == 3) noteQueue3.removeFirstMatchingValue (msg.getNoteNumber());
        else if (msg.getChannel() == 4) noteQueue4.removeFirstMatchingValue (msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff())
    {
        noteQueue1.clear();
        noteQueue2.clear();
        noteQueue3.clear();
        noteQueue4.clear();
    }
    else if (msg.isPitchWheel())
    {
        updateBend = true;
        pitchBend = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2;
    }
    const int curNote1 = noteQueue1.size() > 0 ? noteQueue1.getLast() : -1;
    const int curNote2 = !channelsplit ? curNote1 : noteQueue2.size() > 0 ? noteQueue2.getLast() : -1;
    const int curNote3 = !channelsplit ? curNote1 : noteQueue3.size() > 0 ? noteQueue3.getLast() : -1;
    const int curNote4 = !channelsplit ? curNote1 : noteQueue4.size() > 0 ? noteQueue4.getLast() : -1;

    if (updateBend ||
        lastNote1 != curNote1 ||
        lastNote2 != curNote2 ||
        lastNote3 != curNote3 ||
        lastNote4 != curNote4)
    {
        if (!updateBend && (curNote1 != -1)) vib1.reset();
        if (!updateBend && (curNote2 != -1)) vib2.reset();
        if (!updateBend && (curNote3 != -1)) vib3.reset();
        
        runOscs (curNote1, curNote2, curNote3, curNote4,
                 lastNote1 != curNote1,
                 lastNote2 != curNote2,
                 lastNote3 != curNote3,
                 lastNote4 != curNote4);
        lastNote1 = curNote1;
        lastNote2 = curNote2;
        lastNote3 = curNote3;
        lastNote4 = curNote4;
    }
}

void PAPUEngine::setWave(uint8_t index)
{
    if (index == waveIndex)
        return;
    
    waveIndex = index;
    writeReg (0xff1A, 0x00, true); // reset
    // set pattern
    for (uint8_t s = 0; s < 16; s++) {
        uint8_t high = (wave_samples[waveIndex][s * 2]) & 0xff;
        uint8_t low = (wave_samples[waveIndex][(s * 2) + 1]) & 0xff;
        writeReg (0xff30 + s, (low | (high << 4)), true);
    }
    writeReg (0xff1A, 0x80, true); // enable
}

//==============================================================================
static juce::String percentTextFunction (const gin::Parameter& p, float v)
{
    return juce::String::formatted("%.0f%%", v / p.getUserRangeEnd() * 100);
}

static juce::String enableTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "On" : "Off";
}

static juce::String dutyTextFunction (const gin::Parameter&, float v)
{
    const int duty = int (v);
    switch (duty)
    {
        case 0: return "12.5%";
        case 1: return "25%";
        case 2: return "50%";
        case 3: return "75%";
    }
    return "";
}

static juce::String arTextFunction (const gin::Parameter&, float v)
{
    return juce::String::formatted("%.1f s", v * 1.0/64.0 * 16);
}

static juce::String hzTextFunction (const gin::Parameter&, float v)
{
    return juce::String::formatted("%.1f Hz", v);
}

static juce::String stTextFunction (const gin::Parameter&, float v)
{
    juce::String str;
    switch (abs (int (v)))
    {
        case 0: str = "Off"; break;
        case 1: str = "7.8 ms"; break;
        case 2: str = "15.6 ms"; break;
        case 3: str = "23.4 ms"; break;
        case 4: str = "31.3 ms"; break;
        case 5: str = "39.1 ms"; break;
        case 6: str = "46.9 ms"; break;
        case 7: str = "54.7 ms"; break;
    }
    
    if (v < 0)
        str = "-" + str;
    
    return str;
}

static juce::String stepTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "15" : "7";
}

static juce::String intTextFunction (const gin::Parameter&, float v)
{
    return juce::String (int (v));
}

//==============================================================================
static gin::ProcessorOptions createProcessorOptions()
{
    return gin::ProcessorOptions()
        .withAdditionalCredits ({"Shay Green"})
        .withMidiLearn();
}

PAPUAudioProcessor::PAPUAudioProcessor()
    : gin::Processor (false, createProcessorOptions())
{
    addExtParam (paramPulse1OL,      "Pulse 1 OL",        "Left",          "",  {    0.0f,   1.0f, 1.0f, 1.0f },    1.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse1OR,      "Pulse 1 OR",        "Right",         "",  {    0.0f,   1.0f, 1.0f, 1.0f },    1.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse1Duty,    "Pulse 1 Duty",      "PW",            "",  {    0.0f,   3.0f, 1.0f, 1.0f },    0.0f, 0.0f, dutyTextFunction);
    addExtParam (paramPulse1A,       "Pulse 1 A",         "Attack",        "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse1R,       "Pulse 1 R",         "Release",       "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse1Tune,    "Pulse 1 Tune",      "Tune",          "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse1Fine,    "Pulse 1 Tune Fine", "Fine",          "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse1Sweep,   "Pulse 1 Sweep",     "Sweep",         "",  {   -7.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, stTextFunction);
    addExtParam (paramPulse1Shift,   "Pulse 1 Shift",     "Shift",         "",  {    0.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse1VibRate, "Pulse 1 Vib Rate",  "Rate",          "",  {    0.0f,  15.0f, 0.1f, 1.0f },    5.0f, 0.0f, hzTextFunction);
    addExtParam (paramPulse1VibAmt,  "Pulse 1 Vib Amt",   "Amount",        "",  {    0.0f, 100.0f, 0.5f, 1.0f },    0.0f, 0.0f, percentTextFunction);
    addExtParam (paramPulse2OL,      "Pulse 2 OL",        "Left",          "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse2OR,      "Pulse 2 OR",        "Right",         "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse2Duty,    "Pulse 2 Duty",      "PW",            "",  {    0.0f,   3.0f, 1.0f, 1.0f },    0.0f, 0.0f, dutyTextFunction);
    addExtParam (paramPulse2A,       "Pulse 2 A",         "Attack",        "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse2R,       "Pulse 2 R",         "Release",       "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse2Tune,    "Pulse 2 Tune",      "Tune",          "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse2Fine,    "Pulse 2 Tune Fine", "Fine",          "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse2VibRate, "Pulse 2 Vib Rate",  "Rate",          "",  {    0.0f,  15.0f, 0.1f, 1.0f },    5.0f, 0.0f, hzTextFunction);
    addExtParam (paramPulse2VibAmt,  "Pulse 2 Vib Amt",   "Amount",        "",  {    0.0f, 100.0f, 0.5f, 1.0f },    0.0f, 0.0f, percentTextFunction);
    addExtParam (paramNoiseOL,       "Noise OL",          "Left",          "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramNoiseOR,       "Noise OR",          "Right",         "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramNoiseA,        "Noise A",           "Attack",        "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramNoiseR,        "Noise R",           "Release",       "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramNoiseShift,    "Noise Shift",       "Shift",         "",  {    0.0f,  13.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramNoiseStep,     "Noise Step",        "Steps",         "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, stepTextFunction);
    addExtParam (paramNoiseRatio,    "Noise Ratio",       "Ratio",         "",  {    0.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveOL,        "Wave OL",           "Left",          "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramWaveOR,        "Wave OR",           "Right",         "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramWaveWfm,       "Waveform",          "Waveform",      "",  {    0.0f,  14.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveTune,      "Wave Tune",         "Tune",          "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveFine,      "Wave Tune Fine",    "Fine",          "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveVibRate,   "Wave Vib Rate",     "Rate",          "",  {    0.0f,  15.0f, 0.1f, 1.0f },    5.0f, 0.0f, hzTextFunction);
    addExtParam (paramWaveVibAmt,    "Wave Vib Amt",      "Amount",        "",  {    0.0f, 100.0f, 0.5f, 1.0f },    0.0f, 0.0f, percentTextFunction);
    addExtParam (paramChannelSplit,  "Channel split",     "Channel split", "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramTreble,        "Treble EQ",         "Treble",        "",  {  -50.0f,  50.0f, 1.0f, 1.0f },  -30.0f, 0.0f, intTextFunction);
    addExtParam (paramBass,          "Bass frequency",    "Bass",          "",  {   15.0f, 600.0f, 1.0f, 1.0f },  461.0f, 0.0f, intTextFunction);
    addExtParam (paramOutput,        "Output",            "Output",        "",  {    0.0f,   7.0f, 1.0f, 1.0f },    7.0f, 0.0f, percentTextFunction);
    addExtParam (paramVoices,        "Voices",            "Voices",        "",  {    1.0f,   8.0f, 1.0f, 1.0f },    1.0f, 0.0f, intTextFunction);

    for (int i = 0; i < 16; i++)
        papus.add (new PAPUEngine (*this));
    
    init();
}

PAPUAudioProcessor::~PAPUAudioProcessor()
{
}

//==============================================================================
void PAPUAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    for (auto p : papus)
        p->prepareToPlay (sampleRate);
}

void PAPUAudioProcessor::releaseResources()
{
}

void PAPUAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi)
{
    int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (midiLearn)
        midiLearn->processBlock (midi, numSamples);

   #if JUCE_IOS
    keyState.processNextMidiBuffer (midi, 0, numSamples, true);
   #endif
   
    auto new_waveIndex = uint8_t (parameterIntValue (PAPUAudioProcessor::paramWaveWfm));
    if (new_waveIndex != papus[0]->waveIndex)
        for (int i = 0; i < 16; i++)
            papus[i]->setWave(new_waveIndex);

    float new_treble = parameterValue (PAPUAudioProcessor::paramTreble);
    if (new_treble != papus[0]->treble)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->treble = new_treble;
            papus[i]->getApu()->treble_eq (new_treble);
        }
    }
    
    int new_bass = parameterIntValue (PAPUAudioProcessor::paramBass);
    if (new_bass != papus[0]->bass)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->bass = new_bass;
            papus[i]->getBuffer()->bass_freq (new_bass);
        }
    }
    
    float new_vib1 = 0.25f * parameterValue (PAPUAudioProcessor::paramPulse1VibAmt) / 100.0f;
    if (new_vib1 != papus[0]->vib1Parameters.depth)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib1Parameters.depth = new_vib1;
            papus[i]->vib1.setParameters (papus[i]->vib1Parameters);
        }
    }
    
    float new_rate1 = parameterValue (PAPUAudioProcessor::paramPulse1VibRate);
    if (new_rate1 != papus[0]->vib1Parameters.frequency)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib1Parameters.frequency = new_rate1;
            papus[i]->vib1.setParameters (papus[i]->vib1Parameters);
        }
    }
    
    float new_vib2 = 0.25f * parameterValue (PAPUAudioProcessor::paramPulse2VibAmt) / 100.0f;
    if (new_vib2 != papus[0]->vib2Parameters.depth)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib2Parameters.depth = new_vib2;
            papus[i]->vib2.setParameters (papus[i]->vib2Parameters);
        }
    }
    
    float new_rate2 = parameterValue (PAPUAudioProcessor::paramPulse2VibRate);
    if (new_rate2 != papus[0]->vib2Parameters.frequency)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib2Parameters.frequency = new_rate2;
            papus[i]->vib2.setParameters (papus[i]->vib2Parameters);
        }
    }
    
    float new_vib3 = 0.25f * parameterValue (PAPUAudioProcessor::paramWaveVibAmt) / 100.0f;
    if (new_vib3 != papus[0]->vib3Parameters.depth)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib3Parameters.depth = new_vib3;
            papus[i]->vib3.setParameters (papus[i]->vib3Parameters);
        }
    }
    
    float new_rate3 = parameterValue (PAPUAudioProcessor::paramWaveVibRate);
    if (new_rate3 != papus[0]->vib3Parameters.frequency)
    {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->vib3Parameters.frequency = new_rate3;
            papus[i]->vib3.setParameters (papus[i]->vib3Parameters);
        }
    }

    int voices = parameterIntValue (paramVoices);

    if (voices == 1)
    {
        papus[0]->processBlock (buffer, midi);
    }
    else
    {
        int done = 0;
    
        for (int i = 0; i < voices; i++)
            papus[i]->prepareBlock (buffer);
            
        bool channelsplit = parameterIntValue (PAPUAudioProcessor::paramChannelSplit) ? true : false;
    
        for (auto itr : midi)
        {
            auto msg = itr.getMessage();
            int pos = itr.samplePosition;
    
            runUntil (done, buffer, pos);
    
            if (msg.isNoteOn())
            {
                if (auto voice = findFreeVoice(!channelsplit ? 1 : msg.getChannel()))
                    voice->handleMessage (msg);
            }
            else if (msg.isNoteOff())
            {
                if (auto voice = findVoiceForNote (msg.getNoteNumber(), !channelsplit ? 1 : msg.getChannel()))
                    voice->handleMessage (msg);
            }
            else if (msg.isAllNotesOff())
            {
                for (int i = 0; i < voices; i++)
                    papus[i]->handleMessage (msg);
            }
            else if (msg.isPitchWheel())
            {
                for (int i = 0; i < voices; i++)
                    papus[i]->handleMessage (msg);
            }
        }
    
        runUntil (done, buffer, numSamples);
    }

    if (fifo.getFreeSpace() >= numSamples)
    {
        auto dataL = buffer.getReadPointer (0);
        auto dataR = buffer.getReadPointer (1);

        auto mono = (float*) alloca (size_t (numSamples) * sizeof (float));
        
        for (int i = 0; i < numSamples; i++)
            mono[i] = (dataL[i] + dataR[i]) / 2.0f;
        
        fifo.writeMono (mono, numSamples);
    }
}

void PAPUAudioProcessor::runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos)
{
    int todo = juce::jmin (pos, buffer.getNumSamples()) - done;

    int voices = parameterIntValue (paramVoices);
    for (int i = 0; i < voices; i++)
    {
        int doneCopy = done;
        papus[i]->runUntil (doneCopy, buffer, pos);
    }

    done += todo;
}

PAPUEngine* PAPUAudioProcessor::findFreeVoice(int channel)
{
    int voices = parameterIntValue (paramVoices);
    for (int i = 0; i < voices; i++)
    {
        int vidx = (nextVoice + i) % voices;
        if (papus[vidx]->getNote(channel) == -1)
        {
            nextVoice = (nextVoice + 1) % voices;
            return papus[vidx];
        }
    }
    return nullptr;
}

PAPUEngine* PAPUAudioProcessor::findVoiceForNote (int note, int channel)
{
    int voices = parameterIntValue (paramVoices);
    for (int i = 0; i < voices; i++)
        if (papus[i]->getNote(channel) == note)
            return papus[i];

    return nullptr;
}

//==============================================================================
bool PAPUAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PAPUAudioProcessor::createEditor()
{
    return new PAPUAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PAPUAudioProcessor();
}
