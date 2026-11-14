# Known limits

SCPlus is actively developed and tested mostly for **solo filming**, on a local or private server or offline. These are the current limits.

## Multiplayer

* Firing, movement, the actor reset and date changes are sent to the server and work in multiplayer.
* Holstering, prone edge cases, weapon sway, and several players filming at once are **untested**.
* Everything else (the camera, keys, presets, overlays and Bullet Cam) is local to you.

## Sequencer and camera

* **Keys are fixed in the world.** If a movement event walks your character away, the camera doesn't follow them during playback. Key the camera along the route, or use **Aim At Target** to keep them in frame.
* **Follow Target doesn't apply during playback.**
* **Event edits aren't in undo.** `Ctrl+Z` only undoes key edits.
* **Camera Collision** can let the camera slip through thin fences and some foliage. It's off during playback.

## Events

* **Walk / Jog / Sprint 3 s move in a straight line for 3 seconds.** For a route, use the path events. Waypoint paths are still in development and have no obstacle avoidance.
* **Overlapping events can clash.** A stance change during a walk or a raise may be skipped. Space them out.
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
