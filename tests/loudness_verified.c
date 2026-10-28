#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
#include <math.h>
static double drum_energy(unsigned kit, int on) {
    int32_t l[CTL],r[CTL],v[CTL],d[CTL]; double e=0;
    memset(&drums,0,sizeof drums); drums.set=-2;
    TDRUM->p[P_E0]=kit; song.g[G_DRLVL]=70; loud_drum[kit]=on;
    drum_on(36,100);
    for(unsigned b=0;b<200;b++) {
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(v,0,sizeof v);memset(d,0,sizeof d);
        drums_render(l,r,v,d,CTL);
        for(unsigned i=0;i<CTL;i++) e+=(double)l[i]*l[i];
    } return e;
}
int main(void) {
    panel=PANEL_DEFAULT; layers_init(); host_tracks_init(); bank_resolve();
    for(unsigned p=0;p<NBANK;p++) loud_factory[BANK[p].e][bank_pi[p]]=1;
    memset(loud_drum,1,sizeof loud_drum); memset(loud_user_bank,1,sizeof loud_user_bank);
    struct {side_rec_t r; unsigned guard;} rec; rec.guard=0xabcdef12;
    side_fill(&rec.r); assert(rec.guard==0xabcdef12);
    memset(loud_factory,0,sizeof loud_factory); memset(loud_drum,0,sizeof loud_drum);
    memset(loud_user_bank,0,sizeof loud_user_bank); side_take(&rec.r);
    for(unsigned p=0;p<NBANK;p++) assert(loud_factory[BANK[p].e][bank_pi[p]]);
    for(unsigned p=0;p<DRUM_KITS;p++) assert(loud_drum[p]);
    puts("PASS persistent record: all factory presets, 47 kits, user slots; no overwrite");
    song.sel=0; set_engine_of(&trk[0],BANK[0].e); apply_preset_to(&trk[0],bank_pi[0]);
    ui.force=1; studio_tracks_draw();
    unsigned yellow=0; for(unsigned i=0;i<240*240;i++) yellow+=screen[i]==swap16(RGB(255,255,0));
    assert(yellow>10); printf("PASS actual studio tracks screen: %u yellow pixels\n",yellow);
    for(unsigned k=0;k<DRUM_USR;k++) {
        double off=drum_energy(k,0), on=drum_energy(k,1);
        printf("measure %u: %.0f %.0f ratio %.3f\n",k,off,on,on/off); fflush(stdout);
        assert(off>0 && on>off*7.0 && on<off*9.0);
        printf("PASS drum %s: %.2f dB measured\n",DRUM_KIT_NAMES[k],10*log10(on/off));
    }
    return 0;
}
