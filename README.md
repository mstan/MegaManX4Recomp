> [!NOTE]
> A special thank you to Thomu, Moro, and JMOTZU for their extensive playtesting in the Co-op mod. Also, a special thank you to Thomu for his generous support towards the project's development.

[Read more about it here](https://1379.tech/mega-man-x4-recompiled-x-zero-coop-preview/)

# Mega Man X4 Recompiled

[Fan-subtitled Japanese FMVs](docs/FMV_SUBTITLES.md): one optional Japanese FMVs mod with a subtitle dropdown
from acediez/Hondoori (English) and El Retro Dan (Spanish), with
[full attribution and preserved upstream readmes](docs/fmv-subtitles/CREDITS.md).

**X and Zero. Together.**

Mega Man X4 on Windows and Linux. **1.0.21 Stable Public Beta**, with two-player co-op.
Bring a friend through X's or Zero's campaign, on one computer or over netplay.

**[Downloads](https://github.com/mstan/MegaManX4Recomp/releases)** · [What's new in 1.0.21](RELEASE_NOTES.md) · [Known issues](docs/COOP_PLAYTEST_FEEDBACK_20261010.md)

<p align="center">
  <img src="docs/images/coop-jungle.png" width="640" alt="X and Zero together at the Jungle waterfall, with vertically stacked lifebars">
</p>

## Two heroes, one campaign

Play X and Zero simultaneously, with their own attacks, voices and effects.
Player 1 owns the story, boss clears and permanent progress; Player 2 joins
as the other hero. Arrive together, tackle bosses together, and share team retries.
The hero who lands a boss's final hit gets their victory theme.

| Make it your co-op run | Choices |
| --- | --- |
| Play together | Local co-op with two controllers or separate keyboard bindings; two-player netplay from the launcher. |
| Pick your view | **Unified** camera by default. **Split** gives each netplay player an independent screen. Local co-op also offers **side by side 8:3**, with two 4:3 views in one window. |
| Arrange your HUD | Stacked lifebars by default, or side by side. Independent screens show their own player's meter. |
| Expand stage select | The experimental **Co-op Stage Select Cast** mod adds both heroes and their story companions. Off by default; P1's characters stay in front on both screens. |
| Manage your supplies | Either player opens both heroes' own pause menus. Choose weapons and manage supplies independently; either player resumes both. E-tank energy is separate while independent menus are enabled, and healing blocks unpause while your partner can keep navigating. |
| Set the door rule | Either player opens boss doors by default. Optionally wait for both living, present players. |
| Fight both story bosses | Opt into Double **and** Iris in sequence; return to the inner door for the second fight. |
| Turn up the challenge | Optional double effective boss health and shorter hit cooldowns, with separate timers for each player or one shared timer. Scaling also reduces Zero's boss damage by 20%. |
| Enable the secret codes | Ultimate Armor X's code and Black Zero can be enabled together. X still needs a capsule for the armor. |

The extra boss encounters, scaling and character codes are **off by default**.
The netplay host chooses the shared options.

<p align="center">
  <img src="docs/images/split-x.png" width="48%" alt="Player 1's independent view follows X in a lower Jungle passage">
  <img src="docs/images/split-zero.png" width="48%" alt="Player 2's independent view follows Zero near a different waterfall">
  <br><em>One shared world, two independent netplay views.</em>
</p>

<p align="center">
  <img src="docs/images/double.png" width="48%" alt="X and Zero face Double">
  <img src="docs/images/iris.png" width="48%" alt="X and Zero face Iris in the optional second encounter">
  <br><em>Double and Iris in one co-op run, when you enable the option.</em>
</p>

## Start playing

1. Download the **Windows x64 ZIP** and extract it, or use the **Linux x86_64 AppImage** (glibc 2.38 or newer).
2. Select your own **original, unmodified Mega Man X4 USA (SLUS-00561)** disc image. Choose the `.cue` from a `.cue` + `.bin` set. OpenBIOS is included; a separate BIOS dump is optional.
3. For **local co-op**, enable **Mods → X + Zero Co-op**, assign Player 1 and Player 2 in **Controls**, and choose **Local co-op display** for Unified or side by side 8:3, then press **Play**. For **netplay**, press **Netplay** and host or join using **X + Zero Co-op**. Both peers need the same release.

Only the [verified original USA disc](DISC.md) is accepted, including when every
mod is disabled. All changes belong in **Mods**. Patched discs, undubs and
retranslated disc images are unsupported; use the corresponding mods over the
original disc.

Co-op requires OpenGL and takes effect on a fresh game boot. Single-player remains available.

With side by side 8:3 or Split netplay, each player sees their own character's
weapon or technique award, including its artwork and instructions. P1 advances
the shared award and Save/Continue flow; both heroes share the campaign's
unlocks. Unified uses P1's award screen.

Fast loading is built in. The old **CD Speed** and **Fast Loading** mods are
retired; the game now keeps verified disc data in memory and feeds it through
the original loading routines. Music and cutscenes retain their normal timing.

<p align="center">
  <img src="docs/images/netplay.png" width="720" alt="The launcher offers LAN / Direct IP and Online Netplay">
</p>

Offline co-op also supports **save states (F7)** and **rewind (F8)** when enabled.
The Mods page includes widescreen, character codes and optional input display
and diagnostic recording, **off by default**. [Controls, saves and reporting guide →](docs/PLAYTESTING.md)

Optional language mods offer **Spanish** and **Brazilian Portuguese** dialogue
using the game's own font. Supply your Japanese Rockman X4 disc to choose
Japanese movies, voices and the title screen independently. Translations are
drafts awaiting native-speaker review. [Language setup and scope →](docs/LOCALIZATION.md)

**Complete Story** is an optional local solo/co-op mod, off by default. Add
Zero's playable Memorial Hall duel, fight both Double and Iris, and watch the
additional Colonel, Iris and Zero/Sigma movies in your campaign.
[Complete Story setup and scope →](docs/COMPLETE_STORY.md)

<details>
<summary>See the co-op settings</summary>
<p><img src="docs/images/coop-options.png" width="800" alt="Co-op settings in the shipped launcher"></p>
</details>

## Help improve the public beta

Expect bugs; independent cameras can still slow down. Online sign-in/startup
reports remain under investigation.
[Check the burndown](docs/COOP_PLAYTEST_FEEDBACK_20261010.md) or
[report a bug](https://github.com/mstan/MegaManX4Recomp/issues).

To record a bug, enable **Playtest Diagnostics → Input Display and Recording**
before local play. For netplay, the host enables **X + Zero Co-op → Netplay
diagnostic recording** for both peers. Both recording options default off.
Reproduce the problem, quit normally,
and zip `saves/diagnostics` beside the executable or AppImage. Include a video
and the release version; for netplay, collect both computers' logs. These
recordings include starting game saves. [Details →](docs/PLAYTESTING.md#send-a-useful-bug-report)

---

Built with [PSXRecomp](https://github.com/mstan/psxrecomp) and
[recomp-ui](https://github.com/mstan/recomp-ui).
[Build and contribute](docs/DEVELOPMENT.md) · [R.A.I.D. community](https://discord.gg/Ad9BwSzctP)

Original project code is [MIT licensed](LICENSE). Dependencies retain their
own [licenses and notices](THIRD_PARTY_NOTICES.md). Mega Man X4 and the game
imagery belong to Capcom. You supply the game disc; this is an unofficial project.

## Building player releases

Run the maintained shared command from this repository root:

```powershell
psxrecomp-v4/tools/package_title_release.ps1 --channel public --platform windows --jobs 2
```

Use `--channel playtest` for diagnostics enabled by default, and `--platform linux`
for an AppImage (WSL on Windows). `--platform all` prepares both platforms. The
version comes from `packaging/release/VERSION`; artifacts and checksums go to
`dist/`. `--work-dir` and `--out-dir` select owned build and artifact staging.
The title wrappers accept these shared GNU-style arguments; old title-specific
build/version/cache switches are replaced by the shared interface.

`packaging/release.toml` carries this title's payload. The shared builder owns
regeneration, the fresh per-title selective PGO build/training phases, native
OpenBIOS, launcher assets, notices, channel defaults and final archive checks.
No old profile or private cache is reused. The original-disc inventory found no
separate executable overlay. The resident boot program is regenerated; eligible
uncovered code uses the bundled overlay toolchain. No private or unqualified
cache is shipped.

The command does not publish or upload. Build/package success does not replace
the release's gameplay or owner acceptance checks. See the framework's
`docs/RELEASING_A_TITLE.md` for the shared command and verification contract.
