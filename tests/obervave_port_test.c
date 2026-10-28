/* Integration checks for native OberVave voices on Spampy's scheduler. */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
static void render(unsigned n){int32_t o[CTL*2];while(n--){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++)assert(o[i]>=-32768&&o[i]<=32767);}}
static unsigned active(void){unsigned n=0;for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)n+=trk[p].v[i].active;return n;}
static void release_wait(void){for(unsigned b=0;b<FS*100/CTL;b++){render(1);if(!active())return;}assert(!"voices did not release");}
int main(void){setvbuf(stdout,0,_IOLBF,0);panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();assert(NBANK==104);
for(unsigned k=0;k<OB_FACTORY_N;k++){if(k!=1&&k!=2&&k!=3&&k!=8)continue;
 track_t*t=&trk[0];t->p[P_AMODE]=0;set_engine_of(t,ENGI_OBERVAVE);apply_preset_to(t,k);render(20);assert(t->engine==ENGI_OBERVAVE);obv_part_t*s=obv_part(0);assert(!memcmp(s->patch.OB_v,OB_SELECTED[OB_SLOT_MAP[k]].OB_v,80));
 assert(t->p[P_AMODE]==OB_SELECTED[OB_SLOT_MAP[k]].OB_v[OB_P_ARP_MODE]);assert(t->p[P_AOCT]==OB_SELECTED[OB_SLOT_MAP[k]].OB_v[OB_P_ARP_OCT]);
 assert(t->p[P_AGATE]==clamp(OB_SELECTED[OB_SLOT_MAP[k]].OB_v[OB_P_ARP_GATE]+1,1,127));
 trk_note_on(t,60,100);render(20);if(k==1)assert(trk_nvoice(t)==6);if(k==5)assert(trk_nvoice(t)==8);if(k==6)assert(trk_nvoice(t)==2);
 render(FS/CTL);trk_note_off(t,60);release_wait();printf("PASS OberVave %s: original patch, allocation, bounded audio, release\n",OB_NAMES[k]);
}
for(unsigned p=0;p<NPART;p++){trk[p].p[P_AMODE]=0;set_engine_of(&trk[p],ENGI_OBERVAVE);apply_preset_to(&trk[p],p==0?1:p==1?2:8);}render(20);
for(unsigned j=0;j<24;j++){trk_note_on(&trk[j%NPART],48+j,100);render(4);}
for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);release_wait();
for(unsigned j=0;j<40;j++){track_t*t=&trk[j%NPART];t->p[P_AMODE]=0;set_engine_of(t,j%2?ENGI_PROPHET:ENGI_OBERVAVE);apply_preset_to(t,j%2?3:j%11);render(20);trk_note_on(t,60,100);render(3);trk_note_off(t,60);}
for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);release_wait();puts("PASS mixed native parts, shared budget and Prophet/OberVave engine switches");return 0;}
