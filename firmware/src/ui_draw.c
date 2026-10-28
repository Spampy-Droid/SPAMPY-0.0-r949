/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void draw_menu(void);
static uint32_t str_hash(uint32_t h, const char *s);

/* --------------------------------------------------------- drawing --- */
UI_SMALL static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* at most 5 characters, and no wider than maxw */
UI_SMALL static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, is_eng_name(src) ? 8 : 6);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

UI_SMALL static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
UI_SMALL static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* top bar: transport, BPM, octave | USB, battery, CPU; messages replace it */
UI_SMALL static void draw_head(void)
{
    char b[16];
    uint32_t i;
    int32_t x;
    uint32_t rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;   /* 2 the selected track armed, 1 another */
    uint32_t sig = (uint32_t)song.playing * 3u + rec * 5u + (uint32_t)(song.octave + 8) * 11u + song.sel * 13131u +
                   (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (uint32_t)song.g[G_BPM] * 101u + (ui.bpm_t != 0) * 31u +
                   (uint32_t)batt_shown() * 7777u + (usb.config && !usb.suspended) * 99991u + mlu.on * 7919u;
    if (!ui.force && sig == ui.head_sig)
        return;
    ui.head_sig = sig;
    cv_begin(240, H_HEAD, C_BLACK);
    if (ui.msg_t) {
        cv_text(4, 1, &FONT_S, ui.msg, C_HI);
        cv_blit(0, Y_HEAD);
        return;
    }
    if (song.playing) {                               /* > play, square stop */
        for (i = 0; i < 5u; i++)
            cv_rect(4 + (int32_t)i * 2, 4 + (int32_t)i, 2, 10 - 2 * (int32_t)i, C_WHITE);
    } else {
        cv_rect(4, 5, 8, 8, C_HI);
    }
    if (rec)                                          /* recording armed: white = this track, gray = another */
        te_disc(21, 9, 4, rec == 2u ? TE_RED : C_GRAY);
    fmt_int(b, song.g[G_BPM]);
    x = 32;
    if (FELUCCA_ICONS) {                              /* metronome, then the BPM */
        cv_icon_head(x, 0, ICON_TEMPO, C_GRAY);
        x += 20;
    }
    x = cv_text(x, 1, &FONT_S, b, ui.bpm_t ? C_WHITE : C_HI);   /* white while SELECT turns it */
    if (mlu.on) {                                     /* MIDI LEARN on (2.6, GLO + F#3): in the octave's place */
        cv_text(x + 12, 1, &FONT_S, "LEARN", C_AMB);
    } else if (song.octave) {
        str_cpy(b, song.octave > 0 ? "+" : "", 4);
        fmt_int(b + str_len(b), song.octave);
        cv_text(x + 12, 1, &FONT_S, "OCT", C_GRAY);
        cv_text(x + 40, 1, &FONT_S, b, C_HI);
    }
    if (FELUCCA_ICONS) {                              /* the selected track: tape + number */
        cv_icon(142, 2, ICON_TAPE, C_GRAY);
        b[0] = (char)('1' + song.sel);
        b[1] = 0;
        cv_rect(162, 2, 12, 16, TE_COL[song.sel & 3u]);
        cv_text(164, 1, &FONT_S, b, RGB(255, 255, 255));
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + song.sel);
        b[2] = 0;
        cv_text(158, 1, &FONT_S, b, C_HI);
    }
    {   /* battery, 3 bars; USB when a host is there */
        int32_t lvl = batt_shown(), k;
        int32_t bx = 236 - 19;                          /* right edge (the CPU figure is in the console) */
        cv_rect(bx, 4, 17, 1, C_GRAY);
        cv_rect(bx, 12, 17, 1, C_GRAY);
        cv_rect(bx, 4, 1, 9, C_GRAY);
        cv_rect(bx + 16, 4, 1, 9, C_GRAY);
        cv_rect(bx + 17, 6, 2, 5, C_GRAY);
        for (k = 0; k < lvl; k++)
            cv_rect(bx + 2 + k * 5, 6, 3, 5, lvl == 1 && batt_level() <= 1 ? C_WHITE : C_HI);
        if (usb.config && !usb.suspended)
            cv_text(bx - 37, 1, &FONT_S, "USB", C_DIM);
    }
    cv_blit(0, Y_HEAD);
}
/* full redraw: the strips (head, columns, graph, foot) cover the rest, so only
 * the space between them is cleared, then the rules are drawn */
UI_SMALL static void draw_frame(void)
{
    uint32_t i;
    uint16_t divider = C_LINE;
    uint32_t fam = cur_page()->fam;
    uint32_t slicer = cur_page()->graph == GR_SLCR;
    if (fam == FAM_GLO || fam == FAM_EDIT || fam == FAM_LFO || fam == FAM_ENV || fam == FAM_FX || fam == FAM_SAVE)
        divider = RGB(255, 73, 215);
    if (slicer)
        divider = RGB(255, 73, 215);
    lcd_fill(0, H_HEAD, 240, Y_LABEL - H_HEAD, C_BLACK);
    lcd_fill(0, Y_SEP_END, 240, Y_GRAPH - Y_SEP_END, C_BLACK);
    lcd_fill(0, Y_GRAPH + H_GRAPH, 240, Y_FOOT - Y_GRAPH - H_GRAPH, C_BLACK);
    for (i = 0; i < 4u; i++)                            /* left inset of each column */
        lcd_fill(i * 60u, Y_LABEL, 4, Y_SEP_END - Y_LABEL, C_BLACK);
    lcd_fill(0, H_HEAD, 240, 1, C_LINE);
    lcd_fill(0, Y_FOOT - 2, 240, 1, C_LINE);
    if (slicer) {                                    /* SLICER: fuchsia frames around its four values */
        lcd_fill(0, Y_LABEL - 1, 240, 1, divider);
        lcd_fill(0, Y_SEP_END, 240, 1, divider);
    }
    for (i = 1; i < 4u; i++)
        lcd_fill(i * 60u - 1u, H_HEAD + 4, 1, Y_SEP_END - H_HEAD - 4, divider);
}

/* one column: [icon] LABEL / value unit / gauge, redrawn only when it changed.
 * ratio: 0..1000 for the gauge, -1 = no gauge. icon: ICON_* (icons.c), ICON_AUTO = by label */
#define LABEL_X (FELUCCA_ICONS ? ICON_CELL + ICON_GAP : 0)
static uint8_t draw_capture;

UI_SMALL static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc,
                        int32_t ratio, uint32_t icon)
{
    char l[8], v[8], u[8], key[32];
    int32_t x, gw = 52, fx;
    if (icon == ICON_AUTO)
        icon = icon_for_label(label);
    fit(l, label, &FONT_TINY, 54 - LABEL_X);
    fit(v, val, &FONT_M, 56);   /* engine names whole */
    fit(u, unit, &FONT_TINY, 54 - text_w(&FONT_M, v) - 3);
    str_cpy(key, l, 8);                                 /* cache key: texts + colour + gauge */
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), v, 8);
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), u, 8);
    {
        uint32_t n = str_len(key);
        key[n] = (char)('A' + (vc == C_WHITE) + (vc == C_DIM) * 2);
        key[n + 1] = (char)(' ' + (ratio < 0 ? 0 : 1 + ratio / 20));
        key[n + 2] = (char)(icon == ICON_NONE ? '~' : '!' + icon % 90u);   /* same label, other icon */
        key[n + 3] = 0;
    }
    if (c < 4u) {                                       /* (the big values: their own fit) */
        str_cpy(ui.big_l[c], l, 8);
        fit(ui.big_v[c], val, ui.home ? &FONT_HUGE : &FONT_L, ui.home ? 220 : 112 - (unit[0] ? 4 : 0));
        str_cpy(ui.big_u[c], unit, 8);
        ui.big_c[c] = vc;
    }
    if (c == ui.hot_col) {
        str_cpy(ui.focus_l, l, 8);
        str_cpy(ui.focus_v, v, 8);
        str_cpy(ui.focus_u, u, 8);
    }
    if (ui.home) return;
    if (draw_capture)
        return;
    if (!ui.force && str_eq(key, ui.col[c]))
        return;
    str_cpy(ui.col[c], key, sizeof ui.col[c]);
    cv_begin(55, Y_SEP_END - Y_LABEL, C_BLACK);         /* x 4..58: the rule at 59 stays */
    if (FELUCCA_ICONS && icon != ICON_NONE && l[0])
        cv_icon(0, 1, icon, TE_COL[c & 3u]);            /* icon rows 1..10 = the label's cap height */
    cv_text(l[0] ? LABEL_X : 0, 0, &FONT_TINY, l, C_GRAY);
    x = cv_text(0, 22, &FONT_M, v, vc);
    cv_text(x + 3, 25, &FONT_TINY, u, C_DIM);
    if (ratio >= 0 && 0u) {                                   /* gauge: track, fill, 1 px end line */
        int32_t gy = Y_GAUGE - Y_LABEL;
        fx = ratio * gw / 1000;
        cv_rect(0, gy + 1, gw, 1, C_LINE);
        cv_rect(0, gy, fx, 3, vc == C_DIM ? C_DIM : TE_DIM[c & 3u]);
        cv_rect(fx, gy - 1, 1, 5, vc == C_DIM ? C_HI : vc);
    }
    cv_blit(c * 60u + 4u, Y_LABEL);
}

static void cv_line2(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
    cv_line(x0, y0 - 2, x1, y1 - 2, color);
    cv_line(x0, y0 - 1, x1, y1 - 1, color);
    cv_line(x0, y0, x1, y1, color);
    cv_line(x0, y0 + 1, x1, y1 + 1, color);
    cv_line(x0, y0 + 2, x1, y1 + 2, color);
}

/* Matches voice.c: attack is linear, decay and release are exponential
 * (env += (target - env) * k each tick, ~99 % after the set time). Time
 * axis is the parameter value (the times themselves are exponential). */
UI_SMALL static void graph_adsr(const track_t *t, uint16_t c)
{
    c = RGB(255, 73, 215);
    int32_t a = 4 + t->p[P_ATK] * 50 / 127, d = 6 + t->p[P_DEC] * 50 / 127, r = 6 + t->p[P_REL] * 60 / 127;
    int32_t top = 4, bot = 82, sus = t->p[P_SUS] * 1000 / 127;          /* 0..1000 */
    int32_t x0 = 6, x1 = x0 + a, x3 = 232 - r, i, px, py;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
    cv_line2(x0, bot, x1, top, c);                                       /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        cv_line2(px, py, x1 + i, EGY(lvl), c);
        px = x1 + i;
        py = EGY(lvl);
    }
    cv_line2(px, py, x3, EGY(sus), c);                                    /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        cv_line2(px, py, x3 + i, EGY(sus * e >> 15), c);
        px = x3 + i;
        py = EGY(sus * e >> 15);
    }
    cv_line(0, bot + 1, 239, bot + 1, C_LINE);
#undef EGY
}

UI_SMALL static void graph_lfo(const track_t *t, uint16_t c)
{
    c = RGB(255, 73, 215);
    int32_t x, py = 42;
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25;
    for (x = 0; x < 240; x++) {                      /* (lfo_wave only reads the track) */
        int32_t y = 42 - lfo_wave((track_t *)t, ph + (uint32_t)x * (0xFFFFFFFFu / 120u)) * 38 / 32768;
        if (t->p[P_LWAVE] == 4)
            y = 42 - ((int32_t)((x / 20 * 2654435761u) >> 16) - 32768) * 38 / 32768;
        if (x)
            cv_line2(x - 1, py, x, y, c);
        py = y;
    }
    cv_line(0, 42, 239, 42, C_LINE);
}

UI_SMALL static void graph_steps(const track_t *t, uint16_t c)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN];
    for (i = 0; i < NSTEP; i++) {                    /* 4 rows of 16 thin bars */
        int32_t x = 6 + (int32_t)(i % 16u) * 14 + (int32_t)(i % 16u) / 4 * 4, y = 6 + (int32_t)(i / 16u) * 24;
        const step_t *st = &t->step[i];
        if (i >= len)
            continue;
        if (step_on(st))
            cv_rect(x, y, 1, 14, c);
        else if (st->time == ST_TIE)
            cv_rect(x, y + 6, 1, 8, C_DIM);
        else
            cv_rect(x, y + 13, 1, 1, C_DIM);
        if (st->flags & SF_ACCENT)
            cv_rect(x - 1, y - 2, 3, 1, c);
        if ((song.playing && i == t->seq_idx) || i == ui.cursor)
            cv_rect(x - 1, y + 16, 3, 3, C_WHITE);
    }
}
/* STEP page: the cursor's 16-step bank as a little piano roll */
UI_SMALL static void graph_roll(const track_t *t, uint16_t c)
{
    uint32_t i, j, len = (uint32_t)t->p[P_SLEN], base = ui.bank * 16u;
    int32_t lo = 127, hi = 0, prev_y = -1;
    for (i = 0; i < len; i++)
        for (j = 0; j < t->step[i].n; j++) {
            if (t->step[i].note[j] < lo)
                lo = t->step[i].note[j];
            if (t->step[i].note[j] > hi)
                hi = t->step[i].note[j];
        }
    if (lo > hi) {
        lo = 48;
        hi = 72;
    }
    if (hi - lo < 12)
        hi = lo + 12;
    for (i = 0; i < 16u; i++) {
        uint32_t si = base + i;
        const step_t *st = &t->step[si];
        int32_t x = (int32_t)i * 15, yb = 86;
        if (si >= len)
            break;
        if (si == ui.cursor)
            cv_rect(x + 5, 92, 3, 3, C_WHITE);
        if (song.playing && si == t->seq_idx)
            cv_rect(x + 1, 97, 12, 1, C_WHITE);
        if (st->time == ST_TIE && prev_y >= 0) {
            cv_rect(x, prev_y, 14, 1, C_GRAY);
            continue;
        }
        if (!step_on(st)) {
            cv_rect(x + 6, yb, 2, 1, C_DIM);
            prev_y = -1;
            continue;
        }
        for (j = 0; j < st->n; j++) {
            int32_t y = 80 - (st->note[j] - lo) * 74 / (hi - lo);
            cv_rect(x + 2, y, 10, 1, (st->flags & SF_ACCENT) ? C_WHITE : c);
            if (j == 0)
                prev_y = y;
        }
        if (st->flags & SF_SLIDE)
            cv_line(x + 11, prev_y, x + 17, prev_y + 2, c);
    }
}
UI_SMALL static void graph_scale(const track_t *t, uint16_t c)
{
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t i, mask = scale_mask(t);
    for (i = 0; i < 12u; i++) {
        uint32_t deg = (i + 12u - (uint32_t)t->p[P_ROOT]) % 12u;
        int32_t x = 6 + (int32_t)i * 19;
        uint16_t col = (mask >> deg) & 1u ? (deg == 0u ? C_WHITE : c) : C_DIM;
        cv_rect(x, BLACK[i] ? 10 : 40, 16, 1, col);
        cv_rect(x + 7, BLACK[i] ? 10 : 40, 1, 40, col);
    }
}

UI_SMALL static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t h = t->p[P_DIST + i] * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x, 10, 1, 80, C_LINE);
        cv_rect(x, 90 - h, 1, h, c);
        cv_rect(x - 3, 90 - h, 7, 1, c);
    }
}
/* SLICER page: the pattern's 16 steps, a 'x' step a full bar; a '.' step: GATE a bar as high as
 * it stays open (DEPTH), STUT hatched (it repeats the last 'x'); the step playing underlined.
 * Grey when the SLICER is OFF. */
UI_SMALL static void graph_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t open = 70 - t->p[P_SLDEPTH] * 70 / 127;      /* px a closed GATE step keeps */
    (void)c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2, y;
        uint16_t col = mode == SL_OFF ? C_WHITE : (i & 1u ? RGB(255, 73, 215) : RGB(0, 245, 255));
        if ((pat >> i) & 1u) {
            cv_rect(x, 10, 11, 70, col);
        } else if (mode == SL_STUT) {
            for (y = 10; y < 80; y += 4)
                cv_rect(x, y, 11, 1, col);
        } else {
            cv_rect(x, 79, 11, 1, C_LINE);
            if (open)
                cv_rect(x, 80 - open, 11, open, C_DIM);
        }
        if (mode != SL_OFF && i == cur)
            cv_rect(x, 85, 11, 3, C_WHITE);
    }
}
UI_SMALL static uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &t->step[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
    }
    return h;
}

UI_SMALL static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* a page drawn with its values large (graph_big) */
UI_SMALL static int big_page(const page_t *pg)            /* (2.6: the INSERT pages too: their middle stayed empty) */
{
    return (pg->graph == GR_NONE || pg->graph == GR_INS) && pg->scope != SC_SONG && pg->scope != SC_DRUM;
}
UI_SMALL static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    if (ui.hot_t && settings.zoom)
        h = str_hash(str_hash(str_hash(h ^ 0x5555u, ui.focus_v), ui.focus_l), ui.focus_u);
    if (!ui.home && big_page(pg))                    /* the big values: as the columns show them */
        for (i = 0; i < 4u; i++)
            h = str_hash(str_hash(str_hash(h ^ ui.big_c[i] * 31u, ui.big_v[i]), ui.big_l[i]), ui.big_u[i]);
    if (ui.home)
        return h ^ (ui.frame / 2u);                  /* scope: redraw every other frame */
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u;
    {   /* (2.7) the favourites: the PRESETS list and its FAV column */
        uint32_t q;
        for (q = 0; q < FAV_N; q++) h = h * 31u + fav[q];
    }
    if (pg->graph == GR_SLCR && t->p[P_SLCR])        /* the SLICER's step playing */
        h ^= (sl[song.sel].idx + 1u) * 2654435761u;
    if (pg->graph == GR_SLOTS)                       /* (a checksum over each slot) */
        for (i = 0; i < 4u; i++)
            h ^= (uint32_t)project_used(i) << (20u + i);
    if (pg->graph == GR_STEPS || pg->graph == GR_ROLL) {
        uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu;
        if (pg->graph == GR_ROLL && ph / 16u != ui.bank)
            ph = 0xFFFFu;                            /* the roll shows the cursor's bank only */
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u;
    }
    return h;
}
/* preset browser: the list by kind (ui.c BANK), the current one in white */
UI_SMALL static void graph_browse(void)
{
    uint32_t total, cur = preset_pos(&total), e, k;
    int32_t row;
    if (!total)
        return;
    for (row = -3; row <= 3; row++) {
        int32_t y = 4 + (row + 3) * 17;
        char tag[5], nm[13];
        int sel = row == 0;
        uint32_t n = (cur + total * 4u + (uint32_t)row) % total;
        e = preset_at(n, &k);
        if (e == NENGINES) {                             /* user preset: "U07" and its name */
            up_slot_label(tag, k);
            up_name(k, nm);
        } else {                                         /* its kind: BASS, KEYS, PAD... */
            str_cpy(tag, preset_kind(n), sizeof tag);
            { const preset_t *pr = preset_data(e, k); str_cpy(nm, pr ? pr->name : "", sizeof nm); }
        }
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_GRAY : C_DIM);
        cv_text(54, y, &FONT_S, nm, sel ? C_WHITE : C_GRAY);
        if (e == NENGINES)
            cv_rect(57 + text_w(&FONT_S, nm), y + 6, 4, 4, RGB(255, 45, 45));
    }
}

/* user preset slots around the selected one: "U07  NAME" / EMPTY, the selected one in white */
UI_SMALL static void graph_user(void)
{
    int32_t row, first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
    for (row = 0; row < 7; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 17;
        char tag[4], nm[13];
        int sel = k == ui.uslot, used = up_used(k);
        up_slot_label(tag, k);
        if (used)
            up_name(k, nm);
        else
            str_cpy(nm, "EMPTY", sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(54, y, &FONT_S, nm, used ? (sel ? C_WHITE : C_HI) : C_DIM);
        if (used)
            cv_rect(57 + text_w(&FONT_S, nm), y + 6, 4, 4, RGB(255, 45, 45));
    }
}

/* the sections A..D (the four slots): used / empty, the selected one in white */
UI_SMALL static void graph_slots(void)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t y = 8 + (int32_t)i * 26;
        char b[4];
        int sel = (int32_t)i + 1 == song.g[G_SLOT];
        b[0] = (char)('A' + i);
        b[1] = 0;
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, sel ? C_WHITE : C_GRAY);
        cv_text(40, y, &FONT_S, project_used(i) ? "USED" : "EMPTY", project_used(i) ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* TRACKS page: four channel strips (mixer style), one under each column: number +
 * REC / ARM / MUTE, the sound's short name, then a level fader with the output meter
 * (note activity), the pattern over its LEN (time running down; bar width = notes in
 * the step) and the play head. The selected track is drawn bright. Every part is
 * its own small canvas with its own signature: while the transport runs only the
 * head markers and the meters move. (No ZOOM focus here.) */
#define TS_HEAD_Y (Y_GRAPH + 2)
#define TS_NAME_Y (Y_GRAPH + 20)
#define TS_Y (Y_GRAPH + 38)
#define TS_H 84                                      /* body: rows Y_GRAPH + 38 .. + 121 */
static struct {
    uint32_t head[NTRK], name[NTRK], fader[NTRK], ov[NTRK], mark[NTRK];
    uint8_t meter[NTRK];
} ts;

UI_SMALL static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px: 6 dB = TS_H / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);             /* 8 log2(a): 48 (-54 dB) .. 128 (+6 dB) */
    return clamp((v - 48) * (TS_H - 2) / 80, 0, TS_H - 2);
}

UI_SMALL static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

UI_SMALL static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, "DRUM", 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        { const preset_t *pr = preset_data(t->eng_req % NENGINES, t->preset % preset_logical_limit(t->eng_req)); str_cpy(b, pr ? pr->name : e->name, 13); }
    else
        str_cpy(b, e->name, 13);
}

UI_SMALL static void draw_tracks(void)
{
    uint32_t c;
    if (ui.force) {
        lcd_fill(0, Y_GRAPH, 240, H_GRAPH, C_BLACK);
        for (c = 0; c < NTRK; c++)
            ts.meter[c] = 0;
    }
    for (c = 0; c < NTRK; c++) {
        track_t *t = &trk[c];
        uint32_t x0 = c * 60u + 4u, sel = c == song.sel, lvl = trk_level(c), mute = !lvl || t->p[P_MUTE];
        uint32_t arm = (song.rec >> c) & 1u, st = arm ? (song.playing ? 1u : 2u) : mute ? 3u : 0u, sig;
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, row = 0xFFFFu;
        int32_t pk, m;
        char b[16];
        if (c == TRK_DRUM) {
            pk = drums.peak;
            drums.peak = 0;
        } else {
            pk = t->peak;
            t->peak = 0;
        }
        /* head: number (white = selected), REC / ARM / MUTE */
        sig = 1u + sel + st * 2u + (uint32_t)ui_loud_active(t) * 16u;
        if (ui.force || sig != ts.head[c]) {
            ts.head[c] = sig;
            b[0] = (char)('1' + c);
            b[1] = 0;
            cv_begin(54, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_WHITE : C_GRAY);
            if (sel)
                cv_rect(0, 15, 8, 1, C_WHITE);
            if (ui_loud_active(t)) {                 /* yellow dot between track number and sound name */
                cv_rect(11, 10, 3, 1, RGB(255, 220, 0));
                cv_rect(10, 11, 5, 3, RGB(255, 220, 0));
                cv_rect(11, 14, 3, 1, RGB(255, 220, 0));
            }
            if (st == 1u || st == 2u) {
                cv_rect(14, 5, 6, 6, st == 1u ? C_WHITE : C_AMB);
                cv_text(24, 0, &FONT_S, st == 1u ? "REC" : "ARM", st == 1u ? C_WHITE : C_AMB);
            } else if (st) {
                cv_text(14, 0, &FONT_S, "MUTE", C_DIM);
            }
            cv_blit(x0, TS_HEAD_Y);
        }
        /* the sound: preset name (user preset, DRUM), cut to the column */
        trk_short_name(c, b);
        /* A saved copy reserves ten pixels for its red HOME marker.  The
         * old 44-pixel limit put that marker beyond this 54-pixel canvas,
         * where it was silently clipped. */
        while (b[0] && text_w(&FONT_S, b) > (t->user ? (ui_loud_active(t) ? 28 : 37) : 54))
            b[str_len(b) - 1u] = 0;
        sig = str_hash(2u + sel + (uint32_t)ui_loud_active(t) * 16u + (uint32_t)(t->user != 0) * 32u, b);
        if (ui.force || sig != ts.name[c]) {
            ts.name[c] = sig;
            cv_begin(54, 16, C_BLACK);
            if (ui_loud_active(t))
                cv_rect(0, 5, 6, 6, RGB(255, 255, 0));
            {
                int32_t nx = ui_loud_active(t) ? 9 : 0;
                cv_text(nx, 0, &FONT_S, b, sel ? C_HI : C_DIM);
                /* t->user is set as soon as the saved copy is loaded.  Draw
                 * from that state directly: it must not depend on a later
                 * flash-bank validity check. */
                if (t->user) {
                    int32_t dx = nx + text_w(&FONT_S, b) + 2;
                    cv_rect(dx + 3, 1, 6, 12, RGB(255, 0, 0));
                    cv_rect(dx + 1, 3, 10, 8, RGB(255, 0, 0));
                }
            }
            cv_blit(x0, TS_NAME_Y);
        }
        /* fader (LEVEL) + meter of the output */
        m = mute ? 0 : meter_px(pk);
        if (m < ts.meter[c] - 3)
            m = ts.meter[c] - 3;                     /* falls ~20 dB/s */
        ts.meter[c] = (uint8_t)(m < 0 ? 0 : m);
        sig = 1u + lvl + ts.meter[c] * 128u + sel * 65536u + (sel && ui.hot_t && ui.hot_col == 1u) * 131072u +
              (t->p[P_MUTE] != 0) * 262144u;
        if (ui.force || sig != ts.fader[c]) {
            int32_t fy = (int32_t)(TS_H - 2u) - (int32_t)lvl * (TS_H - 4) / 127;
            ts.fader[c] = sig;
            cv_begin(12, TS_H, C_BLACK);
            cv_rect(3, 0, 1, TS_H, C_LINE);
            if (lvl) {
                cv_rect(2, fy, 3, TS_H - fy, C_DIM);
                cv_rect(0, fy - 1, 7, 2, (sig & 131072u) ? C_WHITE : sel ? C_HI : C_GRAY);
            } else {
                cv_rect(0, TS_H - 2, 7, 2, C_DIM);
            }
            if (ts.meter[c])
                cv_rect(9, TS_H - ts.meter[c], 3, ts.meter[c], C_AMB);
            cv_blit(x0, TS_Y);
        }
        /* the pattern: one band per step over LEN, a note step as wide as its notes */
        sig = steps_hash(t) * 31u + len * 7u + sel;
        if (ui.force || sig != ts.ov[c]) {
            uint32_t i;
            ts.ov[c] = sig;
            cv_begin(34, TS_H, C_BLACK);
            cv_rect(17, 0, 1, TS_H, C_LINE);
            for (i = 0; i < len; i++) {
                const step_t *s = &t->step[i];
                int32_t r0 = (int32_t)(i * TS_H / len), r1 = (int32_t)((i + 1u) * TS_H / len), h = r1 - r0;
                if (h > 2)
                    h--;                             /* a gap between steps when there is room */
                if (h < 1)
                    h = 1;
                if (step_on(s)) {
                    int32_t w = 2 + 6 * (int32_t)s->n;
                    cv_rect(17 - w / 2, r0, w, h, (s->flags & SF_ACCENT) && sel ? C_WHITE : sel ? C_HI : C_GRAY);
                } else if (s->time == ST_TIE) {
                    cv_rect(16, r0, 3, r1 - r0, C_DIM);
                }
            }
            cv_blit(x0 + 14u, TS_Y);
        }
        /* play head: a small arrow right of the pattern (white while recording) */
        if (song.playing)
            row = ((t->seq_idx % len) * 2u + 1u) * TS_H / (2u * len);
        sig = 1u + row + (st == 1u) * 65536u;
        if (ui.force || sig != ts.mark[c]) {
            int32_t i;
            ts.mark[c] = sig;
            cv_begin(5, TS_H, C_BLACK);
            if (row != 0xFFFFu)
                for (i = -2; i <= 2; i++) {
                    int32_t w = 5 - 2 * (i < 0 ? -i : i);
                    cv_rect(5 - w, (int32_t)row + i, w, 1, st == 1u ? C_WHITE : C_AMB);
                }
            cv_blit(x0 + 49u, TS_Y);
        }
    }
}

/* oscilloscope of the output, triggered on a rising zero crossing */
#include "scratch.h"                                /* sloop-lab: the snapshot in the main loop's shared scratch */
_Static_assert(sizeof main_scratch.snap == SCOPE_N * sizeof(int16_t), "the scratch holds the snapshot");
UI_SMALL static void graph_scope(uint16_t c)
{
    int16_t *snap = main_scratch.snap;
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = 50, x, peak = 1500;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_line(0, 50, 239, 50, C_LINE);
    for (x = 0; x < 240; x++) {
        int32_t y = 50 - snap[trig + (uint32_t)x] * 44 / peak;   /* auto-scaled */
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
}

/* SLOOP 2.4: a page without a graph (EDIT, VOICE, the DEST pages, GLOBAL, MASTER, SYSTEM...): its values
 * large in the empty space, 2 x 2 as the knobs (KNOB 1 2 / KNOB 3 4), the one turned in white; one value
 * alone (FILTER) across the width. The columns (draw_columns, drawn first) fill ui.big_* */
UI_SMALL 
static void graph_big(void)
{
    uint32_t c, n = 0;
    for (c = 0; c < 4u; c++)
        n += ui.big_l[c][0] != 0;
    if (n > 1u) {
        cv_rect(119, 0, 1, 96, C_LINE);
        cv_rect(6, 47, 228, 1, C_LINE);
    }
    for (c = 0; c < 4u; c++) {
        int32_t x0, y0, x;
        if (!ui.big_l[c][0])
            continue;
        if (n == 1u) {
            int32_t unit_w = ui.big_u[c][0] ? text_w(&FONT_TINY, ui.big_u[c]) + 4 : 0;
            x0 = (240 - text_w(&FONT_L, ui.big_v[c]) - unit_w) / 2;
            if (x0 < 4) x0 = 4;
            cv_text(x0, 4, &FONT_TINY, ui.big_l[c], TE_COL[c & 3u]);
            x = cv_text(x0, 28, &FONT_L, ui.big_v[c],
                        ui.big_c[c] == C_DIM ? C_DIM : ui.big_c[c] == C_WHITE ? C_WHITE : C_HI);
            if (ui.big_u[c][0])
                cv_text(x + 4, 38, &FONT_TINY, ui.big_u[c], C_GRAY);
            continue;
        }
        x0 = c & 1u ? 126 : 6;
        y0 = c & 2u ? 48 : 0;
        cv_text(x0, y0 + 1, &FONT_TINY, ui.big_l[c], TE_COL[c & 3u]);
        x = cv_text(x0, y0 + 14, &FONT_L, ui.big_v[c],
                    ui.big_c[c] == C_DIM ? C_DIM : ui.big_c[c] == C_WHITE ? C_WHITE : C_HI);
        if (ui.big_u[c][0])
            cv_text(x + 4, y0 + 21, &FONT_TINY, ui.big_u[c], C_GRAY);
    }
}
static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = TE_COL[song.sel & 3u];               /* LIVE: the curves in the track's colour */
    uint32_t sig, top, drum_note = !ui.home && is_drum(t) && !page_for_drum(pg);
    if (!ui.home && pg->graph == GR_TRK) {
        draw_tracks();
        ui.graph_top = 1;                            /* the next graph draws its top rows again */
        return;
    }
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    ui.graph_sig = sig;
    cv_begin(240, H_GRAPH, C_BLACK);
    cv_oy = G_OY;
    if (ui.home) {
        graph_scope(c);
    } else if (drum_note) {                          /* a page the drum track has no use for */
        static const char *const L[2] = {"DRUM TRACK", "SEQ  TRACKS  GLO DRUMS"};
        cv_text((240 - text_w(&FONT_S, L[0])) / 2, 26, &FONT_S, L[0], C_HI);
        cv_text((240 - text_w(&FONT_S, L[1])) / 2, 52, &FONT_S, L[1], C_DIM);
    } else {
        switch (pg->graph) {
        case GR_ADSR:
            graph_adsr(t, c);
            break;
        case GR_LFO:
            graph_lfo(t, c);
            break;
        case GR_STEPS:
            graph_steps(t, c);
            break;
        case GR_ROLL:
            graph_roll(t, c);
            break;
        case GR_SCALE:
            graph_scale(t, c);
            break;
        case GR_FX:
            graph_fx(t, c);
            break;
        case GR_SLCR:
            graph_slicer(t, c);
            break;
        case GR_BROWSE:
            cv_oy = 0;
            graph_browse();
            break;
        case GR_SLOTS:
            cv_oy = 0;
            graph_slots();
            break;
        case GR_USER:
            cv_oy = 0;
            graph_user();
            break;
        default:
            if (big_page(pg)) {
                cv_oy = 0;
                graph_big();
            }
            break;
        }
    }
    top = !ui.home && !drum_note && (pg->graph == GR_BROWSE || pg->graph == GR_SLOTS || pg->graph == GR_USER ||
                                     big_page(pg));   /* these draw from the top */
    cv_oy = 0;
    if (ui.hot_t && settings.zoom && !(top && big_page(pg) && !drum_note)) {   /* (the big values show it already) */                 /* focus (menu ZOOM): the touched value, large and white */
        int32_t x;
        top = 1;
        cv_rect(0, 0, 150, 50, C_BLACK);
        cv_text(4, 0, &FONT_S, ui.focus_l, C_GRAY);
        x = cv_text(4, 16, &FONT_L, ui.focus_v, C_WHITE);
        cv_text(x + 4, 30, &FONT_S, ui.focus_u, C_DIM);
    }
    /* graphs keep out of the top G_OY rows: skip them unless something is (or was) there */
    cv_blit_from(0, Y_GRAPH, top || ui.graph_top || ui.force ? 0u : G_OY);
    ui.graph_top = (uint8_t)top;
}

/* the page shown: its place in its family and how many (*n), as SELECT walks them (2.7: the screens of their own, SONG
 * and DRUMS, are not counted: SEQ is STEP 1/2, PATTERN 2/2; the page shown always counts) */
UI_SMALL static uint32_t page_pos(uint32_t *n)
{
    const page_t *pg = cur_page();
    uint32_t i, k = 0;
    *n = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == pg->fam && (i == ui.page || (page_shown(&PAGES[i]) && PAGES[i].scope != SC_SONG &&
                                                         PAGES[i].scope != SC_DRUM))) {
            (*n)++;
            if (i == ui.page)
                k = *n;
        }
    return k;
}

UI_SMALL static void draw_foot(void)
{
    char s[48], pn[16], en[10], ti[20];
    const track_t *t = TSEL;
    uint32_t sig;
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    const char *ename = is_drum(t) ? "DRUM" : e->name;
    int32_t x;
    pn[0] = 0;
    if (is_drum(t))
        str_cpy(pn, DRUM_KIT_NAMES[drum_kit()], sizeof pn);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), pn);                       /* a user preset */
    else if (e->npresets)
        { const preset_t *pr = preset_data(TSEL->eng_req % NENGINES, TSEL->preset % preset_logical_limit(TSEL->eng_req)); str_cpy(pn, pr ? pr->name : e->name, sizeof pn); }
    if (ui.home) {
        str_cpy(ti, "HOME", sizeof ti);
    } else {                                           /* page title + number in its family: "ENV DEST 2/2" */
        uint32_t n, k = page_pos(&n);
        const char *pt = pg->scope == SC_ENGINE ? e->page_title[pg->id[0] != P_E0] : 0;   /* EDIT: the engine's */
        str_cpy(ti, pt ? pt : pg->title, 10);
        if (n > 1) {
            str_cpy(ti + str_len(ti), " ", 4);
            fmt_int(ti + str_len(ti), (int32_t)k);
            str_cpy(ti + str_len(ti), "/", 4);
            fmt_int(ti + str_len(ti), (int32_t)n);
        }
    }
    str_cpy(s, ename, sizeof s);
    s[str_len(s) + 1u] = 0;
    s[str_len(s)] = (char)('1' + song.sel);
    str_cpy(s + str_len(s), pn, 16);
    str_cpy(s + str_len(s), ti, sizeof ti);
    {   /* step markers: the playhead only when it is in the shown bank, the cursor only in SEQ */
        uint32_t ph = song.playing && t->seq_idx / 16u == ui.bank ? t->seq_idx : 0xFFu;
        sig = str_hash(0x9E3779B9u, s) + ph * 97u + (song.seq_mode ? ui.cursor : 0xFFu) * 3001u + steps_hash(t) +
              ui.bank * 7u + (uint32_t)t->p[P_SLEN] * 13u;
    }
    if (!ui.force && sig == ui.foot_sig)
        return;
    ui.foot_sig = sig;
    cv_begin(240, H_FOOT, C_BLACK);
    {
        uint32_t i;
        for (i = 0; i < 16u; i++) {                   /* row 1: the cursor's bank as 16 thin bars */
            uint32_t si = ui.bank * 16u + i;
            int32_t sx = 6 + (int32_t)i * 14 + (int32_t)(i / 4u) * 4;
            const step_t *st = &t->step[si];
            if (si >= (uint32_t)t->p[P_SLEN])
                continue;
            if (step_on(st))
                cv_rect(sx, 1, 2, 4, TE_COL[song.sel & 3u]);
            else
                cv_rect(sx, 6, 1, 1, C_DIM);
            if ((song.playing && si == t->seq_idx) || (song.seq_mode && si == ui.cursor))
                cv_rect(sx - 1, 5, 3, 2, C_WHITE);
        }
    }
    x = 4;
    if (FELUCCA_ICONS) {                              /* row 2: engine icon + name, preset, page */
        cv_icon(x, 8, engine_icon(ename), C_GRAY);
        x += 14;
    }
    fit(en, ename, &FONT_S, 90 - (x - 4));
    x = cv_text(x, 8, &FONT_S, en, C_HI);
    {
        char pf[16];
        int32_t room = 236 - text_w(&FONT_S, ti) - 10 - (x + 10);
        if (TSEL->user)
            room -= 12;                              /* reserve the red copy marker */
        str_cpy(pf, pn, sizeof pf);
        if (ui_loud_active(TSEL))
            str_cpy(pf + str_len(pf), " +", sizeof pf - str_len(pf));
        while (pf[0] && text_w(&FONT_S, pf) > room)
            pf[str_len(pf) - 1u] = 0;
        if (ui_loud_active(TSEL))
            cv_rect(x + 1, 9, 6, 6, RGB(255, 255, 0));
        {
            int32_t px = x + (ui_loud_active(TSEL) ? 12 : 10);
            cv_text(px, 8, &FONT_S, pf, C_AMB);
            if (TSEL->user) {
                int32_t dx = px + text_w(&FONT_S, pf) + 2;
                cv_rect(dx + 3, 9, 6, 12, RGB(255, 0, 0));
                cv_rect(dx + 1, 11, 10, 8, RGB(255, 0, 0));
            }
        }
    }
    cv_text(236 - text_w(&FONT_S, ti), 8, &FONT_S, ti, C_GRAY);
    cv_blit(0, Y_FOOT);
}
UI_SMALL static void draw_home_big(void)
{
    uint32_t c = ui.hot_col < 4u ? ui.hot_col : 0u;
    int32_t x, value_w;
    cv_begin(240, Y_SEP_END - Y_LABEL, C_BLACK);
    cv_text(8, 0, &FONT_TINY, ui.big_l[c], TE_COL[c & 3u]);
    value_w = text_w(&FONT_HUGE, ui.big_v[c]);
    x = (240 - value_w) / 2;
    if (x < 4) x = 4;
    x = cv_text(x, 16, &FONT_HUGE, ui.big_v[c],
                ui.big_c[c] == C_DIM ? C_DIM : ui.big_c[c] == C_WHITE ? C_WHITE : C_HI);
    if (ui.big_u[c][0])
        cv_text(x + 4, 47, &FONT_TINY, ui.big_u[c], C_GRAY);
    cv_blit(0, Y_LABEL);
}

static void draw_columns(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    if (ui.home) {
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            param_format(d, *vp, val, &unit);
            draw_column(c, d->label, val, unit, VAL(c), RATIO(d, *vp), param_icon(d, *vp));
        }
        draw_home_big();
        return;
    }
    if (is_drum(TSEL) && !page_for_drum(cur_page())) {   /* "DRUM TRACK" (the graph says so) */
        for (c = 0; c < 4u; c++)
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_TRK) {                 /* TRACK LEVEL LEN PAN of the selected track */
        const track_t *t = TSEL;
        uint32_t lvl = trk_level(song.sel);
        fmt_int(val, (int32_t)song.sel + 1);
        draw_column(0, "TRACK", val, "/5", VAL(0u), (int32_t)song.sel * 1000 / (NTRK - 1), ICON_AUTO);
        if (!lvl || t->p[P_MUTE]) {                    /* (MUTE: a turn of KNOB 2 unmutes, tracks_edit) */
            str_cpy(val, "MUTE", 12);
            unit = "";
        } else if (is_drum(t)) {
            fmt_int(val, (int32_t)lvl);
            unit = "";
        } else {
            param_format(&TP[P_LEVEL], (int32_t)lvl, val, &unit);
        }
        draw_column(1, "LEVEL", val, unit, lvl && !t->p[P_MUTE] ? VAL(1u) : C_DIM, (int32_t)lvl * 1000 / 127, ICON_AUTO);
        param_format(&TP[P_SLEN], t->p[P_SLEN], val, &unit);
        draw_column(2, "LEN", val, unit, VAL(2u), RATIO(&TP[P_SLEN], t->p[P_SLEN]), ICON_AUTO);
        param_format(&TP[P_PAN], t->p[P_PAN], val, &unit);
        draw_column(3, "PAN", val, unit, VAL(3u), RATIO(&TP[P_PAN], t->p[P_PAN]), param_icon(&TP[P_PAN], t->p[P_PAN]));
        return;
    }
    if (cur_page()->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        char u[8];
        fmt_int(val, (int32_t)cur + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)total);
        draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "ENG", ENGINES[TSEL->eng_req]->name, "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "FAV", fav_has(fav_key()) ? "ON" : "--", "", VAL(3u), -1, ICON_NONE);   /* (2.7) KNOB 4 */
        return;
    }
    if (cur_page()->graph == GR_USER) {                  /* SLOT, then three GO buttons */
        int used = up_used(ui.uslot);
        up_slot_label(val, ui.uslot);
        draw_column(0, "SLOT", val, "", VAL(0u), (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_STEP) {
        static const char *const TIME_N[3] = {"NOTE", "TIE", "REST"};
        const step_t *st = &TSEL->step[ui.cursor];
        char u[8];
        if (st->n) {
            note_name(val, st->note[0]);
            u[0] = 0;
            if (st->n > 1) {
                str_cpy(u, "+", 8);
                fmt_int(u + 1, st->n - 1);
            }
        } else {
            str_cpy(val, "--", 12);
            u[0] = 0;
        }
        {
            static const char *const FLAG_N[4] = {"-", "ACC", "SLD", "A+S"};
            char sn[8], sl[8];
            fmt_int(sn, (int32_t)ui.cursor + 1);
            str_cpy(sl, "/", 8);
            fmt_int(sl + 1, TSEL->p[P_SLEN]);
            draw_column(0, "STEP", sn, sl, VAL(0u), -1, ICON_AUTO);
            draw_column(1, "NOTE", val, u, step_on(st) ? VAL(1u) : C_DIM, -1, ICON_AUTO);
            if (step_on(st)) {                          /* a note: its LEN (2.6), steps with the TIEs after it */
                char ln[8];
                fmt_int(ln, (int32_t)note_len(TSEL, ui.cursor));
                draw_column(2, "LEN", ln, "STEP", VAL(2u), -1, ICON_AUTO);
            } else {
                draw_column(2, "TIME", TIME_N[st->time % 3u], "", VAL(2u), -1, ICON_AUTO);
            }
            draw_column(3, "FLAG", FLAG_N[(st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u)], "",
                        VAL(3u), -1, ICON_AUTO);
        }
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        if (cur_page()->graph == GR_SLOTS && c == 1u) {   /* 2.6: ERASE, lit while the slot holds a project */
            draw_column(1, "ERASE", "--", "", project_used((uint32_t)song.g[G_SLOT] - 1u) ? C_HI : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL && !usb.config) {   /* not connected: the
                                                       * USB status instead of the CPU (2.4: MIDI is MIDI OUT, ROUT is IN) */
            str_cpy(val, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 12);
            unit = "USB";
            draw_column(c, "USB", val, unit, C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL) {
            fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
            unit = "%";
        } else {
            param_format(d, *vp, val, &unit);
        }
        draw_column(c, d->label, val, unit, VAL(c), d->fmt == F_ENUM && d->max < 2 ? -1 : RATIO(d, *vp),
                    param_icon(d, *vp));
    }
}


/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
UI_SMALL static void ui_timers(void)
{
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
}


static void draw_card_screen(void)
{
    static uint32_t cached;
    static const uint16_t accents[4] = {
        RGB(0, 220, 232), RGB(255, 73, 215), RGB(113, 255, 210), RGB(255, 184, 90)
    };
    uint32_t i, sig = song.sel + TSEL->preset * 32u + TSEL->eng_req * 4096u +
                      (song.playing ? 65536u : 0u) + (ui.hot_col + 1u) * 131071u;
    const char *title = cur_page()->scope == SC_STEP || cur_page()->graph == GR_STEPS ? "SEQ" :
                        cur_page()->graph == GR_SCALE ? "SEL" :
                        cur_page()->graph == GR_ARP ? "ARP" : "FX";
    char name[20], num[3];
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    for (i = 0; i < 4u; i++)
        sig += str_hash(17u + i, ui.big_l[i]) + str_hash(29u + i, ui.big_v[i]) +
               str_hash(41u + i, ui.big_u[i]);
    if (!ui.force && sig == cached)
        return;
    cached = sig;
    cv_begin(240, 120, C_BLACK);
    cv_text(8, 3, &FONT_L, title, C_WHITE);
    num[0] = 'T'; num[1] = (char)('1' + song.sel); num[2] = 0;
    cv_text(200, 3, &FONT_L, num, RGB(0, 220, 232));
    for (i = 0; i < 4u; i++) {
        int32_t x = 6 + (int32_t)(i & 1u) * 116;
        int32_t y = i < 2u ? 38 : 6;
        if (i == 2u) {
            cv_blit(0, 0);
            cv_begin(240, 120, C_BLACK);
        }
        cv_rect(x, y, 112, 82, RGB(0, 82, 111));
        cv_rect(x, y, 112, 3, accents[i]);
        num[0] = (char)('1' + i); num[1] = 0;
        cv_text(x + 8, y + 6, &FONT_S, num, accents[i]);
        cv_text(x + 30, y + 6, &FONT_S, ui.big_l[i], C_WHITE);
        cv_text(x + 10, y + 27, &FONT_L, ui.big_v[i], C_WHITE);
        if (ui.big_u[i][0]) cv_text(x + 10, y + 61, &FONT_TINY, ui.big_u[i], accents[i]);
    }
    if (user_of(TSEL) < UP_SLOTS)
        up_name(user_of(TSEL), name);
    else if (e->npresets)
        { const preset_t *pr = preset_data(TSEL->eng_req % NENGINES, TSEL->preset % preset_logical_limit(TSEL->eng_req)); str_cpy(name, pr ? pr->name : e->name, sizeof name); }
    else
        str_cpy(name, e->name, sizeof name);
    name[12] = 0;
    cv_text(8, 96, &FONT_S, name, C_WHITE);
    cv_text(184, 96, &FONT_S, song.playing ? "PLAY" : "STOP", RGB(0, 220, 232));
    cv_blit(0, 120);
}
static void vis_draw(void);                     /* ui_vis.c */
static int vis_shown(void);
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* the visualiser (ui_vis.c): on, drawn last frame, its name's time */
#ifdef UI_PAGES_HARNESS_ONLY
static void neon_audio_trigger(uint32_t hz, uint32_t ms) { (void)hz; (void)ms; }
#else
static void neon_audio_trigger(uint32_t hz, uint32_t ms);
#endif
static uint8_t neon_game_active, neon_game_running, neon_game_paused, neon_dir, neon_lives, neon_wave;
static uint8_t neon_level_sel=1,rush_level_sel=1,blocks_level_sel=1,bu_level_sel=1,game_bonus_enabled=1;
static uint32_t arcade_level_at;
static void arcade_pick(uint32_t pressed,uint8_t *level){if((pressed>>panel.btn[B_OCTDN])&1u&&*level>1u)(*level)--;if((pressed>>panel.btn[B_OCTUP])&1u&&*level<5u)(*level)++;ui.force=1;}
static int16_t neon_px, neon_fx, neon_fy, neon_bx[3], neon_by[3], neon_ex, neon_ey, neon_bonus_x, neon_bonus_y, neon_boss_x;
static uint8_t neon_bullet, neon_enemy_bullet, neon_invuln, neon_bonus, neon_weapon=1u, neon_boss_hp, neon_ships=1u;
static int8_t neon_boss_dir=1;
static uint32_t neon_alive, neon_score, neon_step_at, neon_fire_at, neon_enemy_at, neon_shot_at, neon_invuln_at, neon_rand, neon_draw_sig, neon_move_at, neon_draw_at;
static void neon_wave_reset(void)
{
    neon_alive = (1u << 24) - 1u; neon_fx = 0; neon_fy = 0; neon_dir = 1;
    neon_bullet = neon_enemy_bullet = neon_bonus = 0; neon_enemy_at = fm1_ms;
    neon_boss_hp = (neon_wave % 5u == 0u) ? 5u : 0u; neon_boss_x = 120; neon_boss_dir = 1;
}
static uint8_t neon_game_suppress_keys;
uint32_t neon_game_is_active(void)
{
    if (neon_game_suppress_keys && !neon_game_active && !fm1_in.notes)
        neon_game_suppress_keys = 0;
    return (uint32_t)(neon_game_active || neon_game_suppress_keys);
}
static void neon_game_start(void)
{
    neon_game_active = 1; neon_game_running = 0; neon_game_paused = 0; neon_game_suppress_keys = 0; neon_level_sel=1; neon_px = 120; neon_lives = 3; neon_wave = 1;
    neon_rand = 0x51A7u; neon_invuln_at = 0; neon_step_at = neon_fire_at = neon_move_at = fm1_ms;
    neon_wave_reset(); ui.menu = 0; ui.home = 0; ui.layer = LY_PLAY; ui.hold_kind = 0; ui.force = 1;
    /* Wait screen: audio/gameplay begin only after PLAY. */
}
static void neon_game_begin(void)
{
    neon_game_running = 1; neon_game_paused = 0; neon_px = 120; neon_lives = 3; neon_wave = neon_level_sel;
    neon_score = 0; neon_rand = 0x51A7u; neon_invuln_at = 0; neon_weapon = 1u; neon_ships = 1u;
    neon_step_at = neon_fire_at = neon_move_at = neon_enemy_at = neon_shot_at = fm1_ms;
    neon_wave_reset(); ui.force = 1; neon_audio_trigger(880, 90);
}
static void neon_game_input(uint32_t pressed, uint32_t notes)
{
    uint32_t held = fm1_in.notes, now = fm1_ms;
    int32_t dx = ((held & 1u) ? -1 : 0) + ((held & 4u) ? 1 : 0);
    (void)notes;
    panel_enc(EN_K1); panel_enc(EN_K2); panel_enc(EN_K3); panel_enc(EN_K4);
    panel_enc(EN_SELECT); panel_enc(EN_ALGO); panel_enc(EN_PRESET);
    if ((pressed >> panel.btn[B_HOME]) & 1u) { neon_game_active = 0; neon_game_running = 0; neon_game_paused = 0; neon_game_suppress_keys = 1; ui.force = 1; return; }
    if (!neon_game_running) {
        if ((pressed >> panel.btn[B_PLAY]) & 1u) neon_game_begin();
        return;
    }
    if (!neon_lives) {
        if ((pressed >> panel.btn[B_PLAY]) & 1u) neon_game_begin();
        return;
    }
    if ((pressed >> panel.btn[B_PLAY]) & 1u) {
        neon_game_paused = (uint8_t)!neon_game_paused;
        neon_step_at = neon_fire_at = neon_move_at = neon_enemy_at = neon_shot_at = now;
        ui.force = 1;
        return;
    }
    if (neon_game_paused) return;
    if ((held & 5u) == 5u) dx = 0;
    if (dx && now - neon_move_at >= 70u) { neon_move_at = now; neon_px = (int16_t)clamp(neon_px + dx * 4, 11, 229); }
    if ((held & (1u << 26)) && now - neon_fire_at >= 230u && neon_lives) {
        neon_fire_at = now; neon_audio_trigger(880, 42);
        if (!neon_bullet) { if (neon_ships>1u) { neon_bullet=(uint8_t)((1u<<neon_ships)-1u); neon_bx[0]=(int16_t)(neon_px-(neon_ships==3u?16:8)); neon_bx[1]=(int16_t)(neon_ships==3u?neon_px:neon_px+8); neon_bx[2]=(int16_t)(neon_px+(neon_ships==3u?16:8)); neon_by[0]=neon_by[1]=neon_by[2]=205; } else { neon_bullet=1u; neon_bx[0]=neon_px; neon_by[0]=205; if (neon_weapon>1u) { neon_bullet|=2u; neon_bx[1]=(int16_t)(neon_px-6); neon_by[1]=205; } if (neon_weapon>2u) { neon_bullet|=4u; neon_bx[2]=(int16_t)(neon_px+6); neon_by[2]=205; } } }
    }
}
static void neon_enemy_pos(uint32_t c, uint32_t r, int32_t *x, int32_t *y)
{
    *x = 14 + neon_fx + (int32_t)c * 28;
    *y = 43 + neon_fy + (int32_t)r * 22 + (neon_boss_hp ? 22 : 0);
}
static uint32_t neon_alive_at(uint32_t c, uint32_t r) { return (neon_alive >> (r * 8u + c)) & 1u; }
static void neon_game_tick(void)
{
    uint32_t now = fm1_ms, c, r, i;
    if (!neon_game_running || neon_game_paused || !neon_lives) return;
    if (now - neon_step_at >= 30u) {
        neon_step_at = now;
        for (i=0;i<3u;i++) if (neon_bullet & (1u<<i)) {
            neon_by[i] -= 8;
            if (neon_by[i] < 30) neon_bullet &= (uint8_t)~(1u<<i);
            if ((neon_bullet & (1u<<i)) && neon_boss_hp &&
                neon_bx[i]+1 >= neon_boss_x-13 && neon_bx[i]-1 <= neon_boss_x+13 &&
                neon_by[i] <= 59 && neon_by[i]+9 >= 43) {
                neon_boss_hp--; neon_score += neon_boss_hp ? 50u : 500u;
                neon_bullet &= (uint8_t)~(1u<<i); neon_audio_trigger(520,90);
            } else for (r=0;r<3u && (neon_bullet & (1u<<i));r++)
                for (c=0;c<8u;c++) if (neon_alive_at(c,r)) {
                    int32_t x,y; neon_enemy_pos(c,r,&x,&y);
                    if (neon_bx[i]+1 >= x+2 && neon_bx[i]-1 <= x+14 &&
                        neon_by[i] <= y+12 && neon_by[i]+9 >= y) {
                        neon_alive &= ~(1u << (r*8u+c)); neon_score += (3u-r)*10u;
                        neon_rand = neon_rand * 1664525u + 1013904223u;
                        if (!neon_bonus && (neon_rand >> 29) == 0u) { neon_bonus=(uint8_t)(2u+((neon_rand>>27)&3u)); neon_bonus_x=(int16_t)(x+8); neon_bonus_y=(int16_t)(y+4); }
                        neon_bullet &= (uint8_t)~(1u<<i); neon_audio_trigger(180u+r*80u,90); break;
                    }
                }
        }
        if (neon_bonus) {
            neon_bonus_y += 2;
            if (neon_bonus_y >= 207 && neon_bonus_y <= 225 && neon_bonus_x >= neon_px-10 && neon_bonus_x <= neon_px+10) {
                if (neon_bonus < 4u) neon_weapon = neon_bonus; else neon_ships = (uint8_t)(neon_bonus - 2u); neon_bonus = 0; neon_score += 100u; neon_audio_trigger(1100,100);
            } else if (neon_bonus_y >= 237) neon_bonus = 0;
        }
        if (neon_enemy_bullet) {
            neon_ey += 4;
            if (neon_ey >= 232) {
                neon_enemy_bullet = 0;
                if (now - neon_invuln_at > 850u && neon_ex >= neon_px-(int32_t)(neon_ships==1u?9u:neon_ships*8u-2u) && neon_ex <= neon_px+(int32_t)(neon_ships==1u?9u:neon_ships*8u-2u)) {
                    if (neon_lives) neon_lives--;
                    neon_weapon=1u; neon_ships=1u; neon_invuln_at = now; neon_audio_trigger(neon_lives ? 130u : 90u, neon_lives ? 150u : 420u);
                }
            }
        }
    }
    if (neon_alive == 0u && !neon_boss_hp) { neon_wave++; neon_wave_reset(); neon_audio_trigger(1200,120); }
    if (now - neon_enemy_at >= (neon_wave > 5u ? 180u : 420u)) {
        neon_enemy_at = now; neon_fx += neon_dir * 3;
        if (neon_boss_hp) { neon_boss_x += neon_boss_dir*4; if (neon_boss_x<20 || neon_boss_x>220) neon_boss_dir=(int8_t)-neon_boss_dir; }
        if (neon_fx >= 8) {
            neon_fx = 8; neon_dir = -1;
            if (neon_fy < 24) neon_fy += 6; else neon_fy = 0;
        } else if (neon_fx <= -8) {
            neon_fx = -8; neon_dir = 1;
            if (neon_fy < 24) neon_fy += 6; else neon_fy = 0;
        }
    }
    if (!neon_enemy_bullet && now - neon_shot_at > (neon_boss_hp ? 650u : 950u)) {
        neon_shot_at = now;
        if (neon_boss_hp) { neon_enemy_bullet=1; neon_ex=neon_boss_x; neon_ey=59; }
        else { neon_rand = neon_rand * 1664525u + 1013904223u; c = (neon_rand >> 16) % 8u;
        for (r = 3u; r > 0u; r--) if (neon_alive_at(c,r-1u)) {
            neon_enemy_bullet = 1; { int32_t x,y; neon_enemy_pos(c,r-1u,&x,&y); neon_ex=(int16_t)(x+8); neon_ey=(int16_t)(y+3); } break;
        } }
    }
}
static void neon_disc(int32_t cx, int32_t cy, int32_t radius, uint16_t color)
{
    static const uint8_t widths[17] = {0,5,8,10,12,13,14,15,16,15,14,13,12,10,8,5,0};
    uint32_t i;
    for (i=0; i<17u; i++) {
        int32_t half = (int32_t)widths[i] * radius / 16;
        int32_t y = cy - radius + (int32_t)(2u * i * (uint32_t)radius / 16u);
        int32_t h = (2 * radius) / 16;
        cv_rect(cx-half, y, half*2+1, h+1, color);
    }
}
static void arcade_start_card(const char *title, int x, const char *hint)
{
    cv_rect(16,78,208,82,RGB(8,3,42));
    cv_rect(16,78,208,2,RGB(0,220,235));
    cv_rect(16,158,208,2,RGB(0,220,235));
    cv_rect(16,78,2,82,RGB(0,220,235));
    cv_rect(222,78,2,82,RGB(0,220,235));
    cv_text(x,88,&FONT_S,title,RGB(255,54,190));
    cv_text(43,119,&FONT_TINY,hint,RGB(0,220,235));
    cv_text(70,140,&FONT_TINY,"PLAY TO START",C_WHITE);
}
static void arcade_start_icon(uint32_t kind)
{
    if(kind==0u){cv_rect(113,54,15,5,RGB(0,220,235));cv_rect(117,49,7,14,RGB(255,54,190));cv_rect(110,59,21,3,RGB(255,224,95));}
    else if(kind==1u){cv_rect(96,62,48,4,RGB(0,220,235));cv_rect(116,53,8,8,RGB(255,224,95));cv_rect(119,50,2,2,C_WHITE);cv_rect(133,45,6,6,RGB(255,54,190));}
    else if(kind==2u){cv_rect(106,46,12,12,RGB(0,220,235));cv_rect(118,46,12,12,RGB(255,54,190));cv_rect(118,58,12,12,RGB(255,224,95));cv_rect(130,58,12,12,RGB(75,244,196));}
    else if(kind==3u){cv_rect(107,48,12,18,RGB(0,220,235));cv_rect(102,53,22,8,RGB(0,220,235));cv_rect(124,44,12,18,RGB(255,54,190));cv_rect(119,49,22,8,RGB(255,54,190));cv_rect(139,55,10,10,RGB(255,224,95));}
    else {cv_rect(107,46,13,18,RGB(255,54,190));cv_rect(103,50,21,10,RGB(255,54,190));cv_rect(112,67,3,14,RGB(255,224,95));cv_rect(109,64,9,4,RGB(255,224,95));cv_rect(135,48,9,15,RGB(0,220,235));cv_rect(132,51,15,9,RGB(0,220,235));cv_rect(132,50,5,3,C_WHITE);}
}
static void arcade_start_screen(const char *title, int x, const char *hint, uint32_t kind, uint8_t *level)
{
    uint32_t pass,i;
    if(fm1_ms-arcade_level_at>=180u){if((fm1_in.buttons>>panel.btn[B_OCTDN])&1u&&*level>1u){(*level)--;arcade_level_at=fm1_ms;}else if((fm1_in.buttons>>panel.btn[B_OCTUP])&1u&&*level<5u){(*level)++;arcade_level_at=fm1_ms;}}
    for(pass=0;pass<2u;pass++) {
        cv_begin(240,120,RGB(4,2,29)); cv_oy=pass ? -120 : 0;
        cv_rect(0,0,240,240,RGB(4,2,29));
        for(i=0;i<24u;i++) cv_rect((int)((i*47u+13u)%238u),28+(int)((i*29u)%198u),2,2,i&1u?RGB(255,54,190):RGB(0,190,235));
        for(i=0;i<6u;i++) cv_line(120,170,(int)i*48-12,239,RGB(78,12,126));
        for(i=0;i<4u;i++) cv_line(0,177+(int)i*(int)i*5,239,177+(int)i*(int)i*5,RGB(48,13,99));
        cv_text(48,10,&FONT_S,"SYNTHGAME 0.0",RGB(0,220,235));
        arcade_start_icon(kind);
        arcade_start_card(title,x,hint);
        {char q[4];fmt_int(q,(int32_t)*level);cv_text(58,196,&FONT_TINY,"OCT- LEVEL",RGB(0,220,235));cv_text(137,196,&FONT_TINY,q,C_WHITE);cv_text(151,196,&FONT_TINY,"OCT+",RGB(0,220,235));cv_text(game_bonus_enabled?66:63,212,&FONT_TINY,game_bonus_enabled?"SEQ BONUS ON":"SEQ BONUS OFF",game_bonus_enabled?RGB(75,244,196):RGB(255,54,190));}
        cv_blit(0,pass*120u);
    }
    cv_oy=0;
}
static void neon_game_draw(void)
{
    uint32_t pass,c,r,i,sig,now=fm1_ms;
    neon_game_tick();
    sig = neon_alive ^ (uint32_t)neon_px*2654435761u ^ neon_score*2246822519u;
    sig ^= (uint32_t)neon_fx*3266489917u ^ neon_fy*668265263u ^ neon_lives*374761393u;
    sig ^= (uint32_t)neon_bullet*374761393u ^ (uint32_t)(neon_bx[0]+neon_by[0]+neon_bx[1]+neon_by[1]+neon_bx[2]+neon_by[2])*668265263u;
    sig ^= (uint32_t)neon_enemy_bullet * 3266489917u ^ (uint32_t)(neon_ex+neon_ey) * 2246822519u;
    sig ^= (uint32_t)neon_bonus*668265263u ^ (uint32_t)(neon_bonus_x+neon_bonus_y)*2654435761u ^ (uint32_t)neon_weapon*2246822519u ^ (uint32_t)neon_boss_hp*3266489917u ^ (uint32_t)neon_boss_x*374761393u;
    if (!ui.force && now - neon_draw_at < 75u) return;
    if (!ui.force && sig == neon_draw_sig) return;
    neon_draw_at = now;
    neon_draw_sig = sig;
    for (pass=0; pass<2u; pass++) {
        cv_begin(240,120,RGB(0,0,12)); cv_oy=pass ? -120 : 0;
        cv_rect(0,0,240,240,RGB(0,0,12));
        for (i=0;i<22u;i++) {
            int32_t sx=(int32_t)((i*47u+13u)%237u), sy=(int32_t)((i*71u+39u)%211u);
            cv_rect(sx,sy,1,1,(i%4u)==0u ? RGB(0,220,235) : C_WHITE);
        }
        cv_rect(0,0,240,38,RGB(0,4,20));
        cv_text(5,1,&FONT_TINY,"SCORE",RGB(0,220,235));
        cv_text(75,1,&FONT_TINY,"NEON ORBIT",RGB(0,220,235));
        cv_text(155,1,&FONT_TINY,"LIVES",RGB(0,220,235));
        cv_text(205,1,&FONT_TINY,"WAVE",RGB(0,220,235));
        cv_text(78,18,&FONT_TINY,"SHOT",RGB(0,220,235));
        { char b[2]={(char)('0'+neon_weapon),0}; cv_text(116,18,&FONT_TINY,b,C_WHITE); }
        if (neon_boss_hp) { cv_text(130,18,&FONT_TINY,"B",RGB(255,54,190)); for(i=0;i<5u;i++) if(i<neon_boss_hp) cv_rect(138+(int32_t)i*4,21,3,4,RGB(255,54,190)); }
        { char b[7]="000000"; uint32_t v=neon_score,d; for(d=0;d<6u;d++) { b[5u-d]=(char)('0'+(v%10u)); v/=10u; } cv_text(5,18,&FONT_TINY,b,C_WHITE); }
        for (i=0;i<neon_lives;i++) {
            int32_t x=165+(int32_t)i*11;
            cv_rect(x+3,20,3,2,C_WHITE); cv_rect(x+1,22,7,3,RGB(0,220,235));
            cv_rect(x,25,9,2,RGB(0,220,235)); cv_rect(x+2,27,2,2,RGB(0,220,235));
        }
        { char b[8]; fmt_int(b,(int32_t)neon_wave); cv_text(220,18,&FONT_TINY,b,C_WHITE); }
        cv_rect(0,36,240,2,RGB(0,220,235));
        if (neon_game_running && neon_boss_hp) { int32_t x=neon_boss_x; cv_rect(x-12,43,24,5,RGB(255,54,190)); cv_rect(x-9,48,18,7,RGB(75,244,196)); cv_rect(x-5,55,10,4,RGB(255,54,190)); cv_rect(x-7,49,3,3,RGB(0,0,12)); cv_rect(x+4,49,3,3,RGB(0,0,12)); }
        if (neon_game_running) for (r=0;r<3u;r++) for(c=0;c<8u;c++) if(neon_alive_at(c,r)) {
            int32_t x,y; neon_enemy_pos(c,r,&x,&y);
            uint16_t q=r==0u?RGB(255,54,190):r==1u?RGB(75,244,196):RGB(255,54,190);
            cv_rect(x+5,y,7,3,q); cv_rect(x+2,y+3,13,6,q);
            cv_rect(x+4,y+9,3,3,q); cv_rect(x+10,y+9,3,3,q);
            cv_rect(x+5,y+4,2,2,RGB(0,0,12)); cv_rect(x+11,y+4,2,2,RGB(0,0,12));
        }
        /* Small Saturn on the left, with a thin tilted ring. */
        cv_line(7,159,33,151,RGB(0,220,235));
        cv_line(8,161,34,153,RGB(0,110,190));
        neon_disc(20,155,6,RGB(0,190,235));
        cv_rect(18,152,3,2,RGB(0,65,145)); cv_rect(22,157,2,2,RGB(0,45,115));
        neon_disc(221,151,8,RGB(0,190,235));
        cv_rect(218,147,4,3,RGB(0,55,125)); cv_rect(223,153,3,3,RGB(0,45,110));
        if(neon_game_running) for(i=0;i<3u;i++) if(neon_bullet&(1u<<i)) cv_rect(neon_bx[i]-1,neon_by[i],3,9,RGB(255,54,190));
        if(neon_game_running && neon_bonus) { char b[2]={(char)(neon_bonus<4u?'0'+neon_bonus:neon_bonus==4u?'D':'T'),0}; uint16_t q=neon_bonus<4u?RGB(75,244,196):RGB(255,54,190); cv_rect(neon_bonus_x-6,neon_bonus_y-6,12,12,q); cv_text(neon_bonus_x-2,neon_bonus_y-5,&FONT_TINY,b,RGB(0,40,105)); }
        if(neon_game_running && neon_enemy_bullet) cv_rect(neon_ex-1,neon_ey,3,8,RGB(255,224,95));
        if(neon_game_running && neon_lives && (fm1_ms-neon_invuln_at>110u || !(fm1_ms/70u&1u))) {
            uint32_t n;
            /* One, two or three player ships, centered on the movement position. */
            for(n=0;n<neon_ships;n++) {
                int32_t x=neon_px+((int32_t)n*2-(int32_t)neon_ships+1)*8;
                cv_rect(x-2,207,4,3,C_WHITE); cv_rect(x-4,210,8,5,RGB(0,220,235));
                cv_rect(x-5,215,10,6,C_WHITE); cv_rect(x-3,215,6,6,RGB(0,40,105));
                cv_rect(x-2,221,4,6,RGB(255,54,190));
            }
        }
        if(neon_game_running && !neon_lives) {
            cv_rect(24,82,192,70,RGB(0,4,20)); cv_rect(24,82,192,2,RGB(255,54,190));
            cv_text(66,88,&FONT_L,"GAME OVER",C_WHITE);
            cv_text(28,128,&FONT_TINY,"PLAY RESTART  HOME EXIT",RGB(0,220,235));
        }
        if (!neon_game_running) {
            arcade_start_card("NEON ORBIT",64,"ARROWS MOVE  KEYS FIRE");
        } else if (neon_game_paused) {
            cv_rect(35,96,170,44,RGB(0,4,20)); cv_rect(35,96,170,2,RGB(255,54,190));
            cv_text(96,103,&FONT_TINY,"PAUSED",C_WHITE);
            cv_text(64,122,&FONT_TINY,"PLAY TO RESUME",RGB(0,220,235));
        }
        /* Continue the starfield to the bottom; the playfield now uses the full display. */
        
        cv_blit(0,pass*120u);
    }
    cv_oy=0;
}
/* ----------------------------------------------------- NEON MAZE --- */
#define MZ_W 15u
#define MZ_H 12u
static const char *const mz_map[MZ_H]={"###############","#.............#","#.###.###.###.#","#o#...#.#...#o#","#.###.#.#.###.#","#.....#.#.....#","###.#.....#.###","#...#.###.#...#","#.###.#.#.###.#","#.....#.#.....#","#.###.....###.#","###############"};
static uint8_t rush_game_active,rush_running,rush_paused,rush_lives,rush_px,rush_py,rush_dx,rush_dy,rush_wantx,rush_wanty,rush_gx[4],rush_gy[4],rush_gdir[4],rush_level;
static int8_t rush_bdx,rush_bdy;
static uint32_t rush_pellet[7],rush_score,rush_last,rush_draw_at,rush_sig,rush_power,rush_bonus_until,rush_slow_until;
static uint8_t rush_bonus,rush_bonus_x,rush_bonus_y;
static int rush_wall(int x,int y){return x<0||y<0||x>=(int)MZ_W||y>=(int)MZ_H||mz_map[y][x]=='#';}
static uint32_t rush_bit(uint32_t x,uint32_t y){return y*MZ_W+x;}
static int rush_eaten(uint32_t x,uint32_t y){uint32_t b=rush_bit(x,y);return (rush_pellet[b>>5]>>(b&31u))&1u;}
static void rush_eat(uint32_t x,uint32_t y){uint32_t b=rush_bit(x,y);rush_pellet[b>>5]|=1u<<(b&31u);}
uint32_t rush_game_is_active(void){return (uint32_t)(rush_game_active||neon_game_suppress_keys);}
static uint32_t rush_paddle_w(void){return fm1_ms<rush_bonus_until?70u:44u;}
static void rush_reset(void){rush_px=96;rush_dx=120;rush_py=164;rush_bdx=(int8_t)(3u+rush_level_sel);rush_bdy=-4;rush_lives=3;rush_score=0;rush_level=rush_level_sel;rush_power=0;rush_bonus=game_bonus_enabled?1u:0u;rush_bonus_x=120;rush_bonus_y=72;rush_bonus_until=0;rush_slow_until=0;rush_pellet[0]=0;rush_last=fm1_ms;}
static void rush_start(void){rush_game_active=1;rush_running=rush_paused=0;rush_reset();ui.menu=0;ui.home=0;ui.layer=LY_PLAY;ui.hold_kind=0;ui.force=1;}
static void rush_begin(void){rush_running=1;rush_paused=0;rush_reset();neon_audio_trigger(740,90);ui.force=1;}
static void rush_input(uint32_t pressed,uint32_t notes){uint32_t held=fm1_in.notes;(void)notes;panel_enc(EN_K1);panel_enc(EN_K2);panel_enc(EN_K3);panel_enc(EN_K4);panel_enc(EN_SELECT);panel_enc(EN_ALGO);panel_enc(EN_PRESET);if((pressed>>panel.btn[B_HOME])&1u){rush_game_active=rush_running=rush_paused=0;neon_game_suppress_keys=1;ui.force=1;return;}if(!rush_running||!rush_lives){if((pressed>>panel.btn[B_SEQ])&1u){game_bonus_enabled^=1u;ui.force=1;return;}if((pressed>>panel.btn[B_PLAY])&1u)rush_begin();return;}if((pressed>>panel.btn[B_PLAY])&1u){rush_paused=(uint8_t)!rush_paused;rush_last=fm1_ms;ui.force=1;return;}if(rush_paused)return;if(held&1u&&rush_px>=8u)rush_px-=5u;if(held&4u&&rush_px<=192u)rush_px+=5u;}
static void rush_tick(void){uint32_t now=fm1_ms,i;if(!rush_running||rush_paused||!rush_lives||now-rush_last<8u)return;rush_last=now;if(now<rush_slow_until&&((now>>3)&1u))return;{int x=(int)rush_dx+rush_bdx,y=(int)rush_py+rush_bdy;if(x<4||x>235)rush_bdx=-rush_bdx;if(y<42)rush_bdy=-rush_bdy;rush_dx=(uint8_t)((int)rush_dx+rush_bdx);rush_py=(uint8_t)((int)rush_py+rush_bdy);}if(rush_py>=195u&&rush_py<=204u&&rush_dx+5u>=rush_px&&rush_dx<=rush_px+rush_paddle_w()-1u){rush_bdy=-4;neon_audio_trigger(420,25);}if(rush_bonus){rush_bonus_y=(uint8_t)(rush_bonus_y+2u);if(rush_bonus_y>=194u&&rush_bonus_y<=207u&&rush_bonus_x>=rush_px&&rush_bonus_x<=rush_px+rush_paddle_w()){if(rush_bonus==1u)rush_bonus_until=now+8000u;else if(rush_bonus==2u)rush_slow_until=now+8000u;else if(rush_lives<5u)rush_lives++;rush_score+=50u;rush_bonus=0;neon_audio_trigger(1320,70);}else if(rush_bonus_y>230u)rush_bonus=0;}if(rush_py>230u){if(--rush_lives){rush_dx=120;rush_py=164;rush_bdx=4;rush_bdy=-4;}neon_audio_trigger(rush_lives?90:55,150);}for(i=0;i<30u;i++)if(!(rush_pellet[0]&(1u<<i))){uint32_t bx=i%6u,by=i/6u;int xx=12+(int)bx*36,yy=47+(int)by*13;if((int)rush_dx>=xx&&(int)rush_dx<=xx+31&&(int)rush_py>=yy&&(int)rush_py<=yy+10){rush_pellet[0]|=1u<<i;rush_bdy=-rush_bdy;rush_score+=20;if(game_bonus_enabled&&!rush_bonus&&((i+rush_score/20u+rush_level*3u)%11u)==0u){rush_bonus=(uint8_t)(1u+((i+rush_level)%3u));rush_bonus_x=(uint8_t)(xx+16);rush_bonus_y=(uint8_t)(yy+11);}neon_audio_trigger(880,32);}}if(rush_score>=rush_level*600u){rush_level++;rush_pellet[0]=0;neon_audio_trigger(1200,90);}}
static void rush_disc(int x,int y,uint16_t c){cv_rect(x-5,y-4,10,9,c);cv_rect(x-3,y-6,6,13,c);cv_rect(x+2,y-2,5,4,RGB(2,4,26));}
static void rush_draw(void){uint32_t pass,i,now=fm1_ms,sig;rush_tick();sig=rush_score^((uint32_t)rush_dx<<8)^((uint32_t)rush_py<<16)^((uint32_t)rush_lives<<25)^((uint32_t)rush_bonus<<4);if(!ui.force&&now-rush_draw_at<30u)return;if(!ui.force&&sig==rush_sig)return;rush_draw_at=now;rush_sig=sig;for(pass=0;pass<2u;pass++){cv_begin(240,120,RGB(2,4,26));cv_oy=pass?-120:0;cv_rect(0,0,240,240,RGB(5,2,31));cv_rect(0,0,240,36,RGB(8,3,42));cv_text(5,2,&FONT_TINY,"SCORE",RGB(0,220,235));cv_text(70,2,&FONT_TINY,"NEON BREAKER",RGB(255,54,190));cv_text(187,2,&FONT_TINY,"LIVES",RGB(0,220,235));{char q[8];fmt_int(q,(int32_t)rush_score);cv_text(5,18,&FONT_TINY,q,C_WHITE);fmt_int(q,(int32_t)rush_level);cv_text(152,18,&FONT_TINY,"LV",RGB(255,224,95));cv_text(168,18,&FONT_TINY,q,C_WHITE);}for(i=0;i<rush_lives;i++)rush_disc(210+(int)i*9,23,RGB(0,220,235));cv_rect(0,35,240,2,RGB(255,54,190));for(i=0;i<30u;i++)if(!(rush_pellet[0]&(1u<<i))){int xx=12+(int)(i%6u)*36,yy=47+(int)(i/6u)*13;uint16_t c=(i/6u)&1u?RGB(0,220,235):(i%3u?RGB(255,54,190):RGB(255,224,95));cv_rect(xx,yy,32,10,c);cv_rect(xx+2,yy+2,28,2,C_WHITE);}cv_rect(0,115,240,125,RGB(8,4,43));for(i=0;i<5u;i++)cv_line(0,130+(int)i*(int)i*5-(i==0?10:0),239,130+(int)i*(int)i*5-(i==0?10:0),RGB(80,8,115));cv_rect((int)rush_px,201,(int)rush_paddle_w(),7,RGB(0,220,235));cv_rect((int)rush_px+5,198,(int)rush_paddle_w()-10,3,RGB(255,54,190));cv_rect((int)rush_dx-3,(int)rush_py-3,7,7,RGB(255,224,95));cv_rect((int)rush_dx-1,(int)rush_py-1,3,3,C_WHITE);if(rush_bonus){char b[2]={(char)(rush_bonus==1u?'W':rush_bonus==2u?'S':'M'),0};cv_rect((int)rush_bonus_x-6,(int)rush_bonus_y-6,12,12,rush_bonus==1u?RGB(75,244,196):RGB(255,54,190));cv_text((int)rush_bonus_x-2,(int)rush_bonus_y-5,&FONT_TINY,b,RGB(0,20,60));}if(!rush_running){cv_rect(23,112,194,52,RGB(12,2,43));cv_rect(23,112,194,2,RGB(255,54,190));cv_text(39,120,&FONT_L,"BREAKER",C_WHITE);cv_text(37,146,&FONT_TINY,"PLAY START  KEYS MOVE",RGB(0,220,235));}else if(!rush_lives){cv_rect(28,112,184,52,RGB(12,2,43));cv_rect(28,112,184,2,RGB(255,54,190));cv_text(57,120,&FONT_L,"GAME OVER",C_WHITE);cv_text(31,146,&FONT_TINY,"PLAY RESTART HOME EXIT",RGB(0,220,235));}else if(rush_paused){cv_rect(35,120,170,40,RGB(12,2,43));cv_rect(35,120,170,2,RGB(255,54,190));cv_text(94,127,&FONT_TINY,"PAUSED",C_WHITE);cv_text(62,144,&FONT_TINY,"PLAY TO RESUME",RGB(0,220,235));}cv_blit(0,pass*120u);}cv_oy=0;}

/* --------------------------------------------------- NEON BLOCKS --- */
static uint8_t blocks_active,blocks_run,blocks_pause,blocks_x,blocks_y,blocks_shape,blocks_next,blocks_rot,blocks_board[140];
uint32_t blocks_game_is_active(void){return (uint32_t)(blocks_active||neon_game_suppress_keys);}
static uint32_t blocks_score,blocks_last,blocks_draw_at,blocks_bonus_until;
static uint8_t blocks_bonus;
static const int8_t blocks_piece[3][4][2]={{{0,0},{1,0},{0,1},{1,1}},{{0,0},{0,1},{0,2},{0,3}},{{0,0},{1,0},{2,0},{1,1}}};
/* Every orientation contains four distinct cells. */
static const int8_t blocks_turn[3][4][4][2]={
 {{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}},{{0,0},{1,0},{0,1},{1,1}}},
 {{{0,0},{0,1},{0,2},{0,3}},{{0,0},{1,0},{2,0},{3,0}},{{0,0},{0,1},{0,2},{0,3}},{{0,0},{1,0},{2,0},{3,0}}},
 {{{0,0},{1,0},{2,0},{1,1}},{{1,0},{0,1},{1,1},{1,2}},{{1,0},{0,1},{1,1},{2,1}},{{0,0},{0,1},{1,1},{0,2}}}
};
static int blocks_ox(uint32_t i){return blocks_turn[blocks_shape][blocks_rot][i][0];}
static int blocks_oy(uint32_t i){return blocks_turn[blocks_shape][blocks_rot][i][1];}
static uint32_t blocks_interval(void){uint32_t level=blocks_score/500u,base=level>=8u?220u:900u-level*85u;return blocks_bonus==1u&&fm1_ms<blocks_bonus_until?base*2u:base;}
static int blocks_hit(int x,int y){uint32_t i;for(i=0;i<4;i++){int xx=x+blocks_ox(i),yy=y+blocks_oy(i);if(xx<0||xx>=10||yy>=14||(yy>=0&&blocks_board[yy*10+xx]))return 1;}return 0;}
static void blocks_new(void){blocks_shape=blocks_next%3u;blocks_rot=0;blocks_next=(blocks_next+1u+(blocks_score>>5))%3u;blocks_x=4;blocks_y=0;if(blocks_hit(blocks_x,blocks_y))blocks_run=0;}
static void blocks_reset(void){uint32_t i;for(i=0;i<140;i++)blocks_board[i]=0;blocks_score=(blocks_level_sel-1u)*500u;blocks_bonus=game_bonus_enabled?1u:0u;blocks_bonus_until=fm1_ms+7000u;blocks_next=1;blocks_new();blocks_last=fm1_ms;}
static void blocks_start(void){blocks_active=1;blocks_run=blocks_pause=0;blocks_reset();ui.menu=ui.home=0;ui.layer=LY_PLAY;ui.hold_kind=0;ui.force=1;}
static void blocks_lock(void){uint32_t i,y,x,cleared=0;for(i=0;i<4;i++){x=blocks_x+blocks_ox(i);y=blocks_y+blocks_oy(i);if(y<14u)blocks_board[y*10u+x]=(uint8_t)(1u+blocks_shape);}for(y=0;y<14u;y++){uint32_t full=1;for(x=0;x<10u;x++)if(!blocks_board[y*10u+x])full=0;if(full){uint32_t yy;for(yy=y;yy;yy--)for(x=0;x<10u;x++)blocks_board[yy*10u+x]=blocks_board[(yy-1u)*10u+x];for(x=0;x<10u;x++)blocks_board[x]=0;blocks_score+=100;cleared++;neon_audio_trigger(1160,70);}}if(cleared&&game_bonus_enabled&&((blocks_score/100u+blocks_shape+cleared)%3u)==0u){blocks_bonus=(uint8_t)(1u+((blocks_score/100u)%3u));blocks_bonus_until=fm1_ms+(blocks_bonus==1u?7000u:1500u);if(blocks_bonus==2u){for(x=0;x<10u;x++)blocks_board[130u+x]=0;blocks_score+=100u;}else if(blocks_bonus==3u)blocks_score+=250u;neon_audio_trigger(1380,90);}neon_audio_trigger(520,45);blocks_new();}
static void blocks_input(uint32_t pressed,uint32_t notes){uint32_t held=fm1_in.notes;(void)notes;if((pressed>>panel.btn[B_HOME])&1u){blocks_active=blocks_run=blocks_pause=0;neon_game_suppress_keys=1;ui.force=1;return;}if(!blocks_run){if((pressed>>panel.btn[B_SEQ])&1u){game_bonus_enabled^=1u;ui.force=1;return;}if((pressed>>panel.btn[B_PLAY])&1u){blocks_run=1;blocks_pause=0;blocks_reset();}return;}if((pressed>>panel.btn[B_PLAY])&1u){blocks_pause=(uint8_t)!blocks_pause;return;}if(blocks_pause)return;if(held&1u&&!blocks_hit((int)blocks_x-1,blocks_y))blocks_x--;if(held&4u&&!blocks_hit(blocks_x+1,blocks_y))blocks_x++;if(notes&(1u<<1)){uint8_t s=blocks_shape,n=blocks_next,r=blocks_rot;blocks_shape=n%3u;blocks_next=s;blocks_rot=0;if(blocks_hit(blocks_x,blocks_y)){blocks_shape=s;blocks_next=n;blocks_rot=r;}else neon_audio_trigger(930,25);}if(notes&(1u<<26)){uint8_t r=blocks_rot;blocks_rot=(uint8_t)((blocks_rot+1u)&3u);if(blocks_hit(blocks_x,blocks_y))blocks_rot=r;}if(notes&(1u<<25)){neon_audio_trigger(760,28);while(!blocks_hit(blocks_x,blocks_y+1))blocks_y++;}}
static void blocks_tick(void){uint32_t now=fm1_ms;if(!blocks_run||blocks_pause||now-blocks_last<blocks_interval())return;blocks_last=now;if(!blocks_hit(blocks_x,blocks_y+1))blocks_y++;else blocks_lock();}
static void blocks_draw(void){uint32_t pass,x,y,i,now=fm1_ms;blocks_tick();if(blocks_bonus&&now>=blocks_bonus_until)blocks_bonus=0;if(!ui.force&&now-blocks_draw_at<50u)return;blocks_draw_at=now;for(pass=0;pass<2;pass++){cv_begin(240,120,RGB(2,4,26));cv_oy=pass?-120:0;cv_rect(0,0,240,240,RGB(5,2,30));for(i=0;i<14;i++)cv_rect(3+(int)((i*37u)%234u),28+(int)((i*53u)%200u),2,2,i&1?RGB(255,54,190):RGB(0,220,235));cv_rect(0,26,240,2,RGB(0,220,235));cv_text(8,1,&FONT_S,"NEON BLOCKS",RGB(255,54,190));cv_text(158,4,&FONT_TINY,"SCORE",RGB(0,220,235));{char q[8];fmt_int(q,(int32_t)blocks_score);cv_text(201,4,&FONT_TINY,q,C_WHITE);}cv_rect(4,29,156,189,RGB(255,54,190));cv_rect(7,32,150,183,RGB(3,8,38));for(x=0;x<11;x++)cv_line(7+(int)x*15,33,7+(int)x*15,214,RGB(18,16,74));for(y=0;y<15;y++)cv_line(7,33+(int)y*13,156,33+(int)y*13,RGB(18,16,74));cv_rect(7,214,150,3,RGB(0,220,235));for(y=0;y<14;y++)for(x=0;x<10;x++){uint8_t v=blocks_board[y*10+x];if(v)cv_rect(9+(int)x*15,34+(int)y*13,13,11,v==1?RGB(0,220,235):v==2?RGB(255,54,190):RGB(255,224,95));}for(i=0;i<4;i++){int xx=blocks_x+blocks_ox(i),yy=blocks_y+blocks_oy(i);if(xx>=0&&xx<10&&yy>=0&&yy<14)cv_rect(9+xx*15,34+yy*13,13,11,RGB(75,244,196));}cv_rect(172,38,62,70,RGB(17,5,58));cv_rect(172,38,62,2,RGB(0,220,235));cv_rect(172,106,62,2,RGB(0,220,235));cv_rect(172,38,2,70,RGB(0,220,235));cv_rect(232,38,2,70,RGB(0,220,235));cv_text(176,48,&FONT_TINY,"NEXT",RGB(255,224,95));for(i=0;i<4;i++)cv_rect(178+blocks_piece[blocks_next%3][i][0]*10,64+blocks_piece[blocks_next%3][i][1]*10,8,8,RGB(255,54,190));if(blocks_bonus){cv_rect(172,116,62,19,RGB(17,5,58));cv_text(178,120,&FONT_TINY,blocks_bonus==1u?"FREEZE":"+200",blocks_bonus==1u?RGB(75,244,196):RGB(255,224,95));}if(!blocks_run){cv_text(184,142,&FONT_TINY,"PLAY",C_WHITE);cv_text(180,159,&FONT_TINY,"START",C_WHITE);}else if(blocks_pause)cv_text(181,142,&FONT_TINY,"PAUSED",C_WHITE);cv_rect(172,176,62,32,RGB(17,5,58));cv_rect(172,212,62,2,RGB(255,54,190));cv_rect(172,174,62,2,RGB(255,54,190));cv_rect(172,174,2,40,RGB(255,54,190));cv_rect(232,174,2,40,RGB(255,54,190));cv_text(177,182,&FONT_TINY,"ROT END",RGB(255,224,95));cv_text(177,195,&FONT_TINY,"DROP -1",RGB(0,220,235));cv_blit(0,pass*120u);}cv_oy=0;}



/* ------------------------------------------------ SYNTH BUBBLES --- */
#define BU_N 72u
static uint8_t bu_active,bu_run,bu_pause,bu_over,bu_flying,bu_color,bu_next,bu_level,bu_misses,bu_round_id,bu_bonus_ready;
static uint8_t bu_cells[BU_N],bu_marks[BU_N],bu_queue[BU_N];
static int16_t bu_x,bu_y,bu_vx;
static int8_t bu_aim;
static uint32_t bu_score,bu_last,bu_draw_at,bu_rng,bu_move_at,bu_idle_at,bu_wide_until;
uint32_t bubbles_game_is_active(void){return bu_active||neon_game_suppress_keys;}
static uint32_t bu_random(void){bu_rng=bu_rng*1664525u+1013904223u;return bu_rng;}
static int bu_cx(int i){return 20+(i%8)*26+((i/8)&1)*13;}
static int bu_cy(int i){return 43+(i/8)*18;}
static int bu_adj(int a,int b){int dx=bu_cx(a)-bu_cx(b),dy=bu_cy(a)-bu_cy(b);return a!=b&&dx*dx+dy*dy<=26*26;}
static uint8_t bu_pick(uint8_t avoid){uint8_t have[5]={0,0,0,0,0};uint32_t i,n=0,k;for(i=0;i<BU_N;i++)if(bu_cells[i])have[bu_cells[i]]=1;for(i=1;i<=4;i++)if(have[i]&&i!=avoid)n++;if(!n)for(i=1;i<=4;i++)if(have[i])n++;if(!n)return (uint8_t)(1u+(bu_random()>>16)%4u);k=bu_random()%n;for(i=1;i<=4;i++)if(have[i]&&(i!=avoid||n==1u)){if(!k)return (uint8_t)i;k--;}return 1;}
static void bu_round(void){uint32_t i;for(i=0;i<BU_N;i++)bu_cells[i]=i<32u?(uint8_t)(1u+(bu_random()>>16)%4u):0;bu_flying=bu_over=bu_misses=bu_bonus_ready=0;bu_color=bu_pick(0);bu_next=game_bonus_enabled?5u:bu_pick(bu_color);bu_aim=0;bu_wide_until=0;bu_x=120;bu_y=214;bu_idle_at=fm1_ms;bu_round_id++;}
static void bu_reset(void){bu_pause=0;bu_rng=fm1_ms|1u;bu_level=bu_level_sel;bu_score=0;bu_round();bu_last=fm1_ms;}
static void bubbles_start(void){neon_game_active=0;rush_game_active=0;blocks_active=0;bu_active=1;bu_run=bu_pause=0;bu_reset();ui.menu=ui.home=0;ui.layer=LY_PLAY;ui.hold_kind=0;ui.force=1;}
/* Flood matching cluster, then drop cells disconnected from the ceiling. */
static void bu_resolve(uint32_t slot){uint32_t i,j,head=0,tail=0,n;uint8_t color=bu_cells[slot];for(i=0;i<BU_N;i++)bu_marks[i]=0;if(color==7u){for(i=0;i<BU_N;i++)if(bu_cells[i]&&bu_adj((int)slot,(int)i)){color=bu_cells[i];bu_cells[slot]=color;break;}}if(color==6u){bu_wide_until=fm1_ms+9000u;bu_cells[slot]=0;bu_score+=40u;bu_misses=0;neon_audio_trigger(1320,75);}else if(color==5u){int sx=bu_cx((int)slot),sy=bu_cy((int)slot);for(i=0;i<BU_N;i++)if(bu_cells[i]){int dx=bu_cx((int)i)-sx,dy=bu_cy((int)i)-sy;if(dx*dx+dy*dy<=2704){bu_cells[i]=0;tail++;}}bu_score+=tail*15u;bu_misses=0;neon_audio_trigger(1380,90);}else{bu_queue[tail++]=(uint8_t)slot;bu_marks[slot]=1;while(head<tail){i=bu_queue[head++];for(j=0;j<BU_N;j++)if(!bu_marks[j]&&bu_cells[j]==color&&bu_adj(i,j)){bu_marks[j]=1;bu_queue[tail++]=(uint8_t)j;}}if(tail>=3u){bu_score+=tail*10u;if(game_bonus_enabled&&((bu_score/10u+tail+bu_level)%3u)==0u)bu_bonus_ready=(uint8_t)(5u+((bu_score/10u+tail)%3u));for(i=0;i<BU_N;i++)if(bu_marks[i])bu_cells[i]=0;neon_audio_trigger(1100,85);for(i=0;i<BU_N;i++)bu_marks[i]=0;head=tail=0;for(i=0;i<8;i++)if(bu_cells[i]){bu_marks[i]=1;bu_queue[tail++]=(uint8_t)i;}while(head<tail){i=bu_queue[head++];for(j=0;j<BU_N;j++)if(!bu_marks[j]&&bu_cells[j]&&bu_adj(i,j)){bu_marks[j]=1;bu_queue[tail++]=(uint8_t)j;}}for(i=0;i<BU_N;i++)if(bu_cells[i]&&!bu_marks[i]){bu_cells[i]=0;bu_score+=20;}bu_misses=0;}else{bu_misses++;neon_audio_trigger(390,35);}}n=0;for(i=0;i<BU_N;i++)if(bu_cells[i])n++;if(!n){bu_level++;bu_round();neon_audio_trigger(1450,140);return;}if(bu_misses>=6u){for(i=64;i<72;i++)if(bu_cells[i])bu_over=1;for(i=BU_N;i-->8;)bu_cells[i]=bu_cells[i-8];for(i=0;i<8;i++)bu_cells[i]=(uint8_t)(1u+(bu_random()>>16)%4u);bu_misses=0;}for(i=64;i<72;i++)if(bu_cells[i])bu_over=1;}
static void bu_attach(int hit){int best=-1,dist=1000000;uint32_t i;uint8_t round=bu_round_id;for(i=0;i<BU_N;i++)if(!bu_cells[i]&&(hit<0?i<8u:bu_adj(hit,(int)i))){int dx=bu_cx(i)-bu_x,dy=bu_cy(i)-bu_y,d=dx*dx+dy*dy;if(d<dist){best=(int)i;dist=d;}}bu_flying=0;if(best<0){bu_over=1;return;}bu_cells[best]=bu_color;bu_resolve((uint32_t)best);if(round!=bu_round_id)return;bu_color=bu_next;bu_next=bu_pick(bu_color);if(bu_bonus_ready){bu_next=5u;bu_bonus_ready=0;}bu_idle_at=fm1_ms;}
static void bu_input(uint32_t pressed,uint32_t notes){uint32_t now=fm1_ms,h=fm1_in.notes;int lim=now<bu_wide_until?15:10;if((pressed>>panel.btn[B_HOME])&1u){bu_active=bu_run=0;neon_game_suppress_keys=1;ui.force=1;return;}if(!bu_run&&!bu_over&&(pressed>>panel.btn[B_SEQ])&1u){game_bonus_enabled^=1u;ui.force=1;return;}if((pressed>>panel.btn[B_PLAY])&1u){if(!bu_run||bu_over){bu_reset();bu_run=1;}else bu_pause=!bu_pause;bu_last=now;ui.force=1;return;}if(!bu_run||bu_pause||bu_over)return;if(now-bu_move_at>=35u){if((h&1u)&&bu_aim>-lim)bu_aim--;if((h&4u)&&bu_aim<lim)bu_aim++;bu_move_at=now;}if(!bu_flying&&(((pressed>>panel.btn[B_REC])&1u)||(notes&(1u<<26))||now-bu_idle_at>=8000u)){bu_flying=1;bu_x=120;bu_y=214;bu_vx=bu_aim;bu_last=now;bu_idle_at=now;neon_audio_trigger(780,30);}}
static void bu_tick(void){uint32_t now=fm1_ms,steps=0,i;if(!bu_run||bu_pause||bu_over)return;while(bu_flying&&now-bu_last>=20u&&steps++<8u){bu_last+=20;bu_x+=bu_vx;bu_y-=5;if(bu_x<12){bu_x=24-bu_x;bu_vx=-bu_vx;}if(bu_x>227){bu_x=454-bu_x;bu_vx=-bu_vx;}if(bu_y<=43){bu_attach(-1);break;}for(i=0;i<BU_N;i++)if(bu_cells[i]){int dx=bu_x-bu_cx(i),dy=bu_y-bu_cy(i);if(dx*dx+dy*dy<20*20){bu_attach((int)i);break;}}}}
static uint16_t bu_palette(uint8_t c){return c==1?RGB(0,225,245):c==2?RGB(255,35,185):c==3?RGB(180,65,255):c==4?RGB(255,220,60):c==5?RGB(0,0,0):RGB(75,244,196);}
static void bu_ball(int x,int y,uint8_t c){uint16_t z=bu_palette(c);if(c==5u){cv_rect(x-7,y-10,15,21,RGB(255,224,95));cv_rect(x-10,y-7,21,15,RGB(255,224,95));}cv_rect(x-6,y-9,13,19,z);cv_rect(x-9,y-6,19,13,z);cv_rect(x-5,y-6,5,3,C_WHITE);cv_rect(x+3,y+5,3,2,RGB(60,20,100));}
static void bu_skyline(void){static const uint8_t h[12]={18,31,23,42,27,35,20,46,29,38,24,33};uint32_t i;for(i=0;i<12;i++){int x=(int)i*21-4,y=166-(int)h[i];cv_rect(x,y,19,h[i],RGB(15,5,54));cv_rect(x,y,19,2,RGB(255,54,190));cv_rect(x+4,y+7,3,7,RGB(0,220,235));cv_rect(x+11,y+13,3,5,RGB(255,224,95));}}
static void bubbles_draw(void){uint32_t p,i,now=fm1_ms;bu_tick();if(!ui.force&&now-bu_draw_at<40u)return;bu_draw_at=now;for(p=0;p<2;p++){cv_begin(240,120,RGB(4,2,29));cv_oy=p?-120:0;cv_rect(0,0,240,240,RGB(4,2,29));bu_skyline();for(i=0;i<7;i++){int x=(int)i*40;cv_line(120,165,x,239,RGB(115,10,135));}for(i=0;i<5;i++)cv_line(0,169+(int)i*(int)i*4,239,169+(int)i*(int)i*4,RGB(80,12,120));cv_rect(0,0,240,29,RGB(10,3,35));cv_text(4,2,&FONT_S,"NEON BUBBLES",RGB(255,54,190));{char q[12];fmt_int(q,(int32_t)bu_score);cv_text(164,5,&FONT_TINY,"SCORE",RGB(0,220,235));cv_text(209,5,&FONT_TINY,q,C_WHITE);}cv_rect(0,28,240,2,RGB(0,220,235));for(i=0;i<BU_N;i++)if(bu_cells[i])bu_ball(bu_cx(i),bu_cy(i),bu_cells[i]);if(!bu_flying){int cx=120+bu_aim*3;cv_line2(120,218,cx,192,RGB(255,228,35));cv_line(120,215,cx,189,RGB(255,228,35));cv_line(120,221,cx,195,RGB(255,228,35));}cv_rect(104,222,32,6,RGB(0,220,235));bu_ball(bu_flying?bu_x:120,bu_flying?bu_y:214,bu_color);cv_text(6,218,&FONT_TINY,"NEXT",RGB(0,220,235));bu_ball(56,221,bu_next);if(bu_next==5u)cv_text(70,218,&FONT_TINY,"BOMB",RGB(75,244,196));if(!bu_run||bu_over||bu_pause){cv_rect(25,106,190,50,RGB(10,3,38));cv_rect(25,106,190,2,RGB(255,54,190));cv_text(bu_pause?(240-text_w(&FONT_S,"PAUSED"))/2:55,114,&FONT_S,bu_over?"GAME OVER":bu_pause?"PAUSED":"NEON BUBBLES",C_WHITE);cv_text(bu_pause?(240-text_w(&FONT_TINY,"PLAY TO RESUME"))/2:48,138,&FONT_TINY,bu_pause?"PLAY TO RESUME":"PLAY START REC FIRE",RGB(0,220,235));}cv_blit(0,p*120u);}cv_oy=0;}

/* ------------------------------------------------------ NEON PANG --- */
#define PANG_BALLS 7u
static uint8_t pang_active,pang_run,pang_pause,pang_over,pang_lives,pang_level,pang_level_sel=1,pang_ball_on[PANG_BALLS],pang_ball_sz[PANG_BALLS],pang_rope,pang_gravity_phase;
static int16_t pang_x,pang_bx[PANG_BALLS],pang_by[PANG_BALLS];
static int8_t pang_vx[PANG_BALLS],pang_vy[PANG_BALLS];
static uint32_t pang_score,pang_last,pang_draw_at,pang_rope_y,pang_fire_at,pang_move_at,pang_safe_until;
uint32_t pang_game_is_active(void){return pang_active||neon_game_suppress_keys;}
static int pang_radius(uint8_t s){return s==2u?10:s==1u?7:4;}
static void pang_add(int x,int y,int vx,int vy,uint8_t size){uint32_t i;for(i=0;i<PANG_BALLS;i++)if(!pang_ball_on[i]){pang_ball_on[i]=1;pang_bx[i]=(int16_t)x;pang_by[i]=(int16_t)y;pang_vx[i]=(int8_t)vx;pang_vy[i]=(int8_t)vy;pang_ball_sz[i]=size;return;}}
static void pang_stage(uint8_t level){uint32_t i;for(i=0;i<PANG_BALLS;i++)pang_ball_on[i]=0;pang_x=108;pang_rope=0;pang_gravity_phase=0;if(level==1u){pang_add(120,72,1,0,2);}else if(level==2u){pang_add(90,74,1,-2,2);pang_add(177,116,-1,-2,0);}else if(level==3u){pang_add(74,72,1,-2,1);pang_add(170,115,-1,-2,1);}else if(level==4u){pang_add(66,70,1,-2,1);pang_add(171,96,-1,-2,1);pang_add(122,148,1,-2,0);}else{pang_add(54,72,1,-2,1);pang_add(122,108,-1,-2,1);pang_add(190,146,1,-2,1);}pang_safe_until=fm1_ms+1400u;}
static void pang_reset(void){pang_score=0;pang_lives=3;pang_over=0;pang_level=pang_level_sel;pang_stage(pang_level);pang_last=fm1_ms;pang_move_at=0;}
static void pang_start(void){neon_game_active=0;rush_game_active=0;blocks_active=0;bu_active=0;pang_active=1;pang_run=pang_pause=0;pang_reset();ui.menu=ui.home=0;ui.layer=LY_PLAY;ui.hold_kind=0;ui.force=1;}
static void pang_split(uint32_t i){uint8_t s=pang_ball_sz[i];int x=pang_bx[i],y=pang_by[i];pang_ball_on[i]=0;pang_score+=(s==2u?100u:s==1u?200u:300u);neon_audio_trigger(s?1040:1420,55);if(s){pang_add(x,y,-2,-3,(uint8_t)(s-1u));pang_add(x,y,2,-3,(uint8_t)(s-1u));}}
static uint32_t pang_remaining(void){uint32_t i,n=0;for(i=0;i<PANG_BALLS;i++)n+=pang_ball_on[i]?1u:0u;return n;}
static void pang_new_ball(void){pang_stage(pang_level);}
static void pang_input(uint32_t pressed,uint32_t notes){uint32_t held=fm1_in.notes,now=fm1_ms;if((pressed>>panel.btn[B_HOME])&1u){pang_active=pang_run=pang_pause=0;neon_game_suppress_keys=1;ui.force=1;return;}if(!pang_run||pang_over){if((pressed>>panel.btn[B_SEQ])&1u){game_bonus_enabled^=1u;return;}if((pressed>>panel.btn[B_PLAY])&1u){pang_reset();pang_run=1;}return;}if((pressed>>panel.btn[B_PLAY])&1u){pang_pause=(uint8_t)!pang_pause;pang_last=now;ui.force=1;return;}if(pang_pause)return;if(now-pang_move_at>=22u){if((held&1u)&&pang_x>=5)pang_x-=4;if((held&4u)&&pang_x<=211)pang_x+=4;pang_move_at=now;}if((notes&(1u<<26))||(!pang_rope&&(held&(1u<<26)))){pang_rope=1;pang_rope_y=221;pang_fire_at=now;neon_audio_trigger(760,28);}}
static void pang_tick(void){uint32_t now=fm1_ms,i,steps=0;if(!pang_run||pang_pause||pang_over)return;while(now-pang_last>=45u&&steps++<8u){pang_last+=45u;if(pang_rope){if(pang_rope_y<=40u)pang_rope=0;else pang_rope_y-=8u;}if(++pang_gravity_phase>=3u){pang_gravity_phase=0;for(i=0;i<PANG_BALLS;i++)if(pang_ball_on[i]&&pang_vy[i]<1)pang_vy[i]++;}for(i=0;i<PANG_BALLS;i++)if(pang_ball_on[i]){int r=pang_radius(pang_ball_sz[i]);int x=pang_bx[i]+pang_vx[i],y=pang_by[i]+pang_vy[i];if(x-r<2||x+r>237){pang_vx[i]=-pang_vx[i];x=pang_bx[i]+pang_vx[i];}if(y-r<38){pang_vy[i]=1;y=38+r;}if(y+r>218){pang_vy[i]=(int8_t)(pang_ball_sz[i]==0u?-6:pang_ball_sz[i]==1u?-5:-4);y=218-r;}pang_bx[i]=(int16_t)x;pang_by[i]=(int16_t)y;if(pang_rope&&x+r>=pang_x+3&&x-r<=pang_x+21&&y+r>=(int)pang_rope_y&&y-r<=225){pang_rope=0;pang_split(i);continue;}if(now>=pang_safe_until&&y+r>=218&&y-r<=234&&x+r>=pang_x-3&&x-r<=pang_x+27){if(--pang_lives){neon_audio_trigger(105,130);pang_x=108;pang_rope=0;pang_safe_until=now+1400u;for(i=0;i<PANG_BALLS;i++)if(pang_ball_on[i])pang_by[i]=(int16_t)(55+(int)i*22);}else{pang_over=1;neon_audio_trigger(60,180);}break;}}if(!pang_remaining()){pang_level++;if(pang_level>5u)pang_level=5u;pang_new_ball();neon_audio_trigger(1500,100);}}}
static void pang_ball_draw(int x,int y,uint8_t s){int r=pang_radius(s);uint16_t c=s==2u?RGB(255,54,190):s==1u?RGB(0,220,235):RGB(255,224,95);cv_rect(x-r,y-r+3,r*2+1,r*2-5,c);cv_rect(x-r+3,y-r,r*2-5,r*2+1,c);cv_rect(x-r/2,y-r/2,r/2+2,r/2+1,C_WHITE);cv_rect(x+r/3,y+r/3,r/2,r/3,RGB(36,8,78));}
static void pang_draw(void){uint32_t pass,i,now=fm1_ms;pang_tick();if(!ui.force&&now-pang_draw_at<35u)return;pang_draw_at=now;for(pass=0;pass<2;pass++){cv_begin(240,120,RGB(4,2,29));cv_oy=pass?-120:0;cv_rect(0,0,240,240,RGB(4,2,29));for(i=0;i<25u;i++)cv_rect((int)((i*47u+9u)%238u),40+(int)((i*29u)%151u),2,2,i&1u?RGB(0,220,235):RGB(255,54,190));cv_rect(0,0,240,34,RGB(10,3,35));cv_text(5,1,&FONT_L,"NEON PANG",RGB(255,54,190));cv_text(164,3,&FONT_TINY,"LIVES",RGB(0,220,235));cv_text(164,17,&FONT_TINY,"SCORE",RGB(0,220,235));{char q[10];fmt_int(q,(int32_t)pang_lives);cv_text(205,3,&FONT_TINY,q,C_WHITE);fmt_int(q,(int32_t)pang_score);cv_text(205,17,&FONT_TINY,q,C_WHITE);}cv_rect(0,33,240,2,RGB(0,220,235));for(i=0;i<6u;i++)cv_line(120,183,(int)i*48-12,239,RGB(78,12,126));for(i=0;i<4u;i++)cv_line(0,188+(int)i*(int)i*4,239,188+(int)i*(int)i*4,RGB(48,13,99));for(i=0;i<PANG_BALLS;i++)if(pang_ball_on[i])pang_ball_draw(pang_bx[i],pang_by[i],pang_ball_sz[i]);if(pang_rope){cv_line(pang_x+12,225,pang_x+12,(int)pang_rope_y,RGB(255,224,95));cv_line(pang_x+10,(int)pang_rope_y+3,pang_x+12,(int)pang_rope_y,RGB(255,224,95));cv_line(pang_x+14,(int)pang_rope_y+3,pang_x+12,(int)pang_rope_y,RGB(255,224,95));}cv_rect(pang_x,226,25,6,RGB(0,220,235));cv_rect(pang_x+5,222,15,5,RGB(255,54,190));if(!pang_run||pang_over||pang_pause){cv_rect(22,103,196,55,RGB(10,3,38));cv_rect(22,103,196,2,RGB(255,54,190));cv_text(pang_pause?(240-text_w(&FONT_S,"PAUSED"))/2:67,111,&FONT_S,pang_over?"GAME OVER":pang_pause?"PAUSED":"NEON PANG",C_WHITE);cv_text(pang_pause?(240-text_w(&FONT_TINY,"PLAY TO RESUME"))/2:43,138,&FONT_TINY,pang_pause?"PLAY TO RESUME":"PLAY START KEY FIRE",RGB(0,220,235));}cv_blit(0,pass*120u);}cv_oy=0;}

UI_SMALL static void ui_draw(void)
{
    ui.frame++;
    pads_tick();
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (ft_bars) {                                      /* a free take closed: the loop it made */
        char m[24];
        uint32_t n = ft_bars;
        ft_bars = 0;
        if (n == 0xFFu) {
            ui_message("TAKE DROPPED");
        } else {
            str_cpy(m, n == 1u ? "LOOP 1 BAR " : n == 2u ? "LOOP 2 BARS " : "LOOP 4 BARS ", sizeof m);
            fmt_int(m + str_len(m), song.g[G_BPM]);
            str_cpy(m + str_len(m), " BPM", sizeof m - str_len(m));
            ui_message(m);
        }
    }
    if (er_flash) {                                     /* EDIT + key took something out */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
    if (neon_game_active) {
        if (!neon_game_running) arcade_start_screen("NEON ORBIT",64,"ARROWS MOVE  KEYS FIRE",0,&neon_level_sel); else neon_game_draw();
        ui.force = 0;
        return;
    }
    if (bu_active) { if(!bu_run&&!bu_over)arcade_start_screen("NEON BUBBLES",53,"AIM  FIRE  MATCH THREE",3,&bu_level_sel); else bubbles_draw(); ui.force=0; return; }
    if (pang_active) { if(!pang_run&&!pang_over)arcade_start_screen("NEON PANG",70,"KEYS MOVE  HARPOON",4,&pang_level_sel); else pang_draw(); ui.force=0; return; }
    if (blocks_active) {
        if(!blocks_run)arcade_start_screen("NEON BLOCKS",58,"MOVE  SWAP  ROTATE",2,&blocks_level_sel); else blocks_draw();
        ui.force = 0;
        return;
    }
    if (rush_game_active) {
        if(!rush_running)arcade_start_screen("NEON BREAKER",52,"MOVE THE PADDLE",1,&rush_level_sel); else rush_draw();
        ui.force = 0;
        return;
    }
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind)
            hold_screen_draw();
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if ((rec_wait || ft_on) && !ui.menu && !on_song_page()) {   /* armed / free take */
        rec_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && on_song_page()) {
        song_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (ui.menu) {
        draw_menu();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (!ui.home && cur_page()->scope == SC_TRK) {
        studio_tracks_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        drum_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    cursor_fix();
    if (!ui.home && (cur_page()->graph == GR_FX || cur_page()->graph == GR_SCALE ||
        cur_page()->graph == GR_ARP || cur_page()->graph == GR_STEPS || cur_page()->scope == SC_STEP)) {
        draw_capture = 1;
        draw_columns();
        draw_capture = 0;
        draw_card_screen();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (ui.force)
        draw_frame();
    felucca_dbg.stage = 3;
    draw_head();
    felucca_dbg.stage = 4;
    draw_columns();
    felucca_dbg.stage = 5;
    draw_graph();
    ui_timers();
    felucca_dbg.stage = 6;
    draw_foot();
    ui.force = 0;
}
