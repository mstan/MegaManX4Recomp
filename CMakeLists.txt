cmake_minimum_required(VERSION 3.20)
project(MegaManX4Recomp C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
    "Path to psxrecomp-v4")

include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")

set(GAME_GENERATED_FULL
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_005.61_full.c")
set(GAME_GENERATED_DISPATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_005.61_dispatch.c")

psxrecomp_v4_add_runtime_target(psx-runtime
    GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
    GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
    DEBUG_PORT 4525
    WINDOW_TITLE "MegaManX4Recomp"
    DEFAULT_BIOS_PATH "${PSXRECOMP_V4_ROOT}/bios/SCPH1001.BIN"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
)

# Shared recomp-ui Dear ImGui launcher. Wires in the PSX-themed launcher UI
# (see main.cpp RECOMP_LAUNCHER gate) instead of the framework's default RmlUi
# launcher. recomp-ui is a real git submodule (github.com/mstan/recomp-ui,
# pinned via the recomp-ui gitlink); locally it is a directory junction to the
# working checkout so builds stay offline and reproducible like everything else.
include("${CMAKE_SOURCE_DIR}/recomp-ui/recomp_ui.cmake")
recomp_target_launcher_ui(psx-runtime
    BOXART "${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga"
    PAD    "${CMAKE_SOURCE_DIR}/recomp/launcher/pad.tga"
    BRAND  "${CMAKE_SOURCE_DIR}/recomp/launcher/brand.tga"
)

# First-divergence co-sim oracle build (COSIM_ORACLE.md): clean, deterministic,
# PSX_NO_DEBUG_TOOLS + PSX_COSIM. Two of these (one normal, one PSX_FORCE_INTERP=1)
# are cycle-lockstepped by tools/cosim.py to find the exact first compiled-vs-interp
# divergence. Only built on explicit request (-DPSX_BUILD_COSIM=ON) to keep normal
# game builds lean.
if(PSX_BUILD_COSIM)
    psxrecomp_v4_add_runtime_target(psx-cosim
        GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
        GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
        WINDOW_TITLE "MMX4-cosim"
        DEFAULT_BIOS_PATH "${PSXRECOMP_V4_ROOT}/bios/SCPH1001.BIN"
        DEFAULT_GAME_CONFIG_PATH "game.toml"
        COSIM
    )
endif()

# Per-game launcher art override. The framework copies its shared launcher assets
# (img/disc.png etc.) next to the exe; this copies this repo's launcher_art/ on
# top AFTERWARD so the launcher shows MMX4's disc. POST_BUILD commands run in add
# order, and the framework's copy is added inside psxrecomp_v4_add_runtime_target
# above, so this override wins. Drop official art into launcher_art/img/ to replace
# the generated placeholder.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/launcher_art")
    add_custom_command(TARGET psx-runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/launcher_art"
            "$<TARGET_FILE_DIR:psx-runtime>"
        COMMENT "Overriding launcher art with MegaManX4Recomp/launcher_art")
endif()
