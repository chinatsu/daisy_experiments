#include "daisysp.h"
#include "daisy_pod.h"

#define UINT32_MSB 0x80000000U
#define MAX_LENGTH 16U

using namespace daisy;
using namespace daisysp;

DaisyPod hardware;

Oscillator osc; // for kick
WhiteNoise noise; // for snare

AdEnv     kickVolEnv, kickPitchEnv, snareEnv; // envelopes for triggers and control pitch
Metro     tick; // for keeping track of time


static ReverbSc rev; // reverb effect
static Overdrive drive; // overdrive effect

bool    kickSeq[MAX_LENGTH]; // step sequence for kick
bool    snareSeq[MAX_LENGTH]; // step sequence for snare

uint8_t cursor, playCursor = 0; // cursor for keeping track of positions in step sequences

float k1old, k2old = 0.f; // for comparing with old pot values

uint8_t innerMode, outerMode = 0; // for keeping track of modes
								  // - outerMode == 0 is "edit", == 1 is "play"
								  //   "edit" lets us modify kickSeq and snareSeq with buttons
								  //   "play" loops the sequences and lets us modify parameters based on innerMode
								  // innerMode only matters during "play".
								  // == 0 is "kick", == 1 is "snare", == 2 is "reverb", == 3 is "overdrive", == 4 is "tempo"?

float tempo = 3;
float drywet;

void ProcessTick();
void ProcessControls();
void GetReverbSample(float &out, float in);

const uint8_t MODE_EDIT = 0;
const uint8_t MODE_PLAY = 1;
const uint8_t MODE_KICK = 0;
const uint8_t MODE_SNARE = 1;
const uint8_t MODE_REVERB = 2;
const uint8_t MODE_DRIVE = 3;
const uint8_t MODE_TEMPO = 4;
const uint8_t TOTAL_MODES = 5;


void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    float osc_out, noise_out, snr_env_out, kck_env_out, sig, output;

    ProcessTick();

    ProcessControls();

    //audio
    for(size_t i = 0; i < size; i += 2)
    {
        snr_env_out = snareEnv.Process();
        kck_env_out = kickVolEnv.Process();

        osc.SetFreq(kickPitchEnv.Process());
        osc.SetAmp(kck_env_out);

        osc_out = osc.Process();

        noise_out = noise.Process();
        noise_out *= snr_env_out;

        sig = .5 * noise_out + .5 * osc_out;
		sig = drive.Process(sig);
		GetReverbSample(output, sig);

        out[i]     = output;
        out[i + 1] = output;
    }
}

void SetupDrums(float samplerate)
{
    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);
    osc.SetAmp(1);

    noise.Init();

	rev.Init(samplerate);
	rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.f);

	drive.Init();
	drive.SetDrive(0.1);

    snareEnv.Init(samplerate);
    snareEnv.SetTime(ADENV_SEG_ATTACK, .01);
    snareEnv.SetTime(ADENV_SEG_DECAY, .2);
    snareEnv.SetMax(1);
    snareEnv.SetMin(0);

    kickPitchEnv.Init(samplerate);
    kickPitchEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickPitchEnv.SetTime(ADENV_SEG_DECAY, .05);
    kickPitchEnv.SetMax(400);
    kickPitchEnv.SetMin(50);

    kickVolEnv.Init(samplerate);
    kickVolEnv.SetTime(ADENV_SEG_ATTACK, .01);
    kickVolEnv.SetTime(ADENV_SEG_DECAY, 1);
    kickVolEnv.SetMax(1);
    kickVolEnv.SetMin(0);
}

void SetSeq(bool* seq, bool in)
{
    for(uint32_t i = 0; i < MAX_LENGTH; i++)
    {
        seq[i] = in;
    }
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioBlockSize(4);
    float samplerate   = hardware.AudioSampleRate();
    float callbackrate = hardware.AudioCallbackRate();

    //setup the drum sounds
    SetupDrums(samplerate);
	
    tick.Init(tempo, callbackrate);

    SetSeq(snareSeq, 0);
    SetSeq(kickSeq, 0);

	hardware.led1.Set(!outerMode, 0, outerMode);
    hardware.StartAdc();
    hardware.StartAudio(AudioCallback);

    // Loop forever
    for(;;) {}
}


void ProcessTick()
{
    if(tick.Process())
    {
        playCursor++;
    	playCursor %= MAX_LENGTH;
    	
        if(kickSeq[playCursor])
        {
            kickVolEnv.Trigger();
            kickPitchEnv.Trigger();
        }

        if(snareSeq[playCursor])
        {
            snareEnv.Trigger();
        }
    }
}


void UpdateEncoder()
{
	if(hardware.encoder.RisingEdge()) {
		outerMode += 1;
		outerMode = (outerMode % 2 + 2) % 2;
		hardware.led1.Set(!outerMode, 0, outerMode);
	}

	if(outerMode == MODE_EDIT) {
		cursor += hardware.encoder.Increment();
		cursor = (cursor % MAX_LENGTH + MAX_LENGTH) % MAX_LENGTH;
	}
	if(outerMode == MODE_PLAY) {
		innerMode += hardware.encoder.Increment();
		innerMode = (innerMode % TOTAL_MODES + TOTAL_MODES) % TOTAL_MODES;
		hardware.led2.Set(0, 0.2f*innerMode, 0);
	}
}

bool hasChanged(float val, float oldVal) {
	return abs(oldVal - val) > 0.01;
}

void EditEnvelope(AdEnv& env, uint8_t type, float val) {
	env.SetTime(type, val);
}

void EditDrive(float val) {
	drive.SetDrive(val);
}

void EditReverbFeedback(float val) {
	rev.SetFeedback(val);
}

void EditReverbCutoff(float val) {
	float cutoff_max = hardware.AudioSampleRate() / 2;
	rev.SetLpFreq(val*cutoff_max);
}

void EditDrywet(float val) {
	drywet = val;
}

void EditTempo(float val) {
	tempo = val*10.0;
	tempo = std::fminf(tempo, 10.f);
    tempo = std::fmaxf(.5f, tempo);
	tick.SetFreq(tempo);
}

void UpdateKnobs()
{

    float k1 = hardware.knob1.Process();
    float k2 = hardware.knob2.Process();
	
	if(outerMode == MODE_PLAY) {
		if (hasChanged(k1, k1old)) {
			switch (innerMode) {
				case MODE_SNARE: EditEnvelope(snareEnv, ADENV_SEG_DECAY, k1*10.0); break;
				case MODE_DRIVE: EditDrive(k1); break;
				case MODE_REVERB: EditReverbFeedback(k1); break;
				case MODE_TEMPO: EditTempo(k1);
				default: EditEnvelope(kickVolEnv, ADENV_SEG_DECAY, k1*10.0); break;
			}
		
			k1old = k1;
		}
		if (hasChanged(k2, k2old)) {
			switch (innerMode) {
				case MODE_SNARE: EditEnvelope(snareEnv, ADENV_SEG_ATTACK, k2*10.0); break;
				case MODE_DRIVE: EditDrywet(k2); break;
				case MODE_REVERB: EditReverbCutoff(k2); break;
				case MODE_TEMPO: break;
				default: EditEnvelope(kickVolEnv, ADENV_SEG_ATTACK, k2*10.0); break;
			}

			k2old = k2;
		}
	}
	else {
		k1old = k1;
		k2old = k2;
	}
}



void UpdateButtons() {
	if(outerMode == MODE_EDIT) {
		if (hardware.button2.RisingEdge()) {
			kickSeq[cursor] = !kickSeq[cursor];
		} else if (hardware.button1.RisingEdge()) {
			snareSeq[cursor] = !snareSeq[cursor];
		}
	}
}

void ProcessControls()
{
    hardware.ProcessDigitalControls();
    hardware.ProcessAnalogControls();

    UpdateEncoder();

    UpdateKnobs();

    UpdateButtons();

    hardware.UpdateLeds();
}

void GetReverbSample(float &out, float in)
{
    rev.Process(in, in, &out, &out);
    out = drywet * out + (1 - drywet) * in;
}
