#include "render_pass_sprite_scene.hpp"

// SLUS-00561: world draw calls background and actor construction. X4 has
// twelve OT buckets; its drawing bank is entirely described by DRAWENV.
struct MMX4Scene {
    static constexpr uint32_t Draw=0x80023D68, End=0x80023D88;
    static constexpr uint32_t Camera=0x80026698, CameraInstruction=0x0C009CA3;
    static constexpr uint32_t Actor=0x80024334, VSync=0x800E4DB0;
    static constexpr uint32_t Base=0x80166C10, Stride=0xA0, Buckets=12;
    static constexpr uint32_t CurrentBuffer=0x80142F80, Layers=0x801419B0;
    static constexpr uint32_t OffsetPackets=0, ClipPackets=0;
    static constexpr uint32_t PutDrawEnv=0x800EA880, DrawOTag=0x800EA80C, DrawSync=0x800EA20C;
    static constexpr uint32_t WaitReturn=0x80012050, PacketFloor=0x12F800;
    static constexpr const char* Package="mmx4.enhancement.frame-interpolation";
};
PSX_MOD_CONSTRUCTOR(mmx4_register_frame_interpolation_plugin) {
    PSXSpriteSceneReplay<MMX4Scene>::install("mmx4.frame-interpolation");
}
