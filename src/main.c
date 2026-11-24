/*
 * main.c - TURBO JANG!
 * Blur uslubidagi 4 kishilik qurolli poyga. Yo'ldagi qutilardan qurol
 * olib, raqiblarga qarshi ishlating. Bitta Wi-Fi / hotspot orqali.
 * Grafika: raylib (3D). Windows (.exe) va Android (.apk) uchun bitta kod.
 *
 * Rejimlar: "Poyga" (4 kishigacha, tarmoq orqali) va "Parkovka" (1 kishi, 20 daraja).
 *
 * Boshqaruv (poyga):
 *   Kompyuter : W/S yoki strelkalar - gaz/tormoz, A/D - rul, PROBEL - qurol
 *   Geympad   : chap stik - rul, RT - gaz, LT - tormoz, A - qurol
 *   Telefon   : chapda rul tugmalari, o'ngda qurol va tormoz (gaz avtomatik)
 *
 * Shriftlar: Poppins va JetBrains Mono (SIL OFL 1.1), dastur ichiga joylangan.
 */
#include "raylib.h"
#include "rlgl.h"
#include "game.h"
#include "track.h"
#include "parking.h"
#include "net.h"
#include "plat.h"
#include "fonts_data.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VH 720

#if defined(__ANDROID__)
#define IS_MOBILE 1
#else
#define IS_MOBILE 0
#endif

/* =================================================================== */
/*                          DIZAYN TIZIMI                              */
/* =================================================================== */
static const Color CAR_COLORS[MAX_PLAYERS] = {
    { 239, 68, 68, 255 }, { 59, 130, 246, 255 }, { 250, 204, 21, 255 }, { 34, 197, 94, 255 },
};
static const Color ITEM_COLORS[ITEM_COUNT] = {
    { 100, 116, 139, 255 },   /* yo'q    */
    { 56, 189, 248, 255 },    /* nitro   */
    { 239, 68, 68, 255 },     /* raketa  */
    { 250, 204, 21, 255 },    /* chaqmoq */
    { 249, 115, 22, 255 },    /* mina    */
    { 168, 85, 247, 255 },    /* zarba   */
    { 34, 197, 94, 255 },     /* qalqon  */
};

#define COL_BG       (Color){ 11, 14, 24, 255 }
#define COL_SURFACE  (Color){ 18, 22, 35, 235 }
#define COL_SURFACE2 (Color){ 30, 36, 54, 255 }
#define COL_SURFACE3 (Color){ 42, 50, 74, 255 }
#define COL_BORDER   (Color){ 255, 255, 255, 22 }
#define COL_TEXT     (Color){ 241, 245, 249, 255 }
#define COL_MUTED    (Color){ 148, 163, 184, 255 }
#define COL_SUBTLE   (Color){ 100, 116, 139, 255 }
#define COL_ACCENT   (Color){ 255, 186, 48, 255 }
#define COL_VIOLET   (Color){ 139, 92, 246, 255 }
#define COL_GOOD     (Color){ 34, 197, 94, 255 }
#define COL_BAD      (Color){ 239, 68, 68, 255 }
#define COL_INFO     (Color){ 56, 189, 248, 255 }
#define COL_DARKTXT  (Color){ 22, 22, 30, 255 }

/* ------------------------------------------------- xarita mavzulari */
enum { DECOR_FOREST = 0, DECOR_DESERT, DECOR_SNOW, DECOR_CITY };
typedef struct {
    Color sky_top, sky_hor, ground, ground2, asphalt, line, curb_a, curb_b, wall, wall_top, hill1, hill2, accent;
    float bright;    /* umumiy yorug'lik */
    int   decor, night;
    const char *desc;
} Theme;
static const Theme THEMES[MAP_COUNT] = {
    { { 74, 144, 226, 255 }, { 196, 224, 248, 255 }, { 88, 164, 72, 255 }, { 98, 176, 80, 255 }, { 54, 57, 66, 255 },
      { 236, 238, 242, 255 }, { 220, 45, 45, 255 }, { 245, 245, 245, 255 }, { 150, 156, 170, 255 }, { 205, 210, 220, 255 },
      { 118, 162, 200, 255 }, { 150, 188, 222, 255 }, { 34, 197, 94, 255 }, 1.0f, DECOR_FOREST, 0, "o'rmon, keng burilishlar" },
    { { 92, 150, 214, 255 }, { 250, 214, 160, 255 }, { 214, 170, 108, 255 }, { 224, 182, 120, 255 }, { 78, 66, 60, 255 },
      { 250, 236, 200, 255 }, { 230, 90, 30, 255 }, { 250, 240, 220, 255 }, { 190, 140, 92, 255 }, { 226, 186, 136, 255 },
      { 196, 128, 84, 255 }, { 226, 168, 118, 255 }, { 249, 115, 22, 255 }, 1.05f, DECOR_DESERT, 0, "kaktuslar, qoyalar, issiq" },
    { { 120, 150, 190, 255 }, { 226, 234, 244, 255 }, { 232, 238, 246, 255 }, { 218, 228, 240, 255 }, { 70, 76, 90, 255 },
      { 255, 255, 255, 255 }, { 37, 99, 235, 255 }, { 245, 248, 252, 255 }, { 200, 210, 226, 255 }, { 250, 252, 255, 255 },
      { 150, 168, 196, 255 }, { 186, 200, 222, 255 }, { 56, 189, 248, 255 }, 0.97f, DECOR_SNOW, 0, "qor, archalar, uzun trassa" },
    { { 8, 10, 30, 255 }, { 38, 34, 78, 255 }, { 30, 32, 44, 255 }, { 36, 38, 52, 255 }, { 40, 42, 54, 255 },
      { 250, 220, 120, 255 }, { 236, 72, 153, 255 }, { 60, 220, 255, 255 }, { 70, 74, 96, 255 }, { 236, 72, 153, 255 },
      { 26, 26, 56, 255 }, { 40, 36, 80, 255 }, { 236, 72, 153, 255 }, 0.62f, DECOR_CITY, 1, "neon, binolar, tez" },
};
static const Theme *TH = &THEMES[0];
#define SKY_TOP      (TH->sky_top)
#define SKY_HORIZON  (TH->sky_hor)
#define GRASS        (TH->ground)
#define ASPHALT      (TH->asphalt)

#define DRAW_DIST 300.0f

/* ------------------------------------------------------------ Shriftlar */
enum { F_BODY, F_SEMI, F_BOLD, F_MONO, F_MONOB, F_COUNT };
static Font fonts[F_COUNT];

static void load_fonts(void)
{
    static int cps[260];
    int n = 0;
    for (int c = 32; c < 127; c++) cps[n++] = c;
    for (int c = 160; c < 256; c++) cps[n++] = c;
    const int extra[] = { 0x2BB, 0x2BC, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2026, 0x2190, 0x2191, 0x2192, 0x2193 };
    for (size_t i = 0; i < sizeof(extra) / sizeof(extra[0]); i++) cps[n++] = extra[i];
    const unsigned char *data[F_COUNT] = { FONT_UI_MEDIUM, FONT_UI_SEMIBOLD, FONT_UI_BOLD, FONT_MONO, FONT_MONO_BOLD };
    const int sizes[F_COUNT] = { FONT_UI_MEDIUM_SIZE, FONT_UI_SEMIBOLD_SIZE, FONT_UI_BOLD_SIZE, FONT_MONO_SIZE, FONT_MONO_BOLD_SIZE };
    for (int i = 0; i < F_COUNT; i++) {
        fonts[i] = LoadFontFromMemory(".ttf", data[i], sizes[i], 80, cps, n);
        GenTextureMipmaps(&fonts[i].texture);
        SetTextureFilter(fonts[i].texture, TEXTURE_FILTER_TRILINEAR);
    }
}

static float twidth(int f, const char *s, float sz) { return MeasureTextEx(fonts[f], s, sz, 0).x; }
static void  tdraw(int f, const char *s, float x, float y, float sz, Color c) { DrawTextEx(fonts[f], s, (Vector2){ x, y }, sz, 0, c); }
static void  tmid(int f, const char *s, float x, float cy, float sz, Color c) { tdraw(f, s, x, cy - sz * 0.56f, sz, c); }
static void  tcenter(int f, const char *s, float cx, float cy, float sz, Color c) { tmid(f, s, cx - twidth(f, s, sz) / 2, cy, sz, c); }
static void  tright(int f, const char *s, float rx, float cy, float sz, Color c) { tmid(f, s, rx - twidth(f, s, sz), cy, sz, c); }

/* ------------------------------------------------------------ Holatlar */
enum { SCR_MENU, SCR_JOIN, SCR_CONNECTING, SCR_GAME, SCR_PARK_SELECT, SCR_PARK };
enum { MODE_NONE, MODE_HOST, MODE_CLIENT };

static int       screen = SCR_MENU;
static int       mode = MODE_NONE;
static Server    server;
static Client    client;
static Discovery disc;
static int       disc_running = 0;

static GameState view, prev_view, demo;
static BotBrain  demo_brains[MAX_PLAYERS];
static int       my_slot = 0, have_view = 0;
static double    view_time = 0;
static char      ip_input[16] = "";
static uint32_t  connecting_ip = 0;
static char      message[128] = "";
static float     message_timer = 0;
static int       menu_open = 0;
static int       quit = 0;
static float     shake = 0;
static double    last_input_send = 0;
static uint8_t   my_fire_seq = 0;
static uint8_t   my_fire_mode = FIRE_FORWARD;
static int       touch_was[3] = { 0, 0, 0 };   /* OT, ORQA, TASHLA */

static float     vw = 1280, scale = 1, off_x = 0, off_y = 0;
/* parkovka rejimi (pastda ta'riflangan) */
static void park_open_select(void);
static int  park_total_stars(void);
static void draw_p_badge(float cx, float cy, float s);
static void draw_star(float cx, float cy, float r, Color c);
static void draw_car_icon(float cx, float cy, float s, Color c);
static int       mouse_consumed = 0;

/* ------------------------------------------------------------ Ovozlar */
enum { SND_ENGINE, SND_PICKUP, SND_SHUNT, SND_BOLT, SND_MINE, SND_BARGE, SND_SHIELD, SND_NITRO,
       SND_BOOM, SND_POP, SND_BEEP, SND_GO, SND_FINISH, SND_CLICK, SND_COUNT };
static Sound sounds[SND_COUNT];
static int   audio_ok = 0;

static Sound make_sound(int kind)
{
    const int rate = 22050;
    const float durs[SND_COUNT] = { 0.2f, 0.3f, 0.55f, 0.18f, 0.2f, 0.7f, 0.45f, 0.8f, 0.7f, 0.25f, 0.18f, 0.5f, 1.2f, 0.05f };
    int n = (int)(rate * durs[kind]);
    short *data = (short *)malloc(sizeof(short) * (size_t)n);
    float ph = 0, ph2 = 0, lp = 0;
    for (int i = 0; i < n; i++) {
        float k = (float)i / n, env = 1.0f - k, f, s = 0;
        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;
        switch (kind) {
        case SND_ENGINE:   /* aylanib turuvchi dvigatel - davr butun bo'lishi uchun 110 Hz */
            ph += 2 * PI * 110.0f / rate; ph2 += 2 * PI * 220.0f / rate;
            s = (fmodf(ph / (2 * PI), 1.0f) * 2 - 1) * 0.22f + sinf(ph2) * 0.12f + noise * 0.03f; break;
        case SND_PICKUP: f = k < 0.5f ? 880 : 1320; ph += 2 * PI * f / rate; s = sinf(ph) * 0.35f * env; break;
        case SND_SHUNT:  lp += (noise - lp) * 0.2f; f = 300 + 500 * k; ph += 2 * PI * f / rate; s = (lp * 0.9f + sinf(ph) * 0.25f) * sinf(PI * k); break;
        case SND_BOLT:   f = 1800 - 1400 * k; ph += 2 * PI * f / rate; s = (sinf(ph) > 0 ? 0.3f : -0.3f) * env; break;
        case SND_MINE:   ph += 2 * PI * 320 / rate; s = sinf(ph) * 0.4f * env * env; break;
        case SND_BARGE:  lp += (noise - lp) * 0.06f; ph += 2 * PI * (90 - 50 * k) / rate; s = (lp * 2.0f + sinf(ph) * 0.6f) * env; break;
        case SND_SHIELD: f = 400 + 800 * k; ph += 2 * PI * f / rate; ph2 += 2 * PI * f * 1.5f / rate; s = (sinf(ph) + sinf(ph2)) * 0.18f * sinf(PI * k); break;
        case SND_NITRO:  lp += (noise - lp) * (0.1f + 0.4f * k); s = lp * 1.4f * sinf(PI * k); break;
        case SND_BOOM:   lp += (noise - lp) * 0.09f; ph += 2 * PI * (70 - 40 * k) / rate; s = (lp * 2.2f + sinf(ph) * 0.5f) * env * env; break;
        case SND_POP:    ph += 2 * PI * (1200 - 800 * k) / rate; s = sinf(ph) * 0.3f * env; break;
        case SND_BEEP:   ph += 2 * PI * 660 / rate; s = sinf(ph) * 0.35f * (k < 0.8f ? 1 : env * 5); break;
        case SND_GO:     ph += 2 * PI * 1320 / rate; s = sinf(ph) * 0.35f * env; break;
        case SND_FINISH: { static const float nt[6] = { 523, 659, 784, 1047, 784, 1047 };
                           int idx = (int)(k * 6); ph += 2 * PI * nt[idx] / rate; s = sinf(ph) * 0.35f * (1 - fmodf(k * 6, 1.0f) * 0.4f); } break;
        default:         ph += 2 * PI * 1200 / rate; s = sinf(ph) * 0.3f * env; break;
        }
        if (s > 1) s = 1;
        if (s < -1) s = -1;
        data[i] = (short)(s * 28000);
    }
    Wave w = { (unsigned int)n, (unsigned int)rate, 16, 1, data };
    Sound snd = LoadSoundFromWave(w);
    free(data);
    return snd;
}

static void play(int kind, float vol)
{
    if (!audio_ok) return;
    SetSoundVolume(sounds[kind], vol);
    PlaySound(sounds[kind]);
}

/* ------------------------------------------------------------ Zarrachalar */
#define MAX_PARTS 900
typedef struct { Vector3 p, v; float life, max, size, grav; Color c; } Part;
static Part parts[MAX_PARTS];
static int  part_next = 0;

static float frand(float a, float b) { return a + (b - a) * (float)(rand() % 10000) / 10000.0f; }

static void emit(Vector3 p, Vector3 v, float life, float size, Color c, float grav)
{
    Part *q = &parts[part_next];
    part_next = (part_next + 1) % MAX_PARTS;
    q->p = p; q->v = v; q->life = q->max = life; q->size = size; q->c = c; q->grav = grav;
}

static void update_parts(float dt)
{
    for (int i = 0; i < MAX_PARTS; i++) {
        Part *q = &parts[i];
        if (q->life <= 0) continue;
        q->life -= dt;
        q->p.x += q->v.x * dt; q->p.y += q->v.y * dt; q->p.z += q->v.z * dt;
        q->v.y -= q->grav * dt;
        if (q->p.y < 0.05f) { q->p.y = 0.05f; q->v.y *= -0.3f; }
    }
}

/* zarba to'lqini halqalari */
#define MAX_RINGS 8
typedef struct { Vector3 p; float t; Color c; } Ring;
static Ring rings[MAX_RINGS];
static void add_ring(float x, float z, Color c)
{
    for (int i = 0; i < MAX_RINGS; i++) if (rings[i].t <= 0) { rings[i] = (Ring){ { x, 0.3f, z }, 0.6f, c }; return; }
}

/* ------------------------------------------------------------ Rang yordamchilari */
static Color lerp_color(Color a, Color b, float t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return (Color){ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                    (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}
static Color mul(Color c, float k)
{
    if (k < 0) k = 0;
    return (Color){ (unsigned char)fminf(255, c.r * k), (unsigned char)fminf(255, c.g * k), (unsigned char)fminf(255, c.b * k), c.a };
}
static uint32_t hash32(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}

/* =================================================================== */
/*                     3D: YORUG'LIK VA SHAKLLAR                       */
/* =================================================================== */
static const Vector3 LIGHT = { -0.38f, 0.82f, -0.43f };
static Vector3 cam_pos;
static float   fog_override = -1;

static float fog_amount(float x, float z)
{
    if (fog_override >= 0) return fog_override;
    float dx = x - cam_pos.x, dz = z - cam_pos.z;
    float f = (sqrtf(dx * dx + dz * dz) - 70.0f) / (DRAW_DIST - 80.0f);
    if (f < 0) f = 0;
    if (f > 1) f = 1;
    return f * f * 0.92f;
}
static Color fogc(Color c, float x, float z) { Color r = lerp_color(c, SKY_HORIZON, fog_amount(x, z)); r.a = c.a; return r; }
static Color lit(Color c, Vector3 n, float x, float z)
{
    float d = n.x * LIGHT.x + n.y * LIGHT.y + n.z * LIGHT.z;
    return fogc(mul(c, (0.58f + 0.47f * (d > 0 ? d : 0)) * TH->bright), x, z);
}

static void tri(Vector3 a, Vector3 b, Vector3 c, Color col)
{
    rlColor4ub(col.r, col.g, col.b, col.a);
    rlVertex3f(a.x, a.y, a.z); rlVertex3f(b.x, b.y, b.z); rlVertex3f(c.x, c.y, c.z);
}
static void quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color col) { tri(a, b, c, col); tri(a, c, d, col); }

static void sbox(Vector3 p, float w, float h, float l, Color c)
{
    float x0 = p.x - w / 2, x1 = p.x + w / 2, y0 = p.y - h / 2, y1 = p.y + h / 2, z0 = p.z - l / 2, z1 = p.z + l / 2;
    rlCheckRenderBatchLimit(36);
    rlBegin(RL_TRIANGLES);
    quad((Vector3){ x0, y1, z0 }, (Vector3){ x1, y1, z0 }, (Vector3){ x1, y1, z1 }, (Vector3){ x0, y1, z1 }, lit(c, (Vector3){ 0, 1, 0 }, p.x, p.z));
    quad((Vector3){ x0, y0, z0 }, (Vector3){ x1, y0, z0 }, (Vector3){ x1, y1, z0 }, (Vector3){ x0, y1, z0 }, lit(c, (Vector3){ 0, 0, -1 }, p.x, p.z));
    quad((Vector3){ x0, y0, z1 }, (Vector3){ x1, y0, z1 }, (Vector3){ x1, y1, z1 }, (Vector3){ x0, y1, z1 }, lit(c, (Vector3){ 0, 0, 1 }, p.x, p.z));
    quad((Vector3){ x0, y0, z0 }, (Vector3){ x0, y1, z0 }, (Vector3){ x0, y1, z1 }, (Vector3){ x0, y0, z1 }, lit(c, (Vector3){ -1, 0, 0 }, p.x, p.z));
    quad((Vector3){ x1, y0, z0 }, (Vector3){ x1, y1, z0 }, (Vector3){ x1, y1, z1 }, (Vector3){ x1, y0, z1 }, lit(c, (Vector3){ 1, 0, 0 }, p.x, p.z));
    rlEnd();
}
static void ebox(Vector3 p, float w, float h, float l, Color c) { DrawCube(p, w, h, l, fogc(c, p.x, p.z)); }

static void scone(Vector3 base, float r, float h, int sides, Color c)
{
    rlCheckRenderBatchLimit(sides * 3);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < sides; i++) {
        float a0 = 2 * PI * i / sides, a1 = 2 * PI * (i + 1) / sides, am = (a0 + a1) / 2;
        Vector3 v0 = { base.x + cosf(a0) * r, base.y, base.z + sinf(a0) * r };
        Vector3 v1 = { base.x + cosf(a1) * r, base.y, base.z + sinf(a1) * r };
        float nl = sqrtf(h * h + r * r);
        tri(v0, (Vector3){ base.x, base.y + h, base.z }, v1, lit(c, (Vector3){ cosf(am) * h / nl, r / nl, sinf(am) * h / nl }, base.x, base.z));
    }
    rlEnd();
}

static void extrude(const float pr[][2], int n, float halfw, Color side, Color top, Color cap)
{
    rlCheckRenderBatchLimit(n * 12);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < n; i++) {
        const float *a = pr[i], *b = pr[(i + 1) % n];
        float dz = b[0] - a[0], dy = b[1] - a[1], len = sqrtf(dz * dz + dy * dy);
        Vector3 nrm = { 0, -dz / len, dy / len };
        quad((Vector3){ -halfw, a[1], a[0] }, (Vector3){ halfw, a[1], a[0] }, (Vector3){ halfw, b[1], b[0] }, (Vector3){ -halfw, b[1], b[0] },
             lit(nrm.y > 0.8f ? top : side, nrm, 0, 0));
    }
    for (int s = -1; s <= 1; s += 2) {
        Color cc = lit(cap, (Vector3){ (float)s, 0, 0 }, 0, 0);
        for (int i = 1; i < n - 1; i++)
            tri((Vector3){ s * halfw, pr[0][1], pr[0][0] }, (Vector3){ s * halfw, pr[i][1], pr[i][0] }, (Vector3){ s * halfw, pr[i + 1][1], pr[i + 1][0] }, cc);
    }
    rlEnd();
}

/* =================================================================== */
/*                            3D SAHNA                                 */
/* =================================================================== */
static int in_view(float x, float z, float margin)
{
    float dx = x - cam_pos.x, dz = z - cam_pos.z;
    return dx * dx + dz * dz < (DRAW_DIST + margin) * (DRAW_DIST + margin);
}

static void draw_ground(void)
{
    rlCheckRenderBatchLimit(6);
    rlBegin(RL_TRIANGLES);
    float R = DRAW_DIST + 400;
    quad((Vector3){ cam_pos.x - R, -0.05f, cam_pos.z - R }, (Vector3){ cam_pos.x + R, -0.05f, cam_pos.z - R },
         (Vector3){ cam_pos.x + R, -0.05f, cam_pos.z + R }, (Vector3){ cam_pos.x - R, -0.05f, cam_pos.z + R }, lerp_color(GRASS, SKY_HORIZON, 0.12f));
    rlEnd();
    /* yaqindagi yer - dog'lar (o't / qum to'lqinlari / qor / shahar plitalari) */
    for (int gx = -6; gx <= 6; gx++)
        for (int gz = -6; gz <= 6; gz++) {
            float cx = floorf(cam_pos.x / 40) * 40 + gx * 40, cz = floorf(cam_pos.z / 40) * 40 + gz * 40;
            uint32_t h = hash32((uint32_t)((int)(cx / 40) * 7349 + (int)(cz / 40) * 31));
            if (TH->decor == DECOR_CITY) {
                Color c = fogc((h & 1) ? TH->ground2 : mul(TH->ground, 1.1f), cx + 20, cz + 20);
                rlCheckRenderBatchLimit(6);
                rlBegin(RL_TRIANGLES);
                quad((Vector3){ cx + 0.6f, -0.04f, cz + 0.6f }, (Vector3){ cx + 39.4f, -0.04f, cz + 0.6f },
                     (Vector3){ cx + 39.4f, -0.04f, cz + 39.4f }, (Vector3){ cx + 0.6f, -0.04f, cz + 39.4f }, c);
                rlEnd();
                continue;
            }
            if (h % 3 == 0) continue;
            float s = 14 + (float)(h % 18), ox = cx + (float)((h >> 5) % 20), oz = cz + (float)((h >> 9) % 20);
            Color c = fogc((h >> 3) & 1 ? TH->ground2 : mul(TH->ground, 0.94f), ox, oz);
            rlCheckRenderBatchLimit(18);
            rlBegin(RL_TRIANGLES);
            for (int k = 0; k < 6; k++) {   /* olti burchakli yumshoq dog' */
                float a0 = k * PI / 3, a1 = (k + 1) * PI / 3;
                tri((Vector3){ ox, -0.04f, oz }, (Vector3){ ox + cosf(a1) * s, -0.04f, oz + sinf(a1) * s * 0.7f },
                    (Vector3){ ox + cosf(a0) * s, -0.04f, oz + sinf(a0) * s * 0.7f }, c);
            }
            rlEnd();
        }
}

static void draw_track(void)
{
    const Track *T = &TRACK;
    const float H = TRACK_W / 2;
    const int city = TH->decor == DECOR_CITY;
    for (int i = 0; i < T->n; i++) {
        int j = (i + 1) % T->n;
        V2 a = T->p[i], b = T->p[j], na = T->nrm[i], nb = T->nrm[j];
        if (!in_view(a.x, a.z, 10)) continue;
        float fx = a.x, fz = a.z;
        rlCheckRenderBatchLimit(72);
        rlBegin(RL_TRIANGLES);
        /* yelka (yo'l chetidagi tuproq/qor/trotuar) */
        Color sh = fogc(city ? (Color){ 64, 66, 84, 255 } : lerp_color(TH->ground, TH->asphalt, 0.35f), fx, fz);
        for (int s = -1; s <= 1; s += 2) {
            float o0 = s * (H + 1.4f), o1 = s * (H + 6.8f);
            quad((Vector3){ a.x + na.x * o0, -0.01f, a.z + na.z * o0 }, (Vector3){ a.x + na.x * o1, -0.01f, a.z + na.z * o1 },
                 (Vector3){ b.x + nb.x * o1, -0.01f, b.z + nb.z * o1 }, (Vector3){ b.x + nb.x * o0, -0.01f, b.z + nb.z * o0 }, sh);
        }
        /* asfalt */
        Color rd = fogc(mul((i / 4) & 1 ? ASPHALT : mul(ASPHALT, 0.96f), TH->bright), fx, fz);
        quad((Vector3){ a.x + na.x * H, 0, a.z + na.z * H }, (Vector3){ a.x - na.x * H, 0, a.z - na.z * H },
             (Vector3){ b.x - nb.x * H, 0, b.z - nb.z * H }, (Vector3){ b.x + nb.x * H, 0, b.z + nb.z * H }, rd);
        /* chekka chiziqlar */
        Color wl = fogc(TH->night ? TH->line : mul(TH->line, TH->bright), fx, fz);
        for (int s = -1; s <= 1; s += 2) {
            float o0 = s * (H - 0.6f), o1 = s * (H - 0.9f);
            quad((Vector3){ a.x + na.x * o0, 0.01f, a.z + na.z * o0 }, (Vector3){ a.x + na.x * o1, 0.01f, a.z + na.z * o1 },
                 (Vector3){ b.x + nb.x * o1, 0.01f, b.z + nb.z * o1 }, (Vector3){ b.x + nb.x * o0, 0.01f, b.z + nb.z * o0 }, wl);
        }
        /* yo'lak chiziqlari (uzuq) */
        if ((i % 4) < 2)
            for (int l = -1; l <= 1; l++) {
                float o0 = l * 5.5f - 0.12f, o1 = l * 5.5f + 0.12f;
                quad((Vector3){ a.x + na.x * o0, 0.01f, a.z + na.z * o0 }, (Vector3){ a.x + na.x * o1, 0.01f, a.z + na.z * o1 },
                     (Vector3){ b.x + nb.x * o1, 0.01f, b.z + nb.z * o1 }, (Vector3){ b.x + nb.x * o0, 0.01f, b.z + nb.z * o0 }, wl);
            }
        /* bordyurlar - burilishlarda kengroq */
        float cw = 1.4f + fminf(fabsf(T->turn[i]) * 3.0f, 0.8f);
        Color cb = fogc(((i / 2) & 1) ? TH->curb_a : TH->curb_b, fx, fz);
        if (!TH->night) cb = fogc(mul(((i / 2) & 1) ? TH->curb_a : TH->curb_b, TH->bright), fx, fz);
        for (int s = -1; s <= 1; s += 2) {
            float o0 = s * H, o1 = s * (H + cw);
            quad((Vector3){ a.x + na.x * o0, 0.06f, a.z + na.z * o0 }, (Vector3){ a.x + na.x * o1, 0.06f, a.z + na.z * o1 },
                 (Vector3){ b.x + nb.x * o1, 0.06f, b.z + nb.z * o1 }, (Vector3){ b.x + nb.x * o0, 0.06f, b.z + nb.z * o0 }, cb);
        }
        /* to'siq devori */
        float wh = city ? 1.3f : 0.9f;
        Color wall = fogc(mul(TH->wall, TH->bright), fx, fz), walltop = fogc(TH->night ? TH->wall_top : mul(TH->wall_top, TH->bright), fx, fz);
        for (int s = -1; s <= 1; s += 2) {
            float o = s * (H + 6.8f);
            Vector3 a0 = { a.x + na.x * o, 0, a.z + na.z * o }, b0 = { b.x + nb.x * o, 0, b.z + nb.z * o };
            quad(a0, b0, (Vector3){ b0.x, wh, b0.z }, (Vector3){ a0.x, wh, a0.z }, (i & 1) ? wall : mul(wall, 0.9f));
            float o2 = s * (H + 7.3f);
            quad((Vector3){ a0.x, wh, a0.z }, (Vector3){ b0.x, wh, b0.z }, (Vector3){ b.x + nb.x * o2, wh, b.z + nb.z * o2 },
                 (Vector3){ a.x + na.x * o2, wh, a.z + na.z * o2 }, walltop);
            if (city) {   /* devordagi neon chiziq */
                Color ne = fogc(s < 0 ? TH->curb_a : TH->curb_b, fx, fz);
                Vector3 a1 = { a.x + na.x * (o - s * 0.05f), 0, a.z + na.z * (o - s * 0.05f) }, b1 = { b.x + nb.x * (o - s * 0.05f), 0, b.z + nb.z * (o - s * 0.05f) };
                quad((Vector3){ a1.x, 0.55f, a1.z }, (Vector3){ b1.x, 0.55f, b1.z }, (Vector3){ b1.x, 0.75f, b1.z }, (Vector3){ a1.x, 0.75f, a1.z }, ne);
            }
        }
        rlEnd();
        /* ustunlar: shaharda chiroq, qorda bayroqcha, cho'lda shina uyumi */
        if (i % 14 == 0) {
            for (int s = -1; s <= 1; s += 2) {
                float o = s * (H + 7.6f);
                Vector3 base = { a.x + na.x * o, 0, a.z + na.z * o };
                if (city) {
                    sbox((Vector3){ base.x, 4.5f, base.z }, 0.35f, 9.0f, 0.35f, (Color){ 60, 64, 80, 255 });
                    Vector3 lamp = { base.x - na.x * s * 2.2f, 9.0f, base.z - na.z * s * 2.2f };
                    sbox((Vector3){ (base.x + lamp.x) / 2, 9.0f, (base.z + lamp.z) / 2 }, 0.3f, 0.25f, 0.3f, (Color){ 60, 64, 80, 255 });
                    ebox(lamp, 1.2f, 0.25f, 1.2f, (Color){ 255, 236, 180, 255 });
                    fog_override = fog_amount(lamp.x, lamp.z);
                    DrawCylinder((Vector3){ lamp.x, 0.03f, lamp.z }, 5.5f, 5.5f, 0.01f, 16, Fade((Color){ 255, 220, 140, 255 }, 0.12f * (1 - fog_override)));
                    fog_override = -1;
                } else if (TH->decor == DECOR_SNOW) {
                    sbox((Vector3){ base.x, 1.6f, base.z }, 0.12f, 3.2f, 0.12f, (Color){ 40, 44, 60, 255 });
                    ebox((Vector3){ base.x, 2.8f, base.z }, 0.05f, 0.7f, 0.9f, (i / 14) & 1 ? (Color){ 37, 99, 235, 255 } : (Color){ 220, 38, 38, 255 });
                } else if (TH->decor == DECOR_DESERT) {
                    for (int t = 0; t < 3; t++)
                        DrawCylinder((Vector3){ base.x, 0.02f + t * 0.36f, base.z }, 0.7f, 0.7f, 0.34f, 10, fogc(t & 1 ? (Color){ 240, 240, 240, 255 } : (Color){ 30, 30, 34, 255 }, base.x, base.z));
                }
            }
        }
    }

    /* start/marra chizig'i va ark */
    V2 c = T->p[0], n0 = T->nrm[0], t0 = T->t[0];
    if (in_view(c.x, c.z, 20)) {
        for (int k = 0; k < 22; k++)
            for (int r = 0; r < 2; r++) {
                float o = -H + 0.5f + k, f = r * 1.0f;
                ebox((Vector3){ c.x + n0.x * o + t0.x * f, 0.02f, c.z + n0.z * o + t0.z * f }, 1.0f, 0.02f, 1.0f,
                     ((k + r) & 1) ? (Color){ 20, 20, 24, 255 } : WHITE);
            }
        float hd = atan2f(t0.x, t0.z) * RAD2DEG;
        for (int s = -1; s <= 1; s += 2)
            sbox((Vector3){ c.x + n0.x * s * (H + 2.0f), 4.0f, c.z + n0.z * s * (H + 2.0f) }, 1.0f, 8.0f, 1.0f, (Color){ 40, 44, 58, 255 });
        rlPushMatrix();
        rlTranslatef(c.x, 0, c.z);
        rlRotatef(hd, 0, 1, 0);
        fog_override = fog_amount(c.x, c.z);
        sbox((Vector3){ 0, 8.2f, 0 }, TRACK_W + 6, 1.8f, 0.8f, (Color){ 24, 26, 38, 255 });
        for (int k = 0; k < 13; k++)
            ebox((Vector3){ -TRACK_W / 2 - 2 + k * 2.0f, 7.25f, -0.42f }, 1.0f, 0.14f, 0.04f, (k & 1) ? TH->accent : WHITE);
        for (int k = 0; k < 13; k++)
            ebox((Vector3){ -TRACK_W / 2 - 2 + k * 2.0f, 7.25f, 0.42f }, 1.0f, 0.14f, 0.04f, (k & 1) ? TH->accent : WHITE);
        fog_override = -1;
        rlPopMatrix();
    }
}

/* --------------------------------------------------------- bezaklar */
static void deco_tree(V2 q, float s, uint32_t h)
{
    if (h % 6 == 0) { scone((Vector3){ q.x, 0, q.z }, 1.3f * s, 1.1f * s, 7, (Color){ 70, 140, 60, 255 }); return; }
    sbox((Vector3){ q.x, 1.0f * s, q.z }, 0.5f * s, 2.0f * s, 0.5f * s, (Color){ 120, 82, 52, 255 });
    Color leaf = (h >> 4) & 1 ? (Color){ 46, 132, 70, 255 } : (Color){ 60, 148, 78, 255 };
    if ((h >> 6) % 3 == 0) {   /* yumaloq bargli daraxt */
        Vector3 c = { q.x, 3.2f * s, q.z };
        DrawSphereEx(c, 2.1f * s, 6, 8, lit(mul(leaf, 1.08f), (Vector3){ 0, 1, 0 }, q.x, q.z));
        DrawSphereEx((Vector3){ c.x + 0.9f * s, c.y - 0.6f * s, c.z + 0.5f * s }, 1.5f * s, 6, 8, lit(leaf, (Vector3){ -0.4f, 0.6f, -0.4f }, q.x, q.z));
        return;
    }
    scone((Vector3){ q.x, 1.6f * s, q.z }, 2.2f * s, 3.4f * s, 8, leaf);
    scone((Vector3){ q.x, 3.3f * s, q.z }, 1.6f * s, 2.8f * s, 8, mul(leaf, 1.08f));
}

static void deco_desert(V2 q, float s, uint32_t h)
{
    int kind = h % 5;
    if (kind <= 1) {   /* kaktus: tana va ikki qo'l */
        Color g = (h >> 4) & 1 ? (Color){ 74, 140, 72, 255 } : (Color){ 92, 156, 80, 255 };
        float th = 0.7f * s, ht = 5.0f * s;
        sbox((Vector3){ q.x, ht / 2, q.z }, th, ht, th, g);
        sbox((Vector3){ q.x, ht + 0.12f * s, q.z }, th * 0.7f, 0.25f * s, th * 0.7f, g);
        float side = (h >> 8) & 1 ? 1.0f : -1.0f;
        sbox((Vector3){ q.x + side * 1.0f * s, ht * 0.42f, q.z }, 1.4f * s, 0.55f * s, 0.55f * s, g);
        sbox((Vector3){ q.x + side * 1.55f * s, ht * 0.58f, q.z }, 0.55f * s, 1.8f * s, 0.55f * s, g);
        sbox((Vector3){ q.x - side * 0.9f * s, ht * 0.6f, q.z }, 1.2f * s, 0.5f * s, 0.5f * s, g);
        sbox((Vector3){ q.x - side * 1.3f * s, ht * 0.74f, q.z }, 0.5f * s, 1.3f * s, 0.5f * s, g);
    } else if (kind <= 3) {   /* qizg'ish qoya */
        Color r = (h >> 5) & 1 ? (Color){ 186, 112, 70, 255 } : (Color){ 168, 98, 62, 255 };
        scone((Vector3){ q.x, 0, q.z }, 3.0f * s, 2.6f * s, 5, r);
        scone((Vector3){ q.x + 1.6f * s, 0, q.z + 0.8f * s }, 1.8f * s, 1.7f * s, 5, mul(r, 1.08f));
    } else {   /* tekis ustli qoya (mesa) */
        Color r = (Color){ 196, 124, 78, 255 };
        sbox((Vector3){ q.x, 2.6f * s, q.z }, 6.0f * s, 5.2f * s, 5.0f * s, r);
        sbox((Vector3){ q.x, 5.35f * s, q.z }, 6.4f * s, 0.4f * s, 5.4f * s, (Color){ 222, 158, 104, 255 });
        sbox((Vector3){ q.x, 1.6f * s, q.z }, 6.2f * s, 0.25f * s, 5.2f * s, (Color){ 160, 92, 58, 255 });
    }
}

static void deco_snow(V2 q, float s, uint32_t h)
{
    if (h % 7 == 0) {   /* qor uyumi */
        DrawSphereEx((Vector3){ q.x, -0.6f * s, q.z }, 2.4f * s, 6, 10, lit((Color){ 245, 248, 252, 255 }, (Vector3){ 0, 1, 0 }, q.x, q.z));
        return;
    }
    Color leaf = (h >> 4) & 1 ? (Color){ 30, 88, 66, 255 } : (Color){ 38, 100, 74, 255 };
    Color snow = (Color){ 246, 250, 255, 255 };
    float k = 1.15f;
    sbox((Vector3){ q.x, 0.8f * s, q.z }, 0.5f * s, 1.6f * s, 0.5f * s, (Color){ 96, 70, 52, 255 });
    scone((Vector3){ q.x, 1.2f * s, q.z }, 2.5f * s * k, 2.6f * s, 8, leaf);
    scone((Vector3){ q.x, 2.25f * s, q.z }, 1.6f * s * k, 1.55f * s, 8, snow);
    scone((Vector3){ q.x, 2.6f * s, q.z }, 1.9f * s * k, 2.3f * s, 8, mul(leaf, 1.08f));
    scone((Vector3){ q.x, 3.6f * s, q.z }, 1.2f * s * k, 1.3f * s, 8, snow);
    scone((Vector3){ q.x, 3.9f * s, q.z }, 1.2f * s * k, 2.0f * s, 8, mul(leaf, 1.15f));
    scone((Vector3){ q.x, 5.2f * s, q.z }, 0.6f * s * k, 0.7f * s, 8, snow);
}

static void deco_building(V2 q, float s, float clear, uint32_t h)
{
    float half = fminf(4.0f + (float)(h % 5) * 1.5f, clear - 1.0f);
    if (half < 2.5f) {   /* tor joy - ko'cha chirog'i o'rniga past bo'lak */
        sbox((Vector3){ q.x, 0.6f, q.z }, 2.0f, 1.2f, 2.0f, (Color){ 52, 56, 74, 255 });
        return;
    }
    float ht = 10.0f + (float)((h >> 4) % 30) * s;
    static const Color BODY[4] = { { 46, 52, 78, 255 }, { 58, 50, 84, 255 }, { 40, 60, 80, 255 }, { 64, 64, 80, 255 } };
    static const Color WIN[4] = { { 255, 220, 130, 255 }, { 120, 220, 255, 255 }, { 255, 140, 210, 255 }, { 255, 240, 200, 255 } };
    Color body = BODY[(h >> 9) & 3], win = WIN[(h >> 12) & 3];
    sbox((Vector3){ q.x, ht / 2, q.z }, half * 2, ht, half * 2, body);
    sbox((Vector3){ q.x, ht + 0.3f, q.z }, half * 1.4f, 0.6f, half * 1.4f, mul(body, 1.2f));
    if (!in_view(q.x, q.z, -DRAW_DIST * 0.35f)) return;   /* uzoqda derazalar chizilmaydi */
    /* derazalar: to'rt tomonda gorizontal yorug' qatorlar */
    int rows = (int)(ht / 3.2f);
    rlCheckRenderBatchLimit(rows * 4 * 6 + 12);
    rlBegin(RL_TRIANGLES);
    for (int r = 0; r < rows; r++) {
        uint32_t hr = hash32(h + (uint32_t)r * 131u);
        if (hr % 4 == 0) continue;   /* chirog'i o'chiq qavat */
        float y0 = 1.8f + r * 3.2f, y1 = y0 + 1.3f;
        Color c = fogc(hr % 5 == 0 ? mul(win, 0.6f) : win, q.x, q.z);
        float e = half + 0.03f, w = half * 0.82f;
        quad((Vector3){ q.x - w, y0, q.z + e }, (Vector3){ q.x + w, y0, q.z + e }, (Vector3){ q.x + w, y1, q.z + e }, (Vector3){ q.x - w, y1, q.z + e }, c);
        quad((Vector3){ q.x - w, y0, q.z - e }, (Vector3){ q.x + w, y0, q.z - e }, (Vector3){ q.x + w, y1, q.z - e }, (Vector3){ q.x - w, y1, q.z - e }, c);
        quad((Vector3){ q.x + e, y0, q.z - w }, (Vector3){ q.x + e, y0, q.z + w }, (Vector3){ q.x + e, y1, q.z + w }, (Vector3){ q.x + e, y1, q.z - w }, c);
        quad((Vector3){ q.x - e, y0, q.z - w }, (Vector3){ q.x - e, y0, q.z + w }, (Vector3){ q.x - e, y1, q.z + w }, (Vector3){ q.x - e, y1, q.z - w }, c);
    }
    rlEnd();
    if ((h >> 15) % 3 == 0)   /* tom ustida qizil signal chirog'i */
        ebox((Vector3){ q.x, ht + 1.0f, q.z }, 0.5f, 0.5f, 0.5f, ((int)(GetTime() * 2) + (int)h) & 1 ? (Color){ 255, 60, 60, 255 } : (Color){ 120, 20, 20, 255 });
}

static void draw_trees(void)
{
    const Track *T = &TRACK;
    for (int i = 0; i < T->tree_count; i++) {
        V2 q = T->trees[i];
        if (!in_view(q.x, q.z, 0)) continue;
        float s = T->tree_size[i];
        uint32_t h = hash32((uint32_t)i + 7u);
        switch (TH->decor) {
        case DECOR_DESERT: deco_desert(q, s, h); break;
        case DECOR_SNOW:   deco_snow(q, s, h); break;
        case DECOR_CITY:   deco_building(q, s, T->tree_clear[i], h); break;
        default:           deco_tree(q, s, h); break;
        }
    }
}

static void draw_hills(void)
{
    if (TH->decor == DECOR_CITY) {   /* uzoqdagi osmono'par binolar silueti */
        float R = DRAW_DIST + 140;
        rlCheckRenderBatchLimit(90 * 6);
        rlBegin(RL_TRIANGLES);
        for (int k = 0; k < 90; k++) {
            uint32_t h = hash32((uint32_t)k * 53u + 9u);
            float a0 = 2 * PI * k / 90, a1 = a0 + 2 * PI / 90 * (0.6f + (float)(h % 40) / 100.0f);
            float hh = 30.0f + (float)((h >> 8) % 70);
            Vector3 l = { cam_pos.x + sinf(a0) * R, -0.1f, cam_pos.z + cosf(a0) * R }, r = { cam_pos.x + sinf(a1) * R, -0.1f, cam_pos.z + cosf(a1) * R };
            Color c = (h >> 3) & 1 ? TH->hill1 : TH->hill2;
            quad(l, r, (Vector3){ r.x, hh, r.z }, (Vector3){ l.x, hh, l.z }, c);
        }
        rlEnd();
        return;
    }
    for (int layer = 0; layer < 2; layer++) {
        float R = DRAW_DIST + 120 + layer * 60;
        Color c = lerp_color(layer == 0 ? TH->hill1 : TH->hill2, SKY_HORIZON, 0.3f);
        float hmul = TH->decor == DECOR_SNOW ? 1.6f : TH->decor == DECOR_DESERT ? 0.8f : 1.0f;
        Color cap = TH->decor == DECOR_DESERT ? lerp_color(c, (Color){ 255, 210, 160, 255 }, 0.3f) : lerp_color(c, WHITE, TH->decor == DECOR_SNOW ? 0.75f : 0.3f);
        rlCheckRenderBatchLimit(40 * 6);
        rlBegin(RL_TRIANGLES);
        for (int k = 0; k < 40; k++) {
            uint32_t h = hash32((uint32_t)(k + layer * 100));
            float a = 2 * PI * (k + (float)(h % 100) / 100.0f) / 40;
            float hh = (22.0f + (float)((h >> 8) % 3000) / 100.0f + layer * 10) * hmul;
            float w = 0.12f + (float)((h >> 4) % 60) / 1000.0f;
            Vector3 top = { cam_pos.x + sinf(a) * R, hh, cam_pos.z + cosf(a) * R };
            Vector3 l = { cam_pos.x + sinf(a - w) * R, -0.1f, cam_pos.z + cosf(a - w) * R };
            Vector3 r = { cam_pos.x + sinf(a + w) * R, -0.1f, cam_pos.z + cosf(a + w) * R };
            if (TH->decor == DECOR_DESERT && (h & 1)) {   /* tekis cho'qqili mesa */
                Vector3 tl = { l.x + (top.x - l.x) * 0.7f, hh * 0.7f, l.z + (top.z - l.z) * 0.7f };
                Vector3 tr = { r.x + (top.x - r.x) * 0.7f, hh * 0.7f, r.z + (top.z - r.z) * 0.7f };
                quad(l, r, tr, tl, mul(c, 0.96f));
                continue;
            }
            tri(l, r, top, mul(c, 0.96f));
            /* qor qalpoq - tog'dan biroz oldinroqda (z-to'qnashuv bo'lmasin) */
            float cf = TH->decor == DECOR_SNOW ? 0.45f : 0.25f, pull = 0.985f;
            Vector3 tp = { cam_pos.x + (top.x - cam_pos.x) * pull, top.y, cam_pos.z + (top.z - cam_pos.z) * pull };
            Vector3 sl = { tp.x + (l.x - top.x) * cf, hh * (1 - cf), tp.z + (l.z - top.z) * cf };
            Vector3 sr = { tp.x + (r.x - top.x) * cf * 0.6f, hh * (1 - cf * 0.6f), tp.z + (r.z - top.z) * cf * 0.6f };
            tri(sl, sr, tp, cap);
        }
        rlEnd();
    }
}

/* mashina modeli sozlamalari (parkovkada: g'ildirak burilishi, orqa chiroq) */
static float car_wheel_steer = 0;   /* gradus, + chapga */
static int   car_lights = 1, car_reverse = 0;

static void draw_car_model(Color c, int boosting, float speed, float time, float roll)
{
    static const float BODY[][2] = { { -2.25f, 0.26f }, { 2.3f, 0.26f }, { 2.38f, 0.44f }, { 2.15f, 0.66f },
                                     { 0.7f, 0.82f }, { -1.9f, 0.88f }, { -2.25f, 0.8f } };
    static const float CABIN[][2] = { { -1.55f, 0.86f }, { 0.65f, 0.83f }, { -0.05f, 1.28f }, { -1.0f, 1.3f } };
    Color glass = TH->night ? (Color){ 30, 40, 70, 255 } : (Color){ 48, 66, 92, 255 }, dark = mul(c, 0.55f), carbon = (Color){ 34, 36, 44, 255 };
    DrawCube((Vector3){ 0, 0.008f, 0 }, 2.7f, 0.01f, 5.1f, Fade(BLACK, 0.14f));
    DrawCube((Vector3){ 0, 0.012f, 0 }, 2.2f, 0.01f, 4.5f, Fade(BLACK, 0.24f));
    if (TH->night && car_lights) {   /* faralar yo'lni yoritadi */
        rlCheckRenderBatchLimit(6);
        rlBegin(RL_TRIANGLES);
        Color l0 = Fade((Color){ 255, 240, 200, 255 }, 0.22f), l1 = Fade((Color){ 255, 240, 200, 255 }, 0.0f);
        rlColor4ub(l0.r, l0.g, l0.b, l0.a); rlVertex3f(-1.0f, 0.02f, 2.3f);
        rlColor4ub(l1.r, l1.g, l1.b, l1.a); rlVertex3f(-4.0f, 0.02f, 16.0f);
        rlColor4ub(l1.r, l1.g, l1.b, l1.a); rlVertex3f(4.0f, 0.02f, 16.0f);
        rlColor4ub(l0.r, l0.g, l0.b, l0.a); rlVertex3f(-1.0f, 0.02f, 2.3f);
        rlColor4ub(l1.r, l1.g, l1.b, l1.a); rlVertex3f(4.0f, 0.02f, 16.0f);
        rlColor4ub(l0.r, l0.g, l0.b, l0.a); rlVertex3f(1.0f, 0.02f, 2.3f);
        rlEnd();
    }
    float b = sinf(time * 22) * 0.012f * fminf(fabsf(speed) / 30.0f, 1.0f);
    rlPushMatrix();
    rlTranslatef(0, b + 0.36f, 0);
    rlRotatef(roll, 0, 0, 1);       /* burilishda kuzov tashqariga og'adi */
    rlTranslatef(0, -0.36f, 0);
    extrude(BODY, 7, 1.0f, c, c, mul(c, 0.95f));
    extrude(CABIN, 4, 0.78f, glass, c, glass);
    /* poyga chiziqlari */
    sbox((Vector3){ 0, 0.885f, -0.6f }, 0.36f, 0.03f, 2.6f, lerp_color(c, WHITE, 0.7f));
    sbox((Vector3){ 0, 1.305f, -0.52f }, 0.36f, 0.02f, 0.9f, lerp_color(c, WHITE, 0.7f));
    sbox((Vector3){ 0, 0.69f, 1.4f }, 0.36f, 0.02f, 1.4f, lerp_color(c, WHITE, 0.7f));
    /* g'ildirak qanotlari va yon pog'ona */
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2)
            sbox((Vector3){ sx * 0.98f, 0.66f, sz * 1.38f }, 0.14f, 0.16f, 1.05f, mul(c, 0.85f));
        sbox((Vector3){ sx * 1.02f, 0.3f, 0 }, 0.08f, 0.14f, 1.6f, carbon);
        sbox((Vector3){ sx * 0.86f, 1.02f, 0.45f }, 0.22f, 0.12f, 0.16f, dark);   /* oyna (ko'zgu) */
    }
    /* orqa qanot (spoyler) */
    sbox((Vector3){ 0, 1.06f, -2.08f }, 2.0f, 0.06f, 0.46f, carbon);
    sbox((Vector3){ -0.7f, 0.95f, -2.02f }, 0.08f, 0.18f, 0.24f, carbon);
    sbox((Vector3){ 0.7f, 0.95f, -2.02f }, 0.08f, 0.18f, 0.24f, carbon);
    for (int sx = -1; sx <= 1; sx += 2) sbox((Vector3){ sx * 1.0f, 1.1f, -2.08f }, 0.04f, 0.2f, 0.5f, c);
    /* old splitter */
    sbox((Vector3){ 0, 0.24f, 2.32f }, 2.0f, 0.06f, 0.24f, carbon);
    /* chiroqlar */
    Color tail = { 255, 50, 60, 255 }, head = { 255, 248, 210, 255 };
    ebox((Vector3){ -0.62f, 0.64f, -2.26f }, 0.56f, 0.1f, 0.04f, tail);
    ebox((Vector3){ 0.62f, 0.64f, -2.26f }, 0.56f, 0.1f, 0.04f, tail);
    ebox((Vector3){ -0.7f, 0.56f, 2.27f }, 0.45f, 0.12f, 0.04f, head);
    ebox((Vector3){ 0.7f, 0.56f, 2.27f }, 0.45f, 0.12f, 0.04f, head);
    ebox((Vector3){ 0, 0.38f, -2.27f }, 1.5f, 0.14f, 0.04f, (Color){ 30, 30, 34, 255 });
    DrawCylinderEx((Vector3){ -0.45f, 0.36f, -2.2f }, (Vector3){ -0.45f, 0.36f, -2.36f }, 0.09f, 0.09f, 8, (Color){ 170, 170, 180, 255 });
    DrawCylinderEx((Vector3){ 0.45f, 0.36f, -2.2f }, (Vector3){ 0.45f, 0.36f, -2.36f }, 0.09f, 0.09f, 8, (Color){ 170, 170, 180, 255 });
    if (car_reverse) {   /* orqaga yurish chirog'i */
        ebox((Vector3){ -0.25f, 0.64f, -2.27f }, 0.2f, 0.1f, 0.04f, WHITE);
        ebox((Vector3){ 0.25f, 0.64f, -2.27f }, 0.2f, 0.1f, 0.04f, WHITE);
    }
    if (TH->night && car_lights) {
        DrawSphereEx((Vector3){ -0.7f, 0.56f, 2.32f }, 0.3f, 6, 8, Fade(head, 0.25f));
        DrawSphereEx((Vector3){ 0.7f, 0.56f, 2.32f }, 0.3f, 6, 8, Fade(head, 0.25f));
        DrawSphereEx((Vector3){ -0.62f, 0.64f, -2.3f }, 0.28f, 6, 8, Fade(tail, 0.25f));
        DrawSphereEx((Vector3){ 0.62f, 0.64f, -2.3f }, 0.28f, 6, 8, Fade(tail, 0.25f));
    }
    rlPopMatrix();
    float spin = time * speed * 2.6f;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            rlPushMatrix();
            rlTranslatef(sx * 0.98f, 0.36f, sz * 1.38f);
            if (sz > 0 && car_wheel_steer != 0) rlRotatef(car_wheel_steer, 0, 1, 0);   /* oldingi g'ildiraklar buriladi */
            Vector3 a = { -sx * 0.12f, 0, 0 }, e = { sx * 0.12f, 0, 0 };
            DrawCylinderEx(a, e, 0.37f, 0.37f, 14, (Color){ 24, 24, 28, 255 });
            Vector3 hub = { e.x + sx * 0.012f, 0, 0 };
            DrawCylinderEx(e, hub, 0.23f, 0.23f, 12, (Color){ 190, 194, 204, 255 });
            for (int k = 0; k < 3; k++) {   /* aylanuvchi parraklar */
                float a2 = spin + k * PI / 3;
                DrawCube((Vector3){ hub.x + sx * 0.004f, 0, 0 }, 0.02f, 0.4f * fabsf(cosf(a2)) + 0.05f, 0.4f * fabsf(sinf(a2)) + 0.05f, (Color){ 120, 124, 134, 255 });
            }
            rlPopMatrix();
        }
    if (boosting) {
        float f = 0.6f + 0.3f * sinf(time * 40);
        for (int sx = -1; sx <= 1; sx += 2) {
            DrawCylinderEx((Vector3){ sx * 0.45f, 0.36f, -2.36f }, (Vector3){ sx * 0.45f, 0.36f, -2.36f - f * 1.6f }, 0.2f, 0.0f, 8, Fade((Color){ 120, 210, 255, 255 }, 0.9f));
            DrawCylinderEx((Vector3){ sx * 0.45f, 0.36f, -2.36f }, (Vector3){ sx * 0.45f, 0.36f, -2.36f - f * 2.6f }, 0.32f, 0.0f, 8, Fade((Color){ 60, 140, 255, 255 }, 0.35f));
        }
    }
}

/* mijozda holatlar orasidagi silliq harakat */
static void car_pose(const Player *p, float since, float *x, float *z)
{
    *x = p->x + sinf(p->heading) * p->speed * since;
    *z = p->z + cosf(p->heading) * p->speed * since;
}

static float car_last_heading[MAX_PLAYERS], car_roll[MAX_PLAYERS];
static double car_last_t[MAX_PLAYERS];

static void draw_car(const Player *p, int slot, float since, float time)
{
    float x, z;
    car_pose(p, since, &x, &z);
    /* og'ish: burilish tezligidan */
    double now = GetTime();
    float dtt = (float)(now - car_last_t[slot]);
    if (dtt > 0.0001f && dtt < 0.5f) {
        float d = p->heading - car_last_heading[slot];
        while (d > PI) d -= 2 * PI;
        while (d < -PI) d += 2 * PI;
        float want = fmaxf(-6.0f, fminf(6.0f, d / dtt * fabsf(p->speed) * 0.22f));
        car_roll[slot] += (want - car_roll[slot]) * fminf(1, dtt * 8);
    }
    car_last_heading[slot] = p->heading;
    car_last_t[slot] = now;
    if (!in_view(x, z, 10)) return;
    float spin = p->stun > 0 ? p->stun * 9.0f : 0;
    fog_override = fog_amount(x, z);
    rlPushMatrix();
    rlTranslatef(x, p->stun > 0 ? fabsf(sinf(p->stun * 6)) * 0.6f : 0, z);
    rlRotatef((p->heading + spin) * RAD2DEG, 0, 1, 0);
    draw_car_model(CAR_COLORS[slot], p->nitro > 0, p->speed, time + slot, car_roll[slot]);
    rlPopMatrix();
    fog_override = -1;
    if (p->shield > 0) {
        float pulse = 0.5f + 0.5f * sinf(time * 6);
        DrawSphereEx((Vector3){ x, 0.9f, z }, 3.1f, 10, 14, Fade((Color){ 80, 230, 140, 255 }, 0.14f + 0.06f * pulse));
        DrawSphereWires((Vector3){ x, 0.9f, z }, 3.15f, 6, 12, Fade((Color){ 120, 255, 170, 255 }, 0.35f));
    }
}

/* ----------------------------------------------- qurollarning 3D shakllari
 * Har bir qurol o'z shakliga ega - qutini uzoqdan ko'rib qaysi qurolligini bilish mumkin. */
static void model_rocket(Color c, float t)
{
    /* tana (z o'qi bo'ylab), uchli burun, 4 ta qanot, olov */
    DrawCylinderEx((Vector3){ 0, 0, -0.9f }, (Vector3){ 0, 0, 0.6f }, 0.32f, 0.32f, 12, c);
    DrawCylinderEx((Vector3){ 0, 0, 0.6f }, (Vector3){ 0, 0, 1.3f }, 0.32f, 0.0f, 12, (Color){ 240, 240, 245, 255 });
    DrawCylinderEx((Vector3){ 0, 0, 0.1f }, (Vector3){ 0, 0, 0.3f }, 0.335f, 0.335f, 12, (Color){ 240, 240, 245, 255 });
    for (int k = 0; k < 4; k++) {
        float a = k * PI / 2 + PI / 4;
        DrawCube((Vector3){ cosf(a) * 0.45f, sinf(a) * 0.45f, -0.7f }, fabsf(cosf(a)) * 0.35f + 0.05f, fabsf(sinf(a)) * 0.35f + 0.05f, 0.5f, mul(c, 0.7f));
    }
    float f = 0.5f + 0.2f * sinf(t * 50);
    DrawCylinderEx((Vector3){ 0, 0, -0.9f }, (Vector3){ 0, 0, -0.9f - f }, 0.26f, 0.0f, 8, (Color){ 255, 200, 80, 230 });
}

static void model_bolt(Color c)
{
    /* zigzag chaqmoq: uchta qiya bo'lak */
    rlPushMatrix(); rlTranslatef(0.18f, 0.55f, 0); rlRotatef(-28, 0, 0, 1); DrawCube((Vector3){ 0, 0, 0 }, 0.34f, 0.9f, 0.3f, c); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(0, 0.05f, 0); rlRotatef(62, 0, 0, 1); DrawCube((Vector3){ 0, 0, 0 }, 0.3f, 0.75f, 0.3f, c); rlPopMatrix();
    rlPushMatrix(); rlTranslatef(-0.18f, -0.5f, 0); rlRotatef(-28, 0, 0, 1); DrawCube((Vector3){ 0, 0, 0 }, 0.34f, 0.9f, 0.3f, c); rlPopMatrix();
    DrawCylinderEx((Vector3){ -0.3f, -0.85f, 0 }, (Vector3){ -0.42f, -1.15f, 0 }, 0.17f, 0.0f, 6, c);
}

static void model_mine(Color c, float t, int armed)
{
    DrawSphereEx((Vector3){ 0, 0, 0 }, 0.55f, 8, 10, c);
    static const Vector3 D[8] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0.6f, 0.6f, 0.6f }, { -0.6f, 0.6f, -0.6f } };
    for (int k = 0; k < 8; k++)
        DrawCylinderEx((Vector3){ D[k].x * 0.45f, D[k].y * 0.45f, D[k].z * 0.45f }, (Vector3){ D[k].x * 0.95f, D[k].y * 0.95f, D[k].z * 0.95f }, 0.13f, 0.0f, 6, (Color){ 170, 172, 182, 255 });
    int blink = armed && ((int)(t * 4) & 1);
    DrawSphereEx((Vector3){ 0, 0.5f, 0 }, 0.16f, 6, 8, blink ? (Color){ 255, 60, 40, 255 } : (Color){ 255, 240, 200, 255 });
}

static void model_nitro(Color c, float t)
{
    /* ballon + ventil + olov tillari */
    DrawCylinderEx((Vector3){ 0, -0.7f, 0 }, (Vector3){ 0, 0.4f, 0 }, 0.42f, 0.42f, 14, c);
    DrawSphereEx((Vector3){ 0, 0.4f, 0 }, 0.42f, 8, 12, c);
    DrawCylinderEx((Vector3){ 0, -0.2f, 0 }, (Vector3){ 0, 0.05f, 0 }, 0.44f, 0.44f, 14, (Color){ 240, 240, 245, 255 });
    DrawCylinderEx((Vector3){ 0, 0.75f, 0 }, (Vector3){ 0, 0.95f, 0 }, 0.12f, 0.12f, 8, (Color){ 180, 184, 196, 255 });
    for (int k = 0; k < 3; k++) {
        float a = t * 3 + k * 2.1f, h = 0.5f + 0.15f * sinf(t * 20 + k);
        Vector3 base = { cosf(a) * 0.18f, 1.0f, sinf(a) * 0.18f };
        DrawCylinderEx(base, (Vector3){ base.x, base.y + h, base.z }, 0.15f, 0.0f, 6, k == 1 ? (Color){ 255, 255, 255, 230 } : (Color){ 120, 220, 255, 230 });
    }
}

static void model_barge(Color c, float t)
{
    DrawSphereEx((Vector3){ 0, 0, 0 }, 0.42f, 8, 10, c);
    for (int k = 0; k < 3; k++) {
        rlPushMatrix();
        rlRotatef(t * 120 + k * 60, 0, 1, 0);
        rlRotatef(60 * k + 20, 1, 0, 0);
        for (int w = 0; w < 3; w++)
            DrawCircle3D((Vector3){ 0, 0, 0 }, 0.8f + w * 0.02f, (Vector3){ 1, 0, 0 }, 90, lerp_color(c, WHITE, 0.3f));
        rlPopMatrix();
    }
    DrawSphereEx((Vector3){ 0, 0, 0 }, 0.9f, 8, 10, Fade(c, 0.18f));
}

static void model_shield(Color c)
{
    /* tik turgan olti burchakli qalqon, ichida yorqin belgi */
    rlPushMatrix();
    rlRotatef(90, 1, 0, 0);
    DrawCylinder((Vector3){ 0, -0.1f, 0 }, 0.9f, 0.9f, 0.2f, 6, c);
    DrawCylinder((Vector3){ 0, -0.13f, 0 }, 0.6f, 0.6f, 0.26f, 6, lerp_color(c, WHITE, 0.45f));
    DrawCylinder((Vector3){ 0, -0.16f, 0 }, 0.28f, 0.28f, 0.32f, 6, c);
    rlPopMatrix();
}

static void draw_item_model(int item, float t)
{
    Color c = ITEM_COLORS[item];
    switch (item) {
    case ITEM_NITRO:  model_nitro(c, t); break;
    case ITEM_SHUNT:  rlPushMatrix(); rlRotatef(-35, 1, 0, 0); model_rocket(c, t); rlPopMatrix(); break;
    case ITEM_BOLT:   model_bolt(c); break;
    case ITEM_MINE:   model_mine(c, t, 0); break;
    case ITEM_BARGE:  model_barge(c, t); break;
    case ITEM_SHIELD: model_shield(c); break;
    }
}

static void draw_items_world(const GameState *g, float time)
{
    /* qurol qutilari: har birining o'z shakli va rangi bor */
    for (int k = 0; k < PICKUPS; k++) {
        const Pickup *pk = &g->pickups[k];
        if (!pk->active || !in_view(pk->x, pk->z, 0) || pk->type == 0 || pk->type >= ITEM_COUNT) continue;
        float bob = sinf(time * 3 + k) * 0.25f;
        Color c = ITEM_COLORS[pk->type];
        float fa = fog_amount(pk->x, pk->z);
        /* yerdagi nur halqasi va yorug' ustun */
        DrawCylinder((Vector3){ pk->x, 0.03f, pk->z }, 1.6f, 1.6f, 0.02f, 20, Fade(c, 0.45f * (1 - fa)));
        DrawCircle3D((Vector3){ pk->x, 0.06f, pk->z }, 1.9f + 0.2f * sinf(time * 4 + k), (Vector3){ 1, 0, 0 }, 90, Fade(c, 0.9f * (1 - fa)));
        rlPushMatrix();
        rlTranslatef(pk->x, 1.9f + bob, pk->z);
        rlRotatef(time * 80 + k * 20, 0, 1, 0);
        rlScalef(1.45f, 1.45f, 1.45f);
        draw_item_model(pk->type, time + k);
        rlPopMatrix();
        /* qurol ostidagi yorug' nur (shaffof - oxirida, chuqurlikka yozmasdan) */
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        DrawCylinder((Vector3){ pk->x, 0.05f, pk->z }, 1.1f, 0.2f, 3.2f, 12, Fade(c, 0.16f * (1 - fa)));
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
    }
    /* snaryadlar */
    for (int k = 0; k < MAX_PROJ; k++) {
        const Projectile *pr = &g->proj[k];
        if (!pr->active || !in_view(pr->x, pr->z, 0)) continue;
        if (pr->type == ITEM_SHUNT) {
            rlPushMatrix();
            rlTranslatef(pr->x, 1.3f, pr->z);
            rlRotatef(pr->heading * RAD2DEG, 0, 1, 0);
            rlScalef(1.3f, 1.3f, 1.3f);
            model_rocket(ITEM_COLORS[ITEM_SHUNT], time);
            rlPopMatrix();
            DrawSphereEx((Vector3){ pr->x, 1.3f, pr->z }, 1.4f, 8, 10, Fade((Color){ 255, 60, 50, 255 }, 0.18f));
        } else {
            rlPushMatrix();
            rlTranslatef(pr->x, 1.0f, pr->z);
            rlRotatef(pr->heading * RAD2DEG, 0, 1, 0);
            for (int s = 0; s < 4; s++) {   /* zigzag elektr yoyi */
                float z0 = -1.6f + s * 0.8f, x0 = (s & 1) ? 0.25f : -0.25f;
                DrawCylinderEx((Vector3){ x0, 0, z0 }, (Vector3){ -x0, 0, z0 + 0.8f }, 0.12f, 0.12f, 6, (Color){ 255, 250, 200, 255 });
            }
            DrawCube((Vector3){ 0, 0, 0 }, 0.9f, 0.9f, 3.4f, Fade((Color){ 250, 204, 21, 255 }, 0.28f));
            rlPopMatrix();
        }
    }
    /* minalar */
    for (int k = 0; k < MAX_MINES; k++) {
        const Mine *m = &g->mines[k];
        if (!m->active || !in_view(m->x, m->z, 0)) continue;
        rlPushMatrix();
        rlTranslatef(m->x, 0.6f, m->z);
        rlRotatef(time * 40 + k * 30, 0, 1, 0);
        model_mine(ITEM_COLORS[ITEM_MINE], time, m->arm <= 0);
        rlPopMatrix();
        if (m->arm <= 0 && ((int)(time * 4) & 1)) DrawCylinder((Vector3){ m->x, 0.03f, m->z }, 2.2f, 2.2f, 0.01f, 16, Fade((Color){ 255, 60, 40, 255 }, 0.25f));
    }
    /* zarba to'lqinlari */
    for (int i = 0; i < MAX_RINGS; i++) {
        if (rings[i].t <= 0) continue;
        float k = 1 - rings[i].t / 0.6f, r = 2 + 14 * k;
        for (int w = 0; w < 3; w++)
            DrawCircle3D((Vector3){ rings[i].p.x, 0.3f + w * 0.4f, rings[i].p.z }, r - w * 0.3f, (Vector3){ 1, 0, 0 }, 90, Fade(rings[i].c, 1 - k));
    }
}

static float cam_heading = 0, smooth_fov = 60;
static Vector3 cam_smooth;
static int cam_init = 0;

static Camera3D make_camera(const Player *p, float since, float dt)
{
    float x, z;
    car_pose(p, since, &x, &z);
    float d = fmodf(p->heading - cam_heading + PI, 2 * PI);
    if (d < 0) d += 2 * PI;
    d -= PI;
    if (!cam_init) { cam_heading = p->heading; d = 0; }
    cam_heading += d * fminf(1, dt * 5.0f);
    Vector3 want = { x - sinf(cam_heading) * 9.5f, 4.2f, z - cosf(cam_heading) * 9.5f };
    if (!cam_init) { cam_smooth = want; cam_init = 1; }
    cam_smooth.x += (want.x - cam_smooth.x) * fminf(1, dt * 12);
    cam_smooth.y += (want.y - cam_smooth.y) * fminf(1, dt * 12);
    cam_smooth.z += (want.z - cam_smooth.z) * fminf(1, dt * 12);
    Camera3D cam = { 0 };
    cam.position = cam_smooth;
    cam.target = (Vector3){ x + sinf(cam_heading) * 12.0f, 1.3f, z + cosf(cam_heading) * 12.0f };
    cam.up = (Vector3){ 0, 1, 0 };
    float want_fov = 58.0f + 10.0f * fminf(fabsf(p->speed) / NITRO_SPEED, 1.0f) + (p->nitro > 0 ? 8.0f : 0);
    smooth_fov += (want_fov - smooth_fov) * fminf(1, dt * 4);
    cam.fovy = smooth_fov;
    cam.projection = CAMERA_PERSPECTIVE;
    if (shake > 0) { cam.position.x += frand(-1, 1) * shake * 0.5f; cam.position.y += frand(-1, 1) * shake * 0.4f; }
    cam_pos = cam.position;
    return cam;
}

static void draw_sky(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangleGradientV(0, 0, sw, (int)(sh * 0.6f), SKY_TOP, SKY_HORIZON);
    DrawRectangle(0, (int)(sh * 0.6f), sw, sh, SKY_HORIZON);
    float t = (float)GetTime();
    float pan = -cam_heading * 300 * scale;
    if (TH->night) {   /* yulduzlar va oy */
        for (int i = 0; i < 120; i++) {
            uint32_t h = hash32((uint32_t)i * 2654435761u + 1);
            float x = fmodf((float)(h % 4000) / 4000.0f * (sw + 600) + pan * 0.5f, (float)sw + 600);
            if (x < 0) x += sw + 600;
            float y = sh * (float)((h >> 12) % 1000) / 1000.0f * 0.45f;
            float tw = 0.5f + 0.5f * sinf(t * (1 + (h % 7)) + i);
            DrawCircleV((Vector2){ x - 300, y }, (1.0f + (h >> 22) % 2) * scale, Fade(WHITE, 0.35f + 0.5f * tw));
        }
        float mx = fmodf(sw * 0.75f + pan * 0.5f, (float)sw + 600);
        if (mx < 0) mx += sw + 600;
        DrawCircleV((Vector2){ mx - 300, sh * 0.14f }, 70 * scale, Fade((Color){ 200, 210, 255, 255 }, 0.08f));
        DrawCircleV((Vector2){ mx - 300, sh * 0.14f }, 34 * scale, (Color){ 240, 240, 225, 255 });
        DrawCircleV((Vector2){ mx - 290, sh * 0.13f }, 7 * scale, (Color){ 220, 220, 205, 255 });
        DrawCircleV((Vector2){ mx - 312, sh * 0.155f }, 5 * scale, (Color){ 220, 220, 205, 255 });
        return;
    }
    if (TH->decor == DECOR_DESERT) {   /* jazirama quyosh */
        float sx = fmodf(sw * 0.3f + pan * 0.5f, (float)sw + 600);
        if (sx < 0) sx += sw + 600;
        DrawCircleV((Vector2){ sx - 300, sh * 0.2f }, 110 * scale, Fade((Color){ 255, 220, 150, 255 }, 0.25f));
        DrawCircleV((Vector2){ sx - 300, sh * 0.2f }, 52 * scale, (Color){ 255, 244, 214, 255 });
    }
    int clouds = TH->decor == DECOR_DESERT ? 2 : TH->decor == DECOR_SNOW ? 9 : 6;
    for (int i = 0; i < clouds; i++) {
        uint32_t h = hash32((uint32_t)i * 7919u + 3);
        float x = fmodf((float)(h % 2000) / 2000.0f * (sw + 400) + t * (6 + i * 2) * scale - cam_heading * 300 * scale, (float)sw + 400);
        if (x < 0) x += sw + 400;
        x -= 200;
        float y = sh * (0.05f + (float)((h >> 11) % 100) / 100.0f * 0.2f);
        float s = scale * (0.7f + (float)((h >> 20) % 60) / 100.0f);
        Color c = Fade(TH->decor == DECOR_SNOW ? (Color){ 236, 240, 248, 255 } : WHITE, 0.85f);
        DrawCircleV((Vector2){ x, y }, 26 * s, c);
        DrawCircleV((Vector2){ x + 30 * s, y - 10 * s }, 32 * s, c);
        DrawCircleV((Vector2){ x + 64 * s, y }, 24 * s, c);
        DrawRectangleRounded((Rectangle){ x - 20 * s, y, 104 * s, 24 * s }, 1, 8, c);
    }
}

static void draw_scene(const GameState *g, int me, float since, float time, float dt)
{
    int focus = (me >= 0 && g->players[me].active) ? me : 0;
    Camera3D cam = make_camera(&g->players[focus], since, dt);
    draw_sky();
    BeginMode3D(cam);
    rlDisableBackfaceCulling();
    draw_hills();
    draw_ground();
    draw_track();
    draw_trees();
    draw_items_world(g, time);
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (g->players[i].active) draw_car(&g->players[i], i, since, time);
    for (int i = 0; i < MAX_PARTS; i++) {
        Part *q = &parts[i];
        if (q->life <= 0) continue;
        Color c = q->c;
        c.a = (unsigned char)(c.a * q->life / q->max);
        float s = q->size * (0.4f + 0.6f * q->life / q->max);
        DrawCube(q->p, s, s, s, c);
    }
    if (TH->decor == DECOR_SNOW) {   /* yog'ayotgan qor */
        for (int i = 0; i < 420; i++) {
            uint32_t h = hash32((uint32_t)i * 7919u + 17u);
            float fx = (float)(h % 1000) / 1000.0f * 120 - 60, fz = (float)((h >> 10) % 1000) / 1000.0f * 120 - 60;
            float fy = fmodf((float)((h >> 20) % 1000) / 1000.0f * 24 - time * (2.5f + (h % 3)), 24.0f);
            if (fy < 0) fy += 24;
            float wx = cam_pos.x + fx + sinf(time + i) * 0.8f, wz = cam_pos.z + fz;
            /* kameraga nisbatan qo'zg'almas to'r - harakatlanganda qor "qochmasin" */
            wx = cam_pos.x + fmodf(wx - cam_pos.x + 6000 - fmodf(cam_pos.x, 120), 120) - 60;
            wz = cam_pos.z + fmodf(wz - cam_pos.z + 6000 - fmodf(cam_pos.z, 120), 120) - 60;
            DrawCube((Vector3){ wx, fy, wz }, 0.2f, 0.2f, 0.2f, Fade(WHITE, 0.9f));
        }
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();

    if (me < 0) return;
    /* boshqa mashinalar ustida nomlar */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        if (!p->active || i == me) continue;
        float x, z;
        car_pose(p, since, &x, &z);
        float dx = x - cam_pos.x, dz = z - cam_pos.z;
        if (dx * sinf(cam_heading) + dz * cosf(cam_heading) < 2 || dx * dx + dz * dz > 160 * 160) continue;
        Vector2 sp = GetWorldToScreen((Vector3){ x, 2.4f, z }, cam);
        const char *name = p->is_bot ? "Bot" : game_color_name(i);
        float fs = 15 * scale, pad = 8 * scale, h = fs + 10 * scale;
        float w = twidth(F_SEMI, name, fs) + pad * 2 + 10 * scale;
        DrawRectangleRounded((Rectangle){ sp.x - w / 2, sp.y - h, w, h }, 1.0f, 10, Fade((Color){ 12, 14, 22, 255 }, 0.8f));
        DrawCircleV((Vector2){ sp.x - w / 2 + pad + 3 * scale, sp.y - h / 2 }, 4 * scale, CAR_COLORS[i]);
        tmid(F_SEMI, name, sp.x - w / 2 + pad + 12 * scale, sp.y - h / 2, fs, WHITE);
    }
}

static void spawn_effects(const GameState *g, float since)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        if (!p->active) continue;
        float x, z;
        car_pose(p, since, &x, &z);
        float bx = -sinf(p->heading), bz = -cosf(p->heading);
        if (p->nitro > 0) {
            for (int k = 0; k < 2; k++) {
                float sx = (k ? 0.45f : -0.45f);
                emit((Vector3){ x + bx * 2.6f + cosf(p->heading) * sx, 0.42f, z + bz * 2.6f - sinf(p->heading) * sx },
                     (Vector3){ bx * frand(4, 9), frand(0.3f, 1.2f), bz * frand(4, 9) }, 0.3f, 0.2f,
                     rand() % 2 ? (Color){ 255, 170, 60, 230 } : (Color){ 120, 210, 255, 230 }, 0);
            }
        }
        if (fabsf(p->lat) > TRACK_W / 2 + 0.6f && fabsf(p->speed) > 6 && rand() % 2 == 0)
            emit((Vector3){ x + bx * 2 + frand(-1, 1), 0.2f, z + bz * 2 + frand(-1, 1) }, (Vector3){ frand(-1, 1), frand(1, 3), frand(-1, 1) },
                 0.6f, 0.3f, (Color){ 140, 120, 80, 160 }, 4);
        if (p->stun > 0 && rand() % 2 == 0)
            emit((Vector3){ x + frand(-1, 1), 1.0f, z + frand(-1, 1) }, (Vector3){ frand(-1, 1), frand(2, 4), frand(-1, 1) },
                 0.8f, 0.4f, (Color){ 70, 70, 76, 180 }, -1);
    }
    for (int k = 0; k < MAX_PROJ; k++) {
        const Projectile *pr = &g->proj[k];
        if (!pr->active) continue;
        Color c = pr->type == ITEM_SHUNT ? (Color){ 255, 120, 60, 220 } : (Color){ 255, 230, 120, 200 };
        emit((Vector3){ pr->x, pr->type == ITEM_SHUNT ? 1.2f : 1.0f, pr->z }, (Vector3){ frand(-0.5f, 0.5f), frand(0, 0.5f), frand(-0.5f, 0.5f) },
             0.35f, pr->type == ITEM_SHUNT ? 0.35f : 0.2f, c, 0);
    }
}

static void explosion(float x, float z, Color c)
{
    for (int k = 0; k < 36; k++)
        emit((Vector3){ x + frand(-0.8f, 0.8f), frand(0.4f, 1.4f), z + frand(-0.8f, 0.8f) },
             (Vector3){ frand(-8, 8), frand(3, 10), frand(-8, 8) }, frand(0.4f, 0.9f), frand(0.2f, 0.45f),
             k % 3 ? (Color){ 255, (unsigned char)(120 + rand() % 100), 40, 255 } : c, 14);
}

/* =================================================================== */
/*                       UI KOMPONENTLARI                              */
/* =================================================================== */
static Vector2 vmouse(void)
{
    Vector2 m = GetMousePosition();
    return (Vector2){ (m.x - off_x) / scale, (m.y - off_y) / scale };
}
static int clicked_in(Rectangle r)
{
    if (mouse_consumed || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return 0;
    if (!CheckCollisionPointRec(vmouse(), r)) return 0;
    mouse_consumed = 1;
    return 1;
}
static float rnd(Rectangle r, float radius) { float m = fminf(r.width, r.height); return m > 0 ? fminf(1, radius * 2 / m) : 0; }

static void card(Rectangle r, float radius, Color fill)
{
    DrawRectangleRounded((Rectangle){ r.x, r.y + 6, r.width, r.height }, rnd(r, radius), 12, Fade(BLACK, 0.28f));
    DrawRectangleRounded(r, rnd(r, radius), 12, fill);
    DrawRectangleRoundedLinesEx(r, rnd(r, radius), 12, 1.5f, COL_BORDER);
}

enum { BTN_PRIMARY, BTN_SECONDARY, BTN_GHOST };
static int ui_button(Rectangle r, const char *label, int style, int enabled)
{
    int hover = enabled && CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
    Rectangle d = r;
    if (hover) d.y -= 2;
    float fs = fminf(24, r.height * 0.4f);
    Color bg, fg, border;
    int font = F_SEMI;
    if (style == BTN_PRIMARY) { bg = hover ? (Color){ 255, 204, 92, 255 } : COL_ACCENT; fg = COL_DARKTXT; border = Fade(WHITE, 0.25f); font = F_BOLD; }
    else if (style == BTN_SECONDARY) { bg = hover ? COL_SURFACE3 : COL_SURFACE2; fg = COL_TEXT; border = hover ? Fade(COL_ACCENT, 0.7f) : COL_BORDER; }
    else { bg = hover ? COL_SURFACE3 : Fade(COL_SURFACE, 0.85f); fg = hover ? COL_TEXT : (Color){ 203, 213, 225, 255 }; border = Fade(WHITE, hover ? 0.3f : 0.16f); }
    if (!enabled) { bg = (Color){ 40, 44, 58, 255 }; fg = COL_SUBTLE; border = COL_BORDER; }
    if (style != BTN_GHOST) DrawRectangleRounded((Rectangle){ d.x, d.y + 4, d.width, d.height }, rnd(d, 14), 10, Fade(BLACK, 0.3f));
    DrawRectangleRounded(d, rnd(d, 14), 10, bg);
    DrawRectangleRoundedLinesEx(d, rnd(d, 14), 10, 1.5f, border);
    tcenter(font, label, d.x + d.width / 2, d.y + d.height / 2, fs, fg);
    if (enabled && clicked_in(r)) { play(SND_CLICK, 0.5f); return 1; }
    return 0;
}

static float chip(float x, float cy, const char *t, Color bg, Color fg, float fs)
{
    float w = twidth(F_SEMI, t, fs) + fs * 1.3f, h = fs + 12;
    DrawRectangleRounded((Rectangle){ x, cy - h / 2, w, h }, 1, 10, bg);
    tmid(F_SEMI, t, x + fs * 0.65f, cy, fs, fg);
    return w;
}

static int segmented(Rectangle r, const char **labels, int n, int sel)
{
    DrawRectangleRounded(r, rnd(r, 12), 10, COL_SURFACE2);
    DrawRectangleRoundedLinesEx(r, rnd(r, 12), 10, 1.5f, COL_BORDER);
    float w = (r.width - 8) / n;
    int out = sel;
    for (int i = 0; i < n; i++) {
        Rectangle s = { r.x + 4 + i * w, r.y + 4, w, r.height - 8 };
        int hover = CheckCollisionPointRec(vmouse(), s) && !IS_MOBILE;
        if (i == sel) DrawRectangleRounded(s, rnd(s, 10), 10, COL_ACCENT);
        else if (hover) DrawRectangleRounded(s, rnd(s, 10), 10, Fade(WHITE, 0.07f));
        tcenter(i == sel ? F_BOLD : F_SEMI, labels[i], s.x + s.width / 2, s.y + s.height / 2, 19, i == sel ? COL_DARKTXT : COL_MUTED);
        if (clicked_in(s)) { out = i; play(SND_CLICK, 0.5f); }
    }
    return out;
}

static void show_message(const char *m) { snprintf(message, sizeof(message), "%s", m); message_timer = 4.0f; }

#define MAX_POPUPS 3
typedef struct { char text[64]; float t; Color c; } Popup;
static Popup popups[MAX_POPUPS];
static void popup(const char *t, Color c)
{
    for (int i = MAX_POPUPS - 1; i > 0; i--) popups[i] = popups[i - 1];
    snprintf(popups[0].text, sizeof(popups[0].text), "%s", t);
    popups[0].t = 1.8f;
    popups[0].c = c;
}

static void local_ip_text(char *out, int cap)
{
    uint32_t ips[6];
    int n = plat_local_ips(ips, 6);
    out[0] = 0;
    for (int i = 0; i < n; i++) {
        char s[16];
        plat_ip_str(ips[i], s);
        if (strlen(out) + strlen(s) + 6 >= (size_t)cap) break;
        if (i) strcat(out, "  /  ");
        strcat(out, s);
    }
    if (n == 0) snprintf(out, (size_t)cap, "tarmoq topilmadi");
}

/* qurol belgisi (oddiy shakllardan) */
static void item_icon(int item, float cx, float cy, float r)
{
    Color c = ITEM_COLORS[item];
    DrawCircleV((Vector2){ cx, cy }, r, Fade(c, 0.22f));
    DrawCircleLines((int)cx, (int)cy, r, Fade(c, 0.7f));
    float s = r * 0.55f;
    switch (item) {
    case ITEM_NITRO:
        DrawTriangle((Vector2){ cx - s * 0.6f, cy + s }, (Vector2){ cx + s * 0.6f, cy + s }, (Vector2){ cx, cy - s }, c);
        DrawTriangle((Vector2){ cx - s * 0.3f, cy + s }, (Vector2){ cx + s * 0.3f, cy + s }, (Vector2){ cx, cy - s * 0.1f }, WHITE);
        break;
    case ITEM_SHUNT:   /* raketa (qiya) */
        rlPushMatrix();
        rlTranslatef(cx, cy, 0);
        rlRotatef(45, 0, 0, 1);
        DrawRectangleRounded((Rectangle){ -s * 0.32f, -s * 0.7f, s * 0.64f, s * 1.4f }, 0.5f, 6, c);
        DrawTriangle((Vector2){ -s * 0.32f, -s * 0.66f }, (Vector2){ s * 0.32f, -s * 0.66f }, (Vector2){ 0, -s * 1.25f }, WHITE);
        DrawTriangle((Vector2){ -s * 0.32f, s * 0.2f }, (Vector2){ -s * 0.75f, s * 0.75f }, (Vector2){ -s * 0.32f, s * 0.7f }, c);
        DrawTriangle((Vector2){ s * 0.32f, s * 0.2f }, (Vector2){ s * 0.32f, s * 0.7f }, (Vector2){ s * 0.75f, s * 0.75f }, c);
        DrawTriangle((Vector2){ -s * 0.22f, s * 0.72f }, (Vector2){ 0, s * 1.2f }, (Vector2){ s * 0.22f, s * 0.72f }, (Color){ 255, 200, 80, 255 });
        rlPopMatrix();
        break;
    case ITEM_BOLT:
        DrawTriangle((Vector2){ cx + s * 0.3f, cy - s }, (Vector2){ cx - s * 0.6f, cy + s * 0.15f }, (Vector2){ cx + s * 0.05f, cy + s * 0.15f }, c);
        DrawTriangle((Vector2){ cx - s * 0.05f, cy - s * 0.15f }, (Vector2){ cx - s * 0.3f, cy + s }, (Vector2){ cx + s * 0.6f, cy - s * 0.15f }, c);
        break;
    case ITEM_MINE:
        for (int k = 0; k < 8; k++) {
            float a = k * PI / 4;
            DrawLineEx((Vector2){ cx, cy }, (Vector2){ cx + cosf(a) * s * 1.1f, cy + sinf(a) * s * 1.1f }, 3, c);
        }
        DrawCircleV((Vector2){ cx, cy }, s * 0.7f, c);
        DrawCircleV((Vector2){ cx, cy }, s * 0.22f, WHITE);
        break;
    case ITEM_BARGE:
        for (int k = 1; k <= 3; k++) DrawRing((Vector2){ cx, cy }, s * k / 3.0f - 2, s * k / 3.0f, 0, 360, 24, c);
        break;
    case ITEM_SHIELD:
        DrawTriangle((Vector2){ cx - s, cy - s * 0.6f }, (Vector2){ cx, cy + s * 1.1f }, (Vector2){ cx + s, cy - s * 0.6f }, c);
        DrawRectangleRounded((Rectangle){ cx - s, cy - s, s * 2, s * 0.5f }, 0.5f, 6, c);
        break;
    default:
        DrawRing((Vector2){ cx, cy }, s - 2, s, 0, 360, 24, Fade(c, 0.6f));
        break;
    }
}

/* =================================================================== */
/*                               HUD                                   */
/* =================================================================== */
static void draw_stats(const GameState *g)
{
    const Player *me = &g->players[my_slot];
    int order[MAX_PLAYERS], place = 1, active = 0;
    game_ranking(g, order);
    for (int k = 0; k < MAX_PLAYERS; k++) if (order[k] == my_slot) place = k + 1;
    for (int i = 0; i < MAX_PLAYERS; i++) active += g->players[i].active;
    Rectangle r = { 16, 16, 250, 124 };
    card(r, 18, COL_SURFACE);
    char t[32];
    snprintf(t, sizeof(t), "%d", me->place ? me->place : place);
    tdraw(F_BOLD, t, 32, 16, 60, COL_ACCENT);
    float w = twidth(F_BOLD, t, 60);
    snprintf(t, sizeof(t), "/%d", active);
    tdraw(F_BOLD, t, 34 + w, 40, 26, COL_MUTED);
    tdraw(F_SEMI, "AYLANA", 128, 30, 13, COL_SUBTLE);
    int lap = me->lap + 1;
    if (lap < 1) lap = 1;
    if (lap > g->laps) lap = g->laps;
    snprintf(t, sizeof(t), "%d", lap);
    tdraw(F_BOLD, t, 128, 44, 30, COL_TEXT);
    snprintf(t, sizeof(t), "/ %d", g->laps);
    tdraw(F_BOLD, t, 132 + twidth(F_BOLD, "0", 30), 54, 18, COL_MUTED);
    DrawRectangle(32, 96, (int)r.width - 32, 1, COL_BORDER);
    snprintf(t, sizeof(t), "%d", (int)(fabsf(me->speed) * 3.6f));
    tmid(F_BOLD, t, 32, 118, 22, me->nitro > 0 ? COL_INFO : COL_TEXT);
    tmid(F_BODY, "km/soat", 36 + twidth(F_BOLD, t, 22), 120, 14, COL_MUTED);
    int secs = (int)g->elapsed;
    snprintf(t, sizeof(t), "%d:%02d", secs / 60, secs % 60);
    tright(F_MONOB, t, r.x + r.width - 16, 118, 18, COL_MUTED);
}

static void draw_leaderboard(const GameState *g)
{
    int order[MAX_PLAYERS], active = 0;
    game_ranking(g, order);
    for (int i = 0; i < MAX_PLAYERS; i++) active += g->players[i].active;
    Rectangle r = { vw - 288, 80, 272, 44.0f + 38.0f * active };
    card(r, 18, COL_SURFACE);
    tmid(F_SEMI, "POYGA", r.x + 18, r.y + 22, 14, COL_SUBTLE);
    int row = 0;
    for (int k = 0; k < MAX_PLAYERS; k++) {
        int i = order[k];
        const Player *p = &g->players[i];
        if (!p->active) continue;
        float cy = r.y + 56 + row * 38;
        if (i == my_slot) DrawRectangleRounded((Rectangle){ r.x + 8, cy - 16, r.width - 16, 32 }, 0.5f, 8, Fade(CAR_COLORS[i], 0.18f));
        char t[16];
        snprintf(t, sizeof(t), "%d", row + 1);
        tcenter(F_BOLD, t, r.x + 28, cy, 17, row == 0 ? COL_ACCENT : COL_MUTED);
        DrawRectangleRounded((Rectangle){ r.x + 46, cy - 7, 14, 14 }, 0.4f, 6, CAR_COLORS[i]);
        tmid(F_SEMI, i == my_slot ? "Siz" : p->is_bot ? "Bot" : game_color_name(i), r.x + 70, cy, 17, COL_TEXT);
        const char *st = p->place ? "marra" : p->stun > 0 ? "urildi" : p->shield > 0 ? "qalqon" : NULL;
        if (st) {
            Color sc = p->place ? COL_GOOD : p->stun > 0 ? COL_BAD : COL_GOOD;
            float w = twidth(F_SEMI, st, 13) + 16;
            DrawRectangleRounded((Rectangle){ r.x + r.width - 14 - w, cy - 11, w, 22 }, 1, 8, Fade(sc, 0.22f));
            tcenter(F_SEMI, st, r.x + r.width - 14 - w / 2, cy, 13, lerp_color(sc, WHITE, 0.35f));
        } else if (p->item) {
            item_icon(p->item, r.x + r.width - 30, cy, 11);
        }
        row++;
    }
}

static void draw_minimap(const GameState *g)
{
    const Track *T = &TRACK;
    float size = 200, x0 = vw - 16 - size - 12, y0 = VH - 16 - size - 12;
    card((Rectangle){ x0 - 12, y0 - 12, size + 24, size + 24 }, 18, COL_SURFACE);
    float w = T->max_x - T->min_x, h = T->max_z - T->min_z, k = size / fmaxf(w, h);
    float ox = x0 + (size - w * k) / 2, oz = y0 + (size - h * k) / 2;
#define MM(px, pz) (Vector2){ ox + (T->max_x - (px)) * k, oz + (T->max_z - (pz)) * k }
    for (int i = 0; i < T->n; i += 2) {
        Vector2 a = MM(T->p[i].x, T->p[i].z), b = MM(T->p[(i + 2) % T->n].x, T->p[(i + 2) % T->n].z);
        DrawLineEx(a, b, 7, (Color){ 70, 76, 96, 255 });
    }
    for (int i = 0; i < T->n; i += 2) {
        Vector2 a = MM(T->p[i].x, T->p[i].z), b = MM(T->p[(i + 2) % T->n].x, T->p[(i + 2) % T->n].z);
        DrawLineEx(a, b, 3, (Color){ 140, 148, 170, 255 });
    }
    Vector2 st = MM(T->p[0].x, T->p[0].z);
    DrawRectangle((int)st.x - 4, (int)st.y - 4, 8, 8, WHITE);
    for (int i = MAX_PLAYERS - 1; i >= 0; i--) {
        const Player *p = &g->players[i];
        if (!p->active) continue;
        Vector2 c = MM(p->x, p->z);
        float r = i == my_slot ? 7 : 5;
        DrawCircleV(c, r + 2, WHITE);
        DrawCircleV(c, r, CAR_COLORS[i]);
    }
#undef MM
}

static float keycap(float x, float cy, const char *key, const char *label, Color c)
{
    float kw = twidth(F_MONOB, key, 12) + 12;
    DrawRectangleRounded((Rectangle){ x, cy - 11, kw, 22 }, 0.4f, 6, Fade(c, 0.22f));
    DrawRectangleRoundedLinesEx((Rectangle){ x, cy - 11, kw, 22 }, 0.4f, 6, 1, Fade(c, 0.6f));
    tcenter(F_MONOB, key, x + kw / 2, cy, 12, lerp_color(c, WHITE, 0.5f));
    tmid(F_BODY, label, x + kw + 6, cy, 13, COL_MUTED);
    return kw + 12 + twidth(F_BODY, label, 13);
}

static void draw_item_slot(const Player *me)
{
    float w = me->item && !IS_MOBILE ? 430 : 300, h = 78, x = vw / 2 - w / 2, y = VH - h - 18;
    card((Rectangle){ x, y, w, h }, 20, COL_SURFACE);
    if (me->item) {   /* yorqin chet - qurol tayyor */
        float p = 0.5f + 0.5f * sinf((float)GetTime() * 5);
        DrawRectangleRoundedLinesEx((Rectangle){ x, y, w, h }, rnd((Rectangle){ x, y, w, h }, 20), 12, 2, Fade(ITEM_COLORS[me->item], 0.4f + 0.4f * p));
    }
    item_icon(me->item, x + 44, y + h / 2, 26);
    if (me->item) {
        char t[32];
        if (me->item == ITEM_BOLT) snprintf(t, sizeof(t), "%s  x%d", game_item_name(me->item), me->charges);
        else snprintf(t, sizeof(t), "%s", game_item_name(me->item));
        tmid(F_BOLD, t, x + 84, y + 26, 22, ITEM_COLORS[me->item]);
        if (IS_MOBILE) tmid(F_BODY, "OT - oldinga,  ORQAGA,  X - tashlash", x + 84, y + 54, 13, COL_MUTED);
        else {
            static const char *BACK[ITEM_COUNT] = { "", "tormoz", "orqaga", "orqaga", "orqaga", "zarba", "qalqon" };
            float kx = x + 84;
            kx += keycap(kx, y + 55, "PROBEL", me->item == ITEM_MINE ? "oldinga ot" : "oldinga", ITEM_COLORS[me->item]);
            kx += keycap(kx, y + 55, "Q", BACK[me->item], ITEM_COLORS[me->item]);
            keycap(kx, y + 55, "E", "tashlash", COL_MUTED);
        }
    } else {
        tmid(F_SEMI, "Qurol yo'q", x + 80, y + 28, 19, COL_MUTED);
        tmid(F_BODY, "Yo'ldagi qutini oling", x + 80, y + 52, 14, COL_SUBTLE);
    }
}

static int pause_button(void)
{
    Rectangle r = { vw - 64, 20, 48, 48 };
    int hover = CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
    DrawCircleV((Vector2){ r.x + 24, r.y + 24 }, 24, hover ? COL_SURFACE3 : COL_SURFACE);
    DrawCircleLines((int)r.x + 24, (int)r.y + 24, 24, COL_BORDER);
    DrawRectangleRounded((Rectangle){ r.x + 16, r.y + 15, 5, 18 }, 1, 4, COL_TEXT);
    DrawRectangleRounded((Rectangle){ r.x + 27, r.y + 15, 5, 18 }, 1, 4, COL_TEXT);
    return clicked_in(r);
}

/* ------------------------------------------------------------ Kirish */
typedef struct { Rectangle left, right, fire, brake, back, drop; } TouchLayout;

static TouchLayout touch_layout(void)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight(), u = sh / 720.0f;
    TouchLayout L;
    L.left = (Rectangle){ 24 * u, sh - 170 * u, 140 * u, 140 * u };
    L.right = (Rectangle){ 184 * u, sh - 170 * u, 140 * u, 140 * u };
    L.fire = (Rectangle){ sw - 184 * u, sh - 200 * u, 160 * u, 160 * u };
    L.brake = (Rectangle){ sw - 340 * u, sh - 140 * u, 130 * u, 110 * u };
    L.back = (Rectangle){ sw - 168 * u, sh - 330 * u, 128 * u, 110 * u };
    L.drop = (Rectangle){ sw - 300 * u, sh - 280 * u, 96 * u, 96 * u };
    return L;
}

static Input gather_input(void)
{
    Input in = { 0, 0, my_fire_seq, my_fire_mode };
    float steer = 0, thr = 0;
    int mode = FIRE_FORWARD;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) steer -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) steer += 1;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) thr += 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) thr -= 1;
    int fire = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_KP_ENTER);
    if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_RIGHT_SHIFT)) { fire = 1; mode = FIRE_BACKWARD; }
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_L) || IsKeyPressed(KEY_BACKSPACE)) { fire = 1; mode = FIRE_DISCARD; }
    if (IsGamepadAvailable(0)) {
        float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(gx) > 0.15f) steer = gx;
        float rt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER), lt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER);
        if (rt > -0.5f && rt != 0) thr = (rt + 1) / 2;
        if (lt > -0.5f && lt != 0) thr -= (lt + 1) / 2;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) thr = 1;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) fire = 1;
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) { fire = 1; mode = FIRE_BACKWARD; }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) { fire = 1; mode = FIRE_DISCARD; }
    }
#if IS_MOBILE
    TouchLayout L = touch_layout();
    int down[3] = { 0, 0, 0 }, brake = 0;
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 p = GetTouchPosition(i);
        if (CheckCollisionPointRec(p, L.left)) steer = -1;
        else if (CheckCollisionPointRec(p, L.right)) steer = 1;
        else if (CheckCollisionPointRec(p, L.fire)) down[0] = 1;
        else if (CheckCollisionPointRec(p, L.back)) down[1] = 1;
        else if (CheckCollisionPointRec(p, L.drop)) down[2] = 1;
        else if (CheckCollisionPointRec(p, L.brake)) brake = 1;
    }
    thr = brake ? -1 : 1;   /* telefonda gaz doimo bosilgan */
    for (int k = 0; k < 3; k++) {
        if (down[k] && !touch_was[k]) { fire = 1; mode = k == 0 ? FIRE_FORWARD : k == 1 ? FIRE_BACKWARD : FIRE_DISCARD; }
        touch_was[k] = down[k];
    }
#endif
    if (fire) { my_fire_seq++; my_fire_mode = (uint8_t)mode; }
    in.steer = (int8_t)(fmaxf(-1, fminf(1, steer)) * 127);
    in.throttle = (int8_t)(fmaxf(-1, fminf(1, thr)) * 127);
    in.fire_seq = my_fire_seq;
    in.fire_mode = my_fire_mode;
    return in;
}

static void draw_touch_controls(const Input *in)
{
#if IS_MOBILE
    TouchLayout L = touch_layout();
    float u = GetScreenHeight() / 720.0f;
    Rectangle rs[6] = { L.left, L.right, L.brake, L.fire, L.back, L.drop };
    int on[6] = { in->steer < -10, in->steer > 10, in->throttle < 0, touch_was[0], touch_was[1], touch_was[2] };
    int has = view.players[my_slot].item != ITEM_NONE;
    for (int i = 0; i < 6; i++) {
        Rectangle r = rs[i];
        if (i >= 4 && !has) continue;
        Color c = i >= 3 ? ITEM_COLORS[view.players[my_slot].item] : WHITE;
        if (i == 5) c = COL_BAD;
        DrawRectangleRounded(r, 0.35f, 10, Fade(i >= 3 ? c : BLACK, on[i] ? 0.55f : (i >= 3 ? 0.32f : 0.28f)));
        DrawRectangleRoundedLinesEx(r, 0.35f, 10, 2 * u, Fade(WHITE, 0.5f));
        float cx = r.x + r.width / 2, cy = r.y + r.height / 2, s = 26 * u;
        if (i == 0) DrawTriangle((Vector2){ cx + s * 0.6f, cy - s }, (Vector2){ cx - s * 0.8f, cy }, (Vector2){ cx + s * 0.6f, cy + s }, Fade(WHITE, 0.9f));
        if (i == 1) DrawTriangle((Vector2){ cx - s * 0.6f, cy - s }, (Vector2){ cx - s * 0.6f, cy + s }, (Vector2){ cx + s * 0.8f, cy }, Fade(WHITE, 0.9f));
        if (i == 2) tcenter(F_BOLD, "TORMOZ", cx, cy, 18 * u, Fade(WHITE, 0.9f));
        if (i == 3) tcenter(F_BOLD, "OT", cx, cy, 34 * u, WHITE);
        if (i == 4) { tcenter(F_BOLD, "ORQAGA", cx, cy + 14 * u, 17 * u, WHITE);
                      DrawTriangle((Vector2){ cx - 12 * u, cy - 22 * u }, (Vector2){ cx, cy - 6 * u }, (Vector2){ cx + 12 * u, cy - 22 * u }, WHITE); }
        if (i == 5) { DrawLineEx((Vector2){ cx - 14 * u, cy - 14 * u }, (Vector2){ cx + 14 * u, cy + 14 * u }, 5 * u, WHITE);
                      DrawLineEx((Vector2){ cx + 14 * u, cy - 14 * u }, (Vector2){ cx - 14 * u, cy + 14 * u }, 5 * u, WHITE); }
    }
#else
    (void)in;
#endif
}

/* =================================================================== */
/*                     ULANISH VA FON POYGASI                          */
/* =================================================================== */
static void leave_game(const char *msg)
{
    if (mode == MODE_HOST) server_stop(&server);
    if (mode == MODE_CLIENT) client_stop(&client);
    mode = MODE_NONE;
    screen = SCR_MENU;
    have_view = 0;
    menu_open = 0;
    my_slot = 0;
    cam_init = 0;
    if (msg) show_message(msg);
}

static void start_join_screen(void)
{
    if (!disc_running) disc_running = discovery_start(&disc);
    if (ip_input[0] == 0) {
        uint32_t ips[4];
        if (plat_local_ips(ips, 4) > 0) {
            char s[16];
            plat_ip_str(ips[0] & 0xFFFFFF00u, s);
            s[strlen(s) - 1] = 0;
            snprintf(ip_input, sizeof(ip_input), "%s", s);
        }
    }
    screen = SCR_JOIN;
}

static void connect_to(uint32_t ip)
{
    if (disc_running) { discovery_stop(&disc); disc_running = 0; }
    if (!client_start(&client, ip)) { show_message("Soketni ochib bo'lmadi"); return; }
    mode = MODE_CLIENT;
    connecting_ip = ip;
    screen = SCR_CONNECTING;
}

static void demo_update(float dt)
{
    Input in[MAX_PLAYERS];
    if (demo.phase == PHASE_LOBBY || demo.phase == PHASE_OVER || demo.map_id != TRACK.map_id) {
        game_init(&demo);
        demo.rng = (uint32_t)rand();
        demo.bots_enabled = BOTS_HARD;
        demo.laps = 20;
        for (int i = 0; i < MAX_PLAYERS; i++) game_add_player(&demo, i, 1);
        game_start_match(&demo);
        demo.countdown = 0.01f;
        memset(demo_brains, 0, sizeof(demo_brains));
    }
    for (int i = 0; i < MAX_PLAYERS; i++) bot_think(&demo, i, &demo_brains[i], &in[i], dt);
    game_step(&demo, in, dt);
}

/* =================================================================== */
/*                             EKRANLAR                                */
/* =================================================================== */
static void screen_menu(void)
{
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.35f));
    DrawRectangleGradientV(0, 200, (int)vw + 2, VH - 200, Fade(COL_BG, 0.0f), Fade(COL_BG, 0.96f));
    /* logotip */
    float ty = 96;
    tcenter(F_BOLD, "TURBO JANG!", cx + 4, ty + 5, 92, Fade(BLACK, 0.45f));
    tcenter(F_BOLD, "TURBO JANG!", cx, ty, 92, WHITE);
    float lw = twidth(F_BOLD, "TURBO JANG!", 92);
    DrawRectangleRounded((Rectangle){ cx - lw / 2, ty + 50, lw * 0.6f, 6 }, 1, 6, COL_BAD);
    DrawRectangleRounded((Rectangle){ cx - lw / 2 + lw * 0.62f, ty + 50, lw * 0.38f, 6 }, 1, 6, COL_ACCENT);
    tcenter(F_SEMI, "Qurolli poyga va parkovka ustasi", cx, ty + 86, 22, Fade(WHITE, 0.9f));

    /* ikki rejim kartasi */
    float cw = 440, ch = 330, gap = 36, y = 222;
    Rectangle A = { cx - cw - gap / 2, y, cw, ch }, B = { cx + gap / 2, y, cw, ch };
    float t = (float)GetTime();
    /* --- Poyga --- */
    card(A, 26, Fade((Color){ 18, 22, 35, 255 }, 0.93f));
    Rectangle ah = { A.x + 12, A.y + 12, A.width - 24, 120 };
    DrawRectangleRounded(ah, rnd(ah, 18), 10, (Color){ 70, 28, 40, 255 });
    DrawRectangle((int)ah.x + 16, (int)(ah.y + ah.height - 26), (int)ah.width - 32, 3, Fade(WHITE, 0.15f));   /* yo'l chizig'i */
    for (int k = 0; k < 6; k++) {   /* tezlik chiziqlari */
        float lx = fmodf(t * 260 + k * 83, ah.width + 80) - 40;
        DrawRectangleRounded((Rectangle){ ah.x + ah.width - lx, ah.y + 22 + k * 15, 46 + (k % 3) * 18, 4 }, 1, 4, Fade(WHITE, 0.18f));
    }
    draw_car_icon(ah.x + 120, ah.y + 62 + sinf(t * 6) * 1.5f, 0.9f, CAR_COLORS[0]);
    draw_car_icon(ah.x + 270, ah.y + 76 + sinf(t * 6 + 1) * 1.5f, 0.62f, CAR_COLORS[1]);
    item_icon(ITEM_SHUNT, ah.x + ah.width - 46, ah.y + 40, 17);
    item_icon(ITEM_BOLT, ah.x + ah.width - 46, ah.y + 88, 17);
    tmid(F_BOLD, "Poyga", A.x + 28, A.y + 166, 32, COL_TEXT);
    tmid(F_BODY, "4 kishigacha, bitta Wi-Fi yoki hotspot orqali", A.x + 28, A.y + 198, 16, COL_MUTED);
    float px = A.x + 28;
    px += chip(px, A.y + 230, "4 xarita", Fade(WHITE, 0.08f), COL_TEXT, 13) + 8;
    px += chip(px, A.y + 230, "6 qurol", Fade(WHITE, 0.08f), COL_TEXT, 13) + 8;
    chip(px, A.y + 230, "botlar", Fade(WHITE, 0.08f), COL_TEXT, 13);
    if (ui_button((Rectangle){ A.x + 24, A.y + ch - 78, 220, 56 }, "O'yin yaratish", BTN_PRIMARY, 1)) {
        if (server_start(&server)) { mode = MODE_HOST; my_slot = 0; screen = SCR_GAME; have_view = 0; cam_init = 0; }
        else show_message("47820-port band. Boshqa nusxa ochiq emasligini tekshiring.");
    }
    if (ui_button((Rectangle){ A.x + 256, A.y + ch - 78, 160, 56 }, "Qo'shilish", BTN_SECONDARY, 1)) start_join_screen();

    /* --- Parkovka --- */
    card(B, 26, Fade((Color){ 18, 22, 35, 255 }, 0.93f));
    Rectangle bh = { B.x + 12, B.y + 12, B.width - 24, 120 };
    DrawRectangleRounded(bh, rnd(bh, 18), 10, (Color){ 30, 36, 52, 255 });
    for (int k = 0; k < 4; k++) {   /* joy chiziqlari */
        float lx = bh.x + 40 + k * 76;
        DrawRectangle((int)lx, (int)bh.y + 18, 4, 84, Fade(WHITE, 0.55f));
    }
    DrawRectangle((int)bh.x + 40, (int)bh.y + 18, 232, 4, Fade(WHITE, 0.55f));
    float pp = 0.5f + 0.5f * sinf(t * 4);
    DrawRectangleRounded((Rectangle){ bh.x + 196, bh.y + 24, 70, 78 }, 0.2f, 6, Fade(COL_GOOD, 0.18f + 0.15f * pp));
    DrawRectangleRoundedLinesEx((Rectangle){ bh.x + 196, bh.y + 24, 70, 78 }, 0.2f, 6, 2, COL_GOOD);
    DrawRectangleRounded((Rectangle){ bh.x + 54, bh.y + 34, 50, 64 }, 0.3f, 6, (Color){ 150, 158, 172, 255 });
    DrawRectangleRounded((Rectangle){ bh.x + 130, bh.y + 34, 50, 64 }, 0.3f, 6, (Color){ 40, 74, 146, 255 });
    draw_p_badge(bh.x + bh.width - 48, bh.y + 60, 0.7f);
    tmid(F_BOLD, "Parkovka", B.x + 28, B.y + 166, 32, COL_TEXT);
    tmid(F_BODY, "Yakka rejim: 20 daraja, garaj, parallel turish", B.x + 28, B.y + 198, 16, COL_MUTED);
    int total = park_total_stars();
    char st[32];
    snprintf(st, sizeof(st), "%d / %d", total, PK_LEVELS * 3);
    Rectangle bar = { B.x + 28, B.y + 226, B.width - 56 - 100, 8 };
    DrawRectangleRounded(bar, 1, 6, Fade(WHITE, 0.08f));
    DrawRectangleRounded((Rectangle){ bar.x, bar.y, fmaxf(8, bar.width * total / (PK_LEVELS * 3.0f)), bar.height }, 1, 6, (Color){ 250, 204, 21, 255 });
    draw_star(bar.x + bar.width + 20, bar.y + 4, 9, (Color){ 250, 204, 21, 255 });
    tmid(F_MONOB, st, bar.x + bar.width + 34, bar.y + 4, 15, (Color){ 250, 204, 21, 255 });
    if (ui_button((Rectangle){ B.x + 24, B.y + ch - 78, B.width - 48, 56 }, total ? "Davom etish" : "Boshlash", BTN_PRIMARY, 1)) park_open_select();

    if (ui_button((Rectangle){ cx - 80, y + ch + 26, 160, 46 }, "Chiqish", BTN_GHOST, 1)) quit = 1;
    tcenter(F_BODY, IS_MOBILE ? "Poyga: rul - chapda, OT - o'ngda.  Parkovka: GAZ / ORQAGA tugmalari"
                              : "Poyga: WASD + PROBEL / Q / E      Parkovka: WASD, PROBEL - tormoz, C - kamera",
            cx, VH - 30, 15, Fade(COL_TEXT, 0.65f));
}

static void screen_join(void)
{
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.7f));
    tcenter(F_BOLD, "O'yinga qo'shilish", cx, 54, 38, COL_TEXT);
    Rectangle L = { cx - 580, 96, 560, 530 };
    card(L, 20, COL_SURFACE);
    tmid(F_BOLD, "Tarmoqdagi o'yinlar", L.x + 28, L.y + 36, 22, COL_TEXT);
    HostEntry hosts[MAX_FOUND_HOSTS];
    int n = disc_running ? discovery_list(&disc, hosts, MAX_FOUND_HOSTS) : 0;
    for (int i = 0; i < n && i < 5; i++) {
        char ip[16], label[64];
        plat_ip_str(hosts[i].ip, ip);
        Rectangle r = { L.x + 24, L.y + 72 + i * 82.0f, L.width - 48, 70 };
        int hover = CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
        DrawRectangleRounded(r, rnd(r, 14), 10, hover ? COL_SURFACE3 : COL_SURFACE2);
        DrawRectangleRoundedLinesEx(r, rnd(r, 14), 10, 1.5f, hover ? COL_ACCENT : COL_BORDER);
        tmid(F_MONOB, ip, r.x + 22, r.y + 26, 22, COL_TEXT);
        snprintf(label, sizeof(label), "%d / 4 o'yinchi  -  %s", hosts[i].players, hosts[i].phase == PHASE_LOBBY ? "kutish xonasida" : "poygada");
        tmid(F_BODY, label, r.x + 22, r.y + 50, 15, COL_MUTED);
        tright(F_BOLD, "Ulanish", r.x + r.width - 22, r.y + 35, 18, COL_ACCENT);
        if (clicked_in(r)) { play(SND_CLICK, 0.5f); connect_to(hosts[i].ip); }
    }
    if (n == 0) {
        float a = (float)GetTime() * 360.0f;
        DrawRing((Vector2){ L.x + L.width / 2, L.y + 200 }, 22, 28, a, a + 260, 30, COL_ACCENT);
        tcenter(F_SEMI, "Qidirilmoqda...", L.x + L.width / 2, L.y + 260, 20, COL_TEXT);
        tcenter(F_BODY, "Host bilan bitta hotspotga ulanganingizni tekshiring.", L.x + L.width / 2, L.y + 304, 16, COL_MUTED);
        tcenter(F_BODY, "Topilmasa, IP manzilni o'ng tomonda kiriting.", L.x + L.width / 2, L.y + 328, 16, COL_MUTED);
    }
    Rectangle R = { cx + 20, 96, 560, 530 };
    card(R, 20, COL_SURFACE);
    tmid(F_BOLD, "IP manzil orqali", R.x + 28, R.y + 36, 22, COL_TEXT);
    Rectangle field = { R.x + 24, R.y + 64, R.width - 48, 64 };
    DrawRectangleRounded(field, rnd(field, 12), 10, (Color){ 10, 12, 20, 255 });
    DrawRectangleRoundedLinesEx(field, rnd(field, 12), 10, 2, COL_ACCENT);
    tmid(F_MONOB, ip_input, field.x + 20, field.y + 32, 30, COL_TEXT);
    if (((int)(GetTime() * 2)) & 1) DrawRectangle((int)(field.x + 22 + twidth(F_MONOB, ip_input, 30)), (int)field.y + 16, 3, 32, COL_ACCENT);
    int ch;
    while ((ch = GetCharPressed()) > 0)
        if (((ch >= '0' && ch <= '9') || ch == '.') && strlen(ip_input) < 15) { size_t l = strlen(ip_input); ip_input[l] = (char)ch; ip_input[l + 1] = 0; }
    if (IsKeyPressed(KEY_BACKSPACE) && ip_input[0]) ip_input[strlen(ip_input) - 1] = 0;
    const char *keys[12] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "<" };
    float kw = (R.width - 48 - 20) / 3;
    for (int i = 0; i < 12; i++) {
        Rectangle k = { R.x + 24 + (i % 3) * (kw + 10), R.y + 146 + (i / 3) * 64.0f, kw, 54 };
        int hover = CheckCollisionPointRec(vmouse(), k) && !IS_MOBILE;
        DrawRectangleRounded(k, rnd(k, 12), 10, hover ? COL_SURFACE3 : COL_SURFACE2);
        DrawRectangleRoundedLinesEx(k, rnd(k, 12), 10, 1.5f, COL_BORDER);
        tcenter(F_MONOB, keys[i][0] == '<' ? "DEL" : keys[i], k.x + k.width / 2, k.y + k.height / 2, keys[i][0] == '<' ? 18 : 24, COL_TEXT);
        if (clicked_in(k)) {
            play(SND_CLICK, 0.4f);
            size_t l = strlen(ip_input);
            if (keys[i][0] == '<') { if (l) ip_input[l - 1] = 0; }
            else if (l < 15) { ip_input[l] = keys[i][0]; ip_input[l + 1] = 0; }
        }
    }
    uint32_t ip = plat_parse_ip(ip_input);
    if (ui_button((Rectangle){ R.x + 24, R.y + R.height - 84, R.width - 48, 60 }, "Ulanish", BTN_PRIMARY, ip != 0) || (ip && IsKeyPressed(KEY_ENTER))) connect_to(ip);
    if (ui_button((Rectangle){ cx - 110, 646, 220, 50 }, "Orqaga", BTN_GHOST, 1) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACK)) {
        if (disc_running) { discovery_stop(&disc); disc_running = 0; }
        screen = SCR_MENU;
    }
}

static void screen_connecting(void)
{
    char ip[16];
    plat_ip_str(connecting_ip, ip);
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.7f));
    Rectangle r = { vw / 2 - 260, 230, 520, 260 };
    card(r, 22, COL_SURFACE);
    float a = (float)GetTime() * 360.0f;
    DrawRing((Vector2){ vw / 2, r.y + 80 }, 24, 31, a, a + 260, 30, COL_ACCENT);
    tcenter(F_MONOB, ip, vw / 2, r.y + 146, 26, COL_TEXT);
    tcenter(F_BODY, "ga ulanilmoqda...", vw / 2, r.y + 178, 18, COL_MUTED);
    int st = client_status(&client);
    if (st == CL_CONNECTED) { screen = SCR_GAME; have_view = 0; cam_init = 0; }
    else if (st != CL_CONNECTING) {
        const char *why = st == CL_REJECTED_FULL ? "O'yinda bo'sh joy yo'q (4/4)." : st == CL_REJECTED_VERSION ? "Host o'yinning boshqa versiyasida." : "Host javob bermadi. IP va hotspotni tekshiring.";
        leave_game(why);
        start_join_screen();
        return;
    }
    if (ui_button((Rectangle){ vw / 2 - 110, r.y + 200, 220, 44 }, "Bekor qilish", BTN_GHOST, 1)) { leave_game(NULL); start_join_screen(); }
}

static void draw_car_icon(float cx, float cy, float s, Color c)
{
    DrawRectangleRounded((Rectangle){ cx - 26 * s, cy - 20 * s, 48 * s, 20 * s }, 0.5f, 8, mul(c, 0.75f));
    DrawRectangle((int)(cx - 19 * s), (int)(cy - 16 * s), (int)(16 * s), (int)(12 * s), (Color){ 160, 205, 240, 255 });
    DrawRectangle((int)(cx + 1 * s), (int)(cy - 16 * s), (int)(16 * s), (int)(12 * s), (Color){ 160, 205, 240, 255 });
    DrawRectangleRounded((Rectangle){ cx - 56 * s, cy - 4 * s, 112 * s, 24 * s }, 0.55f, 8, c);
    DrawRectangleRounded((Rectangle){ cx - 56 * s, cy + 8 * s, 112 * s, 12 * s }, 0.55f, 8, mul(c, 0.8f));
    DrawRectangle((int)(cx - 60 * s), (int)(cy - 10 * s), (int)(14 * s), (int)(5 * s), mul(c, 0.6f));
    DrawRectangle((int)(cx + 46 * s), (int)(cy + 1 * s), (int)(8 * s), (int)(5 * s), (Color){ 255, 245, 200, 255 });
    for (int sx = -1; sx <= 1; sx += 2) {
        DrawCircleV((Vector2){ cx + sx * 32 * s, cy + 20 * s }, 12 * s, (Color){ 22, 22, 26, 255 });
        DrawCircleV((Vector2){ cx + sx * 32 * s, cy + 20 * s }, 5.5f * s, (Color){ 190, 194, 204, 255 });
    }
}

/* xaritalarning kichik ko'rinishi (lobbi kartalari uchun) */
#define PREVIEW_N 120
static Vector2 previews[MAP_COUNT][PREVIEW_N];
static float   preview_len[MAP_COUNT];

static void build_previews(void)
{
    for (int m = 0; m < MAP_COUNT; m++) {
        track_init(m);
        const Track *T = &TRACK;
        float w = T->max_x - T->min_x, h = T->max_z - T->min_z, k = 1.0f / fmaxf(w, h);
        for (int i = 0; i < PREVIEW_N; i++) {
            V2 q = T->p[i * T->n / PREVIEW_N];
            previews[m][i] = (Vector2){ 0.5f + (T->max_x - q.x - w / 2) * k, 0.5f + (T->max_z - q.z - h / 2) * k };
        }
        preview_len[m] = T->length;
    }
}

static void draw_map_card(Rectangle r, int m, int selected, int host)
{
    const Theme *t = &THEMES[m];
    int hover = host && CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
    DrawRectangleRounded(r, rnd(r, 16), 10, selected ? COL_SURFACE3 : hover ? COL_SURFACE3 : COL_SURFACE2);
    /* osmon -> yer gradienti (mavzu rangi) */
    Rectangle art = { r.x + 8, r.y + 8, 92, r.height - 16 };
    DrawRectangleRounded(art, rnd(art, 12), 8, t->ground);
    DrawRectangleGradientV((int)art.x + 4, (int)art.y + 3, (int)art.width - 8, (int)(art.height * 0.45f), t->sky_top, t->sky_hor);
    for (int i = 0; i < PREVIEW_N; i++) {
        Vector2 a = previews[m][i], b = previews[m][(i + 1) % PREVIEW_N];
        Vector2 pa = { art.x + 10 + a.x * (art.width - 20), art.y + 10 + a.y * (art.height - 20) };
        Vector2 pb = { art.x + 10 + b.x * (art.width - 20), art.y + 10 + b.y * (art.height - 20) };
        DrawLineEx(pa, pb, 5, Fade(BLACK, 0.45f));
        DrawLineEx(pa, pb, 2.5f, t->night ? t->curb_b : WHITE);
    }
    Vector2 s0 = previews[m][0];
    DrawCircleV((Vector2){ art.x + 10 + s0.x * (art.width - 20), art.y + 10 + s0.y * (art.height - 20) }, 4, t->accent);
    float tx = art.x + art.width + 12;
    tmid(F_BOLD, track_map_name(m), tx, r.y + 26, 18, selected ? COL_TEXT : lerp_color(COL_TEXT, COL_MUTED, 0.3f));
    char len[24];
    snprintf(len, sizeof(len), "%.1f km", preview_len[m] / 1000.0f);
    tmid(F_MONOB, len, tx, r.y + 50, 14, t->accent);
    tmid(F_BODY, t->desc, tx, r.y + 72, 12, COL_MUTED);
    DrawRectangleRoundedLinesEx(r, rnd(r, 16), 10, selected ? 2.5f : 1.2f, selected ? t->accent : COL_BORDER);
    if (selected) {
        DrawCircleV((Vector2){ r.x + r.width - 18, r.y + 18 }, 10, t->accent);
        DrawLineEx((Vector2){ r.x + r.width - 23, r.y + 18 }, (Vector2){ r.x + r.width - 19, r.y + 22 }, 2.5f, COL_DARKTXT);
        DrawLineEx((Vector2){ r.x + r.width - 19, r.y + 22 }, (Vector2){ r.x + r.width - 13, r.y + 14 }, 2.5f, COL_DARKTXT);
    }
}

static void draw_lobby(const GameState *g)
{
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.55f));
    Rectangle P = { cx - 540, 16, 1080, 688 };
    card(P, 24, COL_SURFACE);
    tmid(F_BOLD, "Kutish xonasi", P.x + 36, P.y + 44, 32, COL_TEXT);
    if (mode == MODE_HOST) {
        char ips[96];
        local_ip_text(ips, sizeof(ips));
        float w = twidth(F_MONOB, ips, 22) + 40;
        Rectangle ir = { P.x + P.width - 36 - w, P.y + 20, w, 46 };
        DrawRectangleRounded(ir, rnd(ir, 12), 10, (Color){ 10, 12, 20, 255 });
        DrawRectangleRoundedLinesEx(ir, rnd(ir, 12), 10, 1.5f, Fade(COL_ACCENT, 0.6f));
        tcenter(F_MONOB, ips, ir.x + w / 2, ir.y + 23, 22, COL_ACCENT);
        tright(F_BODY, "Do'stlaringiz shu IP ga ulanadi:", ir.x - 14, ir.y + 23, 16, COL_MUTED);
    } else {
        float a = (float)GetTime() * 360.0f;
        DrawRing((Vector2){ P.x + P.width - 60, P.y + 44 }, 12, 16, a, a + 260, 24, COL_ACCENT);
        tright(F_SEMI, "Host poygani boshlashini kuting", P.x + P.width - 86, P.y + 44, 18, COL_MUTED);
    }
    /* o'yinchilar */
    float cw = (P.width - 72 - 3 * 16) / 4;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        Rectangle c = { P.x + 36 + i * (cw + 16), P.y + 84, cw, 138 };
        DrawRectangleRounded(c, rnd(c, 18), 10, p->active ? COL_SURFACE2 : (Color){ 22, 26, 40, 255 });
        DrawRectangleRoundedLinesEx(c, rnd(c, 18), 10, i == my_slot ? 2.5f : 1.5f, i == my_slot ? WHITE : COL_BORDER);
        Rectangle e = { c.x + 10, c.y + 10, c.width - 20, 72 };
        if (p->active) {
            DrawRectangleRounded(e, rnd(e, 14), 10, Fade(CAR_COLORS[i], 0.14f));
            draw_car_icon(c.x + c.width / 2, c.y + 44 + sinf((float)GetTime() * 4 + i) * 2, 0.85f, CAR_COLORS[i]);
        } else {
            DrawRectangleRoundedLinesEx(e, rnd(e, 14), 10, 1.5f, Fade(WHITE, 0.12f));
            tcenter(F_SEMI, "bo'sh joy", e.x + e.width / 2, e.y + e.height / 2, 16, COL_SUBTLE);
        }
        tmid(F_BOLD, game_color_name(i), c.x + 18, c.y + 110, 20, p->active ? lerp_color(CAR_COLORS[i], WHITE, 0.25f) : COL_SUBTLE);
        const char *who = !p->active ? "kutilmoqda" : i == my_slot ? "Siz" : p->is_bot ? "Bot" : "O'yinchi";
        Color wc = !p->active ? COL_SUBTLE : i == my_slot ? COL_ACCENT : p->is_bot ? COL_MUTED : COL_GOOD;
        float w = twidth(F_SEMI, who, 14) + 18;
        DrawRectangleRounded((Rectangle){ c.x + c.width - 14 - w, c.y + 98, w, 24 }, 1, 8, Fade(wc, 0.15f));
        tcenter(F_SEMI, who, c.x + c.width - 14 - w / 2, c.y + 110, 14, wc);
    }
    /* xaritalar */
    float my = P.y + 240;
    tmid(F_SEMI, "Xarita", P.x + 36, my + 8, 16, COL_MUTED);
    if (mode != MODE_HOST) tright(F_BODY, "xaritani host tanlaydi", P.x + P.width - 36, my + 8, 14, COL_SUBTLE);
    float mw = (P.width - 72 - 3 * 14) / 4;
    for (int m = 0; m < MAP_COUNT; m++) {
        Rectangle r = { P.x + 36 + m * (mw + 14), my + 22, mw, 104 };
        draw_map_card(r, m, g->map_id == m, mode == MODE_HOST);
        if (mode == MODE_HOST && g->map_id != m && clicked_in(r)) { play(SND_CLICK, 0.5f); server_set_map(&server, m); }
    }
    if (mode == MODE_HOST) {   /* klaviatura: 1-4 */
        for (int m = 0; m < MAP_COUNT; m++)
            if (IsKeyPressed(KEY_ONE + m) && g->map_id != m) server_set_map(&server, m);
    }
    /* sozlamalar */
    float sy = P.y + 384;
    static const char *BOTS[3] = { "O'chiq", "Oson", "Qiyin" };
    static const char *LAPS[3] = { "2 aylana", "3 aylana", "5 aylana" };
    tmid(F_SEMI, "Botlar", P.x + 36, sy + 6, 16, COL_MUTED);
    tmid(F_SEMI, "Poyga uzunligi", P.x + P.width - 476, sy + 6, 16, COL_MUTED);
    int cur = g->laps == 2 ? 0 : g->laps == 3 ? 1 : 2;
    if (mode == MODE_HOST) {
        int b = segmented((Rectangle){ P.x + 36, sy + 20, 440, 48 }, BOTS, 3, g->bots_enabled % 3);
        if (b != g->bots_enabled) server_set_bots(&server, b);
        int ns = segmented((Rectangle){ P.x + P.width - 476, sy + 20, 440, 48 }, LAPS, 3, cur);
        if (ns != cur) server_set_laps(&server, ns == 0 ? 2 : ns == 1 ? 3 : 5);
    } else {
        chip(P.x + 36, sy + 44, BOTS[g->bots_enabled % 3], Fade(WHITE, 0.08f), COL_TEXT, 16);
        chip(P.x + P.width - 476, sy + 44, LAPS[cur], Fade(WHITE, 0.08f), COL_TEXT, 16);
    }
    /* qurollar */
    float ry = P.y + 470;
    float iw = (P.width - 72 - 5 * 10) / 6;
    static const char *DESC[ITEM_COUNT] = { "", "Q: keskin tormoz", "Q: orqadagiga", "3 o'q, Q: orqaga", "oldinga / orqaga", "atrofni itaradi", "bitta zarbadan" };
    for (int k = 1; k < ITEM_COUNT; k++) {
        Rectangle rr = { P.x + 36 + (k - 1) * (iw + 10), ry, iw, 84 };
        DrawRectangleRounded(rr, rnd(rr, 14), 10, Fade(ITEM_COLORS[k], 0.07f));
        DrawRectangleRoundedLinesEx(rr, rnd(rr, 14), 10, 1, Fade(ITEM_COLORS[k], 0.25f));
        item_icon(k, rr.x + 30, rr.y + rr.height / 2, 19);
        tmid(F_BOLD, game_item_name(k), rr.x + 58, rr.y + 32, 16, ITEM_COLORS[k]);
        tmid(F_BODY, DESC[k], rr.x + 58, rr.y + 54, 12, COL_MUTED);
    }
    tcenter(F_BODY, IS_MOBILE ? "Rul - chap pastda,  OT / ORQAGA / X - o'ng pastda,  gaz avtomatik"
                              : "W/S gaz-tormoz    A/D rul    PROBEL oldinga otish    Q orqaga otish    E qurolni tashlash",
            cx, P.y + 578, 15, COL_SUBTLE);
    if (mode == MODE_HOST) {
        if (ui_button((Rectangle){ cx - 170, P.y + P.height - 84, 340, 60 }, "Poygani boshlash", BTN_PRIMARY, 1) || IsKeyPressed(KEY_ENTER)) server_begin_match(&server);
        if (ui_button((Rectangle){ P.x + 36, P.y + P.height - 80, 150, 52 }, "Chiqish", BTN_GHOST, 1)) leave_game(NULL);
    } else {
        if (ui_button((Rectangle){ cx - 90, P.y + P.height - 80, 180, 52 }, "Chiqish", BTN_GHOST, 1)) leave_game(NULL);
    }
}

static void draw_results(const GameState *g)
{
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.6f));
    Rectangle P = { cx - 440, 50, 880, 630 };
    card(P, 24, COL_SURFACE);
    int order[MAX_PLAYERS];
    game_ranking(g, order);
    int winner = order[0];
    char t[96];
    tcenter(F_SEMI, "POYGA TUGADI", cx, P.y + 44, 16, COL_SUBTLE);
    if (winner == my_slot) snprintf(t, sizeof(t), "Siz g'olibsiz!");
    else snprintf(t, sizeof(t), "G'olib: %s", g->players[winner].is_bot ? "Bot" : game_color_name(winner));
    tcenter(F_BOLD, t, cx, P.y + 92, 46, lerp_color(CAR_COLORS[winner], WHITE, 0.2f));
    static const Color MEDALS[3] = { { 250, 204, 21, 255 }, { 203, 213, 225, 255 }, { 217, 119, 6, 255 } };
    float hy = P.y + 150;
    tmid(F_SEMI, "O'YINCHI", P.x + 110, hy, 13, COL_SUBTLE);
    tmid(F_SEMI, "VAQT", P.x + 450, hy, 13, COL_SUBTLE);
    tmid(F_SEMI, "QUROL", P.x + 600, hy, 13, COL_SUBTLE);
    tmid(F_SEMI, "ZARBA OLDI", P.x + 710, hy, 13, COL_SUBTLE);
    int row = 0;
    for (int k = 0; k < MAX_PLAYERS; k++) {
        int i = order[k];
        const Player *p = &g->players[i];
        if (!p->active) continue;
        Rectangle r = { P.x + 30, hy + 22 + row * 66.0f, P.width - 60, 56 };
        DrawRectangleRounded(r, rnd(r, 14), 10, i == my_slot ? Fade(CAR_COLORS[i], 0.16f) : COL_SURFACE2);
        if (i == my_slot) DrawRectangleRoundedLinesEx(r, rnd(r, 14), 10, 1.5f, Fade(CAR_COLORS[i], 0.6f));
        float cy = r.y + r.height / 2;
        snprintf(t, sizeof(t), "%d", row + 1);
        if (row < 3) { DrawCircleV((Vector2){ r.x + 36, cy }, 17, MEDALS[row]); tcenter(F_BOLD, t, r.x + 36, cy, 18, COL_DARKTXT); }
        else tcenter(F_BOLD, t, r.x + 36, cy, 18, COL_MUTED);
        DrawRectangleRounded((Rectangle){ r.x + 76, cy - 8, 16, 16 }, 0.4f, 6, CAR_COLORS[i]);
        snprintf(t, sizeof(t), "%s", game_color_name(i));
        tmid(F_SEMI, t, r.x + 102, cy, 20, COL_TEXT);
        const char *tag = i == my_slot ? "siz" : p->is_bot ? "bot" : NULL;
        if (tag) chip(r.x + 110 + twidth(F_SEMI, t, 20), cy, tag, Fade(WHITE, 0.1f), COL_MUTED, 13);
        if (p->finish_time > 0) { int ms = (int)(p->finish_time * 10); snprintf(t, sizeof(t), "%d:%02d.%d", ms / 600, (ms / 10) % 60, ms % 10); }
        else snprintf(t, sizeof(t), "--");
        tmid(F_MONOB, t, P.x + 450, cy, 20, COL_TEXT);
        snprintf(t, sizeof(t), "%d", p->fires);
        tmid(F_MONOB, t, P.x + 610, cy, 20, COL_ACCENT);
        snprintf(t, sizeof(t), "%d", p->hits);
        tmid(F_MONOB, t, P.x + 730, cy, 20, COL_BAD);
        row++;
    }
    float by = P.y + P.height - 128;
    if (mode == MODE_HOST) {
        if (ui_button((Rectangle){ cx - 330, by, 300, 60 }, "Yana poyga", BTN_PRIMARY, 1) || IsKeyPressed(KEY_ENTER)) server_begin_match(&server);
        if (ui_button((Rectangle){ cx + 30, by, 300, 60 }, "Kutish xonasi", BTN_SECONDARY, 1)) server_back_to_lobby(&server);
    } else {
        tcenter(F_SEMI, "Host yangi poygani boshlashini kuting...", cx, by + 30, 18, COL_MUTED);
    }
    if (ui_button((Rectangle){ cx - 90, by + 74, 180, 38 }, "Chiqish", BTN_GHOST, 1)) leave_game(NULL);
}

/* holatlar orasidagi o'zgarishlar: ovoz, effekt, xabar */
static void detect_events(const GameState *a, const GameState *b)
{
    if (a->phase == PHASE_COUNTDOWN && b->phase == PHASE_PLAY) play(SND_GO, 0.6f);
    if (b->phase == PHASE_COUNTDOWN && (int)ceilf(a->countdown) != (int)ceilf(b->countdown)) play(SND_BEEP, 0.5f);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *o = &a->players[i], *n = &b->players[i];
        if (!o->active || !n->active) continue;
        int me = i == my_slot;
        float dx = n->x - cam_pos.x, dz = n->z - cam_pos.z;
        float vol = me ? 0.7f : fmaxf(0.0f, 0.5f - sqrtf(dx * dx + dz * dz) / 200.0f);
        if (n->fires != o->fires) {
            int it = n->last_used;
            static const int SND_OF[ITEM_COUNT] = { 0, SND_NITRO, SND_SHUNT, SND_BOLT, SND_MINE, SND_BARGE, SND_SHIELD };
            if (it > 0 && it < ITEM_COUNT) play(SND_OF[it], vol);
            if (it == ITEM_BARGE) add_ring(n->x, n->z, ITEM_COLORS[ITEM_BARGE]);
        }
        if (n->hits != o->hits) {
            explosion(n->x, n->z, CAR_COLORS[i]);
            play(SND_BOOM, fmaxf(vol, 0.25f));
            if (me) { shake = 0.5f; popup("Sizga tegdi!", COL_BAD); }
        }
        if (n->pops != o->pops) {
            add_ring(n->x, n->z, ITEM_COLORS[ITEM_SHIELD]);
            play(SND_POP, fmaxf(vol, 0.2f));
            if (me) popup("Qalqon himoya qildi!", COL_GOOD);
        }
        if (me && n->pickups != o->pickups && n->item) {
            char t[48];
            snprintf(t, sizeof(t), "%s olindi!", game_item_name(n->item));
            popup(t, ITEM_COLORS[n->item]);
            play(SND_PICKUP, 0.6f);
        }
        if (me && n->lap > o->lap && n->lap >= 1 && n->lap < b->laps) {
            char t[48];
            snprintf(t, sizeof(t), n->lap == b->laps - 1 ? "Oxirgi aylana!" : "%d-aylana", n->lap + 1);
            popup(t, COL_ACCENT);
        }
        if (me && n->place && !o->place) {
            char t[48];
            snprintf(t, sizeof(t), "Marra!  %d-o'rin", n->place);
            popup(t, COL_ACCENT);
            play(SND_FINISH, 0.6f);
        }
    }
}

static Input current_input;

static void screen_game_update(float dt)
{
    Input in = menu_open ? (Input){ 0, 0, my_fire_seq, my_fire_mode } : gather_input();
    current_input = in;
    if (mode == MODE_HOST) {
        server_update(&server, in, dt);
    } else if (mode == MODE_CLIENT) {
        double now = GetTime();
        if (now - last_input_send >= 1.0 / 60.0) { client_send_input(&client, in); last_input_send = now; }
        int st = client_status(&client);
        if (st == CL_LOST) { leave_game("Host bilan aloqa uzildi."); return; }
        if (st == CL_HOST_CLOSED) { leave_game("Host o'yinni yopdi."); return; }
    }
    GameState g;
    int ok = 0, slot = my_slot;
    if (mode == MODE_HOST) { server_get_state(&server, &g); ok = 1; slot = 0; }
    else if (mode == MODE_CLIENT) ok = client_get_state(&client, &g, &slot);
    if (!ok) return;
    my_slot = slot;
    /* xarita almashgan bo'lsa trassani shu yerda (asosiy oqimda) quramiz */
    if (g.map_id < MAP_COUNT && g.map_id != TRACK.map_id) { track_init(g.map_id); cam_init = 0; }
    game_place_pickups(&g);
    if (!have_view || g.tick != view.tick) {
        prev_view = have_view ? view : g;
        view = g;
        view_time = GetTime();
        if (have_view) detect_events(&prev_view, &view);
        have_view = 1;
    }
}

static void draw_game_ui(void)
{
    if (!have_view) { tcenter(F_SEMI, "Holat kutilmoqda...", vw / 2, VH / 2, 24, COL_TEXT); return; }
    if (view.phase == PHASE_LOBBY) { draw_lobby(&view); return; }
    const Player *me = &view.players[my_slot];
    draw_stats(&view);
    draw_leaderboard(&view);
    if (view.phase != PHASE_OVER) {
        draw_minimap(&view);
        if (!me->place) draw_item_slot(me);
    }
    if (view.phase == PHASE_COUNTDOWN) {
        char t[8];
        snprintf(t, sizeof(t), "%d", (int)ceilf(view.countdown));
        float k = view.countdown - floorf(view.countdown);
        float r = 80 + 20 * k;
        DrawCircleV((Vector2){ vw / 2, VH / 2 - 90 }, r, Fade(COL_BG, 0.75f));
        DrawRing((Vector2){ vw / 2, VH / 2 - 90 }, r - 6, r, -90, -90 + 360 * k, 60, COL_ACCENT);
        tcenter(F_BOLD, t, vw / 2, VH / 2 - 90, 96, WHITE);
        Rectangle ib = { vw / 2 - 380, VH / 2 + 40, 760, 92 };
        card(ib, 18, COL_SURFACE);
        tcenter(F_BOLD, "Qutilardan qurol oling va raqiblarni to'xtating!", vw / 2, ib.y + 32, 22, COL_TEXT);
        tcenter(F_BODY, IS_MOBILE ? "Rul - chap pastda,  OT - o'ng pastda,  gaz avtomatik"
                                  : "W/S - gaz/tormoz    A/D - rul    PROBEL - qurol", vw / 2, ib.y + 64, 17, COL_MUTED);
    }
    if (view.phase == PHASE_PLAY && me->wrong_way && ((int)(GetTime() * 3) & 1)) {
        Rectangle b = { vw / 2 - 210, 170, 420, 64 };
        DrawRectangleRounded(b, 0.5f, 10, Fade(COL_BAD, 0.9f));
        tcenter(F_BOLD, "TESKARI YO'NALISH!", vw / 2, b.y + 32, 28, WHITE);
    }
    if (view.phase == PHASE_PLAY && me->place) {
        Rectangle b = { vw / 2 - 230, VH - 130, 460, 92 };
        card(b, 20, COL_SURFACE);
        char t[48];
        snprintf(t, sizeof(t), "Marra!  %d-o'rin", me->place);
        tcenter(F_BOLD, t, vw / 2, b.y + 34, 30, COL_ACCENT);
        snprintf(t, sizeof(t), "Boshqalar kutilmoqda... %d s", (int)ceilf(view.over_timer));
        tcenter(F_BODY, t, vw / 2, b.y + 66, 16, COL_MUTED);
    }
    if (view.phase == PHASE_OVER) draw_results(&view);

    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].t <= 0) continue;
        float a = fminf(1, popups[i].t * 2.5f);
        float fs = 24, w = twidth(F_BOLD, popups[i].text, fs) + 60, h = 52;
        float y = 100 + i * 62 - (1 - fminf(1, (1.8f - popups[i].t) * 5)) * 20;
        Rectangle r = { vw / 2 - w / 2, y, w, h };
        DrawRectangleRounded((Rectangle){ r.x, r.y + 5, r.width, r.height }, 1, 12, Fade(BLACK, 0.3f * a));
        DrawRectangleRounded(r, 1, 12, Fade((Color){ 15, 20, 32, 255 }, 0.95f * a));
        DrawRectangleRoundedLinesEx(r, 1, 12, 2, Fade(popups[i].c, a));
        DrawCircleV((Vector2){ r.x + 26, r.y + h / 2 }, 7, Fade(popups[i].c, a));
        tmid(F_BOLD, popups[i].text, r.x + 42, r.y + h / 2, fs, Fade(WHITE, a));
    }

    if (!menu_open && view.phase != PHASE_OVER && pause_button()) menu_open = 1;
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACK)) menu_open = !menu_open;
    if (menu_open) {
        DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.7f));
        int host = mode == MODE_HOST;
        Rectangle P = { vw / 2 - 230, host ? 150 : 210, 460, host ? 420 : 280 };
        card(P, 24, COL_SURFACE);
        tcenter(F_BOLD, "Pauza menyusi", vw / 2, P.y + 44, 28, COL_TEXT);
        tcenter(F_BODY, "Poyga boshqalar uchun davom etyapti", vw / 2, P.y + 78, 15, COL_MUTED);
        float y = P.y + 108;
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Davom etish", BTN_PRIMARY, 1)) menu_open = 0;
        y += 70;
        if (host) {
            if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Qayta boshlash", BTN_SECONDARY, 1)) { server_begin_match(&server); menu_open = 0; }
            y += 70;
            if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Kutish xonasi", BTN_SECONDARY, 1)) { server_back_to_lobby(&server); menu_open = 0; }
            y += 70;
        }
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 50 }, "O'yindan chiqish", BTN_GHOST, 1)) { leave_game(NULL); return; }
    }
}

/* =================================================================== */
/*                     PARKOVKA REJIMI (1 o'yinchi)                    */
/* =================================================================== */
static PkGame  pk;
static uint8_t pk_stars[PK_LEVELS];          /* har bir darajadagi eng yaxshi natija (0-3) */
static int     pk_cam_mode = 0;              /* 0 - orqadan, 1 - yuqoridan */
static int     pk_paused = 0, pk_new_best = 0, pk_prev_state = PK_PLAY;
static float   pk_intro = 0, pk_cam_h = 0, pk_steer_in = 0;
static Vector3 pk_cam_smooth;
static int     pk_cam_ready = 0;
static int     pk_touch_gas = 0, pk_touch_rev = 0;
#define PK_SAVE_FILE "turbojang_parking.dat"

static const Color PARKED_COLORS[8] = {
    { 236, 238, 242, 255 }, { 150, 158, 172, 255 }, { 44, 48, 58, 255 }, { 40, 74, 146, 255 },
    { 176, 44, 52, 255 }, { 214, 196, 160, 255 }, { 26, 124, 124, 255 }, { 104, 112, 128, 255 },
};
static const char *PK_GROUP_NAMES[4] = { "Yashil vodiy", "Cho'l", "Qorli tog'", "Tungi shahar" };

static void pk_save(void)
{
    unsigned char buf[4 + PK_LEVELS];
    memcpy(buf, "TJP1", 4);
    memcpy(buf + 4, pk_stars, PK_LEVELS);
    SaveFileData(PK_SAVE_FILE, buf, (int)sizeof(buf));
}

static void pk_load_save(void)
{
    int n = 0;
    unsigned char *d = LoadFileData(PK_SAVE_FILE, &n);
    if (d && n >= 4 + PK_LEVELS && memcmp(d, "TJP1", 4) == 0)
        for (int i = 0; i < PK_LEVELS; i++) pk_stars[i] = d[4 + i] <= 3 ? d[4 + i] : 0;
    if (d) UnloadFileData(d);
}

static int park_unlocked(int k) { return k == 0 || pk_stars[k - 1] > 0; }
static int park_total_stars(void) { int t = 0; for (int i = 0; i < PK_LEVELS; i++) t += pk_stars[i]; return t; }

static void park_start(int k)
{
    pk_load(&pk, k);
    screen = SCR_PARK;
    pk_paused = 0;
    pk_new_best = 0;
    pk_prev_state = PK_PLAY;
    pk_intro = 3.5f;
    pk_cam_ready = 0;
    pk_steer_in = 0;
    for (int i = 0; i < MAX_PARTS; i++) parts[i].life = 0;
}

static void park_open_select(void) { screen = SCR_PARK_SELECT; }

/* ---------------------------------------------------------- yulduz */
static void draw_star(float cx, float cy, float r, Color c)
{
    Vector2 p[10];
    for (int k = 0; k < 10; k++) {
        float a = -PI / 2 + k * PI / 5, rr = (k & 1) ? r * 0.46f : r;
        p[k] = (Vector2){ cx + cosf(a) * rr, cy + sinf(a) * rr };
    }
    for (int k = 0; k < 10; k++) DrawTriangle((Vector2){ cx, cy }, p[(k + 1) % 10], p[k], c);
}

static void draw_stars_row(float cx, float cy, float r, int n, float gap)
{
    for (int k = 0; k < 3; k++) {
        float x = cx + (k - 1) * gap;
        draw_star(x, cy + 1.5f, r, Fade(BLACK, 0.35f));
        draw_star(x, cy, r, k < n ? (Color){ 250, 204, 21, 255 } : (Color){ 58, 66, 92, 255 });
    }
}

static void draw_lock(float cx, float cy, float s, Color c)
{
    DrawRing((Vector2){ cx, cy - 4 * s }, 7 * s, 10 * s, 180, 360, 16, c);
    DrawRectangleRounded((Rectangle){ cx - 13 * s, cy - 4 * s, 26 * s, 20 * s }, 0.3f, 6, c);
    DrawCircleV((Vector2){ cx, cy + 5 * s }, 3 * s, COL_BG);
}

/* "P" belgisi (menyu va HUD uchun) */
static void draw_p_badge(float cx, float cy, float s)
{
    Rectangle r = { cx - 34 * s, cy - 34 * s, 68 * s, 68 * s };
    DrawRectangleRounded((Rectangle){ r.x, r.y + 4 * s, r.width, r.height }, 0.28f, 10, Fade(BLACK, 0.3f));
    DrawRectangleRounded(r, 0.28f, 10, (Color){ 37, 99, 235, 255 });
    DrawRectangleRoundedLinesEx((Rectangle){ r.x + 4 * s, r.y + 4 * s, r.width - 8 * s, r.height - 8 * s }, 0.24f, 10, 2.5f * s, Fade(WHITE, 0.9f));
    tcenter(F_BOLD, "P", cx + 1 * s, cy + 2 * s, 50 * s, WHITE);
}

/* =========================================================== 3D sahna */
static void pk_line(float x0, float z0, float x1, float z1, float w, float y, Color c)
{
    float dx = x1 - x0, dz = z1 - z0, len = sqrtf(dx * dx + dz * dz);
    if (len < 0.001f) return;
    float nx = -dz / len * w / 2, nz = dx / len * w / 2;
    Color f = fogc(c, x0, z0);
    rlCheckRenderBatchLimit(6);
    rlBegin(RL_TRIANGLES);
    quad((Vector3){ x0 + nx, y, z0 + nz }, (Vector3){ x1 + nx, y, z1 + nz }, (Vector3){ x1 - nx, y, z1 - nz }, (Vector3){ x0 - nx, y, z0 - nz }, f);
    rlEnd();
}

static void pk_bay_lines(const PkBay *b, Color c, float w, float y)
{
    float k[4][2];
    pk_corners(b->x, b->z, b->h, b->w, b->l, k);
    /* burchaklar: 0 (+s,+f) 1 (-s,+f) 2 (-s,-f) 3 (+s,-f); f - joyning ichkarisi */
    pk_line(k[0][0], k[0][1], k[3][0], k[3][1], w, y, c);
    pk_line(k[1][0], k[1][1], k[2][0], k[2][1], w, y, c);
    pk_line(k[0][0], k[0][1], k[1][0], k[1][1], w, y, c);
    if (b->parallel) pk_line(k[2][0], k[2][1], k[3][0], k[3][1], w, y, c);
}

static void pk_draw_cone(float x, float z, int down, float rot)
{
    Color orange = { 249, 115, 22, 255 };
    if (!down) {
        DrawCube((Vector3){ x, 0.03f, z }, 0.62f, 0.06f, 0.62f, fogc((Color){ 30, 30, 34, 255 }, x, z));
        scone((Vector3){ x, 0.06f, z }, 0.3f, 0.82f, 10, orange);
        DrawCylinder((Vector3){ x, 0.34f, z }, 0.19f, 0.16f, 0.14f, 10, fogc(WHITE, x, z));
        return;
    }
    rlPushMatrix();
    rlTranslatef(x, 0.28f, z);
    rlRotatef(rot * RAD2DEG, 0, 1, 0);
    rlRotatef(90, 1, 0, 0);
    fog_override = fog_amount(x, z);
    DrawCylinderEx((Vector3){ 0, -0.4f, 0 }, (Vector3){ 0, 0.4f, 0 }, 0.28f, 0.03f, 10, orange);
    DrawCylinderEx((Vector3){ 0, -0.05f, 0 }, (Vector3){ 0, 0.1f, 0 }, 0.2f, 0.17f, 10, WHITE);
    fog_override = -1;
    rlPopMatrix();
}

static void pk_draw_obs(const PkObs *o)
{
    if (!in_view(o->x, o->z, o->l)) return;
    Color concrete = TH->night ? (Color){ 120, 124, 140, 255 } : (Color){ 186, 188, 196, 255 };
    Color yellow = { 250, 204, 21, 255 }, black = { 30, 30, 34, 255 };
    fog_override = fog_amount(o->x, o->z);
    rlPushMatrix();
    rlTranslatef(o->x, 0, o->z);
    rlRotatef(o->h * RAD2DEG, 0, 1, 0);
    switch (o->type) {
    case OB_CAR:
        car_lights = 0;
        draw_car_model(PARKED_COLORS[o->color], 0, 0, 0, 0);
        car_lights = 1;
        break;
    case OB_TRUCK: {
        static const Color TC[4] = { { 37, 99, 235, 255 }, { 220, 38, 38, 255 }, { 22, 163, 74, 255 }, { 234, 88, 12, 255 } };
        Color cab = TC[o->color & 3];
        sbox((Vector3){ 0, 1.45f, 3.35f }, 2.5f, 2.5f, 2.2f, cab);
        ebox((Vector3){ 0, 1.95f, 4.46f }, 2.2f, 0.9f, 0.04f, (Color){ 48, 66, 92, 255 });
        sbox((Vector3){ 0, 2.0f, -1.15f }, 2.6f, 3.3f, 6.6f, (Color){ 238, 240, 244, 255 });
        sbox((Vector3){ 0, 1.2f, -1.15f }, 2.64f, 0.3f, 6.64f, cab);
        sbox((Vector3){ 0, 0.4f, 0 }, 2.2f, 0.3f, 8.6f, (Color){ 40, 42, 50, 255 });
        for (int sx = -1; sx <= 1; sx += 2)
            for (int k = 0; k < 3; k++) {
                float wz = k == 0 ? 3.2f : k == 1 ? -2.4f : -3.6f;
                DrawCylinderEx((Vector3){ sx * 1.0f, 0.5f, wz }, (Vector3){ sx * 1.32f, 0.5f, wz }, 0.5f, 0.5f, 12, (Color){ 24, 24, 28, 255 });
            }
        ebox((Vector3){ -0.9f, 0.9f, 4.46f }, 0.4f, 0.18f, 0.04f, (Color){ 255, 248, 210, 255 });
        ebox((Vector3){ 0.9f, 0.9f, 4.46f }, 0.4f, 0.18f, 0.04f, (Color){ 255, 248, 210, 255 });
        break;
    }
    case OB_WALL: {
        float H = o->w <= 0.35f ? 2.4f : 1.2f;
        sbox((Vector3){ 0, H / 2, 0 }, o->w, H, o->l, concrete);
        int n = (int)(o->l / 1.2f);
        if (n < 1) n = 1;
        for (int i = 0; i < n; i++)   /* sariq-qora ogohlantiruvchi chiziq */
            ebox((Vector3){ 0, H + 0.04f, -o->l / 2 + (i + 0.5f) * o->l / n }, o->w + 0.02f, 0.08f, o->l / n, (i & 1) ? yellow : black);
        if (H > 2) ebox((Vector3){ 0, 0.9f, 0 }, o->w + 0.03f, 0.15f, o->l, TH->night ? TH->curb_b : yellow);
        break;
    }
    case OB_BARRIER: {
        int n = (int)(o->l / 2.0f);
        if (n < 1) n = 1;
        for (int i = 0; i < n; i++) {
            float z = -o->l / 2 + (i + 0.5f) * o->l / n, sl = o->l / n - 0.08f;
            Color c = (i & 1) ? (Color){ 240, 240, 244, 255 } : (Color){ 220, 38, 38, 255 };
            sbox((Vector3){ 0, 0.18f, z }, o->w, 0.36f, sl, c);
            sbox((Vector3){ 0, 0.62f, z }, o->w * 0.5f, 0.56f, sl, c);
        }
        break;
    }
    case OB_PILLAR:
        sbox((Vector3){ 0, 2.0f, 0 }, o->w, 4.0f, o->l, concrete);
        for (int i = 0; i < 4; i++) ebox((Vector3){ 0, 0.15f + i * 0.3f, 0 }, o->w + 0.03f, 0.3f, o->l + 0.03f, (i & 1) ? black : yellow);
        if (TH->night) ebox((Vector3){ 0, 3.8f, 0 }, o->w + 0.06f, 0.2f, o->l + 0.06f, (Color){ 255, 236, 180, 255 });
        break;
    case OB_PLANTER: {
        sbox((Vector3){ 0, 0.35f, 0 }, o->w, 0.7f, o->l, TH->decor == DECOR_DESERT ? (Color){ 200, 150, 100, 255 } : (Color){ 150, 140, 130, 255 });
        sbox((Vector3){ 0, 0.72f, 0 }, o->w - 0.3f, 0.06f, o->l - 0.3f, (Color){ 90, 64, 44, 255 });
        float big = fmaxf(o->w, o->l);
        int n = (int)(big / 1.4f);
        if (n < 1) n = 1;
        Color g = TH->decor == DECOR_SNOW ? (Color){ 230, 236, 244, 255 } : TH->decor == DECOR_DESERT ? (Color){ 110, 150, 80, 255 } : (Color){ 52, 140, 72, 255 };
        for (int i = 0; i < n; i++) {
            float t = -big / 2 + (i + 0.5f) * big / n;
            Vector3 p = o->w > o->l ? (Vector3){ t, 1.1f, 0 } : (Vector3){ 0, 1.1f, t };
            DrawSphereEx(p, fminf(o->w, o->l) * 0.42f + 0.1f * (i & 1), 6, 8, mul(g, (i & 1) ? 1.0f : 0.9f));
        }
        break;
    }
    }
    rlPopMatrix();
    fog_override = -1;
}

/* maqsad joyi: yorug' maydon, chegaralar, yo'nalish strelkalari, "P" belgisi */
static void pk_draw_target(float time)
{
    const PkBay *t = &pk.L.target;
    float fit = pk_fit(&pk.L, pk.car.x, pk.car.z, pk.car.h);
    float pulse = 0.5f + 0.5f * sinf(time * 4);
    int ok = fit >= 0.999f;
    Color col = ok ? (Color){ 74, 222, 128, 255 } : pk.L.need == NEED_REVERSE_IN ? (Color){ 56, 189, 248, 255 } : (Color){ 34, 197, 94, 255 };
    float k[4][2];
    pk_corners(t->x, t->z, t->h, t->w, t->l, k);
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlCheckRenderBatchLimit(6);
    rlBegin(RL_TRIANGLES);
    quad((Vector3){ k[0][0], 0.015f, k[0][1] }, (Vector3){ k[1][0], 0.015f, k[1][1] }, (Vector3){ k[2][0], 0.015f, k[2][1] },
         (Vector3){ k[3][0], 0.015f, k[3][1] }, Fade(col, ok ? 0.45f : 0.16f + 0.12f * pulse));
    rlEnd();
    /* uzoqdan ko'rinadigan nur ustuni */
    float dx = pk.car.x - t->x, dz = pk.car.z - t->z, d = sqrtf(dx * dx + dz * dz);
    if (d > 7 && pk.state == PK_PLAY)
        DrawCylinder((Vector3){ t->x, 0, t->z }, 1.0f, 0.3f, 14, 12, Fade(col, fminf(0.22f, (d - 7) * 0.03f)));
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    for (int i = 0; i < 4; i++)
        pk_line(k[i][0], k[i][1], k[(i + 1) % 4][0], k[(i + 1) % 4][1], 0.24f, 0.025f, ok ? WHITE : lerp_color(col, WHITE, 0.3f * pulse));
    /* yo'nalish talab qilinsa - strelkalar mashina burni qayerga qarashi kerakligini ko'rsatadi */
    if (pk.L.need != NEED_ANY) {
        float h = pk.L.need == NEED_REVERSE_IN ? t->h + PI : t->h;
        float fx = sinf(h), fz = cosf(h), sx = cosf(h), sz = -sinf(h);
        for (int c = 0; c < 2; c++) {
            float o = -0.6f + c * 1.3f + fmodf(time * 0.8f, 1.3f) * 0.3f;
            float tx = t->x + fx * (o + 0.6f), tz = t->z + fz * (o + 0.6f);
            float bx = t->x + fx * o, bz = t->z + fz * o;
            pk_line(bx + sx * 0.8f, bz + sz * 0.8f, tx, tz, 0.22f, 0.03f, WHITE);
            pk_line(bx - sx * 0.8f, bz - sz * 0.8f, tx, tz, 0.22f, 0.03f, WHITE);
        }
    }
    /* joy orqasida ko'k "P" belgisi */
    float bh = t->h, back = t->l / 2 + 0.7f;
    if (t->parallel) { bh = t->h - PI / 2; back = t->w / 2 + 0.7f; }
    float px = t->x + sinf(bh) * back, pz = t->z + cosf(bh) * back;
    fog_override = fog_amount(px, pz);
    rlPushMatrix();
    rlTranslatef(px, 0, pz);
    rlRotatef(bh * RAD2DEG, 0, 1, 0);
    DrawCylinderEx((Vector3){ 0, 0, 0 }, (Vector3){ 0, 2.4f, 0 }, 0.06f, 0.06f, 6, (Color){ 120, 124, 134, 255 });
    DrawCube((Vector3){ 0, 2.75f, 0 }, 1.1f, 1.1f, 0.06f, (Color){ 37, 99, 235, 255 });
    DrawCube((Vector3){ 0, 2.75f, -0.035f }, 0.96f, 0.96f, 0.02f, WHITE);
    DrawCube((Vector3){ 0, 2.75f, -0.045f }, 0.88f, 0.88f, 0.02f, (Color){ 37, 99, 235, 255 });
    /* P harfi (mashina tomonga qaragan yuzada) */
    DrawCube((Vector3){ 0.18f, 2.75f, -0.06f }, 0.12f, 0.64f, 0.02f, WHITE);
    DrawCube((Vector3){ 0.0f, 3.01f, -0.06f }, 0.3f, 0.12f, 0.02f, WHITE);
    DrawCube((Vector3){ 0.0f, 2.77f, -0.06f }, 0.3f, 0.12f, 0.02f, WHITE);
    DrawCube((Vector3){ -0.14f, 2.89f, -0.06f }, 0.12f, 0.24f, 0.02f, WHITE);
    rlPopMatrix();
    fog_override = -1;
}

static void pk_draw_lot(float time)
{
    const PkLevel *L = &pk.L;
    float w = L->lot_w / 2 + 1.2f, l = L->lot_l / 2 + 1.2f;
    /* asfalt va plitalar */
    Color as = mul(TH->decor == DECOR_CITY ? (Color){ 52, 54, 68, 255 } : (Color){ 72, 75, 84, 255 }, TH->decor == DECOR_CITY ? 1.0f : TH->bright);
    rlCheckRenderBatchLimit(6);
    rlBegin(RL_TRIANGLES);
    quad((Vector3){ -w, 0.0f, -l }, (Vector3){ w, 0.0f, -l }, (Vector3){ w, 0.0f, l }, (Vector3){ -w, 0.0f, l }, fogc(as, 0, 0));
    rlEnd();
    for (float x = -w; x < w - 0.1f; x += 6)
        for (float z = -l; z < l - 0.1f; z += 6) {
            uint32_t h = hash32((uint32_t)((int)(x + 100) * 131 + (int)(z + 100) * 7));
            if (h % 3) continue;
            float x1 = fminf(x + 6, w), z1 = fminf(z + 6, l);
            rlCheckRenderBatchLimit(6);
            rlBegin(RL_TRIANGLES);
            quad((Vector3){ x, 0.004f, z }, (Vector3){ x1, 0.004f, z }, (Vector3){ x1, 0.004f, z1 }, (Vector3){ x, 0.004f, z1 }, fogc(mul(as, (h >> 4) & 1 ? 1.06f : 0.95f), x, z));
            rlEnd();
        }
    /* bordyur */
    Color curb = TH->night ? (Color){ 90, 94, 112, 255 } : (Color){ 210, 212, 218, 255 };
    pk_line(-w, -l, w, -l, 0.5f, 0.02f, curb); pk_line(-w, l, w, l, 0.5f, 0.02f, curb);
    pk_line(-w, -l, -w, l, 0.5f, 0.02f, curb); pk_line(w, -l, w, l, 0.5f, 0.02f, curb);
    /* joy chiziqlari */
    Color paint = TH->decor == DECOR_CITY ? (Color){ 250, 220, 120, 255 } : TH->decor == DECOR_SNOW ? (Color){ 250, 204, 21, 255 } : (Color){ 240, 242, 246, 255 };
    for (int i = 0; i < L->nbays; i++) pk_bay_lines(&L->bays[i], paint, 0.14f, 0.012f);
    /* start chizig'i */
    float sx = cosf(L->sh), sz = -sinf(L->sh);
    float bx = L->sx - sinf(L->sh) * 2.8f, bz = L->sz - cosf(L->sh) * 2.8f;
    for (int i = -3; i <= 3; i++) {
        float ox = bx + sx * i * 0.5f, oz = bz + sz * i * 0.5f;
        pk_line(ox - sx * 0.25f, oz - sz * 0.25f, ox + sx * 0.25f, oz + sz * 0.25f, 0.5f, 0.012f, (i & 1) ? WHITE : (Color){ 30, 30, 34, 255 });
    }
    /* tunda: chiroq ustunlari */
    if (TH->night) {
        for (int s = 0; s < 4; s++) {
            int n = (int)((s < 2 ? L->lot_w : L->lot_l) / 16) + 1;
            for (int i = 0; i <= n; i++) {
                float f = (float)i / n;
                float x = s < 2 ? -w + f * 2 * w : (s == 2 ? -w - 1.5f : w + 1.5f);
                float z = s < 2 ? (s == 0 ? -l - 1.5f : l + 1.5f) : -l + f * 2 * l;
                sbox((Vector3){ x, 3.5f, z }, 0.3f, 7.0f, 0.3f, (Color){ 60, 64, 80, 255 });
                ebox((Vector3){ x, 7.1f, z }, 0.9f, 0.25f, 0.9f, (Color){ 255, 236, 180, 255 });
                DrawCylinder((Vector3){ x, 0.03f, z }, 6.0f, 6.0f, 0.01f, 16, Fade((Color){ 255, 220, 140, 255 }, 0.1f));
            }
        }
    }
    /* maydon atrofidagi bezaklar */
    for (int i = 0; i < 90; i++) {
        uint32_t h = hash32((uint32_t)i * 977u + (uint32_t)pk.level * 31u + 5u);
        float per = (float)(h % 10000) / 10000.0f, out = 6.0f + (float)((h >> 14) % 2600) / 100.0f;
        if (TH->decor == DECOR_CITY) out += 8.0f;   /* binolar kamera yo'lidan uzoqroqda */
        float x, z;
        float P = 2 * (w + l), d = per * P * 2;
        d = fmodf(d, P * 2);
        if (d < 2 * w)               { x = -w + d;               z = -l - out; }
        else if (d < 2 * w + 2 * l)  { x = w + out;              z = -l + (d - 2 * w); }
        else if (d < 4 * w + 2 * l)  { x = w - (d - 2 * w - 2 * l); z = l + out; }
        else                         { x = -w - out;             z = l - (d - 4 * w - 2 * l); }
        V2 q = { x, z };
        float sc = 0.8f + (float)((h >> 8) % 60) / 100.0f;
        switch (TH->decor) {
        case DECOR_DESERT: deco_desert(q, sc, h); break;
        case DECOR_SNOW:   deco_snow(q, sc, h); break;
        case DECOR_CITY:   deco_building(q, sc, fminf(9.0f, out - 8.0f), h); break;
        default:           deco_tree(q, sc, h); break;
        }
    }
    (void)time;
}

static Camera3D pk_camera(float dt)
{
    PkCar *c = &pk.car;
    float d = fmodf(c->h - pk_cam_h + 3 * PI, 2 * PI) - PI;
    if (!pk_cam_ready) { pk_cam_h = c->h; d = 0; }
    pk_cam_h += d * fminf(1, dt * 2.5f);
    Vector3 want, tgt;
    if (pk_cam_mode == 0) {
        want = (Vector3){ c->x - sinf(pk_cam_h) * 10.5f, 6.8f, c->z - cosf(pk_cam_h) * 10.5f };
        tgt = (Vector3){ c->x + sinf(pk_cam_h) * 4.0f, 0.4f, c->z + cosf(pk_cam_h) * 4.0f };
    } else {
        /* yuqoridan: mashina va maqsad orasiga qaraydi */
        float mx = c->x + (pk.L.target.x - c->x) * 0.35f, mz = c->z + (pk.L.target.z - c->z) * 0.35f;
        want = (Vector3){ mx, 36.0f, mz - 9.0f };
        tgt = (Vector3){ mx, 0, mz + 1.0f };
    }
    /* kamera maydondan uzoqqa chiqib ketmasin (atrofdagi daraxt/binolar ichida qolmasin) */
    float mw = pk.L.lot_w / 2 + 3.0f, ml = pk.L.lot_l / 2 + 3.0f;
    want.x = fmaxf(-mw, fminf(mw, want.x));
    want.z = fmaxf(-ml, fminf(ml, want.z));
    if (!pk_cam_ready) { pk_cam_smooth = want; pk_cam_ready = 1; }
    float k = fminf(1, dt * 6);
    pk_cam_smooth.x += (want.x - pk_cam_smooth.x) * k;
    pk_cam_smooth.y += (want.y - pk_cam_smooth.y) * k;
    pk_cam_smooth.z += (want.z - pk_cam_smooth.z) * k;
    Camera3D cam = { 0 };
    cam.position = pk_cam_smooth;
    cam.target = tgt;
    cam.up = (Vector3){ 0, 1, 0 };
    cam.fovy = pk_cam_mode ? 50.0f : 56.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    if (shake > 0) { cam.position.x += frand(-1, 1) * shake * 0.4f; cam.position.y += frand(-1, 1) * shake * 0.3f; }
    cam_pos = cam.position;
    cam_heading = pk_cam_mode ? 0 : pk_cam_h;
    return cam;
}

static void pk_draw_scene(float time, float dt)
{
    Camera3D cam = pk_camera(dt);
    draw_sky();
    BeginMode3D(cam);
    rlDisableBackfaceCulling();
    draw_hills();
    draw_ground();
    pk_draw_lot(time);
    pk_draw_target(time);
    for (int i = 0; i < pk.L.nobs; i++) pk_draw_obs(&pk.L.obs[i]);
    for (int i = 0; i < pk.L.ncones; i++) pk_draw_cone(pk.L.cone_x[i], pk.L.cone_z[i], pk.cone_down[i], pk.cone_rot[i]);
    /* o'yinchi mashinasi */
    PkCar *c = &pk.car;
    car_wheel_steer = -c->steer * PK_MAX_STEER * RAD2DEG;
    car_reverse = c->speed < -0.1f;
    fog_override = fog_amount(c->x, c->z);
    rlPushMatrix();
    rlTranslatef(c->x, 0, c->z);
    rlRotatef(c->h * RAD2DEG, 0, 1, 0);
    float roll = -c->steer * fminf(fabsf(c->speed) / PK_MAX_FWD, 1) * 3.0f * (c->speed >= 0 ? 1 : -1);
    draw_car_model(CAR_COLORS[0], 0, c->speed, time, roll);
    rlPopMatrix();
    fog_override = -1;
    car_wheel_steer = 0;
    car_reverse = 0;
    /* maqsadga yo'naltiruvchi strelka (uzoqda bo'lsa) */
    float dx = pk.L.target.x - c->x, dz = pk.L.target.z - c->z, d = sqrtf(dx * dx + dz * dz);
    if (d > 9 && pk.state == PK_PLAY) {
        float a = atan2f(dx, dz), bob = sinf(time * 4) * 0.15f;
        float fx = sinf(a), fz = cosf(a), sx = cosf(a), sz = -sinf(a);
        Vector3 tip = { c->x + fx * 3.4f, 3.0f + bob, c->z + fz * 3.4f };
        Vector3 l = { c->x + fx * 1.8f + sx * 0.9f, 3.0f + bob, c->z + fz * 1.8f + sz * 0.9f };
        Vector3 r = { c->x + fx * 1.8f - sx * 0.9f, 3.0f + bob, c->z + fz * 1.8f - sz * 0.9f };
        Vector3 m = { c->x + fx * 2.3f, 3.0f + bob, c->z + fz * 2.3f };
        rlCheckRenderBatchLimit(6);
        rlBegin(RL_TRIANGLES);
        tri(l, tip, m, (Color){ 74, 222, 128, 230 });
        tri(m, tip, r, (Color){ 34, 197, 94, 230 });
        rlEnd();
    }
    for (int i = 0; i < MAX_PARTS; i++) {
        Part *q = &parts[i];
        if (q->life <= 0) continue;
        Color cc = q->c;
        cc.a = (unsigned char)(cc.a * q->life / q->max);
        float s = q->size * (0.4f + 0.6f * q->life / q->max);
        DrawCube(q->p, s, s, s, cc);
    }
    if (TH->decor == DECOR_SNOW) {
        for (int i = 0; i < 300; i++) {
            uint32_t h = hash32((uint32_t)i * 7919u + 17u);
            float fx = (float)(h % 1000) / 1000.0f * 80 - 40, fz = (float)((h >> 10) % 1000) / 1000.0f * 80 - 40;
            float fy = fmodf((float)((h >> 20) % 1000) / 1000.0f * 20 - time * (2.0f + (h % 3)), 20.0f);
            if (fy < 0) fy += 20;
            DrawCube((Vector3){ c->x + fx + sinf(time + i) * 0.6f, fy, c->z + fz }, 0.16f, 0.16f, 0.16f, Fade(WHITE, 0.9f));
        }
    }
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    EndMode3D();
}

/* ============================================================ boshqaruv */
typedef struct { Rectangle left, right, gas, rev, brake; } PkTouch;

static PkTouch pk_touch_layout(void)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight(), u = sh / 720.0f;
    PkTouch T;
    T.left = (Rectangle){ 24 * u, sh - 170 * u, 140 * u, 140 * u };
    T.right = (Rectangle){ 184 * u, sh - 170 * u, 140 * u, 140 * u };
    T.gas = (Rectangle){ sw - 184 * u, sh - 210 * u, 160 * u, 180 * u };
    T.rev = (Rectangle){ sw - 344 * u, sh - 150 * u, 144 * u, 120 * u };
    T.brake = (Rectangle){ sw - 344 * u, sh - 290 * u, 144 * u, 120 * u };
    return T;
}

static void pk_gather(float *steer, float *thr, int *hb)
{
    float s = 0, t = 0;
    int b = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) s -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) s += 1;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) t += 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) t -= 1;
    if (IsKeyDown(KEY_SPACE)) b = 1;
    if (IsGamepadAvailable(0)) {
        float ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(ax) > 0.15f) s = ax;
        float rt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER), lt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER);
        if (rt > -0.9f) t += (rt + 1) / 2;
        if (lt > -0.9f) t -= (lt + 1) / 2;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) b = 1;
    }
#if IS_MOBILE
    PkTouch T = pk_touch_layout();
    pk_touch_gas = pk_touch_rev = 0;
    for (int i = 0; i < GetTouchPointCount(); i++) {
        Vector2 p = GetTouchPosition(i);
        if (CheckCollisionPointRec(p, T.left)) s = -1;
        else if (CheckCollisionPointRec(p, T.right)) s = 1;
        else if (CheckCollisionPointRec(p, T.gas)) { t = 1; pk_touch_gas = 1; }
        else if (CheckCollisionPointRec(p, T.rev)) { t = -1; pk_touch_rev = 1; }
        else if (CheckCollisionPointRec(p, T.brake)) b = 1;
    }
#endif
    *steer = fmaxf(-1, fminf(1, s));
    *thr = fmaxf(-1, fminf(1, t));
    *hb = b;
}

static void pk_draw_touch(float steer, int hb)
{
#if IS_MOBILE
    PkTouch T = pk_touch_layout();
    float u = GetScreenHeight() / 720.0f;
    Rectangle rs[5] = { T.left, T.right, T.gas, T.rev, T.brake };
    int on[5] = { steer < -0.1f, steer > 0.1f, pk_touch_gas, pk_touch_rev, hb };
    Color cs[5] = { WHITE, WHITE, COL_GOOD, COL_INFO, COL_BAD };
    for (int i = 0; i < 5; i++) {
        Rectangle r = rs[i];
        DrawRectangleRounded(r, 0.35f, 10, Fade(i < 2 ? BLACK : cs[i], on[i] ? 0.55f : 0.28f));
        DrawRectangleRoundedLinesEx(r, 0.35f, 10, 2 * u, Fade(WHITE, 0.5f));
        float cx = r.x + r.width / 2, cy = r.y + r.height / 2, s = 26 * u;
        if (i == 0) DrawTriangle((Vector2){ cx + s * 0.6f, cy - s }, (Vector2){ cx - s * 0.8f, cy }, (Vector2){ cx + s * 0.6f, cy + s }, Fade(WHITE, 0.9f));
        if (i == 1) DrawTriangle((Vector2){ cx - s * 0.6f, cy - s }, (Vector2){ cx - s * 0.6f, cy + s }, (Vector2){ cx + s * 0.8f, cy }, Fade(WHITE, 0.9f));
        if (i == 2) tcenter(F_BOLD, "GAZ", cx, cy, 30 * u, WHITE);
        if (i == 3) tcenter(F_BOLD, "ORQAGA", cx, cy, 20 * u, WHITE);
        if (i == 4) tcenter(F_BOLD, "TO'XTA", cx, cy, 20 * u, WHITE);
    }
#else
    (void)steer; (void)hb;
#endif
}

/* ============================================================== yangilash */
static void pk_update(float dt)
{
    float steer = 0, thr = 0;
    int hb = 0;
    if (pk_intro > 0) pk_intro -= dt;
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACK)) { if (pk.state == PK_PLAY) pk_paused = !pk_paused; else { park_open_select(); return; } }
    if (pk_paused) return;
    if (IsKeyPressed(KEY_C)) pk_cam_mode ^= 1;
    if (IsKeyPressed(KEY_R)) { park_start(pk.level); return; }
    if (pk.state == PK_PLAY) pk_gather(&steer, &thr, &hb);
    pk_steer_in = steer;
    pk_step(&pk, steer, thr, hb, dt);
    if (pk.ev_hit) {
        pk.ev_hit = 0;
        play(SND_SHUNT, 0.6f);
        shake = 0.25f + pk.ev_power * 0.03f;
        for (int k = 0; k < 18; k++)
            emit((Vector3){ pk.ev_x, 0.7f, pk.ev_z }, (Vector3){ frand(-4, 4), frand(2, 6), frand(-4, 4) }, 0.6f, 0.16f, k & 1 ? (Color){ 255, 220, 120, 255 } : WHITE, 12);
        popup("Urilish! Daraja tugadi", COL_BAD);
    }
    if (pk.ev_cone) {
        pk.ev_cone = 0;
        play(SND_POP, 0.6f);
        for (int k = 0; k < 10; k++)
            emit((Vector3){ pk.ev_x, 0.5f, pk.ev_z }, (Vector3){ frand(-3, 3), frand(1, 4), frand(-3, 3) }, 0.5f, 0.14f, (Color){ 249, 115, 22, 255 }, 10);
        popup("Konus urib ketildi!", (Color){ 249, 115, 22, 255 });
    }
    /* g'ildirak ostidan chang/qor */
    if (fabsf(pk.car.speed) > 4 && (rand() % 4 == 0)) {
        Color dust = TH->decor == DECOR_SNOW ? Fade(WHITE, 0.7f) : TH->decor == DECOR_DESERT ? (Color){ 220, 190, 140, 110 } : (Color){ 200, 200, 210, 70 };
        float bx = pk.car.x - sinf(pk.car.h) * 2.2f, bz = pk.car.z - cosf(pk.car.h) * 2.2f;
        emit((Vector3){ bx + frand(-0.8f, 0.8f), 0.2f, bz + frand(-0.8f, 0.8f) }, (Vector3){ frand(-0.5f, 0.5f), frand(0.3f, 1.0f), frand(-0.5f, 0.5f) }, 0.5f, 0.16f, dust, -0.5f);
    }
    if (pk.state != pk_prev_state) {
        if (pk.state == PK_WON) {
            play(SND_FINISH, 0.8f);
            if (pk.stars > pk_stars[pk.level]) { pk_stars[pk.level] = (uint8_t)pk.stars; pk_new_best = 1; pk_save(); }
            for (int k = 0; k < 40; k++)
                emit((Vector3){ pk.car.x, 1.5f, pk.car.z }, (Vector3){ frand(-5, 5), frand(4, 9), frand(-5, 5) }, 1.4f, 0.2f,
                     (Color[]){ { 250, 204, 21, 255 }, { 74, 222, 128, 255 }, { 56, 189, 248, 255 }, { 239, 68, 68, 255 } }[k & 3], 9);
        } else if (pk.state == PK_FAILED) {
            play(SND_BOOM, 0.6f);
        }
        pk_prev_state = pk.state;
    }
}

/* =================================================================== HUD */
static void pk_minimap(void)
{
    const PkLevel *L = &pk.L;
    float sc = fminf(210 / L->lot_w, 210 / L->lot_l);
    float w = L->lot_w * sc, h = L->lot_l * sc;
    Rectangle R = { vw - 16 - w - 20, 84, w + 20, h + 20 };
    card(R, 16, COL_SURFACE);
    float ox = R.x + 10, oy = R.y + 10;
#define MX(x) (ox + (L->lot_w / 2 - (x)) * sc)
#define MY(z) (oy + (L->lot_l / 2 - (z)) * sc)
    DrawRectangleRounded((Rectangle){ ox, oy, w, h }, 0.04f, 6, (Color){ 40, 44, 58, 255 });
    for (int i = 0; i < L->nbays; i++) {
        const PkBay *b = &L->bays[i];
        DrawRectanglePro((Rectangle){ MX(b->x), MY(b->z), b->w * sc, b->l * sc }, (Vector2){ b->w * sc / 2, b->l * sc / 2 }, -b->h * RAD2DEG, Fade(WHITE, 0.07f));
    }
    const PkBay *t = &L->target;
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5);
    DrawRectanglePro((Rectangle){ MX(t->x), MY(t->z), t->w * sc, t->l * sc }, (Vector2){ t->w * sc / 2, t->l * sc / 2 }, -t->h * RAD2DEG, Fade(COL_GOOD, 0.5f + 0.4f * pulse));
    for (int i = 0; i < L->nobs; i++) {
        const PkObs *o = &L->obs[i];
        Color c = o->type == OB_CAR ? PARKED_COLORS[o->color] : o->type == OB_TRUCK ? (Color){ 230, 232, 236, 255 } : o->type == OB_BARRIER ? (Color){ 220, 38, 38, 255 } : (Color){ 150, 156, 170, 255 };
        if (o->type == OB_CAR) c = lerp_color(c, (Color){ 40, 44, 58, 255 }, 0.35f);
        DrawRectanglePro((Rectangle){ MX(o->x), MY(o->z), fmaxf(o->w * sc, 1.5f), fmaxf(o->l * sc, 1.5f) }, (Vector2){ fmaxf(o->w * sc, 1.5f) / 2, fmaxf(o->l * sc, 1.5f) / 2 }, -o->h * RAD2DEG, c);
    }
    for (int i = 0; i < L->ncones; i++)
        if (!pk.cone_down[i]) DrawCircleV((Vector2){ MX(L->cone_x[i]), MY(L->cone_z[i]) }, 1.8f, (Color){ 249, 115, 22, 255 });
    const PkCar *c = &pk.car;
    DrawRectanglePro((Rectangle){ MX(c->x), MY(c->z), PK_CAR_W * sc + 1, PK_CAR_L * sc + 1 }, (Vector2){ (PK_CAR_W * sc + 1) / 2, (PK_CAR_L * sc + 1) / 2 }, -c->h * RAD2DEG, CAR_COLORS[0]);
    Vector2 nose = { MX(c->x + sinf(c->h) * 2.0f), MY(c->z + cosf(c->h) * 2.0f) };
    DrawCircleV(nose, 2.2f, WHITE);
#undef MX
#undef MY
}

static void pk_round_button(float cx, float cy, int kind, int *clicked)
{
    Rectangle r = { cx - 24, cy - 24, 48, 48 };
    int hover = CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
    DrawCircleV((Vector2){ cx, cy }, 24, hover ? COL_SURFACE3 : COL_SURFACE);
    DrawCircleLines((int)cx, (int)cy, 24, COL_BORDER);
    if (kind == 0) {   /* pauza */
        DrawRectangleRounded((Rectangle){ cx - 8, cy - 9, 5, 18 }, 1, 4, COL_TEXT);
        DrawRectangleRounded((Rectangle){ cx + 3, cy - 9, 5, 18 }, 1, 4, COL_TEXT);
    } else {           /* kamera */
        DrawRectangleRounded((Rectangle){ cx - 12, cy - 7, 24, 16 }, 0.3f, 6, COL_TEXT);
        DrawRectangle((int)cx - 5, (int)cy - 11, 10, 5, COL_TEXT);
        DrawCircleV((Vector2){ cx, cy + 1 }, 5, COL_SURFACE);
        DrawCircleV((Vector2){ cx, cy + 1 }, 2.5f, COL_TEXT);
    }
    *clicked = clicked_in(r);
}

static void fmt_time(char *out, int cap, float t)
{
    int ds = (int)(t * 10);
    snprintf(out, (size_t)cap, "%d:%02d.%d", ds / 600, (ds / 10) % 60, ds % 10);
}

static void pk_results(void)
{
    float a = fminf(1, (pk.end_timer - 0.6f) * 3);
    if (a <= 0) return;
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.6f * a));
    Rectangle P = { cx - 310, 110 + (1 - a) * 30, 620, 490 };
    card(P, 26, COL_SURFACE);
    char t[64];
    int won = pk.state == PK_WON;
    snprintf(t, sizeof(t), "%d-DARAJA  ·  %s", pk.level + 1, pk.L.name);
    tcenter(F_SEMI, t, cx, P.y + 40, 15, COL_SUBTLE);
    if (won) {
        tcenter(F_BOLD, pk.stars == 3 ? "Mukammal parkovka!" : pk.stars == 2 ? "Yaxshi parkovka!" : "Daraja o'tildi", cx, P.y + 84, 38, COL_TEXT);
        for (int k = 0; k < 3; k++) {   /* yulduzlar ketma-ket "otilib" chiqadi */
            float appear = pk.end_timer - 0.9f - k * 0.3f;
            float x = cx + (k - 1) * 120, y = P.y + 178 - (k == 1 ? 14 : 0);
            draw_star(x, y + 3, 46, Fade(BLACK, 0.35f));
            draw_star(x, y, 46, (Color){ 58, 66, 92, 255 });
            if (k < pk.stars && appear > 0) {
                float s = appear < 0.25f ? appear / 0.25f * 1.25f : appear < 0.4f ? 1.25f - (appear - 0.25f) / 0.15f * 0.25f : 1.0f;
                draw_star(x, y, 46 * s, (Color){ 250, 204, 21, 255 });
                draw_star(x - 8 * s, y - 10 * s, 10 * s, Fade(WHITE, 0.45f));
            }
        }
    } else {
        tcenter(F_BOLD, "Urilib ketdingiz!", cx, P.y + 84, 38, COL_TEXT);
        DrawCircleV((Vector2){ cx, P.y + 172 }, 52, Fade(COL_BAD, 0.18f));
        DrawCircleV((Vector2){ cx, P.y + 172 }, 38, COL_BAD);
        DrawLineEx((Vector2){ cx - 14, P.y + 158 }, (Vector2){ cx + 14, P.y + 186 }, 7, WHITE);
        DrawLineEx((Vector2){ cx + 14, P.y + 158 }, (Vector2){ cx - 14, P.y + 186 }, 7, WHITE);
    }
    /* ko'rsatkichlar */
    float bw = 170, by = P.y + 250;
    const char *lab[3] = { "VAQT", "URILISH", "3 YULDUZ" };
    char val[3][24];
    fmt_time(val[0], 24, pk.time);
    snprintf(val[1], 24, "%d / %d", pk.hits, PK_MAX_HITS);
    fmt_time(val[2], 24, pk.L.par);
    for (int k = 0; k < 3; k++) {
        Rectangle b = { cx - 1.5f * bw - 15 + k * (bw + 15), by, bw, 76 };
        DrawRectangleRounded(b, rnd(b, 14), 10, COL_SURFACE2);
        tcenter(F_SEMI, lab[k], b.x + bw / 2, b.y + 22, 13, COL_SUBTLE);
        Color vc = k == 1 ? (pk.hits ? COL_BAD : COL_GOOD) : k == 0 && won && pk.time <= pk.L.par ? COL_GOOD : COL_TEXT;
        tcenter(F_MONOB, val[k], b.x + bw / 2, b.y + 52, 24, vc);
    }
    if (won && pk_new_best) {
        float w = twidth(F_SEMI, "Yangi rekord!", 15) + 15 * 1.3f;
        chip(cx - w / 2, P.y + 350, "Yangi rekord!", Fade(COL_ACCENT, 0.18f), COL_ACCENT, 15);
    } else if (!won) {
        tcenter(F_BODY, "Bitta urilish ham daraja tugashiga olib keladi. Sekinroq yuring!", cx, P.y + 350, 15, COL_MUTED);
    }
    float y = P.y + P.height - 104;
    int last = pk.level >= PK_LEVELS - 1;
    if (won) {
        if (ui_button((Rectangle){ P.x + 40, y, 260, 58 }, last ? "Darajalar" : "Keyingi daraja", BTN_PRIMARY, 1) || IsKeyPressed(KEY_ENTER)) {
            if (last) park_open_select(); else park_start(pk.level + 1);
            return;
        }
        if (ui_button((Rectangle){ P.x + 316, y, 130, 58 }, "Qayta", BTN_SECONDARY, 1)) { park_start(pk.level); return; }
        if (ui_button((Rectangle){ P.x + 458, y, 122, 58 }, "Ro'yxat", BTN_GHOST, 1)) { park_open_select(); return; }
    } else {
        if (ui_button((Rectangle){ P.x + 40, y, 330, 58 }, "Qayta urinish", BTN_PRIMARY, 1) || IsKeyPressed(KEY_ENTER)) { park_start(pk.level); return; }
        if (ui_button((Rectangle){ P.x + 386, y, 194, 58 }, "Darajalar", BTN_SECONDARY, 1)) { park_open_select(); return; }
    }
}

static void pk_draw_ui(void)
{
    const PkLevel *L = &pk.L;
    char t[96];
    /* chap yuqori: daraja, vaqt, urilishlar */
    Rectangle C = { 16, 16, 320, 132 };
    card(C, 18, COL_SURFACE);
    DrawRectangleRounded((Rectangle){ C.x + 16, C.y + 16, 44, 44 }, 0.3f, 8, (Color){ 37, 99, 235, 255 });
    snprintf(t, sizeof(t), "%d", pk.level + 1);
    tcenter(F_BOLD, t, C.x + 38, C.y + 39, 24, WHITE);
    snprintf(t, sizeof(t), "%s  ·  %d-DARAJA", PK_GROUP_NAMES[L->theme & 3], pk.level + 1);
    tmid(F_SEMI, t, C.x + 72, C.y + 26, 12, COL_SUBTLE);
    tmid(F_BOLD, L->name, C.x + 72, C.y + 49, 21, COL_TEXT);
    fmt_time(t, sizeof(t), pk.time);
    tmid(F_MONOB, t, C.x + 18, C.y + 88, 26, pk.time <= L->par ? COL_TEXT : COL_ACCENT);
    draw_star(C.x + 138, C.y + 88, 9, pk.time <= L->par ? (Color){ 250, 204, 21, 255 } : (Color){ 80, 88, 110, 255 });
    fmt_time(t, sizeof(t), L->par);
    tmid(F_MONO, t, C.x + 152, C.y + 88, 15, COL_MUTED);
    tmid(F_SEMI, "Urilmang - bitta urinish", C.x + 42, C.y + 116, 13, COL_SUBTLE);
    for (int k = 0; k < PK_MAX_HITS; k++) {
        float x = C.x + 26 + k * 24;
        if (k < pk.hits) { DrawCircleV((Vector2){ x, C.y + 116 }, 8, COL_BAD); DrawLineEx((Vector2){ x - 3.5f, C.y + 112.5f }, (Vector2){ x + 3.5f, C.y + 119.5f }, 2, WHITE); DrawLineEx((Vector2){ x + 3.5f, C.y + 112.5f }, (Vector2){ x - 3.5f, C.y + 119.5f }, 2, WHITE); }
        else { DrawCircleV((Vector2){ x, C.y + 116 }, 8, Fade(COL_GOOD, 0.2f)); DrawRing((Vector2){ x, C.y + 116 }, 6.5f, 8, 0, 360, 20, COL_GOOD); }
    }
    /* uzatma va tezlik */
    const PkCar *c = &pk.car;
    const char *gear = c->speed > 0.2f ? "D" : c->speed < -0.2f ? "R" : "N";
    Color gc = c->speed > 0.2f ? COL_GOOD : c->speed < -0.2f ? COL_INFO : COL_MUTED;
    Rectangle G = { 16, IS_MOBILE ? 160 : VH - 112, 200, 92 };
    card(G, 18, COL_SURFACE);
    DrawRectangleRounded((Rectangle){ G.x + 14, G.y + 14, 64, 64 }, 0.3f, 8, Fade(gc, 0.18f));
    tcenter(F_BOLD, gear, G.x + 46, G.y + 47, 40, gc);
    snprintf(t, sizeof(t), "%d", (int)(fabsf(c->speed) * 3.6f + 0.5f));
    tmid(F_MONOB, t, G.x + 94, G.y + 40, 30, COL_TEXT);
    tmid(F_SEMI, "km/soat", G.x + 96, G.y + 66, 12, COL_SUBTLE);
    /* rul holati */
    float sx = G.x + 170, sy = G.y + 46;
    DrawRing((Vector2){ sx, sy }, 13, 17, 0, 360, 24, Fade(WHITE, 0.25f));
    float ang = c->steer * 90;
    DrawRing((Vector2){ sx, sy }, 13, 17, -90 + ang - 25, -90 + ang + 25, 12, COL_ACCENT);
    pk_minimap();
    int pc = 0, cc = 0;
    pk_round_button(vw - 40, 44, 0, &pc);
    pk_round_button(vw - 100, 44, 1, &cc);
    if (cc && pk.state == PK_PLAY) pk_cam_mode ^= 1;
    if (pc && pk.state == PK_PLAY) pk_paused = 1;
    if (!IS_MOBILE) {
        const char *hk = "W/S gaz-orqaga   A/D rul   PROBEL tormoz   C kamera   R qayta";
        float hw = twidth(F_BODY, hk, 14) + 28;
        DrawRectangleRounded((Rectangle){ vw - 16 - hw, VH - 44, hw, 30 }, 1, 10, Fade(COL_BG, 0.75f));
        tright(F_BODY, hk, vw - 30, VH - 29, 14, Fade(COL_TEXT, 0.8f));
    }

    /* yo'l-yo'riq paneli */
    if (pk.state == PK_PLAY) {
        float fit = pk_fit(L, c->x, c->z, c->h);
        const char *msg = NULL;
        Color mc = COL_INFO;
        float d0 = fabsf(fmodf(c->h - L->target.h + 3 * PI, 2 * PI) - PI);
        if (fit >= 0.999f) {
            msg = fabsf(c->speed) < 0.35f ? "Joyida! Biroz turing..." : "Joyida - to'xtang (PROBEL)";
            mc = COL_GOOD;
        } else if (fit > 0.2f) {
            if (L->need == NEED_REVERSE_IN && d0 < 0.6f) { msg = "Orqasi bilan kirish kerak!"; mc = COL_ACCENT; }
            else if (L->need == NEED_NOSE_IN && d0 > PI - 0.6f) { msg = "Oldi bilan kirish kerak!"; mc = COL_ACCENT; }
            else msg = "Mashinani chiziqlar ichiga tekislang";
        }
        if (msg) {
            float w = twidth(F_BOLD, msg, 20) + 56;
            Rectangle M = { vw / 2 - w / 2, VH - (IS_MOBILE ? 250 : 110), w, 60 };
            card(M, 18, COL_SURFACE);
            DrawRectangleRoundedLinesEx(M, rnd(M, 18), 12, 2, Fade(mc, 0.7f));
            tcenter(F_BOLD, msg, vw / 2, M.y + 26, 20, mc);
            Rectangle bar = { M.x + 20, M.y + 44, M.width - 40, 6 };
            DrawRectangleRounded(bar, 1, 6, Fade(WHITE, 0.1f));
            float k = fit >= 0.999f ? pk.hold / PK_HOLD_TIME : fit * 0.6f;
            DrawRectangleRounded((Rectangle){ bar.x, bar.y, bar.width * fminf(k, 1), bar.height }, 1, 6, mc);
        }
    }
    /* daraja boshidagi banner */
    if (pk_intro > 0 && pk.state == PK_PLAY) {
        float a = fminf(1, pk_intro * 1.5f) * fminf(1, (3.5f - pk_intro) * 4);
        float w = fmaxf(twidth(F_BOLD, L->name, 40), twidth(F_BODY, L->hint, 18)) + 80;
        Rectangle B = { vw / 2 - w / 2, 170, w, 128 };
        DrawRectangleRounded((Rectangle){ B.x, B.y + 6, B.width, B.height }, rnd(B, 22), 12, Fade(BLACK, 0.3f * a));
        DrawRectangleRounded(B, rnd(B, 22), 12, Fade((Color){ 18, 22, 35, 255 }, 0.94f * a));
        DrawRectangleRoundedLinesEx(B, rnd(B, 22), 12, 1.5f, Fade(COL_ACCENT, 0.5f * a));
        snprintf(t, sizeof(t), "%d-DARAJA", pk.level + 1);
        tcenter(F_SEMI, t, vw / 2, B.y + 28, 15, Fade(COL_ACCENT, a));
        tcenter(F_BOLD, L->name, vw / 2, B.y + 64, 40, Fade(WHITE, a));
        tcenter(F_BODY, L->hint, vw / 2, B.y + 102, 18, Fade(COL_MUTED, a));
    }
    /* xabarlar */
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (popups[i].t <= 0) continue;
        float a = fminf(1, popups[i].t * 2.5f);
        float fs = 22, w = twidth(F_BOLD, popups[i].text, fs) + 60, h = 48;
        Rectangle r = { vw / 2 - w / 2, 100 + i * 58, w, h };
        DrawRectangleRounded(r, 1, 12, Fade((Color){ 15, 20, 32, 255 }, 0.95f * a));
        DrawRectangleRoundedLinesEx(r, 1, 12, 2, Fade(popups[i].c, a));
        DrawCircleV((Vector2){ r.x + 26, r.y + h / 2 }, 7, Fade(popups[i].c, a));
        tmid(F_BOLD, popups[i].text, r.x + 42, r.y + h / 2, fs, Fade(WHITE, a));
    }
    if (pk.state != PK_PLAY) { pk_results(); return; }
    if (pk_paused) {
        DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.7f));
        Rectangle P = { vw / 2 - 230, 150, 460, 420 };
        card(P, 24, COL_SURFACE);
        tcenter(F_BOLD, "Pauza", vw / 2, P.y + 48, 30, COL_TEXT);
        snprintf(t, sizeof(t), "%d-daraja: %s", pk.level + 1, L->name);
        tcenter(F_BODY, t, vw / 2, P.y + 84, 16, COL_MUTED);
        float y = P.y + 116;
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Davom etish", BTN_PRIMARY, 1)) pk_paused = 0;
        y += 70;
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Qayta boshlash", BTN_SECONDARY, 1)) { park_start(pk.level); return; }
        y += 70;
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 56 }, "Darajalar ro'yxati", BTN_SECONDARY, 1)) { park_open_select(); return; }
        y += 70;
        if (ui_button((Rectangle){ P.x + 40, y, P.width - 80, 50 }, "Bosh menyu", BTN_GHOST, 1)) { screen = SCR_MENU; return; }
    }
}

/* ========================================================= daraja tanlash */
static void screen_park_select(void)
{
    float cx = vw / 2;
    DrawRectangle(0, 0, (int)vw + 2, VH, Fade(COL_BG, 0.62f));
    Rectangle P = { cx - 560, 24, 1120, 672 };
    card(P, 26, COL_SURFACE);
    if (ui_button((Rectangle){ P.x + 28, P.y + 26, 140, 48 }, "< Menyu", BTN_GHOST, 1) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACK)) { screen = SCR_MENU; return; }
    draw_p_badge(cx - 118, P.y + 50, 0.62f);
    tmid(F_BOLD, "Parkovka", cx - 84, P.y + 50, 38, COL_TEXT);
    /* yulduzlar soni */
    int total = park_total_stars();
    char t[48];
    snprintf(t, sizeof(t), "%d / %d", total, PK_LEVELS * 3);
    float tw = twidth(F_MONOB, t, 22);
    Rectangle S = { P.x + P.width - 28 - tw - 70, P.y + 26, tw + 70, 48 };
    DrawRectangleRounded(S, 1, 12, Fade((Color){ 250, 204, 21, 255 }, 0.12f));
    draw_star(S.x + 28, S.y + 24, 14, (Color){ 250, 204, 21, 255 });
    tmid(F_MONOB, t, S.x + 50, S.y + 25, 22, (Color){ 250, 204, 21, 255 });
    /* umumiy progress chizig'i */
    Rectangle bar = { P.x + 28, P.y + 92, P.width - 56, 6 };
    DrawRectangleRounded(bar, 1, 6, Fade(WHITE, 0.07f));
    DrawRectangleRounded((Rectangle){ bar.x, bar.y, bar.width * total / (PK_LEVELS * 3.0f), bar.height }, 1, 6, (Color){ 250, 204, 21, 255 });

    float lx = P.x + 28, gx = lx + 176, gw = P.width - 56 - 176;
    float tile_w = (gw - 4 * 14) / 5, tile_h = 118;
    int first_open = -1;
    for (int k = 0; k < PK_LEVELS; k++) if (park_unlocked(k) && pk_stars[k] == 0) { first_open = k; break; }
    for (int g = 0; g < 4; g++) {
        const Theme *th = &THEMES[g];
        float y = P.y + 118 + g * (tile_h + 16);
        /* guruh nomi va mavzu rangi */
        Rectangle art = { lx, y, 160, tile_h };
        DrawRectangleRounded(art, rnd(art, 16), 10, th->ground);
        DrawRectangleGradientV((int)art.x + 6, (int)art.y + 4, (int)art.width - 12, (int)(art.height * 0.55f), th->sky_top, th->sky_hor);
        DrawRectangleRounded((Rectangle){ art.x, art.y + art.height - 44, art.width, 44 }, 0.4f, 8, Fade(COL_BG, 0.75f));
        tmid(F_BOLD, PK_GROUP_NAMES[g], art.x + 14, art.y + art.height - 22, 17, COL_TEXT);
        int gs = 0;
        for (int k = 0; k < 5; k++) gs += pk_stars[g * 5 + k];
        snprintf(t, sizeof(t), "%d/15", gs);
        tright(F_MONOB, t, art.x + art.width - 12, art.y + 20, 14, WHITE);
        draw_star(art.x + art.width - 58, art.y + 20, 7, (Color){ 250, 204, 21, 255 });
        for (int k = 0; k < 5; k++) {
            int lv = g * 5 + k;
            int open = park_unlocked(lv);
            Rectangle r = { gx + k * (tile_w + 14), y, tile_w, tile_h };
            int hover = open && CheckCollisionPointRec(vmouse(), r) && !IS_MOBILE;
            if (hover) r.y -= 3;
            DrawRectangleRounded((Rectangle){ r.x, r.y + 4, r.width, r.height }, rnd(r, 16), 10, Fade(BLACK, 0.25f));
            DrawRectangleRounded(r, rnd(r, 16), 10, open ? (hover ? COL_SURFACE3 : COL_SURFACE2) : (Color){ 22, 26, 40, 255 });
            if (open) DrawRectangleRounded((Rectangle){ r.x + 14, r.y, r.width - 28, 4 }, 1, 6, th->accent);
            if (lv == first_open) {
                float p = 0.5f + 0.5f * sinf((float)GetTime() * 4);
                DrawRectangleRoundedLinesEx(r, rnd(r, 16), 10, 2.5f, Fade(COL_ACCENT, 0.5f + 0.5f * p));
            } else DrawRectangleRoundedLinesEx(r, rnd(r, 16), 10, 1.2f, hover ? Fade(th->accent, 0.8f) : COL_BORDER);
            if (open) {
                snprintf(t, sizeof(t), "%d", lv + 1);
                tcenter(F_BOLD, t, r.x + r.width / 2, r.y + 36, 34, COL_TEXT);
                PkLevel tmp;
                pk_build(lv, &tmp);
                tcenter(F_SEMI, tmp.name, r.x + r.width / 2, r.y + 68, 13, COL_MUTED);
                draw_stars_row(r.x + r.width / 2, r.y + 96, 10, pk_stars[lv], 26);
                if (clicked_in(r)) { play(SND_CLICK, 0.5f); park_start(lv); return; }
            } else {
                draw_lock(r.x + r.width / 2, r.y + 50, 1.3f, (Color){ 70, 80, 104, 255 });
                snprintf(t, sizeof(t), "%d", lv + 1);
                tcenter(F_SEMI, t, r.x + r.width / 2, r.y + 94, 16, (Color){ 70, 80, 104, 255 });
            }
        }
    }
    tcenter(F_BODY, "Har bir daraja oldingisidan keyin ochiladi.  Bitta urilish - daraja qaytadan.  3 yulduz: belgilangan vaqt ichida.",
            cx, P.y + P.height - 22, 14, COL_SUBTLE);
    if (IsKeyPressed(KEY_ENTER) && first_open >= 0) park_start(first_open);
}

/* =================================================================== */
/*                                MAIN                                 */
/* =================================================================== */
int main(void)
{
    srand((unsigned)(plat_time() * 1000));
    plat_net_init();
    build_previews();
    pk_load_save();
    track_init(rand() % MAP_COUNT);

#if IS_MOBILE
    SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(0, 0, "TURBO JANG");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "TURBO JANG");
    SetWindowMinSize(800, 450);
#endif
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    load_fonts();

    InitAudioDevice();
    audio_ok = IsAudioDeviceReady();
    if (audio_ok) for (int i = 0; i < SND_COUNT; i++) sounds[i] = make_sound(i);

    game_init(&demo);
    float time = 0;

    while (!WindowShouldClose() && !quit) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;
        time += dt;
        mouse_consumed = 0;

        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        scale = sh / VH;
        vw = sw / scale;
        if (vw < 1280) { vw = 1280; scale = sw / 1280.0f; }
        off_x = 0;
        off_y = (sh - VH * scale) * 0.5f;

        TH = &THEMES[TRACK.map_id < MAP_COUNT ? TRACK.map_id : 0];
        if (screen == SCR_PARK) TH = &THEMES[pk.L.theme & 3];
        update_parts(dt);
        for (int i = 0; i < MAX_RINGS; i++) if (rings[i].t > 0) rings[i].t -= dt;
        for (int i = 0; i < MAX_POPUPS; i++) if (popups[i].t > 0) popups[i].t -= dt;
        if (shake > 0) shake -= dt;
        if (message_timer > 0) message_timer -= dt;

        if (screen == SCR_GAME) screen_game_update(dt);
        if (screen == SCR_PARK) pk_update(dt);
        int in_game = screen == SCR_GAME && have_view && view.phase != PHASE_LOBBY;
        int in_park = screen == SCR_PARK;
        if (!in_game && !in_park) demo_update(dt);

        /* dvigatel ovozi */
        if (audio_ok) {
            if (in_game && view.phase != PHASE_OVER && !menu_open) {
                float sp = fabsf(view.players[my_slot].speed) / MAX_SPEED;
                if (!IsSoundPlaying(sounds[SND_ENGINE])) PlaySound(sounds[SND_ENGINE]);
                SetSoundPitch(sounds[SND_ENGINE], 0.55f + sp * 1.1f);
                SetSoundVolume(sounds[SND_ENGINE], 0.18f + sp * 0.2f);
            } else if (in_park && !pk_paused) {
                float sp = fabsf(pk.car.speed) / PK_MAX_FWD;
                if (!IsSoundPlaying(sounds[SND_ENGINE])) PlaySound(sounds[SND_ENGINE]);
                SetSoundPitch(sounds[SND_ENGINE], 0.5f + sp * 0.7f);
                SetSoundVolume(sounds[SND_ENGINE], 0.12f + sp * 0.15f);
            } else if (IsSoundPlaying(sounds[SND_ENGINE])) StopSound(sounds[SND_ENGINE]);
        }

        TH = &THEMES[TRACK.map_id < MAP_COUNT ? TRACK.map_id : 0];
        if (screen == SCR_PARK) TH = &THEMES[pk.L.theme & 3];
        BeginDrawing();
        ClearBackground(TH->sky_hor);
        if (screen == SCR_PARK) {
            pk_draw_scene(time, dt);
            if (pk.state == PK_PLAY && !pk_paused) pk_draw_touch(pk_steer_in, IsKeyDown(KEY_SPACE));
        } else if (in_game) {
            float since = (float)(GetTime() - view_time);
            if (since > 0.1f) since = 0.1f;
            spawn_effects(&view, since);
            draw_scene(&view, my_slot, since, time, dt);
            if (view.phase == PHASE_PLAY && !menu_open && !view.players[my_slot].place) draw_touch_controls(&current_input);
        } else {
            spawn_effects(&demo, 0);
            draw_scene(&demo, -1, 0, time, dt);
        }
        Camera2D ui = { 0 };
        ui.offset = (Vector2){ off_x, off_y };
        ui.zoom = scale;
        BeginMode2D(ui);
        switch (screen) {
        case SCR_MENU:       screen_menu(); break;
        case SCR_JOIN:       screen_join(); break;
        case SCR_CONNECTING: screen_connecting(); break;
        case SCR_GAME:       draw_game_ui(); break;
        case SCR_PARK_SELECT: screen_park_select(); break;
        case SCR_PARK:       pk_draw_ui(); break;
        }
        if (message_timer > 0 && message[0]) {
            float a = fminf(1, message_timer);
            float w = twidth(F_SEMI, message, 18) + 48;
            Rectangle r = { vw / 2 - w / 2, VH - 120, w, 48 };
            DrawRectangleRounded(r, 1, 12, Fade((Color){ 127, 29, 29, 255 }, 0.95f * a));
            DrawRectangleRoundedLinesEx(r, 1, 12, 1.5f, Fade(COL_BAD, a));
            tcenter(F_SEMI, message, vw / 2, r.y + 24, 18, Fade(WHITE, a));
        }
        EndMode2D();
        EndDrawing();
    }

    leave_game(NULL);
    if (disc_running) discovery_stop(&disc);
    for (int i = 0; i < F_COUNT; i++) UnloadFont(fonts[i]);
    if (audio_ok) {
        for (int i = 0; i < SND_COUNT; i++) UnloadSound(sounds[i]);
        CloseAudioDevice();
    }
    CloseWindow();
    plat_net_shutdown();
    return 0;
}
