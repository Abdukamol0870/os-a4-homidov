/*
 * parking.c - "Parkovka" rejimi: 20 ta daraja, mashina fizikasi,
 * to'qnashuvlar (SAT usuli bilan yo'naltirilgan to'rtburchaklar) va baholash.
 */
#include "parking.h"

#include <math.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979f
#endif

/* ================================================================ */
/*                      Daraja quruvchi yordamchilar                */
/* ================================================================ */
static PkLevel *B;
static uint32_t brng;

static float rnd01(void)
{
    brng = brng * 1664525u + 1013904223u;
    return (float)(brng >> 8) / 16777216.0f;
}

static void obs(int type, float x, float z, float h, float w, float l)
{
    if (B->nobs >= PK_MAX_OBS) return;
    PkObs *o = &B->obs[B->nobs++];
    o->type = (uint8_t)type;
    o->x = x; o->z = z; o->h = h; o->w = w; o->l = l;
    o->color = (uint8_t)(rnd01() * 8);
}

/* turgan mashina: ba'zan oldi, ba'zan orqasi bilan */
static void car(float x, float z, float h)
{
    float side = (rnd01() - 0.5f) * 0.3f;
    if (rnd01() < 0.35f) h += PI;
    obs(OB_CAR, x + cosf(h) * side, z - sinf(h) * side, h + (rnd01() - 0.5f) * 0.06f, PK_CAR_W, PK_CAR_L);
}

static void truck(float x, float z, float h) { obs(OB_TRUCK, x, z, h, 2.6f, 9.0f); }
static void pillar(float x, float z) { obs(OB_PILLAR, x, z, 0, 0.9f, 0.9f); }
static void planter(float x, float z, float w, float l) { obs(OB_PLANTER, x, z, 0, w, l); }

/* (x0,z0)-(x1,z1) kesma bo'ylab devor yoki to'siq */
static void seg(int type, float x0, float z0, float x1, float z1, float t)
{
    float dx = x1 - x0, dz = z1 - z0, len = sqrtf(dx * dx + dz * dz);
    obs(type, (x0 + x1) / 2, (z0 + z1) / 2, atan2f(dx, dz), t, len);
}

static void cone(float x, float z)
{
    if (B->ncones >= PK_MAX_CONES) return;
    B->cone_x[B->ncones] = x;
    B->cone_z[B->ncones] = z;
    B->ncones++;
}

static void cone_line(float x0, float z0, float x1, float z1, float step)
{
    float dx = x1 - x0, dz = z1 - z0, len = sqrtf(dx * dx + dz * dz);
    int n = (int)(len / step + 0.5f);
    for (int i = 0; i <= n; i++) cone(x0 + dx * i / (n ? n : 1), z0 + dz * i / (n ? n : 1));
}

static void bay(float x, float z, float h, float w, float l, int parallel)
{
    if (B->nbays >= PK_MAX_BAYS) return;
    PkBay *b = &B->bays[B->nbays++];
    b->x = x; b->z = z; b->h = h; b->w = w; b->l = l; b->parallel = (uint8_t)parallel;
}

static void target(float x, float z, float h, float w, float l, int need, int parallel)
{
    B->target = (PkBay){ x, z, h, w, l, (uint8_t)parallel };
    B->need = need;
    bay(x, z, h, w, l, parallel);
}

/* Bir qator perpendikulyar joylar. h - joyga burun bilan kirish yo'nalishi.
 * tgt - maqsad joyi raqami (-1 - yo'q), empty - bo'sh qoladigan joylar bitlari. */
static void row(float x0, float z0, float dx, float dz, int n, float h, float w, int tgt, int need, unsigned empty)
{
    for (int i = 0; i < n; i++) {
        float x = x0 + dx * i, z = z0 + dz * i;
        if (i == tgt) target(x, z, h, w, 5.6f, need, 0);
        else {
            bay(x, z, h, w, 5.6f, 0);
            if (!((empty >> i) & 1)) car(x, z, h);
        }
    }
}

static void start(float x, float z, float h) { B->sx = x; B->sz = z; B->sh = h; }

static void lot(float w, float l, int theme, const char *name, const char *hint)
{
    B->lot_w = w; B->lot_l = l; B->theme = theme; B->name = name; B->hint = hint;
}

/* ================================================================ */
/*                              20 daraja                           */
/* ================================================================ */
/* Eslatma: yuqoriga qarab (+z) turgan haydovchi uchun chap tomon +x. */
#define N  0.0f          /* shimol (+z) */
#define S  PI            /* janub (-z)  */
#define W_ (PI / 2)      /* +x (haydovchining chapi) */
#define E_ (-PI / 2)     /* -x (haydovchining o'ngi) */

static void level_def(int k)
{
    switch (k) {
    case 0:
        lot(24, 40, 0, "Birinchi qadam", "To'g'ri yurib, yashil joyda to'xtang");
        start(0, -12, N);
        row(-6, 11, 3, 0, 5, N, 3.0f, 2, NEED_ANY, 0x1Fu);
        break;
    case 1:
        lot(24, 40, 0, "Qo'shnilar orasida", "Yon tomondagi mashinalarga tegmang");
        start(0, -12, N);
        row(-6, 11, 3, 0, 5, N, 3.0f, 3, NEED_ANY, 0);
        break;
    case 2:
        lot(34, 40, 0, "Chapga burilish", "Yo'lak bo'ylab yurib, chapdagi joyga buriling");
        start(-7, -14, N);
        row(8.2f, -9, 0, 3, 8, W_, 3.0f, 5, NEED_ANY, 0);
        row(-14, -9, 0, 3, 8, E_, 3.0f, -1, NEED_ANY, 0x24u);
        break;
    case 3:
        lot(34, 40, 0, "O'ngga burilish", "Endi o'ng tomondagi joy");
        start(7, -14, N);
        row(-8.2f, -9, 0, 3, 8, E_, 3.0f, 4, NEED_ANY, 0);
        row(14, -9, 0, 3, 8, W_, 3.0f, -1, NEED_ANY, 0x41u);
        cone(0, 2); cone(0, 6);
        break;
    case 4:
        lot(24, 36, 0, "Orqaga kirish", "S tugmasi bilan orqaga yuring - mashina oldi chiqishga qarasin");
        start(0, -1, N);
        row(-6, -12, 3, 0, 5, S, 3.0f, 2, NEED_REVERSE_IN, 0);
        row(-6, 12.5f, 3, 0, 5, N, 3.0f, -1, NEED_ANY, 0x05u);
        break;
    case 5:
        lot(24, 56, 1, "Konuslar yo'lagi", "Konuslar orasidan ilonsimon o'ting");
        start(-7, -24, N);
        cone_line(-12, -14, 3, -14, 1.5f);
        cone_line(-3, -2, 12, -2, 1.5f);
        cone_line(-12, 10, 3, 10, 1.5f);
        row(-6, 22, 3, 0, 5, N, 3.0f, 3, NEED_ANY, 0);
        break;
    case 6:
        lot(34, 40, 1, "Tor joy", "Joy tor - sekin va aniq kiring");
        start(7, -14, N);
        row(-8.2f, -10.4f, 0, 2.6f, 9, E_, 2.6f, 5, NEED_ANY, 0);
        row(14, -9, 0, 3, 8, W_, 3.0f, -1, NEED_ANY, 0x12u);
        break;
    case 7:
        lot(34, 36, 1, "Orqaga 90 gradus", "Joydan o'tib keting, so'ng orqaga burilib kiring");
        start(-5, -14, N);
        row(8.2f, -9, 0, 3, 8, W_, 3.0f, 3, NEED_REVERSE_IN, 0);
        row(-14, -9, 0, 3, 8, E_, 3.0f, -1, NEED_ANY, 0x18u);
        planter(-4, 13, 6, 2);
        break;
    case 8:
        lot(20, 60, 1, "Parallel parkovka", "Ikki mashina orasidagi bo'sh joyga yonma-yon turing");
        start(-2.5f, -24, N);
        car(6.6f, -18.4f, N); car(6.6f, -11.9f, N); car(6.6f, -5.4f, N);
        target(6.6f, 1.0f, N, 2.6f, 6.4f, NEED_ANY, 1);
        car(6.6f, 7.4f, N); car(6.6f, 13.9f, N); car(6.6f, 20.4f, N);
        car(-7.0f, -10, S); car(-7.0f, 4, S); car(-7.0f, 18, S);
        break;
    case 9:
        lot(30, 40, 1, "Garaj", "Garaj ichiga kiring, devorlarga tegmang");
        start(-7, -14, N);
        seg(OB_WALL, 4.15f, 9.8f, 4.15f, 16.6f, 0.3f);
        seg(OB_WALL, 7.85f, 9.8f, 7.85f, 16.6f, 0.3f);
        seg(OB_WALL, 4.0f, 16.6f, 8.0f, 16.6f, 0.3f);
        target(6, 13.3f, N, 3.2f, 6.0f, NEED_ANY, 0);
        car(-1, 12, N); car(11.5f, 12, S);
        cone(0, 0); cone(3, 0); cone(-3, -4);
        planter(-10, 4, 2, 6);
        break;
    case 10:
        lot(40, 44, 2, "Ustunlar", "Ustunlar orasidagi joyga kiring");
        start(-6, -17, N);
        row(9.2f, -10, 0, 3, 9, W_, 3.0f, 4, NEED_ANY, 0);
        for (int i = 0; i < 4; i++) { pillar(6.7f, -8.5f + i * 6); pillar(-6.7f, -8.5f + i * 6); }
        row(-9.2f, -10, 0, 3, 9, E_, 3.0f, -1, NEED_ANY, 0x41u);
        break;
    case 11: {
        lot(36, 44, 2, "Qiya joylar", "Qiya joylarga faqat oldinga yurib kiriladi");
        start(-6, -17, N);
        float a = PI / 4;
        for (int i = 0; i < 7; i++) {
            float x = 9.0f, z = -12 + i * 4.24f;
            if (i == 3) target(x, z, a, 3.0f, 5.6f, NEED_NOSE_IN, 0);
            else { bay(x, z, a, 3.0f, 5.6f, 0); obs(OB_CAR, x, z, a, PK_CAR_W, PK_CAR_L); }
        }
        row(-14.5f, -10, 0, 3, 8, E_, 3.0f, -1, NEED_ANY, 0x09u);
        break;
    }
    case 12:
        lot(30, 64, 2, "Ilon yo'li", "To'siqlar orasidan aylanib o'ting");
        start(-8, -27, N);
        seg(OB_BARRIER, -15, -16, 5, -16, 0.8f);
        seg(OB_BARRIER, -5, -2, 15, -2, 0.8f);
        seg(OB_BARRIER, -15, 12, 5, 12, 0.8f);
        row(-6, 25, 3, 0, 5, N, 3.0f, 1, NEED_ANY, 0);
        break;
    case 13:
        lot(20, 60, 2, "Qisqa parallel", "Bu safar joy qisqaroq");
        start(-2.5f, -24, N);
        car(6.6f, -18.85f, N); car(6.6f, -12.35f, N); car(6.6f, -5.85f, N);
        target(6.6f, 0.4f, N, 2.6f, 6.0f, NEED_ANY, 1);
        car(6.6f, 6.65f, N); car(6.6f, 13.15f, N); car(6.6f, 19.65f, N);
        car(-7.0f, -6, S); car(-7.0f, 9, S);
        cone(8.6f, -1.6f); cone(8.6f, 2.4f);
        break;
    case 14:
        lot(36, 44, 2, "Yuk mashinalari", "Katta mashinalar orasida orqaga kiring");
        start(0, -18, N);
        for (int i = 0; i < 5; i++) { if (i != 2) truck(-10, -12 + i * 6, E_); bay(-10, -12 + i * 6, E_, 3.4f, 9.6f, 0); }
        row(6.8f, -10.5f, 0, 3, 9, W_, 3.0f, 4, NEED_REVERSE_IN, 0);
        planter(-10, -0.5f, 2, 2);
        break;
    case 15:
        lot(40, 40, 3, "Labirint", "Devorlar orasidan yo'l toping");
        start(-14, -15, N);
        seg(OB_WALL, -20, -6, 8, -6, 0.6f);
        seg(OB_WALL, -8, 5, 20, 5, 0.6f);
        pillar(0, -1); pillar(10, 0); pillar(-10, 1);
        row(6, 16.6f, 3, 0, 4, N, 3.0f, 2, NEED_ANY, 0);
        car(-15, 14, W_);
        break;
    case 16:
        lot(12, 52, 3, "Orqaga burilish", "Tor ko'chada ortga buriling - mashina pastga qarab tursin");
        start(-2.5f, -20, N);
        car(4.4f, -16, S); car(4.4f, -9.5f, S); car(4.4f, -3.0f, S);
        target(4.4f, 4.5f, S, 2.6f, 8.0f, NEED_NOSE_IN, 1);
        car(4.4f, 12.0f, S); car(4.4f, 18.5f, S);
        planter(0, 24, 12, 2);
        break;
    case 17:
        lot(30, 40, 3, "Tungi garaj", "Garajga orqasi bilan kiring");
        start(-6, -14, N);
        seg(OB_WALL, 4.25f, 9.8f, 4.25f, 16.6f, 0.3f);
        seg(OB_WALL, 7.75f, 9.8f, 7.75f, 16.6f, 0.3f);
        seg(OB_WALL, 4.1f, 16.6f, 7.9f, 16.6f, 0.3f);
        target(6, 13.3f, N, 3.0f, 6.0f, NEED_REVERSE_IN, 0);
        car(0.5f, 12, N); car(11.5f, 12, S);
        seg(OB_BARRIER, -15, 4, -6, 4, 0.8f);
        cone(4, -2); cone(7, -2); cone(10, -2);
        break;
    case 18:
        lot(16, 60, 3, "Tor ko'cha", "Ikki tomonda mashinalar - parallel turing");
        start(0, -25, N);
        car(5.6f, -18.95f, N); car(5.6f, -12.45f, N); car(5.6f, -5.95f, N);
        target(5.6f, 0.6f, N, 2.6f, 6.1f, NEED_ANY, 1);
        car(5.6f, 7.15f, N); car(5.6f, 13.65f, N); car(5.6f, 20.15f, N);
        car(-5.6f, -16, S); car(-5.6f, -9.5f, S); car(-5.6f, -3, S); car(-5.6f, 3.5f, S); car(-5.6f, 10, S); car(-5.6f, 16.5f, S);
        break;
    case 19:
        lot(44, 48, 3, "Usta", "Labirint, ustunlar va tor joy - orqaga kiring");
        start(-17, -19, N);
        seg(OB_WALL, -22, -10, 10, -10, 0.6f);
        seg(OB_WALL, -10, 1, 22, 1, 0.6f);
        cone_line(-6, -4, 6, -4, 1.5f);
        row(-4.2f, 15.4f, 2.8f, 0, 7, N, 2.8f, 4, NEED_REVERSE_IN, 0);
        pillar(-5.6f, 11.4f); pillar(0.0f, 11.4f); pillar(5.6f, 11.4f);
        car(17, -18, N); car(-18, 8, E_);
        break;
    }
}

/* 3 yulduz uchun vaqt (tests/parking_test.c dagi yechuvchi asosida tanlangan) */
static const float PAR[PK_LEVELS] = {
    16, 16, 17, 16, 14, 28, 18, 24, 22, 20,
    18, 17, 33, 28, 24, 34, 30, 26, 25, 44,
};

void pk_build(int level, PkLevel *out)
{
    memset(out, 0, sizeof(*out));
    if (level < 0) level = 0;
    if (level >= PK_LEVELS) level = PK_LEVELS - 1;
    B = out;
    brng = 977u + (uint32_t)level * 7919u;
    level_def(level);
    /* tashqi devorlar */
    float w = out->lot_w / 2, l = out->lot_l / 2;
    seg(OB_WALL, -w - 0.4f, -l, -w - 0.4f, l, 0.8f);
    seg(OB_WALL, w + 0.4f, -l, w + 0.4f, l, 0.8f);
    seg(OB_WALL, -w - 0.8f, -l - 0.4f, w + 0.8f, -l - 0.4f, 0.8f);
    seg(OB_WALL, -w - 0.8f, l + 0.4f, w + 0.8f, l + 0.4f, 0.8f);
    out->par = PAR[level];
    B = NULL;
}

/* ================================================================ */
/*                          Geometriya                              */
/* ================================================================ */
void pk_corners(float x, float z, float h, float w, float l, float out[4][2])
{
    float fx = sinf(h), fz = cosf(h), sx = cosf(h), sz = -sinf(h);
    static const float K[4][2] = { { 1, 1 }, { -1, 1 }, { -1, -1 }, { 1, -1 } };
    for (int i = 0; i < 4; i++) {
        out[i][0] = x + sx * w / 2 * K[i][0] + fx * l / 2 * K[i][1];
        out[i][1] = z + sz * w / 2 * K[i][0] + fz * l / 2 * K[i][1];
    }
}

/* Ikki yo'naltirilgan to'rtburchak kesishadimi (ajratuvchi o'qlar teoremasi) */
static int obb_overlap(float x1, float z1, float h1, float w1, float l1,
                       float x2, float z2, float h2, float w2, float l2)
{
    float dx = x2 - x1, dz = z2 - z1;
    float r1 = 0.5f * sqrtf(w1 * w1 + l1 * l1), r2 = 0.5f * sqrtf(w2 * w2 + l2 * l2);
    if (dx * dx + dz * dz > (r1 + r2) * (r1 + r2)) return 0;
    float ax[4][2] = { { sinf(h1), cosf(h1) }, { cosf(h1), -sinf(h1) }, { sinf(h2), cosf(h2) }, { cosf(h2), -sinf(h2) } };
    for (int k = 0; k < 4; k++) {
        float a0 = ax[k][0], a1 = ax[k][1];
        float d = fabsf(dx * a0 + dz * a1);
        float p1 = w1 / 2 * fabsf(ax[1][0] * a0 + ax[1][1] * a1) + l1 / 2 * fabsf(ax[0][0] * a0 + ax[0][1] * a1);
        float p2 = w2 / 2 * fabsf(ax[3][0] * a0 + ax[3][1] * a1) + l2 / 2 * fabsf(ax[2][0] * a0 + ax[2][1] * a1);
        if (d > p1 + p2) return 0;
    }
    return 1;
}

float pk_margin = 0;

static int cone_hit(float x, float z, float h, float cx, float cz)
{
    float dx = cx - x, dz = cz - z;
    float lz = dx * sinf(h) + dz * cosf(h), lx = dx * cosf(h) - dz * sinf(h);
    float hw = (PK_CAR_W + pk_margin) / 2, hl = (PK_CAR_L + pk_margin) / 2;
    float qx = fmaxf(-hw, fminf(hw, lx)), qz = fmaxf(-hl, fminf(hl, lz));
    float ex = lx - qx, ez = lz - qz;
    return ex * ex + ez * ez < PK_CONE_R * PK_CONE_R;
}

int pk_collides(const PkLevel *L, const uint8_t *cone_down, float x, float z, float h)
{
    for (int i = 0; i < L->nobs; i++) {
        const PkObs *o = &L->obs[i];
        if (obb_overlap(x, z, h, PK_CAR_W + pk_margin, PK_CAR_L + pk_margin, o->x, o->z, o->h, o->w, o->l)) return 1;
    }
    for (int i = 0; i < L->ncones; i++)
        if (!(cone_down && cone_down[i]) && cone_hit(x, z, h, L->cone_x[i], L->cone_z[i])) return 2;
    return 0;
}

static float angle_diff(float a, float b)
{
    float d = fmodf(a - b + PI, 2 * PI);
    if (d < 0) d += 2 * PI;
    return fabsf(d - PI);
}

static int aligned(const PkLevel *L, float h)
{
    const float tol = 0.21f;   /* ~12 gradus */
    float d0 = angle_diff(h, L->target.h), d1 = angle_diff(h, L->target.h + PI);
    if (L->need == NEED_NOSE_IN) return d0 < tol;
    if (L->need == NEED_REVERSE_IN) return d1 < tol;
    return d0 < tol || d1 < tol;
}

float pk_fit(const PkLevel *L, float x, float z, float h)
{
    const PkBay *t = &L->target;
    float c[4][2];
    pk_corners(x, z, h, PK_CAR_W, PK_CAR_L, c);
    int inside = 0;
    for (int i = 0; i < 4; i++) {
        float dx = c[i][0] - t->x, dz = c[i][1] - t->z;
        float lz = dx * sinf(t->h) + dz * cosf(t->h), lx = dx * cosf(t->h) - dz * sinf(t->h);
        if (fabsf(lx) <= t->w / 2 && fabsf(lz) <= t->l / 2) inside++;
    }
    return inside / 4.0f * (aligned(L, h) ? 1.0f : 0.75f);
}

int pk_in_spot(const PkLevel *L, float x, float z, float h) { return pk_fit(L, x, z, h) >= 0.999f; }

int pk_stars_for(const PkLevel *L, float time, int hits)
{
    (void)hits;   /* urilsa daraja baribir yutqaziladi - yulduzlar faqat vaqtga bog'liq */
    if (time <= L->par) return 3;
    if (time <= L->par * 1.7f) return 2;
    return 1;
}

/* ================================================================ */
/*                              Fizika                              */
/* ================================================================ */
void pk_load(PkGame *g, int level)
{
    memset(g, 0, sizeof(*g));
    g->level = level;
    pk_build(level, &g->L);
    g->car.x = g->L.sx;
    g->car.z = g->L.sz;
    g->car.h = g->L.sh;
}

static float approach(float v, float target, float step)
{
    if (v < target) return fminf(v + step, target);
    return fmaxf(v - step, target);
}

void pk_step(PkGame *g, float steer, float throttle, int handbrake, float dt)
{
    PkCar *c = &g->car;
    /* konuslar sirpanib to'xtaydi (o'yin tugagandan keyin ham) */
    for (int i = 0; i < g->L.ncones; i++) {
        if (!g->cone_down[i]) continue;
        g->L.cone_x[i] += g->cone_vx[i] * dt;
        g->L.cone_z[i] += g->cone_vz[i] * dt;
        float k = fmaxf(0, 1 - dt * 3);
        g->cone_vx[i] *= k; g->cone_vz[i] *= k;
        g->cone_rot[i] += sqrtf(g->cone_vx[i] * g->cone_vx[i] + g->cone_vz[i] * g->cone_vz[i]) * dt * 2;
    }
    if (g->state != PK_PLAY) {
        g->end_timer += dt;
        c->speed = approach(c->speed, 0, 14 * dt);
        throttle = 0; steer = 0;
    } else {
        g->time += dt;
    }
    if (g->hit_cd > 0) g->hit_cd -= dt;

    steer = fmaxf(-1, fminf(1, steer));
    c->steer = approach(c->steer, steer, dt * 3.2f);

    if (throttle > 0.05f) {
        if (c->speed < -0.2f) c->speed = approach(c->speed, 0, 14 * dt);
        else c->speed += 5.0f * throttle * dt;
    } else if (throttle < -0.05f) {
        if (c->speed > 0.2f) c->speed = approach(c->speed, 0, 14 * dt);
        else c->speed += 4.0f * throttle * dt;
    } else {
        c->speed = approach(c->speed, 0, 2.5f * dt);
    }
    if (handbrake) c->speed = approach(c->speed, 0, 20 * dt);
    c->speed = fmaxf(-PK_MAX_REV, fminf(PK_MAX_FWD, c->speed));

    /* kichik qadamlar bilan harakat - tez yurganda ham devordan "o'tib ketmaydi" */
    float dist = fabsf(c->speed) * dt;
    int n = (int)(dist / 0.08f) + 1;
    float sdt = dt / n;
    for (int s = 0; s < n; s++) {
        float nh = c->h - c->speed * tanf(c->steer * PK_MAX_STEER) / PK_WHEELBASE * sdt;
        float mh = (c->h + nh) / 2;
        float nx = c->x + sinf(mh) * c->speed * sdt, nz = c->z + cosf(mh) * c->speed * sdt;
        int hit = 0;
        for (int i = 0; i < g->L.nobs && !hit; i++) {
            const PkObs *o = &g->L.obs[i];
            if (obb_overlap(nx, nz, nh, PK_CAR_W, PK_CAR_L, o->x, o->z, o->h, o->w, o->l)) hit = 1;
        }
        if (hit) {
            float v = fabsf(c->speed);
            if (v > 0.8f && g->hit_cd <= 0 && g->state == PK_PLAY) {
                g->hits++;
                g->hit_cd = 0.8f;
                g->ev_hit = 1;
                g->ev_power = v;
                g->ev_x = c->x + sinf(c->h) * (c->speed > 0 ? 2.3f : -2.3f);
                g->ev_z = c->z + cosf(c->h) * (c->speed > 0 ? 2.3f : -2.3f);
            }
            c->speed = -c->speed * 0.2f;
            break;
        }
        c->x = nx; c->z = nz; c->h = nh;
        /* konuslar urib yuboriladi */
        for (int i = 0; i < g->L.ncones; i++) {
            if (g->cone_down[i] || !cone_hit(c->x, c->z, c->h, g->L.cone_x[i], g->L.cone_z[i])) continue;
            g->cone_down[i] = 1;
            float dx = g->L.cone_x[i] - c->x, dz = g->L.cone_z[i] - c->z, d = sqrtf(dx * dx + dz * dz) + 0.01f;
            g->cone_vx[i] = sinf(c->h) * c->speed * 1.2f + dx / d * 2.5f;
            g->cone_vz[i] = cosf(c->h) * c->speed * 1.2f + dz / d * 2.5f;
            g->ev_cone = 1;
            g->ev_x = g->L.cone_x[i]; g->ev_z = g->L.cone_z[i];
            if (g->state == PK_PLAY && g->hit_cd <= 0) { g->hits++; g->hit_cd = 0.5f; }
        }
    }

    if (g->state != PK_PLAY) return;
    if (g->hits >= PK_MAX_HITS) { g->state = PK_FAILED; return; }
    if (fabsf(c->speed) < 0.35f && pk_in_spot(&g->L, c->x, c->z, c->h)) {
        g->hold += dt;
        if (g->hold >= PK_HOLD_TIME) {
            g->state = PK_WON;
            g->stars = pk_stars_for(&g->L, g->time, g->hits);
        }
    } else {
        g->hold = fmaxf(0, g->hold - dt * 2);
    }
}
