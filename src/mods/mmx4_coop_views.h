#ifndef MMX4_COOP_VIEWS_H
#define MMX4_COOP_VIEWS_H
#include <stdint.h>
#include "mod_visible_placements.h"

#define MMX4_COOP_PLACEMENTS 4096u

typedef struct {
    int32_t camera_x,camera_y;
} Mmx4CoopView;
typedef struct {
    int16_t world_x,world_y;
    unsigned active;
} Mmx4CoopViewActor;
typedef struct {
    int32_t minimum_x,maximum_x,minimum_y,maximum_y;
    int32_t left,right,top,bottom;
} Mmx4CoopViewBounds;
typedef struct {
    uint8_t seen[MMX4_COOP_PLACEMENTS/8u];
} Mmx4CoopViewVisits;

int mmx4_coop_views_requested(int online,const char *cameras);
int mmx4_coop_camera_scope_requested(int split,int ready,int first_alive,int second_alive);
Mmx4CoopView mmx4_coop_view_follow(Mmx4CoopView previous,
    Mmx4CoopViewActor actor,Mmx4CoopViewBounds bounds);
unsigned mmx4_coop_view_regions(const Mmx4CoopView views[2],
    const Mmx4CoopViewActor actors[2],int scene_owner,int margin,
    PSXPlacementRect regions[2]);
int mmx4_coop_view_contains(const PSXPlacementRect *regions,unsigned count,
    int32_t position_x,int32_t position_y);
int mmx4_coop_view_beam_contains(Mmx4CoopView view,int position_x,int position_y,
    int start_x,int end_x,int start_y,int end_y,int margin);
unsigned mmx4_coop_view_nearest(const Mmx4CoopViewActor actors[2],
    int16_t position_x,int16_t position_y,unsigned preferred);
void mmx4_coop_view_visit_update(Mmx4CoopViewVisits *visits,unsigned index,
    int inside,int live);
int mmx4_coop_view_spawn_allowed(const Mmx4CoopViewVisits *visits,
    unsigned index,int inside,int live);
#endif
