# Preloaded Mega Man X4 mods

These packages are copied beside the runtime and appear on the launcher's Mods
page. The catalog owns MMX4's game-specific display enhancements as well as a
testing cheat for reproducing late-game bug reports without repeated full
playthroughs.

The Languages group includes one Japanese FMVs mod with a Subtitles dropdown:
No subtitles, English (acediez/Hondoori), or Español (El Retro Dan).
Only metadata and full upstream credits are bundled;
import the owner's supplied patch locally and select its verified movie file.
See `docs/FMV_SUBTITLES.md` and each package's `CREDITS.md` and upstream readmes.

Widescreen and resident asset loading are enabled by default. The damage
override remains at one (original damage). Interpolation is archived and
unavailable. These packages target the verified USA
SLUS-00561 MODE2/2352 data-track BIN documented in `DISC.md`.
Package archives contain metadata and guarded declarative writes only; trusted
native implementations are compiled into MegaManX4Recomp.

Generic host pacing and CD Speed are excluded. The old title-named Fast Loading
wrapper is parked under `mods/development`; it was not a game-specific loader.
A verified title loading implementation remains campaign work.

Ultimate Armor X Code and Black Zero are separate optional mods, disabled by
default. Both may be enabled together for local play. The X code retains the
original capsule requirement; Black Zero uses the original black palette.
The same codes are also options under X + Zero Co-op for local and online play.
Select the co-op feature to configure its codes, boss doors and boss settings.
For online co-op, the host chooses these settings for both peers.

Keep moving during co-op cutscenes is a separate experimental checkbox, off by
default. With either co-op mod it keeps the other players controllable during
ordinary scenes, while preserving doors and actual room transfers. See
`docs/COOP_FREE_CUTSCENES.md` for scope and native validation.
