# Events (actor actions)

Events are actions your own character performs at a set time while the sequence plays. They show as orange markers under the timeline, and you can drag them to retime them.

## Adding an event

1. Move the playhead to the moment you want.
2. Open the **Events** section and click the **Type** dropdown.
3. Pick an event and click **Add Event**.

To remove one, select its marker and click **Delete Event**.

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
| **Walk 3 s** / **Jog 3 s** / **Sprint 3 s** | Moves the character straight ahead from the way they face, for 3 seconds. |
| **Crouch** / **Stand** / **Prone** | Changes stance. |
| **Walk path** / **Jog path** / **Sprint path** | Follows the placed [waypoint path](waypoint-paths.md). *In development.* |

{% hint style="warning" %}
**Walk / Jog / Sprint 3 s** move in a straight line for a fixed 3 seconds. To follow a route instead, use the path events with [Waypoint paths](waypoint-paths.md), which are still in development.
{% endhint %}

## Actor start

The first time you press **Play** from `0:00`, SCPlus remembers where your character is standing and which way they face. **Stop** and every loop of **Loop Playback** put them back there and cancel any walk or stance change in progress, so each take starts the same.

To move the start, stand where you want the scene to begin and click **Set Actor Start** in the Events section. Save the sequence to keep it.

## Tips

* Keep events from overlapping where you can. A stance change landing in the middle of a walk or a raise can be ignored.
* Fire events need a loaded weapon in hand. Raise it first, or put a **Raise weapon** event a moment before.
* For poses that should hold for the whole shot, use [Pose Hold](pose-hold.md).
* The client log lines for events start with `[SCPlus Events]`, which helps if something doesn't fire.

## How it works

While the camera is open, Simple Cinematics disables your character's input so flying the camera doesn't move them. For each event, SCPlus briefly hands control back, applies the action and then takes it away again. Movement is mirrored on the server so your character actually travels across the terrain in multiplayer instead of animating on the spot.
