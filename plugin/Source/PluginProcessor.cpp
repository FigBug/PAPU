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
juce::String PAPUAudioProcessor::paramPulse2Duty       = "duty2";
juce::String PAPUAudioProcessor::paramPulse2A          = "A2";
juce::String PAPUAudioProcessor::paramPulse2R          = "R2";
juce::String PAPUAudioProcessor::paramPulse2OL         = "OL2";
juce::String PAPUAudioProcessor::paramPulse2OR         = "OR2";
juce::String PAPUAudioProcessor::paramPulse2Tune       = "tune2";
juce::String PAPUAudioProcessor::paramPulse2Fine       = "fine2";
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
juce::String PAPUAudioProcessor::paramTreble         = "trebeq";
juce::String PAPUAudioProcessor::paramBass         = "bassf";
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
}

int PAPUEngine::parameterIntValue (const juce::String& uid)
{
    return processor.parameterIntValue (uid);
}

void PAPUEngine::runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos)
{
    int todo = juce::jmin (pos, buffer.getNumSamples()) - done;

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

void PAPUEngine::runOscs (int curNote, bool trigger)
{
    if (curNote != -1)
    {
        // Ch 1
        uint8_t sweep = uint8_t (std::abs (parameterIntValue (PAPUAudioProcessor::paramPulse1Sweep)));
        uint8_t neg   = parameterIntValue (PAPUAudioProcessor::paramPulse1Sweep) < 0;
        uint8_t shift = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1Shift));

        writeReg (0xff10, (sweep << 4) | ((neg ? 1 : 0) << 3) | shift, trigger);
        writeReg (0xff11, (parameterIntValue (PAPUAudioProcessor::paramPulse1Duty) << 6), trigger);

        freq1 = float (gin::getMidiNoteInHertz (curNote + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse1Tune) + parameterIntValue (PAPUAudioProcessor::paramPulse1Fine) / 100.0f));
        uint16_t period1 = uint16_t (((4194304 / freq1) - 65536) / -32);
        writeReg (0xff13, period1 & 0xff, trigger);
        uint8_t a1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1A));
        writeReg (0xff12, a1 ? (0x00 | (1 << 3) | a1) : 0xf0, trigger);
        writeReg (0xff14, (trigger ? 0x80 : 0x00) | ((period1 >> 8) & 0x07), trigger);

        // Ch 2
        writeReg (0xff16, (parameterIntValue (PAPUAudioProcessor::paramPulse2Duty) << 6), trigger);

        freq2 = float (gin::getMidiNoteInHertz (curNote + pitchBend + parameterIntValue (PAPUAudioProcessor::paramPulse2Tune) + parameterIntValue (PAPUAudioProcessor::paramPulse2Fine) / 100.0f));
        uint16_t period2 = uint16_t (((4194304 / freq2) - 65536) / -32);
        writeReg (0xff18, period2 & 0xff, trigger);
        uint8_t a2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2A));
        writeReg (0xff17, a2 ? (0x00 | (1 << 3) | a2) : 0xf0, trigger);
        writeReg (0xff19, (trigger ? 0x80 : 0x00) | ((period2 >> 8) & 0x07), trigger);

        // Ch 3
        apu.resetStopWave();
        freq3 = float (gin::getMidiNoteInHertz (curNote + pitchBend + parameterIntValue (PAPUAudioProcessor::paramWaveTune) + parameterIntValue (PAPUAudioProcessor::paramWaveFine) / 100.0f));
        uint16_t period3 = uint16_t (-((65536 - 2048 * freq3)/freq3));
        writeReg ( 0xff1D, period3 & 0xff, trigger); // lower freq bits
        writeReg ( 0xff1C, 0x20, trigger);
        writeReg ( 0xff1E, (trigger ? 0x80 : 0x00) | ((period3 >> 8) & 0x07), trigger); // trigger, high freq bits

        // Noise
        uint8_t aN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseA));
        writeReg (0xff21, aN ? (0x00 | (1 << 3) | aN) : 0xf0, trigger);
        writeReg (0xff22, (parameterIntValue (PAPUAudioProcessor::paramNoiseShift) << 4) |
                  (parameterIntValue (PAPUAudioProcessor::paramNoiseStep)  << 3) |
                  (parameterIntValue (PAPUAudioProcessor::paramNoiseRatio)), trigger);
        writeReg (0xff23, trigger ? 0x80 : 0x00, trigger);
    }
    else if (trigger)
    {
        uint8_t r1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1R));
        uint8_t a1 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse1A));

        if (a1 == 0 && r1 != 0)
        {
            uint16_t period1 = uint16_t (((4194304 / freq1) - 65536) / -32);

            writeReg (0xff13, period1 & 0xff, trigger);
            writeReg (0xff12, r1 ? (0xf0 | (0 << 3) | r1) : 0, trigger);
            writeReg (0xff14, (trigger ? 0x80 : 0x00) | ((period1 >> 8) & 0x07), trigger);
        }
        else
        {
            writeReg (0xff12, r1 ? (0xf0 | (0 << 3) | r1) : 0, trigger);
        }

        uint8_t r2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2R));
        uint8_t a2 = uint8_t (parameterIntValue (PAPUAudioProcessor::paramPulse2A));

        if (a2 == 0 && r2 != 0)
        {
            uint16_t period2 = uint16_t (((4194304 / freq2) - 65536) / -32);

            writeReg (0xff18, period2 & 0xff, trigger);
            writeReg (0xff17, r2 ? (0xf0 | (0 << 3) | r2) : 0, trigger);
            writeReg (0xff19, (trigger ? 0x80 : 0x00) | ((period2 >> 8) & 0x07), trigger);
        }
        else
        {
            writeReg (0xff17, r2 ? (0xf0 | (0 << 3) | r2) : 0, trigger);
        }

        apu.stopWave();

        uint8_t rN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseR));
        uint8_t aN = uint8_t (parameterIntValue (PAPUAudioProcessor::paramNoiseA));

        if (aN == 0 && rN != 0)
        {
            writeReg (0xff21, rN ? (0xf0 | (0 << 3) | rN) : 0, trigger);
            writeReg (0xff23, trigger ? 0x80 : 0x00, trigger);
        }
        else
        {
            writeReg (0xff21, rN ? (0xf0 | (0 << 3) | rN) : 0, trigger);
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

    int done = 0;
    runOscs (lastNote, false);
    runUntil (done, buffer, 0);

    for (auto itr : midi)
    {
        auto msg = itr.getMessage();
        int pos = itr.samplePosition;

        bool updateBend = false;
        runUntil (done, buffer, pos);

        if (msg.isNoteOn())
        {
            noteQueue.add (msg.getNoteNumber());
        }
        else if (msg.isNoteOff())
        {
            noteQueue.removeFirstMatchingValue (msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff())
        {
            noteQueue.clear();
        }
        else if (msg.isPitchWheel())
        {
            updateBend = true;
            pitchBend = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2;
        }
        const int curNote = noteQueue.size() > 0 ? noteQueue.getLast() : -1;

        if (updateBend || lastNote != curNote)
        {
            runOscs (curNote, lastNote != curNote);
            lastNote = curNote;
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
    runOscs (lastNote, false);
    runUntil (done, buffer, 0);

    jassert (done == 0);
}

void PAPUEngine::handleMessage (const juce::MidiMessage& msg)
{
    bool updateBend = false;

    if (msg.isNoteOn())
    {
        noteQueue.add (msg.getNoteNumber());
    }
    else if (msg.isNoteOff())
    {
        noteQueue.removeFirstMatchingValue (msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff())
    {
        noteQueue.clear();
    }
    else if (msg.isPitchWheel())
    {
        updateBend = true;
        pitchBend = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2;
    }
    const int curNote = noteQueue.size() > 0 ? noteQueue.getLast() : -1;

    if (updateBend || lastNote != curNote)
    {
        runOscs (curNote, lastNote != curNote);
        lastNote = curNote;
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
juce::String percentTextFunction (const gin::Parameter& p, float v)
{
    return juce::String::formatted("%.0f%%", v / p.getUserRangeEnd() * 100);
}

juce::String enableTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "On" : "Off";
}

juce::String dutyTextFunction (const gin::Parameter&, float v)
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

juce::String arTextFunction (const gin::Parameter&, float v)
{
    return juce::String::formatted("%.1f s", v * 1.0/64.0 * 16);
}

juce::String stTextFunction (const gin::Parameter&, float v)
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

juce::String stepTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "15" : "7";
}

juce::String intTextFunction (const gin::Parameter&, float v)
{
    return juce::String (int (v));
}

//==============================================================================
PAPUAudioProcessor::PAPUAudioProcessor()
{
    addExtParam (paramPulse1OL,    "Pulse 1 OL",        "Left",    "",  {    0.0f,   1.0f, 1.0f, 1.0f },    1.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse1OR,    "Pulse 1 OR",        "Right",   "",  {    0.0f,   1.0f, 1.0f, 1.0f },    1.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse1Duty,  "Pulse 1 Duty",      "PW",      "",  {    0.0f,   3.0f, 1.0f, 1.0f },    0.0f, 0.0f, dutyTextFunction);
    addExtParam (paramPulse1A,     "Pulse 1 A",         "Attack",  "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse1R,     "Pulse 1 R",         "Release", "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse1Tune,  "Pulse 1 Tune",      "Tune",    "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse1Fine,  "Pulse 1 Tune Fine", "Fine",    "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse1Sweep, "Pulse 1 Sweep",     "Sweep",   "",  {   -7.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, stTextFunction);
    addExtParam (paramPulse1Shift, "Pulse 1 Shift",     "Shift",   "",  {    0.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse2OL,    "Pulse 2 OL",        "Left",    "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse2OR,    "Pulse 2 OR",        "Right",   "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramPulse2Duty,  "Pulse 2 Duty",      "PW",      "",  {    0.0f,   3.0f, 1.0f, 1.0f },    0.0f, 0.0f, dutyTextFunction);
    addExtParam (paramPulse2A,     "Pulse 2 A",         "Attack",  "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse2R,     "Pulse 2 R",         "Release", "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramPulse2Tune,  "Pulse 2 Tune",      "Tune",    "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramPulse2Fine,  "Pulse 2 Tune Fine", "Fine",    "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramNoiseOL,     "Noise OL",          "Left",    "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramNoiseOR,     "Noise OR",          "Right",   "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramNoiseA,      "Noise A",           "Attack",  "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramNoiseR,      "Noise R",           "Release", "",  {    0.0f,   7.0f, 1.0f, 1.0f },    1.0f, 0.0f, arTextFunction);
    addExtParam (paramNoiseShift,  "Noise Shift",       "Shift",   "",  {    0.0f,  13.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramNoiseStep,   "Noise Step",        "Steps",   "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, stepTextFunction);
    addExtParam (paramNoiseRatio,  "Noise Ratio",       "Ratio",   "",  {    0.0f,   7.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveOL,      "Wave OL",           "Left",    "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramWaveOR,      "Wave OR",           "Right",   "",  {    0.0f,   1.0f, 1.0f, 1.0f },    0.0f, 0.0f, enableTextFunction);
    addExtParam (paramWaveWfm,     "Waveform",          "Waveform", "", {    0.0f,  14.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveTune,    "Wave Tune",         "Tune",    "",  {  -48.0f,  48.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramWaveFine,    "Wave Tune Fine",    "Fine",    "",  { -100.0f, 100.0f, 1.0f, 1.0f },    0.0f, 0.0f, intTextFunction);
    addExtParam (paramTreble,      "Treble EQ",         "Treble",  "",  {  -50.0f,  50.0f, 1.0f, 1.0f },  -30.0f, 0.0f, intTextFunction);
    addExtParam (paramBass,        "Bass frequency",    "Bass",    "",  {   15.0f, 600.0f, 1.0f, 1.0f },  461.0f, 0.0f, intTextFunction);
    addExtParam (paramOutput,      "Output",            "Output",  "",  {    0.0f,   7.0f, 1.0f, 1.0f },    7.0f, 0.0f, percentTextFunction);
    addExtParam (paramVoices,      "Voices",            "Voices",  "",  {    1.0f,   8.0f, 1.0f, 1.0f },    1.0f, 0.0f, intTextFunction);

    for (int i = 0; i < 16; i++)
        papus.add (new PAPUEngine (*this));
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

   #if JUCE_IOS
    state.processNextMidiBuffer (midi, 0, numSamples, true);
   #endif
   
    uint8_t new_waveIndex = parameterIntValue (PAPUAudioProcessor::paramWaveWfm);
    if (new_waveIndex != papus[0]->waveIndex) {
        for (int i = 0; i < 16; i++)
        {
            papus[i]->setWave(new_waveIndex);
            
        }
    }
    
    float new_treble = parameterValue (PAPUAudioProcessor::paramTreble);
    if (new_treble != papus[0]->treble) {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->treble = new_treble;
            papus[i]->getApu()->treble_eq(new_treble);
        }
    }
    
    float new_bass = parameterIntValue (PAPUAudioProcessor::paramBass);
    if (new_bass != papus[0]->bass) {
        for (int i = 0; i < 16; i++)
        {   
            papus[i]->bass = new_bass;
            papus[i]->getBuffer()->bass_freq(new_bass);
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

        for (auto itr : midi)
        {
            auto msg = itr.getMessage();
            int pos = itr.samplePosition;

            runUntil (done, buffer, pos);

            if (msg.isNoteOn())
            {
                if (auto voice = findFreeVoice())
                    voice->handleMessage (msg);
            }
            else if (msg.isNoteOff())
            {
                if (auto voice = findVoiceForNote (msg.getNoteNumber()))
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

PAPUEngine* PAPUAudioProcessor::findFreeVoice()
{
    int voices = parameterIntValue (paramVoices);
    for (int i = 0; i < voices; i++)
    {
        int vidx = (nextVoice + i) % voices;
        if (papus[vidx]->getNote() == -1)
        {
            nextVoice = (nextVoice + 1) % voices;
            return papus[vidx];
        }
    }
    return nullptr;
}

PAPUEngine* PAPUAudioProcessor::findVoiceForNote (int note)
{
    int voices = parameterIntValue (paramVoices);
    for (int i = 0; i < voices; i++)
        if (papus[i]->getNote() == note)
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
