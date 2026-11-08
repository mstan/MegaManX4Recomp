# Local keyboard and controller ownership

The public 1.0.27 build merged each player's keyboard map alongside an assigned
controller. All slots start with the same default keyboard map, so assigning
Keyboard to one player and a controller to the other let arrows and face-button
keys control both heroes.

Version 1.0.28 reserves the actual bound keyboard/mouse sources of active local
Keyboard slots for those slots. A controller's non-conflicting keyboard/mouse
bindings continue to work. Buttons, analog directions and controller-mode
policy queries use the same source filtering. Saved bindings are unchanged.
The rule applies in either player order. Two Keyboard players still need
distinct bindings because they share one physical keyboard.

Netplay captures one local human device per peer; unused offline controller
cards do not reserve keyboard sources there. Explicit development input sharing
remains shared. Simulation input, native game logic and save data are unchanged.

The keybind loader also preserves an explicitly all-None primary map and
alternate-only maps. Missing settings continue to use initialized defaults.

Regression coverage extends the existing keyboard/chord and mouse-source tests:
both seat orders, button/stick/policy agreement, non-conflicting mouse controls,
temporary title bindings, cleared-map reload and alternate-only reload.
Release qualification uses the real SDL routing path with a virtual Xbox
360-compatible controller, without injecting guest pad words.

The separate Storm Owl capsule/rapid-ascent report remains unverified and is
not part of this fix. Central Beads intake is pending because the configured
database is unavailable; no replacement database was created.
