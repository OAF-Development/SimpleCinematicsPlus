# Graph editor

The graph editor shows one channel's curve over time, so you can fine-tune how the camera moves between keys.

1. Open the **Graph Editor** section and pick a channel from the **Channel** dropdown.
2. The curve appears above the timeline, with each key and its handles.
3. Drag a **key** to change its value, or drag a **handle** to change the curve's shape.

| Channels | |
| --- | --- |
| Camera | Position X, Height, Position Z, Yaw, Pitch, Roll, FOV, DOF Distance, DOF Blur |
| Actor track | Actor X, Actor Height, Actor Z, Actor Heading |
| | **Off** hides the graph. |

The **Actor** channels edit the [actor track](auto-key.md#actor-track) that Auto-Key records from your character's walk. They only show a curve once a track exists.

| Action | Mouse |
| --- | --- |
| Move both handles together | Drag a handle |
| Move one side only | `Shift` + drag a handle |
| Back to automatic handles | Right-click the handle |

A key whose handles you've edited keeps your manual tangents until you reset it. If a channel shows "Set keys to see this channel's curve", the sequence needs more keys first. Edits here can be undone with `Ctrl+Z`.
