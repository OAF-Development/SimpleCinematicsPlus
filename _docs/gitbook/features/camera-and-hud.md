# Camera, HUD and presets

## The HUD

Open the camera with `Insert` and press `Space` for the cursor. The right panel is laid out from the top in the order you'll usually work:

| Section | What's in it |
| --- | --- |
| **Presets** | Saved looks (starts folded). |
| **Sequencer** | Saved sequences, playback buttons, key tools, timeline settings and toggles. See [Sequencer and timeline](sequencer.md). |
| **Graph Editor** | The curve of one channel. See [Graph editor](graph-editor.md). |
| **Camera Options** | Camera speed, mouse speed, smoothing, FOV, depth of field, near plane and view distances. |
| **Events** | Actor actions on the timeline, plus Hold Pose (starts folded). See [Events](events.md). |
| **Waypoint Editor** | Paths for your character, Show Motion Path, Follow Actor, Aim At Target and the actor track. See [Waypoint paths](waypoint-paths.md). |
| **Focus & Follow Target** | Simple Cinematics' target tools. |
| **Bullet Camera** | See [Bullet Cam](bullet-cam.md). |
| **Secondary Camera Settings** | Camera Filters (with Letterbox), Camera Color and Camera Blur (all start folded). |
| **Environment Settings** | Weather Controls and Date Controls (all start folded). |

Click a section's title to fold or unfold it. Dropdowns (Presets, Sequence, Channel and Event Type) open a list under their header, and the active entry is marked.

Hover over any button, slider or checkbox for a short tooltip. Press `F1`, or click **Keys (F1)**, for a sheet of every key.

The left panel lists the keys of the current sequence. The timeline bar runs along the bottom of the screen.

## Presets

A preset stores a whole look: FOV, depth of field, filters, blur and colour.

* **Save Preset** saves the current look under the name you type. To replace a different preset that already has that name, click it twice.
* Pick a preset from the **Presets** dropdown to load it, or **Delete Preset** (click twice) to remove it.
* **Reset Look** puts every look setting back to its default.

Before a preset is overwritten or deleted, a copy goes to the Backups folder. See [Saved files and settings](../reference/files-and-settings.md).

When you open the camera, your last look is restored automatically.

## Control settings

Camera speed, mouse speed, movement and mouse smoothing, view distance and object view distance are saved to your profile. You can change them in the HUD or in **Options > Simple Cinematics**.

**Camera Speed is locked while the HUD is shown**, so scrolling over the panel doesn't change it by accident. Tick its box to step it with the mouse wheel, or hide the HUD (`Y`) and the wheel changes it as usual.

## Framing tools

| Tool | Where | What it does |
| --- | --- | --- |
| **Letterbox** | Secondary Camera Settings > Camera Filters | Click to cycle cinema bars: Off, 2.39:1, 2:1, 1.85:1 and 1:1. These do show in recordings. |
| **Thirds Grid** | Sequencer | Draws rule-of-thirds lines over the view. Hidden during Record Take. |
| **Auto-Hide HUD** | Sequencer | Hides the HUD and cursor while a sequence plays. On by default. |
| **Camera Collision** | Sequencer | Stops the free-flying camera about 20 cm short of walls, objects and the ground, and slides it along them. Off by default. |
| **Show Motion Path** | Waypoint Editor | Draws the camera's path, keys and playhead on screen while you edit. See [Sequencer](sequencer.md#show-motion-path). |
| **Record Take** | Sequencer | Plays the sequence once for a clean recording. See [Sequencer](sequencer.md#record-take). |

> [!NOTE]
> Camera Collision only applies while you fly the camera yourself. Playback ignores it, so a keyframed shot can still pass through a window or doorway. Thin objects like fences and some foliage have weak collision and the camera may slip through them.


## Focus and Follow Target

These come from Simple Cinematics and live in the **Focus & Follow Target** section. **Select Target**, then click something in the world to pick it.

* **Focus on Target** keeps the camera looking at the target (for a player, their head).
* **Follow Target** moves the camera along with the target while you fly.

How they work with the sequencer:

* **Aim At Target** (Waypoint Editor section) makes playback point the camera at the target. Your keys then only control position.
* **Follow Target itself doesn't apply during playback.** Keys stay fixed in the world. To have the camera move with your own character, tick **Follow Actor**. For any other target (an animal, a vehicle, another player), use [Auto-Key](auto-key.md) to bake the follow into keys.
* **Set Target Start** (Events section) remembers where the target is, so Stop and every loop put it back. See [Events](events.md#scene-target-start).

## Date and time

Date Controls and Weather Controls are in **Environment Settings**. Changing the date and time is sent to the server, so everyone on the server sees the same time. **Lock Time** stops the clock so the light doesn't change between takes.
