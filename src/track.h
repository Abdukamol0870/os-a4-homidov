/*
 * track.h - Poyga trassasi: tayanch nuqtalardan Catmull-Rom egri chizig'i
 * bilan hosil qilingan yopiq halqa. Trassa teng oraliqli namunalarga
 * bo'linadi; mashinaning qayerdaligi (yo'l bo'ylab masofa, markazdan
 * chetlanish) shu namunalar orqali tez topiladi.
 */
#ifndef TRACK_H
#define TRACK_H

#define TRACK_W        22.0f     /* yo'l kengligi, m */
#define TRACK_MAX      1200
#define TRACK_TREES    420
#define MAP_COUNT      4

typedef struct { float x, z; } V2;

typedef struct {
    int   map_id;
    int   n;                     /* namunalar soni */
    float length;                /* halqa uzunligi, m */
    float spacing;
    V2    p[TRACK_MAX];          /* markaz chizig'i */
    V2    t[TRACK_MAX];          /* yo'nalish (birlik vektor) */
    V2    nrm[TRACK_MAX];        /* chapga perpendikulyar */
    float s[TRACK_MAX];          /* boshidan masofa */
    float turn[TRACK_MAX];       /* burilish keskinligi (rad / 10 m) */
    float min_x, max_x, min_z, max_z;
    int   tree_count;
    V2    trees[TRACK_TREES];
    float tree_size[TRACK_TREES];
    float tree_clear[TRACK_TREES];   /* yo'l devoridan bo'sh masofa (m) */
} Track;

extern Track TRACK;

void  track_init(int map);
const char *track_map_name(int map);
/* eng yaqin namuna; hint >= 0 bo'lsa faqat atrofidan qidiradi.
 * lateral - markazdan chetlanish (chapga musbat), s_out - yo'l bo'ylab masofa */
int   track_nearest(float x, float z, int hint, float *lateral, float *s_out);
int   track_index_at(float s);
V2    track_point_at(float s, float lateral);
float track_heading_at(int i);   /* namunadagi yo'nalish burchagi (atan2(x, z)) */
float track_min_separation(void);

#endif
