# Sequencer and timeline

The sequencer keyframes the camera the way Blender, Unreal and Maya do. Keys sit at absolute times on a timeline, and the camera is interpolated between them.

## Setting keys

**Set Key** (`I`) keys every channel at the playhead at once:

* position
* rotation
* field of view
* depth of field distance and blur

Setting a key where one already exists replaces it. After a key is set, the playhead moves forward by **Key Step** seconds so you can frame the next shot straight away.

Don't want to key by hand? [Auto-Key](auto-key.md) can record a whole move for you and turn it into keys.

## Buttons

The Sequencer section has two rows of buttons.

| Playback row | What it does |
| --- | --- |
| **Previous Key** / **Next Key** | Jump to the key before or after the playhead and select it (`Down` / `Up`). |
| **Play / Pause** | Play from the playhead, or pause (`Right Alt`). |
| **Stop** | Rewind to the start and put your character back at the [actor start](events.md#actor-start). |
| **Set Key** | Key the camera at the playhead (`I`). |
| **Clear Keys** | Remove every key (`Ctrl+Z` brings them back). |
| **Record Take** | Play the sequence once for a clean recording. See [Record Take](#record-take). |
| **Keys (F1)** | Show the key sheet. |

| Key row | What it does |
| --- | --- |
| *(selected key)* | Shows which key is selected. |
| **Update Key** | Re-key the selected key with the current camera. |
| **Delete Key** | Remove the selected key (`Delete`). |
| **Move Key Here** | Move the selected key to the playhead. |
| **Undo** / **Redo** | Undo or redo key and event edits (`Ctrl+Z` / `Ctrl+Y`). |
| **Curve** | Cycle the selected key's interpolation. |

Click a key in the left panel, or its diamond on the timeline, to select it and jump to it.

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
* With **Ripple Edit** ticked (on by default), every later key moves along with the one you drag.
* Mouse wheel zooms around the cursor, middle-drag pans, and a double-click shows the whole sequence.
* **Walk / Jog / Sprint** events are drawn as bars. Drag the end of a bar to change how long the move lasts.
* Events that clash are drawn **red**, and a message says which one may be cut short or skipped. See [Events](events.md#clashes).
* If you've recorded an [actor track](auto-key.md#actor-track), its keys show as small green diamonds just above the camera keys.

## Playback settings

| Setting | What it does |
| --- | --- |
| **Scrubber (s)** | The playhead position. Drag it to scrub. |
| **Runtime (s)** | The timeline length. It always grows to cover every key and event. |
| **Key Step (s)** | How many whole seconds the playhead moves after Set Key. |
| **Speed (x)** | Playback speed from 0.25x to 2x. Events and actor moves stay in real time. |

| Toggle | What it does |
| --- | --- |
| **Loop Playback** | Wraps back to the start at the end. The actor is put back each loop. |
| **Auto-Key** | The next Play or Record Take bakes the camera (and your character's walk) into keys. See [Auto-Key](auto-key.md). |
| **Camera Collision** | Stops the free-flying camera at walls and the ground. Off by default. |
| **Auto-Hide HUD** | Hides the HUD while playing. On by default. |
| **Ripple Edit** | Retiming a key shifts every key after it. On by default. |
| **Thirds Grid** | Rule-of-thirds lines for framing. |

**Aim At Target**, **Follow Actor** and **Show Motion Path** are in the [Waypoint Editor](waypoint-paths.md) section.

**Play / Pause** (`Right Alt`) plays from the playhead. **Stop** rewinds to the start and puts your character back at the [actor start](events.md#actor-start). A sequence needs at least two keys to play, unless [Auto-Key](auto-key.md) is on.

## Follow Actor

With **Follow Actor** ticked, playback moves the camera along with your character, so a walk, a run or a path keeps them in the same spot of the frame.

Set your keys with the character standing at the actor start. During playback, every key is shifted by how far the character has moved from that start. If you also tick **Aim At Target** with your character as the target, the camera keeps looking at them too.

## Show Motion Path

**Show Motion Path** (Waypoint Editor section) draws the move on screen while you edit:

* the camera's path in yellow,
* each key as a white mark with a line for its look direction (the selected key in blue),
* the playhead in red,
* the [actor track](auto-key.md#actor-track), if there is one, as a green line near the ground with a dot and a heading line for each key.

It's only drawn while the HUD is shown, never during playback or Record Take, so it can't end up in a recording.

## Record Take

**Record Take** gets everything out of the way for a clean recording:

1. It hides the HUD, the cursor, the thirds grid, waypoint paths and actor markers.
2. It turns Loop off for this take, rewinds to the start and counts down 3-2-1.
3. After a one-second pause it plays the sequence once.
4. When playback stops, everything comes back as it was.

Press **Play / Pause** (`Right Alt`) during the countdown to cancel. Start your screen recorder before you click.

## Saving sequences

Type a name in the **Sequence** box and click **Save Sequence**. Pick one from the **Sequence** dropdown to load it, or **Delete Sequence** to remove it. **New Sequence** starts empty and leaves saved sequences alone.

Replacing a *different* saved sequence, or deleting one, asks you to click a second time within a few seconds. The old version is copied to the Backups folder first. See [Saved files and settings](../reference/files-and-settings.md).

A sequence saves its keys, events, actor start, scene target start, waypoint path, actor track, Loop, Follow Actor and Replay Actor Track. It also records the map it was made on, and you get a warning if you load it on a different map. Auto-Key isn't saved: it's always off when you load.
