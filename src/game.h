/*
 * game.h - TURBO JANG: qurolli poyga o'yini holati va simulyatsiyasi
 * (grafikasiz, sof C).
 *
 * 4 ta mashina burilishli trassada bir necha aylana poyga qiladi.
 * Yo'ldagi sovg'a qutilaridan tasodifiy qurol olinadi: nitro, raketa
 * (o'zi nishonni quvadi), chaqmoq (3 o'q), mina, zarba to'lqini, qalqon.
 * Orqada qolganlarga kuchliroq qurollar tushadi.
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "track.h"

#define GAME_VERSION    2
#define MAX_PLAYERS     4
#define MAX_PROJ        24
#define MAX_MINES       16
#define PICKUP_ROWS     6
#define PICKUPS         (PICKUP_ROWS * 4)
#define TICK_RATE       60

#define COUNTDOWN_TIME  3.0f
#define MAX_SPEED       44.0f     /* m/s ~ 158 km/soat */
#define NITRO_SPEED     62.0f
#define OFFROAD_SPEED   20.0f
#define ACCEL           17.0f
#define BRAKE           36.0f
#define CAR_RADIUS      1.5f
#define FINISH_WAIT     30.0f

enum { PHASE_LOBBY = 0, PHASE_COUNTDOWN, PHASE_PLAY, PHASE_OVER };
enum { BOTS_OFF = 0, BOTS_EASY = 1, BOTS_HARD = 2 };
enum { ITEM_NONE = 0, ITEM_NITRO, ITEM_SHUNT, ITEM_BOLT, ITEM_MINE, ITEM_BARGE, ITEM_SHIELD, ITEM_COUNT };
/* qurol ishlatish rejimi (Blur'dagidek) */
enum { FIRE_FORWARD = 0, FIRE_BACKWARD = 1, FIRE_DISCARD = 2 };

typedef struct {
    int8_t  steer;      /* -127 chap .. 127 o'ng */
    int8_t  throttle;   /* -127 tormoz/orqaga .. 127 gaz */
    uint8_t fire_seq;   /* har o'q uzishda +1 */
    uint8_t fire_mode;  /* FIRE_FORWARD / FIRE_BACKWARD / FIRE_DISCARD */
} Input;

typedef struct {
    uint8_t  active, is_bot;
    float    x, z, heading;      /* heading: atan2(dx, dz); yo'nalish = (sin h, cos h) */
    float    speed;
    int      idx;                /* trassadagi eng yaqin namuna */
    float    lat;                /* markazdan chetlanish */
    float    s;                  /* aylana ichidagi masofa */
    int8_t   lap;                /* tugatilgan aylanalar (startdan oldin -1) */
    float    progress;           /* lap * L + s - o'rin aniqlash uchun */
    uint8_t  place;
    float    finish_time;
    uint8_t  item, charges;
    float    stun, shield, nitro;
    uint8_t  wrong_way;
    uint8_t  last_fire_seq;
    /* hisoblagichlar - mijoz ovoz va effektlar uchun */
    uint8_t  fires, last_used, hits, pickups, pops;
} Player;

typedef struct {
    uint8_t active, type, owner, target;
    float   x, z, heading, speed, life;
} Projectile;

typedef struct {
    uint8_t active, owner;
    float   x, z, arm, life;
} Mine;

typedef struct {
    float   x, z, timer;
    uint8_t active;
    uint8_t type;        /* qutidagi qurol - uzoqdan ko'rinadi */
} Pickup;

typedef struct {
    uint8_t    phase, bots_enabled, laps, finished;
    uint8_t    map_id;
    uint32_t   tick;
    float      elapsed, countdown, over_timer;
    uint32_t   rng;
    Player     players[MAX_PLAYERS];
    Projectile proj[MAX_PROJ];
    Mine       mines[MAX_MINES];
    Pickup     pickups[PICKUPS];
} GameState;

typedef struct {
    float   lane, lane_timer, item_timer;
    uint8_t seq;
    uint32_t rng;
} BotBrain;

void game_init(GameState *g);
void game_set_map(GameState *g, int map);
void game_place_pickups(GameState *g);   /* qutilar joyini joriy trassadan hisoblash */
void game_start_match(GameState *g);
void game_add_player(GameState *g, int slot, int is_bot);
void game_remove_player(GameState *g, int slot);
void game_step(GameState *g, const Input inputs[MAX_PLAYERS], float dt);
void game_ranking(const GameState *g, int order[MAX_PLAYERS]);
void bot_think(const GameState *g, int slot, BotBrain *b, Input *out, float dt);

const char *game_color_name(int slot);
const char *game_item_name(int item);

#define SNAPSHOT_MAX 1200
int  game_serialize(const GameState *g, uint8_t *buf, int cap);
int  game_deserialize(GameState *g, const uint8_t *buf, int len);

#endif
