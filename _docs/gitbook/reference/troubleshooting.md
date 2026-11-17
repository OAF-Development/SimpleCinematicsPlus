# Troubleshooting and FAQ

**The Sequencer sections don't appear.**
Check the load order: SCPlus must come after Simple Cinematics. Check that Dabs Framework, CF and COT are loaded too.

**I can't find a section or a setting.**
Several sections start folded (Presets, Events, and everything under Secondary Camera Settings and Environment Settings). Click a section's title to open it. See [The HUD](../features/camera-and-hud.md#the-hud) for where everything lives.

**I can't open the camera on my server.**
Your Steam ID must be in `AvailableUsers` in Simple Cinematics' `simple_cinematics.json`.

**The mouse wheel doesn't change the camera speed.**
Camera Speed is locked while the HUD is shown. Hide the HUD (`Y`), or tick the Camera Speed box to step it with the wheel.

**Events do nothing in multiplayer.**
The server needs SCPlus as well, with its `.bikey` in the server's `keys` folder.

**My character animates on the spot instead of walking.**
This happens when the server doesn't have SCPlus (or has an older version). Update the server.

**My character doesn't go back to the start on Stop.**
The actor start is saved the first time you press Play from `0:00`. Use **Set Actor Start** to set it by hand, then save the sequence.

**The animal or vehicle I'm filming doesn't go back between takes.**
Pick it as the target and click **Set Target Start** (Events section), then save the sequence.

**Play does nothing.**
A sequence needs at least two keys, unless Auto-Key is on.

**Auto-Key didn't change my keys.**
With two or more keys, Auto-Key only bakes when playback reaches the end. Stopping early keeps your old keys.

**My Walk / Jog / Sprint or path events were ignored.**
Is **Replay Actor Track** on? While it is, the actor track moves your character and those events are skipped. A red event on the timeline also means it clashes with another event.

**My character walked the path the wrong way, or mirrored.**
This was fixed in the 2026-11-17 build. Update to the latest version.

**My emote drops back to idle mid-shot.**
Turn on **Hold Pose** (`,`), at the bottom of the Events section.

**The arrow keys change a slider instead of the playhead.**
A quick-slider box is ticked. Untick it to use the arrows on the playhead.

**`H`, `Y` or `Delete` doesn't do what COT normally does.**
SCPlus blocks those COT keys while the camera is open, so filming doesn't teleport you or delete objects. They come back when you close the camera.

**I deleted or overwrote a sequence or preset by mistake.**
Look in `$profile:SimpleCinematics/Backups`. See [Saved files and settings](files-and-settings.md#backups).

**The view went white while using Focus on Target.**
This was a Simple Cinematics bug, fixed in SCPlus. Update to the latest version.

**Where are the logs?**
Your client script log (`script_*.log` in your DayZ profile folder). Release builds keep SCPlus logging to a minimum: errors when a sequence can't be saved or loaded start with `[SimpleCinematicsPlus]`. On a server, `[SimpleCinematicsPlus] Rejected ...` lines show weather or date requests from players who aren't allowed.
