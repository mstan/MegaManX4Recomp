# Portable generated-default adoption and rebuild

This is a source contribution prepared from game commit
`a3c75e2b5c00e7283cd4de771b074cd73c037011` (the annotated `v1.0.2` tag resolves
to that commit). No release, package build, game run or publication is claimed.

## Framework source

The game's `psxrecomp-v4` pin advances from
`00f484fee6ace3f7f73420f0d60b0235d23af176` to the accepted commit
`4c08013a0f4277a73583f41ce77278e15f454b92`, merged through
RetroPortingToolKit/psxrecomp#747 into the consumed framework branch. Its tree
`4f9a1ad91042f309be23f68de66bbffe371ade4c` exactly matches the reviewed and tested
backport `8c65e235a1473f01410da18a1bd872800289a925`. It is a conflict-free cherry-pick of
`572558394b5b737690138f8374c80b032c3d84ba`, the five-file portable-default repair
merged in RetroPortingToolKit/psxrecomp#685 as
`ac71aec441b9fa6382e687dd412fc228d9ef7f3f`. Both diffs have stable patch ID
`132bcc399628b3368dec7c612d7fb29eb918584d`.

This preserves the complete consumed framework lineage. Replacing it wholesale
with `ac71aec` would also change unrelated framework behavior: the two histories
diverge at `68425a4add89acd0a6ca325bf90e3f8a69d6377f`. `recomp-ui` remains pinned
to `b964eabe5a3438b8ed121064354ba27c1accef3f`. No generated game C, disc data,
BIOS profile, renderer, netplay policy, explicit player path or game configuration
is changed here.

The declared submodule URL `https://github.com/mstan/psxrecomp.git` resolves to
`RetroPortingToolKit/psxrecomp`. Its maintained branch
`feat/native-font-localization-20261020` now exposes the accepted `4c08013a`
through that declared source. This preserves the consumed branch rather than
replacing it with framework `master`, where #685 was already merged.

The game submission target is `mstan/MegaManX4Recomp:master` (currently exact
`a3c75e2b5c00e7283cd4de771b074cd73c037011`), from planned
`boburning/MegaManX4Recomp:feature/portable-generated-default-adoption`. That
source proposal uses that exact reachable framework commit in its gitlink. No
alternate contributor submodule URL is introduced. Source equality with the
tested backport was checked after the upstream squash; the commit identity is
the accepted `4c08013a`, not the contributor's earlier `8c65e235`.

The repair registers generated texture directories in the existing ephemeral
manager map rooted at the current mods directory. The launcher getter and
committed resolver use the same fallback. Explicit selections, including missing
absolute external paths, continue to win. No legacy absolute-path migration is
included. Do not delete `mods/state.toml` or preserve the executable mods tree to
hide the previous default-persistence failure.

## Focused checks

- Review `git -C psxrecomp-v4 diff 00f484f..4c08013`: exactly the original header,
  package manager, runtime registration and two framework test files.
- Run `python tests/test_release_config.py` against this game source. This checks
  the existing title/configuration contracts, not runtime relocation.
- In a disposable resource-admitted framework build, compile and run the existing
  `mod_packages_test` and `mod_runtime_test` targets. The repair's disabled-default
  relocation, launcher/resolver, cwd, explicit selection, containment and
  shared-source refusal assertions must remain intact. Earlier native framework
  checks at other source pins do not qualify this new integration. The previously
  observed same-size/restored-mtime file-identity failure remains recorded; do not
  claim the complete runtime suite passes without its actual result.

## Produce two genuine game artifacts

The concrete proposed next pair is Windows `v1.0.3` and the following ordinary
`v1.0.4` release, each rebuilt from its own exact game source and the fixed
framework. These labels are prepared release inputs, not existing releases or
publisher authorization. A maintainer may select the actual next ordinary
versions, but must retain two different exact source/tag/asset identities.

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
   Both version files now propose `v1.0.3`; CMake reads
   `packaging/release/VERSION`. A rebuilt executable/version stamp must match it.
3. Stage with the existing non-destructive packager, supplying the actual fresh
   build and binary directories:

   ```text
   python tools/package_coop_playtest.py --build-dir BUILD_A --binary-dir BIN_A --output-dir release-portable-1.0.3
   ```

   It requires a new output directory, production CMake settings, the exact
   version stamp and current mod catalog. It applies the existing framework
   executable/manifest, license, system-DLL, toolchain and private-data gates.
   Expected artifact: `MegaManX4Recomp-v1.0.3-windows-x64.zip`, with its generated
   `SHA256SUMS.txt` and `package-report.json`. Retain source/tree/framework pin,
   CMake cache, complete build log, executable SHA-256, execution manifest and
   package SHA-256. A version stamp alone does not authenticate the executable.
4. For the following ordinary `v1.0.4` source, advance the normal version files,
   keep the repaired framework, regenerate/rebuild in different fresh directories
   and use a different fresh package output. Expected second artifact:
   `MegaManX4Recomp-v1.0.4-windows-x64.zip`. Do not rezip an old executable,
   rename framework test binaries or reuse the first package as a second artifact.
5. The maintainer's normal release publication must bind each actual tag/source
   and complete asset inventory/digest. Then verify the downloaded bytes and
   runtime's source identity independently. Local package hashes are observations,
   not authority for an arbitrary upstream download.

The historical public `v1.0.0` Windows asset was `663658987`, release `420007858`,
36,291,201 bytes, provider SHA-256
`2d6863b02a0baf32900b257d65b4607b580e0f9cb17e2964b3217582e541b68c`.
Its source still uses the unfixed framework. New public v1.0.1 and v1.0.2 sources also retain the same unpatched framework00f. Their occupied release labels are not reused for the proposed repaired pair. This is provider/source inspection
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
