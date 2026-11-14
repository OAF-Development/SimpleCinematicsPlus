# Your first shot

This walks through a simple two-key camera move with one event.

1. **Open the camera** with `Insert`. Press `Space` to show the cursor when you want to click the HUD, and again to fly.
2. **Fly to your start framing** with `W A S D`, `E` (up) and `Q` (down). Hold `Left Shift` to go faster or `Left Alt` to go slower.
3. **Set the first key.** Make sure the playhead is at `0s` on the timeline bar at the bottom, then press `I` (or click **Set Key**). A diamond appears on the timeline.
4. **Move the playhead.** Setting a key moves the playhead forward by **Key Step** seconds, so you're ready for the next one. You can also drag on the timeline bar, or use `Left` / `Right` arrows (0.1 s, or 1 s with `Shift`).
5. **Fly to the end framing** and press `I` again.
6. **Play it** with `Right Alt`. Press it again to pause. **Stop** rewinds to the start.
7. **Add an event.** Move the playhead to where you want the action, open **Events**, pick a type such as **Fire 1 shot** or **Walk 3 s**, and click **Add Event**. An orange marker appears under the timeline.
8. **Save the sequence.** Type a name in the Sequencer section and click **Save Sequence**.

{% hint style="success" %}
For a clean take, tick **Auto-Hide HUD** so the HUD disappears while the sequence plays, and turn on **Letterbox** if you want cinema bars.
{% endhint %}

## Tips

* **Undo and redo** key edits with `Ctrl+Z` and `Ctrl+Y`.
* **Show Motion Path** draws the camera's curve in the world so you can see the whole move before playing it.
* **Aim At Target** keeps the camera pointed at the focus target, so the keys only control position.
* The first time you press Play from `0:00`, SCPlus remembers where your character is standing. **Stop** and every loop put them back there. See [Events](../features/events.md#actor-start).
