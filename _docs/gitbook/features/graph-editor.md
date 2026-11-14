# Graph editor

The graph editor shows one channel's curve over time, so you can fine-tune how the camera moves between keys.

1. Open the **Graph Editor** section and pick a channel from the **Channel** dropdown (Position X, Height, Position Z, Yaw, Pitch, Roll, FOV, DOF Distance or DOF Blur).
2. The curve appears above the timeline, with each key and its handles.
3. Drag a **key** to change its value, or drag a **handle** to change the curve's shape.

| Action | Mouse |
| --- | --- |
| Move both handles together | Drag a handle |
| Move one side only | `Shift` + drag a handle |
| Back to automatic handles | Right-click the handle |

A key whose handles you've edited keeps your manual tangents until you reset it. If a channel shows "Set keys to see this channel's curve", the sequence needs more keys first.
