# Simple Cinematics Plus

Simple Cinematics Plus (SCPlus) is an addon for **Simple Cinematics**. It turns the free camera into a small filmmaking tool: keyframe camera moves on a timeline the way you would in Blender, Unreal or Maya, shape the motion in a graph editor, and have your character fire, raise a weapon, emote, walk or change stance on cue while the shot plays.

It doesn't replace Simple Cinematics. It loads on top of it and changes it through modded classes, so the original mod stays untouched and you keep every feature it already has.

## What you get

* **Sequencer.** Set keys for position, rotation, FOV and depth of field at the playhead, then play them back with Bezier, Linear, Constant or Ease curves.
* **Timeline bar and graph editor.** Scrub, zoom, pan and drag keys on a Blender-style timeline, and edit curve handles per channel.
* **Events.** Put actions on the timeline: fire a shot, a burst or a full magazine, raise or lower the weapon, play or stop an emote, walk, jog, sprint, crouch, stand or go prone.
* **Pose Hold.** Keep a full-body emote (vanilla or SurvivorAnimations) playing instead of snapping back to idle.
* **Bullet Cam.** Follow your next shot from the muzzle to the impact in slow motion.
* **Framing tools.** Letterbox bars, a rule-of-thirds grid, camera collision, and an auto-hidden HUD for clean takes.
* **Presets and saved settings.** Save and load looks (FOV, DOF, filters, blur, colour), and keep your control settings between sessions.
* **Fixes** to a number of Simple Cinematics bugs. See [Fixes to Simple Cinematics](reference/fixes.md).

{% hint style="info" %}
SCPlus is built for **solo filming on your own server or offline**. Multiplayer works for the core features, but some edge cases are untested. See [Known limits](reference/known-limits.md).
{% endhint %}

## Requirements

| Mod | Why |
| --- | --- |
| Simple Cinematics | SCPlus extends it (`SimpleCinematics_Scripts`) |
| Dabs Framework | UI framework used by Simple Cinematics and SCPlus |
| Community Framework (CF) | RPCs used for server-side actions |
| Community Online Tools (COT) | Exclusively tested with COT; functionality without COT has not been conducted |

Continue to [Installation](getting-started/installation.md).

## Credits & Disclaimers

* **Simple Cinematics Plus** by iLLMannerSevs and Claude Opus 5.5.
* **Simple Cinematics** by InclementDab and JLK ([Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=2547108825)). SCPlus builds on their work and requires it.
* **Dabs Framework** by InclementDab.
