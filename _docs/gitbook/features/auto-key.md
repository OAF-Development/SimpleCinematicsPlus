# Auto-Key and actor track

**Auto-Key** records a move for you and turns it into normal, editable keys. Tick it in the **Sequencer** section, then press **Play** or **Record Take**.

Auto-Key isn't saved with the sequence, and it turns itself off after each bake.

## Baking a sequence you've already keyed

With two or more keys, Auto-Key plays the sequence from the start, with **Follow Actor** and **Aim At Target** applied, and records what the camera actually did.

* Let it reach the end and the recording replaces your keys.
* Stop it early and nothing changes.

This is how you turn a "follow my character" or "aim at the target" shot into plain keys that you can fine-tune in the [graph editor](graph-editor.md). After the bake, Follow Actor and Aim At Target are switched off, because the keys already contain the move.

## Tracking a target with no keys

With fewer than two keys, Auto-Key holds your current framing on a target and records it:

1. Pick what to follow: a **Follow Target** or **Focus Target** in the Focus & Follow Target section, or your own character with **Follow Actor** ticked.
2. Frame the shot, tick **Auto-Key** and press **Play**.
3. The camera keeps the same offset from the target as it moves, and aims at the Focus target if you have one.
4. Press **Stop** whenever you like. The recording becomes your keys.

The recording runs for the Runtime or until the last event ends, whichever is longer.

## Clean keys

Auto-Key samples the camera ten times a second and then thins the samples out, keeping only the keys needed to stay within a few centimetres and under a degree of the recorded move. The result is a light set of keys rather than hundreds, and the whole bake is one step in undo (`Ctrl+Z`).

## Actor track

While Auto-Key records, it also records where your character goes and which way they face, as a separate **actor track**. The track is kept only if your character moved at least half a metre or turned at least 20 degrees, so a still take doesn't wipe out an earlier track.

* **See it:** the track's keys are small green diamonds just above the camera keys on the timeline. With **Show Motion Path** on, it's drawn as a green line near the ground with a dot and a heading line for each key.
* **Edit it:** in the [graph editor](graph-editor.md), pick **Actor X**, **Actor Height**, **Actor Z** or **Actor Heading**.
* **Replay it:** tick **Replay Actor Track** in the Waypoint Editor section. Playback then walks your character along the track, choosing walk, jog or sprint to keep up. Follow Actor and Aim At Target use the track, so the framing matches what you keyed.
* **Clear it:** **Clear Actor Track** (click twice; `Ctrl+Z` brings it back).

While Replay Actor Track is on:

* Walk / Jog / Sprint and path events are skipped, because the track moves your character.
* Auto-Key leaves the track as it is, so you can re-bake the camera around an edited walk.

The actor track is saved with the sequence.

> [!NOTE]
> DayZ can only move a character by walking, jogging or sprinting at fixed speeds, so a replay won't land exactly on the track. Expect a few tenths of a metre on a steady walk, and more on sudden speed changes. Where the track stands still, the character stops and turns to the recorded heading.

## A typical workflow

1. Set the actor start, tick **Follow Actor** and frame your character.
2. Add the Walk / Jog / Sprint or path events that move your character, tick **Auto-Key** and press **Play**.
3. Stop. You now have camera keys and an actor track.
4. Tidy the curves in the graph editor, tick **Replay Actor Track** and press **Record Take**.
