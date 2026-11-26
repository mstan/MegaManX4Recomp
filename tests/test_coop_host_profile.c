#include "mmx4_coop_host_profile.h"
#include "execution_profile.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t state_digest(void) { return 1; }
static PSXModNetplayProfile profile = {
    .plugin_id = "mmx4.coop", .compatibility_id = "test", .skip_bios_intro = 1,
    .state_digest = state_digest, .requires_retained_texture_banks = 1,
    .settings_package_id = "mmx4.coop", .settings_feature_id = "coop",
    .renderer_option_id = "cameras", .renderer_option_value = "split"
};
const PSXModNetplayProfile *psx_mod_netplay_profile(void) { return &profile; }

int main(void) {
    uint8_t image[32] = {1};
    PSXModNetplayHostStartup c = {0};
    c.cpu.architecture = PSX_HOST_ARCH_X64;
    c.cpu.features_known = 1;
    c.cpu.features = 127;
    c.compiled_backends = c.available_backends = PSX_HOST_BACKEND_ENHANCED;
    c.fp_contract = 2;
    c.interpreted_overlays_sealed = c.builtin_linked_bios = 1;
    c.actual_executable_sha256 = image;
    c.declared_build_manifest_sha256 = "a123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    c.declared_build_manifest_schema = 1;
    c.builtin_bios_image_id = "OPENBIOS";
    c.builtin_bios_image_wordsum = 0xc4daeb88u;
    c.slot_count = c.player_count = 2;
    c.occupied_mask = 3;
    c.renderer = c.retained_texture_banks = 1;
    PSXHostProfileCatalog cat;
#ifdef EXPECT_EXECUTION_MISMATCH
    /* A refusal control must really exercise the old/current ID boundary. */
    assert(strcmp(PSX_EXECUTION_ID, MMX4_HOST_EXECUTION));
#endif
#ifdef EXPECT_REFUSAL
    assert(!mmx4_coop_host_profiles(&c, &cat) && cat.count == 0);
#else
    assert(!strcmp(PSX_EXECUTION_ID, MMX4_HOST_EXECUTION));
    assert(mmx4_coop_host_profiles(&c, &cat) && cat.count == 1);
    assert(cat.rules->flags == PSX_HOST_PROFILE_PORTABLE);
    assert(!strcmp(cat.rules->profile, MMX4_HOST_PROFILE));
    /* The normal two-host route and narrower loopback route share one policy. */
    c.direct_loopback_scope = 1;
    assert(mmx4_coop_host_profiles(&c, &cat));
    c.direct_loopback_scope = 0;
    c.cpu.features = 63;
    assert(mmx4_coop_host_profiles(&c, &cat));
#define REFUSE(field, value) do { PSXModNetplayHostStartup bad = c; bad.field = value; \
    assert(!mmx4_coop_host_profiles(&bad, &cat) && cat.count == 0); } while (0)
    REFUSE(interpreted_overlays_sealed, 0);
    REFUSE(builtin_linked_bios, 0);
    REFUSE(builtin_bios_image_wordsum, 0);
    REFUSE(actual_executable_sha256, NULL);
    REFUSE(declared_build_manifest_schema, 0);
    REFUSE(declared_build_manifest_sha256, "bad");
    REFUSE(cpu.features_known, 0);
    REFUSE(cpu.features, 128);
    REFUSE(available_backends, 0);
    REFUSE(fp_contract, 1);
    REFUSE(slot_count, 3);
    REFUSE(player_count, 1);
    REFUSE(occupied_mask, 1);
    REFUSE(rollback, 1);
    REFUSE(renderer, 0);
    REFUSE(retained_texture_banks, 0);
    REFUSE(aspect_index, 1);
    REFUSE(observer, 1);
    profile.renderer_option_value = "unified";
    assert(!mmx4_coop_host_profiles(&c, &cat) && cat.count == 0);
    profile.renderer_option_value = "split";
    profile.savestates_supported = 1;
    assert(!mmx4_coop_host_profiles(&c, &cat) && cat.count == 0);
    assert(!mmx4_coop_host_profiles(&c, NULL));
#endif
    puts("X4 host profile admission controls passed");
    return 0;
}
