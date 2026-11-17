# Controls and keybinds

Every key below can be changed in **Options > Controls > Simple Cinematics**, except the few marked *fixed*.

Press `F1` (or click **Keys (F1)** in the Sequencer section) while the camera is open to see this list on screen. It shows your own binds if you've changed them.

## Camera (from Simple Cinematics)

| Action | Default |
| --- | --- |
| Open / close camera | `Insert` |
| Show / hide HUD | `Y` |
| Show / hide cursor | `Space` |
| Forward / back / left / right | `W` / `S` / `A` / `D` |
| Up / down | `E` / `Q` (SCPlus default) |
| Slow | `Left Alt` |
| Turbo | `Left Shift` |
| Camera speed | Mouse wheel (see below) |
| Mouse sensitivity | `Left Ctrl` + mouse wheel |
| Movement smoothing | `N` + mouse wheel |
| Mouse smoothing | `J` + mouse wheel |
| Depth of field distance | `F` + mouse wheel |
| Field of view | Hold right mouse button + move mouse up / down |
| Step the ticked quick slider | `Left` / `Right` arrows, or mouse wheel |

> [!TIP]
> Simple Cinematics' own default for Up / Down is `Q` / `Z`. SCPlus changes it to `E` / `Q` to match the usual WASD + EQ layout of 3D software. If you've already set your own binds, they're kept.

> [!NOTE]
> **Camera speed is locked while the HUD is shown**, so scrolling over the panel or the timeline can't change it by accident. Tick the Camera Speed box to step it with the wheel, or hide the HUD (`Y`) and the mouse wheel changes it as usual.

## Sequencer (SCPlus)

| Action | Default |
| --- | --- |
| Set Key | `I` |
| Play / Pause (also cancels a Record Take countdown) | `Right Alt` |
| Next key | `Up arrow` |
| Previous key | `Down arrow` |
| Delete the selected event or key | `Delete` |
| Nudge playhead 0.1 s | `Left` / `Right` arrows (*fixed*) |
| Nudge playhead 1 s | `Shift` + `Left` / `Right` (*fixed*) |
| Undo / Redo key and event edits | `Ctrl+Z` / `Ctrl+Y` (*fixed*) |
| Toggle Hold Pose | `,` (comma) |
| Toggle Place Waypoints | `P` (*fixed*) |
| Show / hide the key sheet | `F1` (*fixed*) |

The arrow keys only move the playhead while no quick slider is ticked. With a quick-slider box ticked, they step that slider instead, as in Simple Cinematics.

`P` and `F1` are read straight from the keyboard while the camera is open, so they don't add a bind or change any of yours. `P` is ignored while you type in a name box.

## Timeline bar and graph editor (mouse)

| Action | Mouse |
| --- | --- |
| Scrub | Click or drag on the timeline bar |
| Select a key | Click its diamond |
| Retime a key or event | Drag its diamond or marker (snaps to 0.1 s, hold `Ctrl` for no snapping) |
| Change a Walk / Jog / Sprint duration | Drag the end of its bar |
| Zoom the timeline | Mouse wheel over the bar |
| Pan a zoomed timeline | Middle mouse drag |
| Show the whole sequence | Double-click the bar |
| Edit a curve handle | Drag it in the graph editor (`Shift` moves one side only) |
| Reset a handle to automatic | Right-click it |

## Waypoint Editor (mouse, with Place Waypoints on)

| Action | Mouse |
| --- | --- |
| Add a waypoint | Click the ground (it goes after the selected waypoint) |
| Select a waypoint | Click its disc |
| Move a waypoint | Drag its disc |

## Keys SCPlus blocks while the camera is open

So that filming doesn't trigger other mods by accident, SCPlus temporarily disables a few inputs while the camera is open and turns them back on shortly after you close it. Nothing is rebound.

* Vanilla free-look (`Left Alt` is also the camera's Slow key).
* COT's camera toggle and zoom, cursor teleport (`H`), delete-on-cursor (`Delete`) and button toggle (`Y`).
