# Events (actor actions)

Events are actions your own character performs at a set time while the sequence plays. They show as orange markers under the timeline, and you can drag them to retime them.

## Adding an event

1. Move the playhead to the moment you want.
2. Click the **Events** title to unfold the section (it starts folded), then open the **Event Type** dropdown.
3. Pick an event and click **Add Event**.

To remove one, select its marker and click **Delete Event**, or press `Delete`. Adding, moving and deleting events can all be undone with `Ctrl+Z`.

## Event types

| Event | What happens |
| --- | --- |
| **Fire 1 shot** | Fires the weapon in hand once. |
| **Fire 3-shot burst** | Fires three rounds. |
| **Fire 10 shots** | Fires ten rounds. |
| **Raise weapon** | Raises the weapon to the ready position. |
| **Lower weapon** | Lowers it again. |
| **Emote (current)** | Replays the emote you're playing right now. Start it from the emote wheel first, then add the event. |
| **Stop emote** | Ends the emote. |
| **Walk 3 s** / **Jog 3 s** / **Sprint 3 s** | Moves the character straight ahead from the way they face. They start at 3 seconds: drag the end of the bar on the timeline to make them shorter or longer. |
| **Crouch** / **Stand** / **Prone** | Changes stance. |
| **Walk path** / **Jog path** / **Sprint path** | Follows the placed [waypoint path](waypoint-paths.md). *In testing.* |

> [!NOTE]
> **Walk / Jog / Sprint** always go straight ahead. To follow a route, use the path events with the [Waypoint Editor](waypoint-paths.md), or record the walk with [Auto-Key](auto-key.md#actor-track) and replay it.

## Clashes

Some events can't run at the same time as others. A timed walk, jog or sprint that gets interrupted by another move, path, stance change or emote, or a stance change less than a second after a raise or lower, is a clash. Clashing events are drawn **red** on the timeline, and a message tells you which one may be cut short or skipped. Move one of them to fix it.

## Actor start

The first time you press **Play** from `0:00`, SCPlus remembers where your character is standing and which way they face. **Stop** and every loop of **Loop Playback** put them back there and cancel any walk or stance change in progress, so each take starts the same.

To move the start, stand where you want the scene to begin and click **Set Actor Start** in the Events section. Save the sequence to keep it.

## Scene target start

If your shot follows or focuses on something else, such as an animal, a vehicle or another player, SCPlus can put that back too. Pick the target in **Focus & Follow Target**, put it where the scene starts, and click **Set Target Start**. Playing from the start, **Stop** and every loop then return it to that spot (a vehicle also stops moving). With no target picked, the button clears the saved target start.

The target start is also set automatically along with the first actor start. In multiplayer the server does the move, only for animals, vehicles and players, within 1 km.

> [!NOTE]
> Animals and AI decide for themselves what to do next, so they won't act the same in every take even when they start in the same place.

## Hold Pose

The **Hold Pose ( , )** checkbox sits at the bottom of the Events section. See [Pose Hold](pose-hold.md).

## Actor markers

SCPlus adds **Actor Marker** items in five colours (Green, Yellow, Red, Blue and White). Spawn them with COT and drop them on the ground to show helpers where to stand, walk or run. Everyone can see them while you set up, but they're hidden on your screen while the sequence plays, so they never end up in your shot.

## Tips

* Fire events need a loaded weapon in hand. Raise it first, or put a **Raise weapon** event a moment before.
* For poses that should hold for the whole shot, use [Pose Hold](pose-hold.md).
* While **Replay Actor Track** is on, Walk / Jog / Sprint and path events are skipped, because the actor track moves your character instead.

## How it works

While the camera is open, Simple Cinematics disables your character's input so flying the camera doesn't move them. For each event, SCPlus briefly hands control back, applies the action and then takes it away again. Movement is mirrored on the server so your character actually travels across the terrain in multiplayer instead of animating on the spot.
