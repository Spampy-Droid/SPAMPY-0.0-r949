#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
typedef struct{uint8_t engine;int16_t p[P_COUNT];} up_rec_t;
static up_rec_t fixture;
static const up_rec_t*up_rec(uint32_t k){(void)k;return &fixture;}
static void up_values(const up_rec_t*r,int16_t*v){memcpy(v,r->p,sizeof r->p);}
UI_SMALL static int up_load_actual(track_t *t, uint32_t k)       /* user preset k into track t (2.6: any track, MIDI PC) */
{
    const up_rec_t *r;
    int16_t v[P_COUNT];
    uint32_t i;
    if (!up_used(k) || is_drum(t))
        return 1;
    r = up_rec(k);
    up_values(r, v);
    for (i = 0; i < P_COUNT; i++)                       /* (LEN etc. of a kept pattern changed too, and */
        if (param_kept(i) ||   /* a preset pattern then counted as */
            (i == P_AHOLD && !t->p[P_AMODE] && t->p[P_AHOLD]))   /* (2.7: KBD HOLD stays on) */
            v[i] = t->p[i];                             /* edited; the loaded record supplies its own arp) */
    if (!v[P_AMODE] && !t->p[P_AMODE])
        v[P_AHOLD] = t->p[P_AHOLD];                     /* (2.7) KBD HOLD (HOLD with the arp OFF) is the track's, not the
                                                         * preset's: as it was (a 2.6 preset's HOLD meant nothing there) */
    sound_change(t);                                    /* (2.6: what sounds fades out with the old sound, no click) */
    arp_keep_req &= (uint8_t)~(1u << trk_index(t)); /* do not inherit the previous sound's arp latch */
    panic_req |= (uint8_t)(1u << trk_index(t));
    fm1_irq_off();                                      /* the audio ISR must not see half a sound */
    t->eng_req = r->engine;
    for (i = 0; i < P_COUNT; i++)
        t->p[i] = v[i];
    t->preset = 0;
    fm1_irq_on();
    fm6_track_loaded(t);                                /* FM6: a user preset holds the PTCH and the macros */
    t->user = (uint8_t)(k + 1u);
    if (t == TSEL)
        sync_reload = 1;
    ui.force = 1;
    return 0;
}

int main(void){panel=PANEL_DEFAULT;layers_init();host_tracks_init();track_t*t=&trk[0];int32_t out[CTL*2];
for(unsigned mode=0;mode<3;mode++){
 set_engine_of(t,0);apply_preset_to(t,0);fixture.engine=0;memcpy(fixture.p,t->p,sizeof fixture.p);
 for(unsigned i=0;i<8;i++)fixture.p[P_AMODE+i]=TP[P_AMODE+i].def;
 fixture.p[P_AMODE]=mode;fixture.p[P_ARATE]=5;fixture.p[P_AHOLD]=mode?1:0;
 t->p[P_AMODE]=1;t->p[P_AHOLD]=1;t->nheld=2;t->arp_phys=2;arp_keep_req|=1;t->p[P_LEVEL]=83;
 assert(!up_load_actual(t,0));assert(!memcmp(t->p+P_AMODE,fixture.p+P_AMODE,8*sizeof(int16_t)));assert(t->p[P_LEVEL]==83);assert(!(arp_keep_req&1));
 mix_block(out,CTL);assert(!t->nheld&&!t->arp_phys);
}
puts("PASS USER preset arp on/off/changed mode, mix retained and old held notes released");return 0;}
