# Bullet Cam

Bullet Cam follows your next shot from the muzzle to wherever it lands, in slow motion, then puts the camera back where it was (or lets the sequence carry on).

## Using it

1. Tick **Bullet Cam** in the HUD to arm it.
2. Fire, either yourself or with a [Fire event](events.md).
3. The camera rides behind the bullet to the impact.

| Setting | What it does |
| --- | --- |
| **Bullet Speed %** | How fast the camera travels along the flight. Lower is slower. |
| **Cam Distance** | How far behind the bullet the camera sits. |
| **Show Round** | Shows a bullet model in front of the camera. It's scaled to the calibre of the ammo. |
| **Show Path (debug)** | Draws the computed flight as lines, for checking the aim. |

## How accurate it is

DayZ bullets aren't script objects, so the camera can't attach to the real one. SCPlus re-simulates the flight from your aim direction, the ammo's speed and air friction, and gravity, and finds the hit with a raycast. The path matches the real shot closely at normal ranges. It ignores wind and ricochets.
