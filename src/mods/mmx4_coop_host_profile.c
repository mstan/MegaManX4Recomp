/* Conservative native co-op policy. Matching declarations are not proof:
 * qualification also exercises this policy on the supported physical hosts. */
#include "mmx4_coop_host_profile.h"
#include "execution_profile.h"
#include "psx_sha256.h"
#include <stdio.h>
#include <string.h>

#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__) && \
    defined(_WIN32) && defined(_WIN64) && defined(__MINGW64__) && \
    __GNUC__ == 15 && __GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ == 0
#define MMX4_HOST_COMPILER 1
#else
#define MMX4_HOST_COMPILER 0
#endif

static int valid_sha(const char *s) {
    unsigned nonzero = 0;
    if (!s || strlen(s) != 64) return 0;
    for (unsigned i = 0; i < 64; ++i) {
        const unsigned c = (unsigned char)s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
        nonzero |= c != '0';
    }
    return nonzero != 0;
}

static int release_context(const PSXModNetplayHostStartup *c) {
    if (!c || !c->actual_executable_sha256) return 0;
    unsigned nonzero = 0;
    for (unsigned i = 0; i < 32; ++i) nonzero |= c->actual_executable_sha256[i];
    const int players = c->player_count ? c->player_count : c->slot_count;
    const uint32_t seats = c->occupied_mask ? c->occupied_mask : 3u;
    return nonzero && c->cpu.architecture == PSX_HOST_ARCH_X64 &&
        c->cpu.features_known == 1 && !(c->cpu.features & ~(uint64_t)0x7fu) &&
        c->compiled_backends == PSX_HOST_BACKEND_ENHANCED &&
        c->available_backends == PSX_HOST_BACKEND_ENHANCED && c->fp_contract == 2 &&
        c->interpreted_overlays_sealed == 1 && c->builtin_linked_bios == 1 &&
        c->builtin_bios_image_id && !strcmp(c->builtin_bios_image_id, "OPENBIOS") &&
        c->builtin_bios_image_wordsum == 0xc4daeb88u &&
        c->slot_count == 2 && players == 2 && seats == 3u && !c->rollback &&
        c->renderer == 1 && c->retained_texture_banks == 1 && c->aspect_index == 0 &&
        !c->observer && (c->direct_loopback_scope == 0 || c->direct_loopback_scope == 1) &&
        c->declared_build_manifest_schema == 1 && valid_sha(c->declared_build_manifest_sha256);
}

int mmx4_coop_host_profiles(const PSXModNetplayHostStartup *c, PSXHostProfileCatalog *out) {
    if (!out) return 0;
    out->rules = NULL;
    out->count = 0;
#if MMX4_HOST_COMPILER && defined(PSX_NO_DEBUG_TOOLS) && (!defined(PSX_PGXP) || !PSX_PGXP)
    if (!PSX_EXECUTION_ENHANCED || strcmp(PSX_EXECUTION_ID, MMX4_HOST_EXECUTION) ||
        !release_context(c)) return 0;
    const PSXModNetplayProfile *p = psx_mod_netplay_profile();
    if (!p || !p->plugin_id || strcmp(p->plugin_id, "mmx4.coop") ||
        !p->compatibility_id || !p->state_digest || p->rollback_supported ||
        p->savestates_supported || !p->requires_retained_texture_banks || !p->skip_bios_intro ||
        !p->settings_package_id || strcmp(p->settings_package_id, "mmx4.coop") ||
        !p->settings_feature_id || strcmp(p->settings_feature_id, "coop") ||
        !p->renderer_option_id || strcmp(p->renderer_option_id, "cameras") ||
        !p->renderer_option_value || strcmp(p->renderer_option_value, "split")) return 0;
    static PSXHostProfileRule rule;
    memset(&rule, 0, sizeof rule);
    strcpy(rule.profile, MMX4_HOST_PROFILE);
    rule.revision = 1;
    rule.priority = 1;
    rule.flags = PSX_HOST_PROFILE_PORTABLE;
    rule.implementation_count = 1;
    rule.implementations[0] = (PSXHostImplementation){PSX_HOST_BACKEND_ENHANCED,
        1u << PSX_HOST_ARCH_X64, 0};
    psx_sha256_compute((const uint8_t *)MMX4_HOST_POLICY, strlen(MMX4_HOST_POLICY), rule.qualification);
    fprintf(stderr, "mmx4.host: declared manifest schema=%u sha256=%s direct-loopback=%d\n",
        c->declared_build_manifest_schema, c->declared_build_manifest_sha256, c->direct_loopback_scope);
    out->rules = &rule;
    out->count = 1;
    return 1;
#else
    (void)c;
    return 0;
#endif
}
