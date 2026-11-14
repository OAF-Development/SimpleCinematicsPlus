// Blender-style timeline bar along the bottom of the screen:
//  - ruler with second marks, a marker for every key, a red playhead
//  - click or drag on the bar to scrub
//  - click a key marker to select it and jump to it
//  - drag a key marker to retime it (snaps to 0.1 s, hold Ctrl for no snapping; it can't pass its neighbours,
//    with Ripple Edit on every later key moves with it)
//  - mouse wheel over the bar zooms around the cursor, middle-drag pans, double-click resets the view
// Drawn with a CanvasWidget and driven by polling the mouse each frame, so it doesn't depend on widget event routing.

modded class SimpleCinematicsHudController
{
	SCPlusCameraSequence GetSequence()
	{
		return m_Sequence;
	}
	
	int GetSelectedKey()
	{
		return m_SelectedKey;
	}
	
	SimpleCinematicsCamera GetSequencerCamera()
	{
		return m_SimpleCinematicsCamera;
	}
	
	// Scrubbing from the timeline bar
	void ScrubTo(float time)
	{
		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}
		
		m_SimpleCinematicsCamera.Pause();
		m_SimpleCinematicsCamera.SetPlayhead(time);
		RefreshCameraSliders();
		SyncTimeline();
	}
	
	// Called once when a key drag starts, so the whole drag is one undo step
	void BeginKeyDrag()
	{
		PushUndo();
	}
	
	// Retimes a key while it is dragged. Keys can't pass their neighbours, so the order never changes mid-drag.
	// With Ripple Edit, every later key moves by the same amount.
	void DragKey(int index, float time)
	{
		if (!m_Sequence || !m_Sequence.scp_Keys.IsValidIndex(index)) {
			return;
		}
		
		float min_time = 0;
		if (index > 0) {
			min_time = m_Sequence.scp_Keys[index - 1].scp_KeyTime + SCPlusCameraSequence.KEY_TOLERANCE * 2;
		}
		
		float max_time = 100000;
		if (!RippleEdit && index < m_Sequence.Count() - 1) {
			max_time = m_Sequence.scp_Keys[index + 1].scp_KeyTime - SCPlusCameraSequence.KEY_TOLERANCE * 2;
		}
		
		float new_time = Math.Clamp(time, min_time, max_time);
		float delta = new_time - m_Sequence.scp_Keys[index].scp_KeyTime;
		m_Sequence.scp_Keys[index].scp_KeyTime = new_time;
		if (RippleEdit) {
			for (int later = index + 1; later < m_Sequence.Count(); later++) {
				m_Sequence.scp_Keys[later].scp_KeyTime = m_Sequence.scp_Keys[later].scp_KeyTime + delta;
			}
		}
		m_SelectedKey = index;
		m_SimpleCinematicsCamera.SetPreviewSelection(index);
		// Only the curves are rebuilt while dragging; the key list is rebuilt once the drag ends
		m_SimpleCinematicsCamera.OnSequenceEdited();
	}
	
	void FinishKeyDrag()
	{
		OnSequenceEdited();
	}
}

modded class SimpleCinematicsHud
{
	static const int SCPLUS_TIMELINE_LABELS = 11;
	static const float SCPLUS_TIMELINE_MARGIN = 16; // px between the panel edge and 0 s / the end
	static const float SCPLUS_KEY_HIT = 7; // px either side of a key marker that count as clicking it
	
	static const int SCPLUS_COLOR_TRACK = 0xFF5A5A5A;
	static const int SCPLUS_COLOR_TICK = 0xFF8C8C8C;
	static const int SCPLUS_COLOR_RANGE = 0x40FFC832;
	static const int SCPLUS_COLOR_KEY = 0xFFE6E6E6;
	static const int SCPLUS_COLOR_KEY_SELECTED = 0xFFFFC832;
	static const int SCPLUS_COLOR_PLAYHEAD = 0xFFFF3C3C;
	
	protected Widget m_TimelinePanel;
	protected CanvasWidget m_TimelineCanvas;
	protected ref array<TextWidget> m_TimelineLabels;
	protected bool m_TimelineReady;
	
	protected bool m_TimelineMouseWasDown;
	protected bool m_TimelineScrubbing;
	protected int m_TimelineDragKey = -1;
	protected bool m_TimelineDragMoved;
	protected int m_TimelineDragStartX;
	
	// Visible part of the timeline. m_TimelineViewLength 0 = whole sequence.
	protected float m_TimelineViewStart;
	protected float m_TimelineViewLength;
	protected bool m_TimelineMiddleWasDown;
	protected int m_TimelinePanLastX;
	protected int m_TimelineLastClickTime;
	protected ref SCPlusTimelineWheelHandler m_TimelineWheelHandler;
	
	override void Update(float dt)
	{
		super.Update(dt);
		
		if (!InitTimeline() || !m_TimelinePanel.IsVisible() || !GetLayoutRoot().IsVisible()) {
			return;
		}
		
		SimpleCinematicsHudController controller = GetTemplateController();
		if (!controller || !controller.GetSequence() || !controller.GetSequencerCamera()) {
			return;
		}
		
		UpdateTimelineMouse(controller);
		DrawTimeline(controller);
	}
	
	protected bool InitTimeline()
	{
		if (m_TimelineReady) {
			return true;
		}
		
		if (!GetLayoutRoot()) {
			return false;
		}
		
		m_TimelinePanel = GetLayoutRoot().FindAnyWidget("SCPlusTimeline");
		m_TimelineCanvas = CanvasWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusTimelineCanvas"));
		if (!m_TimelinePanel || !m_TimelineCanvas) {
			return false;
		}
		
		m_TimelineLabels = new array<TextWidget>();
		for (int i = 0; i < SCPLUS_TIMELINE_LABELS; i++) {
			TextWidget label = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusTimelineLabel" + i.ToString()));
			if (label) {
				m_TimelineLabels.Insert(label);
			}
		}
		
		// Mouse wheel arrives as a widget event, so the bar gets a small handler of its own
		m_TimelineWheelHandler = new SCPlusTimelineWheelHandler(this);
		m_TimelinePanel.SetHandler(m_TimelineWheelHandler);
		
		m_TimelineReady = true;
		return true;
	}
	
	// --- Coordinates ---
	// "length" is the full sequence length; the visible window is [view start, view start + view length]
	
	protected float GetViewLength(float length)
	{
		if (m_TimelineViewLength <= 0 || m_TimelineViewLength >= length) {
			return length;
		}
		
		return m_TimelineViewLength;
	}
	
	protected float GetViewStart(float length)
	{
		return Math.Clamp(m_TimelineViewStart, 0, Math.Max(length - GetViewLength(length), 0));
	}
	
	protected float TimelineTimeToX(float time, float length, float width)
	{
		float view_length = GetViewLength(length);
		if (view_length <= 0) {
			return SCPLUS_TIMELINE_MARGIN;
		}
		
		return SCPLUS_TIMELINE_MARGIN + ((time - GetViewStart(length)) / view_length) * (width - SCPLUS_TIMELINE_MARGIN * 2);
	}
	
	protected float TimelineXToTime(float x, float length, float width)
	{
		float usable = width - SCPLUS_TIMELINE_MARGIN * 2;
		if (usable <= 0) {
			return 0;
		}
		
		float fraction = Math.Clamp((x - SCPLUS_TIMELINE_MARGIN) / usable, 0, 1);
		return GetViewStart(length) + fraction * GetViewLength(length);
	}
	
	// Mouse wheel over the bar: zoom in / out around the time under the cursor
	void OnTimelineWheel(int x, int wheel)
	{
		SimpleCinematicsHudController controller = GetTemplateController();
		if (!controller || !controller.GetSequence()) {
			return;
		}
		
		float length = controller.GetSequence().GetLength();
		float panel_x, panel_y, width, height;
		m_TimelinePanel.GetScreenPos(panel_x, panel_y);
		m_TimelinePanel.GetScreenSize(width, height);
		
		float anchor_time = TimelineXToTime(x - panel_x, length, width);
		float old_length = GetViewLength(length);
		float new_length = old_length * 0.8;
		if (wheel < 0) {
			new_length = old_length * 1.25;
		}
		
		new_length = Math.Clamp(new_length, Math.Min(0.5, length), length);
		// Keep the time under the cursor in the same place
		float anchor_fraction = (anchor_time - GetViewStart(length)) / old_length;
		m_TimelineViewLength = new_length;
		m_TimelineViewStart = anchor_time - anchor_fraction * new_length;
		m_TimelineViewStart = Math.Clamp(m_TimelineViewStart, 0, Math.Max(length - new_length, 0));
		if (new_length >= length) {
			m_TimelineViewLength = 0;
			m_TimelineViewStart = 0;
		}
	}
	
	// A "nice" ruler step that gives at most SCPLUS_TIMELINE_LABELS labels
	protected float TimelineLabelStep(float length)
	{
		array<float> steps = {0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60, 120};
		foreach (float step: steps) {
			if (length / step <= SCPLUS_TIMELINE_LABELS - 1) {
				return step;
			}
		}
		
		return 300;
	}
	
	// --- Mouse ---
	
	protected void UpdateTimelineMouse(SimpleCinematicsHudController controller)
	{
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = mouse_down && !m_TimelineMouseWasDown;
		m_TimelineMouseWasDown = mouse_down;
		
		SCPlusCameraSequence sequence = controller.GetSequence();
		float length = sequence.GetLength();
		
		float panel_x, panel_y, width, height;
		m_TimelinePanel.GetScreenPos(panel_x, panel_y);
		m_TimelinePanel.GetScreenSize(width, height);
		
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		float local_x = mouse_x - panel_x;
		float local_y = mouse_y - panel_y;
		bool over_bar = local_x >= 0 && local_x <= width && local_y >= 0 && local_y <= height;
		
		// Middle mouse drag pans a zoomed-in view
		bool middle_down = (GetMouseState(MouseState.MIDDLE) & MB_PRESSED_MASK) != 0;
		if (middle_down && !m_TimelineMiddleWasDown && over_bar) {
			m_TimelinePanLastX = mouse_x;
			m_TimelineMiddleWasDown = true;
		} else if (middle_down && m_TimelineMiddleWasDown) {
			float usable_width = Math.Max(width - SCPLUS_TIMELINE_MARGIN * 2, 1);
			m_TimelineViewStart = GetViewStart(length) - (mouse_x - m_TimelinePanLastX) / usable_width * GetViewLength(length);
			m_TimelineViewStart = Math.Clamp(m_TimelineViewStart, 0, Math.Max(length - GetViewLength(length), 0));
			m_TimelinePanLastX = mouse_x;
		} else if (!middle_down) {
			m_TimelineMiddleWasDown = false;
		}
		
		// Start of a click on the bar (only while the cursor is shown, i.e. the user is working in the HUD)
		if (pressed) {
			if (!over_bar || !GetGame().GetUIManager().IsCursorVisible()) {
				return;
			}
			
			// Double-click on the bar shows the whole sequence again
			int now = GetGame().GetTime();
			bool double_click = (now - m_TimelineLastClickTime) < 300;
			m_TimelineLastClickTime = now;
			if (double_click && m_TimelineViewLength > 0) {
				m_TimelineViewLength = 0;
				m_TimelineViewStart = 0;
				return;
			}
			
			m_TimelineDragKey = FindTimelineKeyAt(sequence, local_x, length, width);
			if (m_TimelineDragKey != -1) {
				m_TimelineDragMoved = false;
				m_TimelineDragStartX = mouse_x;
			} else {
				m_TimelineScrubbing = true;
				controller.ScrubTo(TimelineXToTime(local_x, length, width));
			}
			
			return;
		}
		
		// Button released: finish the scrub or the key click / drag
		if (!mouse_down) {
			if (m_TimelineDragKey != -1) {
				if (m_TimelineDragMoved) {
					controller.FinishKeyDrag();
				} else {
					controller.JumpToKey(m_TimelineDragKey);
				}
			}
			
			m_TimelineDragKey = -1;
			m_TimelineScrubbing = false;
			return;
		}
		
		// Button held
		if (m_TimelineScrubbing) {
			controller.ScrubTo(TimelineXToTime(local_x, length, width));
			return;
		}
		
		if (m_TimelineDragKey != -1) {
			if (!m_TimelineDragMoved && Math.AbsInt(mouse_x - m_TimelineDragStartX) < 4) {
				return; // still a click, not a drag
			}
			
			if (!m_TimelineDragMoved) {
				controller.BeginKeyDrag();
			}
			
			m_TimelineDragMoved = true;
			float time = TimelineXToTime(local_x, length, width);
			if (!KeyState(KeyCode.KC_LCONTROL)) {
				time = Math.Round(time * 10) / 10;
			}
			
			controller.DragKey(m_TimelineDragKey, time);
		}
	}
	
	protected int FindTimelineKeyAt(SCPlusCameraSequence sequence, float local_x, float length, float width)
	{
		int best = -1;
		float best_distance = SCPLUS_KEY_HIT;
		foreach (int i, SCPlusKeyframe key: sequence.scp_Keys) {
			float distance = Math.AbsFloat(TimelineTimeToX(key.scp_KeyTime, length, width) - local_x);
			if (distance <= best_distance) {
				best = i;
				best_distance = distance;
			}
		}
		
		return best;
	}
	
	// --- Drawing ---
	
	protected void DrawTimeline(SimpleCinematicsHudController controller)
	{
		SCPlusCameraSequence sequence = controller.GetSequence();
		float length = sequence.GetLength();
		
		float width, height;
		m_TimelinePanel.GetScreenSize(width, height);
		m_TimelineCanvas.Clear();
		
		float track_y = height * 0.62;
		float left = SCPLUS_TIMELINE_MARGIN;
		float right = width - SCPLUS_TIMELINE_MARGIN;
		
		DrawZoomIndicator(length, width, height);
		
		// Highlight the part of the timeline covered by keys
		if (sequence.Count() >= 2) {
			float first_x = TimelineTimeToX(sequence.scp_Keys[0].scp_KeyTime, length, width);
			float last_x = TimelineTimeToX(sequence.scp_Keys[sequence.Count() - 1].scp_KeyTime, length, width);
			first_x = Math.Clamp(first_x, left, right);
			last_x = Math.Clamp(last_x, left, right);
			if (last_x > first_x) {
				m_TimelineCanvas.DrawLine(first_x, track_y, last_x, track_y, 14, SCPLUS_COLOR_RANGE);
			}
		}
		
		m_TimelineCanvas.DrawLine(left, track_y, right, track_y, 2, SCPLUS_COLOR_TRACK);
		
		// Ruler: labelled marks every step, small marks at half steps
		float view_start = GetViewStart(length);
		float view_end = view_start + GetViewLength(length);
		float step = TimelineLabelStep(view_end - view_start);
		int label_index = 0;
		float first_tick = Math.Ceil(view_start / (step / 2) - 0.001) * (step / 2);
		for (float t = first_tick; t <= view_end + 0.001; t += step / 2) {
			float tick_x = TimelineTimeToX(t, length, width);
			bool major = Math.AbsFloat(Math.Round(t / step) * step - t) < 0.001;
			if (major) {
				m_TimelineCanvas.DrawLine(tick_x, track_y - 9, tick_x, track_y + 9, 1, SCPLUS_COLOR_TICK);
				if (label_index < m_TimelineLabels.Count()) {
					TextWidget label = m_TimelineLabels[label_index];
					label.SetText(FormatTimelineLabel(t));
					label.SetPos(tick_x - 30, 2);
					label.Show(true);
					label_index++;
				}
			} else {
				m_TimelineCanvas.DrawLine(tick_x, track_y - 4, tick_x, track_y + 4, 1, SCPLUS_COLOR_TICK);
			}
		}
		
		for (int i = label_index; i < m_TimelineLabels.Count(); i++) {
			m_TimelineLabels[i].Show(false);
		}
		
		// Keys: a diamond each, the selected one highlighted
		int selected = controller.GetSelectedKey();
		foreach (int key_index, SCPlusKeyframe key: sequence.scp_Keys) {
			int color = SCPLUS_COLOR_KEY;
			if (key_index == selected) {
				color = SCPLUS_COLOR_KEY_SELECTED;
			}
			
			float key_x = TimelineTimeToX(key.scp_KeyTime, length, width);
			if (key_x < left - 1 || key_x > right + 1) {
				continue; // outside the zoomed view
			}
			
			DrawDiamond(key_x, track_y, 6, color);
		}
		
		// Playhead
		float playhead_x = TimelineTimeToX(controller.GetSequencerCamera().GetPlayhead(), length, width);
		if (playhead_x < left - 1 || playhead_x > right + 1) {
			return;
		}
		
		m_TimelineCanvas.DrawLine(playhead_x, 2, playhead_x, height - 2, 2, SCPLUS_COLOR_PLAYHEAD);
		m_TimelineCanvas.DrawLine(playhead_x - 5, 3, playhead_x + 5, 3, 4, SCPLUS_COLOR_PLAYHEAD);
	}
	
	// Filled diamond made of horizontal lines that narrow towards the top and bottom
	protected void DrawDiamond(float x, float y, float radius, int color)
	{
		for (float offset = -radius; offset <= radius; offset += 1) {
			float half_width = radius - Math.AbsFloat(offset);
			m_TimelineCanvas.DrawLine(x - half_width - 0.5, y + offset, x + half_width + 0.5, y + offset, 1, color);
		}
	}
	
	// Zoom indicator: a thin bar under the ruler showing which part of the sequence is visible
	protected void DrawZoomIndicator(float length, float width, float height)
	{
		if (GetViewLength(length) >= length) {
			return;
		}
		
		float usable = width - SCPLUS_TIMELINE_MARGIN * 2;
		float x0 = SCPLUS_TIMELINE_MARGIN + GetViewStart(length) / length * usable;
		float x1 = x0 + GetViewLength(length) / length * usable;
		m_TimelineCanvas.DrawLine(x0, height - 3, x1, height - 3, 3, SCPLUS_COLOR_TICK);
	}
	
	protected string FormatTimelineLabel(float seconds)
	{
		int tenths = Math.Round(seconds * 10);
		if (tenths % 10 == 0) {
			return (tenths / 10).ToString() + "s";
		}
		
		return string.Format("%1.%2s", tenths / 10, tenths % 10);
	}
}

// Forwards mouse wheel events on the timeline bar to the HUD
class SCPlusTimelineWheelHandler: ScriptedWidgetEventHandler
{
	protected SimpleCinematicsHud m_Hud;
	
	void SCPlusTimelineWheelHandler(SimpleCinematicsHud hud)
	{
		m_Hud = hud;
	}
	
	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (m_Hud) {
			m_Hud.OnTimelineWheel(x, wheel);
		}
		
		return true;
	}
}
