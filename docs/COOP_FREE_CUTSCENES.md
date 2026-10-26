# Optional co-op cutscene movement experiment

Enable **Keep moving during co-op cutscenes** alongside either co-op mod,
then start a fresh game process. This is a separate default-off package,
`mmx4.coop-free-cutscenes`; disable it to restore the ordinary carry policy.
The package selection participates in the netplay/session identity and resets
when the committed mod plan changes.

Ordinary native scene commands retain one script owner and one conversation.
The other living players keep their bodies, controls, collisions, weapons and
own Split cameras. Either character may own the scene. Shared stage progress,
movies and actual section loads remain native and common to the team.

Physical boss doors still request their original passenger transport. An
already pending door or room transfer does not become free movement halfway
through its outgoing/incoming animation. Refight portals and genuine room
boundaries keep their carry paths. No dead player is revived by this option.

The native world-freeze gate is bypassed only while a non-owner's controller
or private camera executes. The world and the scene owner's freeze are
restored afterward. Three/four-player mode already retains ordinary extra-seat
input; this addon also lets those heroes navigate a frozen scene. Full-campaign
interactions between a moving partner and another scene remain experimental.

Reproduce the bounded native midboss and door checks with a development build:

```powershell
python tools/test_mmx4_coop_regressions.py --exe build/MegaManX4Recomp.exe --bios build/bios/openbios.bin --disc "path/to/Mega Man X4.cue" --free-cutscenes --local-cameras split --custom-renderer --hidden-window --quick-boot --cases free_cutscenes --output qa-free-scenes
python tools/test_mmx4_coop4_local.py --exe build/MegaManX4Recomp.exe --disc "path/to/Mega Man X4.cue" --players 4 --free-cutscenes --report-dir qa-four-free-scenes
```

The fixtures select a native checkpoint and position the owner before the
midboss. Controller input must trigger the scene, move the partner during its
live script, finish the scene and traverse a physical Jungle boss door.
The probe never writes a scene-completion flag. Its settings/cards are private
and its output is muted.
An explicitly injected frozen-world phase also checks movement while retaining
the live scene and its shared freeze. That fixture is controller-gate evidence,
not a claim that this midboss naturally requests the freeze.
