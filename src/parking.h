/*
 * parking.h - "Parkovka" rejimi (bitta o'yinchi, 20 daraja).
 * Sof C: grafikaga bog'liq emas, shuning uchun testda ham ishlatiladi.
 *
 * Koordinatalar poyga bilan bir xil: mashina yo'nalishi h bo'lsa,
 * oldinga vektor (sin h, cos h). rul > 0 - o'ngga.
 */
#ifndef PARKING_H
#define PARKING_H

#include <stdint.h>

#define PK_LEVELS     20
#define PK_MAX_OBS    72
#define PK_MAX_CONES  64
#define PK_MAX_BAYS   48
#define PK_MAX_HITS   1      /* bitta urilish - daraja yutqaziladi */

#define PK_CAR_W      2.0f   /* mashina eni   */
#define PK_CAR_L      4.5f   /* mashina bo'yi */
#define PK_WHEELBASE  2.7f
#define PK_MAX_STEER  0.6f   /* g'ildirakning eng katta burilishi, radian (~34 gradus) */
#define PK_MAX_FWD    9.0f   /* m/s */
#define PK_MAX_REV    4.5f
#define PK_CONE_R     0.35f
#define PK_HOLD_TIME  1.0f   /* joyda shuncha turib qolish kerak */

enum { OB_CAR = 0, OB_WALL, OB_BARRIER, OB_PILLAR, OB_PLANTER, OB_TRUCK };
enum { NEED_ANY = 0, NEED_NOSE_IN, NEED_REVERSE_IN };
enum { PK_PLAY = 0, PK_WON, PK_FAILED };

typedef struct { float x, z, h, w, l; uint8_t type, color; } PkObs;   /* yo'naltirilgan to'rtburchak */
typedef struct { float x, z, h, w, l; uint8_t parallel; } PkBay;     /* bo'yalgan joy chiziqlari */

typedef struct {
    const char *name, *hint;
    int    theme;                 /* 0 vodiy, 1 cho'l, 2 qor, 3 tungi shahar */
    float  lot_w, lot_l;          /* maydon: x in [-w/2, w/2], z in [-l/2, l/2] */
    float  sx, sz, sh;            /* boshlang'ich holat */
    PkBay  target;                /* maqsad joyi; target.h - "burun bilan kirish" yo'nalishi */
    int    need;                  /* NEED_* */
    float  par;                   /* 3 yulduz uchun vaqt, soniya */
    int    nobs, ncones, nbays;
    PkObs  obs[PK_MAX_OBS];
    float  cone_x[PK_MAX_CONES], cone_z[PK_MAX_CONES];
    PkBay  bays[PK_MAX_BAYS];
} PkLevel;

typedef struct { float x, z, h, speed, steer; } PkCar;

typedef struct {
    int     level;
    PkLevel L;
    PkCar   car;
    int     state;                /* PK_PLAY / PK_WON / PK_FAILED */
    float   time, hold, hit_cd, end_timer;
    int     hits, stars;
    uint8_t cone_down[PK_MAX_CONES];
    float   cone_vx[PK_MAX_CONES], cone_vz[PK_MAX_CONES], cone_rot[PK_MAX_CONES];
    /* effektlar uchun hodisalar (har kadrda o'qib, nolga tushiriladi) */
    int     ev_hit;  float ev_x, ev_z, ev_power;
    int     ev_cone;
} PkGame;

void  pk_build(int level, PkLevel *out);
void  pk_load(PkGame *g, int level);
/* steer -1..1 (o'ng musbat), throttle -1..1 (S: avval tormoz, keyin orqaga), handbrake 0/1 */
void  pk_step(PkGame *g, float steer, float throttle, int handbrake, float dt);

extern float pk_margin;   /* faqat test yechuvchisi uchun: mashinani biroz kattaroq deb hisoblash */
int   pk_collides(const PkLevel *L, const uint8_t *cone_down, float x, float z, float h);
float pk_fit(const PkLevel *L, float x, float z, float h);   /* 0..1: joyga qanchalik to'g'ri kirgan (1 - to'liq) */
int   pk_in_spot(const PkLevel *L, float x, float z, float h);
void  pk_corners(float x, float z, float h, float w, float l, float out[4][2]);
int   pk_stars_for(const PkLevel *L, float time, int hits);

#endif
