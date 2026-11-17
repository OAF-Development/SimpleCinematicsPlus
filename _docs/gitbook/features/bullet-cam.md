# Bullet Cam

Bullet Cam follows your next shot from the muzzle to wherever it lands, in slow motion, then puts the camera back where it was (or lets the sequence carry on).

## Using it

1. Open the **Bullet Camera** section and tick **Bullet Cam** to arm it.
2. Fire, either yourself or with a [Fire event](events.md).
3. The camera rides behind the bullet to the impact.

| Setting | What it does |
| --- | --- |
| **Bullet Cam** | Arms Bullet Cam for your next shot. |
| **Show Round** | Shows a bullet model in front of the camera. It's scaled to the calibre of the ammo. |
| **Debug Bullet Path** | Draws the barrel and the computed flight as lines, for checking the direction. |
| **Bullet Speed %** | How fast the camera travels along the flight, as a percentage of the real bullet speed (1 to 25). Lower is slower. |
| **Cam Distance** | How far behind the bullet the camera sits, in metres (0.2 to 5). |

## How accurate it is

DayZ bullets aren't script objects, so the camera can't attach to the real one. SCPlus re-simulates the flight from your aim direction, the ammo's speed and air friction, and gravity, and finds the hit with a raycast. The path matches the real shot closely at normal ranges. It ignores wind and ricochets.
