#include "mmx4_coop_views.h"
#include <string.h>

int mmx4_coop_views_requested(int online,const char *cameras) {
    return online && cameras && !strcmp(cameras,"split");
}
int mmx4_coop_camera_scope_requested(int split,int ready,int first_alive,int second_alive) {
    return ready && (second_alive || (split && first_alive));
}

static int32_t clamp(int32_t value,int32_t minimum,int32_t maximum) {
    if(maximum<minimum)maximum=minimum;
    return value<minimum?minimum:value>maximum?maximum:value;
}
static int32_t follow(int32_t previous,int32_t position,
                      int32_t lower,int32_t upper) {
    if(position-previous<lower)return position-lower;
    if(position-previous>upper)return position-upper;
    return previous;
}
Mmx4CoopView mmx4_coop_view_follow(Mmx4CoopView previous,
    Mmx4CoopViewActor actor,Mmx4CoopViewBounds bounds) {
    if(!actor.active)return previous;
    Mmx4CoopView view={
        clamp(follow(previous.camera_x,actor.world_x,bounds.left,bounds.right),
            bounds.minimum_x,bounds.maximum_x),
        clamp(follow(previous.camera_y,actor.world_y,bounds.top,bounds.bottom),
            bounds.minimum_y,bounds.maximum_y)};
    return view;
}
unsigned mmx4_coop_view_regions(const Mmx4CoopView views[2],
    const Mmx4CoopViewActor actors[2],int scene_owner,int margin,
    PSXPlacementRect regions[2]) {
    unsigned count=0;
    if(margin<0)margin=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (scene_owner>=0 && scene_owner!=(int)seat))continue;
        Mmx4CoopView view=views[seat];
        regions[count++]=(PSXPlacementRect){view.camera_x-48-margin,view.camera_x+368+margin,
            view.camera_y-48,view.camera_y+288};
    }
    return count;
}
int mmx4_coop_view_contains(const PSXPlacementRect *regions,unsigned count,
    int32_t position_x,int32_t position_y) {
    for(unsigned region=0;region<count;++region)
        if(psx_placement_contains(regions[region],position_x,position_y))return 1;
    return 0;
}
int mmx4_coop_view_beam_contains(Mmx4CoopView view,int position_x,int position_y,
    int start_x,int end_x,int start_y,int end_y,int margin) {
    int delta_x=end_x-start_x,delta_y=end_y-start_y;
    if(delta_x<0)delta_x=-delta_x;
    if(delta_y<0)delta_y=-delta_y;
    int relative_x=position_x-view.camera_x,relative_y=position_y-view.camera_y;
    int origin=(uint16_t)(relative_x+delta_x+margin)<(uint16_t)(320+2*delta_x+2*margin) &&
        (uint16_t)(relative_y+delta_y)<(uint16_t)(240+2*delta_y);
    int centre=(uint16_t)(relative_x+start_x+delta_x/2+delta_x+margin)<
            (uint16_t)(320+2*delta_x+2*margin) &&
        (uint16_t)(relative_y+start_y+delta_y/2+delta_y)<(uint16_t)(240+2*delta_y);
    return origin || centre;
}
unsigned mmx4_coop_view_nearest(const Mmx4CoopViewActor actors[2],
    int16_t position_x,int16_t position_y,unsigned preferred) {
    unsigned first=preferred<2?preferred:0,nearest=first;
    uint64_t best=UINT64_MAX;
    for(unsigned order=0;order<2;++order) {
        unsigned seat=first^order;
        if(!actors[seat].active)continue;
        int64_t delta_x=(int64_t)actors[seat].world_x-position_x;
        int64_t delta_y=(int64_t)actors[seat].world_y-position_y;
        uint64_t distance=(uint64_t)(delta_x*delta_x+delta_y*delta_y);
        if(distance<best) {best=distance;nearest=seat;}
    }
    return nearest;
}
void mmx4_coop_view_visit_update(Mmx4CoopViewVisits *visits,unsigned index,
    int inside,int live) {
    if(index>=MMX4_COOP_PLACEMENTS)return;
    unsigned byte=index/8u,mask=1u<<(index&7u);
    if(!inside)visits->seen[byte]&=(uint8_t)~mask;
    else if(live)visits->seen[byte]|=(uint8_t)mask;
}
int mmx4_coop_view_spawn_allowed(const Mmx4CoopViewVisits *visits,
    unsigned index,int inside,int live) {
    return index<MMX4_COOP_PLACEMENTS && inside && !live &&
        !(visits->seen[index/8u]&(1u<<(index&7u)));
}
