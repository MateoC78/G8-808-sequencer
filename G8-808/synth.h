// G8-808 drum synthesis sounds using integer math

#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h>
#include <stdbool.h>

#define NUM_TRACKS      6

// Voice: control parameters used to synthesize a 'drum' sound from sine wave
typedef struct {
    bool active;
    uint32_t phase;
    uint32_t phaseBase;
    uint8_t pitchShift;
    uint32_t amp;
    uint8_t ampShift;
} Voice;

void Synth_Init(void);
void Synth_Trigger(uint8_t track, bool accent);
int32_t Synth_Next(void);

#endif