/*
 * net_test.c - Grafikasiz avtomatik testlar.
 *  1) Trassa: halqa yopiq, qismlari kesishmaydi.
 *  2) Tarmoq: host + 3 mijoz, 5-o'yinchi rad etiladi, mijozning gazi va
 *     o'q uzishi serverga yetadi, holat mijozlarga keladi.
 *  3) To'liq poyga botlar bilan: hamma marraga yetadi, qurollar ishlatiladi,
 *     hech kim qotib qolmaydi.
 *
 *   gcc -O2 -Isrc tests/net_test.c src/game.c src/track.c src/net.c src/plat.c -lm -lpthread -o net_test
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/net.h"

#define CHECK(cond, msg) do { if (!(cond)) { printf("XATO: %s\n", msg); return 1; } else printf("OK:   %s\n", msg); } while (0)

static Input client_in[4];

static void pump(Server *s, Client *c, int nc, double seconds)
{
    double t0 = plat_time(), last = t0;
    while (plat_time() - t0 < seconds) {
        double now = plat_time();
        Input local = { 0, 127, 0, 0 };
        server_update(s, local, now - last);
        last = now;
        for (int i = 0; i < nc; i++) client_send_input(&c[i], client_in[i]);
        plat_sleep_ms(16);
    }
}

static int race(int map, int level, int laps, float *time_out, int *fires, int *hits, int *stuck)
{
    GameState g;
    BotBrain br[MAX_PLAYERS];
    Input in[MAX_PLAYERS];
    memset(br, 0, sizeof(br));
    game_init(&g);
    game_set_map(&g, map);
    g.bots_enabled = (uint8_t)level;
    g.laps = (uint8_t)laps;
    for (int i = 0; i < MAX_PLAYERS; i++) game_add_player(&g, i, 1);
    game_start_match(&g);
    float last_prog[MAX_PLAYERS] = { 0 };
    *stuck = 0;
    for (int t = 0; t < TICK_RATE * 900 && g.phase != PHASE_OVER; t++) {
        for (int i = 0; i < MAX_PLAYERS; i++) bot_think(&g, i, &br[i], &in[i], 1.0f / TICK_RATE);
        game_step(&g, in, 1.0f / TICK_RATE);
        if (g.phase == PHASE_PLAY && t % (TICK_RATE * 10) == 0 && g.elapsed > 5) {
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!g.players[i].place && g.players[i].progress - last_prog[i] < 30.0f) (*stuck)++;
                last_prog[i] = g.players[i].progress;
            }
        }
    }
    *time_out = g.elapsed;
    *fires = 0; *hits = 0;
    int places = 0, finished_all = 1;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        *fires += g.players[i].fires;
        *hits += g.players[i].hits;
        places |= 1 << g.players[i].place;
        if (g.players[i].lap < laps) finished_all = 0;
    }
    return g.phase == PHASE_OVER && places == 0x1E && finished_all;
}

int main(void)
{
    plat_net_init();
    for (int m = 0; m < MAP_COUNT; m++) {
        track_init(m);
        printf("      %-14s %.0f m, %d namuna, qismlar orasidagi eng kichik masofa %.0f m\n",
               track_map_name(m), TRACK.length, TRACK.n, track_min_separation());
        CHECK(track_min_separation() > TRACK_W + 20, "trassa qismlari bir-biriga tegmaydi");
    }
    track_init(0);

    /* ---- rul: o'ngga bosilganda mashina o'ngga (ekranda) buriladi ---- */
    {
        GameState t;
        Input in[MAX_PLAYERS];
        memset(in, 0, sizeof(in));
        game_init(&t);
        game_add_player(&t, 0, 0);
        game_start_match(&t);
        t.phase = PHASE_PLAY;
        in[0] = (Input){ 127, 127, 0, 0 };
        float h0 = t.players[0].heading;
        for (int k = 0; k < 60; k++) game_step(&t, in, 1.0f / TICK_RATE);
        /* kamera +z ga qarab turganda ekranning o'ng tomoni -x: heading kamayadi */
        CHECK(t.players[0].heading < h0 - 0.05f, "o'ngga rul - mashina o'ngga buriladi");
    }

    /* ---- qurol rejimlari: oldinga / orqaga / tashlash ---- */
    {
        GameState t;
        Input in[MAX_PLAYERS];
        memset(in, 0, sizeof(in));
        game_init(&t);
        for (int i = 0; i < 3; i++) game_add_player(&t, i, 0);
        game_start_match(&t);
        t.phase = PHASE_PLAY;
        /* 0 - o'rtada, 1 - oldinda, 2 - orqada */
        V2 q0 = track_point_at(400, 0), q1 = track_point_at(430, 0), q2 = track_point_at(370, 0);
        t.players[0].x = q0.x; t.players[0].z = q0.z; t.players[0].heading = track_heading_at(track_index_at(400));
        t.players[1].x = q1.x; t.players[1].z = q1.z;
        t.players[2].x = q2.x; t.players[2].z = q2.z;
        for (int i = 0; i < 3; i++) {
            t.players[i].heading = t.players[0].heading;
            t.players[i].idx = track_index_at(i == 0 ? 400 : i == 1 ? 430 : 370);
            t.players[i].s = 300; t.players[i].lap = 0;
        }
        for (int k = 0; k < 2; k++) game_step(&t, in, 1.0f / TICK_RATE);

        t.players[0].item = ITEM_SHUNT; t.players[0].charges = 1;
        in[0] = (Input){ 0, 0, 1, FIRE_BACKWARD };
        game_step(&t, in, 1.0f / TICK_RATE);
        int target = -1; float dir = 0;
        for (int k = 0; k < MAX_PROJ; k++) if (t.proj[k].active) {
            target = t.proj[k].target;
            dir = cosf(t.proj[k].heading - t.players[0].heading);
        }
        CHECK(target == 2 && dir < -0.9f, "orqaga otilgan raketa orqadagi mashinani quvadi");

        memset(t.proj, 0, sizeof(t.proj));
        t.players[0].item = ITEM_MINE; t.players[0].charges = 1;
        in[0] = (Input){ 0, 0, 2, FIRE_FORWARD };
        game_step(&t, in, 1.0f / TICK_RATE);
        float ahead = -99;
        for (int k = 0; k < MAX_MINES; k++) if (t.mines[k].active)
            ahead = (t.mines[k].x - t.players[0].x) * sinf(t.players[0].heading) + (t.mines[k].z - t.players[0].z) * cosf(t.players[0].heading);
        CHECK(ahead > 20.0f, "oldinga otilgan mina mashinadan oldinga tushdi");

        memset(t.mines, 0, sizeof(t.mines));
        t.players[0].item = ITEM_MINE; t.players[0].charges = 1;
        in[0] = (Input){ 0, 0, 3, FIRE_BACKWARD };
        game_step(&t, in, 1.0f / TICK_RATE);
        for (int k = 0; k < MAX_MINES; k++) if (t.mines[k].active)
            ahead = (t.mines[k].x - t.players[0].x) * sinf(t.players[0].heading) + (t.mines[k].z - t.players[0].z) * cosf(t.players[0].heading);
        CHECK(ahead < -2.0f, "orqaga mina - mashina ortiga tashlandi");

        t.players[0].item = ITEM_BOLT; t.players[0].charges = 3;
        in[0] = (Input){ 0, 0, 4, FIRE_DISCARD };
        game_step(&t, in, 1.0f / TICK_RATE);
        int nproj = 0;
        for (int k = 0; k < MAX_PROJ; k++) nproj += t.proj[k].active;
        CHECK(t.players[0].item == ITEM_NONE && nproj == 0, "qurolni tashlab yuborish ishlaydi");

        int types[ITEM_COUNT] = { 0 };
        for (int k = 0; k < PICKUPS; k++) types[t.pickups[k].type]++;
        int kinds = 0;
        for (int k = 1; k < ITEM_COUNT; k++) kinds += types[k] > 0;
        CHECK(types[0] == 0 && kinds >= 4, "qutilarda turli qurollar bor (uzoqdan ko'rinadi)");
    }

    /* ---- tarmoq ---- */
    Server s;
    Client c[4];
    GameState g;
    int slot;
    CHECK(server_start(&s), "server 47820-portda ishga tushdi");
    server_set_bots(&s, BOTS_OFF);
    server_set_map(&s, 2);
    for (int i = 0; i < 3; i++) CHECK(client_start(&c[i], 0x7F000001u), "mijoz ishga tushdi");
    pump(&s, c, 3, 1.0);
    int slot_of[3];
    for (int i = 0; i < 3; i++) {
        CHECK(client_status(&c[i]) == CL_CONNECTED, "mijoz ulandi");
        client_get_state(&c[i], &g, &slot);
        slot_of[i] = slot;
    }
    CHECK(client_start(&c[3], 0x7F000001u), "5-mijoz ishga tushdi");
    pump(&s, c, 3, 1.0);
    CHECK(client_status(&c[3]) == CL_REJECTED_FULL, "5-o'yinchi 'joy yo'q' javobini oldi");
    client_stop(&c[3]);

    server_begin_match(&s);
    client_in[0] = (Input){ 0, 127, 0, 0 };    /* gaz */
    client_in[1] = (Input){ 0, 0, 0, 0 };      /* turibdi */
    client_in[2] = (Input){ 127, 127, 0, 0 };  /* o'ngga buriladi */
    pump(&s, c, 3, COUNTDOWN_TIME + 2.0);
    server_get_state(&s, &g);
    const Player *a = &g.players[slot_of[0]], *b = &g.players[slot_of[1]];
    CHECK(g.phase == PHASE_PLAY && a->speed > 15.0f && fabsf(b->speed) < 0.1f, "mijozning gazi serverga yetdi");
    CHECK(fabsf(g.players[slot_of[2]].heading - track_heading_at(g.players[slot_of[2]].idx)) > 0.2f, "mijozning rul burishi serverga yetdi");

    /* qurolni qo'lda beramiz va o'q uzamiz */
    plat_mutex_lock(s.lock);
    s.gs.players[slot_of[0]].item = ITEM_SHIELD;
    s.gs.players[slot_of[0]].charges = 1;
    plat_mutex_unlock(s.lock);
    client_in[0].fire_seq = 1;
    pump(&s, c, 3, 0.4);
    server_get_state(&s, &g);
    CHECK(g.players[slot_of[0]].item == ITEM_NONE && g.players[slot_of[0]].shield > 0 && g.players[slot_of[0]].fires == 1,
          "o'q uzish buyrug'i bir marta bajarildi (qalqon yoqildi)");

    GameState cg;
    client_get_state(&c[1], &cg, &slot);
    CHECK(cg.map_id == 2, "mijoz tanlangan xaritani (Qorli tog') bildi");
    CHECK(cg.phase == PHASE_PLAY && fabsf(cg.players[slot_of[0]].x - g.players[slot_of[0]].x) < 5.0f, "mijozlar yangi holatni olyapti");
    uint8_t buf[SNAPSHOT_MAX];
    int len = game_serialize(&g, buf, sizeof(buf));
    printf("      snapshot hajmi: %d bayt\n", len);
    CHECK(len > 0 && len < 1200, "snapshot bitta UDP paketga sig'adi");

    server_stop(&s);
    plat_sleep_ms(600);
    CHECK(client_status(&c[0]) == CL_HOST_CLOSED, "mijoz server yopilganini bildi");
    for (int i = 0; i < 3; i++) client_stop(&c[i]);

    /* ---- to'liq poyga ---- */
    for (int m = 0; m < MAP_COUNT; m++)
    for (int level = BOTS_EASY; level <= BOTS_HARD; level++) {
        float t;
        int fires, hits, stuck;
        int ok = race(m, level, 3, &t, &fires, &hits, &stuck);
        printf("      %-14s %s botlar, 3 aylana: %.0f s, %d qurol, %d urildi, qotib qolish: %d\n",
               track_map_name(m), level == BOTS_EASY ? "oson " : "qiyin", t, fires, hits, stuck);
        CHECK(ok, "hamma 3 aylanani tugatdi, o'rinlar 1..4");
        CHECK(fires > 10 && hits > 3, "qurollar ishlatildi va nishonga tegdi");
        CHECK(stuck == 0, "hech bir mashina qotib qolmadi");
    }

    plat_net_shutdown();
    printf("\nBarcha testlar o'tdi.\n");
    return 0;
}
