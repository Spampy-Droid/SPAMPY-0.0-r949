#define UI_PAGES_HARNESS_ONLY
#include "ui_pages_test.c"
#include <assert.h>

int main(void)
{
    track_t *t = &trk[0];
    uint32_t k = NBANK, q;
    int32_t base;
    panel = PANEL_DEFAULT;
    layers_init();
    host_tracks_init();
    bank_resolve();
    for (q = 0; q < NBANK; q++)
        if (BANK[q].e != 4u) {                          /* skip GM KIT */
            k = q;
            break;
        }
    assert(k < NBANK);
    set_engine_of(t, BANK[k].e);
    apply_preset_to(t, bank_pi[k]);
    base = preset_trim(t->eng_req, t->preset);
    assert(t->p[P_ED_FX] == base && !ui_loud_active(t));
    loudness_toggle(t);
    assert(t->p[P_ED_FX] == base && ui_loud_active(t));
    loudness_toggle(t);
    assert(t->p[P_ED_FX] == base && !ui_loud_active(t));
    loudness_toggle(t);
    apply_preset_to(t, bank_pi[(k + 1u) % NBANK]);
    assert(!ui_loud_active(t));                       /* next sound starts at its own factory level */
    /* The panel shortcut is HOLD FX then TAP SEL. It must not open either layer. */
    set_engine_of(t, BANK[k].e);
    apply_preset_to(t, bank_pi[k]);
    assert(ui_loud_active(t));                       /* returning to a sound keeps its state */
    { uint32_t page = ui.page, fb = BT(B_FX), sb = BT(B_SCL);
      fm1_in.buttons |= fb; edges_btn |= fb; frame();
      fm1_in.buttons |= sb; edges_btn |= sb; frame();
      assert(!ui_loud_active(t));
      fm1_in.buttons &= ~(fb | sb); frame();
      assert(ui.page == page);
    }
    loudness_toggle(TDRUM);
    assert(ui_loud_active(TDRUM) && ui_drum_loud());
    loudness_toggle(TDRUM);
    assert(!ui_loud_active(TDRUM) && !ui_drum_loud());
    printf("PASS LOUD: selected synth +9 dB, remembered per preset, drum kit support and FX+SEL work\n");
    return 0;
}
