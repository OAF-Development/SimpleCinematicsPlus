# Sequencer and timeline

The sequencer keyframes the camera the way Blender, Unreal and Maya do. Keys sit at absolute times on a timeline, and the camera is interpolated between them.

## Setting keys

**Set Key** (`I`) keys every channel at the playhead at once:

* position
* rotation
* field of view
* depth of field distance and blur

Setting a key where one already exists replaces it. After a key is set, the playhead moves forward by **Key Step** seconds so you can frame the next shot straight away.

## Working with keys

* Click a key in the left panel, or its diamond on the timeline, to select it and jump to it.
* **Update Key** re-keys the selected key with the current camera.
* **Move Key Here** moves the selected key to the playhead.
* **Delete Key** (`Delete`) removes it. **Clear Keys** removes all of them.
* **Previous / Next Key** (`Down` / `Up`) jump the playhead between keys.
* `Ctrl+Z` and `Ctrl+Y` undo and redo key edits.

## Interpolation

Each key has an interpolation that shapes the curve from that key to the next. Cycle it with the **Curve** button.

| Mode | Behaviour |
| --- | --- |
| **Bezier** (default) | Smooth, with automatic clamped handles like Blender's default. The curve passes through every key and never overshoots. |
| **Linear** | Straight, constant-speed moves between keys. |
| **Constant** | Holds the value, then jumps at the next key. Good for hard cuts. |
| **Ease In / Ease Out / Ease In-Out** | Speeds up, slows down, or both. |

Each channel is its own curve, and rotation always turns the short way. Before the first key the camera holds the first key, and after the last key it holds the last key.

## Timeline bar

The bar along the bottom shows a ruler, a diamond for each key, orange markers for events, and the red playhead.

* Click or drag to scrub.
* Drag a key's diamond to retime it. It snaps to 0.1 s (hold `Ctrl` for no snapping) and can't pass its neighbours.
* With **Ripple Edit** ticked, every later key moves along with the one you drag.
* Mouse wheel zooms around the cursor, middle-drag pans, and a double-click shows the whole sequence.

## Playback settings

| Setting | What it does |
| --- | --- |
| **Time (s)** | The playhead position. |
| **Length (s)** | The timeline length. It always grows to cover every key. |
| **Key Step (s)** | How far the playhead moves after Set Key. |
| **Speed (x)** | Playback speed from 0.25x to 2x. |
| **Loop Playback** | Wraps back to the start at the end. |
| **Aim At Target** | Points the camera at the focus target and ignores the keyed rotation. |
| **Auto-Hide HUD** | Hides the HUD while playing. |
| **Show Motion Path** | Draws the camera curve, each key with its look direction, and the playhead in the world. |

**Play / Pause** (`Right Alt`) plays from the playhead. **Stop** rewinds to the start and puts your character back at the [actor start](events.md#actor-start). A sequence needs at least two keys to play.

## Saving sequences

Type a name and click **Save Sequence**. Pick one from the dropdown to load it, or **Delete Sequence** to remove it. **New Sequence** starts empty.

A sequence saves its keys, events and actor start position. It also records the map it was made on, and you get a warning if you load it on a different map.
