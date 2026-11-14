# Troubleshooting and FAQ

**The Sequencer sections don't appear.**
Check the load order: SCPlus must come after Simple Cinematics. Check that Dabs Framework, CF and COT are loaded too.

**I can't open the camera on my server.**
Your Steam ID must be in `AvailableUsers` in Simple Cinematics' `simple_cinematics.json`.

**Events do nothing in multiplayer.**
The server needs SCPlus as well, with its `.bikey` in the server's `keys` folder.

**My character animates on the spot instead of walking.**
This happens when the server doesn't have SCPlus (or has an older version). Update the server.

**My character doesn't go back to the start on Stop.**
The actor start is saved the first time you press Play from `0:00`. Use **Set Actor Start** to set it by hand, then save the sequence.

**Play does nothing.**
A sequence needs at least two keys.

**My emote drops back to idle mid-shot.**
Turn on **Hold Pose** (`,`).

**The arrow keys change a slider instead of the playhead.**
A quick-slider box is ticked. Untick it to use the arrows on the playhead.

**`H`, `Y` or `Delete` doesn't do what COT normally does.**
SCPlus blocks those COT keys while the camera is open, so filming doesn't teleport you or delete objects. They come back when you close the camera.

**The view went white while using Focus on Target.**
This was a Simple Cinematics bug, fixed in SCPlus. Update to the latest version.

**Where are the logs?**
Your client script log (`script_*.log` in your DayZ profile folder). SCPlus lines start with `[SCPlus Events]`, `[SCPlus Camera]` or `[SCPlus BulletCam]`.
