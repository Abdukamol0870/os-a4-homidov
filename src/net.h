/*
 * net.h - Tarmoq o'yini: server (host), mijoz va LAN'da hostni topish.
 *
 * Arxitektura: "avtoritar server".
 *   - Host o'yinchi kompyuteri/telefoni ham server, ham o'yinchi (slot 0).
 *   - Mijozlar faqat tugmalar holatini (Input) yuboradi.
 *   - Server sekundiga 60 marta o'yinni hisoblaydi va to'liq holatni
 *     (snapshot) hamma mijozlarga yuboradi.
 *
 * Har bir obyektda alohida qabul qiluvchi oqim (thread) bor.
 * Asosiy oqim va tarmoq oqimi umumiy ma'lumotga mutex orqali kiradi.
 *
 * Protokol (UDP, port 47820). Har paket: 'B', turi, ma'lumot...
 */
#ifndef NET_H
#define NET_H

#include "game.h"
#include "plat.h"

#define GAME_PORT 47820
#define PKT_MAGIC 'B'

enum {
    PKT_DISCOVER  = 1,   /* mijoz -> broadcast   : "kim host?"          */
    PKT_HOST_INFO = 2,   /* host  -> mijoz       : o'yinchilar, faza    */
    PKT_JOIN      = 3,   /* mijoz -> host        : versiya              */
    PKT_WELCOME   = 4,   /* host  -> mijoz       : sizning slotingiz    */
    PKT_REJECT    = 5,   /* host  -> mijoz       : sabab                */
    PKT_INPUT     = 6,   /* mijoz -> host        : rul, gaz, o'q        */
    PKT_STATE     = 7,   /* host  -> mijoz       : slot + snapshot      */
    PKT_LEAVE     = 8,   /* mijoz -> host        : chiqib ketdim        */
    PKT_CLOSED    = 9    /* host  -> mijoz       : server yopildi       */
};
enum { REJECT_FULL = 1, REJECT_VERSION = 2 };

#define CLIENT_TIMEOUT 4.0

/* --------------------------------------------------------- Server -- */
typedef struct {
    int      used;
    uint32_t ip;
    uint16_t port;
    double   last_seen;
} RemoteClient;

typedef struct {
    plat_sock    sock;
    plat_thread  thread;
    plat_mutex   lock;
    volatile int running;

    GameState    gs;
    Input        inputs[MAX_PLAYERS];
    BotBrain     bots[MAX_PLAYERS];
    RemoteClient clients[MAX_PLAYERS];   /* slot 0 - hostning o'zi */
    double       accum;
} Server;

int  server_start(Server *s);
void server_stop(Server *s);
void server_update(Server *s, Input local, double dt);
void server_get_state(Server *s, GameState *out);
void server_begin_match(Server *s);
void server_back_to_lobby(Server *s);
void server_set_laps(Server *s, int laps);
void server_set_bots(Server *s, int level);
void server_set_map(Server *s, int map);       /* 0..MAP_COUNT-1, faqat lobbida */   /* 0 o'chiq, 1 oson, 2 qiyin */

/* --------------------------------------------------------- Mijoz -- */
enum {
    CL_IDLE = 0, CL_CONNECTING, CL_CONNECTED,
    CL_REJECTED_FULL, CL_REJECTED_VERSION, CL_TIMEOUT, CL_LOST, CL_HOST_CLOSED
};

typedef struct {
    plat_sock    sock;
    plat_thread  thread;
    plat_mutex   lock;
    volatile int running;

    uint32_t     host_ip;
    int          slot;
    int          status;
    GameState    gs;
    int          have_state;
    double       started;
    double       last_recv;
    double       last_join;
} Client;

int  client_start(Client *c, uint32_t host_ip);
void client_stop(Client *c);
void client_send_input(Client *c, Input in);
int  client_status(Client *c);
int  client_get_state(Client *c, GameState *out, int *slot);

/* ------------------------------------------------------ Qidiruv -- */
#define MAX_FOUND_HOSTS 8
typedef struct {
    uint32_t ip;
    int      players;
    int      phase;
    double   seen;
} HostEntry;

typedef struct {
    plat_sock    sock;
    plat_thread  thread;
    plat_mutex   lock;
    volatile int running;
    HostEntry    hosts[MAX_FOUND_HOSTS];
    int          count;
} Discovery;

int  discovery_start(Discovery *d);
void discovery_stop(Discovery *d);
int  discovery_list(Discovery *d, HostEntry *out, int max);

#endif
