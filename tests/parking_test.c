/*
 * parking_test.c - "Parkovka" rejimi testlari.
 *
 * 1) Har bir daraja uchun Hybrid A* qidiruvi (mashinaning haqiqiy burilish
 *    radiusi bilan, oldinga va orqaga) yo'l topadi - demak daraja yechiladi.
 * 2) Topilgan yo'l o'yinning o'z fizikasi (pk_step) bilan haydab ko'riladi:
 *    mashina joyiga kirib, daraja "yutildi" holatiga o'tishi kerak.
 * 3) Urilish, konus, yutqazish qoidalari (bitta urilish - yutqazish).
 */
#include "parking.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265f
static int failures = 0;
#define CHECK(c, msg) do { if (c) printf("  OK:   %s\n", msg); else { printf("  XATO: %s\n", msg); failures++; } } while (0)

/* ------------------------------------------------------ Hybrid A* */
#define CELL   0.35f
#define HBINS  72
#define STEP   0.7f
#define MAXN   6000000

typedef struct { float x, z, h, g, len; int parent; signed char dir; unsigned char sw; } Node;

static Node  *pool;
static int   npool;
static int   *heap, nheap;
static float *best;
static float  heur_x, heur_z;

static float fval(int i) { const Node *n = &pool[i]; float dx = n->x - heur_x, dz = n->z - heur_z; return n->g + sqrtf(dx * dx + dz * dz) * 1.1f; }
static void hpush(int id)
{
    int i = nheap++;
    heap[i] = id;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (fval(heap[p]) <= fval(heap[i])) break;
        int t = heap[p]; heap[p] = heap[i]; heap[i] = t; i = p;
    }
}
static int hpop(void)
{
    int top = heap[0];
    heap[0] = heap[--nheap];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = l + 1, m = i;
        if (l < nheap && fval(heap[l]) < fval(heap[m])) m = l;
        if (r < nheap && fval(heap[r]) < fval(heap[m])) m = r;
        if (m == i) break;
        int t = heap[m]; heap[m] = heap[i]; heap[i] = t; i = m;
    }
    return top;
}

typedef struct { int found, goal, expanded; float len; int switches; } Plan;

/* Yechuvchi uchun maqsad qat'iyroq: joyning o'rtasida va deyarli tekis turish.
 * Shunda haydovchining kichik xatolari ham joy ichida qoladi. */
static int goal_ok(const PkLevel *L, float x, float z, float h)
{
    PkLevel S = *L;
    S.target.w -= 0.3f; S.target.l -= 0.4f;
    if (!pk_in_spot(&S, x, z, h)) return 0;
    float th = L->need == NEED_REVERSE_IN ? L->target.h + PI_F : L->target.h;
    float d = fmodf(h - th + 3 * PI_F, 2 * PI_F) - PI_F;
    if (L->need == NEED_ANY && fabsf(d) > PI_F / 2) d = fabsf(d) - PI_F;
    return fabsf(d) < 0.08f;
}

static Plan solve(const PkLevel *L)
{
    Plan P = { 0, -1, 0, 0, 0 };
    int gw = (int)(L->lot_w / CELL) + 2, gl = (int)(L->lot_l / CELL) + 2;
    long cells = (long)gw * gl * HBINS;
    best = malloc(sizeof(float) * cells);
    for (long i = 0; i < cells; i++) best[i] = 1e30f;
    npool = 0; nheap = 0;
    heur_x = L->target.x; heur_z = L->target.z;
    pool[npool] = (Node){ L->sx, L->sz, L->sh, 0, 0, -1, 0, 0 };
    hpush(npool++);
    static const float STEERS[5] = { -1, -0.5f, 0, 0.5f, 1 };
    while (nheap > 0 && npool < MAXN - 20) {
        int id = hpop();
        Node cur = pool[id];
        P.expanded++;
        if (goal_ok(L, cur.x, cur.z, cur.h)) { P.found = 1; P.goal = id; P.len = cur.len; P.switches = cur.sw; break; }
        for (int d = -1; d <= 1; d += 2)
            for (int k = 0; k < 5; k++) {
                float x = cur.x, z = cur.z, h = cur.h, v = (float)d;
                int ok = 1;
                for (int s = 0; s < 7 && ok; s++) {   /* fizikadagi bilan bir xil model */
                    float ds = STEP / 7;
                    float nh = h - v * tanf(STEERS[k] * PK_MAX_STEER) / PK_WHEELBASE * ds;
                    float mh = (h + nh) / 2;
                    x += sinf(mh) * v * ds; z += cosf(mh) * v * ds; h = nh;
                    if (pk_collides(L, NULL, x, z, h)) ok = 0;
                }
                if (!ok) continue;
                int sw = cur.dir != 0 && cur.dir != d;
                float g = cur.g + STEP * (d < 0 ? 1.4f : 1.0f) + (sw ? 3.0f : 0) + 0.05f * fabsf(STEERS[k]);
                int ix = (int)((x + L->lot_w / 2) / CELL), iz = (int)((z + L->lot_l / 2) / CELL);
                float hn = fmodf(h, 2 * PI_F); if (hn < 0) hn += 2 * PI_F;
                int ih = (int)(hn / (2 * PI_F) * HBINS) % HBINS;
                if (ix < 0 || iz < 0 || ix >= gw || iz >= gl) continue;
                long c = ((long)iz * gw + ix) * HBINS + ih;
                if (g >= best[c]) continue;
                best[c] = g;
                pool[npool] = (Node){ x, z, h, g, cur.len + STEP, id, (signed char)d, (unsigned char)(cur.sw + sw) };
                hpush(npool++);
            }
    }
    free(best);
    return P;
}

/* ------------------------------------------- yo'lni fizika bilan haydash */
#define MAXP 4000
static float px[MAXP], pz[MAXP], ph[MAXP];
static int   pd[MAXP], np;

static float target_x, target_z;
static const PkLevel *cur_level;

static void extract(int goal)
{
    int ids[MAXP], n = 0;
    for (int i = goal; i >= 0 && n < MAXP; i = pool[i].parent) ids[n++] = i;
    np = 0;
    for (int k = n - 1; k >= 0; k--) {
        Node *q = &pool[ids[k]];
        px[np] = q->x; pz[np] = q->z; ph[np] = q->h;
        pd[np] = k > 0 ? pool[ids[k - 1]].dir : q->dir;   /* shu nuqtadan keyingi harakat yo'nalishi */
        np++;
    }
    /* oxirida joy markaziga qadar to'g'ri davom ettiramiz (chegarada emas, o'rtada to'xtash uchun) */
    int d = pd[np - 1] ? pd[np - 1] : 1;
    for (int k = 0; k < 20 && np < MAXP; k++) {
        float x = px[np - 1] + sinf(ph[np - 1]) * d * 0.2f, z = pz[np - 1] + cosf(ph[np - 1]) * d * 0.2f;
        float d0 = hypotf(px[np - 1] - target_x, pz[np - 1] - target_z), d1 = hypotf(x - target_x, z - target_z);
        if (d1 >= d0 || pk_collides(cur_level, NULL, x, z, ph[np - 1])) break;
        px[np] = x; pz[np] = z; ph[np] = ph[np - 1]; pd[np - 1] = d; pd[np] = d; np++;
    }
}

static float wrap(float a) { while (a > PI_F) a -= 2 * PI_F; while (a < -PI_F) a += 2 * PI_F; return a; }

/* Yo'l bo'ylab boshqaruvchi: oldinga/orqaga bo'laklar, Stanley usuli.
 * Har bir bo'lak oxirida to'xtab, uzatma almashtiriladi. */
/* Yo'lni bosib o'tadi. Qaytaradi: 0 - oxiriga yetdi, 1 - urildi (qayta rejalash kerak) */
static int follow(PkGame *gp)
{
    PkGame g = *gp;
    int seg_start = 0, hits0 = g.hits;
    float wait = 0;
    const float dt = 1.0f / 60;
    while (g.state == PK_PLAY && g.time < 240) {
        if (g.hits != hits0) {   /* urildik: to'xtab, qaytadan rejalaymiz */
            for (int i = 0; i < 40; i++) pk_step(&g, 0, 0, 1, dt);
            *gp = g;
            return 1;
        }
        int dir = pd[seg_start];
        int seg_end = seg_start;
        while (seg_end + 1 < np && pd[seg_end + 1] == dir) seg_end++;
        if (seg_end + 1 < np) seg_end++;   /* bo'lak oxiri - keyingi nuqta */
        /* eng yaqin nuqta (bo'lak ichida) */
        int bi = seg_start;
        float bd = 1e9f;
        for (int i = seg_start; i <= seg_end; i++) {
            float dx = px[i] - g.car.x, dz = pz[i] - g.car.z, d = dx * dx + dz * dz;
            if (d < bd) { bd = d; bi = i; }
        }
        /* oldingi g'ildiraklar (yoki orqaga yurganda orqa) nuqtasi */
        float axle = dir > 0 ? PK_WHEELBASE / 2 : -PK_WHEELBASE / 2;
        float fx = g.car.x + sinf(g.car.h) * axle, fz = g.car.z + cosf(g.car.h) * axle;
        int ti = bi;
        float td = 1e9f;
        for (int i = seg_start; i <= seg_end; i++) {
            float ax = px[i] + sinf(ph[i]) * axle, az = pz[i] + cosf(ph[i]) * axle;
            float dx = ax - fx, dz = az - fz, d = dx * dx + dz * dz;
            if (d < td) { td = d; ti = i; }
        }
        float ax = px[ti] + sinf(ph[ti]) * axle, az = pz[ti] + cosf(ph[ti]) * axle;
        float ex = fx - ax, ez = fz - az;
        float cte = ex * cosf(ph[ti]) - ez * sinf(ph[ti]);   /* +: yo'ldan chapda (+x tomonda) */
        float he = wrap(ph[ti] - g.car.h);
        float v = fmaxf(fabsf(g.car.speed), 0.5f);
        /* u - yo'nalishni qancha oshirish kerak. cte > 0: yo'ldan chapda.
         * Oldinga: rul > 0 yo'nalishni kamaytiradi; orqaga - aksincha. */
        float steer;
        if (dir > 0) steer = -(he - atanf(1.5f * cte / v)) / PK_MAX_STEER;
        else         steer = (he + atanf(2.5f * cte / v)) / PK_MAX_STEER;
        /* oldindan: yo'l egriligidan kerakli rul */
        if (ti + 1 <= seg_end) {
            float k = wrap(ph[ti + 1] - ph[ti]) / STEP;
            steer += atanf(-dir * PK_WHEELBASE * k) / PK_MAX_STEER;
        }
        steer = fmaxf(-1, fminf(1, steer));
        /* tezlik: bo'lak oxiriga yaqinlashganda sekinlashadi */
        float rem = 0;
        for (int i = ti; i < seg_end; i++) rem += hypotf(px[i + 1] - px[i], pz[i + 1] - pz[i]);
        float dxe = px[seg_end] - g.car.x, dze = pz[seg_end] - g.car.z;
        float along = dxe * sinf(g.car.h) + dze * cosf(g.car.h);
        if (dir < 0) along = -along;
        float left = rem < 1.0f ? fminf(rem, fmaxf(along, 0)) : rem;
        float vt = fminf(dir > 0 ? 1.6f : 1.0f, fmaxf(0.3f, left * 1.2f));
        float thr = dir > 0 ? (vt - g.car.speed) * 1.5f : -(vt + g.car.speed) * 1.5f;
        thr = fmaxf(-1, fminf(1, thr));
        int at_end = rem < 0.3f && along < 0.06f;
        if (at_end && fabsf(g.car.speed) < 0.15f) {
            if (seg_end >= np - 1) {
                pk_step(&g, steer, 0, 1, dt);
                wait += dt;
                if (wait > PK_HOLD_TIME + 0.3f) break;
                continue;
            }
            seg_start = seg_end;
            continue;
        }
        if (at_end) { pk_step(&g, steer, 0, 1, dt); continue; }
        pk_step(&g, steer, thr, 0, dt);
    }
    *gp = g;
    return 0;
}

static int drive(int level, float *out_time, int *out_hits, int *replans)
{
    PkGame g;
    pk_load(&g, level);
    *replans = 0;
    for (int attempt = 0; attempt < 6 && g.state == PK_PLAY; attempt++) {
        if (attempt > 0) {   /* joriy holatdan qayta yechamiz */
            PkLevel L = g.L;
            L.sx = g.car.x; L.sz = g.car.z; L.sh = g.car.h;
            pk_margin = 0.3f;
            Plan P = solve(&L);
            if (!P.found) { pk_margin = 0.1f; P = solve(&L); }
            pk_margin = 0;
            if (!P.found) break;
            extract(P.goal);
            (*replans)++;
        }
        follow(&g);
    }
    if (getenv("PKDBG")) {
        float fit = pk_fit(&g.L, g.car.x, g.car.z, g.car.h);
        printf("     oxiri: car (%.2f %.2f %.2f) v=%.2f target (%.2f %.2f %.2f) fit=%.2f seg=%d/%d path_end (%.2f %.2f %.2f)\n",
               g.car.x, g.car.z, g.car.h, g.car.speed, g.L.target.x, g.L.target.z, g.L.target.h, fit, *replans, np, px[np-1], pz[np-1], ph[np-1]);
    }
    *out_time = g.time;
    *out_hits = g.hits;
    return g.state == PK_WON;
}

int main(void)
{
    printf("== Parkovka: darajalar tuzilishi\n");
    int ok_build = 1;
    for (int k = 0; k < PK_LEVELS; k++) {
        PkLevel L;
        pk_build(k, &L);
        if (pk_collides(&L, NULL, L.sx, L.sz, L.sh)) { printf("  %d: boshlanish joyi to'siqqa tegib turibdi\n", k + 1); ok_build = 0; }
        float th = L.need == NEED_REVERSE_IN ? L.target.h + PI_F : L.target.h;
        if (pk_collides(&L, NULL, L.target.x, L.target.z, th)) { printf("  %d: maqsad joyi band\n", k + 1); ok_build = 0; }
        if (!pk_in_spot(&L, L.target.x, L.target.z, th)) { printf("  %d: mashina maqsadga sig'maydi\n", k + 1); ok_build = 0; }
    }
    CHECK(ok_build, "20 ta daraja: boshlanish va maqsad joylari to'g'ri");

    printf("== Parkovka: har bir daraja yechiladi (Hybrid A*) va fizika bilan haydaladi\n");
    pool = malloc(sizeof(Node) * MAXN);
    heap = malloc(sizeof(int) * MAXN);
    int all_solved = 1, all_driven = 1;
    int only = getenv("PKONLY") ? atoi(getenv("PKONLY")) : -1;
    for (int k = 0; k < PK_LEVELS; k++) {
        if (only >= 0 && k != only) continue;
        PkLevel L;
        pk_build(k, &L);
        /* yechuvchi to'siqlardan uzoqroq yursin (haydovchining kichik xatolari urilishga olib kelmasin) */
        pk_margin = 0.6f;
        Plan P = solve(&L);
        if (!P.found) { pk_margin = 0.45f; P = solve(&L); }
        if (!P.found) { pk_margin = 0.3f; P = solve(&L); }
        if (getenv("PKDBG")) printf("     margin %.2f\n", pk_margin);
        pk_margin = 0;
        float t = 0;
        int hits = 0, won = 0, replans = 0;
        if (P.found) { target_x = L.target.x; target_z = L.target.z; cur_level = &L; extract(P.goal); won = drive(k, &t, &hits, &replans); }
        float est = P.len / 3.0f + P.switches * 3.0f + 4.0f;
        printf("  %2d %-20s %s  yo'l %5.1f m, %d uzatma almashuvi, taxminiy %4.1f s (par %2.0f) | fizika: %s %5.1f s, %d urilish\n",
               k + 1, L.name, P.found ? "yechildi" : "YECHILMADI", P.len, P.switches, est, L.par,
               won ? "yutdi" : "YUTMADI", t, hits);
        if (replans) printf("       (%d marta qayta rejalandi)\n", replans);
        if (!P.found) all_solved = 0;
        if (!won) all_driven = 0;
    }
    CHECK(all_solved, "barcha 20 daraja yechiladi");
    CHECK(all_driven, "topilgan yo'l o'yin fizikasi bilan ham bosib o'tiladi");

    printf("== Parkovka: qoidalar\n");
    {
        PkGame g;
        pk_load(&g, 0);
        for (int i = 0; i < 60 * 20 && g.state == PK_PLAY; i++) {
            float want = fmaxf(0, fminf(5, (g.L.target.z - g.car.z) * 0.9f));
            pk_step(&g, 0, fmaxf(-1, fminf(1, (want - g.car.speed))), 0, 1.0f / 60);
        }
        CHECK(g.state == PK_WON && g.stars == 3, "1-daraja: to'g'ri yurib, joyda to'xtash - 3 yulduz");
    }
    {
        PkGame g;
        pk_load(&g, 0);
        for (int i = 0; i < 60 * 40 && g.state == PK_PLAY; i++) {
            /* devorga qayta-qayta urilish: 4 s oldinga, 1.5 s orqaga */
            float t = fmodf(i / 60.0f, 5.5f);
            pk_step(&g, 0, t < 4.0f ? 1 : -1, 0, 1.0f / 60);
        }
        CHECK(g.state == PK_FAILED && g.hits >= PK_MAX_HITS, "bitta urilish - daraja yutqaziladi");
    }
    {
        PkGame g;
        pk_load(&g, 5);
        g.car.x = -6; g.car.z = -17; g.car.h = 0;
        for (int i = 0; i < 60 * 3; i++) pk_step(&g, 0, 1, 0, 1.0f / 60);
        int down = 0;
        for (int i = 0; i < g.L.ncones; i++) down += g.cone_down[i];
        CHECK(down > 0 && g.hits >= 1 && g.state == PK_FAILED, "konusga tegilsa - konus yiqiladi va daraja yutqaziladi");
    }
    {
        PkGame g;
        pk_load(&g, 4);   /* orqaga kirish talab qilinadi */
        PkLevel *L = &g.L;
        CHECK(!pk_in_spot(L, L->target.x, L->target.z, L->target.h) && pk_in_spot(L, L->target.x, L->target.z, L->target.h + PI_F),
              "orqaga kirish darajasida burun bilan kirish hisobga olinmaydi");
    }
    free(pool);
    free(heap);
    printf(failures ? "\n%d ta test xato.\n" : "\nBarcha parkovka testlari o'tdi.\n", failures);
    return failures ? 1 : 0;
}
