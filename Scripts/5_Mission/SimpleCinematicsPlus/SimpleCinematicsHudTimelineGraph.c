// Graph editor, like Blender's Graph Editor / Unreal's curve editor, for one channel at a time
// (pick it with the "Graph Editor" button in the Sequencer section):
//  - the curve of the channel over the same time range as the timeline bar (zoom / pan there)
//  - drag a key up / down to change its value, sideways to retime it
//  - the selected key shows its Bezier handles: drag a handle to shape the curve (both sides stay aligned),
//    hold Shift to move only that side (broken handle)
//  - right-click a key to give it automatic handles again
//  - click or drag on empty space to scrub
// Handles only shape Bezier segments; Linear / Constant / Ease curves ignore them.

modded class SimpleCinematicsHudController
{
	// Graph channels from ACTOR_GRAPH_BASE on show the actor track (Actor X / Height / Z / Heading)
	static const int ACTOR_GRAPH_BASE = 100;
	protected int m_SelectedActorKey = -1;

	int GetGraphChannel()
	{
		return GraphChannel;
	}

	bool IsActorGraph()
	{
		return GraphChannel >= ACTOR_GRAPH_BASE;
	}

	// The channel inside the shown key list (0-8 camera, 0-3 actor)
	int GetGraphCurveChannel()
	{
		if (IsActorGraph()) {
			return GraphChannel - ACTOR_GRAPH_BASE;
		}

		return GraphChannel;
	}

	SCPlusSequenceEvaluator GetGraphEvaluator()
	{
		if (!m_SimpleCinematicsCamera) {
			return null;
		}

		if (IsActorGraph()) {
			return m_SimpleCinematicsCamera.GetActorEvaluator();
		}

		return m_SimpleCinematicsCamera.GetEvaluator();
	}

	// The keys the graph edits: the camera's, or the actor track's
	array<ref SCPlusKeyframe> GetGraphKeys()
	{
		if (!m_Sequence) {
			return null;
		}

		if (IsActorGraph()) {
			return m_Sequence.scp_ActorKeys;
		}

		return m_Sequence.scp_Keys;
	}

	int GetSelectedActorKey()
	{
		return m_SelectedActorKey;
	}

	int GetGraphSelectedKey()
	{
		if (IsActorGraph()) {
			return m_SelectedActorKey;
		}

		return m_SelectedKey;
	}

	protected bool IsGraphKey(int index)
	{
		array<ref SCPlusKeyframe> keys = GetGraphKeys();
		return keys && keys.IsValidIndex(index);
	}

	void BeginGraphEdit()
	{
		PushUndo();
	}

	// Selects a key without moving the playhead
	void SelectKey(int index)
	{
		if (!IsGraphKey(index)) {
			return;
		}

		if (IsActorGraph()) {
			m_SelectedActorKey = index;
			m_SimpleCinematicsCamera.SetActorPreviewSelection(index);
			SyncTimeline();
			return;
		}

		m_SelectedKey = index;
		OnSelectionChanged();
	}

	void SetKeyChannelValue(int index, int channel, float value)
	{
		if (!IsGraphKey(index)) {
			return;
		}

		array<ref SCPlusKeyframe> keys = GetGraphKeys();
		keys[index].SetChannel(channel, value);
		m_SimpleCinematicsCamera.OnSequenceEdited();
	}

	void SetKeyTangents(int index, int channel, float in_slope, float out_slope)
	{
		if (!IsGraphKey(index)) {
			return;
		}

		array<ref SCPlusKeyframe> keys = GetGraphKeys();
		keys[index].SetManualTangent(channel, in_slope, out_slope);
		m_SimpleCinematicsCamera.OnSequenceEdited();
	}

	// Retimes a key of whichever list the graph shows
	void GraphDragKey(int index, float time)
	{
		if (!IsActorGraph()) {
			DragKey(index, time);
			return;
		}

		DragActorKey(index, time);
	}

	// Like DragKey, for the actor track (keys can't pass their neighbours; Ripple Edit moves the later ones)
	void DragActorKey(int index, float time)
	{
		if (!m_Sequence || !m_Sequence.scp_ActorKeys.IsValidIndex(index)) {
			return;
		}

		array<ref SCPlusKeyframe> keys = m_Sequence.scp_ActorKeys;
		float min_time = 0;
		if (index > 0) {
			min_time = keys[index - 1].scp_KeyTime + SCPlusCameraSequence.KEY_TOLERANCE * 2;
		}

		float max_time = 100000;
		if (!RippleEdit && index < keys.Count() - 1) {
			max_time = keys[index + 1].scp_KeyTime - SCPlusCameraSequence.KEY_TOLERANCE * 2;
		}

		float new_time = Math.Clamp(time, min_time, max_time);
		float delta = new_time - keys[index].scp_KeyTime;
		keys[index].scp_KeyTime = new_time;
		if (RippleEdit) {
			for (int later = index + 1; later < keys.Count(); later++) {
				keys[later].scp_KeyTime = keys[later].scp_KeyTime + delta;
			}
		}

		m_SelectedActorKey = index;
		m_SimpleCinematicsCamera.SetActorPreviewSelection(index);
		m_SimpleCinematicsCamera.OnSequenceEdited();
	}

	void ResetKeyTangents(int index, int channel)
	{
		if (!IsGraphKey(index)) {
			return;
		}

		PushUndo();
		array<ref SCPlusKeyframe> keys = GetGraphKeys();
		keys[index].ClearManualTangent(channel);
		FinishGraphEdit();
		Notify("Automatic handles");
	}

	// After a drag: rebuild the key list and put the camera on the edited curve
	void FinishGraphEdit()
	{
		OnSequenceEdited();
		m_SimpleCinematicsCamera.SetPlayhead(m_SimpleCinematicsCamera.GetPlayhead());
		RefreshCameraSliders();
	}
}

modded class SimpleCinematicsHud
{
	static const float SCPLUS_GRAPH_TOP = 26; // px reserved for the title row
	static const float SCPLUS_GRAPH_BOTTOM = 10;
	static const float SCPLUS_GRAPH_HANDLE = 45; // handle length in px
	static const float SCPLUS_GRAPH_HIT = 7;

	static const int SCPLUS_GRAPH_COLOR_GRID = 0x40FFFFFF;
	static const int SCPLUS_GRAPH_COLOR_CURVE = 0xFF5AC8FF;
	static const int SCPLUS_GRAPH_COLOR_HANDLE = 0xFFC8C8C8;

	protected Widget m_GraphPanel;
	protected CanvasWidget m_GraphCanvas;
	protected TextWidget m_GraphTitle;
	protected TextWidget m_GraphMax;
	protected TextWidget m_GraphMid;
	protected TextWidget m_GraphMin;
	protected TextWidget m_GraphHint;
	protected bool m_GraphReady;

	protected bool m_GraphMouseWasDown;
	protected bool m_GraphRightWasDown;
	protected int m_GraphDragKey = -1;
	protected int m_GraphDragMode; // 0 none, 1 key, 2 in-handle, 3 out-handle, 4 scrub
	protected bool m_GraphDragMoved;
	protected int m_GraphDragStartX;
	protected int m_GraphDragStartY;

	// Value range shown; frozen while dragging so the view doesn't jump under the mouse
	protected float m_GraphMinValue;
	protected float m_GraphMaxValue = 1;

	override void Update(float dt)
	{
		super.Update(dt);

		if (!InitGraph()) {
			return;
		}

		SimpleCinematicsHudController controller = GetTemplateController();
		bool show = controller != null && controller.GetSequence() != null && controller.GetSequencerCamera() != null && controller.GetGraphChannel() >= 0 && GetLayoutRoot().IsVisible();
		m_GraphPanel.Show(show);
		if (!show) {
			m_GraphDragMode = 0;
			return;
		}

		int channel = controller.GetGraphCurveChannel();
		m_GraphTitle.SetText(SimpleCinematicsHudController.GetGraphChannelName(controller.GetGraphChannel()));

		float width, height;
		m_GraphPanel.GetScreenSize(width, height);
		// Title on the left, hint right after it (placing the hint against the right edge cut it off)
		m_GraphTitle.SetPos(8, 4);
		m_GraphHint.SetPos(236, 4);

		SCPlusSequenceEvaluator evaluator = controller.GetGraphEvaluator();
		if (!evaluator || evaluator.GetKeyCount() == 0) {
			m_GraphCanvas.Clear();
			if (controller.IsActorGraph()) {
				m_GraphHint.SetText("Record an Auto-Key take to get the actor track");
			} else {
				m_GraphHint.SetText("Set keys to see this channel's curve");
			}

			m_GraphMax.Show(false);
			m_GraphMid.Show(false);
			m_GraphMin.Show(false);
			return;
		}

		m_GraphHint.SetText("Drag keys or handles   Shift: one side   Right-click: auto");
		float length = controller.GetSequence().GetLength();

		if (m_GraphDragMode == 0) {
			ComputeGraphRange(evaluator, channel, length, width);
		}

		UpdateGraphMouse(controller, evaluator, channel, length, width, height);

		// The drag may have rebuilt the evaluator
		evaluator = controller.GetGraphEvaluator();
		if (evaluator && evaluator.GetKeyCount() > 0) {
			DrawGraph(controller, evaluator, channel, length, width, height);
		}
	}

	protected bool InitGraph()
	{
		if (m_GraphReady) {
			return true;
		}

		if (!GetLayoutRoot()) {
			return false;
		}

		m_GraphPanel = GetLayoutRoot().FindAnyWidget("SCPlusGraph");
		m_GraphCanvas = CanvasWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphCanvas"));
		m_GraphTitle = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphTitle"));
		m_GraphMax = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphMax"));
		m_GraphMid = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphMid"));
		m_GraphMin = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphMin"));
		m_GraphHint = TextWidget.Cast(GetLayoutRoot().FindAnyWidget("SCPlusGraphHint"));
		if (!m_GraphPanel || !m_GraphCanvas || !m_GraphTitle || !m_GraphMax || !m_GraphMid || !m_GraphMin || !m_GraphHint) {
			return false;
		}

		m_GraphReady = true;
		return true;
	}

	// --- Value axis ---

	protected void ComputeGraphRange(SCPlusSequenceEvaluator evaluator, int channel, float length, float width)
	{
		float view_start = GetViewStart(length);
		float view_length = GetViewLength(length);

		float min_value = evaluator.EvaluateChannelAt(channel, view_start);
		float max_value = min_value;
		for (int sample = 1; sample <= 100; sample++) {
			float sample_value = evaluator.EvaluateChannelAt(channel, view_start + view_length * sample / 100);
			min_value = Math.Min(min_value, sample_value);
			max_value = Math.Max(max_value, sample_value);
		}

		for (int key = 0; key < evaluator.GetKeyCount(); key++) {
			min_value = Math.Min(min_value, evaluator.GetKeyValue(channel, key));
			max_value = Math.Max(max_value, evaluator.GetKeyValue(channel, key));
		}

		float span = max_value - min_value;
		if (span < 0.5) {
			float middle = (max_value + min_value) / 2;
			min_value = middle - 0.5;
			max_value = middle + 0.5;
			span = 1;
		}

		m_GraphMinValue = min_value - span * 0.12;
		m_GraphMaxValue = max_value + span * 0.12;
	}

	protected float GraphPlotHeight(float height)
	{
		return Math.Max(height - SCPLUS_GRAPH_TOP - SCPLUS_GRAPH_BOTTOM, 1);
	}

	protected float GraphValueToY(float value, float height)
	{
		float fraction = (m_GraphMaxValue - value) / (m_GraphMaxValue - m_GraphMinValue);
		return SCPLUS_GRAPH_TOP + fraction * GraphPlotHeight(height);
	}

	protected float GraphYToValue(float y, float height)
	{
		float fraction = (y - SCPLUS_GRAPH_TOP) / GraphPlotHeight(height);
		return m_GraphMaxValue - fraction * (m_GraphMaxValue - m_GraphMinValue);
	}

	// Seconds per pixel and value per pixel, for turning slopes into screen directions
	protected float GraphSecondsPerPixel(float length, float width)
	{
		return GetViewLength(length) / Math.Max(width - SCPLUS_TIMELINE_MARGIN * 2, 1);
	}

	protected float GraphValuePerPixel(float height)
	{
		return (m_GraphMaxValue - m_GraphMinValue) / GraphPlotHeight(height);
	}

	// Screen offset of a handle end for a slope (value per second), pointing right (side 1) or left (side -1)
	protected void GraphHandleOffset(float slope, int side, float length, float width, float height, out float dx, out float dy)
	{
		float px = 1;
		float py = -slope * GraphSecondsPerPixel(length, width) / GraphValuePerPixel(height);
		float handle_length = Math.Sqrt(px * px + py * py);
		dx = side * px / handle_length * SCPLUS_GRAPH_HANDLE;
		dy = side * py / handle_length * SCPLUS_GRAPH_HANDLE;
	}

	// --- Mouse ---

	protected void UpdateGraphMouse(SimpleCinematicsHudController controller, SCPlusSequenceEvaluator evaluator, int channel, float length, float width, float height)
	{
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = mouse_down && !m_GraphMouseWasDown;
		m_GraphMouseWasDown = mouse_down;
		bool right_down = (GetMouseState(MouseState.RIGHT) & MB_PRESSED_MASK) != 0;
		bool right_pressed = right_down && !m_GraphRightWasDown;
		m_GraphRightWasDown = right_down;

		float panel_x, panel_y;
		m_GraphPanel.GetScreenPos(panel_x, panel_y);
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		float local_x = mouse_x - panel_x;
		float local_y = mouse_y - panel_y;
		bool over_graph = local_x >= 0 && local_x <= width && local_y >= 0 && local_y <= height && GetGame().GetUIManager().IsCursorVisible();

		int selected = controller.GetGraphSelectedKey();

		if (right_pressed && over_graph) {
			int right_key = FindGraphKeyAt(evaluator, channel, local_x, local_y, length, width, height);
			if (right_key != -1) {
				controller.ResetKeyTangents(right_key, channel);
			}

			return;
		}

		if (pressed) {
			if (!over_graph) {
				return;
			}

			m_GraphDragMoved = false;
			m_GraphDragStartX = mouse_x;
			m_GraphDragStartY = mouse_y;

			// Handles of the selected key first, they sit on top
			if (selected >= 0 && selected < evaluator.GetKeyCount()) {
				float key_x = TimelineTimeToX(evaluator.GetKeyTime(selected), length, width);
				float key_y = GraphValueToY(evaluator.GetKeyValue(channel, selected), height);
				float dx, dy;
				GraphHandleOffset(evaluator.GetInTangent(channel, selected), -1, length, width, height, dx, dy);
				if (Math.AbsFloat(local_x - (key_x + dx)) <= SCPLUS_GRAPH_HIT && Math.AbsFloat(local_y - (key_y + dy)) <= SCPLUS_GRAPH_HIT) {
					m_GraphDragKey = selected;
					m_GraphDragMode = 2;
					return;
				}

				GraphHandleOffset(evaluator.GetOutTangent(channel, selected), 1, length, width, height, dx, dy);
				if (Math.AbsFloat(local_x - (key_x + dx)) <= SCPLUS_GRAPH_HIT && Math.AbsFloat(local_y - (key_y + dy)) <= SCPLUS_GRAPH_HIT) {
					m_GraphDragKey = selected;
					m_GraphDragMode = 3;
					return;
				}
			}

			int hit_key = FindGraphKeyAt(evaluator, channel, local_x, local_y, length, width, height);
			if (hit_key != -1) {
				m_GraphDragKey = hit_key;
				m_GraphDragMode = 1;
				controller.SelectKey(hit_key);
				return;
			}

			m_GraphDragMode = 4;
			controller.ScrubTo(TimelineXToTime(local_x, length, width));
			return;
		}

		if (!mouse_down) {
			if (m_GraphDragMode >= 1 && m_GraphDragMode <= 3 && m_GraphDragMoved) {
				controller.FinishGraphEdit();
			}

			m_GraphDragMode = 0;
			m_GraphDragKey = -1;
			return;
		}

		// Held
		if (m_GraphDragMode == 4) {
			controller.ScrubTo(TimelineXToTime(local_x, length, width));
			return;
		}

		if (m_GraphDragMode == 0 || m_GraphDragKey < 0 || m_GraphDragKey >= evaluator.GetKeyCount()) {
			return;
		}

		if (!m_GraphDragMoved) {
			if (Math.AbsInt(mouse_x - m_GraphDragStartX) < 3 && Math.AbsInt(mouse_y - m_GraphDragStartY) < 3) {
				return;
			}

			controller.BeginGraphEdit();
			m_GraphDragMoved = true;
		}

		if (m_GraphDragMode == 1) {
			// Value from the vertical position, time from the horizontal position
			controller.SetKeyChannelValue(m_GraphDragKey, channel, GraphYToValue(local_y, height));
			if (Math.AbsInt(mouse_x - m_GraphDragStartX) >= 4) {
				float time = TimelineXToTime(local_x, length, width);
				if (!KeyState(KeyCode.KC_LCONTROL)) {
					time = Math.Round(time * 10) / 10;
				}

				controller.GraphDragKey(m_GraphDragKey, time);
			}

			return;
		}

		// Handle drag: slope from the vector key -> mouse
		SCPlusSequenceEvaluator current = controller.GetGraphEvaluator();
		if (!current) {
			return;
		}

		float handle_key_x = TimelineTimeToX(current.GetKeyTime(m_GraphDragKey), length, width);
		float handle_key_y = GraphValueToY(current.GetKeyValue(channel, m_GraphDragKey), height);
		float vector_x = local_x - handle_key_x;
		if (m_GraphDragMode == 2) {
			vector_x = Math.Min(vector_x, -2); // in-handle stays left of the key
		} else {
			vector_x = Math.Max(vector_x, 2); // out-handle stays right of the key
		}

		float slope = -(local_y - handle_key_y) * GraphValuePerPixel(height) / (vector_x * GraphSecondsPerPixel(length, width));
		float in_slope = slope;
		float out_slope = slope;
		if (KeyState(KeyCode.KC_LSHIFT) != 0) {
			// Broken handle: keep the other side as it is
			if (m_GraphDragMode == 2) {
				out_slope = current.GetOutTangent(channel, m_GraphDragKey);
			} else {
				in_slope = current.GetInTangent(channel, m_GraphDragKey);
			}
		}

		controller.SetKeyTangents(m_GraphDragKey, channel, in_slope, out_slope);
	}

	protected int FindGraphKeyAt(SCPlusSequenceEvaluator evaluator, int channel, float local_x, float local_y, float length, float width, float height)
	{
		for (int key = 0; key < evaluator.GetKeyCount(); key++) {
			float key_x = TimelineTimeToX(evaluator.GetKeyTime(key), length, width);
			float key_y = GraphValueToY(evaluator.GetKeyValue(channel, key), height);
			if (Math.AbsFloat(local_x - key_x) <= SCPLUS_GRAPH_HIT && Math.AbsFloat(local_y - key_y) <= SCPLUS_GRAPH_HIT) {
				return key;
			}
		}

		return -1;
	}

	// --- Drawing ---

	protected void DrawGraph(SimpleCinematicsHudController controller, SCPlusSequenceEvaluator evaluator, int channel, float length, float width, float height)
	{
		m_GraphCanvas.Clear();
		float left = SCPLUS_TIMELINE_MARGIN;
		float right = width - SCPLUS_TIMELINE_MARGIN;

		// Grid and value labels at top / middle / bottom of the plot
		float mid_value = (m_GraphMaxValue + m_GraphMinValue) / 2;
		array<float> grid_values = {m_GraphMaxValue, mid_value, m_GraphMinValue};
		array<TextWidget> grid_labels = {m_GraphMax, m_GraphMid, m_GraphMin};
		for (int g = 0; g < 3; g++) {
			float grid_y = GraphValueToY(grid_values[g], height);
			m_GraphCanvas.DrawLine(left, grid_y, right, grid_y, 1, SCPLUS_GRAPH_COLOR_GRID);
			grid_labels[g].SetText(FormatGraphValue(grid_values[g]));
			// Labels sit just under their line (the bottom one just above), clear of the title row
			float label_y = grid_y + 2;
			if (g == 2) {
				label_y = grid_y - 17;
			}

			grid_labels[g].SetPos(8, label_y);
			grid_labels[g].Show(true);
		}


		// Curve, sampled every 3 px
		float previous_x = left;
		float previous_y = GraphValueToY(evaluator.EvaluateChannelAt(channel, TimelineXToTime(left, length, width)), height);
		for (float x = left + 3; x <= right; x += 3) {
			float y = GraphValueToY(evaluator.EvaluateChannelAt(channel, TimelineXToTime(x, length, width)), height);
			m_GraphCanvas.DrawLine(previous_x, previous_y, x, y, 2, SCPLUS_GRAPH_COLOR_CURVE);
			previous_x = x;
			previous_y = y;
		}

		// Playhead
		float playhead_x = TimelineTimeToX(controller.GetSequencerCamera().GetPlayhead(), length, width);
		if (playhead_x >= left && playhead_x <= right) {
			m_GraphCanvas.DrawLine(playhead_x, SCPLUS_GRAPH_TOP, playhead_x, height - 2, 1, SCPLUS_COLOR_PLAYHEAD);
		}

		// Keys, and handles on the selected key
		int selected = controller.GetGraphSelectedKey();
		for (int key = 0; key < evaluator.GetKeyCount(); key++) {
			float key_x = TimelineTimeToX(evaluator.GetKeyTime(key), length, width);
			if (key_x < left - 1 || key_x > right + 1) {
				continue;
			}

			float key_y = GraphValueToY(evaluator.GetKeyValue(channel, key), height);
			int color = SCPLUS_COLOR_KEY;
			if (key == selected) {
				color = SCPLUS_COLOR_KEY_SELECTED;
				float dx, dy;
				GraphHandleOffset(evaluator.GetInTangent(channel, key), -1, length, width, height, dx, dy);
				m_GraphCanvas.DrawLine(key_x, key_y, key_x + dx, key_y + dy, 1, SCPLUS_GRAPH_COLOR_HANDLE);
				m_GraphCanvas.DrawLine(key_x + dx - 3, key_y + dy, key_x + dx + 3, key_y + dy, 6, SCPLUS_GRAPH_COLOR_HANDLE);
				GraphHandleOffset(evaluator.GetOutTangent(channel, key), 1, length, width, height, dx, dy);
				m_GraphCanvas.DrawLine(key_x, key_y, key_x + dx, key_y + dy, 1, SCPLUS_GRAPH_COLOR_HANDLE);
				m_GraphCanvas.DrawLine(key_x + dx - 3, key_y + dy, key_x + dx + 3, key_y + dy, 6, SCPLUS_GRAPH_COLOR_HANDLE);
			}

			GraphDiamond(key_x, key_y, 5, color);
		}
	}

	protected void GraphDiamond(float x, float y, float radius, int color)
	{
		for (float offset = -radius; offset <= radius; offset += 1) {
			float half_width = radius - Math.AbsFloat(offset);
			m_GraphCanvas.DrawLine(x - half_width - 0.5, y + offset, x + half_width + 0.5, y + offset, 1, color);
		}
	}

	protected string FormatGraphValue(float value)
	{
		int hundredths = Math.Round(value * 100);
		string sign = "";
		if (hundredths < 0) {
			sign = "-";
			hundredths = -hundredths;
		}

		int fraction = hundredths % 100;
		return string.Format("%1%2.%3", sign, hundredths / 100, fraction.ToStringLen(2));
	}
}
