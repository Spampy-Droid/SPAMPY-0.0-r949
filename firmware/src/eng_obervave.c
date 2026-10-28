/* SPDX-License-Identifier: GPL-3.0-only
 * Native OberVave integration. Synth Copyright (C) 2026 zvenson.
 * Spampy uses its own allocator, arpeggiator, mix and effects. */
#include "obv_native_core.h"
#include "obv_selected.h"
typedef struct {OB_voice_t voice[NVOICE];OB_part_t state,sub[2];OB_patch_t patch;uint8_t ready,pick,seeded;} obv_part_t;
static obv_part_t *obv_part(uint32_t p);
static OB_voice_t *obv_voice(track_t*t,voice_t*v){return &obv_part((uint32_t)(t-trk)%NPART)->voice[(uint32_t)(v-t->v)%NVOICE];}
static obv_part_t *obv_prepare(track_t*t){obv_part_t*s=obv_part((uint32_t)(t-trk)%NPART);unsigned pick=(uint32_t)t->p[P_E7]%OB_FACTORY_N;
 if(!s->seeded){for(unsigned i=0;i<NVOICE;i++){OB_voice_t*v=&s->voice[i];v->OB_noise=0x12345678u*(i+1u)+0x9E3779B9u;v->OB_o1.OB_ph=OB_xorshift(&v->OB_noise);v->OB_o2.OB_ph=OB_xorshift(&v->OB_noise);v->OB_gl=v->OB_gr=2896;}s->state.OB_rng=0x9E3779B9u;s->state.OB_lfo_rnd_a=OB_xorshift(&s->state.OB_rng);s->state.OB_lfo_rnd_b=OB_xorshift(&s->state.OB_rng);s->seeded=1;}
 if(!s->ready || (s->pick!=pick&&!t->xf)){s->patch=OB_SELECTED[OB_SLOT_MAP[pick]];s->pick=pick;s->ready=1;s->state.OB_cut_sm=s->patch.OB_v[OB_P_CUTOFF]<<8;s->state.OB_reso_sm=s->patch.OB_v[OB_P_RESO]<<8;s->state.OB_last_pitch16=60<<16;}
 s->patch=OB_SELECTED[OB_SLOT_MAP[s->pick]];
 static const uint8_t controls[]={OB_P_CUTOFF,OB_P_RESO,OB_P_FENV_AMT,OB_P_AA,OB_P_AR,OB_P_LFO_PWM,OB_P_VINTAGE};
 for(unsigned i=0;i<7;i++){int value=s->patch.OB_v[controls[i]]+t->p[P_E0+i];s->patch.OB_v[controls[i]]=(uint8_t)clamp(value,0,127);}
 /* Factory patches keep their analogue character without random pitch drift.
  * EDIT > VINT adds it back deliberately (0..63) when wanted. */
 s->patch.OB_v[OB_P_VINTAGE]=(uint8_t)clamp(t->p[P_E6],0,127);
 s->state.OB_p=&s->patch;return s;}
static uint32_t obv_cap(const track_t*t){const uint8_t*p=OB_SELECTED[OB_SLOT_MAP[(uint32_t)t->p[P_E7]%OB_FACTORY_N]].OB_v;return t->p[P_VOICE]==V_UNISON?(p[OB_P_MODE]==OB_M_DUAL?2u:p[OB_P_MODE]==OB_M_UNISON?(uint32_t)clamp(p[OB_P_UNI_N],1,NVOICE):NVOICE):NVOICE;}
static void obv_note_on(track_t*t,voice_t*v){obv_part_t*s=obv_prepare(t);OB_voice_t*q=obv_voice(t,v);const uint8_t*p=s->patch.OB_v;unsigned card=(uint32_t)(v-t->v)%NVOICE,n=obv_cap(t);int det=(p[OB_P_UNI_DET]*p[OB_P_UNI_DET])>>6,pan=(OB_PAN_CARD[card]*p[OB_P_SPREAD])>>7;
 q->OB_note=v->note;q->OB_vel=v->vel;q->OB_gate=1;q->OB_det8=0;
 if(t->p[P_VOICE]==V_UNISON){q->OB_det8=p[OB_P_MODE]==OB_M_DUAL?(card&1?det/2:-det/2):n>1?(det*(int)(2u*card)/(int)(n-1u))-det:0;pan=n>1?((int)(128u*card)/(int)(n-1u)-64)*p[OB_P_SPREAD]>>7:0;}
 q->OB_target16=(int32_t)v->note<<16;q->OB_pitch16=p[OB_P_GLIDE]?s->state.OB_last_pitch16:q->OB_target16;s->state.OB_last_pitch16=q->OB_target16;q->OB_fe.OB_stage=q->OB_ae.OB_stage=OB_E_ATT;q->OB_lfo_blk=0;q->OB_lfo_fade=p[OB_P_LFO_DELAY]?0:32767;OB_voice_pan(q,pan);
 if(p[OB_P_LFO_PHASE]==OB_LP_SYNC){unsigned active=0;for(unsigned i=0;i<NVOICE;i++)active+=t->v[i].active&&t->v[i].gate&&i!=card;if(!active)s->state.OB_lfo_ph=0;}}
static void obv_block(track_t*t){obv_part_t*s=obv_prepare(t);for(unsigned b=0;b<2;b++){OB_lfo_tick(&s->state);OB_block_globals(&s->state);s->sub[b]=s->state;}}
static int obv_done(track_t*t,voice_t*v){OB_voice_t*q=obv_voice(t,v);return !v->gate&&q->OB_ae.OB_stage==OB_E_IDLE;}
static void obv_render(track_t*t,voice_t*v,int32_t*out,uint32_t n,const vmod_t*m){obv_part_t*s=obv_part((uint32_t)(t-trk)%NPART);OB_voice_t*q=obv_voice(t,v);unsigned card=(uint32_t)(v-t->v)%NVOICE;q->OB_gate=v->gate;q->OB_note=v->note;q->OB_target16=m->pitch16<<12;
 for(unsigned b=0;b<n;b+=OB_CTL){int32_t stereo[2*OB_CTL]={0};OB_part_t*g=&s->sub[b/OB_CTL%2];OB_voice_block(q,card,g);OB_voice_render(q,stereo,g);for(unsigned i=0;i<OB_CTL&&b+i<n;i++){int32_t y=(stereo[2*i]+stereo[2*i+1])/2;int32_t amp=m->amp0+(int32_t)(((int64_t)(m->amp1-m->amp0)*(b+i))/CTL);out[b+i]+=mulq15(y,amp)*VOICE_FS/32768;}}}
static const char*const OB_NAMES[]={"UNISON LD","UNISON LD","PWM STRING","SWEEP PAD","UNISON LD","UNISON LD","UNISON LD","UNISON LD","NEON PAD","UNISON LD","UNISON LD","DREAM ARP","BIG BRASS","HORN SECT","WARM PAD","DARK PAD","AIR CHOIR","SLOW SWEEP","GHOST PAD","OB ENSEMBLE","TAPE KEYS","PWM LEAD"};
static const preset_t OB_PRESETS[]={
{"UNISON LD",{0,0,0,0,0,0,0,1},{0,0,127,0},0,0,FX(0,0,30,25),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,3,P_DETUNE+1,0)},
{"PWM STRING",{0,0,0,0,0,0,0,2},{0,0,127,0},0,0,FX(0,55,0,35),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"SWEEP PAD",{0,0,0,0,0,0,0,3},{0,0,127,0},0,0,FX(0,40,0,45),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"NEON PAD",{0,0,0,0,0,0,0,8},{0,0,127,0},0,0,FX(0,70,25,70),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"DREAM ARP",{0,0,0,0,0,0,0,11},{0,0,127,0},0,0,FX(0,55,50,60),ARP(5,1,2,86),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"BIG BRASS",{0,0,0,0,0,0,0,12},{0,0,127,0},0,0,FX(0,40,0,35),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"HORN SECT",{0,0,0,0,0,0,0,13},{0,0,127,0},0,0,FX(0,0,0,25),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"WARM PAD",{0,0,0,0,0,0,0,14},{0,0,127,0},0,0,FX(0,60,0,50),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"DARK PAD",{0,0,0,0,0,0,0,15},{0,0,127,0},0,0,FX(0,0,25,60),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"AIR CHOIR",{0,0,0,0,0,0,0,16},{0,0,127,0},0,0,FX(0,75,0,55),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"SLOW SWEEP",{0,0,0,0,0,0,0,17},{0,0,127,0},0,0,FX(0,35,0,50),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"GHOST PAD",{0,0,0,0,0,0,0,18},{0,0,127,0},0,0,FX(0,0,30,70),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"OB ENSEMBLE",{0,0,0,0,0,0,0,19},{0,0,127,0},0,0,FX(0,80,0,40),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"TAPE KEYS",{0,0,0,0,0,0,0,20},{0,0,127,0},0,0,FX(0,40,35,30),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,0,P_DETUNE+1,0)},
{"PWM LEAD",{0,0,0,0,0,0,0,21},{0,0,127,0},0,0,FX(0,0,30,20),ARP(0,2,1,65),XP(P_ALLOC+1,1,P_VOICE+1,2,P_DETUNE+1,0)},
};
static const engine_t ENG_OBERVAVE={.name="OBERVAVE",.page_title={"OBERVAVE","PATCH"},.edit={{"CUT",F_INT,-63,63,0,0,0},{"RES",F_INT,-63,63,0,0,0},{"FENV",F_INT,-63,63,0,0,0},{"ATK",F_INT,-63,63,0,0,0},{"REL",F_INT,-63,63,0,0,0},{"PWM",F_INT,-63,63,0,0,0},{"VINT",F_INT,-63,63,0,0,0},{"PATCH",F_ENUM,0,21,0,OB_NAMES,0}},.presets=OB_PRESETS,.npresets=15,.poly=NVOICE,.ownenv=1,.note_on=obv_note_on,.render=obv_render,.block=obv_block,.done=obv_done,.macro={P_E0,P_E1,P_E3,P_E7}};
