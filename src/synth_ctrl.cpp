#include "synth_ctrl.h"

SynthCtrl::SynthCtrl()
{
    initTables();

    currentF0 = FP_TYPE(0.0);
    currentVol = FP_TYPE(0.0);

    startPitch = -FP_TYPE(1.0);
    currentPitch = -FP_TYPE(1.0);
    targetPitch = -FP_TYPE(1.0);
    glideTicks = 1;
    glideProgress = 0;
    glideInvTicks = FP_TYPE(1.0);

    currentBendIndex = 0;
    fmDepth = FP_TYPE(0.0);
    amDepth = FP_TYPE(0.0);
}

void SynthCtrl::update()
{
    FP_TYPE mainEnv = mainADSR.update();
    FP_TYPE fmEnv = fmADSR.update();
    FP_TYPE amEnv = amADSR.update();

    if (currentPitch != targetPitch)
    {
        glideProgress++;
        if (glideProgress >= glideTicks)
        {
            currentPitch = targetPitch;
        }
        else
        {
            FP_TYPE t = (FP_TYPE)glideProgress * glideInvTicks;
            currentPitch = startPitch + t * (targetPitch - startPitch);
        }
    }
    else
    {
        currentPitch = targetPitch;
    }

    FP_TYPE baseFreq = FP_TYPE(0.0);
    if (currentPitch >= FP_TYPE(0.0))
    {
        int pitchInt = (int)currentPitch;
        if (pitchInt >= int(MIDI_NOTE_COUNT) - 1)
        {
            baseFreq = freqTable[MIDI_NOTE_COUNT - 1];
        }
        else
        {
            FP_TYPE frac = currentPitch - pitchInt;
            int fracBendIndex = (int)(frac * BEND_STEPS_PER_SEMITONE);
            baseFreq = freqTable[pitchInt] * pitchBendTable[fracBendIndex] * pitchBendTable[currentBendIndex];
        }
    }

    FP_TYPE fmOsc = fmLFO.update(sineTable);
    FP_TYPE fmModAmount = fmOsc * fmEnv * fmDepth;
    currentF0 = baseFreq * (FP_TYPE(1.0) + fmModAmount);

    FP_TYPE amOsc = amLFO.update(sineTable);
    FP_TYPE amModAmount = FP_TYPE(1.0) - (amEnv * amDepth) * (FP_TYPE(0.5) - FP_TYPE(0.5) * amOsc);
    currentVol = mainEnv * amModAmount;
}

FP_TYPE SynthCtrl::getF0() const
{
    return currentF0;
}

FP_TYPE SynthCtrl::getVol() const
{
    return currentVol;
}

bool SynthCtrl::active() const
{
    return (mainADSR.getState() != ADSR::State::IDLE);
}

void SynthCtrl::handleNoteOn(byte /*channel*/, byte note, byte velocity)
/*{
    if (velocity == 0)
    {
        handleNoteOff(0, note, 0);
        return;
    }
    FP_TYPE velNorm = velocity / MAX_MIDI_VAL_F;
    if (mainADSR.getState() != ADSR::IDLE && noteTracker.getTopNote() != NOTE_INVALID)
    {
        startPitch = currentPitch;
        targetPitch = (FP_TYPE)note;
        glideProgress = 0;
        // fmADSR.noteOn(velNorm);
        // amADSR.noteOn(velNorm);
    }
    else
    {
        currentPitch = targetPitch = (FP_TYPE)note;
        mainADSR.noteOn(velNorm);
        fmADSR.noteOn(velNorm);
        amADSR.noteOn(velNorm);
    }
    noteTracker.noteOn(note);
}*/
{
	if (velocity == 0)
	{
		handleNoteOff(0, note, 0);
		return;
	}

	FP_TYPE velNorm = velocity / MAX_MIDI_VAL_F;

	bool hasHeldNote = noteTracker.getTopNote() != NOTE_INVALID;
	bool envActive = mainADSR.getState() != ADSR::IDLE;

	if (envActive)
	{
		startPitch = currentPitch;
		targetPitch = (FP_TYPE)note;
		glideProgress = 0;
	}
	else
	{
		currentPitch = targetPitch = (FP_TYPE)note;
	}

	if (!hasHeldNote || !envActive)
	{
		mainADSR.noteOn(velNorm);
		fmADSR.noteOn(FP_TYPE(1));
		amADSR.noteOn(FP_TYPE(1));
	}

	noteTracker.noteOn(note);
}

void SynthCtrl::handleNoteOff(byte /*channel*/, byte note, byte /*velocity*/)
{
    int8_t topNote = noteTracker.getTopNote();
    noteTracker.noteOff(note);
    if (note == topNote)
    {
        topNote = noteTracker.getTopNote();
        if (topNote != NOTE_INVALID)
        {
            startPitch = currentPitch;
            targetPitch = (FP_TYPE)topNote;
            glideProgress = 0;
        }
        else
        {
            mainADSR.noteOff();
            fmADSR.noteOff();
            amADSR.noteOff();
        }
    }
}

void SynthCtrl::handlePitchBend(byte /*channel*/, int bend)
{
    currentBendIndex = bend;
    /*
    if (currentBendIndex < -PITCH_BEND_CENTER)
        currentBendIndex = -PITCH_BEND_CENTER;
    if (currentBendIndex >= PITCH_BEND_CENTER)
        currentBendIndex = PITCH_BEND_CENTER - 1;
    */
}

void SynthCtrl::handleControlChange(byte /*channel*/, byte number, byte value)
{
    switch (number)
    {
    case 5:
        // MIDI 0..127 maps to 1..128 ticks; cache the reciprocal here only.
        glideTicks = uint32_t(value) + 1;
        glideInvTicks = FP_TYPE(1.0) / FP_TYPE(glideTicks);
        break;
    case 73:
        mainADSR.setA(value + 1);
        break;
    case 76:
        mainADSR.setP(value);
        break;
    case 75:
        mainADSR.setD(value + 1);
        break;
    case 79:
        mainADSR.setS(value);
        break;
    case 72:
        mainADSR.setR(value + 1);
        break;
    case 102:
        fmADSR.setA(value + 1);
        break;
    case 103:
        fmADSR.setP(value);
        break;
    case 104:
        fmADSR.setD(value + 1);
        break;
    case 105:
        fmADSR.setS(value);
        break;
    case 106:
        fmADSR.setR(value + 1);
        break;
    case 107:
        fmLFO.setTicks(value);
        break;
    case 108:
        fmDepth = (value / MAX_MIDI_VAL_F) * MAX_FM_DEPTH_RATIO;
        break;
    case 109:
        amADSR.setA(value + 1);
        break;
    case 110:
        amADSR.setP(value);
        break;
    case 111:
        amADSR.setD(value + 1);
        break;
    case 112:
        amADSR.setS(value);
        break;
    case 113:
        amADSR.setR(value + 1);
        break;
    case 114:
        amLFO.setTicks(value);
        break;
    case 115:
        amDepth = (value / MAX_MIDI_VAL_F) * MAX_AM_DEPTH_RATIO;
        break;
    case 120:
        allSoundOff();
        break;
    case 123:
        allNotesOff();
        break;
    }
}

void SynthCtrl::allSoundOff()
{
    while (noteTracker.getTopNote() != NOTE_INVALID)
        noteTracker.noteOff(noteTracker.getTopNote());
    mainADSR.forceIdle();
    fmADSR.forceIdle();
    amADSR.forceIdle();
    currentVol = FP_TYPE(0.0);
}

void SynthCtrl::allNotesOff()
{
    while (noteTracker.getTopNote() != NOTE_INVALID)
        handleNoteOff(0, noteTracker.getTopNote(), 0);
}

