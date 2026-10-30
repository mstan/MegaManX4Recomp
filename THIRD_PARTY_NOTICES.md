# Third-party notices

The root [MIT license](LICENSE) covers this project's original code and
documentation. It does not relicense submodules, third-party code, Mega Man X4,
recompiled game code or the game imagery in screenshots.

| Component | License / attribution |
| --- | --- |
| PSXRecomp, the pinned runtime and recompiler | [PolyForm Noncommercial 1.0.0](psxrecomp-v4/LICENSE) |
| recomp-ui | [MIT](recomp-ui/LICENSE) |
| OpenBIOS and its bundled components | [OpenBIOS notice](psxrecomp-v4/bios/OpenBIOS.LICENSE) |
| Runtime libraries and bundled toolchain | [Framework attribution](psxrecomp-v4/THIRD_PARTY_ATTRIBUTION.md) and [individual notices](psxrecomp-v4/runtime/licenses) |
| Launcher fonts and images | [Font notices](recomp-ui/assets/common/fonts/NOTICE.md), [image notices](recomp-ui/assets/common/img/NOTICE.md) |
| Dear ImGui | [MIT](recomp-ui/src/third_party/imgui/LICENSE.txt) |
| Shared netplay library | [recomp-net license](psxrecomp-v4/lib/recomp-net/LICENSE) |
| Fan-subtitled Japanese movies | acediez/Hondoori (English), El Retro Dan (Spanish); [complete roles, source links and preserved readmes](docs/fmv-subtitles/CREDITS.md). Audiovisual data is imported locally, not bundled. |

Future release packaging stages the project license alongside
`bios/OpenBIOS.LICENSE`, `licenses/` and the launcher asset notices. The
repository license change after Playtest 13 does not replace the license files
in previously published archives.

Mega Man X4, its characters, art, music and game data are copyright Capcom.
The README screenshots show the actual game for documenting this project;
their game imagery is not covered by the project's MIT license.

The optional fan-movie metadata preserves upstream contributor credits and
per-movie provenance. Neither this repository's MIT license nor its native
adapter grants a license to the fan subtitles or underlying audiovisual work.
