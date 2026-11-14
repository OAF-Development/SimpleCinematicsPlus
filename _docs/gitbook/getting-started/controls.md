# Controls and keybinds

Every key below can be changed in **Options > Controls > Simple Cinematics**, except the few marked *fixed*.

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
| Camera speed | Mouse wheel |
| Mouse sensitivity | `Left Ctrl` + mouse wheel |
| Movement smoothing | `N` + mouse wheel |
| Mouse smoothing | `J` + mouse wheel |
| Depth of field distance | `F` + mouse wheel |
| Field of view | Hold right mouse button + move mouse up / down |
| Step the ticked quick slider | `Left` / `Right` arrows, or mouse wheel |

{% hint style="info" %}
Simple Cinematics' own default for Up / Down is `Q` / `Z`. SCPlus changes it to `E` / `Q` to match the usual WASD + EQ layout of 3D software. If you've already set your own binds, they're kept.
{% endhint %}

## Sequencer (SCPlus)

| Action | Default |
| --- | --- |
| Set Key | `I` |
| Play / Pause | `Right Alt` |
| Next key | `Up arrow` |
| Previous key | `Down arrow` |
| Delete selected key | `Delete` |
| Nudge playhead 0.1 s | `Left` / `Right` arrows (*fixed*) |
| Nudge playhead 1 s | `Shift` + `Left` / `Right` (*fixed*) |
| Undo / Redo key edits | `Ctrl+Z` / `Ctrl+Y` (*fixed*) |
| Toggle Hold Pose | `,` (comma) |

The arrow keys only move the playhead while no quick slider is ticked. With a quick-slider box ticked, they step that slider instead, as in Simple Cinematics.

## Timeline bar and graph editor (mouse)

| Action | Mouse |
| --- | --- |
| Scrub | Click or drag on the timeline bar |
| Select a key | Click its diamond |
| Retime a key | Drag its diamond (snaps to 0.1 s, hold `Ctrl` for no snapping) |
| Zoom the timeline | Mouse wheel over the bar |
| Pan a zoomed timeline | Middle mouse drag |
| Show the whole sequence | Double-click the bar |
| Edit a curve handle | Drag it in the graph editor (`Shift` moves one side only) |
| Reset a handle to automatic | Right-click it |

## Keys SCPlus blocks while the camera is open

So that filming doesn't trigger other mods by accident, SCPlus temporarily disables a few inputs while the camera is open and turns them back on shortly after you close it. Nothing is rebound.

* Vanilla free-look (`Left Alt` is also the camera's Slow key).
* COT's camera toggle and zoom, cursor teleport (`H`), delete-on-cursor (`Delete`) and button toggle (`Y`).
