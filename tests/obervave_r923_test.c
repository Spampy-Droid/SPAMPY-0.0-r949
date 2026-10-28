/* Check new OberVave native programs on the Spampy scheduler. */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
static void render(unsigned n){int32_t o[CTL*2];while(n--){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++)assert(o[i]>=-32768&&o[i]<=32767);}}
static unsigned active(void){unsigned n=0;for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)n+=trk[p].v[i].active;return n;}
static void release_wait(void){for(unsigned b=0;b<FS*100/CTL;b++){render(1);if(!active())return;}assert(!"voices did not release");}
int main(void){setvbuf(stdout,0,_IOLBF,0);panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();assert(NBANK==115);
for(unsigned k=11;k<22;k++){
 track_t*t=&trk[0];t->p[P_AMODE]=0;set_engine_of(t,ENGI_OBERVAVE);apply_preset_to(t,k);render(20);assert(t->engine==ENGI_OBERVAVE);obv_part_t*s=obv_part(0);const uint8_t*raw=OB_SELECTED[OB_SLOT_MAP[k]].OB_v;assert(!memcmp(s->patch.OB_v,raw,80));
 assert(t->p[P_AMODE]==raw[OB_P_ARP_MODE]);assert(t->p[P_AOCT]==raw[OB_P_ARP_OCT]);assert(t->p[P_AGATE]==clamp(raw[OB_P_ARP_GATE]+1,1,127));
 trk_note_on(t,60,100);render(FS/CTL);trk_note_off(t,60);release_wait();printf("PASS new OberVave %s: original patch, native defaults, audio bounds, note release\n",OB_NAMES[k]);
}
for(unsigned p=0;p<NPART;p++){trk[p].p[P_AMODE]=0;set_engine_of(&trk[p],ENGI_OBERVAVE);apply_preset_to(&trk[p],11+p);}render(20);
for(unsigned j=0;j<24;j++){trk_note_on(&trk[j%NPART],48+j,100);render(4);}
for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);release_wait();
for(unsigned j=0;j<33;j++){track_t*t=&trk[j%NPART];t->p[P_AMODE]=0;set_engine_of(t,j%2?ENGI_PROPHET:ENGI_OBERVAVE);apply_preset_to(t,j%2?3:11+j%11);render(20);trk_note_on(t,60,100);render(3);trk_note_off(t,60);}
for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);release_wait();puts("PASS mixed parts and Prophet/OberVave switching");return 0;}
