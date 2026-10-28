/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* User presets (editor protocol v2, cmds 16-21; the SAVE > USER page): 32
 * slots in two storage.c objects (OBJ_UPRESET0/1, 0xDC000..0xDFFFF), 16
 * records each. sloop-lab: no RAM mirror; the banks are read where they are, in the flash's plain
 * XIP window (storage.c st_xip), as the samples and the FM6 bank already are; a write stages the
 * bank in the project buffer (project.c up_stage) and goes through st_save (6,160 B of RAM back).
 * The host test (UP_HOST) keeps the RAM banks. A record:
 * engine, name, the instrument parameters, a 16-step pattern.
 *
 * A bank whose magic, record size or slot count differ reads as empty; so
 * does a record with another UP_VER. A record keeps np = the P_COUNT it was
 * stored with; when that differs it is mapped by count: its last 8 values
 * are P_E0..P_E7, the first np - 8 are P_LEVEL.. in order, missing ones take
 * their defaults. So common parameters may only be added just before P_E0
 * (else bump UP_VER).
 *
 * With -DUP_HOST (host test) only the part above #ifndef UP_HOST is built;
 * it needs nothing but core.h. */
#ifndef UI_SMALL                 /* (2.7) compiled for size: not the audio path (gfx.c UI_SMALL); the host tests: nothing */
#define UI_SMALL
#endif
#define UP_PER_BANK 16u
#define UP_PMAX 72u                              /* room for P_COUNT to grow */
#define UP_USED 0xA5u
/* Version 3 deliberately starts this 5-track build with an empty USER bank.
 * Version 1/2 records came from the faulty preset-browser experiment: they
 * did not retain the exact factory preset they copied.  New records carry
 * that source in the spare final parameter word, so an original and its edit
 * can always be paired without guessing from their displayed names. */
#define UP_VER 3u
#define UP_BANK_MAGIC 0x31425055u                /* "UPB1" */
typedef struct {
    uint8_t used, ver, engine, np;               /* UP_USED, UP_VER, engine, P_COUNT when stored */
    char name[12];                               /* ASCII 32..126, 0-padded (no 0 when 12 long) */
    int16_t p[UP_PMAX];
    uint8_t note[16], flags[16];                 /* note 0 = rest; flags 1 accent, 2 slide, 4 tie */
} up_rec_t;
typedef struct {
    uint32_t magic;
    uint16_t rsize, nslot;
    up_rec_t r[UP_PER_BANK];
} up_bank_t;
_Static_assert(sizeof(up_rec_t) == 192, "user preset record layout");
/* One final word is metadata, never a synth parameter.  Keep this assertion
 * strict: silently letting P_COUNT grow into that word would corrupt both the
 * last sound parameter and the factory-source identity of a saved copy. */
_Static_assert(P_COUNT < UP_PMAX && P_COUNT < 128, "user preset record: P_COUNT needs metadata word");
#ifdef UP_HOST
static up_bank_t up_bank[UP_SLOTS / UP_PER_BANK];
UI_SMALL static up_rec_t *up_rec(uint32_t k) { return &up_bank[k / UP_PER_BANK].r[k % UP_PER_BANK]; }
#else
static const up_bank_t *up_bank_xip[UP_SLOTS / UP_PER_BANK];   /* each bank's current copy in the window, 0 = none */
static const up_rec_t up_rec_none;                              /* an empty slot (up_valid says no) */
UI_SMALL static const up_rec_t *up_rec(uint32_t k)
{
    const up_bank_t *bk = up_bank_xip[(k / UP_PER_BANK) % NELEM(up_bank_xip)];
    return bk ? &bk->r[k % UP_PER_BANK] : &up_rec_none;
}
static void *up_stage(void);                                    /* project.c: the project buffer (main loop) */
#endif

UI_SMALL static int up_valid(const up_rec_t *r)
{
    return r->used == UP_USED && r->ver == UP_VER && r->engine < NENGINES && r->np >= 8u && r->np <= UP_PMAX &&
           r->name[0];
}

UI_SMALL static int up_used(uint32_t k) { return k < UP_SLOTS && up_valid(up_rec(k)); }

UI_SMALL static int up_bank_ok(const up_bank_t *bk, int len)   /* a bank of this layout, whole */
{
    return len == (int)sizeof *bk && bk->magic == UP_BANK_MAGIC && bk->rsize == sizeof(up_rec_t) &&
           bk->nslot == UP_PER_BANK;
}
#ifdef UP_HOST
UI_SMALL static void up_bank_check(uint32_t b, int len)  /* after loading bank b (len bytes, -1 = none): wrong shape -> empty */
{
    up_bank_t *bk = &up_bank[b];
    if (!up_bank_ok(bk, len))
        memset(bk, 0, sizeof *bk);
}
#else
UI_SMALL static void up_bank_find(uint32_t b)            /* bank b's current copy in the XIP window, or none (after a save too) */
{
    const void *p = 0;
    uint32_t len = 0;
#if FELUCCA_FLASH
    p = flash_ok ? st_xip(OBJ_UPRESET0 + b, &len) : 0;
#endif
    up_bank_xip[b % NELEM(up_bank_xip)] = p && up_bank_ok((const up_bank_t *)p, (int)len) ? (const up_bank_t *)p : 0;
}
#endif

/* the record's values in today's P_* order (mapped by count, see above); def = the defaults */
UI_SMALL static void up_params(const up_rec_t *r, int16_t *out, const int16_t *def)
{
    uint32_t i, nc = r->np - 8u;
    for (i = 0; i < P_E0; i++)
        out[i] = i < nc ? r->p[i] : def[i];
    for (i = 0; i < 8u; i++)
        out[P_E0 + i] = r->p[nc + i];
}

UI_SMALL static int up_name_ok(const uint8_t *s, uint32_t n)   /* 1..12 printable ASCII */
{
    uint32_t i;
    if (!n || n > 12u)
        return 0;
    for (i = 0; i < n; i++)
        if (s[i] < 32u || s[i] > 126u)
            return 0;
    return 1;
}

UI_SMALL static void up_name(uint32_t k, char *b)       /* upper case, 0-terminated: b holds 13 */
{
    const up_rec_t *r = up_rec(k);
    uint32_t i;
    for (i = 0; i < 12u && r->name[i]; i++)
        b[i] = r->name[i] >= 'a' && r->name[i] <= 'z' ? (char)(r->name[i] - 32) : r->name[i];
    b[i] = 0;
}

UI_SMALL static void up_pat_norm(uint8_t *note, uint8_t *flags)   /* tie: no note; rest: no flags */
{
    *note &= 127u;
    if (*flags & 4u) {
        *note = 0;
        *flags = 4;
    } else {
        *flags = *note ? (uint8_t)(*flags & (SF_ACCENT | SF_SLIDE)) : 0u;
    }
}

UI_SMALL static void up_pat_from(up_rec_t *r, const step_t *st)   /* the first 16 steps -> the pattern */
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        r->note[i] = st[i].time == ST_NOTE && st[i].n ? st[i].note[0] : 0u;
        r->flags[i] = st[i].time == ST_TIE ? 4u : st[i].flags;
        up_pat_norm(&r->note[i], &r->flags[i]);
    }
}

UI_SMALL static int up_pat_empty(const up_rec_t *r)
{
    uint32_t i;
    for (i = 0; i < 16u; i++)
        if (r->note[i])
            return 0;
    return 1;
}

/* UP_PUT arguments: slot, engine, name, P_COUNT x v14, 16 x (note, flags) -> *r (values not yet
 * clamped); 0 ok, 1 bad arguments. *slot gets the slot byte when there is one. */
UI_SMALL static int up_parse(const uint8_t *a, uint32_t na, up_rec_t *r, uint32_t *slot)
{
    uint32_t i, n, k;
    if (na < 3u)
        return 1;
    *slot = a[0];
    for (n = 0; 2u + n < na && a[2 + n]; n++)
        ;
    k = 3u + n;                                  /* after the name's 0 */
    if (a[0] >= UP_SLOTS || a[1] >= NENGINES || 2u + n >= na || !up_name_ok(a + 2, n) ||
        na < k + 2u * P_COUNT + 32u)
        return 1;
    memset(r, 0, sizeof *r);
    r->used = UP_USED;
    r->ver = UP_VER;
    r->engine = a[1];
    r->np = P_COUNT;
    for (i = 0; i < n; i++)
        r->name[i] = (char)a[2 + i];
    for (i = 0; i < P_COUNT; i++, k += 2u)
        r->p[i] = (int16_t)((int32_t)((a[k] & 127u) | (a[k + 1] & 127u) << 7) - 8192);
    for (i = 0; i < 16u; i++, k += 2u) {
        r->note[i] = a[k];
        r->flags[i] = a[k + 1];
        up_pat_norm(&r->note[i], &r->flags[i]);
    }
    return 0;
}

#ifndef UP_HOST
/* Factory BANK entry + 1 from which this user copy was made; 0 = no matching
 * factory entry.  This is deliberately the first spare word, rather than a
 * magic constant: the assertion above makes it impossible for a future sound
 * parameter to overwrite the identity used to restore the original. */
#define UP_SOURCE_PARAM P_COUNT
UI_SMALL static uint32_t up_source_bank(uint32_t k)
{
    uint32_t v = up_used(k) ? (uint16_t)up_rec(k)->p[UP_SOURCE_PARAM] : 0u;
    return v && v <= NBANK ? v - 1u : NBANK;
}

UI_SMALL static const param_desc_t *up_desc(uint32_t e, uint32_t i)
{
    return i >= P_E0 ? &ENGINES[e]->edit[i - P_E0] : &TP[i];
}

UI_SMALL static void up_values(const up_rec_t *r, int16_t *v)   /* mapped and clamped for its engine */
{
    int16_t def[P_COUNT];
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        def[i] = up_desc(r->engine, i)->def;
    up_params(r, v, def);
    for (i = 0; i < P_COUNT; i++)
        v[i] = (int16_t)clamp(v[i], up_desc(r->engine, i)->min, up_desc(r->engine, i)->max);
}

UI_SMALL static void up_boot(void)                      /* persist_boot: the banks, where they are in the flash */
{
    uint32_t b;
    for (b = 0; b < UP_SLOTS / UP_PER_BANK; b++)
        up_bank_find(b);
}

/* record k = *r (0: erase), then the bank to flash: 0 ok, 2 flash error, 3 no flash (nothing kept: there is no
 * RAM mirror). The bank is staged in the project buffer: its current copy, or an empty one, with the record put */
UI_SMALL static int up_put(uint32_t k, const up_rec_t *r)
{
    uint32_t b = (k / UP_PER_BANK) % NELEM(up_bank_xip), i;
    up_bank_t *bk = (up_bank_t *)up_stage();
    int rc = 3;
    if (up_bank_xip[b])
        memcpy(bk, up_bank_xip[b], sizeof *bk);
    else
        memset(bk, 0, sizeof *bk);
    bk->magic = UP_BANK_MAGIC;
    bk->rsize = sizeof(up_rec_t);
    bk->nslot = UP_PER_BANK;
    if (r)
        bk->r[k % UP_PER_BANK] = *r;
    else
        memset(&bk->r[k % UP_PER_BANK], 0, sizeof(up_rec_t));
    if (!r)
        for (i = 0; i < NTRK; i++)
            if (trk[i].user == k + 1u)
                trk[i].user = 0;
    up_gen++;
#if FELUCCA_FLASH
    if (flash_ok)
        rc = st_save(OBJ_UPRESET0 + b, bk, sizeof *bk) ? 2 : 0;
#endif
    up_bank_find(b);
    return rc;
}

UI_SMALL static void up_slot_label(char *b, uint32_t k)  /* "U07" */
{
    b[0] = 'U';
    b[1] = (char)('0' + (k + 1u) / 10u);
    b[2] = (char)('0' + (k + 1u) % 10u);
    b[3] = 0;
}

/* the selected part's sound -> slot k; name 0 or "": original preset name.
 * 1 = the drum track is selected (it has no sound to store) */
UI_SMALL static int up_store(uint32_t k, const char *name)
{
    up_rec_t r;
    uint32_t i, source = NBANK;
    if (is_drum(TSEL))
        return 1;
    memset(&r, 0, sizeof r);
    r.used = UP_USED;
    r.ver = UP_VER;
    r.engine = TSEL->eng_req;
    r.np = P_COUNT;
    if (TSEL->user)
        source = up_source_bank(TSEL->user - 1u);       /* editing an edit keeps its original */
    else {
        if (!bank_ready)
            bank_resolve();
        for (i = 0; i < NBANK; i++)
            if (BANK[i].e == TSEL->eng_req && bank_pi[i] == TSEL->preset) {
                source = i;
                break;
            }
    }
    /* A quick copy is allowed only when it has an exact factory source.  An
     * unpaired record would be impossible to place beside, or safely restore
     * to, its original and was the route behind the apparent replacement of
     * presets in earlier experimental builds. */
    if (source >= NBANK)
        return 1;
    r.p[UP_SOURCE_PARAM] = (int16_t)(source + 1u);
    if (name && name[0]) {
        for (i = 0; i < 12u && name[i]; i++)
            r.name[i] = name[i];
    } else {
        char b[16];
        const char *src = up_source_name(TSEL);
        if (TSEL->user)
            up_name(TSEL->user - 1u, b);
        else
            str_cpy(b, src ? src : ENGINES[TSEL->eng_req]->name, sizeof b);
        for (i = 0; i < 12u && b[i]; i++)
            r.name[i] = b[i];
    }
    for (i = 0; i < P_COUNT; i++)
        r.p[i] = TSEL->p[i];
    up_pat_from(&r, TSEL->step);
    return up_put(k, &r);
}

/* slot k -> the selected part's sound: engine and every parameter except its mix (LEVEL,
 * PAN, MUTE: the TRACKS faders) and its pattern parameters (param_kept). LIVE: the pattern
 * stored in the record is not loaded: changing the sound never changes the sequence.
 * 0 ok, 1 empty (or the drum track is selected) */
UI_SMALL static int up_load_to(track_t *t, uint32_t k)       /* user preset k into track t (2.6: any track, MIDI PC) */
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
    loudness_loaded(t);
    if (t == TSEL)
        sync_reload = 1;
    ui.force = 1;
    return 0;
}
UI_SMALL static int up_load(uint32_t k) { return up_load_to(TSEL, k); }   /* into the selected track */

UI_SMALL static uint32_t up_count(void)                 /* used slots */
{
    uint32_t k, n = 0;
    for (k = 0; k < UP_SLOTS; k++)
        n += (uint32_t)up_used(k);
    return n;
}

UI_SMALL static uint32_t up_nth(uint32_t n)             /* slot of the n-th used one (n < up_count()) */
{
    uint32_t k;
    for (k = 0; k < UP_SLOTS; k++)
        if (up_used(k) && !n--)
            return k;
    return 0;
}

UI_SMALL static uint32_t up_rank(uint32_t slot)         /* used slots before it */
{
    uint32_t k, n = 0;
    for (k = 0; k < slot && k < UP_SLOTS; k++)
        n += (uint32_t)up_used(k);
    return n;
}

UI_SMALL static uint32_t up_engine(uint32_t slot)
{
    return up_used(slot) ? up_rec(slot)->engine : NENGINES;
}

/* SAVE > USER page actions, with the message in the top bar */
UI_SMALL static void up_ui(uint32_t op, uint32_t k)     /* 0 load, 1 erase, 2 save */
{
    char l[4];
    int rc;
    up_slot_label(l, k);
    if (op != 1u && is_drum(TSEL)) {
        ui_message("DRUM TRACK: NO SOUND");
        return;
    }
    if (op < 2u && !up_used(k)) {
        ui_message("EMPTY SLOT");
        return;
    }
    if (op == 0u) {
        up_load(k);
        ui_say("LOADED ", l);
        return;
    }
    if (song.playing || transport_req) {               /* a flash erase stops the audio ~50 ms */
        ui_message("STOP BEFORE SAVE");
        return;
    }
    rc = op == 1u ? up_put(k, 0) : up_store(k, 0);
    if (rc == 3)
        ui_message("NO FLASH");                        /* (no RAM mirror to keep it in) */
    else if (rc)
        ui_message(op == 1u ? "ERASE ERROR" : "SAVE ERROR");
    else {
        ui_say(op == 1u ? "ERASED " : "SAVED ", l);
    }
    ui.force = 1;
}
#endif
