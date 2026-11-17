# Known limits

SCPlus is actively developed and tested mostly for **solo filming**, on a local or private server or offline. These are the current limits.

## Multiplayer

* Firing, movement, path following, actor track replay, the actor and scene target reset, and date changes are sent to the server and work in multiplayer.
* Holstering, prone edge cases, weapon sway, and several players filming at once are **untested**.
* Everything else (the camera, keys, presets, overlays and Bullet Cam) is local to you.

## Sequencer and camera

* **Keys are fixed in the world.** To keep a walking character in frame, tick **Follow Actor**, use **Aim At Target**, or bake the move with [Auto-Key](../features/auto-key.md).
* **Simple Cinematics' Follow Target doesn't apply during playback.** Follow Actor covers your own character. For other targets, bake the follow with Auto-Key.
* **Camera Collision** can let the camera slip through thin fences and some foliage. It's off during playback, and off by default.
* **Show Motion Path** is only drawn while the HUD is shown.

## Events and actors

* **Walk / Jog / Sprint always go straight ahead.** You can change how long they last by dragging the end of their bar, but not their direction. For a route, use the path events or an actor track.
* **Waypoint paths are still in testing** and have no obstacle avoidance.
* **Actor track replay is close, not exact.** DayZ only moves a character at fixed walk, jog and sprint speeds, so expect a few tenths of a metre of error on a steady walk, and more on sudden speed changes.
* **Overlapping events can clash.** Clashes are drawn red on the timeline. One of the events may be cut short or skipped, so space them out.
* **Animals and AI** go back to the scene target start, but they don't behave the same in every take.
* Fire events need a loaded weapon in hand.

## Input

* **Typing in a name box can still nudge the camera** in some setups. Close the cursor (`Space`) before flying again if you notice drift.
* **Mouse wheel over a slider row** doesn't step the ticked slider. Use the arrow keys or drag it.
* `Left Alt` is both the camera's Slow key and vanilla free-look. SCPlus blocks free-look while the camera is open and resets your head when you close it.

## Bullet Cam

* The flight is re-simulated, so it ignores wind and ricochets.

## Compatibility

* SCPlus fully replaces three Simple Cinematics methods (the camera's frame update, the HUD's `SetCamera` and the module's update). A future Simple Cinematics update that changes those may need an SCPlus update too.
* Mods that bind the same keys (for example COT) are disabled only while the camera is open, never rebound.
