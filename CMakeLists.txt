cmake_minimum_required(VERSION 3.20)
project(MegaManX4Recomp C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
    "Path to psxrecomp-v4")

include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")

# The current recompiler emits the game as split translation units. Keep the
# monolithic fallback for older generated/ directories.
file(GLOB GAME_GENERATED_FULL_SHARDS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_005.61_full_*.c")
if(GAME_GENERATED_FULL_SHARDS)
    set(GAME_GENERATED_FULL ${GAME_GENERATED_FULL_SHARDS})
    list(POP_FRONT GAME_GENERATED_FULL GAME_GENERATED_PRIMARY)
    set(GAME_GENERATED_EXTRAS ${GAME_GENERATED_FULL})
    set(GAME_GENERATED_FULL ${GAME_GENERATED_PRIMARY})
else()
    set(GAME_GENERATED_FULL
        "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_005.61_full.c")
endif()
set(GAME_GENERATED_DISPATCH
    "${CMAKE_CURRENT_SOURCE_DIR}/generated/SLUS_005.61_dispatch.c")

file(GLOB MMX4_MOD_PLUGIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.c")

psxrecomp_v4_add_runtime_target(psx-runtime
    GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
    GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
    DEBUG_PORT 4525
    WINDOW_TITLE "MegaManX4Recomp"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    LAUNCHER_BOXART "${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga"
    EXTRAS_SOURCES ${GAME_GENERATED_EXTRAS} ${MMX4_MOD_PLUGIN_SOURCES}
)

# Preload X4's game-owned, default-off mod catalog beside the runtime. Package
# manifests contain metadata and guarded writes only; trusted plugin
# implementations are compiled above.
set(MMX4_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(EXISTS "${MMX4_PRELOADED_MODS}/packages")
    add_custom_command(TARGET psx-runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MMX4_PRELOADED_MODS}"
            "$<TARGET_FILE_DIR:psx-runtime>/mods"
        COMMENT "Preloading the Mega Man X4 mod catalog")
endif()

if(BUILD_TESTING)
    add_executable(mmx4_preloaded_mods_test
        tests/test_preloaded_mods.cpp
        "${PSXRECOMP_V4_ROOT}/runtime/src/mod_packages.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/crc32.c"
        "${PSXRECOMP_V4_ROOT}/runtime/src/psx_sha256.c"
    )
    target_include_directories(mmx4_preloaded_mods_test PRIVATE
        "${PSXRECOMP_V4_ROOT}/runtime/include"
        "${PSXRECOMP_V4_ROOT}/recompiler/lib/toml11"
    )
    add_test(
        NAME mmx4_preloaded_mods_test
        COMMAND "$<TARGET_FILE:mmx4_preloaded_mods_test>"
                "${MMX4_PRELOADED_MODS}"
    )
endif()

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
        DEFAULT_GAME_CONFIG_PATH "game.toml"
        EXTRAS_SOURCES ${GAME_GENERATED_EXTRAS} ${MMX4_MOD_PLUGIN_SOURCES}
        COSIM
    )
endif()
