/* SPDX-License-Identifier: GPL-3.0-only
 * Native OberVave synth: Copyright (C) 2026 zvenson.
 * Adapted for the Spampy per-part scheduler; upstream notices retained. */
#include "obv_native_params.h"
#include "obv_native_dsp.h"
#define OB_NV 8u
#define OB_NHELD 16u
#define OB_ENV_ONE (1 << 24)
#define OB_LEVEL_FLOOR 2048                        /* -78 dB: a release below this frees the voice */

enum { OB_E_IDLE, OB_E_ATT, OB_E_DEC, OB_E_SUS, OB_E_REL };
typedef struct { uint32_t OB_stage; int32_t OB_level; } OB_env_t;   /* level Q24 */

typedef struct {
    OB_osc_t OB_o1, OB_o2;
    uint32_t OB_noise;
    OB_env_t OB_fe, OB_ae;
    OB_svf_state_t OB_f1, OB_f2;
    OB_svf_coef_t OB_c1, OB_c2;
    uint32_t OB_inc1, OB_inc2, OB_pw1, OB_pw2;              /* this block's oscillator settings */
    int32_t OB_amp0, OB_amp1;                         /* Q15 at the block's start and end */
    int32_t OB_pitch16, OB_target16;                  /* Q16 semitones: the glide */
    int32_t OB_det8;                               /* unison / dual offset, Q8 */
    int32_t OB_gl, OB_gr;                             /* pan gains Q12 */
    int32_t OB_drift;                              /* the vintage random walk */
    uint32_t OB_lfo_blk;                           /* blocks since the note on (LFO delay / fade) */
    int32_t OB_lfo_fade;                           /* Q15 */
    uint32_t OB_seq;                               /* allocation order */
    uint8_t OB_note, OB_vel, OB_gate;                    /* gate: the key (or the arp) holds it */
    uint8_t OB_part;                               /* 0: sound A, 1: track B's own sound */
} OB_voice_t;

typedef struct {
    const OB_patch_t *OB_p;
    uint32_t OB_lfo_ph, OB_lfo_rnd_a, OB_lfo_rnd_b;
    int32_t OB_lfo_val;                            /* this block, Q15 */
    int32_t OB_last_pitch16;                       /* where a glide starts from */
    int32_t OB_fenv_amt, OB_keytrk, OB_velflt, OB_lfo_pd, OB_lfo_fd, OB_lfo_pwd, OB_lfo_ad, OB_wheel_q, OB_bend8, OB_env_o2, OB_det_fine8;
    int32_t OB_lvl1, OB_lvl2, OB_lvln, OB_drive_q12, OB_reso_k, OB_vint, OB_sus_f, OB_sus_a, OB_gain_q12, OB_morph_q;
    int32_t OB_cut_sm, OB_reso_sm;                    /* CUTOFF and RESO slewed (Q8), ~25 ms: a CC sweep in semitone steps glides */
    uint32_t OB_pw1_base, OB_pw2_base, OB_w1, OB_w2, OB_ftype, OB_sync_on, OB_xmod_q, OB_glide_k;
uint32_t OB_rng;
} OB_part_t;
static const int16_t OB_VINT_TUNE8[OB_NV] = {-20, 12, 7, -15, 18, -9, 3, -17};     /* +-8 cents */
static const int16_t OB_VINT_CUT8[OB_NV] = {-300, 250, 120, -380, 60, 330, -180, 220};   /* +-1.5 semitones */
static const int8_t OB_VINT_ENV[OB_NV] = {10, -6, 14, -12, 3, -16, 8, -4};         /* /128: +-12 % on the times */
static const int8_t OB_PAN_CARD[OB_NV] = {-40, 40, -24, 24, -56, 56, -10, 10};     /* the originals' pan pots */

static void OB_voice_pan(OB_voice_t *OB_v, int32_t OB_pan)   /* -64..64 */
{
    uint32_t OB_pos = (uint32_t)(OB_clampi(OB_pan, -64, 64) + 64);
    OB_v->OB_gl = OB_PAN_Q12[128u - OB_pos];
    OB_v->OB_gr = OB_PAN_Q12[OB_pos];
}

static inline void OB_env_tick(OB_env_t *OB_e, uint32_t OB_a, uint32_t OB_d, int32_t OB_sus, uint32_t OB_r, int32_t OB_gate, int32_t OB_vint)
{
    int32_t OB_k, OB_target;
    if (OB_gate && OB_e->OB_stage >= OB_E_REL) OB_e->OB_stage = OB_E_ATT;
    if (!OB_gate && OB_e->OB_stage && OB_e->OB_stage < OB_E_REL) OB_e->OB_stage = OB_E_REL;
    switch (OB_e->OB_stage) {
    case OB_E_ATT:
        OB_k = (int32_t)OB_ENV_K_ATT[OB_a];
        OB_k += (int32_t)(((int64_t)OB_k * OB_vint) >> 7);
        OB_target = OB_ENV_ONE + (OB_ENV_ONE * 3 >> 3);  /* aims at 1.3, stops at 1.0 */
        OB_e->OB_level += (int32_t)(((int64_t)(OB_target - OB_e->OB_level) * OB_k) >> 24);
        if (OB_e->OB_level >= OB_ENV_ONE) { OB_e->OB_level = OB_ENV_ONE; OB_e->OB_stage = OB_E_DEC; }
        break;
    case OB_E_DEC:
        OB_k = (int32_t)OB_ENV_K_DEC[OB_d];
        OB_k += (int32_t)(((int64_t)OB_k * OB_vint) >> 7);
        OB_e->OB_level += (int32_t)(((int64_t)(OB_sus - OB_e->OB_level) * OB_k) >> 24) - 16;
        if (OB_e->OB_level <= OB_sus + 64) { OB_e->OB_level = OB_sus; OB_e->OB_stage = OB_E_SUS; }
        break;
    case OB_E_SUS:
        OB_e->OB_level = OB_sus;                         /* the knob moves it */
        break;
    case OB_E_REL:
        OB_k = (int32_t)OB_ENV_K_DEC[OB_r];
        OB_k += (int32_t)(((int64_t)OB_k * OB_vint) >> 7);
        OB_e->OB_level += (int32_t)(((int64_t)(0 - OB_e->OB_level) * OB_k) >> 24) - 32;
        if (OB_e->OB_level < OB_LEVEL_FLOOR) { OB_e->OB_level = 0; OB_e->OB_stage = OB_E_IDLE; }
        break;
    default:
        OB_e->OB_level = 0;
    }
}

/* ---- the LFO (global; per voice a phase offset in SPREAD and a delay / fade) */
static int32_t OB_lfo_shape(const OB_part_t *OB_g, uint32_t OB_ph, uint32_t OB_shape)
{
    int32_t OB_p;
    switch (OB_shape) {
    case OB_L_TRI:
        OB_p = (int32_t)((OB_ph + 0x40000000u) >> 15);
        return OB_p < 65536 ? OB_p - 32768 : 98303 - OB_p;
    case OB_L_SAW: return 32767 - (int32_t)(OB_ph >> 16);
    case OB_L_RAMP: return (int32_t)(OB_ph >> 16) - 32768;
    case OB_L_SQR: return OB_ph < 0x80000000u ? 32767 : -32767;
    case OB_L_SH: return (int32_t)(OB_g->OB_lfo_rnd_b >> 16) - 32768;
    default: {                                  /* smooth random: a line from one random value to the next */
        int32_t OB_a = (int32_t)(OB_g->OB_lfo_rnd_a >> 16) - 32768, OB_b = (int32_t)(OB_g->OB_lfo_rnd_b >> 16) - 32768;
        return OB_a + (int32_t)(((int64_t)(OB_b - OB_a) * (OB_ph >> 8)) >> 24);
    }
    }
}

static void OB_lfo_tick(OB_part_t *OB_g)
{
    uint32_t OB_inc = OB_LFO_INC[OB_g->OB_p->OB_v[OB_P_LFO_RATE]], OB_old = OB_g->OB_lfo_ph;
    OB_g->OB_lfo_ph += OB_inc;
    if (OB_g->OB_lfo_ph < OB_old) { OB_g->OB_lfo_rnd_a = OB_g->OB_lfo_rnd_b; OB_g->OB_lfo_rnd_b = OB_xorshift(&OB_g->OB_rng); }
    OB_g->OB_lfo_val = OB_lfo_shape(OB_g, OB_g->OB_lfo_ph, OB_g->OB_p->OB_v[OB_P_LFO_SHAPE]);
}

/* ---- one control block for a voice: pitch, cutoff, envelopes, amp */
static int32_t OB_vol_q12;                         /* the output stage: CC 7 (each sound's VOLUME is in its voices) */

static void OB_block_globals(OB_part_t *OB_g)            /* a sound's derived numbers, once per block */
{
    const uint8_t *OB_pv = OB_g->OB_p->OB_v;
    int32_t OB_d;
    OB_g->OB_fenv_amt = (int32_t)OB_pv[OB_P_FENV_AMT] - 64;
    OB_g->OB_keytrk = OB_pv[OB_P_KEYTRK];
    OB_g->OB_velflt = OB_pv[OB_P_VEL_FLT];
    OB_d = OB_pv[OB_P_LFO_PITCH]; OB_g->OB_lfo_pd = (OB_d * OB_d) >> 7;
    OB_d = OB_pv[OB_P_LFO_FLT]; OB_g->OB_lfo_fd = (OB_d * OB_d) >> 7;
    OB_d = OB_pv[OB_P_LFO_PWM]; OB_g->OB_lfo_pwd = (OB_d * OB_d) >> 7;
    OB_g->OB_lfo_ad = OB_pv[OB_P_LFO_AMP];
    OB_g->OB_wheel_q = OB_pv[OB_P_MOD_DEPTH];               /* the knob and the mod wheel (CC 1) are the same value */
    OB_g->OB_bend8 = (0 * OB_pv[OB_P_BEND] * 256) >> 13;
    OB_g->OB_env_o2 = (int32_t)OB_pv[OB_P_ENV_O2] - 64;
    OB_g->OB_det_fine8 = ((int32_t)OB_pv[OB_P_O2_FINE] - 64) * 2;
    OB_d = OB_pv[OB_P_O1_LVL]; OB_g->OB_lvl1 = (OB_d * OB_d) >> 7;
    OB_d = OB_pv[OB_P_O2_LVL]; OB_g->OB_lvl2 = (OB_d * OB_d) >> 7;
    OB_d = OB_pv[OB_P_NOISE]; OB_g->OB_lvln = (OB_d * OB_d) >> 7;
    OB_d = OB_pv[OB_P_DRIVE]; OB_g->OB_drive_q12 = OB_d ? 4096 + (OB_d * OB_d * 7 * 4096 / 16129) : 0;
    OB_g->OB_cut_sm += ((OB_pv[OB_P_CUTOFF] << 8) - OB_g->OB_cut_sm) >> 6;   /* tau = 64 blocks = 23 ms */
    OB_g->OB_reso_sm += ((OB_pv[OB_P_RESO] << 8) - OB_g->OB_reso_sm) >> 6;
    OB_g->OB_reso_k = 8192 - OB_g->OB_reso_sm * 8400 / (127 * 256);
    OB_g->OB_vint = OB_pv[OB_P_VINTAGE];
    OB_g->OB_sus_f = OB_pv[OB_P_FS] * (OB_ENV_ONE / 127);
    OB_g->OB_sus_a = OB_pv[OB_P_AS] * (OB_ENV_ONE / 127);
    OB_d = OB_pv[OB_P_VOLUME];
    OB_g->OB_gain_q12 = (OB_d * OB_d) >> 2;                 /* (x CC 7 at the output: as VOLUME x CC 7 squared was) */
    OB_g->OB_pw1_base = ((uint32_t)OB_pv[OB_P_O1_PW] << 25) + (1u << 24);
    OB_g->OB_pw2_base = ((uint32_t)OB_pv[OB_P_O2_PW] << 25) + (1u << 24);
    OB_g->OB_w1 = OB_pv[OB_P_O1_WAVE];
    OB_g->OB_w2 = OB_pv[OB_P_O2_WAVE];
    OB_g->OB_ftype = OB_pv[OB_P_FTYPE];
    OB_g->OB_morph_q = OB_pv[OB_P_FMORPH];                  /* SEM: 0 LP, 64 notch, 127 HP */
    OB_g->OB_sync_on = OB_pv[OB_P_SYNC];
    OB_d = OB_pv[OB_P_XMOD]; OB_g->OB_xmod_q = (uint32_t)((OB_d * OB_d) >> 7);
    OB_g->OB_glide_k = OB_GLIDE_K[OB_pv[OB_P_GLIDE]];
}

static inline uint32_t OB_pw_mod(uint32_t OB_base, int32_t OB_mod)
{
    int64_t OB_p = (int64_t)OB_base + ((int64_t)OB_mod << 8);
    if (OB_p < (1 << 26)) OB_p = 1 << 26;
    if (OB_p > (int64_t)0xFFFFFFFFu - (1 << 26)) OB_p = (int64_t)0xFFFFFFFFu - (1 << 26);
    return (uint32_t)OB_p;
}

static void OB_voice_block(OB_voice_t *OB_v, uint32_t OB_card,const OB_part_t *OB_g)
{
    const uint8_t *OB_pv = OB_g->OB_p->OB_v;
    int32_t OB_lfo, OB_p8, OB_p1, OB_p2, OB_cut, OB_fenv, OB_tmp, OB_velg, OB_trem, OB_vint = OB_g->OB_vint, OB_vt = (OB_vint * OB_VINT_ENV[OB_card]) >> 7, OB_lfo_to, OB_a;
    uint32_t OB_ph, OB_glide_k = OB_g->OB_glide_k;
    /* glide */
    if (OB_v->OB_pitch16 != OB_v->OB_target16) {
        if (!OB_glide_k) OB_v->OB_pitch16 = OB_v->OB_target16;
        else {
            int32_t OB_diff = OB_v->OB_target16 - OB_v->OB_pitch16;
            OB_v->OB_pitch16 += (int32_t)(((int64_t)OB_diff * OB_glide_k) >> 16);
            if (OB_diff > -64 && OB_diff < 64) OB_v->OB_pitch16 = OB_v->OB_target16;
        }
    }
    /* envelopes */
    OB_env_tick(&OB_v->OB_fe, OB_pv[OB_P_FA], OB_pv[OB_P_FD], OB_g->OB_sus_f, OB_pv[OB_P_FR], OB_v->OB_gate, OB_vt);
    OB_env_tick(&OB_v->OB_ae, OB_pv[OB_P_AA], OB_pv[OB_P_AD], OB_g->OB_sus_a, OB_pv[OB_P_AR], OB_v->OB_gate, OB_vt);
    OB_fenv = OB_v->OB_fe.OB_level >> 9;                    /* Q15 */
    /* the LFO as this voice sees it */
    OB_ph = OB_g->OB_lfo_ph;
    if (OB_pv[OB_P_LFO_PHASE] == OB_LP_SPREAD) OB_ph += OB_card << 29;
    OB_lfo = OB_pv[OB_P_LFO_PHASE] == OB_LP_SPREAD ? OB_lfo_shape(OB_g, OB_ph, OB_pv[OB_P_LFO_SHAPE]) : OB_g->OB_lfo_val;
    if (OB_v->OB_lfo_fade < 32767) {
        uint32_t OB_dl = OB_LFO_DELAY_BLK[OB_pv[OB_P_LFO_DELAY]];
        if (OB_v->OB_lfo_blk >= OB_dl) { OB_v->OB_lfo_fade += 32767 / (int32_t)(OB_dl / 2u + 1u) + 1; if (OB_v->OB_lfo_fade > 32767) OB_v->OB_lfo_fade = 32767; }
        OB_v->OB_lfo_blk++;
    }
    OB_lfo = (OB_lfo * OB_v->OB_lfo_fade) >> 15;
    /* vintage: a slow random walk on the pitch */
    OB_v->OB_drift += (int32_t)(OB_xorshift(&OB_v->OB_noise) & 31u) - 16;
    OB_v->OB_drift -= OB_v->OB_drift >> 11;
    /* pitch, Q8 semitones */
    OB_p8 = (OB_v->OB_pitch16 >> 8) + OB_g->OB_bend8 + 0 + OB_v->OB_det8 + ((OB_VINT_TUNE8[OB_card] * OB_vint) >> 7) + ((OB_v->OB_drift * OB_vint) >> 10);
    OB_p8 += (OB_lfo * OB_g->OB_wheel_q * 4) >> 15;         /* the wheel's vibrato */
#if OB_BOARD_SMK37
    OB_p8 += (OB_lfo * OB_ctl_vib) >> 15;                /* the SMK-37's mod wheel: pitch only, gentle */
#endif
    OB_lfo_to = (OB_lfo * OB_g->OB_lfo_pd * 24) >> 15;
    OB_p1 = OB_p8 + (OB_pv[OB_P_LFO_PITCH_TO] != OB_TO_O2 ? OB_lfo_to : 0);
    OB_p2 = OB_p8 + (OB_pv[OB_P_LFO_PITCH_TO] != OB_TO_O1 ? OB_lfo_to : 0) + ((int32_t)OB_pv[OB_P_O2_SEMI] - 24) * 256 + OB_g->OB_det_fine8
       + ((OB_fenv * OB_g->OB_env_o2 * 97) >> 15);
    OB_v->OB_inc1 = OB_pitch_inc(OB_p1);
    OB_v->OB_inc2 = OB_pitch_inc(OB_p2);
    OB_tmp = (OB_lfo * OB_g->OB_lfo_pwd);
    OB_v->OB_pw1 = OB_pw_mod(OB_g->OB_pw1_base, OB_pv[OB_P_LFO_PWM_TO] != OB_TO_O2 ? OB_tmp : 0);
    OB_v->OB_pw2 = OB_pw_mod(OB_g->OB_pw2_base, OB_pv[OB_P_LFO_PWM_TO] != OB_TO_O1 ? OB_tmp : 0);
    /* cutoff, Q8 semitones */
    OB_cut = OB_g->OB_cut_sm + 8 * 256 + ((OB_fenv * OB_g->OB_fenv_amt * 390) >> 15)
        + ((int32_t)OB_v->OB_note - 60) * OB_g->OB_keytrk * 2 + (((int32_t)OB_v->OB_vel - 64) * OB_g->OB_velflt >> 1)
        + ((OB_lfo * OB_g->OB_lfo_fd * 97) >> 15) + ((OB_lfo * OB_g->OB_wheel_q * 48) >> 15) + ((OB_VINT_CUT8[OB_card] * OB_vint) >> 7);
    OB_svf_coef(&OB_v->OB_c1, OB_cut, OB_g->OB_reso_k);
    if (OB_g->OB_ftype == OB_F_LP24) OB_svf_coef(&OB_v->OB_c2, OB_cut, 5793);
    /* amp */
    OB_velg = 32767 - ((((127 - (int32_t)OB_v->OB_vel) * (int32_t)OB_pv[OB_P_VEL_AMP]) >> 7) * 258);
    OB_trem = 32767 - (((32767 - OB_lfo) * OB_g->OB_lfo_ad) >> 8);
    OB_v->OB_amp0 = OB_v->OB_amp1;
    OB_a = (int32_t)(((int64_t)(OB_v->OB_ae.OB_level >> 9) * OB_velg >> 15) * OB_trem >> 15);
    OB_v->OB_amp1 = (OB_a * OB_g->OB_gain_q12) >> 12;          /* the sound's VOLUME */
}

/* ---- the samples of one voice: oscillators -> mix -> drive -> filter -> amp -> pan */
static void OB_voice_render(OB_voice_t *OB_v, int32_t *OB_out,const OB_part_t *OB_g)
{
    uint32_t OB_i, OB_inc1 = OB_v->OB_inc1, OB_inc2 = OB_v->OB_inc2, OB_pw1 = OB_v->OB_pw1, OB_pw2 = OB_v->OB_pw2, OB_d1 = OB_inc1 >> 15, OB_d2 = OB_inc2 >> 15;
    int32_t OB_amp0 = OB_v->OB_amp0, OB_da = OB_v->OB_amp1 - OB_v->OB_amp0, OB_gl = OB_v->OB_gl, OB_gr = OB_v->OB_gr, OB_o2n = OB_v->OB_o2.OB_prev;
    int32_t OB_l1 = OB_g->OB_lvl1, OB_l2 = OB_g->OB_lvl2, OB_ln = OB_g->OB_lvln, OB_drv = OB_g->OB_drive_q12, OB_morph_q = OB_g->OB_morph_q;
    uint32_t OB_ft = OB_g->OB_ftype, OB_xq = OB_g->OB_xmod_q, OB_sy = OB_g->OB_sync_on, OB_ww1 = OB_g->OB_w1, OB_ww2 = OB_g->OB_w2;
    int32_t OB_hw1 = (OB_ww1 & OB_OSC_SAW ? -65536 : 0) + (OB_ww1 & OB_OSC_PLS ? -65536 : 0);
    int32_t OB_hw2 = (OB_ww2 & OB_OSC_SAW ? -65536 : 0) + (OB_ww2 & OB_OSC_PLS ? -65536 : 0);
    for (OB_i = 0; OB_i < OB_CTL; OB_i++) {
        int32_t OB_s1, OB_s2, OB_mix, OB_lp, OB_bp, OB_hp, OB_y, OB_amp, OB_rt = -1;
        uint32_t OB_ie = OB_inc1;
        if (OB_xq) {                               /* OSC2 -> OSC1 linear FM, the step never below inc/16 */
            int32_t OB_m = (int32_t)(OB_inc1 >> 8 > 0x200000u ? 0x200000u : OB_inc1 >> 8) * (((OB_o2n >> 8) * (int32_t)OB_xq) >> 6);
            OB_m += (int32_t)OB_inc1;
            OB_ie = OB_m < (int32_t)(OB_inc1 >> 4) ? OB_inc1 >> 4 : (uint32_t)OB_m;
            OB_d1 = OB_ie >> 15;
        }
        OB_s1 = OB_osc_step(&OB_v->OB_o1, OB_ie, OB_d1, OB_pw1, OB_ww1, OB_hw1, -1);
        if (OB_sy && OB_v->OB_o1.OB_ph < OB_ie && OB_d1)
            OB_rt = (int32_t)(OB_v->OB_o1.OB_ph / OB_d1);
        OB_s2 = OB_osc_step(&OB_v->OB_o2, OB_inc2, OB_d2, OB_pw2, OB_ww2, OB_hw2, OB_rt);
        OB_o2n = OB_v->OB_o2.OB_prev;
        OB_mix = OB_s1 * OB_l1 + OB_s2 * OB_l2;
        if (OB_ln) OB_mix += ((int32_t)(OB_xorshift(&OB_v->OB_noise) >> 16) - 32768) * OB_ln;
        OB_mix >>= 7;
        if (OB_drv) OB_mix = OB_softclip(((OB_mix >> 3) * OB_drv) >> 9);
        else OB_mix = OB_clampi(OB_mix, -65535, 65535);
        OB_svf_step(&OB_v->OB_c1, &OB_v->OB_f1, OB_mix, &OB_lp, &OB_bp, &OB_hp);
        switch (OB_ft) {
        case OB_F_LP12: OB_y = OB_lp; break;
        case OB_F_HP12: OB_y = OB_hp; break;
        case OB_F_BP12: OB_y = OB_bp; break;
        case OB_F_NOTCH: OB_y = OB_hp + OB_lp; break;
        case OB_F_SEM: OB_y = OB_lp + (((OB_hp - OB_lp) * OB_morph_q) >> 7); break;   /* the SEM's LP .. notch .. HP pot */
        default: OB_y = OB_svf_lp(&OB_v->OB_c2, &OB_v->OB_f2, OB_lp); break;
        }
        OB_y = OB_clampi(OB_y, -65000, 65000);
        OB_amp = OB_amp0 + ((OB_da * (int32_t)OB_i) >> 4);
        OB_y = (OB_y * OB_amp) >> 16;
        OB_out[2u * OB_i] += (OB_y * OB_gl) >> 12;
        OB_out[2u * OB_i + 1u] += (OB_y * OB_gr) >> 12;
    }
    (void)OB_o2n;
}

