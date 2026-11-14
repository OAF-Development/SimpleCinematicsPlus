# Waypoint paths (in development)

{% hint style="warning" %}
**This feature is being built and tested right now.** The controls below are in the current development build, but they haven't been tested in game yet and may change before release.
{% endhint %}

Waypoint paths let your character walk, jog or sprint along a route you place, instead of in a straight line.

## Placing a path

1. Tick **Place Waypoints** in the Sequencer section and show the cursor (`Space`).
2. Click in the world to drop a point. Each point snaps to the ground, floor or roof under the cursor.
3. **Delete Last Point** (in the Events section) removes the most recent point. **Clear Path** removes them all.
4. Untick **Place Waypoints** when you're done, so clicks go back to the HUD.

The path is saved with the sequence.

## Following it

Add a **Walk path**, **Jog path** or **Sprint path** event where the character should set off. They steer toward each waypoint in turn at that speed. There's no obstacle avoidance, so place points around walls and fences yourself.

## Seeing it

Tick **Show Paths** to draw the route on the ground as ribbons. They're real objects in the world, so walls and hills hide them naturally. Only you can see them, and they hide automatically during playback so they never end up in a recording.

## Path colours (in testing)

Each segment is checked about every half metre:

| Colour | Meaning |
| --- | --- |
| Green | Clear. |
| Yellow | Risky: a steep but walkable slope, an obstacle near the edge, a step up, or shallow water. |
| Red | Blocked: something solid at knee or chest height, a slope too steep to walk, or deep water. |

The colours are a planning aid, not a guarantee. A doorway or a low branch can still trip the character up.

Later, the route may also be checked against the game's AI navigation mesh, so a segment turns yellow when the real walkable route is much longer than the straight line.
