# Portable generated-default adoption and rebuild

This source contribution originally started from game commit
`a3c75e2b5c00e7283cd4de771b074cd73c037011` (the annotated `v1.0.2` tag resolves
to that commit). No release, package build, game run or publication is claimed.

## Framework source

The October 24 conflict resolution originally merged game master
`8032596` (the 1.0.6 release) into this proposal. The framework gitlink is
`5d5238ba0a686b4f347d41876e8e57da7a05408e`, a merge of the current game's
`a251a98616f0c8f9e45c90c7ca71d20da71b8b21` with the accepted portable-default
backport `4c08013a0f4277a73583f41ce77278e15f454b92` (#747).
Both parents are ancestors: original-disc content/geometry checks and conditional
FMV resource selection are preserved alongside the generated-default repair.
The two textual framework conflicts retain both adjacent manager methods and
both runtime regression blocks. The combined resolver retains the condition
filter and passes the generated fallback into effective resource resolution.

This integration commit is retained on
[boburning/psxrecomp:feature/mmx4-portable-defaults-integration](https://github.com/boburning/psxrecomp/tree/feature/mmx4-portable-defaults-integration).
It is a contributor integration, not a newly accepted upstream framework merge.
A fresh empty Git repository successfully fetched its exact SHA through the
unchanged declared URL `https://github.com/mstan/psxrecomp.git` on October 24.
`recomp-ui` remains `b964eabe5a3438b8ed121064354ba27c1accef3f`.
No disc data, generated game C or built game output is included.

The earlier accepted backport `4c08013a` had tree
`4f9a1ad91042f309be23f68de66bbffe371ade4c`, matching the reviewed/tested
`8c65e235a1473f01410da18a1bd872800289a925`. Those historical byte-equality
claims apply to that backport, not the new integration tree.

The repair registers generated texture directories in the existing ephemeral
manager map rooted at the current mods directory. The launcher getter and
committed resolver use the same fallback. Explicit selections, including missing
absolute external paths, continue to win. No legacy absolute-path migration is
included. Do not delete `mods/state.toml` or preserve the executable mods tree to
hide the previous default-persistence failure.

## Focused checks

October 24 source validation:

- All seven `python tests/test_release_config.py` checks passed.
- A fresh Release CMake build of the existing `mod_packages_test` and
  `mod_runtime_test` targets passed with two compile jobs and the game's exact
  recomp-ui header (`RECOMP_LAUNCHER=1`). Both test executables passed, including
  retained conditional-picker and portable relocation assertions. The runtime
  executable also passed all four registered netplay argument variants:
  `--missing-profile`, `--missing-renderer`, `--profile-options` and
  `--option-renderer-missing`.
- This focused framework configuration disables netplay, rewind, setup wizard
  and Vulkan and allows absent BIOS; it builds test targets, not a game artifact.
  The first configure refused the absent rewind backend; the corrected configure
  explicitly disabled rewind. CTest was unavailable in PATH, so the six exact
  registered executable/argument combinations were run directly.
- Whitespace checks passed. Earlier framework file-identity and native game
  default-persistence failures retain their historical scope; these passing
  focused tests do not reclassify them or qualify runtime relocation in a game.

## Current-master refresh (October 25)

The follow-up merges game master `d68a03a4d01f1f1901dd77d48efe2a2990980ef2`
(the v1.0.7 integration) without changing the reviewed framework integration
`5d5238ba0a686b4f347d41876e8e57da7a05408e` or recomp-ui pin. All current game,
co-op, packaging and published release-note changes are preserved. The earlier
speculative version bump is removed from the contribution: both version files
and RELEASE_NOTES.md now equal current master. This fix is not claimed to be
in the already published v1.0.7, whose source still pins the unrepaired a251a986.
Release numbering and publication remain the maintainer's normal process.
All seven existing release-configuration tests pass on this combined tree;
the retained framework test results above apply to its unchanged exact pin.
No rebuilt game artifact or native journey is inferred from either result.

## Produce two genuine game artifacts

Use the next two distinct maintainer-selected ordinary Windows releases that
actually incorporate this framework integration. No version label is reserved
by this contribution. Retain each exact source/tag/asset identity and verify
incorporation before qualification.

1. Make the framework source available at its declared upstream and verify the
   exact game gitlink. Initialize the existing framework and recomp-ui
   submodules in a new private checkout. Use the supported owned USA
   `SLUS-00561` source from `DISC.md`; keep all disc material and generated outputs
   untracked. Regenerate through the normal recompiler instead of hand-editing
   `generated/`. Build the game target `psx-runtime` with at most two compile jobs.
2. Use a fresh build/runtime output directory. Configure Release with
   `PSX_DEBUG_TOOLS=OFF`, `MMX4_COOP_NETPLAY_DEBUG=OFF`, `PSX_STATIC_RUNTIME=ON`
   and `PSX_EXECUTION_PROFILE=ENHANCED`, with both framework-root CMake variables
   pointing at this exact checkout. Preserve the existing BIOS/recompiler setup.
   Both version files remain equal to master; the maintainer must set the actual
   release version. CMake reads
   `packaging/release/VERSION`. A rebuilt executable/version stamp must match it.
3. Stage with the existing non-destructive packager, supplying the actual fresh
   build and binary directories:

   ```text
   python tools/package_coop_playtest.py --build-dir BUILD_A --binary-dir BIN_A --output-dir release-portable-FIRST
   ```

   It requires a new output directory, production CMake settings, the exact
   version stamp and current mod catalog. It applies the existing framework
   executable/manifest, license, system-DLL, toolchain and private-data gates.
   Expected artifact: the actual first versioned Windows ZIP, with its generated
   `SHA256SUMS.txt` and `package-report.json`. Retain source/tree/framework pin,
   CMake cache, complete build log, executable SHA-256, execution manifest and
   package SHA-256. A version stamp alone does not authenticate the executable.
4. For the following distinct ordinary source, advance the normal version files,
   keep the repaired framework, regenerate/rebuild in different fresh directories
   and use a different fresh package output. Expected second artifact:
   the actual second versioned Windows ZIP. Do not rezip an old executable,
   rename framework test binaries or reuse the first package as a second artifact.
5. The maintainer's normal release publication must bind each actual tag/source
   and complete asset inventory/digest. Then verify the downloaded bytes and
   runtime's source identity independently. Local package hashes are observations,
   not authority for an arbitrary upstream download.

The historical public `v1.0.0` Windows asset was `663658987`, release `420007858`,
36,291,201 bytes, provider SHA-256
`2d6863b02a0baf32900b257d65b4607b580e0f9cb17e2964b3217582e541b68c`.
Its source still uses the unfixed framework. Public v1.0.1, v1.0.2 and v1.0.3 sources also retain the same unpatched framework00f. The occupied v1.0.3 release421284185 points to source160818ef7b77d5e349e9f86f5429dfcf5b2657ca and asset667335830; it does not incorporate this repair. Public v1.0.4 is now occupied too: release422093155, source458f5bfbecec78ce02cfb5d9325952f691a19a62 and Windows asset669763307 (35,997,076bytes, provider SHA25643532dabe39e1015b21dbc0d0e397af147a67f850611b1e4bed05b6464970519). Its framework pin91fc45d4151eb3cbe20b3773a6c7a55cb8423e10 still lacks the generated-default API and registers an absolute texture-pack directory through set_feature_resource_path. It does not incorporate this repair; no package execution is claimed. Historical exact tag reads for v1.0.5 and v1.0.6 returned404 on October23; those labels have since been used upstream. The October24 tag absence was historical: v1.0.7 is now published and still pins a251a986. No occupied release label is reused for the repaired pair. This is provider/source inspection
only: that package was not downloaded or run in this preparation. The earlier
real `.35`/`.36` full-default-persistence failures remain failures; `.42`/`.43`
also retain their unrepaired-source scope.

After two real fixed artifacts exist, test ordinary materialization and launcher
behavior from installation A to B with A unavailable, including the feature
disabled, an enabled committed HD resource, changed cwd and an unavailable
explicit player-selected external path. Retain the complete legitimate settings,
input/cards/mod state across actual Core install/update/rollback and normal
collection; preserve executable/cache/toolchain separation. These observations
remain distinct from gameplay, audio, controller and in-game save/load coverage.
No Portcove compiled-catalog entry or protected acceptance follows from this
source contribution.
