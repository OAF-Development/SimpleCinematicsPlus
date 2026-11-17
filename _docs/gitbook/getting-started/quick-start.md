# Your first shot

This walks through a simple two-key camera move with one event.

1. **Open the camera** with `Insert`. Press `Space` to show the cursor when you want to click the HUD, and again to fly.
2. **Fly to your start framing** with `W A S D`, `E` (up) and `Q` (down). Hold `Left Shift` to go faster or `Left Alt` to go slower. To change the flying speed with the mouse wheel, hide the HUD with `Y` first (the wheel can't change it while the HUD is shown).
3. **Set the first key.** Make sure the playhead is at `0s` on the timeline bar at the bottom, then press `I` (or click **Set Key**). A diamond appears on the timeline.
4. **Move the playhead.** Setting a key moves the playhead forward by **Key Step** seconds, so you're ready for the next one. You can also drag on the timeline bar, or use `Left` / `Right` arrows (0.1 s, or 1 s with `Shift`).
5. **Fly to the end framing** and press `I` again.
6. **Play it** with `Right Alt`. Press it again to pause. **Stop** rewinds to the start.
7. **Add an event.** Move the playhead to where you want the action, click the **Events** title to unfold it, pick a type such as **Fire 1 shot** or **Walk 3 s** from the **Event Type** dropdown, and click **Add Event**. An orange marker appears under the timeline.
8. **Save the sequence.** Type a name at the bottom of the Sequencer section and click **Save Sequence**.
9. **Record it.** Start your screen recorder and click **Record Take**. SCPlus hides the HUD and overlays, counts down 3-2-1 and plays the sequence once from the start.

> [!TIP]
> **Auto-Hide HUD** is on by default, so the HUD disappears whenever the sequence plays. For cinema bars, open **Secondary Camera Settings > Camera Filters** and click **Letterbox**.

## Tips

* **Undo and redo** key and event edits with `Ctrl+Z` and `Ctrl+Y`.
* Hover over any button or slider for a tooltip, and press `F1` for every key.
* **Show Motion Path** (Waypoint Editor section) draws the camera's curve on screen so you can see the whole move before playing it.
* **Aim At Target** (Waypoint Editor section) keeps the camera pointed at the target you picked, so the keys only control position.
* Want the camera to move with your character? Tick **Follow Actor**, or let [Auto-Key](../features/auto-key.md) bake the move into keys for you.
* The first time you press Play from `0:00`, SCPlus remembers where your character is standing. **Stop** and every loop put them back there. See [Events](../features/events.md#actor-start).
