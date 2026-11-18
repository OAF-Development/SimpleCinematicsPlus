# Waypoint Paths (WIP)

> \[!WARNING] **Path following is still being tested.** Placing and editing paths works, and the path events have had several fixes from in-game testing, but they may still change before release.

{% embed url="https://www.youtube.com/watch?v=UjeuxGKaBnw" %}

Waypoint paths let your character walk, jog or sprint along a route you place, instead of in a straight line. Everything for paths lives in the **Waypoint Editor** section.

{% embed url="https://www.youtube.com/watch?v=khLzLqfOnF4" %}

## The Waypoint Editor section

| Control                | What it does                                                                                              |
| ---------------------- | --------------------------------------------------------------------------------------------------------- |
| **Place Waypoints**    | While ticked, clicking the ground edits the path instead of the HUD. `P` toggles it.                      |
| **Show Paths**         | Draws the path on the ground as coloured strips. Off by default.                                          |
| **Show Motion Path**   | Draws the camera's move (and the actor track) on screen. See [Sequencer](sequencer.md#show-motion-path).  |
| **Follow Actor**       | Playback moves the camera along with your character. See [Sequencer](sequencer.md#follow-actor).          |
| **Aim At Target**      | Playback keeps the camera pointed at the picked target.                                                   |
| **Replay Actor Track** | Playback walks your character along the track Auto-Key recorded. See [Auto-Key](auto-key.md#actor-track). |
| **Waypoint Curve**     | How rounded the selected waypoint's corner is, from 0 (sharp) to 1 (fully rounded).                       |
| **Delete Point**       | Removes the selected waypoint, or the last one if none is selected.                                       |
| **Clear Path**         | Removes every waypoint.                                                                                   |
| **Clear Actor Track**  | Deletes the recorded actor track (click twice).                                                           |

## Placing a path

1. Tick **Place Waypoints** (or press `P`) and show the cursor (`Space`).
2. Click the ground to drop a point. Each point snaps to the ground, floor or roof under the cursor. The first click also adds a point where your character is standing, so the path always starts at the actor.
3. Click a waypoint's disc to select it (the selected disc is bigger and white), and drag it to move it. A new click on the ground adds the next point after the selected one.
4. Set **Waypoint Curve** for the selected point to round off its corner. New points start fully rounded.
5. Untick **Place Waypoints** (or press `P`) when you're done, so clicks go back to the HUD.

The path, including each corner's curve, is saved with the sequence. Edits can be undone with `Ctrl+Z`.

## Following it

Add a **Walk path**, **Jog path** or **Sprint path** event where the character should set off. If they're standing within 3 m of the first point, the path starts from where they are. Otherwise SCPlus adds their position as the first point.

The character follows the rounded path smoothly without stopping to turn, and drops to a walk on sharp corners. There's no obstacle avoidance, so place points around walls and fences yourself.

## Seeing it

Tick **Show Paths** to draw the route on the ground as strips, with a disc on each waypoint. They're real objects in the world, so walls and hills hide them naturally. Only you can see them, and they hide automatically during playback and Record Take so they never end up in a recording.

## Path colours

Each segment of the path is checked:

| Colour | Meaning                                                                                     |
| ------ | ------------------------------------------------------------------------------------------- |
| Green  | Clear.                                                                                      |
| Yellow | Risky: a steep but walkable slope, an obstacle near the edge, a step up, or shallow water.  |
| Red    | Blocked: something solid at knee or chest height, a slope too steep to walk, or deep water. |

The colours are a planning aid, not a guarantee. A doorway or a low branch can still trip the character up.

Later, the route may also be checked against the game's AI navigation mesh, so a segment turns yellow when the real walkable route is much longer than the straight line.
