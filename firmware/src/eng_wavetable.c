/* WAVETABLE: compact two-oscillator table synthesizer.  Tables live in flash;
 * there are no samples or large RAM buffers, so it remains safe on FM-1. */
static const char *const N_WT[] = {"SINE", "TRI", "SAW", "PULSE", "SOFT", "FORM", "METAL", "DIGI"};
/* 8 cyclic waves, 16 points each. Linear interpolation makes the tables smooth. */
static const int16_t WT[8][16] = {
    {0,12539,23170,30273,32767,30273,23170,12539,0,-12539,-23170,-30273,-32767,-30273,-23170,-12539},
    {0,8192,16384,24576,32767,24576,16384,8192,0,-8192,-16384,-24576,-32767,-24576,-16384,-8192},
    {-32767,-28672,-24576,-20480,-16384,-12288,-8192,-4096,0,4096,8192,12288,16384,20480,24576,28672},
    {32767,32767,32767,32767,32767,32767,32767,32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767,-32767},
    {0,19700,29000,32767,30000,25000,16000,7000,0,-7000,-16000,-25000,-30000,-32767,-29000,-19700},
    {0,24500,32767,21000,9000,26000,17000,6000,0,-6000,-17000,-26000,-9000,-21000,-32767,-24500},
    {0,32767,8000,-26000,18000,-12000,28000,-5000,0,5000,-28000,12000,-18000,26000,-8000,-32767},
    {0,10000,26000,6000,32767,-4000,18000,28000,0,-28000,-18000,4000,-32767,-6000,-26000,-10000}
};

static inline int32_t wt_sample(uint32_t wave, uint32_t ph)
{
    const int16_t *w = WT[wave & 7u];
    uint32_t p = ph >> 28, f = (ph >> 13) & 32767u;
    int32_t a = w[p], b = w[(p + 1u) & 15u];
    return a + ((b - a) * (int32_t)f >> 15);
}

static void wt_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[0] = 0;
    v->ph[1] = 0x40000000u;
    v->s[0] = 0;
}

static void wt_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t i, ph1 = v->ph[0], ph2 = v->ph[1], inc1 = m->inc;
    uint32_t inc2 = inc1 + ((inc1 >> 10) * (uint32_t)(p[P_E3] & 127));
    int32_t z = v->s[0], morph = clamp(p[P_E2] + ((m->shape - (64 << 8)) >> 8), 0, 127);
    int32_t cut = 16 + p[P_E4];
    for (i = 0; i < n; i++) {
        int32_t a = wt_sample((uint32_t)p[P_E0], ph1);
        int32_t b = wt_sample((uint32_t)p[P_E1], ph2);
        int32_t y = a + ((b - a) * morph >> 7);
        /* resonant character is intentionally gentle: cutoff is stable even at high notes. */
        z += ((y - z) >> 7) * cut;
        if (p[P_E5] > 0) z = soft_knee(z, 30000 - p[P_E5] * 140);
        if (p[P_E7] > 0) z = softclip(z + mulq15(z, p[P_E7] * 180));
        out[i] += voice_amp(z, m, i);
        ph1 += inc1;
        ph2 += inc2;
    }
    v->ph[0] = ph1;
    v->ph[1] = ph2;
    v->s[0] = z;
}

static const preset_t WT_PRESETS[] = {
 {"WAVE RUNNER", {2,2,40,8,92,16,0,10}, {4,72,102,42},0,0,FX(0,30,20,30)},
 {"CYBER BASS", {2,3,74,3,54,36,0,24}, {0,58,92,26},0,1,FX(8,0,8,8),XP(P_TRANS+1,-24)},
 {"GLASS MOTION", {0,6,68,18,102,8,0,4}, {10,84,76,70},0,0,FX(0,42,30,64)},
 {"VECTOR PAD", {4,5,64,11,62,12,0,0}, {76,96,112,92},0,0,FX(0,70,28,76)},
 {"ICE PLUCK", {0,6,78,15,110,4,0,6}, {0,60,0,54},0,0,FX(0,24,26,54)},
 {"NEON SWEEP", {2,5,58,9,68,28,0,8}, {54,92,106,84},0,0,FX(0,64,30,74),XP(P_LRATE+1,24,P_LD_FLT+1,18)},
 {"POLY PIXEL", {7,3,48,4,84,14,0,16}, {8,74,92,50},0,0,FX(0,36,18,32)},
 {"ORBIT ARP", {2,6,66,14,88,12,0,10}, {2,66,94,34},0,0,FX(0,40,26,46),ARP(1,2,2,74)},
 {"DEEP VECTOR", {4,2,84,5,44,34,0,28}, {18,76,104,54},0,0,FX(12,18,14,24)},
 {"CITY LIGHTS", {5,2,60,10,72,16,0,12}, {48,88,108,74},0,0,FX(0,46,24,58)},
 {"RETRO GLASS", {0,6,76,12,98,6,0,4}, {4,78,20,58},0,0,FX(0,26,28,52)},
 {"PPG BELL", {0,7,82,8,106,5,0,4}, {0,82,0,72},0,0,FX(0,34,26,64)},
 {"WAVE STRINGS", {4,5,70,12,64,12,0,4}, {58,94,112,90},0,0,FX(0,82,22,66)}
};

static const engine_t ENG_WAVETABLE = {
 "MORPH", {"MORPH", "TONE"},
 {{"A",F_ENUM,0,7,2,N_WT,0},{"B",F_ENUM,0,7,3,N_WT,0},{"MORPH",F_PCT,0,127,64,0,0},{"DTN",F_INT,0,127,8,0,"ct"},
  {"CUT",F_CUTOFF,0,127,84,0,0},{"RES",F_PCT,0,127,16,0,0},{"SPRD",F_PCT,0,127,0,0,0},{"DRV",F_PCT,0,127,0,0,0}},
 WT_PRESETS, NELEM(WT_PRESETS), 1, wt_note_on, wt_render, 0x55DF, {P_E2,P_E3,P_E4,P_E7}
};
