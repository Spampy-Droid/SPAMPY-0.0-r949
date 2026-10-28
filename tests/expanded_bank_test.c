/* Native bank regression: all presets release, rapid switching, mixed native engines. */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
static void render(unsigned b){int32_t o[CTL*2];while(b--){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++)assert(o[i]>=-32768&&o[i]<=32767);}}
static void release_wait(void){for(unsigned b=0;b<100*FS/CTL;b++){render(1);unsigned active=0;for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)active+=trk[p].v[i].active;if(!active)return;}assert(0);}
int main(void){setvbuf(stdout,0,_IOLBF,0);panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();assert(NBANK==105);
for(unsigned k=63;k<NBANK;k++){
 track_t*t=&trk[0];set_engine_of(t,BANK[k].e);apply_preset_to(t,bank_pi[k]);render(20);assert(t->engine==BANK[k].e);
 trk_note_on(t,60,100);render(FS/CTL);trk_note_off(t,60);release_wait();
 for(unsigned i=0;i<NVOICE;i++)assert(!t->v[i].active);
 printf("PASS release %s\n",BANK[k].name);
}
for(unsigned j=0;j<220;j++){track_t*t=&trk[j%NPART];unsigned k=63+j%42;set_engine_of(t,BANK[k].e);apply_preset_to(t,bank_pi[k]);render(12);trk_note_on(t,48+j%25,100);render(4);trk_note_off(t,48+j%25);}
for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);release_wait();
for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)assert(!trk[p].v[i].active);
puts("PASS rapid preset/engine switches and mixed native parts");return 0;}
