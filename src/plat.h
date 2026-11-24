/*
 * plat.h - Platforma qatlami (Windows / Linux / Android)
 *
 * Bu modul operatsion tizimning past darajali xizmatlarini bitta
 * interfeys ortiga yashiradi:
 *   - oqimlar (threads)        : Win32 CreateThread  / POSIX pthread
 *   - mutekslar (mutex)        : CRITICAL_SECTION    / pthread_mutex_t
 *   - vaqt va uyqu             : QueryPerformanceCounter / clock_gettime
 *   - UDP soketlar             : Winsock2            / BSD sockets
 *   - lokal IP manzillar       : getaddrinfo         / getifaddrs
 *
 * MUHIM: bu fayl raylib.h ni ulamaydi, chunki windows.h va raylib.h
 * nomlari (Rectangle, CloseWindow, DrawText ...) to'qnashadi.
 */
#ifndef PLAT_H
#define PLAT_H

#include <stdint.h>

/* ---------- Oqimlar va mutekslar ---------- */
typedef void *plat_mutex;
typedef void *plat_thread;
typedef int (*plat_thread_fn)(void *arg);

plat_mutex  plat_mutex_create(void);
void        plat_mutex_lock(plat_mutex m);
void        plat_mutex_unlock(plat_mutex m);
void        plat_mutex_destroy(plat_mutex m);

plat_thread plat_thread_start(plat_thread_fn fn, void *arg);
void        plat_thread_join(plat_thread t);

double      plat_time(void);          /* monoton vaqt, soniyalarda */
void        plat_sleep_ms(int ms);

/* ---------- UDP tarmoq ---------- */
typedef intptr_t plat_sock;
#define PLAT_BAD_SOCK ((plat_sock)-1)

int       plat_net_init(void);
void      plat_net_shutdown(void);

/* port = 0 bo'lsa tizim bo'sh portni tanlaydi.
 * Har bir soketda 100 ms qabul qilish taymauti o'rnatiladi, shunda
 * qabul qiluvchi oqim "running" bayrog'ini tekshirib tura oladi. */
plat_sock plat_udp_open(uint16_t port, int allow_broadcast);
void      plat_udp_close(plat_sock s);

/* IP manzillar host tartibida (masalan 192.168.1.5 = 0xC0A80105). */
int       plat_udp_send(plat_sock s, uint32_t ip, uint16_t port, const void *data, int len);
/* Qaytaradi: >0 baytlar soni, 0 taymaut, <0 xato */
int       plat_udp_recv(plat_sock s, void *buf, int cap, uint32_t *ip, uint16_t *port);

int       plat_local_ips(uint32_t *out, int max);  /* 127.x dan tashqari IPv4 lar */
uint32_t  plat_parse_ip(const char *str);          /* 0 = noto'g'ri */
void      plat_ip_str(uint32_t ip, char out[16]);

#endif
