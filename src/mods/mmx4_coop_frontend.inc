/* Original stage-board actors, native animation tables and immutable donor
 * graphics. Included beside the co-op plugin's existing UI texture banks. */
static void board_package_activate(void) { /* Hooks use the co-op snapshot/profile. */ }
static int board_active(void) {
    unsigned phase=psx_mod_read_byte(PLAY+1);
    /* Conversation actors share this backdrop. The partner's update below
     * accepts only idle/selection commands, never the campaign's dialogue. */
    return board_enabled && !failed && board_assets[0].loaded && board_assets[1].loaded &&
        psx_mod_read_byte(PLAY)==3 && ((phase>=1 && phase<=7) || phase==9);
}
static int board_activate(void) {
    memset(board_heroes,0,sizeof board_heroes);board_call=0;
    memset(board_companions,0,sizeof board_companions);
    for(unsigned character=0;character<2;++character) {
        BoardAssets *a=&board_assets[character];
        free(a->file);free(a->colors);free(a->pixels);memset(a,0,sizeof *a);
    }
    /* Netplay uses a host-synchronized profile, not personal offline mods. */
    char option[8];
    board_enabled=psx_mod_netplay_is_active()?
        (psx_mod_current_option_value("stage_select_cast",option,sizeof option) && !strcmp(option,"true")):
        mod_runtime_snapshot_package_active(BOARD_ID);
    if(!board_enabled)return 1;
    for(unsigned character=0;character<2;++character) {
        BoardAssets *a=&board_assets[character];
        const char *name=character?"ARC/ST0D_0Z.ARC":"ARC/ST0D_0X.ARC";
        if(!psx_mod_read_disc_file(name,NULL,0,&a->size) || a->size>0x50000u ||
           !(a->file=malloc(a->size)) ||
           !psx_mod_read_disc_file(name,a->file,a->size,&a->size))return 0;
        Mmx4Asset image,layout;
        if(!mmx4_coop_arc_asset(a->file,a->size,0,&image) || image.type!=0x10102u ||
           !mmx4_coop_arc_asset(a->file,a->size,1,&layout) || layout.type!=12 || layout.size!=20 ||
           le32(layout.data+8)!=0xF800u ||
           !mmx4_coop_arc_asset(a->file,a->size,2,&a->poses) || a->poses.type!=10 ||
           a->poses.size>0x2000u || a->poses.size<16)return 0;
        a->origin[0]=(uint16_t)(le32(layout.data+8)>>7);
        a->origin[1]=(uint16_t)(le32(layout.data+12)>>7);
        CharacterAssets raw={0};raw.ui_pixels=calloc(1024u*512u,sizeof(uint16_t));
        a->pixels=raw.ui_pixels;
        if(!a->pixels || !retain_ui(&raw,&image))return 0;
        uint8_t col[6144];uint32_t size=0;
        name=character?"ARC/COL0D_0Z.ARC":"ARC/COL0D_0X.ARC";
        Mmx4Asset colors;
        if(!psx_mod_read_disc_file(name,col,sizeof col,&size) ||
           !mmx4_coop_arc_asset(col,size,0,&colors) || colors.type!=5 || colors.size!=4096 ||
           !(a->colors=malloc(4096)))return 0;
        memcpy(a->colors,colors.data,4096);
        a->assembly=psx_mod_alloc_guest_memory(0x2000u,16);
        if(!a->assembly)return 0;
        project(a->assembly,a->poses.data,(uint32_t)a->poses.size);a->loaded=1;
    }
    return 1;
}
static void board_admit(CPUState *cpu,uint32_t address);
static int board_initialize(CPUState *cpu,uint32_t address) {
    if(board_call || !board_enabled || psx_mod_local_view_scope())return 0;
    board_call=1;
    memset(board_heroes,0,sizeof board_heroes);
    memset(board_companions,0,sizeof board_companions);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    board_call=0;
    board_admit(cpu,address);
    return mmx4_coop_finish(cpu,result);
}
static void board_admit(CPUState *cpu,uint32_t address) {
    (void)address;
    if(board_call || !board_active() || board_heroes[0] || psx_mod_local_view_scope())return;
    board_call=1;
    unsigned campaign=psx_mod_read_byte(PLAY+0x43);
    /* Native companion admission owns the shared campaign's story boundary.
     * Both companions leave at that same authored point, with no P2 story. */
    for(uint32_t actor=0x80173CA0u;actor<0x801754A0u;actor+=0x60u)
        if(psx_mod_read_byte(actor)==0x41 && psx_mod_read_byte(actor+1)==10) {
            if(!psx_mod_read_byte(actor+2))board_heroes[0]=actor;
            else if(campaign<2)board_companions[campaign]=actor;
        }
    if(board_heroes[0]) {
        uint32_t actor=guest(cpu,0x8002AE50u,0,0);
        if(actor>=0x80173CA0u && actor<0x801754A0u && !((actor-0x80173CA0u)%0x60u)) {
            psx_mod_write_byte(actor,0x41);psx_mod_write_byte(actor+1,10);psx_mod_write_byte(actor+2,0);
            board_heroes[1]=actor;psx_mod_counter_add("mmx4.coop.board-partner-created",1);
        }
    }
    if(campaign<2 && board_companions[campaign]) {
        uint32_t actor=guest(cpu,0x8002AE50u,0,0);
        if(actor>=0x80173CA0u && actor<0x801754A0u && !((actor-0x80173CA0u)%0x60u)) {
            psx_mod_write_byte(actor,0x41);psx_mod_write_byte(actor+1,10);psx_mod_write_byte(actor+2,1);
            board_companions[campaign^1u]=actor;
            psx_mod_counter_add("mmx4.coop.board-companion-created",1);
        }
    }
    board_call=0;
}
static void board_companion_position(uint32_t actor,unsigned character,unsigned viewer) {
    unsigned primary=character==viewer;
    psx_mod_write_word(actor+8,(primary?288u:302u)<<16);
    psx_mod_write_word(actor+12,(primary?432u:412u)<<16);
}
static int board_character(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(board_call || !board_active() || psx_mod_local_view_scope())return 0;
    unsigned campaign=psx_mod_read_byte(PLAY+0x43),character=campaign^1u,npc=0;
    if(campaign>1 || psx_mod_read_byte(actor)!=0x41 || psx_mod_read_byte(actor+1)!=10)return 0;
    if(actor==board_companions[0] || actor==board_companions[1]) {
        npc=1;character=actor==board_companions[1];
    }else if(actor==board_heroes[0])character=campaign;
    else if(actor!=board_heroes[1])return 0;
    if(character==campaign)return 0; /* Original actors keep their native update. */
    uint32_t assembly=psx_mod_read_word(SCRATCH+0x20);
    uint8_t control=psx_mod_read_byte(PLAY+0x40);
    board_call=1;
    unsigned initialize=!psx_mod_read_byte(actor+4);
    uint32_t result=0;
    if(initialize) {
        /* Raw donor assembly is immutable but lives in snapshotted guest
         * storage. Refill it before native initialization after any restore. */
        project(board_assets[character].assembly,board_assets[character].poses.data,
            (uint32_t)board_assets[character].poses.size);
        psx_mod_write_word(SCRATCH+0x20,board_assets[character].assembly);
        result=guest(cpu,address,actor,cpu->gpr[5]);
        psx_mod_write_word(SCRATCH+0x20,assembly);
        /* Native initialization chooses a table from the campaign. Replace
         * only this added actor's table, without projecting PLAY+43 even
         * briefly: frontend/save observers always see P1's campaign. */
        psx_mod_write_word(actor+0x30,npc?(character?0x8010E55Cu:0x8010E4ECu):
            (character?0x8010E538u:0x8010E514u));
        psx_mod_write_half(actor+0x40,board_assets[character].origin[npc]);
        guest(cpu,0x80015D60u,actor,0);
    }else {
        /* The added cast uses retail animation/visibility helpers directly.
         * Do not alter the shared dialogue control byte. Sequence8 is a talk
         * command outside Iris's table; only stage-confirm sequence1 can
         * interrupt the added characters' idle animations. */
        unsigned phase=psx_mod_read_byte(PLAY+1);
        if(phase>=4 && phase<=7 && (control&0x10u) && (control&15u)==1u)
            guest(cpu,0x80015D60u,actor,1);
        else if((phase<4 || phase==9) && psx_mod_read_byte(actor+0x17))
            guest(cpu,0x80015D60u,actor,0);
        else guest(cpu,0x80015DC8u,actor,0);
        guest(cpu,0x8002B288u,actor,0);
    }
    if(npc)board_companion_position(actor,character,campaign);
    else psx_mod_write_word(actor+8,54u<<16);
    board_call=0;return mmx4_coop_finish(cpu,result);
}
static void board_draw_begin(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(board_active())world_packet_used=0;
}
static int render_board_actor(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(rendering || board_call || !board_active() || psx_mod_read_byte(actor)!=0x41 ||
       psx_mod_read_byte(actor+1)!=10)return 0;
    unsigned campaign=psx_mod_read_byte(PLAY+0x43),character=campaign,npc=0;
    unsigned partner=actor==board_heroes[1];
    if(actor==board_companions[0] || actor==board_companions[1]) {
        npc=1;character=actor==board_companions[1];
    }else if(actor!=board_heroes[0] && !partner)return 0;
    else if(partner)character^=1u;
    uint32_t x=psx_mod_read_word(actor+8);
    uint32_t y=psx_mod_read_word(actor+12);
    uint8_t priority=psx_mod_read_byte(actor+0x16);
    /* Both views use campaign/P1 first, with P2 and their support behind. */
    psx_mod_write_byte(actor+0x16,character!=campaign?4:3);
    if(npc)board_companion_position(actor,character,campaign);
    else psx_mod_write_word(actor+8,(partner?54u:24u)<<16);
    rendering=1;
    if(character==campaign) {
        uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
        psx_mod_write_word(actor+8,x);psx_mod_write_word(actor+12,y);
        psx_mod_write_byte(actor+0x16,priority);rendering=0;
        return mmx4_coop_finish(cpu,result);
    }
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA,start=base+world_packet_used;
    psx_mod_write_word(SCRATCH+0x100,start);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    uint32_t end=psx_mod_read_word(SCRATCH+0x100);
    if(end>=start && end-base<=40000 && !((end-start)%40u)) {
        for(uint32_t at=start;at<end;at+=40,mode+=8) {
            uint32_t q[10];for(unsigned i=0;i<10;++i)q[i]=psx_mod_read_word(at+i*4);
            uint16_t tile=UINT16_MAX;
            unsigned command=q[1]>>24;
            if((command&0xFC)==0x7C) {
                int px=(int16_t)q[2],py=(int16_t)(q[2]>>16);uint32_t clut=q[3]&0xFFFF0000u;
                tile=(uint16_t)q[3];q[1]=(q[1]&0x03FFFFFFu)|0x2C000000u;
                for(unsigned i=0;i<4;++i) {
                    q[2+i*2]=(uint16_t)(px+(i&1)*16)|((uint32_t)(uint16_t)(py+(i>>1)*16)<<16);
                    q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
                }
                q[3]|=clut;q[5]|=(psx_mod_read_word(mode+4)&0x60u)<<16;
            }else if((command&0xFC)!=0x2C)continue;
            uint16_t page=tile==UINT16_MAX?(uint16_t)(q[5]>>16):(uint16_t)psx_mod_read_word(mode+4);
            uint16_t bank=ui_tile(character+2u,page,(uint16_t)(q[3]>>16),tile,1);
            if(!bank) {psx_mod_counter_add("mmx4.coop.board-bank-failed",1);continue;}
            uint32_t expanded=base+40000+(at-base)*2;
            triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,q[0],q,2,1,3,bank);
            psx_mod_tag_world_primitive(expanded,0);psx_mod_tag_world_primitive(expanded+40,0);
            psx_mod_write_word(at,expanded&0xFFFFFFu);
            unsigned layer=psx_mod_read_byte(actor+0x16);
            uint32_t tail=0x8013BC40u+(psx_mod_read_word(SCRATCH)&1u)*128+(layer>>4)*32+(layer&15)*4;
            if(psx_mod_read_word(tail)==at)psx_mod_write_word(tail,expanded+40);
        }
        world_packet_used=end-base;
    }else psx_mod_counter_add("mmx4.coop.board-packet-overflow",1);
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(actor+8,x);
    psx_mod_write_word(actor+12,y);
    psx_mod_write_byte(actor+0x16,priority);rendering=0;
    return mmx4_coop_finish(cpu,result);
}
