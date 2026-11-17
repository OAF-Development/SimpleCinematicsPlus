# What's new

Changes since these docs were first published (11 November 2026).

## 17 November 2026: actor track and a tidier panel

**New**

* **Actor track.** Auto-Key now also records where your character walks and which way they face. Edit it in the graph editor (Actor X, Actor Height, Actor Z, Actor Heading), see it as green diamonds on the timeline and a green motion path, and replay it with **Replay Actor Track**. See [Auto-Key and actor track](../features/auto-key.md#actor-track).
* **Waypoint Editor section** with everything for paths in one place. `P` toggles **Place Waypoints**. Click a waypoint's disc to select it, drag it to move it, and set **Waypoint Curve** to round its corner. The first click adds a point where your character stands, so paths always start at the actor.

**Changed**

* **New panel layout:** Presets, Sequencer, Graph Editor, Camera Options, Events, Waypoint Editor, Focus & Follow Target, Bullet Camera, Secondary Camera Settings (Camera Filters, Color, Blur) and Environment Settings (Weather, Date). Presets, Events and the secondary and environment groups start folded.
* Presets, Sequence, Channel and Event Type are now dropdowns that mark the active entry.
* Renamed: **Time (s)** is now **Scrubber (s)**, **Length (s)** is **Runtime (s)**, **Show Path (debug)** is **Debug Bullet Path**, and the event picker is **Event Type**. **Key Step** now moves in whole seconds.
* Moved: **Hold Pose** to the Events section; **Show Motion Path**, **Aim At Target** and **Follow Actor** to the Waypoint Editor; **Letterbox** to Camera Filters; Bullet Cam settings to their own **Bullet Camera** section.
* Removed: the **Shorter**, **Longer** and **Angle ±45** event buttons. Drag the end of an event bar to change its duration, and use paths or the actor track for direction.
* Defaults: only **Auto-Hide HUD** and **Ripple Edit** start ticked. **Camera Collision**, **Show Paths** and **Show Motion Path** start off.
* **Camera Speed** is locked while the HUD is shown, so scrolling over the panel can't change it. Tick its box, or hide the HUD, to change it with the wheel.

**Fixed**

* Characters following a path drifted off it, or walked its mirror image.
* Walking a path now follows the rounded curve smoothly, slowing to a walk on sharp corners, instead of zig-zagging.
* **Show Motion Path** now draws on screen (it was invisible in the retail game).

## 14 November 2026: scene target reset

* **Set Target Start** remembers where the Follow or Focus target is (an animal, a vehicle or another player). Playing from the start, Stop and every loop put it back. See [Events](../features/events.md#scene-target-start).
* Release builds no longer write SCPlus debug lines to the script log.

## 12 November 2026: Auto-Key and filming workflow

* **Auto-Key** bakes the camera into keys: either play a keyed sequence (with Follow Actor or Aim At Target) and turn it into plain keys, or track a target with no keys at all. See [Auto-Key](../features/auto-key.md).
* **Record Take**: hides the HUD and overlays, counts down 3-2-1 and plays the sequence once.
* **Follow Actor**: the camera moves with your character during playback.
* **Event bars**: Walk / Jog / Sprint show as bars; drag the end to change the duration. Clashing events turn red.
* **Undo and redo** now cover events as well as keys.
* **Tooltips** on HUD controls, and a key sheet on `F1`.
* **Backups and confirmations**: overwriting a different sequence or preset, or deleting one, needs a second click, and the old file is kept in `Backups`.
* **Actor Marker** items (five colours) to show helpers where to go. They're hidden on your screen during playback.
* Fixed: a walk could stop the next walk early on a server, and an invalid camera value could make free flying stutter.
