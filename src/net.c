/*
 * net.c - Server, mijoz va hostni topish oqimlari.
 */
#include "net.h"

#include <string.h>
#include <stdlib.h>

/* ================================================================ */
/*                              SERVER                              */
/* ================================================================ */

static int find_client(Server *s, uint32_t ip, uint16_t port)
{
    for (int i = 1; i < MAX_PLAYERS; i++)
        if (s->clients[i].used && s->clients[i].ip == ip && s->clients[i].port == port) return i;
    return -1;
}

static int human_count(Server *s)
{
    int n = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (s->gs.players[i].active && !s->gs.players[i].is_bot) n++;
    return n;
}

/* Slot bo'shaganda: botlar yoqilgan bo'lsa o'rniga bot qo'yiladi */
static void free_slot(Server *s, int slot)
{
    s->clients[slot].used = 0;
    memset(&s->inputs[slot], 0, sizeof(Input));
    if (s->gs.bots_enabled) {
        game_add_player(&s->gs, slot, 1);
        memset(&s->bots[slot], 0, sizeof(BotBrain));
    } else {
        game_remove_player(&s->gs, slot);
    }
}

static void send_small(plat_sock sock, uint32_t ip, uint16_t port, uint8_t type, uint8_t a, uint8_t b, uint8_t c)
{
    uint8_t p[5] = { PKT_MAGIC, type, a, b, c };
    plat_udp_send(sock, ip, port, p, 5);
}

static int server_thread(void *arg)
{
    Server *s = (Server *)arg;
    uint8_t buf[2048];
    while (s->running) {
        uint32_t ip; uint16_t port;
        int n = plat_udp_recv(s->sock, buf, sizeof(buf), &ip, &port);
        if (n < 2 || buf[0] != PKT_MAGIC) continue;
        double now = plat_time();

        plat_mutex_lock(s->lock);
        switch (buf[1]) {
        case PKT_DISCOVER:
            send_small(s->sock, ip, port, PKT_HOST_INFO, (uint8_t)human_count(s), s->gs.phase, GAME_VERSION);
            break;

        case PKT_JOIN: {
            if (n < 3 || buf[2] != GAME_VERSION) {
                send_small(s->sock, ip, port, PKT_REJECT, REJECT_VERSION, 0, 0);
                break;
            }
            int slot = find_client(s, ip, port);
            if (slot < 0) {
                /* avval butunlay bo'sh slot, keyin bot egallagan slot */
                for (int i = 1; i < MAX_PLAYERS && slot < 0; i++)
                    if (!s->gs.players[i].active) slot = i;
                for (int i = 1; i < MAX_PLAYERS && slot < 0; i++)
                    if (s->gs.players[i].is_bot) slot = i;
                if (slot < 0) {
                    send_small(s->sock, ip, port, PKT_REJECT, REJECT_FULL, 0, 0);
                    break;
                }
                s->clients[slot].used = 1;
                s->clients[slot].ip = ip;
                s->clients[slot].port = port;
                game_add_player(&s->gs, slot, 0);
                memset(&s->inputs[slot], 0, sizeof(Input));
            }
            s->clients[slot].last_seen = now;
            send_small(s->sock, ip, port, PKT_WELCOME, (uint8_t)slot, 0, 0);
            break;
        }

        case PKT_INPUT: {
            int slot = find_client(s, ip, port);
            if (slot < 0 || n < 5) break;
            s->inputs[slot].steer = (int8_t)buf[2];
            s->inputs[slot].throttle = (int8_t)buf[3];
            s->inputs[slot].fire_seq = buf[4];
            s->inputs[slot].fire_mode = n >= 6 && buf[5] <= FIRE_DISCARD ? buf[5] : FIRE_FORWARD;
            s->clients[slot].last_seen = now;
            break;
        }

        case PKT_LEAVE: {
            int slot = find_client(s, ip, port);
            if (slot >= 0) free_slot(s, slot);
            break;
        }
        }
        plat_mutex_unlock(s->lock);
    }
    return 0;
}

int server_start(Server *s)
{
    memset(s, 0, sizeof(*s));
    s->sock = plat_udp_open(GAME_PORT, 1);
    if (s->sock == PLAT_BAD_SOCK) return 0;
    s->lock = plat_mutex_create();
    game_init(&s->gs);
    game_add_player(&s->gs, 0, 0);       /* host o'zi - slot 0 */
    s->clients[0].used = 1;
    /* standart holatda bo'sh joylarda oson botlar - yolg'iz ham o'ynash mumkin */
    s->gs.bots_enabled = 1;
    for (int i = 1; i < MAX_PLAYERS; i++) game_add_player(&s->gs, i, 1);
    s->running = 1;
    s->thread = plat_thread_start(server_thread, s);
    if (!s->thread) {
        s->running = 0;
        plat_udp_close(s->sock);
        plat_mutex_destroy(s->lock);
        return 0;
    }
    return 1;
}

void server_stop(Server *s)
{
    if (!s->running) return;
    /* mijozlarga server yopilganini 3 marta aytamiz (UDP paket yo'qolishi mumkin) */
    for (int k = 0; k < 3; k++)
        for (int i = 1; i < MAX_PLAYERS; i++)
            if (s->clients[i].used) send_small(s->sock, s->clients[i].ip, s->clients[i].port, PKT_CLOSED, 0, 0, 0);
    s->running = 0;
    plat_thread_join(s->thread);
    plat_udp_close(s->sock);
    plat_mutex_destroy(s->lock);
}

void server_update(Server *s, Input local, double dt)
{
    static uint8_t pkt[SNAPSHOT_MAX + 3];
    RemoteClient targets[MAX_PLAYERS];
    int len = -1;
    double now = plat_time();
    const double step = 1.0 / TICK_RATE;

    plat_mutex_lock(s->lock);

    /* javob bermay qolgan mijozlarni chiqarib yuborish */
    for (int i = 1; i < MAX_PLAYERS; i++)
        if (s->clients[i].used && now - s->clients[i].last_seen > CLIENT_TIMEOUT) free_slot(s, i);

    s->inputs[0] = local;
    if (dt > 0.25) dt = 0.25;
    s->accum += dt;
    int steps = 0;
    while (s->accum >= step && steps < 8) {
        for (int i = 0; i < MAX_PLAYERS; i++)
            if (s->gs.players[i].active && s->gs.players[i].is_bot)
                bot_think(&s->gs, i, &s->bots[i], &s->inputs[i], (float)step);
        game_step(&s->gs, s->inputs, (float)step);
        s->accum -= step;
        steps++;
    }
    if (steps == 8) s->accum = 0;

    if (steps > 0) {
        len = game_serialize(&s->gs, pkt + 3, SNAPSHOT_MAX);
        memcpy(targets, s->clients, sizeof(targets));
    }
    plat_mutex_unlock(s->lock);

    /* yuborish mutexdan tashqarida - qabul qiluvchi oqimni ushlab turmaymiz */
    if (len > 0) {
        pkt[0] = PKT_MAGIC;
        pkt[1] = PKT_STATE;
        for (int i = 1; i < MAX_PLAYERS; i++) {
            if (!targets[i].used) continue;
            pkt[2] = (uint8_t)i;
            plat_udp_send(s->sock, targets[i].ip, targets[i].port, pkt, len + 3);
        }
    }
}

void server_get_state(Server *s, GameState *out)
{
    plat_mutex_lock(s->lock);
    *out = s->gs;
    plat_mutex_unlock(s->lock);
}

void server_begin_match(Server *s)
{
    plat_mutex_lock(s->lock);
    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (s->clients[i].used) continue;
        if (s->gs.bots_enabled) game_add_player(&s->gs, i, 1);
        else game_remove_player(&s->gs, i);
        memset(&s->bots[i], 0, sizeof(BotBrain));
    }
    game_start_match(&s->gs);
    plat_mutex_unlock(s->lock);
}

void server_back_to_lobby(Server *s)
{
    plat_mutex_lock(s->lock);
    s->gs.phase = PHASE_LOBBY;
    plat_mutex_unlock(s->lock);
}

void server_set_laps(Server *s, int laps)
{
    plat_mutex_lock(s->lock);
    if (s->gs.phase == PHASE_LOBBY || s->gs.phase == PHASE_OVER) s->gs.laps = (uint8_t)laps;
    plat_mutex_unlock(s->lock);
}

/* trassa global (TRACK) - faqat asosiy oqimdan chaqiriladi */
void server_set_map(Server *s, int map)
{
    plat_mutex_lock(s->lock);
    if (s->gs.phase == PHASE_LOBBY || s->gs.phase == PHASE_OVER) {
        game_set_map(&s->gs, map);
        memset(s->bots, 0, sizeof(s->bots));
    }
    plat_mutex_unlock(s->lock);
}

void server_set_bots(Server *s, int level)
{
    plat_mutex_lock(s->lock);
    int enabled = level > 0;
    s->gs.bots_enabled = (uint8_t)(level < 0 ? 0 : level > 2 ? 2 : level);
    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (s->clients[i].used) continue;
        if (enabled && s->gs.players[i].active && s->gs.players[i].is_bot) continue;  /* daraja o'zgardi, xolos */
        if (enabled) game_add_player(&s->gs, i, 1);
        else game_remove_player(&s->gs, i);
    }
    plat_mutex_unlock(s->lock);
}


/* ================================================================ */
/*                              MIJOZ                               */
/* ================================================================ */

static int client_thread(void *arg)
{
    Client *c = (Client *)arg;
    uint8_t buf[2048];
    while (c->running) {
        double now = plat_time();

        plat_mutex_lock(c->lock);
        int status = c->status;
        if (status == CL_CONNECTING) {
            if (now - c->last_join > 0.5) {
                uint8_t p[3] = { PKT_MAGIC, PKT_JOIN, GAME_VERSION };
                plat_udp_send(c->sock, c->host_ip, GAME_PORT, p, 3);
                c->last_join = now;
            }
            if (now - c->started > 6.0) c->status = CL_TIMEOUT;
        } else if (status == CL_CONNECTED && now - c->last_recv > CLIENT_TIMEOUT) {
            c->status = CL_LOST;
        }
        plat_mutex_unlock(c->lock);

        uint32_t ip; uint16_t port;
        int n = plat_udp_recv(c->sock, buf, sizeof(buf), &ip, &port);
        if (n < 2 || buf[0] != PKT_MAGIC || ip != c->host_ip || port != GAME_PORT) continue;

        plat_mutex_lock(c->lock);
        if (c->status == CL_CONNECTING || c->status == CL_CONNECTED) {
            switch (buf[1]) {
            case PKT_WELCOME:
                if (n >= 3) { c->slot = buf[2]; c->status = CL_CONNECTED; c->last_recv = now; }
                break;
            case PKT_REJECT:
                if (c->status == CL_CONNECTING)
                    c->status = (n >= 3 && buf[2] == REJECT_VERSION) ? CL_REJECTED_VERSION : CL_REJECTED_FULL;
                break;
            case PKT_STATE:
                if (n >= 3 && game_deserialize(&c->gs, buf + 3, n - 3)) {
                    c->slot = buf[2];
                    c->have_state = 1;
                    c->status = CL_CONNECTED;
                    c->last_recv = now;
                }
                break;
            case PKT_CLOSED:
                c->status = CL_HOST_CLOSED;
                break;
            }
        }
        plat_mutex_unlock(c->lock);
    }
    return 0;
}

int client_start(Client *c, uint32_t host_ip)
{
    memset(c, 0, sizeof(*c));
    c->sock = plat_udp_open(0, 0);
    if (c->sock == PLAT_BAD_SOCK) return 0;
    c->lock = plat_mutex_create();
    c->host_ip = host_ip;
    c->status = CL_CONNECTING;
    c->started = plat_time();
    c->last_join = 0;
    c->running = 1;
    c->thread = plat_thread_start(client_thread, c);
    if (!c->thread) {
        c->running = 0;
        plat_udp_close(c->sock);
        plat_mutex_destroy(c->lock);
        return 0;
    }
    return 1;
}

void client_stop(Client *c)
{
    if (!c->running) return;
    uint8_t p[2] = { PKT_MAGIC, PKT_LEAVE };
    for (int k = 0; k < 3; k++) plat_udp_send(c->sock, c->host_ip, GAME_PORT, p, 2);
    c->running = 0;
    plat_thread_join(c->thread);
    plat_udp_close(c->sock);
    plat_mutex_destroy(c->lock);
}

void client_send_input(Client *c, Input in)
{
    uint8_t p[6] = { PKT_MAGIC, PKT_INPUT, (uint8_t)in.steer, (uint8_t)in.throttle, in.fire_seq, in.fire_mode };
    plat_udp_send(c->sock, c->host_ip, GAME_PORT, p, 6);
}

int client_status(Client *c)
{
    plat_mutex_lock(c->lock);
    int s = c->status;
    plat_mutex_unlock(c->lock);
    return s;
}

int client_get_state(Client *c, GameState *out, int *slot)
{
    plat_mutex_lock(c->lock);
    int ok = c->have_state;
    if (ok) { *out = c->gs; *slot = c->slot; }
    plat_mutex_unlock(c->lock);
    return ok;
}

/* ================================================================ */
/*                    HOSTNI TOPISH (broadcast)                     */
/* ================================================================ */

static int discovery_thread(void *arg)
{
    Discovery *d = (Discovery *)arg;
    uint8_t buf[256];
    double last_ping = 0;
    while (d->running) {
        double now = plat_time();
        if (now - last_ping > 1.0) {
            uint8_t p[2] = { PKT_MAGIC, PKT_DISCOVER };
            /* umumiy broadcast + har bir tarmoq uchun x.y.z.255 (hotspotlar odatda /24) */
            plat_udp_send(d->sock, 0xFFFFFFFFu, GAME_PORT, p, 2);
            uint32_t ips[8];
            int n = plat_local_ips(ips, 8);
            for (int i = 0; i < n; i++) plat_udp_send(d->sock, ips[i] | 0xFFu, GAME_PORT, p, 2);
            last_ping = now;
        }

        uint32_t ip; uint16_t port;
        int n = plat_udp_recv(d->sock, buf, sizeof(buf), &ip, &port);
        if (n < 5 || buf[0] != PKT_MAGIC || buf[1] != PKT_HOST_INFO) continue;

        plat_mutex_lock(d->lock);
        int idx = -1;
        for (int i = 0; i < d->count; i++) if (d->hosts[i].ip == ip) idx = i;
        if (idx < 0 && d->count < MAX_FOUND_HOSTS) idx = d->count++;
        if (idx >= 0) {
            d->hosts[idx].ip = ip;
            d->hosts[idx].players = buf[2];
            d->hosts[idx].phase = buf[3];
            d->hosts[idx].seen = now;
        }
        plat_mutex_unlock(d->lock);
    }
    return 0;
}

int discovery_start(Discovery *d)
{
    memset(d, 0, sizeof(*d));
    d->sock = plat_udp_open(0, 1);
    if (d->sock == PLAT_BAD_SOCK) return 0;
    d->lock = plat_mutex_create();
    d->running = 1;
    d->thread = plat_thread_start(discovery_thread, d);
    if (!d->thread) {
        d->running = 0;
        plat_udp_close(d->sock);
        plat_mutex_destroy(d->lock);
        return 0;
    }
    return 1;
}

void discovery_stop(Discovery *d)
{
    if (!d->running) return;
    d->running = 0;
    plat_thread_join(d->thread);
    plat_udp_close(d->sock);
    plat_mutex_destroy(d->lock);
}

int discovery_list(Discovery *d, HostEntry *out, int max)
{
    int n = 0;
    double now = plat_time();
    plat_mutex_lock(d->lock);
    for (int i = 0; i < d->count && n < max; i++)
        if (now - d->hosts[i].seen < 3.5) out[n++] = d->hosts[i];
    plat_mutex_unlock(d->lock);
    return n;
}
