/* DRONE: an intentionally slow, continuously evolving ambient instrument.
 * It is a small oscillator engine, not a sample player, so it costs no RAM pool. */
static const char *const N_DRONE_SHAPE[] = {"SINE", "HUM", "AIR", "DARK"};

static void drone_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[0] = 0;
    v->ph[1] = 0x55555555u;
    v->s[0] = v->s[1] = 0;
}

static void drone_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t i, a = v->ph[0], b = v->ph[1], ia = m->inc;
    uint32_t ib = ia + ((ia >> 12) * (uint32_t)(2 + (p[P_E1] & 63)));
    int32_t low = v->s[0], air = v->s[1];
    for (i = 0; i < n; i++) {
        int32_t x = sine_i(a), y = sine_i(b), s;
        switch ((uint32_t)p[P_E0] & 3u) {
        case 1: s = (x + (y >> 1)) >> 1; break;                 /* HUM */
        case 2: s = (x + y + sine_i(a >> 1)) / 3; break;         /* AIR */
        case 3: s = (x >> 1) + (sine_i(a >> 2) >> 2); break;     /* DARK */
        default: s = (x + y) >> 1; break;
        }
        /* slow one-pole colour filter; MOTION modulates its opening over time */
        air += ((sine_i(a >> 3) >> 4) - air) >> 8; /* bounded, very slow movement */
        low += ((s - low) >> 7) * (10 + p[P_E3]);
        s = low + ((air >> 5) * p[P_E2]);
        if (p[P_E4]) s = soft_knee(s, 32000 - p[P_E4] * 120);
        out[i] += voice_amp(s, m, i);
        a += ia;
        b += ib;
    }
    v->ph[0] = a;
    v->ph[1] = b;
    v->s[0] = low;
    v->s[1] = air;
}

static const preset_t DRONE_PRESETS[] = {
 {"NIGHT FOG", {0,7,8,30,0,0,0,0}, {100,110,118,108},0,0,FX(0,66,34,92)},
 {"DEEP SPACE", {3,3,4,16,8,0,0,0}, {112,114,120,112},0,0,FX(0,54,42,96)},
 {"AURORA AIR", {2,15,18,46,0,0,0,0}, {96,104,116,104},0,0,FX(0,74,38,94)},
 {"CATHEDRAL HUM", {1,5,6,22,4,0,0,0}, {106,108,118,110},0,0,FX(0,60,46,98)},
 {"DARK TIDE", {3,10,5,18,18,0,0,0}, {120,112,114,116},0,0,FX(0,38,36,88)},
 {"ABYSS CHOIR", {2,9,20,28,6,0,0,0}, {114,112,120,112},0,0,FX(0,76,44,100)},
 {"VIOLET CLOUD", {1,18,12,42,0,0,0,0}, {102,108,118,104},0,0,FX(0,70,40,94)},
 {"ECLIPSE", {3,4,3,14,26,0,0,0}, {124,116,118,120},0,0,FX(0,44,42,98)}
};

static const engine_t ENG_DRONE = {
 "DRONE", {"DRONE", "COLOUR"},
 {{"SHAPE",F_ENUM,0,3,0,N_DRONE_SHAPE,0},{"DRIFT",F_PCT,0,63,8,0,0},{"MOTION",F_PCT,0,127,8,0,0},{"COLOUR",F_CUTOFF,0,127,32,0,0},
  {"DARK",F_PCT,0,127,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0}},
 DRONE_PRESETS, NELEM(DRONE_PRESETS), 1, drone_note_on, drone_render, 0x51A8, {P_E1,P_E2,P_E3,P_REL}
};
