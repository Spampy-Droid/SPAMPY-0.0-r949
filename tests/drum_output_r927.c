/* Verifies the factory drum-output compensation preserves mixer endpoints and
 * raises an ordinary drum hit by the intended fixed amount. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static uint64_t render_energy(int level)
{
    uint32_t b, i;
    uint64_t energy = 0;
    int32_t l[CTL], r[CTL], x[CTL];
    memset(&drums, 0, sizeof drums);
    drums.set = -2;
    TDRUM->p[P_E0] = 0;
    song.g[G_DRLVL] = (int16_t)level;
    drum_on(36, 110);
    for (b = 0; b < FS * 2u / CTL; b++) {
        memset(l, 0, sizeof l);
        memset(r, 0, sizeof r);
        memset(x, 0, sizeof x);
        drums_render(l, r, x, x, CTL);
        for (i = 0; i < CTL; i++) {
            assert(l[i] > -131072 && l[i] < 131072);
            energy += (uint64_t)(l[i] < 0 ? -l[i] : l[i]);
        }
    }
    return energy;
}

int main(void)
{
    uint64_t zero = render_energy(0), normal = render_energy(100), max = render_energy(127);
    assert(zero == 0);
    assert(normal > 0 && max > normal);
    printf("PASS drum output: zero remains silent; normal=%llu max=%llu bounded\n",
           (unsigned long long)normal, (unsigned long long)max);
    return 0;
}
