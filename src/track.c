/*
 * track.c - Trassani hosil qilish va unda joy topish.
 */
#include "track.h"

#include <math.h>
#include <string.h>

Track TRACK;

/* Xaritalar: har biri o'z tayanch nuqtalari bilan. Birinchi nuqta -
 * uzun to'g'ri yo'lning o'rtasida, start shu yerda. */
static const V2 CTRL0[] = {   /* Yashil vodiy: S-burilishli keng halqa */
    { 0, 60 }, { 0, 160 }, { 10, 300 }, { 70, 390 }, { 170, 420 }, { 270, 390 },
    { 320, 310 }, { 300, 220 }, { 230, 170 }, { 200, 100 }, { 240, 30 }, { 330, 0 },
    { 400, -70 }, { 400, -180 }, { 330, -260 }, { 210, -280 }, { 110, -240 }, { 30, -170 }, { 0, -60 },
};
static const V2 CTRL1[] = {   /* Cho'l: uzun to'g'ri yo'llar va keskin qaytishlar */
    { 0, 120 }, { 0, 320 }, { 50, 380 }, { 190, 380 }, { 250, 320 }, { 250, 230 }, { 320, 170 },
    { 430, 190 }, { 490, 270 }, { 570, 250 }, { 590, 110 }, { 530, -30 }, { 390, -70 },
    { 280, -150 }, { 160, -170 }, { 60, -130 }, { 0, -20 },
};
static const V2 CTRL2[] = {   /* Qorli tog': ilonizi burilishlar */
    { 0, 60 }, { 0, 180 }, { -60, 260 }, { -50, 370 }, { 50, 420 }, { 150, 360 }, { 130, 260 },
    { 200, 190 }, { 310, 220 }, { 350, 320 }, { 450, 330 }, { 500, 220 }, { 440, 110 },
    { 470, -10 }, { 390, -120 }, { 250, -110 }, { 150, -170 }, { 50, -130 }, { 0, -40 },
};
static const V2 CTRL3[] = {   /* Tungi shahar: to'g'ri burchakli ko'chalar (yumaloqlangan burchaklar) */
    { 0, 100 }, { 0, 200 }, { 25, 270 }, { 80, 305 }, { 200, 305 }, { 250, 280 }, { 270, 230 },
    { 275, 170 }, { 305, 125 }, { 360, 112 }, { 420, 112 }, { 465, 85 }, { 480, 30 },
    { 480, -120 }, { 460, -175 }, { 410, -200 }, { 250, -200 }, { 200, -180 }, { 180, -130 },
    { 172, -80 }, { 140, -45 }, { 85, -35 }, { 30, -20 }, { 3, 25 },
};

static const struct { const V2 *p; int n; const char *name; } MAPS[MAP_COUNT] = {
    { CTRL0, (int)(sizeof(CTRL0) / sizeof(V2)), "Yashil vodiy" },
    { CTRL1, (int)(sizeof(CTRL1) / sizeof(V2)), "Cho'l" },
    { CTRL2, (int)(sizeof(CTRL2) / sizeof(V2)), "Qorli tog'" },
    { CTRL3, (int)(sizeof(CTRL3) / sizeof(V2)), "Tungi shahar" },
};

const char *track_map_name(int map) { return (map >= 0 && map < MAP_COUNT) ? MAPS[map].name : "?"; }

static V2 catmull(V2 a, V2 b, V2 c, V2 d, float t)
{
    float t2 = t * t, t3 = t2 * t;
    V2 r;
    r.x = 0.5f * (2 * b.x + (-a.x + c.x) * t + (2 * a.x - 5 * b.x + 4 * c.x - d.x) * t2 + (-a.x + 3 * b.x - 3 * c.x + d.x) * t3);
    r.z = 0.5f * (2 * b.z + (-a.z + c.z) * t + (2 * a.z - 5 * b.z + 4 * c.z - d.z) * t2 + (-a.z + 3 * b.z - 3 * c.z + d.z) * t3);
    return r;
}

static float dist2(V2 a, V2 b) { float dx = a.x - b.x, dz = a.z - b.z; return dx * dx + dz * dz; }

static unsigned int tr_rng = 2024u;
static float tr_rand(void) { tr_rng = tr_rng * 1664525u + 1013904223u; return (float)(tr_rng >> 8) / 16777216.0f; }

void track_init(int map)
{
    if (map < 0 || map >= MAP_COUNT) map = 0;
    const V2 *CTRL = MAPS[map].p;
    const int CTRL_N = MAPS[map].n;
    /* 1) egri chiziqni zich namunalash */
    static V2 dense[32 * 64];
    int nd = 0;
    for (int i = 0; i < CTRL_N; i++) {
        V2 a = CTRL[(i - 1 + CTRL_N) % CTRL_N], b = CTRL[i], c = CTRL[(i + 1) % CTRL_N], d = CTRL[(i + 2) % CTRL_N];
        for (int k = 0; k < 64; k++) dense[nd++] = catmull(a, b, c, d, k / 64.0f);
    }
    /* 2) teng oraliq (~3 m) bilan qayta namunalash */
    float total = 0;
    for (int i = 0; i < nd; i++) total += sqrtf(dist2(dense[i], dense[(i + 1) % nd]));
    int n = (int)(total / 3.0f);
    if (n > TRACK_MAX) n = TRACK_MAX;
    float step = total / n;
    Track *T = &TRACK;
    memset(T, 0, sizeof(*T));
    T->map_id = map;
    T->n = n;
    T->length = total;
    T->spacing = step;
    int j = 0;
    float acc = 0, seg = sqrtf(dist2(dense[0], dense[1]));
    for (int i = 0; i < n; i++) {
        float want = i * step;
        while (acc + seg < want && j < nd - 1) { acc += seg; j++; seg = sqrtf(dist2(dense[j], dense[(j + 1) % nd])); }
        float f = seg > 0 ? (want - acc) / seg : 0;
        V2 a = dense[j], b = dense[(j + 1) % nd];
        T->p[i] = (V2){ a.x + (b.x - a.x) * f, a.z + (b.z - a.z) * f };
        T->s[i] = want;
    }
    T->min_x = T->min_z = 1e9f;
    T->max_x = T->max_z = -1e9f;
    for (int i = 0; i < n; i++) {
        V2 a = T->p[(i - 1 + n) % n], b = T->p[(i + 1) % n];
        float dx = b.x - a.x, dz = b.z - a.z, l = sqrtf(dx * dx + dz * dz);
        T->t[i] = (V2){ dx / l, dz / l };
        T->nrm[i] = (V2){ T->t[i].z, -T->t[i].x };
        if (T->p[i].x < T->min_x) T->min_x = T->p[i].x;
        if (T->p[i].x > T->max_x) T->max_x = T->p[i].x;
        if (T->p[i].z < T->min_z) T->min_z = T->p[i].z;
        if (T->p[i].z > T->max_z) T->max_z = T->p[i].z;
    }
    for (int i = 0; i < n; i++) {
        V2 a = T->t[i], b = T->t[(i + 3) % n];
        float cr = a.x * b.z - a.z * b.x, dt = a.x * b.x + a.z * b.z;
        T->turn[i] = atan2f(cr, dt) * (10.0f / (3 * step));
    }

    /* 3) bezaklar (daraxt, kaktus, bino) - yo'ldan uzoqroqda */
    tr_rng = 2024u + (unsigned)map * 977u;
    int tries = 0;
    while (T->tree_count < TRACK_TREES && tries < 20000) {
        tries++;
        V2 q = { T->min_x - 140 + tr_rand() * (T->max_x - T->min_x + 280), T->min_z - 140 + tr_rand() * (T->max_z - T->min_z + 280) };
        float best = 1e9f;
        for (int i = 0; i < n; i += 2) { float d = dist2(q, T->p[i]); if (d < best) best = d; }
        float need = TRACK_W / 2 + 9;
        if (best < need * need) continue;
        T->trees[T->tree_count] = q;
        T->tree_size[T->tree_count] = 0.8f + tr_rand() * 0.8f;
        T->tree_clear[T->tree_count] = sqrtf(best) - (TRACK_W / 2 + 7.5f);
        T->tree_count++;
    }
}

int track_nearest(float x, float z, int hint, float *lateral, float *s_out)
{
    const Track *T = &TRACK;
    V2 q = { x, z };
    int best = 0;
    float bd = 1e18f;
    if (hint < 0) {
        for (int i = 0; i < T->n; i++) { float d = dist2(q, T->p[i]); if (d < bd) { bd = d; best = i; } }
    } else {
        for (int k = -40; k <= 40; k++) {
            int i = ((hint + k) % T->n + T->n) % T->n;
            float d = dist2(q, T->p[i]);
            if (d < bd) { bd = d; best = i; }
        }
    }
    V2 c = T->p[best], t = T->t[best], nr = T->nrm[best];
    float dx = x - c.x, dz = z - c.z;
    float along = dx * t.x + dz * t.z;
    if (lateral) *lateral = dx * nr.x + dz * nr.z;
    if (s_out) {
        float s = T->s[best] + along;
        while (s < 0) s += T->length;
        while (s >= T->length) s -= T->length;
        *s_out = s;
    }
    return best;
}

int track_index_at(float s)
{
    const Track *T = &TRACK;
    while (s < 0) s += T->length;
    while (s >= T->length) s -= T->length;
    int i = (int)(s / T->spacing);
    return i >= T->n ? T->n - 1 : i;
}

V2 track_point_at(float s, float lateral)
{
    const Track *T = &TRACK;
    int i = track_index_at(s);
    int k = (i + 1) % T->n;
    float f = (s - (float)i * T->spacing) / T->spacing;
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    V2 c = { T->p[i].x + (T->p[k].x - T->p[i].x) * f, T->p[i].z + (T->p[k].z - T->p[i].z) * f };
    return (V2){ c.x + T->nrm[i].x * lateral, c.z + T->nrm[i].z * lateral };
}

float track_heading_at(int i) { return atan2f(TRACK.t[i].x, TRACK.t[i].z); }

/* trassaning bir-biriga eng yaqin, lekin qo'shni bo'lmagan qismlari orasidagi masofa */
float track_min_separation(void)
{
    const Track *T = &TRACK;
    float best = 1e18f;
    int skip = (int)(80.0f / T->spacing);
    for (int i = 0; i < T->n; i += 2)
        for (int j = i + skip; j < T->n; j += 2) {
            int gap = j - i;
            if (gap > T->n - skip) continue;
            float d = dist2(T->p[i], T->p[j]);
            if (d < best) best = d;
        }
    return sqrtf(best);
}
