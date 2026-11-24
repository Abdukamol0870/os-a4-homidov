/*
 * game.c - TURBO JANG o'yin mantig'i: mashina fizikasi, trassadagi joy,
 * aylanalar, sovg'a qutilari, qurollar (snaryadlar, minalar, zarba
 * to'lqini), to'qnashuvlar, o'rinlar, botlar va ketma-ketlashtirish.
 */
#include "game.h"

#include <math.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static const char *COLOR_NAMES[MAX_PLAYERS] = { "Qizil", "Moviy", "Sariq", "Yashil" };
static const char *ITEM_NAMES[ITEM_COUNT] = { "", "Nitro", "Raketa", "Chaqmoq", "Mina", "Zarba", "Qalqon" };

const char *game_color_name(int slot) { return (slot >= 0 && slot < MAX_PLAYERS) ? COLOR_NAMES[slot] : "?"; }
const char *game_item_name(int item) { return (item > 0 && item < ITEM_COUNT) ? ITEM_NAMES[item] : ""; }

static float angdiff(float a, float b)
{
    float d = fmodf(b - a + PI, 2 * PI);
    if (d < 0) d += 2 * PI;
    return d - PI;
}
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static uint32_t rnext(uint32_t *r) { *r = *r * 1664525u + 1013904223u; return *r >> 8; }
static float rfloat(uint32_t *r) { return (float)(rnext(r) & 0xFFFF) / 65535.0f; }

/* ------------------------------------------------------- Boshqaruv */
void game_init(GameState *g)
{
    if (TRACK.n == 0) track_init(0);
    memset(g, 0, sizeof(*g));
    g->map_id = (uint8_t)TRACK.map_id;
    g->phase = PHASE_LOBBY;
    g->laps = 3;
    g->rng = 4242u;
}

static int random_item(GameState *g);
static void place_on_grid(Player *p, int slot)
{
    static const float LAT[MAX_PLAYERS] = { -2.8f, 2.8f, -8.0f, 8.0f };
    float s = TRACK.length - 9.0f - slot * 6.0f;
    V2 q = track_point_at(s, LAT[slot]);
    p->x = q.x;
    p->z = q.z;
    p->idx = track_nearest(p->x, p->z, -1, &p->lat, &p->s);
    p->heading = track_heading_at(p->idx);
    p->speed = 0;
    p->lap = -1;
    p->progress = p->s - TRACK.length;
}

static void reset_player(Player *p, int slot)
{
    uint8_t bot = p->is_bot, seq = p->last_fire_seq;
    uint8_t fires = p->fires, hits = p->hits, pick = p->pickups, pops = p->pops;
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->is_bot = bot;
    p->last_fire_seq = seq;
    p->fires = fires; p->hits = hits; p->pickups = pick; p->pops = pops;
    place_on_grid(p, slot);
}

void game_add_player(GameState *g, int slot, int is_bot)
{
    if (slot < 0 || slot >= MAX_PLAYERS) return;
    Player *p = &g->players[slot];
    memset(p, 0, sizeof(*p));
    p->active = 1;
    p->is_bot = (uint8_t)(is_bot ? 1 : 0);
    place_on_grid(p, slot);
    if (g->phase == PHASE_PLAY) {   /* o'yin o'rtasida - eng orqadagi mashina yonidan */
        int order[MAX_PLAYERS];
        game_ranking(g, order);
        for (int k = MAX_PLAYERS - 1; k >= 0; k--) {
            const Player *o = &g->players[order[k]];
            if (order[k] == slot || !o->active || o->place) continue;
            V2 q = track_point_at(o->s - 10.0f, 0);
            p->x = q.x; p->z = q.z;
            p->idx = track_nearest(p->x, p->z, -1, &p->lat, &p->s);
            p->heading = track_heading_at(p->idx);
            p->lap = o->lap;
            if (p->s > o->s + TRACK.length / 2) p->lap--;
            p->progress = p->lap * TRACK.length + p->s;
            p->shield = 3.0f;
            break;
        }
    }
}

void game_remove_player(GameState *g, int slot)
{
    if (slot < 0 || slot >= MAX_PLAYERS) return;
    memset(&g->players[slot], 0, sizeof(Player));
}

void game_start_match(GameState *g)
{
    g->phase = PHASE_COUNTDOWN;
    g->countdown = COUNTDOWN_TIME;
    g->elapsed = 0;
    g->finished = 0;
    g->over_timer = 0;
    g->rng ^= g->tick * 2654435761u + 0x9E3779B9u;
    if (TRACK.map_id != g->map_id || TRACK.n == 0) track_init(g->map_id);
    memset(g->proj, 0, sizeof(g->proj));
    memset(g->mines, 0, sizeof(g->mines));
    game_place_pickups(g);
    for (int k = 0; k < PICKUPS; k++) {
        g->pickups[k].active = 1;
        g->pickups[k].timer = 0;
        g->pickups[k].type = (uint8_t)random_item(g);
    }
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (g->players[i].active) reset_player(&g->players[i], i);
}

void game_ranking(const GameState *g, int order[MAX_PLAYERS])
{
    for (int i = 0; i < MAX_PLAYERS; i++) order[i] = i;
    for (int i = 0; i < MAX_PLAYERS; i++)
        for (int j = i + 1; j < MAX_PLAYERS; j++) {
            const Player *a = &g->players[order[i]], *b = &g->players[order[j]];
            int swap;
            if (a->active != b->active) swap = b->active;
            else if (a->place && b->place) swap = b->place < a->place;
            else if (a->place || b->place) swap = b->place != 0;
            else swap = b->progress > a->progress;
            if (swap) { int t = order[i]; order[i] = order[j]; order[j] = t; }
        }
}


/* ----------------------------------------------------------- Qurollar */
static void hit_player(GameState *g, int v, float stun)
{
    Player *p = &g->players[v];
    if (!p->active || p->place) return;
    if (p->shield > 0) { p->shield = 0; p->pops++; return; }
    if (p->stun > 0) return;
    p->stun = stun;
    p->speed *= 0.25f;
    p->hits++;
    (void)g;
}

/* qutiga tasodifiy qurol: nitro, raketa, chaqmoq, mina, zarba, qalqon */
static int random_item(GameState *g)
{
    static const int W[6] = { 20, 16, 20, 15, 14, 15 };
    int total = 0;
    for (int k = 0; k < 6; k++) total += W[k];
    int r = (int)(rnext(&g->rng) % (uint32_t)total);
    for (int k = 0; k < 6; k++) { if (r < W[k]) return k + 1; r -= W[k]; }
    return ITEM_NITRO;
}

void game_place_pickups(GameState *g)
{
    for (int r = 0; r < PICKUP_ROWS; r++)
        for (int k = 0; k < 4; k++) {
            V2 q = track_point_at(TRACK.length * (r + 0.45f) / PICKUP_ROWS, -7.5f + k * 5.0f);
            g->pickups[r * 4 + k].x = q.x;
            g->pickups[r * 4 + k].z = q.z;
        }
}

void game_set_map(GameState *g, int map)
{
    track_init(map);
    g->map_id = (uint8_t)TRACK.map_id;
    game_place_pickups(g);
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (g->players[i].active) place_on_grid(&g->players[i], i);
}

static void spawn_proj(GameState *g, int type, int owner, int target, float speed, float life, int backward)
{
    Player *p = &g->players[owner];
    float h = backward ? p->heading + PI : p->heading;
    for (int k = 0; k < MAX_PROJ; k++) {
        Projectile *pr = &g->proj[k];
        if (pr->active) continue;
        pr->active = 1;
        pr->type = (uint8_t)type;
        pr->owner = (uint8_t)owner;
        pr->target = (uint8_t)target;
        pr->x = p->x + sinf(h) * 3.2f;
        pr->z = p->z + cosf(h) * 3.2f;
        pr->heading = h;
        pr->speed = speed + (backward ? 0 : (p->speed > 0 ? p->speed : 0));
        pr->life = life;
        return;
    }
}

/* Rejimlar (Blur'dagidek):
 *   oldinga  - raketa oldingi mashinani, chaqmoq oldinga, mina oldinga otiladi, nitro tezlatadi
 *   orqaga   - raketa orqadagi mashinani quvadi, chaqmoq orqaga, mina orqaga tashlanadi,
 *              nitro esa keskin tormoz bo'ladi (burilishga kirishda qulay)
 *   tashlash - qurolni tashlab yuborib, boshqa qutini olishga joy ochish */
static void use_item(GameState *g, int i, int mode)
{
    Player *p = &g->players[i];
    int item = p->item, back = mode == FIRE_BACKWARD;
    if (mode == FIRE_DISCARD) {
        p->item = ITEM_NONE;
        p->charges = 0;
        p->fires++;
        p->last_used = 0;
        return;
    }
    switch (item) {
    case ITEM_NITRO:
        if (back) p->speed *= 0.45f;   /* nitro-tormoz */
        else p->nitro = 3.0f;
        p->item = ITEM_NONE;
        break;
    case ITEM_SHUNT: {
        /* nishon: oldinga - aynan oldingi mashina, orqaga - aynan orqadagi */
        int order[MAX_PLAYERS], me = 0, target = 255;
        game_ranking(g, order);
        for (int k = 0; k < MAX_PLAYERS; k++) if (order[k] == i) me = k;
        if (back) { for (int k = me + 1; k < MAX_PLAYERS; k++) if (g->players[order[k]].active && !g->players[order[k]].place) { target = order[k]; break; } }
        else      { for (int k = me - 1; k >= 0; k--) if (g->players[order[k]].active && !g->players[order[k]].place) { target = order[k]; break; } }
        spawn_proj(g, ITEM_SHUNT, i, target, back ? 50.0f : 62.0f, 7.0f, back);
        p->item = ITEM_NONE;
        break;
    }
    case ITEM_BOLT:
        spawn_proj(g, ITEM_BOLT, i, 255, 110.0f, 1.1f, back);
        if (p->charges > 0) p->charges--;
        if (p->charges == 0) p->item = ITEM_NONE;
        break;
    case ITEM_MINE:
        for (int k = 0; k < MAX_MINES; k++) {
            Mine *m = &g->mines[k];
            if (m->active) continue;
            m->active = 1;
            m->owner = (uint8_t)i;
            if (back) {
                m->x = p->x - sinf(p->heading) * 4.0f;
                m->z = p->z - cosf(p->heading) * 4.0f;
                m->arm = 0.6f;
            } else {   /* oldinga otilgan mina 30 m oldinga, yo'lak bo'ylab tushadi */
                V2 q = track_point_at(p->s + 30.0f + fmaxf(p->speed, 0) * 0.3f, p->lat);
                m->x = q.x;
                m->z = q.z;
                m->arm = 0.25f;
            }
            m->life = 45.0f;
            break;
        }
        p->item = ITEM_NONE;
        break;
    case ITEM_BARGE:
        for (int j = 0; j < MAX_PLAYERS; j++) {
            Player *o = &g->players[j];
            if (j == i || !o->active) continue;
            float dx = o->x - p->x, dz = o->z - p->z, d = sqrtf(dx * dx + dz * dz);
            if (d > 15.0f || d < 0.01f) continue;
            hit_player(g, j, 1.0f);
            float push = 4.0f * (1.0f - d / 15.0f) + 1.0f;
            o->x += dx / d * push;
            o->z += dz / d * push;
        }
        p->item = ITEM_NONE;
        break;
    case ITEM_SHIELD:
        p->shield = 8.0f;
        p->item = ITEM_NONE;
        break;
    default:
        return;
    }
    p->fires++;
    p->last_used = (uint8_t)item;
}

/* ------------------------------------------------------------- Fizika */
static void update_car(GameState *g, int i, const Input *in, float dt)
{
    Player *p = &g->players[i];
    float steer = 0, thr = 0;
    if (p->stun > 0) {
        p->stun -= dt;
        p->speed -= (p->speed > 0 ? 1 : -1) * 28.0f * dt;
        if (fabsf(p->speed) < 1.0f) p->speed = 0;
    } else {
        steer = clampf(in->steer / 127.0f, -1, 1);
        thr = clampf(in->throttle / 127.0f, -1, 1);
    }
    if (p->nitro > 0) { p->nitro -= dt; if (thr < 0.6f && p->stun <= 0) thr = 1.0f; }
    if (p->shield > 0) p->shield -= dt;

    float top = p->nitro > 0 ? NITRO_SPEED : MAX_SPEED;
    int offroad = fabsf(p->lat) > TRACK_W / 2 + 0.6f;
    if (offroad) top = fminf(top, OFFROAD_SPEED);

    if (thr > 0.01f) {
        if (p->speed < 0) p->speed += BRAKE * thr * dt;
        else if (p->speed < top * thr) p->speed += ACCEL * thr * (p->nitro > 0 ? 2.2f : 1.0f) * (1.0f - 0.45f * p->speed / top) * dt;
    } else if (thr < -0.01f) {
        if (p->speed > 0) p->speed -= BRAKE * -thr * dt;
        else if (p->speed > -10.0f) p->speed -= 9.0f * -thr * dt;
    } else {
        float c = 5.0f * dt;
        p->speed = fabsf(p->speed) < c ? 0 : p->speed - (p->speed > 0 ? c : -c);
    }
    if (p->speed > top) p->speed -= (offroad ? 30.0f : 12.0f) * dt;

    /* tezlik oshgani sari rul sekinroq buriladi - burilishlarda sekinlash kerak */
    float v = fabsf(p->speed);
    float rate = 1.6f - 0.95f * clampf(v / MAX_SPEED, 0, 1.2f);
    float grip = clampf(v / 7.0f, 0, 1);
    /* heading oshsa mashina chapga buriladi (kamera ortidan qaraganda +x chapda),
     * shuning uchun o'ngga rul (steer > 0) heading'ni kamaytiradi */
    p->heading -= steer * rate * grip * dt * (p->speed >= 0 ? 1.0f : -1.0f);
    if (p->stun > 0) p->heading += 0;   /* aylanish faqat ko'rinishda */

    p->x += sinf(p->heading) * p->speed * dt;
    p->z += cosf(p->heading) * p->speed * dt;

    float prev_s = p->s;
    p->idx = track_nearest(p->x, p->z, p->idx, &p->lat, &p->s);

    /* trassa chetidagi to'siq */
    float limit = TRACK_W / 2 + 6.0f;
    if (fabsf(p->lat) > limit) {
        float corr = (p->lat > 0 ? limit : -limit) - p->lat;
        p->x += TRACK.nrm[p->idx].x * corr;
        p->z += TRACK.nrm[p->idx].z * corr;
        p->lat += corr;
        p->speed *= 1.0f - 2.5f * dt;
    }

    /* aylanalarni sanash */
    float L = TRACK.length;
    if (prev_s > L * 0.75f && p->s < L * 0.25f) p->lap++;
    else if (prev_s < L * 0.25f && p->s > L * 0.75f) p->lap--;
    p->progress = p->lap * L + p->s;

    float dir_dot = sinf(p->heading) * TRACK.t[p->idx].x + cosf(p->heading) * TRACK.t[p->idx].z;
    p->wrong_way = (uint8_t)(dir_dot < -0.3f && p->speed > 3.0f && !p->place);

    if (!p->place && g->phase == PHASE_PLAY) {
        if (p->lap >= g->laps) {
            p->place = ++g->finished;
            p->finish_time = g->elapsed;
            p->item = ITEM_NONE;
            if (g->finished == 1) g->over_timer = FINISH_WAIT;
        }
        /* qurol ishlatish (har bosish bir marta) */
        if (in->fire_seq != p->last_fire_seq) {
            p->last_fire_seq = in->fire_seq;
            if (p->item && p->stun <= 0) use_item(g, i, in->fire_mode);
        }
    } else {
        p->last_fire_seq = in->fire_seq;
    }
}

static void update_world(GameState *g, float dt)
{
    /* sovg'a qutilari */
    for (int k = 0; k < PICKUPS; k++) {
        Pickup *pk = &g->pickups[k];
        if (!pk->active) { pk->timer -= dt; if (pk->timer <= 0) pk->active = 1; continue; }
        for (int i = 0; i < MAX_PLAYERS; i++) {
            Player *p = &g->players[i];
            if (!p->active || p->place || p->item) continue;
            float dx = p->x - pk->x, dz = p->z - pk->z;
            if (dx * dx + dz * dz > 3.0f * 3.0f) continue;
            p->item = pk->type;
            p->charges = p->item == ITEM_BOLT ? 3 : 1;
            p->pickups++;
            pk->active = 0;
            pk->timer = 4.0f;
            pk->type = (uint8_t)random_item(g);   /* qayta paydo bo'lganda boshqa qurol */
            break;
        }
    }
    /* snaryadlar */
    for (int k = 0; k < MAX_PROJ; k++) {
        Projectile *pr = &g->proj[k];
        if (!pr->active) continue;
        pr->life -= dt;
        if (pr->life <= 0) { pr->active = 0; continue; }
        if (pr->type == ITEM_SHUNT) {
            float desired;
            if (pr->target < MAX_PLAYERS && g->players[pr->target].active && !g->players[pr->target].place) {
                const Player *t = &g->players[pr->target];
                desired = atan2f(t->x - pr->x, t->z - pr->z);
            } else {   /* nishonsiz raketa trassa bo'ylab uchadi */
                float lat, s;
                track_nearest(pr->x, pr->z, -1, &lat, &s);
                V2 q = track_point_at(s + 25.0f, 0);
                desired = atan2f(q.x - pr->x, q.z - pr->z);
            }
            float d = angdiff(pr->heading, desired), mx = 3.2f * dt;
            pr->heading += clampf(d, -mx, mx);
        }
        pr->x += sinf(pr->heading) * pr->speed * dt;
        pr->z += cosf(pr->heading) * pr->speed * dt;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            Player *p = &g->players[i];
            if (!p->active || i == pr->owner || p->place) continue;
            float dx = p->x - pr->x, dz = p->z - pr->z;
            if (dx * dx + dz * dz > (CAR_RADIUS + 1.3f) * (CAR_RADIUS + 1.3f)) continue;
            hit_player(g, i, pr->type == ITEM_SHUNT ? 1.7f : 0.9f);
            pr->active = 0;
            break;
        }
    }
    /* minalar */
    for (int k = 0; k < MAX_MINES; k++) {
        Mine *m = &g->mines[k];
        if (!m->active) continue;
        m->life -= dt;
        if (m->arm > 0) m->arm -= dt;
        if (m->life <= 0) { m->active = 0; continue; }
        if (m->arm > 0) continue;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            Player *p = &g->players[i];
            if (!p->active || p->place) continue;
            float dx = p->x - m->x, dz = p->z - m->z;
            if (dx * dx + dz * dz > 2.6f * 2.6f) continue;
            hit_player(g, i, 1.5f);
            m->active = 0;
            break;
        }
    }
    /* mashinalar to'qnashuvi */
    for (int a = 0; a < MAX_PLAYERS; a++)
        for (int b = a + 1; b < MAX_PLAYERS; b++) {
            Player *p = &g->players[a], *q = &g->players[b];
            if (!p->active || !q->active) continue;
            float dx = q->x - p->x, dz = q->z - p->z, d2 = dx * dx + dz * dz, r = CAR_RADIUS * 2;
            if (d2 >= r * r || d2 < 1e-6f) continue;
            float d = sqrtf(d2), push = (r - d) / 2;
            dx /= d; dz /= d;
            p->x -= dx * push; p->z -= dz * push;
            q->x += dx * push; q->z += dz * push;
            p->speed *= 0.985f;
            q->speed *= 0.985f;
        }
}

/* Avtopilot: botlar va marraga yetganlar uchun.
 * Oldindagi nuqtaga qarab rul buradi, burilish keskinligiga qarab sekinlaydi. */
static void autopilot(const GameState *g, int i, float lane, float skill, Input *out)
{
    const Player *p = &g->players[i];
    int n = TRACK.n;
    float look = 9.0f + fabsf(p->speed) * 0.42f;
    int ti = (p->idx + (int)(look / TRACK.spacing)) % n;
    float tx = TRACK.p[ti].x + TRACK.nrm[ti].x * lane, tz = TRACK.p[ti].z + TRACK.nrm[ti].z * lane;
    float desired = atan2f(tx - p->x, tz - p->z);
    float diff = angdiff(p->heading, desired);
    out->steer = (int8_t)(clampf(-diff * 2.6f, -1, 1) * 127);

    /* oldindagi eng keskin burilish */
    float tmax = 0;
    int ahead = (int)((fabsf(p->speed) * 1.4f + 15.0f) / TRACK.spacing);
    for (int k = 0; k < ahead; k++) { float t = fabsf(TRACK.turn[(p->idx + k) % n]); if (t > tmax) tmax = t; }
    float c = tmax / 10.0f;   /* rad/m */
    float vmax = (1.6f * 0.82f) / (c + 0.95f * 0.82f / MAX_SPEED);
    float target = fminf(MAX_SPEED, vmax) * skill;
    float thr;
    if (fabsf(diff) > 1.6f) thr = 0.6f;            /* teskari turib qolgan */
    else if (p->speed < target - 1.0f) thr = 1.0f;
    else if (p->speed > target + 3.0f) thr = -0.7f;
    else thr = 0.25f;
    out->throttle = (int8_t)(thr * 127);
}

void game_step(GameState *g, const Input inputs[MAX_PLAYERS], float dt)
{
    g->tick++;
    if (g->phase == PHASE_LOBBY || g->phase == PHASE_OVER) return;
    if (g->phase == PHASE_COUNTDOWN) {
        g->countdown -= dt;
        if (g->countdown <= 0) { g->countdown = 0; g->phase = PHASE_PLAY; }
        for (int i = 0; i < MAX_PLAYERS; i++) g->players[i].last_fire_seq = inputs[i].fire_seq;
        return;
    }
    g->elapsed += dt;
    int active = 0, done = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &g->players[i];
        if (!p->active) continue;
        Input in = inputs[i];
        if (p->place) {   /* marradan keyin avtopilot sekin haydaydi */
            Input ap = { 0, 0, in.fire_seq, FIRE_FORWARD };
            autopilot(g, i, 0, 0.55f, &ap);
            in = ap;
        }
        update_car(g, i, &in, dt);
        active++;
        done += p->place != 0;
    }
    update_world(g, dt);
    if (g->finished > 0) g->over_timer -= dt;
    if (active > 0 && (done == active || (g->finished > 0 && g->over_timer <= 0))) {
        int order[MAX_PLAYERS];
        game_ranking(g, order);
        for (int k = 0; k < MAX_PLAYERS; k++) {
            Player *p = &g->players[order[k]];
            if (p->active && !p->place) p->place = ++g->finished;
        }
        g->phase = PHASE_OVER;
    }
}

/* ------------------------------------------------------------- Botlar */
void bot_think(const GameState *g, int slot, BotBrain *b, Input *out, float dt)
{
    const Player *p = &g->players[slot];
    out->steer = 0; out->throttle = 0; out->fire_seq = b->seq; out->fire_mode = FIRE_FORWARD;
    if (g->phase != PHASE_PLAY || p->place) return;
    if (b->rng == 0) { b->rng = 777u + (uint32_t)slot * 7919u; b->lane = -5 + slot * 3.0f; }
    int hard = g->bots_enabled == BOTS_HARD;

    /* yo'lak tanlash: qutilarga qarab, minalardan qochib */
    b->lane_timer -= dt;
    if (b->lane_timer <= 0) { b->lane = -6.5f + 13.0f * rfloat(&b->rng); b->lane_timer = 2.5f + 3.0f * rfloat(&b->rng); }
    float lane = b->lane;
    float fx = sinf(p->heading), fz = cosf(p->heading);
    if (!p->item) {
        float best = 45.0f;
        for (int k = 0; k < PICKUPS; k++) {
            const Pickup *pk = &g->pickups[k];
            if (!pk->active) continue;
            float dx = pk->x - p->x, dz = pk->z - p->z, ahead = dx * fx + dz * fz;
            if (ahead < 5.0f || ahead > best) continue;
            best = ahead;
            float l, s;
            track_nearest(pk->x, pk->z, p->idx, &l, &s);
            lane = l;
        }
    }
    for (int k = 0; k < MAX_MINES; k++) {
        const Mine *m = &g->mines[k];
        if (!m->active) continue;
        float dx = m->x - p->x, dz = m->z - p->z, ahead = dx * fx + dz * fz;
        if (ahead < 0 || ahead > 35.0f) continue;
        float l, s;
        track_nearest(m->x, m->z, p->idx, &l, &s);
        if (fabsf(l - lane) < 3.5f) lane = l + (l > 0 ? -5.0f : 5.0f);
    }
    autopilot(g, slot, clampf(lane, -8.5f, 8.5f), hard ? 0.98f : 0.88f, out);

    /* qurol ishlatish */
    if (!p->item) { b->item_timer = 0; return; }
    b->item_timer += dt;
    float delay = hard ? 0.6f : 1.8f;
    if (b->item_timer < delay) return;
    int fire = 0, mode = FIRE_FORWARD;
    float turn = fabsf(TRACK.turn[(p->idx + 8) % TRACK.n]);
    switch (p->item) {
    case ITEM_NITRO:  fire = turn < 0.05f || b->item_timer > 6; break;
    case ITEM_SHUNT: {
        /* birinchi bo'lsa - raketani orqadagiga otadi */
        int order[MAX_PLAYERS];
        game_ranking(g, order);
        fire = 1;
        if (order[0] == slot) mode = FIRE_BACKWARD;
        break;
    }
    case ITEM_SHIELD: fire = 1; break;
    case ITEM_BOLT:
    case ITEM_BARGE:
    case ITEM_MINE:
        for (int j = 0; j < MAX_PLAYERS; j++) {
            const Player *o = &g->players[j];
            if (j == slot || !o->active || o->place) continue;
            float dx = o->x - p->x, dz = o->z - p->z, d = sqrtf(dx * dx + dz * dz);
            float ahead = dx * fx + dz * fz, side = fabsf(dx * fz - dz * fx);
            if (p->item == ITEM_BOLT && ahead > 3 && ahead < 70 && side < 3.5f) { fire = 1; mode = FIRE_FORWARD; }
            if (p->item == ITEM_BOLT && !fire && ahead < -3 && ahead > -45 && side < 3.0f) { fire = 1; mode = FIRE_BACKWARD; }
            if (p->item == ITEM_BARGE && d < 11.0f) fire = 1;
            if (p->item == ITEM_MINE && ahead < -3 && ahead > -40 && side < 6) { fire = 1; mode = FIRE_BACKWARD; }
            if (p->item == ITEM_MINE && hard && !fire && ahead > 25 && ahead < 45 && side < 4) { fire = 1; mode = FIRE_FORWARD; }
        }
        if (b->item_timer > 9) { fire = 1; mode = p->item == ITEM_MINE ? FIRE_BACKWARD : FIRE_FORWARD; }
        break;
    }
    if (fire) { b->seq++; b->item_timer = p->item == ITEM_BOLT ? delay - 0.45f : 0; out->fire_seq = b->seq; out->fire_mode = (uint8_t)mode; }
}

/* ------------------------------------------------ Ketma-ketlashtirish */
typedef struct { uint8_t *p; int len, cap, ok; } Writer;
typedef struct { const uint8_t *p; int len, pos, ok; } Reader;

static void w_bytes(Writer *w, const void *d, int n)
{
    if (!w->ok || w->len + n > w->cap) { w->ok = 0; return; }
    memcpy(w->p + w->len, d, (size_t)n);
    w->len += n;
}
static void w_u8(Writer *w, uint8_t v)   { w_bytes(w, &v, 1); }
static void w_u16(Writer *w, uint16_t v) { w_bytes(w, &v, 2); }
static void w_u32(Writer *w, uint32_t v) { w_bytes(w, &v, 4); }
static void w_f32(Writer *w, float v)    { w_bytes(w, &v, 4); }

static void r_bytes(Reader *r, void *d, int n)
{
    if (!r->ok || r->pos + n > r->len) { r->ok = 0; memset(d, 0, (size_t)n); return; }
    memcpy(d, r->p + r->pos, (size_t)n);
    r->pos += n;
}
static uint8_t  r_u8(Reader *r)  { uint8_t v;  r_bytes(r, &v, 1); return v; }
static uint16_t r_u16(Reader *r) { uint16_t v; r_bytes(r, &v, 2); return v; }
static uint32_t r_u32(Reader *r) { uint32_t v; r_bytes(r, &v, 4); return v; }
static float    r_f32(Reader *r) { float v;    r_bytes(r, &v, 4); return v; }

int game_serialize(const GameState *g, uint8_t *buf, int cap)
{
    Writer w = { buf, 0, cap, 1 };
    w_u8(&w, g->phase); w_u8(&w, g->bots_enabled); w_u8(&w, g->laps); w_u8(&w, g->finished); w_u8(&w, g->map_id);
    w_u32(&w, g->tick);
    w_f32(&w, g->elapsed); w_f32(&w, g->countdown); w_f32(&w, g->over_timer);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        const Player *p = &g->players[i];
        w_u8(&w, (uint8_t)((p->active ? 1 : 0) | (p->is_bot ? 2 : 0) | (p->shield > 0 ? 4 : 0) | (p->nitro > 0 ? 8 : 0) | (p->wrong_way ? 16 : 0)));
        w_u8(&w, p->place); w_u8(&w, (uint8_t)p->lap); w_u8(&w, p->item); w_u8(&w, p->charges);
        w_f32(&w, p->x); w_f32(&w, p->z); w_f32(&w, p->heading); w_f32(&w, p->speed);
        w_u8(&w, (uint8_t)clampf(p->stun * 50.0f, 0, 255));
        w_f32(&w, p->progress); w_f32(&w, p->lat); w_f32(&w, p->finish_time);
        w_u16(&w, (uint16_t)p->idx);
        w_u8(&w, p->last_fire_seq);
        w_u8(&w, p->fires); w_u8(&w, p->last_used); w_u8(&w, p->hits); w_u8(&w, p->pickups); w_u8(&w, p->pops);
    }
    uint32_t mask = 0;
    for (int k = 0; k < PICKUPS; k++) if (g->pickups[k].active) mask |= 1u << k;
    w_u32(&w, mask);
    for (int k = 0; k < PICKUPS; k++) w_u8(&w, g->pickups[k].type);
    uint8_t n = 0;
    for (int k = 0; k < MAX_PROJ; k++) n += g->proj[k].active;
    w_u8(&w, n);
    for (int k = 0; k < MAX_PROJ; k++) {
        const Projectile *pr = &g->proj[k];
        if (!pr->active) continue;
        w_u8(&w, pr->type); w_u8(&w, pr->owner);
        w_f32(&w, pr->x); w_f32(&w, pr->z); w_f32(&w, pr->heading); w_f32(&w, pr->speed);
    }
    n = 0;
    for (int k = 0; k < MAX_MINES; k++) n += g->mines[k].active;
    w_u8(&w, n);
    for (int k = 0; k < MAX_MINES; k++) {
        const Mine *m = &g->mines[k];
        if (!m->active) continue;
        w_f32(&w, m->x); w_f32(&w, m->z); w_u8(&w, (uint8_t)(m->arm <= 0));
    }
    return w.ok ? w.len : -1;
}

int game_deserialize(GameState *g, const uint8_t *buf, int len)
{
    Reader r = { buf, len, 0, 1 };
    GameState s;
    memset(&s, 0, sizeof(s));
    s.phase = r_u8(&r); s.bots_enabled = r_u8(&r); s.laps = r_u8(&r); s.finished = r_u8(&r); s.map_id = r_u8(&r);
    s.tick = r_u32(&r);
    s.elapsed = r_f32(&r); s.countdown = r_f32(&r); s.over_timer = r_f32(&r);
    if (s.phase > PHASE_OVER || s.laps == 0 || s.laps > 20 || s.map_id >= MAP_COUNT) return 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player *p = &s.players[i];
        uint8_t f = r_u8(&r);
        p->active = f & 1; p->is_bot = (f >> 1) & 1;
        p->shield = (f & 4) ? 1.0f : 0.0f; p->nitro = (f & 8) ? 1.0f : 0.0f; p->wrong_way = (f >> 4) & 1;
        p->place = r_u8(&r); p->lap = (int8_t)r_u8(&r); p->item = r_u8(&r); p->charges = r_u8(&r);
        p->x = r_f32(&r); p->z = r_f32(&r); p->heading = r_f32(&r); p->speed = r_f32(&r);
        p->stun = r_u8(&r) / 50.0f;
        p->progress = r_f32(&r); p->lat = r_f32(&r); p->finish_time = r_f32(&r);
        p->idx = r_u16(&r);
        p->last_fire_seq = r_u8(&r);
        p->fires = r_u8(&r); p->last_used = r_u8(&r); p->hits = r_u8(&r); p->pickups = r_u8(&r); p->pops = r_u8(&r);
        if (p->item >= ITEM_COUNT || p->idx >= TRACK_MAX) return 0;
    }
    uint32_t mask = r_u32(&r);
    for (int k = 0; k < PICKUPS; k++) s.pickups[k].active = (mask >> k) & 1;
    for (int k = 0; k < PICKUPS; k++) { s.pickups[k].type = r_u8(&r); if (s.pickups[k].type >= ITEM_COUNT) return 0; }
    int n = r_u8(&r);
    if (n > MAX_PROJ) return 0;
    for (int k = 0; k < n; k++) {
        Projectile *pr = &s.proj[k];
        pr->active = 1;
        pr->type = r_u8(&r); pr->owner = r_u8(&r);
        pr->x = r_f32(&r); pr->z = r_f32(&r); pr->heading = r_f32(&r); pr->speed = r_f32(&r);
    }
    n = r_u8(&r);
    if (n > MAX_MINES) return 0;
    for (int k = 0; k < n; k++) {
        Mine *m = &s.mines[k];
        m->active = 1;
        m->x = r_f32(&r); m->z = r_f32(&r); m->arm = r_u8(&r) ? 0.0f : 1.0f;
    }
    if (!r.ok) return 0;
    /* qutilar joyi trassadan hisoblanadi (game_place_pickups) - buni asosiy oqim qiladi,
     * chunki trassani almashtirish faqat asosiy oqimda xavfsiz */
    s.rng = g->rng;
    *g = s;
    return 1;
}
