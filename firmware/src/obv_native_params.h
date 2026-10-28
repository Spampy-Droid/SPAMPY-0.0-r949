enum {
    OB_P_O1_WAVE, OB_P_O1_PW, OB_P_O1_LVL, OB_P_NOISE,
    OB_P_O2_WAVE, OB_P_O2_PW, OB_P_O2_LVL, OB_P_O2_SEMI,
    OB_P_SYNC, OB_P_XMOD, OB_P_ENV_O2, OB_P_O2_FINE,
    OB_P_CUTOFF, OB_P_RESO, OB_P_FENV_AMT, OB_P_KEYTRK, OB_P_FTYPE,
    OB_P_DRIVE, OB_P_VEL_FLT, OB_P_VEL_AMP,
    OB_P_FA, OB_P_FD, OB_P_FS, OB_P_FR,
    OB_P_AA, OB_P_AD, OB_P_AS, OB_P_AR,
    OB_P_LFO_SHAPE, OB_P_LFO_RATE, OB_P_LFO_DELAY, OB_P_LFO_PHASE,
    OB_P_LFO_PITCH, OB_P_LFO_PWM, OB_P_LFO_FLT, OB_P_LFO_AMP, OB_P_LFO_PITCH_TO, OB_P_LFO_PWM_TO,
    OB_P_MODE, OB_P_UNI_N, OB_P_UNI_DET, OB_P_GLIDE,
    OB_P_VINTAGE, OB_P_SPREAD, OB_P_BEND, OB_P_VOLUME,
    OB_P_MOD_DEPTH,
    OB_P_ARP_MODE, OB_P_ARP_DIV, OB_P_ARP_OCT, OB_P_ARP_GATE,
    OB_P_FMORPH,                                   /* (appended: the MIDI CC numbers of the above stay) */
    OB_P_CHO_RATE, OB_P_CHO_DEPTH, OB_P_CHO_MIX,
    OB_P_DLY_TIME, OB_P_DLY_FB, OB_P_DLY_TONE, OB_P_DLY_MIX,
    OB_P_REV_SIZE, OB_P_REV_DAMP, OB_P_REV_PRE, OB_P_REV_MIX,
    OB_NPARAM
};
#define OB_PATCH_NAME 10u
#define OB_PATCH_BYTES 80u                         /* NPARAM values, the name at NPARAM, the rest spare */
#if OB_NPARAM + OB_PATCH_NAME > OB_PATCH_BYTES
#error "the patch outgrew PATCH_BYTES"
#endif
#define OB_NPROG 99u                               /* 1-64 factory sounds, 65-99 INIT; the user's saves over them */
#define OB_NFACT 64u

enum { OB_W_OFF, OB_W_SAW, OB_W_PLS, OB_W_BOTH };
enum { OB_F_LP12, OB_F_HP12, OB_F_BP12, OB_F_NOTCH, OB_F_LP24, OB_F_SEM, OB_NFTYPE };   /* SEM: LP .. notch .. HP by MORPH */
enum { OB_M_POLY, OB_M_DUAL, OB_M_UNISON, OB_M_MONO };
enum { OB_L_TRI, OB_L_SAW, OB_L_RAMP, OB_L_SQR, OB_L_SH, OB_L_RND };
enum { OB_LP_SYNC, OB_LP_FREE, OB_LP_SPREAD };
enum { OB_TO_O1, OB_TO_O2, OB_TO_BOTH };
enum { OB_A_OFF, OB_A_UP, OB_A_DOWN, OB_A_UPDN, OB_A_RND, OB_A_PLAY, OB_NARPMODE };
enum { OB_D_4, OB_D_8, OB_D_8T, OB_D_16, OB_D_16T, OB_D_32, OB_D_32T, OB_NDIV };
enum { OB_DD_16, OB_DD_8T, OB_DD_8, OB_DD_8D, OB_DD_4, OB_DD_4D, OB_DD_2, OB_NDDIV };   /* the delay's note values */

typedef struct {uint8_t OB_v[OB_PATCH_BYTES];} OB_patch_t;
