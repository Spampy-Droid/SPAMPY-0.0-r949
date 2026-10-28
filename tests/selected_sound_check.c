#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
int main(int argc,char **argv) {
    assert(argc==2); panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();
    unsigned found=NBANK;for(unsigned i=0;i<NBANK;i++)if(!strcmp(argv[1],BANK[i].name))found=i;
    assert(found<NBANK && bank_pi[found]!=255);track_t *t=&trk[0];
    set_engine_of(t,BANK[found].e);apply_preset_to(t,bank_pi[found]);t->engine=t->eng_req;
    t->p[P_AMODE]=0;int32_t out[CTL*2];mix_block(out,CTL);
    trk_note_on(t,strcmp(argv[1],"GM KIT")?60:36,100);uint64_t hash=1469598103934665603ULL,energy=0;
    for(unsigned b=0;b<400;b++) {mix_block(out,CTL);for(unsigned i=0;i<CTL*2;i++) {uint32_t v=(uint32_t)out[i];hash^=v;hash*=1099511628211ULL;energy+=(uint64_t)(out[i]<0?-out[i]:out[i]);}}
    assert(energy>0);printf("%s %016llx %llu\n",argv[1],(unsigned long long)hash,(unsigned long long)energy);return 0;
}
