/*
 * plat.c - Platforma qatlamining amalga oshirilishi.
 * Windows uchun Win32/Winsock2, qolganlari (Linux, Android) uchun POSIX.
 */
#include "plat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* ======================================================= WINDOWS == */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

plat_mutex plat_mutex_create(void)
{
    CRITICAL_SECTION *cs = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs);
    return cs;
}
void plat_mutex_lock(plat_mutex m)    { EnterCriticalSection((CRITICAL_SECTION *)m); }
void plat_mutex_unlock(plat_mutex m)  { LeaveCriticalSection((CRITICAL_SECTION *)m); }
void plat_mutex_destroy(plat_mutex m) { DeleteCriticalSection((CRITICAL_SECTION *)m); free(m); }

typedef struct { plat_thread_fn fn; void *arg; } ThreadBoot;

static DWORD WINAPI thread_trampoline(LPVOID p)
{
    ThreadBoot b = *(ThreadBoot *)p;
    free(p);
    return (DWORD)b.fn(b.arg);
}

plat_thread plat_thread_start(plat_thread_fn fn, void *arg)
{
    ThreadBoot *b = (ThreadBoot *)malloc(sizeof(ThreadBoot));
    b->fn = fn; b->arg = arg;
    HANDLE h = CreateThread(NULL, 0, thread_trampoline, b, 0, NULL);
    if (!h) { free(b); return NULL; }
    return h;
}

void plat_thread_join(plat_thread t)
{
    if (!t) return;
    WaitForSingleObject((HANDLE)t, INFINITE);
    CloseHandle((HANDLE)t);
}

double plat_time(void)
{
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}

void plat_sleep_ms(int ms) { Sleep((DWORD)ms); }

int plat_net_init(void)
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}
void plat_net_shutdown(void) { WSACleanup(); }

plat_sock plat_udp_open(uint16_t port, int allow_broadcast)
{
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return PLAT_BAD_SOCK;

    BOOL yes = TRUE;
    if (allow_broadcast) setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));

    DWORD timeout = 100;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    /* Windows'da yopilgan portga yuborilgan paketdan keyin ICMP xabari
     * recvfrom ni WSAECONNRESET bilan buzadi. Shu xatti-harakatni o'chiramiz. */
    BOOL no = FALSE;
    DWORD ret = 0;
    WSAIoctl(s, SIO_UDP_CONNRESET, &no, sizeof(no), NULL, 0, &ret, NULL, NULL);

    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0) { closesocket(s); return PLAT_BAD_SOCK; }
    return (plat_sock)s;
}

void plat_udp_close(plat_sock s) { if (s != PLAT_BAD_SOCK) closesocket((SOCKET)s); }

int plat_udp_send(plat_sock s, uint32_t ip, uint16_t port, const void *data, int len)
{
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(ip);
    a.sin_port = htons(port);
    return sendto((SOCKET)s, (const char *)data, len, 0, (struct sockaddr *)&a, sizeof(a));
}

int plat_udp_recv(plat_sock s, void *buf, int cap, uint32_t *ip, uint16_t *port)
{
    struct sockaddr_in a;
    int alen = sizeof(a);
    int n = recvfrom((SOCKET)s, (char *)buf, cap, 0, (struct sockaddr *)&a, &alen);
    if (n == SOCKET_ERROR) {
        int e = WSAGetLastError();
        if (e == WSAETIMEDOUT || e == WSAEWOULDBLOCK || e == WSAECONNRESET || e == WSAEMSGSIZE) return 0;
        return -1;
    }
    if (ip) *ip = ntohl(a.sin_addr.s_addr);
    if (port) *port = ntohs(a.sin_port);
    return n;
}

int plat_local_ips(uint32_t *out, int max)
{
    char name[256];
    int count = 0;
    if (gethostname(name, sizeof(name)) != 0) return 0;
    struct addrinfo hints, *res = NULL, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(name, NULL, &hints, &res) != 0) return 0;
    for (p = res; p && count < max; p = p->ai_next) {
        uint32_t ip = ntohl(((struct sockaddr_in *)p->ai_addr)->sin_addr.s_addr);
        if ((ip >> 24) == 127) continue;
        int dup = 0;
        for (int i = 0; i < count; i++) if (out[i] == ip) dup = 1;
        if (!dup) out[count++] = ip;
    }
    freeaddrinfo(res);
    return count;
}

#else
/* ================================================= POSIX / ANDROID == */
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>

plat_mutex plat_mutex_create(void)
{
    pthread_mutex_t *m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);
    return m;
}
void plat_mutex_lock(plat_mutex m)    { pthread_mutex_lock((pthread_mutex_t *)m); }
void plat_mutex_unlock(plat_mutex m)  { pthread_mutex_unlock((pthread_mutex_t *)m); }
void plat_mutex_destroy(plat_mutex m) { pthread_mutex_destroy((pthread_mutex_t *)m); free(m); }

typedef struct { plat_thread_fn fn; void *arg; } ThreadBoot;

static void *thread_trampoline(void *p)
{
    ThreadBoot b = *(ThreadBoot *)p;
    free(p);
    b.fn(b.arg);
    return NULL;
}

plat_thread plat_thread_start(plat_thread_fn fn, void *arg)
{
    pthread_t *t = (pthread_t *)malloc(sizeof(pthread_t));
    ThreadBoot *b = (ThreadBoot *)malloc(sizeof(ThreadBoot));
    b->fn = fn; b->arg = arg;
    if (pthread_create(t, NULL, thread_trampoline, b) != 0) { free(b); free(t); return NULL; }
    return t;
}

void plat_thread_join(plat_thread t)
{
    if (!t) return;
    pthread_join(*(pthread_t *)t, NULL);
    free(t);
}

double plat_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void plat_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int plat_net_init(void) { return 1; }
void plat_net_shutdown(void) {}

plat_sock plat_udp_open(uint16_t port, int allow_broadcast)
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) return PLAT_BAD_SOCK;

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (allow_broadcast) setsockopt(s, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0) { close(s); return PLAT_BAD_SOCK; }
    return (plat_sock)s;
}

void plat_udp_close(plat_sock s) { if (s != PLAT_BAD_SOCK) close((int)s); }

int plat_udp_send(plat_sock s, uint32_t ip, uint16_t port, const void *data, int len)
{
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(ip);
    a.sin_port = htons(port);
    return (int)sendto((int)s, data, (size_t)len, 0, (struct sockaddr *)&a, sizeof(a));
}

int plat_udp_recv(plat_sock s, void *buf, int cap, uint32_t *ip, uint16_t *port)
{
    struct sockaddr_in a;
    socklen_t alen = sizeof(a);
    ssize_t n = recvfrom((int)s, buf, (size_t)cap, 0, (struct sockaddr *)&a, &alen);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ECONNREFUSED) return 0;
        return -1;
    }
    if (ip) *ip = ntohl(a.sin_addr.s_addr);
    if (port) *port = ntohs(a.sin_port);
    return (int)n;
}

int plat_local_ips(uint32_t *out, int max)
{
    struct ifaddrs *list = NULL, *p;
    int count = 0;
    if (getifaddrs(&list) != 0) return 0;
    for (p = list; p && count < max; p = p->ifa_next) {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        if (!(p->ifa_flags & IFF_UP)) continue;
        uint32_t ip = ntohl(((struct sockaddr_in *)p->ifa_addr)->sin_addr.s_addr);
        if ((ip >> 24) == 127) continue;
        out[count++] = ip;
    }
    freeifaddrs(list);
    return count;
}
#endif

/* ============================================== Umumiy funksiyalar == */
uint32_t plat_parse_ip(const char *str)
{
    unsigned a, b, c, d;
    char tail;
    if (!str || sscanf(str, "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4) return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255) return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

void plat_ip_str(uint32_t ip, char out[16])
{
    snprintf(out, 16, "%u.%u.%u.%u", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
}
