/* Host integration checks for the customized Spampy UI. */
#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
int main(int argc, char **argv)
{
    unsigned i;
    outdir = argc > 1 ? argv[1] : "build/host";
    panel = PANEL_DEFAULT; layers_init(); settings.palette = 4; palette_set(4); host_tracks_init();
    for (i=0;i<NPART;i++) {set_engine_of(&trk[i],TRK_DEF[i][0]);apply_preset_to(&trk[i],TRK_DEF[i][1]);trk[i].engine=trk[i].eng_req;}
    TDRUM->p[P_E0]=DRUM_DEFAULT_KIT;
    bank_resolve(); assert(NBANK == 107u);
    for(i=0;i<NBANK;i++) {
        assert(bank_pi[i]!=0xff);
        const preset_t *pr=preset_data(BANK[i].e,bank_pi[i]); assert(pr && str_eq(pr->name,BANK[i].name));
        set_engine_of(&trk[0],BANK[i].e); apply_preset_to(&trk[0],bank_pi[i]);
        assert(trk[0].preset==bank_pi[i]);
        for(unsigned j=0;j<8;j++) assert(trk[0].p[P_E0+j]==pr->e[j]);
    }
    sloop_splash();ppm("spampy-splash");
    go_home();ui.force=1;frame();ppm("spampy-tracks");
    for(i=0;i<NPAGES;i++) {ui.menu=0;ui.home=0;ui.page=i;ui.force=1;frame();char name[48];snprintf(name,sizeof name,"spampy-page-%02u",i);ppm(name);}
    ui.menu=2;ui.force=1;frame();ppm("spampy-about");
    ui.menu=1;ui.menu_sel=MI_CPU1;ui.force=1;frame();ppm("spampy-system");
    ui.menu_sel=MI_NEON;ui.force=1;frame();ppm("spampy-system-game");
    tap(B_OCTUP);assert(neon_game_active);assert(!neon_game_running);ppm("spampy-game-ready");
    tap(B_PLAY);assert(neon_game_running);frames(30);ppm("spampy-game-play");
    tap(B_PLAY);assert(neon_game_paused);ppm("spampy-game-paused");
    tap(B_PLAY);assert(!neon_game_paused);
    tap(B_HOME);assert(!neon_game_active);
    fm1_in.notes=0;frames(3);assert(!neon_game_is_active());
    go_home();press(B_HOME);press(B_PLAY);assert(neon_game_active);release(B_PLAY);release(B_HOME);
    tap(B_PLAY);assert(neon_game_running);frames(100);
    tap(B_HOME);assert(!neon_game_active);frames(3);
    ui.menu=1;ui.menu_sel=MI_RUSH;ui.force=1;frame();ppm("spampy-system-rush");
    tap(B_OCTUP);assert(rush_game_active);assert(!rush_running);ppm("spampy-rush-ready");
    tap(B_PLAY);assert(rush_running);frames(40);ppm("spampy-rush-play");
    tap(B_PLAY);assert(rush_paused);tap(B_PLAY);assert(!rush_paused);tap(B_HOME);assert(!rush_game_active);frames(3);
    /* All rotations retain four distinct cells and fit after hard drop. */
    blocks_start();blocks_run=1;
    blocks_shape=0;blocks_next=1;blocks_rot=0;blocks_x=4;blocks_y=0;
    blocks_input(0,1u<<1);assert(blocks_shape==1u&&blocks_next==0u&&blocks_rot==0u);
    for(unsigned sh=0;sh<3;sh++)for(unsigned rot=0;rot<4;rot++){
        blocks_shape=sh;blocks_rot=rot;blocks_x=4;blocks_y=0;
        for(unsigned a=0;a<4;a++)for(unsigned b=a+1;b<4;b++)
            assert(blocks_ox(a)!=blocks_ox(b)||blocks_oy(a)!=blocks_oy(b));
        while(!blocks_hit(blocks_x,blocks_y+1))blocks_y++;
        assert(!blocks_hit(blocks_x,blocks_y));assert(blocks_hit(blocks_x,blocks_y+1));
    }
    blocks_score=0;assert(blocks_interval()==900);
    blocks_score=500;assert(blocks_interval()<900);
    blocks_score=10000;assert(blocks_interval()==220);
    blocks_score=0;blocks_pause=1;ui.force=1;frame();ppm("neon-blocks-r994");
    blocks_active=0;
    go_home();fm1_in.notes=0;press(B_HOME);press(B_ARP);
    assert(bu_active);release(B_ARP);release(B_HOME);tap(B_PLAY);assert(bu_run);
    for(unsigned k=0;k<BU_N;k++)bu_cells[k]=0;
    /* The next bubble must use a colour still on the board and avoid the last one. */
    bu_cells[0]=1;bu_cells[1]=2;bu_cells[2]=3;bu_rng=12345;
    for(unsigned k=0;k<32;k++)assert(bu_pick(1)!=1);
    for(unsigned k=0;k<BU_N;k++)bu_cells[k]=0;
    bu_cells[0]=1;assert(bu_pick(1)==1);
    for(unsigned k=0;k<BU_N;k++)bu_cells[k]=0;
    bu_cells[0]=bu_cells[1]=bu_cells[2]=1;bu_cells[7]=2;
    bu_resolve(1);assert(!bu_cells[0]&&!bu_cells[1]&&!bu_cells[2]);assert(bu_score==30);
    for(unsigned k=0;k<BU_N;k++)bu_cells[k]=0;
    bu_cells[0]=bu_cells[1]=bu_cells[8]=1;bu_cells[16]=2;bu_cells[7]=3;
    bu_resolve(8);assert(!bu_cells[16]);assert(bu_cells[7]==3);
    bu_reset();bu_run=1;tap(B_REC);assert(bu_flying);
    for(unsigned k=0;k<100;k++){fm1_ms+=20;bu_tick();}
    assert(!bu_flying);
    bu_flying=0;bu_idle_at=fm1_ms;fm1_ms+=8000;bu_input(0,0);assert(bu_flying);
    bu_flying=0;
    ui.force=1;frame();ppm("neon-bubbles-r995");
    tap(B_PLAY);assert(bu_pause);tap(B_PLAY);assert(!bu_pause);
    tap(B_HOME);assert(!bu_active);
    printf("SPAMPY UI: %u pages rendered; preset bank resolves; both arcade menus, play, pause, resume, exit PASS\n",NPAGES);
    return 0;
}
