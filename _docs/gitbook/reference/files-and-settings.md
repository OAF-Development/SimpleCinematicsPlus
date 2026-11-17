# Saved files and settings

All SCPlus files live in your DayZ profile folder (`$profile:`), next to Simple Cinematics' own files.

| What | Where |
| --- | --- |
| Presets | `$profile:SimpleCinematics/Presets/*.json` |
| Sequences | `$profile:SimpleCinematics/Sequences/*.json` |
| Backups | `$profile:SimpleCinematics/Backups/Presets/` and `.../Backups/Sequences/` |
| Control settings | Saved to your profile automatically, shown in **Options > Simple Cinematics** |

To share a sequence or preset, copy its `.json` file to a friend's matching folder. Sequences are tied to a position on a map, so they're only useful on the same map.

A sequence file holds the keys, events, actor start, scene target start, waypoint path (with each corner's curve), actor track, and the Loop, Follow Actor and Replay Actor Track settings.

Older sequences keep loading after updates. New fields get default values.

## Backups

Before SCPlus overwrites or deletes a preset or a sequence, it copies the old file to the Backups folder as `<name>.1.json`. The last five versions of each are kept (`.1` is the newest). Names that start with `_` aren't backed up.

To restore one, copy it back to the Presets or Sequences folder and remove the `.1` (or `.2` and so on) from its name.

## Server settings

SCPlus has no settings file of its own. Camera permissions come from Simple Cinematics' `simple_cinematics.json` (`AvailableUsers`).
