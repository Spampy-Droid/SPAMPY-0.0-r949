#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
#define PROJ_HOST 1
#include "../firmware/src/project.c"
int main(void){panel=PANEL_DEFAULT;layers_init();host_tracks_init();bank_resolve();project_t p;for(unsigned k=0;k<NBANK;k++){track_t*t=&trk[0];set_engine_of(t,BANK[k].e);apply_preset_to(t,bank_pi[k]);int16_t ep[8];memcpy(ep,t->p+P_E0,sizeof ep);proj_capture(&p);assert(proj_ok(&p));t->p[P_E7]=0;t->preset=0;proj_apply(&p,1);assert(t->eng_req==BANK[k].e);assert(t->preset==bank_pi[k]);assert(!memcmp(ep,t->p+P_E0,sizeof ep));}puts("PASS all 107 retained and added factory presets persist across project capture/restore with stable logical IDs");return 0;}
