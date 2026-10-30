cmake_minimum_required(VERSION 3.20)
project(MegaManX4Recomp C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/release/VERSION" MMX4_RELEASE_VERSION LIMIT_COUNT 1)
string(REGEX REPLACE "^v" "" MMX4_RELEASE_VERSION "${MMX4_RELEASE_VERSION}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS packaging/release/VERSION)

# The maintained shared builder selects the same framework for codegen,
# runtime and selective PGO. Retain the legacy path for direct title builds.
if(DEFINED PSXRECOMP_ROOT)
    set(PSXRECOMP_V4_ROOT "${PSXRECOMP_ROOT}")
else()
    set(PSXRECOMP_V4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp-v4" CACHE PATH
        "Path to psxrecomp-v4")
endif()

# This title ships a co-op netplay profile on both platforms. The framework's
# generic runtime defaults netplay off; select it before including that runtime.
option(PSX_NETPLAY "Build X4 netplay and co-op profile support" ON)
option(MMX4_COOP_NETPLAY_DEBUG "Development-only synchronized co-op cheats" OFF)
# The shared single-pass batcher preserves painter order without submitting
# every translucent world tile separately. Keep the conservative build floor
# available through REFERENCE or an explicit -DPSX_GL_BATCH_IMPL=LLE.
if(NOT DEFINED PSX_GL_BATCH_IMPL)
    set(MMX4_DEFAULT_GL_BATCH "HLE")
    if(PSX_EXECUTION_PROFILE STREQUAL "REFERENCE")
        set(MMX4_DEFAULT_GL_BATCH "LLE")
    endif()
    set(PSX_GL_BATCH_IMPL "${MMX4_DEFAULT_GL_BATCH}" CACHE STRING
        "OpenGL textured batching: LLE or HLE")
endif()
include("${PSXRECOMP_V4_ROOT}/runtime/runtime.cmake")
option(MMX4_NATIVE_IDLE_WAIT "Select the audited native PsyQ frame wait" ON)
set(MMX4_NATIVE_IDLE_WAIT_SITES "")
if(MMX4_NATIVE_IDLE_WAIT)
    if(NOT COMMAND psxrecomp_native_idle_wait)
        message(FATAL_ERROR "MMX4_NATIVE_IDLE_WAIT needs native wait-policy support")
    endif()
    set(MMX4_NATIVE_IDLE_WAIT_SITES 0x800E4F20 0x800E4F6C)
endif()

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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/*.cpp")
list(REMOVE_ITEM MMX4_MOD_PLUGIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx4_resident.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/mods/mmx4_resident_reference.c")

# X4's game-owned mod catalog. Package manifests contain metadata
# and guarded writes only; the trusted plugin implementations are compiled in
# above.
#
# The FRAMEWORK stages it: PRELOADED_MODS_DIR below hands the directory to
# psxrecomp_add_runtime_target(), which wipes <exe-dir>/mods/bundled and copies
# both its own mods/builtin/packages and these packages into it, plus
# mods/preloaded/README.md -> mods/README.md. This repo must NOT copy the tree
# itself. It used to, with a hand-written POST_BUILD copy_directory into
# <exe-dir>/mods, and because that named the destination as a STRING, framework
# commit 4cc04be3's rename of the staged catalog from mods/packages to
# mods/bundled broke it silently -- nothing a configure, compile or link step
# can see. The four mmx4.* packages went on landing in mods/packages, which
# neither the launcher nor the release packager's Add-ModCatalog reads.
# Bead beads-eio.3.101.
set(MMX4_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")

psxrecomp_v4_add_runtime_target(psx-runtime
    NATIVE_IDLE_WAIT_EXCLUDE_DEBUG_TURBO
    NATIVE_IDLE_WAIT_SITES ${MMX4_NATIVE_IDLE_WAIT_SITES}
    GAME_GENERATED_FULL_C "${GAME_GENERATED_FULL}"
    GAME_GENERATED_DISPATCH_C "${GAME_GENERATED_DISPATCH}"
    DEBUG_PORT 4525
    WINDOW_TITLE "MegaManX4Recomp"
    GAME_VERSION "${MMX4_RELEASE_VERSION}"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    LAUNCHER_BOXART "${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga"
    PRELOADED_MODS_DIR "${MMX4_PRELOADED_MODS}"
    # Loading policy belongs to this title; generic acceleration is retired.
    EXCLUDE_BUILTIN_MODS
        psx.enhancement.cd-speed
        psx.enhancement.fast-loading
    EXTRAS_SOURCES ${GAME_GENERATED_EXTRAS} ${MMX4_MOD_PLUGIN_SOURCES}
)
psxrecomp_add_implementation(psx-runtime
    NAME resident-loading CONTRACT slus00561-arc-callback-v1
    HLE_SOURCES src/mods/mmx4_resident.cpp
    LLE_SOURCES src/mods/mmx4_resident_reference.c
    CONTRACT_FILES src/mods/mmx4_resident_catalog.h
        "${PSXRECOMP_V4_ROOT}/runtime/include/mod_resident_sector.hpp")
# Trusted co-op callbacks contain simulation state outside guest RAM. Include
# their source contract in the profile handshake so different spike builds
# cannot join under the same fixed development version.
set(MMX4_COOP_CONTRACT_FILES
    src/mods/mmx4_localization.cpp src/mods/mmx4_language_assets.h
    game.toml src/mods/mmx4_coop_plugin.c src/mods/mmx4_coop_lifecycle.c
    src/mods/mmx4_coop_combat.c src/mods/mmx4_coop_assets.c src/mods/mmx4_coop_audio.c
    src/mods/mmx4_coop_internal.h src/mods/mmx4_coop_assets.h
    src/mods/mmx4_coop_memory.c src/mods/mmx4_coop_memory.h
    src/mods/mmx4_coop_texture_cache.c src/mods/mmx4_coop_texture_cache.h
    src/mods/mmx4_coop_snapshot.h
    src/mods/mmx4_story_damage.h
    src/mods/mmx4_cheats.c src/mods/mmx4_cheats.h
    src/mods/mmx4_coop_views.c src/mods/mmx4_coop_views.h
    src/mods/mmx4_coop_split.c src/mods/mmx4_coop_web.c src/mods/mmx4_coop_bosses.c
    src/mods/mmx4_coop_balance.c src/mods/mmx4_coop_assessment.c
    src/mods/mmx4_coop_debug_protocol.c src/mods/mmx4_coop_debug_protocol.h
    src/mods/mmx4_diagnostics.cpp src/mods/mmx4_diagnostics.h src/mods/mmx4_diagnostic_paths.h
    src/mods/mmx4_diagnostic_writer.h src/mods/mmx4_diagnostic_mode.h
    src/mods/mmx4_widescreen_plugin.c
    mods/preloaded/packages/mmx4.coop/0.0.1/manifest.toml
    mods/preloaded/packages/mmx4.complete-story/1.0.0/manifest.toml)
set(MMX4_COOP_CONTRACT "mmx4-coop-delay-v3\n")
set(MMX4_COOP_PROFILE_PREFIX "mmx4-coop-delay-v3")
if(MMX4_COOP_NETPLAY_DEBUG)
    if(NOT PSX_DEBUG_TOOLS)
        message(FATAL_ERROR "MMX4_COOP_NETPLAY_DEBUG requires PSX_DEBUG_TOOLS")
    endif()
    set(MMX4_COOP_PROFILE_PREFIX "mmx4-coop-debug-v1")
    string(APPEND MMX4_COOP_CONTRACT "synchronized-development-fixtures-v1\n")
    target_compile_definitions(psx-runtime PRIVATE MMX4_COOP_NETPLAY_DEBUG=1)
endif()
foreach(MMX4_COOP_SOURCE IN LISTS MMX4_COOP_CONTRACT_FILES)
    file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/${MMX4_COOP_SOURCE}" MMX4_COOP_SOURCE_HASH)
    string(APPEND MMX4_COOP_CONTRACT "${MMX4_COOP_SOURCE_HASH}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${MMX4_COOP_SOURCE}")
endforeach()
set(MMX4_COOP_FRAMEWORK_CONTRACT_FILES
    runtime/src/psx_cycles.c runtime/include/psx_cycle_freeze.h
    runtime/src/render_pass.c runtime/include/mod_plugins.h
    runtime/src/mod_netplay.c runtime/include/mod_netplay.h
    runtime/src/mod_runtime.cpp runtime/include/mod_runtime.h runtime/src/mod_texture_banks.c
    runtime/src/mod_snapshot.c runtime/include/mod_snapshot.h
    runtime/src/boot_state.c runtime/include/boot_state.h
    runtime/src/memory.c runtime/include/mod_memory.h
    runtime/src/host_osd.c runtime/include/host_osd.h
    runtime/src/gpu.c runtime/include/mod_capcom_bg2d.h
    runtime/src/spu.c runtime/include/spu.h runtime/src/mod_guest_services.c)
foreach(MMX4_COOP_SOURCE IN LISTS MMX4_COOP_FRAMEWORK_CONTRACT_FILES)
    set(MMX4_COOP_SOURCE_PATH "${PSXRECOMP_V4_ROOT}/${MMX4_COOP_SOURCE}")
    file(SHA256 "${MMX4_COOP_SOURCE_PATH}" MMX4_COOP_SOURCE_HASH)
    string(APPEND MMX4_COOP_CONTRACT "${MMX4_COOP_SOURCE_HASH}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MMX4_COOP_SOURCE_PATH}")
endforeach()
string(SHA256 MMX4_COOP_CONTRACT_HASH "${MMX4_COOP_CONTRACT}")
# The framework limits compatibility IDs to 63 bytes; 160 hash bits fit.
string(SUBSTRING "${MMX4_COOP_CONTRACT_HASH}" 0 40 MMX4_COOP_CONTRACT_SHORT_HASH)
set_property(SOURCE src/mods/mmx4_coop_plugin.c APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX4_COOP_NETPLAY_COMPATIBILITY=\"${MMX4_COOP_PROFILE_PREFIX}:${MMX4_COOP_CONTRACT_SHORT_HASH}\"")
set_property(SOURCE src/mods/mmx4_diagnostics.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX4_DIAGNOSTIC_BUILD_ID=\"${MMX4_COOP_PROFILE_PREFIX}:${MMX4_COOP_CONTRACT_SHORT_HASH}\""
    "MMX4_DIAGNOSTIC_RELEASE_VERSION=\"${MMX4_RELEASE_VERSION}\"")
set_property(SOURCE src/mods/mmx4_startup_log.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    "MMX4_STARTUP_BUILD_ID=\"${MMX4_COOP_PROFILE_PREFIX}:${MMX4_COOP_CONTRACT_SHORT_HASH}\"")
add_custom_command(TARGET psx-runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/game.toml"
        "$<TARGET_FILE_DIR:psx-runtime>/game.toml"
    VERBATIM)

if(BUILD_TESTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME mmx4_vwait_contract
            COMMAND "${Python3_EXECUTABLE}"
                "${PSXRECOMP_V4_ROOT}/runtime/tests/native_wait_caller.py"
                --framework "${PSXRECOMP_V4_ROOT}" --cc "${CMAKE_C_COMPILER}"
                --generated "${CMAKE_CURRENT_SOURCE_DIR}/generated"
                --contract "${CMAKE_CURRENT_SOURCE_DIR}/tests/vwait_contract.json")
        set_tests_properties(mmx4_vwait_contract PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    endif()
    add_executable(mmx4_localization_assets_test
        tests/test_localization_assets.cpp src/mods/mmx4_localization.cpp
        "${PSXRECOMP_V4_ROOT}/runtime/src/native_text.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/third_party/json11/json11.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/mod_media.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/iso_reader.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/cue_sheet.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/psx_sha256.c")
    target_include_directories(mmx4_localization_assets_test PRIVATE
        src/mods "${PSXRECOMP_V4_ROOT}/runtime/include" "${PSXRECOMP_V4_ROOT}/recompiler/include")
    target_link_libraries(mmx4_localization_assets_test PRIVATE chdr-static)
    add_test(NAME mmx4_localization_assets_test COMMAND mmx4_localization_assets_test)
    add_executable(mmx4_startup_log_test tests/test_mmx4_startup_log.cpp src/mods/mmx4_startup_log.cpp)
    target_compile_definitions(mmx4_startup_log_test PRIVATE MMX4_STARTUP_LOG_TEST=1)
    target_include_directories(mmx4_startup_log_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    target_link_libraries(mmx4_startup_log_test PRIVATE Threads::Threads)
    add_test(NAME mmx4_startup_log_test COMMAND mmx4_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-log-qa")
    add_test(NAME mmx4_startup_log_no_console_test COMMAND mmx4_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-log-no-console-qa" closed)
    add_test(NAME mmx4_startup_log_unavailable_test COMMAND mmx4_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-log-unavailable-qa" unavailable)
    add_test(NAME mmx4_startup_log_pointer_unavailable_test COMMAND mmx4_startup_log_test "${CMAKE_CURRENT_BINARY_DIR}/startup-log-pointer-unavailable-qa" pointer-unavailable)
    if(WIN32)
        foreach(MMX4_STARTUP_LAUNCH detached null invalid tee detached-pointer-unavailable)
            add_test(NAME mmx4_startup_log_${MMX4_STARTUP_LAUNCH}_test COMMAND mmx4_startup_log_test
                "${CMAKE_CURRENT_BINARY_DIR}/startup-log-${MMX4_STARTUP_LAUNCH}-qa" ${MMX4_STARTUP_LAUNCH})
        endforeach()
    endif()
    add_executable(mmx4_coop_memory_test tests/test_mmx4_coop_memory.c src/mods/mmx4_coop_memory.c)
    target_include_directories(mmx4_coop_memory_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_memory_test COMMAND mmx4_coop_memory_test)
    add_executable(mmx4_coop_texture_cache_test tests/test_mmx4_coop_texture_cache.c src/mods/mmx4_coop_texture_cache.c)
    target_include_directories(mmx4_coop_texture_cache_test PRIVATE src/mods)
    add_test(NAME mmx4_coop_texture_cache_test COMMAND mmx4_coop_texture_cache_test)
    add_executable(mmx4_cheats_test tests/test_mmx4_cheats.c src/mods/mmx4_cheats.c)
    target_include_directories(mmx4_cheats_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_cheats_test COMMAND mmx4_cheats_test)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mmx4/SLUS_005.61")
        add_test(NAME mmx4_cheats_native_contract_test COMMAND mmx4_cheats_test
            "${CMAKE_CURRENT_SOURCE_DIR}/mmx4/SLUS_005.61")
    endif()
    add_executable(mmx4_coop_debug_protocol_test tests/test_mmx4_coop_debug_protocol.c
        src/mods/mmx4_coop_debug_protocol.c)
    target_include_directories(mmx4_coop_debug_protocol_test PRIVATE src/mods)
    add_test(NAME mmx4_coop_debug_protocol_test COMMAND mmx4_coop_debug_protocol_test)
    add_executable(mmx4_coop_assets_test tests/test_mmx4_coop_assets.c src/mods/mmx4_coop_assets.c)
    target_include_directories(mmx4_coop_assets_test PRIVATE src/mods)
    add_test(NAME mmx4_coop_assets_test COMMAND mmx4_coop_assets_test)
    # Exercise actual texture-bank sampling, blend order and readbacks, rather
    # than qualifying atlas metadata alone. SDL's imported target carries its
    # vendor dependencies (including static iconv where applicable).
    if(PSX_SDL3 AND PSX_SDL_LIBRARIES)
        find_package(OpenGL QUIET)
        if(TARGET OpenGL::GL)
            include(CheckIPOSupported)
            check_ipo_supported(RESULT MMX4_GL_TEST_IPO LANGUAGES C)
            if(MMX4_GL_TEST_IPO OR NOT WIN32)
                set(MMX4_GL_TEST_SRC "${PSXRECOMP_V4_ROOT}/runtime/src")
                foreach(MMX4_GL_TIER IN ITEMS lle hle)
                    set(MMX4_GL_TARGET mmx4_sprite_atlas_gl_test)
                    if(MMX4_GL_TIER STREQUAL "hle")
                        set(MMX4_GL_TARGET mmx4_sprite_atlas_gl_hle_test)
                    endif()
                    add_executable(${MMX4_GL_TARGET}
                        tests/test_mmx4_sprite_atlas_gl.c
                        "${MMX4_GL_TEST_SRC}/gpu_sw_renderer.c"
                        "${MMX4_GL_TEST_SRC}/frame_interpolation.c"
                        "${MMX4_GL_TEST_SRC}/render_pass_plan.c"
                        "${MMX4_GL_TEST_SRC}/psx_openxr.c"
                        "${MMX4_GL_TEST_SRC}/render_thread.c"
                        "${MMX4_GL_TEST_SRC}/present_thread.c"
                        "${MMX4_GL_TEST_SRC}/gpu_render.c"
                        "${MMX4_GL_TEST_SRC}/frame_gen.c")
                    target_include_directories(${MMX4_GL_TARGET} PRIVATE
                        "${PSXRECOMP_V4_ROOT}/runtime/include" "${MMX4_GL_TEST_SRC}"
                        "${PSXRECOMP_V4_ROOT}/runtime/tests")
                    target_compile_definitions(${MMX4_GL_TARGET} PRIVATE
                        PSX_SDL3=1 PSX_NO_DEBUG_TOOLS=1 GL_SILENCE_DEPRECATION=1
                        "PSX_GL_BATCH_HLE=$<STREQUAL:${MMX4_GL_TIER},hle>")
                    target_link_libraries(${MMX4_GL_TARGET} PRIVATE
                        ${PSX_SDL_LIBRARIES} OpenGL::GL)
                    set_property(TARGET ${MMX4_GL_TARGET} PROPERTY
                        INTERPROCEDURAL_OPTIMIZATION ${MMX4_GL_TEST_IPO})
                    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
                        target_compile_options(${MMX4_GL_TARGET} PRIVATE
                            -ffunction-sections -fdata-sections)
                        if(NOT APPLE)
                            target_link_options(${MMX4_GL_TARGET} PRIVATE -Wl,--gc-sections)
                        endif()
                    endif()
                    if(NOT WIN32)
                        target_link_libraries(${MMX4_GL_TARGET} PRIVATE m)
                    endif()
                    add_test(NAME ${MMX4_GL_TARGET} COMMAND ${MMX4_GL_TARGET})
                    set_tests_properties(${MMX4_GL_TARGET} PROPERTIES
                        SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 120)
                endforeach()
                find_package(Python3 COMPONENTS Interpreter QUIET)
                if(Python3_Interpreter_FOUND)
                    add_test(NAME mmx4_gl_tiers_test
                        COMMAND "${Python3_EXECUTABLE}"
                            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx4_gl_tiers.py"
                            "$<TARGET_FILE:mmx4_sprite_atlas_gl_test>"
                            "$<TARGET_FILE:mmx4_sprite_atlas_gl_hle_test>")
                    set_tests_properties(mmx4_gl_tiers_test PROPERTIES
                        # This aggregate runs both renderers repeatedly. The
                        # per-process 90s guard remains; headless software GL
                        # can need more than four minutes for the full matrix.
                        SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600)
                endif()
            endif()
        endif()
    endif()
    add_executable(mmx4_coop_audio_test tests/test_mmx4_coop_audio.c
        src/mods/mmx4_coop_audio.c src/mods/mmx4_coop_assets.c)
    target_include_directories(mmx4_coop_audio_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_audio_test COMMAND mmx4_coop_audio_test)
    add_executable(mmx4_coop_combat_test tests/test_mmx4_coop_combat.c
        src/mods/mmx4_coop_combat.c src/mods/mmx4_coop_views.c)
    target_include_directories(mmx4_coop_combat_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_combat_test COMMAND mmx4_coop_combat_test)
    add_executable(mmx4_coop_lifecycle_test tests/test_mmx4_coop_lifecycle.c)
    target_include_directories(mmx4_coop_lifecycle_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_lifecycle_test COMMAND mmx4_coop_lifecycle_test)
    add_executable(mmx4_diagnostics_test tests/test_mmx4_diagnostics.cpp)
    target_include_directories(mmx4_diagnostics_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    target_link_libraries(mmx4_diagnostics_test PRIVATE Threads::Threads)
    add_test(NAME mmx4_diagnostics_test COMMAND mmx4_diagnostics_test "${CMAKE_CURRENT_BINARY_DIR}/diagnostics-qa")
    add_executable(mmx4_diagnostic_writer_test tests/test_mmx4_diagnostic_writer.cpp)
    target_include_directories(mmx4_diagnostic_writer_test PRIVATE src/mods)
    target_link_libraries(mmx4_diagnostic_writer_test PRIVATE Threads::Threads)
    add_test(NAME mmx4_diagnostic_writer_test COMMAND mmx4_diagnostic_writer_test
        "${CMAKE_CURRENT_BINARY_DIR}/diagnostic-writer-qa")
    set_tests_properties(mmx4_diagnostic_writer_test PROPERTIES TIMEOUT 15)
    add_executable(mmx4_coop_views_test tests/test_mmx4_coop_views.c src/mods/mmx4_coop_views.c)
    target_include_directories(mmx4_coop_views_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_views_test COMMAND mmx4_coop_views_test)
    add_executable(mmx4_coop_split_test tests/test_mmx4_coop_split.c
        src/mods/mmx4_coop_split.c src/mods/mmx4_coop_views.c src/mods/mmx4_coop_memory.c)
    target_include_directories(mmx4_coop_split_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_split_test COMMAND mmx4_coop_split_test)
    add_executable(mmx4_coop_web_test tests/test_mmx4_coop_web.c src/mods/mmx4_coop_web.c)
    target_include_directories(mmx4_coop_web_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_web_test COMMAND mmx4_coop_web_test)
    add_executable(mmx4_coop_bosses_test tests/test_mmx4_coop_bosses.c src/mods/mmx4_coop_bosses.c src/mods/mmx4_coop_assets.c)
    target_include_directories(mmx4_coop_bosses_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_bosses_test COMMAND mmx4_coop_bosses_test)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mmx4/SLUS_005.61")
        add_test(NAME mmx4_story_native_contract_test COMMAND mmx4_coop_bosses_test
            "${CMAKE_CURRENT_SOURCE_DIR}/mmx4/SLUS_005.61")
    endif()
    add_executable(mmx4_coop_balance_test tests/test_mmx4_coop_balance.c src/mods/mmx4_coop_balance.c)
    target_include_directories(mmx4_coop_balance_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_balance_test COMMAND mmx4_coop_balance_test)
    add_executable(mmx4_coop_assessment_test tests/test_mmx4_coop_assessment.c
        src/mods/mmx4_coop_assessment.c src/mods/mmx4_coop_memory.c)
    target_include_directories(mmx4_coop_assessment_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_assessment_test COMMAND mmx4_coop_assessment_test)
    add_executable(mmx4_coop_background_test tests/test_mmx4_coop_background.c src/mods/mmx4_coop_views.c)
    target_include_directories(mmx4_coop_background_test PRIVATE src/mods "${PSXRECOMP_V4_ROOT}/runtime/include")
    add_test(NAME mmx4_coop_background_test COMMAND mmx4_coop_background_test)
    find_package(Python3 3.11 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME mmx4_fmv_import_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_fmv_import.py")
        add_test(NAME mmx4_translation_mod_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_translation_mod.py")
        add_test(NAME mmx4_diagnostic_replay_prepare_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_replay_mmx4_diagnostics.py")
        add_test(NAME mmx4_release_config_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_release_config.py")
        add_test(NAME mmx4_playtest_save_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_prepare_mmx4_playtest_save.py")
        add_test(NAME mmx4_coop_hook_registration_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx4_coop_hook_registration.py")
        add_test(NAME mmx4_coop_netplay_harness_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx4_coop_netplay_harness.py")
        add_test(NAME mmx4_coop_netplay_runtime_test
            COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mmx4_netplay_runtime.py")
    endif()
    add_executable(mmx4_preloaded_mods_test
        tests/test_preloaded_mods.cpp
        "${PSXRECOMP_V4_ROOT}/runtime/src/mod_packages.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/mod_media.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/iso_reader.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/disc_path.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/cue_sheet.cpp"
        "${PSXRECOMP_V4_ROOT}/runtime/src/crc32.c"
        "${PSXRECOMP_V4_ROOT}/runtime/src/psx_sha256.c"
    )
    target_include_directories(mmx4_preloaded_mods_test PRIVATE
        "${PSXRECOMP_V4_ROOT}/runtime/include"
        "${PSXRECOMP_V4_ROOT}/recompiler/include"
        "${PSXRECOMP_V4_ROOT}/recompiler/lib/toml11"
    )
    psxrecomp_sha256_file(mmx4_preloaded_mods_test)
    target_link_libraries(mmx4_preloaded_mods_test PRIVATE chdr-static)
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
    psxrecomp_add_implementation(psx-cosim
        NAME resident-loading CONTRACT slus00561-arc-callback-v1
        HLE_SOURCES src/mods/mmx4_resident.cpp
        LLE_SOURCES src/mods/mmx4_resident_reference.c
        CONTRACT_FILES src/mods/mmx4_resident_catalog.h
            "${PSXRECOMP_V4_ROOT}/runtime/include/mod_resident_sector.hpp")
endif()
