#include "mmx4_coop_views.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(value) do { if(!(value)) { \
    fprintf(stderr,"line %d: %s\n",__LINE__,#value);exit(1); } } while(0)

int main(void) {
    CHECK(!mmx4_coop_views_requested(0,"split"));
    CHECK(!mmx4_coop_views_requested(1,"unified"));
    CHECK(!mmx4_coop_views_requested(1,NULL));
    CHECK(!mmx4_coop_views_requested(1,"invalid"));
    CHECK(mmx4_coop_views_requested(1,"split"));
    CHECK(!mmx4_coop_camera_scope_requested(0,1,1,0));
    CHECK(mmx4_coop_camera_scope_requested(1,1,1,0));
    CHECK(mmx4_coop_camera_scope_requested(0,1,0,1));
    CHECK(mmx4_coop_camera_scope_requested(1,1,0,1));
    CHECK(!mmx4_coop_camera_scope_requested(1,1,0,0));
    CHECK(!mmx4_coop_camera_scope_requested(1,0,1,1));
    Mmx4CoopViewBounds bounds={0,8192,0,4096,128,192,96,160};
    Mmx4CoopView previous={0,0};
    Mmx4CoopViewActor actors[2]={{56,138,1},{1000,2000,1}};
    Mmx4CoopView views[2]={mmx4_coop_view_follow(previous,actors[0],bounds),
        mmx4_coop_view_follow(previous,actors[1],bounds)};
    CHECK(views[0].camera_x==0 && views[0].camera_y==0);
    CHECK(views[1].camera_x==808 && views[1].camera_y==1840);
    Mmx4CoopView still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);
    actors[0].world_x=3000;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);
    actors[1].world_x=-50;actors[1].world_y=5000;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==0 && still.camera_y==4096);
    actors[1].active=0;
    still=mmx4_coop_view_follow(views[1],actors[1],bounds);
    CHECK(still.camera_x==views[1].camera_x && still.camera_y==views[1].camera_y);

    PSXPlacementRect regions[2];
    actors[1].active=1;
    unsigned count=mmx4_coop_view_regions(views,actors,-1,0,regions);
    CHECK(count==2);
    CHECK(mmx4_coop_view_contains(regions,count,80,140));
    CHECK(mmx4_coop_view_contains(regions,count,1000,2000));
    CHECK(!mmx4_coop_view_beam_contains(views[0],1000,2000,0,64,0,64,0));
    CHECK(mmx4_coop_view_beam_contains(views[1],1000,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[0],100,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[1],100,2000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[0],600,1000,0,64,0,64,0));
    CHECK(!mmx4_coop_view_beam_contains(views[1],600,1000,0,64,0,64,0));
    CHECK(mmx4_coop_view_beam_contains(views[0],400,140,-240,-220,0,16,0));
    CHECK(!mmx4_coop_view_contains(regions,count,500,1000));
    CHECK(!mmx4_coop_view_contains(regions,count,80,2000));
    CHECK(!mmx4_coop_view_contains(regions,count,1000,140));
    CHECK(!mmx4_coop_view_contains(regions,count,-48,140));
    CHECK(!mmx4_coop_view_contains(regions,count,368,140));
    CHECK(mmx4_coop_view_regions(views,actors,1,0,regions)==1);
    CHECK(!mmx4_coop_view_contains(regions,1,80,140));
    CHECK(mmx4_coop_view_contains(regions,1,1000,2000));
    actors[1].active=0;
    CHECK(mmx4_coop_view_regions(views,actors,-1,0,regions)==1);
    actors[0].active=0;
    CHECK(!mmx4_coop_view_regions(views,actors,-1,0,regions));

    actors[0]=(Mmx4CoopViewActor){0,0,1};
    actors[1]=(Mmx4CoopViewActor){1000,0,1};
    CHECK(mmx4_coop_view_nearest(actors,900,0,0)==1);
    CHECK(mmx4_coop_view_nearest(actors,100,0,1)==0);
    CHECK(mmx4_coop_view_nearest(actors,500,0,0)==0);
    CHECK(mmx4_coop_view_nearest(actors,500,0,1)==1);
    actors[0].active=0;
    CHECK(mmx4_coop_view_nearest(actors,0,0,0)==1);

    Mmx4CoopViewVisits visits={0};
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,0);
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,1);
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,1));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,1,0);
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    mmx4_coop_view_visit_update(&visits,7,0,0);
    CHECK(mmx4_coop_view_spawn_allowed(&visits,7,1,0));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,7,0,0));
    CHECK(!mmx4_coop_view_spawn_allowed(&visits,MMX4_COOP_PLACEMENTS,1,0));
    puts("mmx4_coop_views_test: PASS");
    return 0;
}
