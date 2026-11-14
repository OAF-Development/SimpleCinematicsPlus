# Camera, HUD and presets

## The HUD

Open the camera with `Insert` and press `Space` for the cursor. The right panel holds Simple Cinematics' sections (Camera Options, Filters, Blur, Colour, Weather, Date, Focus on Target, Follow Target) plus the SCPlus sections: **Sequencer**, **Graph Editor**, **Events** and **Presets**. Click a section's title to fold or unfold it.

The left panel lists the keys of the current sequence. The timeline bar runs along the bottom of the screen.

## Presets

A preset stores a whole look: FOV, depth of field, filters, blur and colour.

* **Save Preset** saves the current look under the name you type.
* Pick a preset from the dropdown to load it, or **Delete Preset** to remove it.
* **Reset Look** goes back to the built-in **Default** preset, which is always listed first.

When you open the camera, your last look is restored automatically.

## Control settings

Camera speed, mouse speed, movement and mouse smoothing, view distance and object view distance are saved to your profile. You can change them in the HUD or in **Options > Simple Cinematics**.

## Framing tools

| Tool | What it does |
| --- | --- |
| **Letterbox** | Click to cycle cinema bars: Off, 2.39:1, 2:1, 1.85:1 and 1:1. |
| **Thirds Grid** | Draws rule-of-thirds lines over the view. |
| **Auto-Hide HUD** | Hides the HUD while a sequence plays, for clean recordings. |
| **Camera Collision** | Stops the free-flying camera about 20 cm short of walls, objects and the ground, and slides it along them. On by default. |

> [!NOTE]
> Camera Collision only applies while you fly the camera yourself. Playback ignores it, so a keyframed shot can still pass through a window or doorway. Thin objects like fences and some foliage have weak collision and the camera may slip through them.


## Focus and Follow Target

These come from Simple Cinematics. **Select Target**, then click something in the world to pick it.

* **Focus on Target** keeps the camera looking at the target (for a player, their head).
* **Follow Target** moves the camera along with the target.

How they work with the sequencer:

* **Aim At Target** in the Sequencer makes playback point the camera at the focus target. Your keys then only control position.
* **Follow Target doesn't apply during playback.** Keys stay fixed in the world, so if your character walks off with a movement event, the camera won't follow unless you key it.

## Date and time

Changing the date and time in the HUD is sent to the server, so everyone on the server sees the same time. **Lock Time** stops the clock so the light doesn't change between takes.
