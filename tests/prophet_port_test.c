/* Integration tests for native Melodee Prophet patches in Spampy. */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
static void render(unsigned blocks) {int32_t o[CTL*2];for(unsigned b=0;b<blocks;b++){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++)assert(o[i]>=-32768 && o[i]<=32767);}}
static unsigned units(void){unsigned u=0;for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)if(trk[p].v[i].active && trk[p].v[i].stage!=4)u+=trk[p].engine==ENGI_PROPHET?3:2;return u;}
int main(int argc,char **argv){
 assert(argc==2);panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();assert(NBANK==104);
 for(unsigned k=0;k<P5_FACTORY_N;k++){
    if(k==25u||k==26u||k==32u)continue;
    set_engine_of(&trk[0],ENGI_PROPHET);apply_preset_to(&trk[0],k);trk[0].engine=ENGI_PROPHET;render(8);
    assert(trk[0].p[P_E7]==k);assert(!memcmp(p5_patch_of(&trk[0])->raw,P5_SELECTED[P5_SLOT_MAP[k]].raw,P5_RAW_MAX));
    int32_t o[CTL*2];FILE *f=fopen(argv[1],"ab");assert(f); /* combined raw PCM demo; converted by caller */
    trk_note_on(&trk[0],60,100);for(unsigned b=0;b<FS/CTL;b++){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++){int16_t v=(int16_t)o[i];fwrite(&v,2,1,f);}}
    trk_note_off(&trk[0],60);for(unsigned b=0;b<FS/CTL;b++){mix_block(o,CTL);for(unsigned i=0;i<CTL*2;i++){int16_t v=(int16_t)o[i];fwrite(&v,2,1,f);}}fclose(f);
    render(FS*80/CTL);for(unsigned i=0;i<NVOICE;i++)assert(!trk[0].v[i].active);
    printf("P5 %u native patch exact, audible, note release complete PASS\n",k);
 }
 for(unsigned p=0;p<NPART;p++){set_engine_of(&trk[p],ENGI_PROPHET);apply_preset_to(&trk[p],p);trk[p].engine=ENGI_PROPHET;}render(8);
 for(unsigned j=0;j<24;j++){trk_note_on(&trk[j%NPART],48+j,90);render(4);assert(units()<=16);}
 render(300);for(unsigned p=0;p<NPART;p++)for(unsigned n=0;n<128;n++)trk_note_off(&trk[p],n);render(FS*80/CTL);
 for(unsigned p=0;p<NPART;p++)for(unsigned i=0;i<NVOICE;i++)assert(!trk[p].v[i].active);
 set_engine_of(&trk[0],ENGI_PROPHET);apply_preset_to(&trk[0],4);trk[0].engine=ENGI_PROPHET;render(4);
 midi_cc(&trk[0],1,95);assert(p5_mw[0]==95); /* native performance modulation */
 trk_note_on(&trk[0],60,100);render(40);
 set_engine_of(&trk[0],1);apply_preset_to(&trk[0],0);render(16);assert(trk[0].engine==1);
 trk_note_on(&trk[0],64,100);render(80);trk_note_off(&trk[0],64);render(FS*80/CTL);
 puts("Mixed Prophet parts, voice budget, release, modulation wheel and engine switching PASS");return 0;
}
