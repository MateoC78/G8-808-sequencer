// synth source


#include "../synth.h"
#include "../audio.h"

#define FREQ_TO_INC(hz)     ((uint32_t)(hz) * (uint32_t)(4294967296.0 / SAMPLE_RATE_HZ))

#define AMP_NORMAL          42000
#define AMP_ACCENT          65535
#define AMP_OFF             256

static const int16_t sineTable[256] = {
    0,    50,   100,   151,   201,   251,   300,   350,   399,   449,   497,   546,   594,   642,   690,   737,
      783,   830,   875,   920,   965,  1009,  1052,  1095,  1137,  1179,  1219,  1259,  1299,  1337,  1375,  1411,
     1447,  1483,  1517,  1550,  1582,  1614,  1644,  1674,  1702,  1729,  1756,  1781,  1805,  1828,  1850,  1871,
     1891,  1910,  1927,  1944,  1959,  1973,  1986,  1997,  2008,  2017,  2025,  2032,  2037,  2041,  2045,  2046,
     2047,  2046,  2045,  2041,  2037,  2032,  2025,  2017,  2008,  1997,  1986,  1973,  1959,  1944,  1927,  1910,
     1891,  1871,  1850,  1828,  1805,  1781,  1756,  1729,  1702,  1674,  1644,  1614,  1582,  1550,  1517,  1483,
     1447,  1411,  1375,  1337,  1299,  1259,  1219,  1179,  1137,  1095,  1052,  1009,   965,   920,   875,   830,
      783,   737,   690,   642,   594,   546,   497,   449,   399,   350,   300,   251,   201,   151,   100,    50,
        0,   -50,  -100,  -151,  -201,  -251,  -300,  -350,  -399,  -449,  -497,  -546,  -594,  -642,  -690,  -737,
     -783,  -830,  -875,  -920,  -965, -1009, -1052, -1095, -1137, -1179, -1219, -1259, -1299, -1337, -1375, -1411,
    -1447, -1483, -1517, -1550, -1582, -1614, -1644, -1674, -1702, -1729, -1756, -1781, -1805, -1828, -1850, -1871,
    -1891, -1910, -1927, -1944, -1959, -1973, -1986, -1997, -2008, -2017, -2025, -2032, -2037, -2041, -2045, -2046,
    -2047, -2046, -2045, -2041, -2037, -2032, -2025, -2017, -2008, -1997, -1986, -1973, -1959, -1944, -1927, -1910,
    -1891, -1871, -1850, -1828, -1805, -1781, -1756, -1729, -1702, -1674, -1644, -1614, -1582, -1550, -1517, -1483,
    -1447, -1411, -1375, -1337, -1299, -1259, -1219, -1179, -1137, -1095, -1052, -1009,  -965,  -920,  -875,  -830,
     -783,  -737,  -690,  -642,  -594,  -546,  -497,  -449,  -399,  -350,  -300,  -251,  -201,  -151,  -100,   -50
};

static Voice voices[NUM_TRACKS];

void Synth_Init(void) {
    for (uint8_t i = 0; i < NUM_TRACKS; i++) voices[i].active = false;
}

void Synth_Trigger(uint8_t track, bool accent) {
    Voice *v = &voices[track];

    switch (track) {
        case TRACK_KICK:
            // sine wave starting at 170 Hz and falls to 50 Hz
            // v->phase = 0;
            // v->phaseBase = FREQ_TO_INC(60);
            // v->phaseSweep = FREQ_TO_INC(240);
            // v->pitchShift = 9;
            // v->ampShift = 12;
            v->phase      = 0;
            v->phaseBase  = FREQ_TO_INC(1000);   // TEST: short, high blip
            v->phaseSweep = 0;
            v->pitchShift = 9;
            v->ampShift   = 8;                   // dies in about 80 ms
            break;

        // add other drum sounds here
        default:
            return;
    }

    v->amp = accent ? AMP_ACCENT : AMP_NORMAL;
    v->active = true;
}

static int32_t Voice_Next(Voice *v) {
    v->phase += v->phaseBase + v->phaseSweep;
    v->phaseSweep -= v->phaseSweep >> v->pitchShift;
    v->amp -= (v->amp >> v->ampShift) + 1;

    if (v->amp < AMP_OFF) {
        v->active = false;
        return 0;
    }

    return ((int32_t)sineTable[v->phase >> 24] * (int32_t)(v->amp >> 4)) >> 12;
}

int32_t Synth_Next(void) {
    int32_t mix = 0;
    for (int i = 0; i < NUM_TRACKS; i++) if (voices[i].active) mix += Voice_Next(&voices[i]);
    return mix;
}
