// Workflow and UX helpers (must compile after SimpleCinematicsHudTimelineEvents.c, before SimpleCinematicsModule.c):
//  - Record Take and Keys (F1) buttons: requests picked up by the module (SCPlusUIRequests)
//  - timed events (Walk / Jog / Sprint) drawn as bars on the timeline; drag a bar's end to change its
//    duration; Shorter / Longer / Angle buttons for the selected event; clashing events drawn red
//  - "click again to confirm" before deleting or overwriting a sequence or preset (backups are kept too,
//    see SCPlusBackups)
//  - hover tooltips on HUD controls (Dabs Framework TooltipView)
//  - the key cheat-sheet panel (SCPlusKeyHelp), toggled with F1 or the Keys button

// Requests from HUD buttons to the module (the module compiles after this file, so the HUD can't call it)
class SCPlusUIRequests
{
	static bool scp_RecordTake;
	static bool scp_ToggleKeyHelp;
}

// Key cheat-sheet: a panel over the HUD listing every camera and sequencer key. Bound keys are read from
// the current bindings where the game exposes them, so rebinding in Options > Controls shows up here.
class SCPlusKeyHelp
{
	protected static Widget s_Root;

	static bool IsShown()
	{
		return s_Root != null;
	}

	static void Toggle()
	{
		Show(!IsShown());
	}

	static void Show(bool show)
	{
		if (!show) {
			if (s_Root) {
				s_Root.Unlink();
			}

			s_Root = null;
			return;
		}

		if (s_Root) {
			return;
		}

		s_Root = GetGame().GetWorkspace().CreateWidgets("SimpleCinematicsPlus/GUI/layouts/hud/keyhelp.layout");
		if (!s_Root) {
			return;
		}

		MultilineTextWidget text = MultilineTextWidget.Cast(s_Root.FindAnyWidget("SCPlusKeyHelpText"));
		if (text) {
			text.SetText(BuildText());
		}
	}

	// The bound key for an input, or the default if the game doesn't report one
	static string KeyName(string input_name, string fallback)
	{
		string bound = InputUtils.GetButtonNameFromInput(input_name, EUAINPUT_DEVICE_KEYBOARDMOUSE);
		if (bound == string.Empty) {
			return fallback;
		}

		return bound;
	}

	protected static string Line(string keys, string action)
	{
		return string.Format("%1   -   %2\n", keys, action);
	}

	static string BuildText()
	{
		string text = "CAMERA\n";
		text += Line(KeyName("SCCameraToggle", "Insert"), "open / close the camera");
		text += Line(KeyName("SCCameraHudToggle", "Y"), "show / hide the HUD");
		text += Line(KeyName("SCCameraToggleCursor", "Space"), "show / hide the cursor");
		text += Line(KeyName("SCCameraForward", "W") + " " + KeyName("SCCameraLeft", "A") + " " + KeyName("SCCameraBackward", "S") + " " + KeyName("SCCameraRight", "D"), "fly");
		text += Line(KeyName("SCCameraUp", "E") + " / " + KeyName("SCCameraDown", "Q"), "up / down");
		text += Line(KeyName("SCCameraSlow", "Left Alt") + " / " + KeyName("SCCameraTurbo", "Left Shift"), "slow / turbo");
		text += Line("Mouse wheel", "camera speed (steps the ticked quick slider instead)");
		text += Line("Hold right mouse + move up / down", "field of view");
		text += "\nSEQUENCER\n";
		text += Line(KeyName("SCSeqSetKey", "I"), "set key at the playhead");
		text += Line(KeyName("SCSeqPlayPause", "Right Alt"), "play / pause (also cancels a take countdown)");
		text += Line(KeyName("SCSeqNextKey", "Up") + " / " + KeyName("SCSeqPreviousKey", "Down"), "next / previous key");
		text += Line(KeyName("SCSeqDeleteKey", "Delete"), "delete the selected event or key");
		text += Line("Left / Right  (Shift: 1 s)", "nudge the playhead 0.1 s");
		text += Line("Ctrl+Z / Ctrl+Y", "undo / redo key and event edits");
		text += Line(KeyName("SCPlusTogglePoseHold", ","), "toggle Hold Pose");
		text += Line("P", "toggle Place Waypoints");
		text += Line("F1", "show / hide this list");
		text += "\nTIMELINE (cursor shown)\n";
		text += Line("Click / drag the bar", "scrub");
		text += Line("Drag a key or event marker", "retime it (Ctrl: no snapping)");
		text += Line("Drag the end of an event bar", "change a Walk / Jog / Sprint duration");
		text += Line("Mouse wheel / middle drag / double-click", "zoom / pan / show all");
		text += "\nRed event markers clash: one of them may be cut short or skipped.";
		return text;
	}
}

modded class SimpleCinematicsHudController
{
	// --- Event duration and angle ---

	override void SetupSequencer()
	{
		super.SetupSequencer();
		EventTools.Insert(new SCPlusButton("Shorter", this, "OnEventShorterClicked"));
		EventTools.Insert(new SCPlusButton("Longer", this, "OnEventLongerClicked"));
		EventTools.Insert(new SCPlusButton("Angle -45", this, "OnEventAngleLeftClicked"));
		EventTools.Insert(new SCPlusButton("Angle +45", this, "OnEventAngleRightClicked"));
	}

	void OnEventShorterClicked(SCPlusButton button)
	{
		SCPlusAdjustSelectedMove(-0.5, 0);
	}

	void OnEventLongerClicked(SCPlusButton button)
	{
		SCPlusAdjustSelectedMove(0.5, 0);
	}

	void OnEventAngleLeftClicked(SCPlusButton button)
	{
		SCPlusAdjustSelectedMove(0, -45);
	}

	void OnEventAngleRightClicked(SCPlusButton button)
	{
		SCPlusAdjustSelectedMove(0, 45);
	}

	protected SCPlusEvent SCPlusGetSelectedMove()
	{
		if (!HasSelectedEvent()) {
			Notify("Click a Walk / Jog / Sprint marker on the timeline first");
			return null;
		}

		SCPlusEvent event_key = m_Sequence.scp_Events[m_SelectedEvent];
		if (!event_key.IsTimed()) {
			Notify("Only Walk / Jog / Sprint events have a duration and an angle");
			return null;
		}

		return event_key;
	}

	void SCPlusAdjustSelectedMove(float duration_delta, float angle_delta)
	{
		SCPlusEvent event_key = SCPlusGetSelectedMove();
		if (!event_key) {
			return;
		}

		PushUndo();
		event_key.scp_Duration = Math.Clamp(event_key.scp_Duration + duration_delta, 0.5, 60);
		float angle = event_key.scp_Angle + angle_delta;
		if (angle > 180) {
			angle -= 360;
		}

		if (angle < -180) {
			angle += 360;
		}

		event_key.scp_Angle = angle;
		OnSequenceEdited();
		Notify(event_key.GetCaption());
		SCPlusWarnIfClash(m_SelectedEvent);
	}

	// Timeline: dragging the end of an event bar. Returns the event's (unchanged) index.
	protected bool m_DurationDragUndoPushed;

	void SCPlusDragEventEnd(int index, float end_time)
	{
		if (!m_Sequence.scp_Events.IsValidIndex(index)) {
			return;
		}

		SCPlusEvent event_key = m_Sequence.scp_Events[index];
		if (!event_key.IsTimed()) {
			return;
		}

		if (!m_DurationDragUndoPushed) {
			PushUndo();
			m_DurationDragUndoPushed = true;
		}

		m_SelectedEvent = index;
		event_key.scp_Duration = Math.Clamp(end_time - event_key.scp_Time, 0.2, 60);
	}

	void SCPlusFinishEventEndDrag(int index)
	{
		m_DurationDragUndoPushed = false;
		OnSequenceEdited();
		if (m_Sequence.scp_Events.IsValidIndex(index)) {
			Notify(m_Sequence.scp_Events[index].GetCaption());
		}

		SCPlusWarnIfClash(index);
	}

	// --- Record Take / key help buttons ---

	void OnRecordTakeClicked(SCPlusButton button)
	{
		if (m_Sequence.Count() < 2) {
			Notify("Set at least two keys before recording a take");
			return;
		}

		SCPlusUIRequests.scp_RecordTake = true;
	}

	void OnKeyHelpClicked(SCPlusButton button)
	{
		SCPlusUIRequests.scp_ToggleKeyHelp = true;
	}

	bool SCPlusHasSequenceKeys()
	{
		return m_Sequence && (m_Sequence.Count() >= 2 || m_SimpleCinematicsCamera.CanBakeTrack());
	}
	
	// --- Auto-Key ---
	
	// Called when playback stops: swaps the sequence's keys for a finished bake (one undo step)
	void SCPlusCommitBake()
	{
		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}
		
		array<ref SCPlusKeyframe> baked = m_SimpleCinematicsCamera.GetBakeResult();
		string message = m_SimpleCinematicsCamera.GetBakeMessage();
		if (!baked || baked.Count() < 2) {
			if (message != string.Empty) {
				Notify(message);
			}
			
			m_SimpleCinematicsCamera.ClearBakeResult();
			return;
		}
		
		PushUndo();
		m_Sequence.scp_Keys.Clear();
		foreach (SCPlusKeyframe key: baked) {
			m_Sequence.scp_Keys.Insert(key.Clone());
		}
		
		int key_count = baked.Count();
		float recorded = m_SimpleCinematicsCamera.GetBakeRecordedLength();
		m_SimpleCinematicsCamera.ClearBakeResult();
		m_SelectedKey = -1;
		
		// The keys now hold the follow and the aim, so playing them with these on would apply them twice
		m_Sequence.scp_FollowActor = false;
		SequenceFollowActor = false;
		if (m_FollowActorBox) {
			m_FollowActorBox.ResetToDefault();
		}
		
		SequenceAimAtTarget = false;
		m_SimpleCinematicsCamera.SequenceAimAtTarget = false;
		if (m_AimAtTargetBox) {
			m_AimAtTargetBox.ResetToDefault();
		}
		
		SequenceAutoKey = false;
		m_SimpleCinematicsCamera.scp_AutoKey = false;
		if (m_AutoKeyBox) {
			m_AutoKeyBox.ResetToDefault();
		}
		
		OnSequenceEdited();
		Notify(string.Format("Auto-Key: baked %1 keys over %2 s (Ctrl+Z undoes). Follow Actor, Aim At Target and Auto-Key are now off.", key_count, FormatSeconds(recorded)));
	}

	// --- Confirm before deleting or overwriting ---

	static const int SCPLUS_CONFIRM_MS = 4000;
	protected string m_ConfirmAction;
	protected int m_ConfirmUntil;

	// True on the second click of the same action within a few seconds; the first click only explains
	protected bool SCPlusConfirm(string action, string message)
	{
		int now = GetGame().GetTime();
		if (m_ConfirmAction == action && now <= m_ConfirmUntil) {
			m_ConfirmAction = string.Empty;
			return true;
		}

		m_ConfirmAction = action;
		m_ConfirmUntil = now + SCPLUS_CONFIRM_MS;
		Notify(message);
		return false;
	}

	override void OnDeleteSequenceClicked(SCPlusButton button)
	{
		string sequence_name = SCPlusPreset.SanitizeName(SequenceName);
		if (sequence_name != string.Empty && FileExist(SCPlusCameraSequence.GetFilePath(sequence_name))) {
			if (!SCPlusConfirm("delete-sequence:" + sequence_name, string.Format("Click Delete Sequence again to delete '%1' (a backup is kept in SimpleCinematics\\Backups)", sequence_name))) {
				return;
			}
		}

		super.OnDeleteSequenceClicked(button);
	}

	override void OnSaveSequenceClicked(SCPlusButton button)
	{
		string sequence_name = SCPlusPreset.SanitizeName(SequenceName);
		bool other_file = sequence_name != string.Empty && sequence_name != m_ActiveSequence && FileExist(SCPlusCameraSequence.GetFilePath(sequence_name));
		if (other_file && m_Sequence.Count() > 0) {
			if (!SCPlusConfirm("save-sequence:" + sequence_name, string.Format("'%1' already exists. Click Save Sequence again to replace it (a backup is kept)", sequence_name))) {
				return;
			}
		}

		super.OnSaveSequenceClicked(button);
	}

	override void OnDeletePresetClicked(SCPlusButton button)
	{
		string preset_name = SCPlusPreset.SanitizeName(PresetName);
		if (preset_name != string.Empty && FileExist(SCPlusPreset.GetFilePath(preset_name))) {
			if (!SCPlusConfirm("delete-preset:" + preset_name, string.Format("Click Delete Preset again to delete '%1' (a backup is kept in SimpleCinematics\\Backups)", preset_name))) {
				return;
			}
		}

		super.OnDeletePresetClicked(button);
	}

	override void OnSavePresetClicked(SCPlusButton button)
	{
		string preset_name = SCPlusPreset.SanitizeName(PresetName);
		bool other_file = preset_name != string.Empty && preset_name != m_ActivePreset && FileExist(SCPlusPreset.GetFilePath(preset_name));
		if (other_file) {
			if (!SCPlusConfirm("save-preset:" + preset_name, string.Format("'%1' already exists. Click Save Preset again to replace it (a backup is kept)", preset_name))) {
				return;
			}
		}

		super.OnSavePresetClicked(button);
	}

	// --- Tooltips ---

	protected ref map<string, string> m_Tooltips;

	protected void SCPlusTip(string binding_name, string text)
	{
		m_Tooltips.Insert(binding_name, text);
	}

	// Help text per control, keyed by what the control is bound to (a function for buttons, a variable for
	// sliders, checkboxes and text boxes), so captions can change without losing their tooltip
	protected void SCPlusBuildTooltips()
	{
		m_Tooltips = new map<string, string>();

		// Sequencer: transport and keys
		SCPlusTip("OnSequenceDropdownClicked", "Open the list of saved sequences. Click one to load it.");
		SCPlusTip("OnPreviousKeyClicked", "Jump to the key before the playhead and select it.");
		SCPlusTip("OnPlayPauseClicked", "Play the sequence from the playhead, or pause it.");
		SCPlusTip("OnNextKeyClicked", "Jump to the key after the playhead and select it.");
		SCPlusTip("OnStopClicked", "Stop, rewind to the start and put the actor back at the actor start.");
		SCPlusTip("OnSetKeyClicked", "Key every channel from the camera's current view at the playhead, then move the playhead on by Key Step.");
		SCPlusTip("OnClearKeysClicked", "Remove every key (Ctrl+Z brings them back).");
		SCPlusTip("OnRecordTakeClicked", "Clean take for recording: hides the HUD, grid, paths and actor markers, counts down 3-2-1, waits a second, then plays the sequence once from the start.");
		SCPlusTip("OnKeyHelpClicked", "Show every camera and sequencer key (F1).");
		SCPlusTip("OnSelectedKeyClicked", "The selected key. Click a key in the left list or on the timeline to select it.");
		SCPlusTip("OnUpdateKeyClicked", "Overwrite the selected key with the camera's current view.");
		SCPlusTip("OnDeleteKeyClicked", "Delete the selected key.");
		SCPlusTip("OnMoveKeyClicked", "Move the selected key to the playhead.");
		SCPlusTip("OnUndoClicked", "Undo the last key or event edit (Ctrl+Z).");
		SCPlusTip("OnRedoClicked", "Redo the last undone edit (Ctrl+Y).");
		SCPlusTip("OnCycleInterpolationClicked", "Change how the camera moves into the selected key: Bezier (smooth), Linear or Constant (cut).");
		SCPlusTip("OnKeyClicked", "Jump to this key and select it.");

		// Sequencer: settings and toggles
		SCPlusTip("TimelineTime", "The playhead. Drag to scrub through the sequence.");
		SCPlusTip("SequenceLength", "Length of the timeline in seconds. It always grows to fit the last key or event.");
		SCPlusTip("KeySpacing", "How far Set Key moves the playhead on after each key, so fly - I - fly - I builds a move.");
		SCPlusTip("PlaybackSpeed", "Playback speed. Events and actor moves stay in real time.");
		SCPlusTip("WaypointCurve", "Corner rounding of the selected waypoint: 0 is a sharp turn, 1 fully rounded.");
		SCPlusTip("SequenceLoop", "Play the sequence again from the start when it ends. The actor is put back each loop.");
		SCPlusTip("SequenceAimAtTarget", "During playback, keep the camera pointed at the target picked with the cursor (click a player or object).");
		SCPlusTip("SequenceAutoKey", "Bake: the next Play or Record Take records the camera into keys from the start. With no keys, the camera holds your framing on the target (Follow or Focus) and aims at the Focus target. Stopping early keeps nothing, except with no keys.");
		SCPlusTip("SequenceFollowActor", "During playback, move the camera with the actor. Make the keys with the actor standing at the actor start, and the framing follows them as they walk, run or follow a path.");
		SCPlusTip("CameraCollision", "Stop the camera passing through walls and terrain while flying. Off during playback.");
		SCPlusTip("PlaceWaypoints", "Clicking the ground adds a waypoint for path events (P). Click a waypoint disc to select or drag it.");
		SCPlusTip("ShowPaths", "Show the waypoint path strips. Hidden during playback, so they never show in a recording.");
		SCPlusTip("SequenceHideHud", "Hide the HUD and cursor while the sequence plays.");
		SCPlusTip("SequenceShowPreview", "Draw the camera's motion path, keys and look directions in the world while editing.");
		SCPlusTip("RippleEdit", "Retiming a key also shifts every key after it.");
		SCPlusTip("ShowThirdsGrid", "Rule-of-thirds grid for framing. Hidden during Record Take.");
		SCPlusTip("HoldPose", "Keep your character's emote playing while the camera is open (comma).");
		SCPlusTip("BulletCam", "Follow your next shot from the muzzle to the hit, in slow motion.");
		SCPlusTip("BulletCamShowRound", "Show the bullet model during Bullet Cam.");
		SCPlusTip("BulletCamShowPath", "Debug lines for the bullet's path.");
		SCPlusTip("BulletCamSpeed", "Bullet Cam speed, as a percentage of the real bullet speed.");
		SCPlusTip("BulletCamDistance", "How far behind the bullet the camera follows, in metres.");
		SCPlusTip("OnLetterboxClicked", "Cycle the letterbox bars for a cinematic aspect ratio. These do show in recordings.");
		SCPlusTip("OnGraphChannelClicked", "Pick a channel to edit its curve in the graph editor.");

		// Saved sequences and presets
		SCPlusTip("SequenceName", "Name to save the sequence under.");
		SCPlusTip("OnSaveSequenceClicked", "Save the sequence. Replacing a different saved sequence asks you to click twice. The old version goes to Backups.");
		SCPlusTip("OnDeleteSequenceClicked", "Delete the named sequence (click twice). A copy is kept in SimpleCinematics\\Backups\\Sequences.");
		SCPlusTip("OnNewSequenceClicked", "Start an empty sequence. Saved sequences aren't touched.");
		SCPlusTip("OnPresetDropdownClicked", "Open the list of saved camera looks. Click one to apply it.");
		SCPlusTip("PresetName", "Name to save the current look under.");
		SCPlusTip("OnSavePresetClicked", "Save the current camera look (lens, filters, colour, blur). Replacing a different preset asks you to click twice.");
		SCPlusTip("OnDeletePresetClicked", "Delete the named preset (click twice). A copy is kept in SimpleCinematics\\Backups\\Presets.");
		SCPlusTip("OnResetLookClicked", "Put every look setting back to its default.");

		// Events
		SCPlusTip("OnEventTypeClicked", "Pick the type of event that Add Event creates.");
		SCPlusTip("OnAddEventClicked", "Add the chosen event at the playhead. It shows as an orange marker under the timeline.");
		SCPlusTip("OnDeleteEventClicked", "Delete the selected event marker (Delete key).");
		SCPlusTip("OnSetTargetStartClicked", "Remember where the Follow / Focus target (animal, vehicle or player) is now. Play from the start, Stop and each loop put it back. With no target picked, clears it.");
		SCPlusTip("OnSetActorStartClicked", "Remember where your character stands now. Stop and each loop put them back here.");
		SCPlusTip("OnDeleteLastWaypointClicked", "Delete the selected waypoint, or the last one.");
		SCPlusTip("OnClearPathClicked", "Remove every waypoint.");
		SCPlusTip("OnEventShorterClicked", "Make the selected Walk / Jog / Sprint half a second shorter.");
		SCPlusTip("OnEventLongerClicked", "Make the selected Walk / Jog / Sprint half a second longer.");
		SCPlusTip("OnEventAngleLeftClicked", "Turn the selected move's direction 45 degrees one way (relative to where the body faces).");
		SCPlusTip("OnEventAngleRightClicked", "Turn the selected move's direction 45 degrees the other way.");

		// Camera (Simple Cinematics' own sliders)
		SCPlusTip("CameraSpeed", "How fast the camera flies (mouse wheel).");
		SCPlusTip("CameraMouseSensitivity", "How fast the camera turns with the mouse (Left Ctrl + wheel).");
		SCPlusTip("CameraMovementSmoothing", "Higher values ease the camera in and out of moves.");
		SCPlusTip("CameraMouseSmoothing", "Higher values smooth out mouse turns.");
		SCPlusTip("CameraFOV", "Field of view (hold right mouse and move up / down).");
		SCPlusTip("CameraNearPlane", "Closest distance the camera draws. Raise it to cut through nearby grass or walls.");
		SCPlusTip("CameraDOFBlur", "Strength of the depth of field blur.");
		SCPlusTip("CameraDOFDistance", "Focus distance in metres (F + wheel).");
		SCPlusTip("CameraViewDistance", "How far the terrain is drawn.");
		SCPlusTip("CameraObjectViewDistance", "How far objects are drawn.");
		SCPlusTip("LockTime", "Stop the clock at the chosen time (also on the server, if you're allowed).");
	}

	// The HUD control under the screen point, or null. Screen-rect tests, like the fold sections, because
	// GetWidgetUnderCursor misses some prefab widgets.
	ScriptView SCPlusFindViewAt(int mouse_x, int mouse_y)
	{
		array<ObservableCollection<ref ScriptView>> collections = new array<ObservableCollection<ref ScriptView>>();
		collections.Insert(SequenceHeader);
		collections.Insert(SequenceTransport);
		collections.Insert(SequenceKeyTools);
		collections.Insert(SequenceSettings);
		collections.Insert(SequenceToggles);
		collections.Insert(SequenceSettings2);
		collections.Insert(SequenceFile);
		collections.Insert(PresetHeader);
		collections.Insert(PresetControls);
		collections.Insert(EventHeader);
		collections.Insert(EventTools);
		collections.Insert(GraphHeader);
		collections.Insert(CameraOptions);
		collections.Insert(CameraFilters);
		collections.Insert(CameraBlur);
		collections.Insert(CameraColor);
		collections.Insert(DateControls);
		collections.Insert(WeatherControls);
		ScriptView best;
		float best_area = -1;
		foreach (ObservableCollection<ref ScriptView> collection: collections) {
			if (!collection) {
				continue;
			}

			for (int i = 0; i < collection.Count(); i++) {
				ScriptView view = collection.Get(i);
				if (!view || !view.GetLayoutRoot() || !view.GetLayoutRoot().IsVisibleHierarchy()) {
					continue;
				}

				float x, y, width, height;
				view.GetLayoutRoot().GetScreenPos(x, y);
				view.GetLayoutRoot().GetScreenSize(width, height);
				if (mouse_x < x || mouse_x > x + width || mouse_y < y || mouse_y > y + height) {
					continue;
				}

				float area = width * height;
				if (best_area < 0 || area < best_area) {
					best = view;
					best_area = area;
				}
			}
		}

		return best;
	}

	// Tooltip text for a control, or empty if it has none
	string SCPlusGetTooltip(ScriptView view)
	{
		if (!view) {
			return string.Empty;
		}

		if (!m_Tooltips) {
			SCPlusBuildTooltips();
		}

		string binding_name;
		EnScript.GetClassVar(view, "m_BindingName", 0, binding_name);
		string text;
		if (binding_name != string.Empty && m_Tooltips.Find(binding_name, text)) {
			return text;
		}

		return string.Empty;
	}

	// The control's caption, used as the tooltip title
	string SCPlusGetCaption(ScriptView view)
	{
		string caption;
		if (view && view.GetController()) {
			EnScript.GetClassVar(view.GetController(), "Caption", 0, caption);
		}

		return caption;
	}
}

modded class SimpleCinematicsHud
{
	static const int SCPLUS_COLOR_EVENT_BAR = 0xC0FF8C32;
	static const int SCPLUS_COLOR_EVENT_CLASH = 0xFFFF3232;
	static const float SCPLUS_BAR_HANDLE = 5; // px either side of a bar's end that grab it

	protected int m_EventEndDragIndex = -1;
	protected bool m_EventEndMouseWasDown;

	protected ref TooltipView m_SCPlusTooltip;
	protected ScriptView m_SCPlusTooltipView;
	protected int m_SCPlusTooltipSince;

	override void Update(float dt)
	{
		super.Update(dt);
		UpdateTooltip();
	}

	// --- Tooltips: hover a control for half a second ---

	protected void HideTooltip()
	{
		m_SCPlusTooltip = null;
		m_SCPlusTooltipView = null;
	}

	protected void UpdateTooltip()
	{
		SimpleCinematicsHudController controller = GetTemplateController();
		if (!controller || !GetLayoutRoot() || !GetLayoutRoot().IsVisible() || !GetGame().GetUIManager().IsCursorVisible() || (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0) {
			HideTooltip();
			return;
		}

		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		ScriptView view = controller.SCPlusFindViewAt(mouse_x, mouse_y);
		if (view != m_SCPlusTooltipView) {
			HideTooltip();
			m_SCPlusTooltipView = view;
			m_SCPlusTooltipSince = GetGame().GetTime();
		}

		if (!view) {
			return;
		}

		if (!m_SCPlusTooltip && GetGame().GetTime() - m_SCPlusTooltipSince >= 500) {
			string text = controller.SCPlusGetTooltip(view);
			if (text == string.Empty) {
				return;
			}

			string title = controller.SCPlusGetCaption(view);
			if (title == string.Empty) {
				title = text;
				text = string.Empty;
			}

			m_SCPlusTooltip = TooltipView.CreateOnWidget(null, title, TooltipPosition.BOTTOM_RIGHT, text);
		}

		// Below and right of the cursor, kept on screen
		if (m_SCPlusTooltip && m_SCPlusTooltip.GetLayoutRoot()) {
			float tip_w, tip_h;
			m_SCPlusTooltip.GetLayoutRoot().GetScreenSize(tip_w, tip_h);
			int screen_w, screen_h;
			GetScreenSize(screen_w, screen_h);
			float tip_x = Math.Min(mouse_x + 18, screen_w - tip_w - 4);
			float tip_y = mouse_y + 22;
			if (tip_y + tip_h > screen_h - 4) {
				tip_y = mouse_y - tip_h - 8;
			}

			m_SCPlusTooltip.GetLayoutRoot().SetScreenPos(tip_x, tip_y);
		}
	}

	// --- Timed event bars ---

	override protected void UpdateTimelineMouse(SimpleCinematicsHudController controller)
	{
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = mouse_down && !m_EventEndMouseWasDown;
		m_EventEndMouseWasDown = mouse_down;

		SCPlusCameraSequence sequence = controller.GetSequence();
		float length = sequence.GetLength();
		float panel_x, panel_y, width, height;
		m_TimelinePanel.GetScreenPos(panel_x, panel_y);
		m_TimelinePanel.GetScreenSize(width, height);
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		float local_x = mouse_x - panel_x;
		float local_y = mouse_y - panel_y;

		// Dragging a bar's end changes the event's duration
		if (m_EventEndDragIndex != -1) {
			if (!mouse_down) {
				controller.SCPlusFinishEventEndDrag(m_EventEndDragIndex);
				m_EventEndDragIndex = -1;
				return;
			}

			float end_time = TimelineXToTime(local_x, length, width);
			if (!KeyState(KeyCode.KC_LCONTROL)) {
				end_time = Math.Round(end_time * 10) / 10;
			}

			controller.SCPlusDragEventEnd(m_EventEndDragIndex, end_time);
			return;
		}

		bool in_lane = local_x >= 0 && local_x <= width && local_y >= height * 0.62 + 8 && local_y <= height;
		if (pressed && in_lane && GetGame().GetUIManager().IsCursorVisible()) {
			foreach (int i, SCPlusEvent event_key: sequence.scp_Events) {
				if (!event_key.IsTimed()) {
					continue;
				}

				float end_x = TimelineTimeToX(event_key.GetEnd(), length, width);
				float start_x = TimelineTimeToX(event_key.scp_Time, length, width);
				// The start marker wins when the bar is too short to tell them apart
				if (Math.AbsFloat(end_x - local_x) <= SCPLUS_BAR_HANDLE && Math.AbsFloat(start_x - local_x) > SCPLUS_KEY_HIT) {
					m_EventEndDragIndex = i;
					return;
				}
			}
		}

		super.UpdateTimelineMouse(controller);
	}

	override protected void DrawTimeline(SimpleCinematicsHudController controller)
	{
		super.DrawTimeline(controller);

		SCPlusCameraSequence sequence = controller.GetSequence();
		if (sequence.scp_Events.Count() == 0) {
			return;
		}

		float length = sequence.GetLength();
		float width, height;
		m_TimelinePanel.GetScreenSize(width, height);
		float lane_y = height * 0.62 + SCPLUS_EVENT_LANE_OFFSET;
		float left = SCPLUS_TIMELINE_MARGIN;
		float right = width - SCPLUS_TIMELINE_MARGIN;
		int selected = controller.GetSelectedEvent();

		foreach (int i, SCPlusEvent event_key: sequence.scp_Events) {
			bool clash = sequence.FindClash(i) != -1;
			float x = TimelineTimeToX(event_key.scp_Time, length, width);

			// Bar from the start to the end of a timed event, with a handle at its end
			if (event_key.IsTimed()) {
				float end_x = TimelineTimeToX(event_key.GetEnd(), length, width);
				float bar_from = Math.Clamp(x, left, right);
				float bar_to = Math.Clamp(end_x, left, right);
				if (bar_to > bar_from) {
					int bar_color = SCPLUS_COLOR_EVENT_BAR;
					if (clash) {
						bar_color = SCPLUS_COLOR_EVENT_CLASH;
					} else if (i == selected) {
						bar_color = SCPLUS_COLOR_EVENT_SELECTED;
					}

					m_TimelineCanvas.DrawLine(bar_from, lane_y + 5, bar_to, lane_y + 5, 3, bar_color);
					if (end_x >= left && end_x <= right) {
						m_TimelineCanvas.DrawLine(end_x, lane_y + 1, end_x, lane_y + 9, 2, bar_color);
					}
				}
			}

			// Clashing markers are drawn again in red
			if (clash && x >= left - 1 && x <= right + 1) {
				for (float row = 0; row <= 5; row += 1) {
					m_TimelineCanvas.DrawLine(x - row - 0.5, lane_y - 3 + row, x + row + 0.5, lane_y - 3 + row, 1, SCPLUS_COLOR_EVENT_CLASH);
				}
			}
		}
	}
}
