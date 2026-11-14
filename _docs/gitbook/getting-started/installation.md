# Installation

## Load order

SCPlus must load **after** Simple Cinematics and its dependencies:

```
-mod=@DabsFramework;@CF;@COT;@SimpleCinematics;@SimpleCinematicsPlus
```

If you use a launcher (DayZ Launcher, DZSA, a mod manager), subscribe to all five and make sure SCPlus comes last.

## Client and server

SCPlus is needed on **both the client and the server**. Several features send small requests to the server (firing, movement, the actor reset, and setting the date and time), so a server without SCPlus will ignore them.

For a dedicated server:

1. Add `@SimpleCinematicsPlus` to the server's `-mod=` line after `@SimpleCinematics`.
2. Copy the `.bikey` from `@SimpleCinematicsPlus\Keys` into the server's `keys` folder.
3. Restart the server.

## Who can use the camera

SCPlus uses Simple Cinematics' own permission list. Only players listed in `AvailableUsers` in Simple Cinematics' `simple_cinematics.json` can open the camera or change the weather and time. SCPlus also makes the server reject weather and time requests from anyone not on that list.

Offline (singleplayer) mode needs no list: the camera just works.

## Checking it loaded

* Open **Options > Controls**. You should see a **Simple Cinematics** category with the Sequencer keys and **Toggle Hold Pose** in it.
* Open **Options**. There should be a **Simple Cinematics** tab with Movement and View settings.
* Open the camera (`Insert` by default). The right panel should have **Sequencer**, **Graph Editor**, **Events** and **Presets** sections.
