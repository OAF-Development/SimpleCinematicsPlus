# Fixes to Simple Cinematics

SCPlus fixes these Simple Cinematics issues without editing the original mod:

* The mouse speed slider (and `Ctrl` + wheel) works. It was bound to a field that didn't exist.
* The "decrease movement smoothing" key works.
* The camera works offline (singleplayer).
* The keybinds appear under their own **Simple Cinematics** category in **Options > Controls**.
* The hour and minute sliders start at 0.
* Clicking empty sky while picking a target no longer throws an error.
* Following a target that no longer exists no longer crashes.
* **Focus on Target** on a player no longer whites out the screen. The head is found correctly, and a bad frame snaps back to the last good camera position.
* Your character's head no longer stays turned after you close the camera.
* Moving the mouse in the camera no longer turns your character's aim underneath it, so shots go where the character was aiming.
* The server's weather and time requests reject players who aren't in `AvailableUsers`.
* The hardcoded default Steam ID is no longer kept in the synced user list on clients.
* Camera speed is always initialised, and unused zoom input reads were removed.
