Mega Man X4 Native Scene Interpolation

This default-disabled mod redraws actors and scrolling layers at intermediate
positions through the stable mmx4.frame-interpolation plugin id. It uses the
shared sprite-scene replay and guest-state restoration services. HUD positions
and sprite animation frames remain discrete. Teleports and scene cuts reset
motion history instead of dragging objects from the previous scene.

It does not alter guest VBlank, game logic, timers, or audio. Output rate can
follow the measured display refresh or use a fixed presentation rate. The
OpenGL presenter is selected automatically when interpolation is enabled.

Credit

mstan - mod integration
