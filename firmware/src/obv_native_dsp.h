/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave DSP building blocks, all fixed point (no FPU, no 64-bit division on the chip).
 * Oscillators: naive saw / pulse with polyBLEP corrections applied as discontinuity events, one sample late:
 * a jump of height H at fraction t before the current sample (t = 0..32767 ~ 0..1 sample) adds
 * H/2 * t^2 to the previous sample and takes H/2 * (1-t)^2 from the current one. A natural wrap and a
 * hard-sync reset are the same event, so one piece of code serves both (as the plan in CLAUDE.md). */
#include "obv_native_tables.h"

static inline int32_t OB_clampi(int32_t OB_v, int32_t OB_lo, int32_t OB_hi) { return OB_v < OB_lo ? OB_lo : OB_v > OB_hi ? OB_hi : OB_v; }

/* tanh(x / 32768 * 2) * 32767, interpolated: x = +-32768 -> +-0.76 full scale, +-65536 -> +-0.96 */
static inline __attribute__((always_inline)) int32_t OB_softclip(int32_t OB_x)
{
    int32_t OB_a = OB_x < 0 ? -OB_x : OB_x, OB_i = OB_a >> 8, OB_y;
    if (OB_i >= 256)
        OB_y = OB_TANH_Q15[256];
    else
        OB_y = OB_TANH_Q15[OB_i] + (((OB_TANH_Q15[OB_i + 1] - OB_TANH_Q15[OB_i]) * (OB_a & 255)) >> 8);
    return OB_x < 0 ? -OB_y : OB_y;
}

static inline uint32_t OB_xorshift(uint32_t *OB_st)
{
    uint32_t OB_s = *OB_st;
    OB_s ^= OB_s << 13;
    OB_s ^= OB_s >> 17;
    OB_s ^= OB_s << 5;
    *OB_st = OB_s;
    return OB_s;
}

/* Q8 semitones (0 = MIDI 0, 8.18 Hz) -> phase step per frame; 12 octave values x 256 fine steps */
static inline uint32_t OB_pitch_inc(int32_t OB_p)
{
    uint32_t OB_n, OB_o;
    OB_p = OB_clampi(OB_p, 0, 131 * 256 + 255);
    OB_n = (uint32_t)OB_p >> 8;
    OB_o = OB_n / 12u;
    return (uint32_t)(((uint64_t)OB_INC_C9[OB_n - OB_o * 12u] * OB_FINE_Q16[OB_p & 255]) >> (26u - OB_o));
}

/* ---- oscillators */
typedef struct {
    uint32_t OB_ph;                                /* phase, wraps */
    int32_t OB_prev;                               /* the previous sample, still open to a correction */
} OB_osc_t;

#define OB_OSC_SAW 1u
#define OB_OSC_PLS 2u

static inline __attribute__((always_inline)) int32_t OB_osc_naive(uint32_t OB_ph, uint32_t OB_pw, uint32_t OB_wave)   /* -98304 .. 98303 */
{
    int32_t OB_v = 0;
    if (OB_wave & OB_OSC_SAW)
        OB_v = (int32_t)(OB_ph >> 16) - 32768;
    if (OB_wave & OB_OSC_PLS)                         /* saw(ph) - saw(ph + pw): a pulse with mean 0 */
        OB_v += (int32_t)(OB_ph >> 16) - (int32_t)((OB_ph + OB_pw) >> 16);
    return OB_v;
}

/* the event: a jump of H (any size up to +-200000) at t (0..32767) before the current sample */
static inline __attribute__((always_inline)) void OB_osc_event(int32_t *OB_prev, int32_t *OB_cur, int32_t OB_h, int32_t OB_t)
{
    int32_t OB_u = 32767 - OB_t, OB_h16 = OB_h >> 4;
    *OB_prev += (OB_h16 * ((OB_t * OB_t) >> 15)) >> 12;   /* H/2 * t^2: h16 = H/16, t^2 in Q15, so >> 12 */
    *OB_cur -= (OB_h16 * ((OB_u * OB_u) >> 15)) >> 12;
}

/* one sample: advance by inc (d = inc >> 15, hw = the height of a wrap of ph: -65536 per saw component,
 * both precomputed per block), return the delayed output; o->prev then holds this sample's naive value (the
 * X-MOD source). reset_t >= 0: a hard-sync reset to phase 0 happened reset_t before the current sample
 * (0..32767 = 0..1 sample). Inlined by force: it is the inner loop. */
static inline __attribute__((always_inline)) int32_t OB_osc_step(OB_osc_t *OB_o, uint32_t OB_inc, uint32_t OB_d, uint32_t OB_pw,
                                                              uint32_t OB_wave, int32_t OB_hw, int32_t OB_reset_t)
{
    uint32_t OB_old = OB_o->OB_ph, OB_ph = OB_old + OB_inc;
    int32_t OB_cur, OB_out = OB_o->OB_prev;
    if (OB_reset_t < 0 || !OB_d) {
        OB_cur = OB_osc_naive(OB_ph, OB_pw, OB_wave);
        if (OB_ph < OB_inc && OB_d)                      /* wrapped: the saw(s) drop */
            OB_osc_event(&OB_out, &OB_cur, OB_hw, (int32_t)(OB_ph / OB_d));
        if ((OB_wave & OB_OSC_PLS) && OB_ph + OB_pw < OB_inc && OB_d)   /* the subtracted saw wraps: the pulse rises */
            OB_osc_event(&OB_out, &OB_cur, 65536, (int32_t)((OB_ph + OB_pw) / OB_d));
    } else {
        uint32_t OB_at = OB_ph - OB_d * (uint32_t)OB_reset_t;   /* the phase at the reset instant */
        int32_t OB_t;
        OB_ph = OB_d * (uint32_t)OB_reset_t;             /* reset, then reset_t on */
        OB_cur = OB_osc_naive(OB_ph, OB_pw, OB_wave);
        if (OB_at < OB_old) {                         /* ph wrapped on the way to the reset */
            OB_t = (int32_t)(OB_at / OB_d) + OB_reset_t;
            OB_osc_event(&OB_out, &OB_cur, OB_hw, OB_t > 32767 ? 32767 : OB_t);
        }
        if ((OB_wave & OB_OSC_PLS) && OB_at + OB_pw < OB_old + OB_pw) {   /* ph + pw wrapped on the way to the reset */
            OB_t = (int32_t)((OB_at + OB_pw) / OB_d) + OB_reset_t;
            OB_osc_event(&OB_out, &OB_cur, 65536, OB_t > 32767 ? 32767 : OB_t);
        }
        OB_osc_event(&OB_out, &OB_cur, OB_osc_naive(0, OB_pw, OB_wave) - OB_osc_naive(OB_at, OB_pw, OB_wave), OB_reset_t);
    }
    OB_o->OB_ph = OB_ph;
    OB_o->OB_prev = OB_cur;
    return OB_out;
}

/* ---- trapezoidal state-variable filter (A. Simper), coefficients per block in Q13 */
typedef struct { int32_t OB_a1, OB_a2, OB_a3, OB_k; } OB_svf_coef_t;
typedef struct { int32_t OB_ic1, OB_ic2; } OB_svf_state_t;

/* cut: Q8 semitones (0..136 << 8); k: damping Q12 (2.0 = 8192; ~0 self-oscillates) */
static inline void OB_svf_coef(OB_svf_coef_t *OB_c, int32_t OB_cut, int32_t OB_k)
{
    int32_t OB_i, OB_g, OB_den;
    OB_cut = OB_clampi(OB_cut, 0, 136 << 8);
    OB_i = OB_cut >> 8;
    OB_g = OB_SVF_G[OB_i];
    if (OB_i < 136)
        OB_g += ((OB_SVF_G[OB_i + 1] - OB_g) * (OB_cut & 255)) >> 8;
    OB_den = 4096 + ((OB_g * (OB_g + OB_k)) >> 12);
    if (OB_den < 64) OB_den = 64;
    OB_c->OB_a1 = (int32_t)((4096u << 13) / (uint32_t)OB_den);
    OB_c->OB_a2 = (OB_c->OB_a1 * OB_g) >> 12;
    OB_c->OB_a3 = (OB_c->OB_a2 * OB_g) >> 12;
    OB_c->OB_k = OB_k;
}

/* one sample; *lp, *bp, *hp out. The band-pass state passes a soft saturation: resonance stays warm and bounded
 * (self-oscillation settles instead of running away). Signals within +-150000 keep the products in 32 bits. */
static inline __attribute__((always_inline)) void OB_svf_step(const OB_svf_coef_t *OB_c, OB_svf_state_t *OB_s, int32_t OB_in, int32_t *OB_lp, int32_t *OB_bp, int32_t *OB_hp)
{
    int32_t OB_v3 = OB_in - OB_s->OB_ic2;
    int32_t OB_v1 = (OB_c->OB_a1 * OB_s->OB_ic1 + OB_c->OB_a2 * OB_v3) >> 13;
    int32_t OB_v2 = OB_s->OB_ic2 + ((OB_c->OB_a2 * OB_s->OB_ic1 + OB_c->OB_a3 * OB_v3) >> 13);
    int32_t OB_n1 = 2 * OB_v1 - OB_s->OB_ic1;
    OB_s->OB_ic1 = (uint32_t)(OB_n1 + 24576) < 49152u ? OB_n1 : OB_softclip(OB_n1 >> 1) << 1;   /* linear below +-0.75, then soft */
    OB_s->OB_ic2 = 2 * OB_v2 - OB_s->OB_ic2;
    *OB_lp = OB_v2;
    *OB_bp = OB_v1;
    *OB_hp = OB_in - ((OB_c->OB_k * OB_v1) >> 12) - OB_v2;
}

static inline __attribute__((always_inline)) int32_t OB_svf_lp(const OB_svf_coef_t *OB_c, OB_svf_state_t *OB_s, int32_t OB_in)   /* the plain stage (24 dB's second) */
{
    int32_t OB_v3 = OB_in - OB_s->OB_ic2;
    int32_t OB_v1 = (OB_c->OB_a1 * OB_s->OB_ic1 + OB_c->OB_a2 * OB_v3) >> 13;
    int32_t OB_v2 = OB_s->OB_ic2 + ((OB_c->OB_a2 * OB_s->OB_ic1 + OB_c->OB_a3 * OB_v3) >> 13);
    OB_s->OB_ic1 = 2 * OB_v1 - OB_s->OB_ic1;
    OB_s->OB_ic2 = 2 * OB_v2 - OB_s->OB_ic2;
    return OB_v2;
}
