// G8-808 drum synthesis sounds using integer math

#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h>
#include <stdbool.h>

#define NUM_TRACKS      6
#define SAMPLE_RATE_HZ      16000


enum {
    TRACK_KICK = 0,
    TRACK_SNARE,        // Milestone 2
    TRACK_CHAT,         // closed hi-hat (Milestone 2)
    TRACK_OHAT,         // open hi-hat   (Milestone 2)
    TRACK_CLAP,         // Milestone 2
    TRACK_TOM           // Milestone 2
};

// Voice: control parameters used to synthesize a 'drum' sound from sine wave
typedef struct {
    bool active;
    uint32_t phase;
    uint32_t phaseBase;
    uint32_t phaseSweep;
    uint8_t pitchShift;
    uint32_t amp;
    uint8_t ampShift;
} Voice;

void Synth_Init(void);
void Synth_Trigger(uint8_t track, bool accent);
int32_t Synth_Next(void);

#endif