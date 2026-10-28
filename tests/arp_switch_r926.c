#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
int main(void){panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();assert(NBANK==107);track_t*t=&trk[0];int32_t out[CTL*2];unsigned on=0,off=0;
for(unsigned k=0;k<NBANK;k++){
 set_engine_of(t,BANK[k].e);t->p[P_AMODE]=0;t->p[P_AHOLD]=0;apply_preset_to(t,bank_pi[k]);int16_t expected[8];memcpy(expected,t->p+P_AMODE,sizeof expected);if(expected[0])on++;else off++;
 /* A running, edited arp with a held chord must not override this sound. */
 for(unsigned j=0;j<8;j++)t->p[P_AMODE+j]=TP[P_AMODE+j].max;
 t->p[P_AMODE]=1;t->p[P_AHOLD]=1;t->nheld=2;t->held[0]=60;t->held[1]=64;t->arp_phys=2;t->arp_note=61;arp_keep_req|=1;
 apply_preset_to(t,bank_pi[k]);assert(!memcmp(expected,t->p+P_AMODE,sizeof expected));assert(!(arp_keep_req&1));
 mix_block(out,CTL);assert(t->nheld==0&&t->arp_phys==0);
}
assert(on&&off);
/* Rapid scrolling can load multiple sounds before the audio ISR runs. */
for(unsigned k=0;k<NBANK;k++){set_engine_of(t,BANK[k].e);t->p[P_AMODE]=1;t->p[P_AHOLD]=1;apply_preset_to(t,bank_pi[k]);const preset_t*pr=preset_data(BANK[k].e,bank_pi[k]);assert(t->p[P_AMODE]==(pr->arp[0]?pr->arp[0]-1:TP[P_AMODE].def));}
/* Existing keyboard HOLD, without arp, remains a separate track setting. */
for(unsigned k=0;k<NBANK;k++){const preset_t*pr=preset_data(BANK[k].e,bank_pi[k]);if(pr->arp[0]>1)continue;set_engine_of(t,BANK[k].e);t->p[P_AMODE]=0;t->p[P_AHOLD]=1;apply_preset_to(t,bank_pi[k]);assert(t->p[P_AHOLD]==1);break;}
printf("PASS all %u bank entries: own arp settings, held-note release, rapid scroll; %u arp and %u non-arp programs; keyboard HOLD retained\n",NBANK,on,off);return 0;}
