#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
#include <assert.h>

int main(void)
{
    uint32_t q, k;
    track_t *t = &trk[0];
    panel = PANEL_DEFAULT;
    layers_init();
    host_tracks_init();
    bank_resolve();
    for (q = 0; q < NBANK; q++) {
        if (BANK[q].e == 4u) continue; /* GM KIT is the drum entry, checked below */
        set_engine_of(t, BANK[q].e);
        apply_preset_to(t, bank_pi[q]);
        loudness_toggle(t);
        assert(ui_loud_active(t));
        loudness_toggle(t);
        assert(!ui_loud_active(t));
    }
    for (k = 0; k < DRUM_KITS; k++) {
        TDRUM->p[P_E0] = (int16_t)k;
        loudness_toggle(TDRUM);
        assert(ui_loud_active(TDRUM) && ui_drum_loud());
        loudness_toggle(TDRUM);
        assert(!ui_loud_active(TDRUM) && !ui_drum_loud());
    }
    printf("PASS LOUD ALL: %u factory presets and %u drum kits\n", NBANK - 1u, (unsigned)DRUM_KITS);
    return 0;
}
