// Camera sequencer UI, modelled on the timeline / dope sheet workflow in Blender, Unreal and Maya:
//  - fly the camera, press Set Key (I): every channel is keyed at the playhead, then the playhead
//    moves on by Key Spacing so the next Set Key makes the next key (fly, I, fly, I...)
//  - scrub Time to move the playhead; the camera shows the sequence at that time
//  - Play / Pause (P) from the playhead, Stop rewinds to the start
//  - Previous / Next Key (Down / Up arrows) jump the playhead between keys and select them
//  - the selected key can be updated from the camera, moved, deleted, or given another curve
// The key list lives in the left panel (the original mod's unused Camera Editor panel); the right
// "Sequencer" section has rows for playback, the selected key, timeline settings and saved sequences.
modded class SimpleCinematicsHudController
{
	ref ObservableCollection<ref ScriptView> SequenceKeys = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceHeader = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceTransport = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceKeyTools = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceSettings = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceToggles = new ObservableCollection<ref ScriptView>(this);
	// "Waypoint Editor" and "Bullet Camera" sections, and the Hold Pose box under the Events buttons
	ref ObservableCollection<ref ScriptView> WaypointToggles = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> WaypointSettings = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> WaypointTools = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> BulletToggles = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> BulletSettings = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> EventToggles = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceFile = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> SequenceList = new ObservableCollection<ref ScriptView>(this);
	// "Graph Editor" section: the channel dropdown and its list
	ref ObservableCollection<ref ScriptView> GraphHeader = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> GraphList = new ObservableCollection<ref ScriptView>(this);

	// Bound to the sequencer controls
	float TimelineTime; // playhead, in seconds
	float SequenceLength = 10;
	float KeySpacing = 2;
	bool SequenceLoop;
	bool SequenceAimAtTarget;
	bool SequenceFollowActor; // camera moves with the actor during playback (saved with the sequence)
	protected ref CheckBoxPrefab m_FollowActorBox;
	bool SequenceAutoKey; // the next Play / Record Take bakes the camera into keys (see SimpleCinematicsCamera)
	protected ref CheckBoxPrefab m_AutoKeyBox;
	protected ref CheckBoxPrefab m_AimAtTargetBox;
	// Checkbox defaults: only Auto-Hide HUD and Ripple Edit start ticked
	bool CameraCollision;
	bool PlaceWaypoints; // a click in the world (cursor shown) adds a waypoint instead of picking a target
	bool ShowPaths;
	float WaypointCurve = 1.0; // the selected waypoint's corner rounding
	protected ref BindableSliderPrefab m_WaypointCurveSlider;

	// Puts the selected waypoint's curve value on the slider
	void SCPlusSyncWaypointCurve()
	{
		if (!m_SimpleCinematicsCamera || !m_WaypointCurveSlider) {
			return;
		}

		SetBoundFloat(m_WaypointCurveSlider, "WaypointCurve", m_SimpleCinematicsCamera.GetWaypointCurve(m_SimpleCinematicsCamera.GetSelectedWaypoint()));
	}
	bool SequenceHideHud = true;
	bool SequenceShowPreview;
	float PlaybackSpeed = 1;
	bool RippleEdit = true; // retiming a key also shifts every key after it
	bool ShowThirdsGrid; // read by the module's framing overlay
	int LetterboxMode; // 0 = off, see GetLetterboxRatio
	int GraphChannel = -1; // channel shown in the graph editor, -1 = graph hidden
	bool HoldPose; // keep the player's emote playing while the camera is active, see SCPlusPoseHold
	// Bullet Cam (see SCPlusBulletCam): follow your next shot from the muzzle to the hit, in slow motion
	bool BulletCam;
	bool BulletCamShowRound;
	bool BulletCamShowPath;
	float BulletCamSpeed = 4; // percent of real speed
	float BulletCamDistance = 1; // metres behind the bullet
	string SequenceName;
	string TimelineReadout; // shown in the left panel header

	protected ref SCPlusCameraSequence m_Sequence;
	protected int m_SelectedKey = -1;
	protected bool m_SyncingTimeline;
	protected float m_TimelineRangeMax = -1;

	// Undo / redo: snapshots of the keys and events taken before every edit (one history for both)
	static const int UNDO_LIMIT = 50;
	protected ref array<ref SCPlusUndoState> m_UndoStack = new array<ref SCPlusUndoState>();
	protected ref array<ref SCPlusUndoState> m_RedoStack = new array<ref SCPlusUndoState>();

	protected ref SCPlusButton m_LetterboxButton;
	protected ref SCPlusDropdownButton m_GraphButton;
	protected ref array<SCPlusDropdownItem> m_GraphRows = new array<SCPlusDropdownItem>();
	protected bool m_GraphListOpen;

	protected ref BindableSliderPrefab m_TimelineSlider;
	protected ref BindableSliderPrefab m_LengthSlider;
	protected ref BindableSliderPrefab m_KeySpacingSlider;
	protected ref CheckBoxPrefab m_LoopBox;
	protected ref CheckBoxPrefab m_HoldPoseBox;
	protected ref EditBoxPrefab m_SequenceNameBox;
	protected ref SCPlusButton m_SelectedKeyButton;
	protected ref SCPlusButton m_InterpolationButton;

	// Sequences dropdown: a combo box that opens and closes the list of saved sequences
	protected ref SCPlusDropdownButton m_SequenceDropdown;
	protected string m_ActiveSequence;
	protected bool m_SequenceListOpen;

	override void SetCamera(SimpleCinematicsCamera camera)
	{
		super.SetCamera(camera);
		SetupSequencer();
	}

	// Called from SetCamera
	void SetupSequencer()
	{
		m_Sequence = new SCPlusCameraSequence();
		m_Sequence.scp_Length = SequenceLength;
		m_SimpleCinematicsCamera.SetSequence(m_Sequence);
		m_SimpleCinematicsCamera.SequenceShowPreview = SequenceShowPreview;
		m_SimpleCinematicsCamera.CameraCollision = CameraCollision;
		m_SimpleCinematicsCamera.ShowPaths = ShowPaths;

		// Saved sequences dropdown
		m_SequenceDropdown = new SCPlusDropdownButton("Sequence", this, "OnSequenceDropdownClicked");
		SequenceHeader.Insert(m_SequenceDropdown);

		// Playback row
		SequenceTransport.Insert(new SCPlusButton("Previous Key", this, "OnPreviousKeyClicked"));
		SequenceTransport.Insert(new SCPlusButton("Play / Pause", this, "OnPlayPauseClicked"));
		SequenceTransport.Insert(new SCPlusButton("Next Key", this, "OnNextKeyClicked"));
		SequenceTransport.Insert(new SCPlusButton("Stop", this, "OnStopClicked"));
		SequenceTransport.Insert(new SCPlusButton("Set Key", this, "OnSetKeyClicked"));
		SequenceTransport.Insert(new SCPlusButton("Clear Keys", this, "OnClearKeysClicked"));
		SequenceTransport.Insert(new SCPlusButton("Record Take", this, "OnRecordTakeClicked"));
		SequenceTransport.Insert(new SCPlusButton("Keys (F1)", this, "OnKeyHelpClicked"));

		// Selected key row
		m_SelectedKeyButton = new SCPlusButton("No key selected", this, "OnSelectedKeyClicked");
		SequenceKeyTools.Insert(m_SelectedKeyButton);
		SequenceKeyTools.Insert(new SCPlusButton("Update Key", this, "OnUpdateKeyClicked"));
		SequenceKeyTools.Insert(new SCPlusButton("Delete Key", this, "OnDeleteKeyClicked"));
		SequenceKeyTools.Insert(new SCPlusButton("Move Key Here", this, "OnMoveKeyClicked"));
		SequenceKeyTools.Insert(new SCPlusButton("Undo", this, "OnUndoClicked"));
		SequenceKeyTools.Insert(new SCPlusButton("Redo", this, "OnRedoClicked"));
		m_InterpolationButton = new SCPlusWideButton("Curve: -", this, "OnCycleInterpolationClicked");
		SequenceKeyTools.Insert(m_InterpolationButton);

		// Timeline settings
		m_TimelineSlider = new BindableSliderPrefab("Scrubber (s)", this, "TimelineTime", 0, SequenceLength, 0.1);
		SequenceSettings.Insert(m_TimelineSlider);
		m_LengthSlider = new BindableSliderPrefab("Runtime (s)", this, "SequenceLength", 1, 300, 1);
		SequenceSettings.Insert(m_LengthSlider);
		// Whole seconds only (rounded in PropertyChanged, the slider itself is continuous)
		m_KeySpacingSlider = new BindableSliderPrefab("Key Step (s)", this, "KeySpacing", 0, 30, 1);
		SequenceSettings.Insert(m_KeySpacingSlider);
		SequenceSettings.Insert(new BindableSliderPrefab("Speed (x)", this, "PlaybackSpeed", 0.25, 2, 0.25));

		// Sequencer toggles in two columns (their own spacer, each checkbox half width)
		m_LoopBox = new CheckBoxPrefab("Loop Playback", this, "SequenceLoop");
		AddToggle(m_LoopBox);
		m_AutoKeyBox = new CheckBoxPrefab("Auto-Key", this, "SequenceAutoKey");
		AddToggle(m_AutoKeyBox);
		AddToggle(new CheckBoxPrefab("Camera Collision", this, "CameraCollision"));
		AddToggle(new CheckBoxPrefab("Auto-Hide HUD", this, "SequenceHideHud"));
		AddToggle(new CheckBoxPrefab("Ripple Edit", this, "RippleEdit"));
		AddToggle(new CheckBoxPrefab("Thirds Grid", this, "ShowThirdsGrid"));

		// Events section: Hold Pose sits under the event buttons
		m_HoldPoseBox = new CheckBoxPrefab("Hold Pose ( , )", this, "HoldPose");
		AddToggleTo(EventToggles, m_HoldPoseBox);

		// Waypoint Editor section (Delete Point / Clear Path are added by the event UI, which owns them)
		AddToggleTo(WaypointToggles, new CheckBoxPrefab("Place Waypoints", this, "PlaceWaypoints"));
		AddToggleTo(WaypointToggles, new CheckBoxPrefab("Show Paths", this, "ShowPaths"));
		AddToggleTo(WaypointToggles, new CheckBoxPrefab("Show Motion Path", this, "SequenceShowPreview"));
		m_FollowActorBox = new CheckBoxPrefab("Follow Actor", this, "SequenceFollowActor");
		AddToggleTo(WaypointToggles, m_FollowActorBox);
		m_AimAtTargetBox = new CheckBoxPrefab("Aim At Target", this, "SequenceAimAtTarget");
		AddToggleTo(WaypointToggles, m_AimAtTargetBox);
		m_WaypointCurveSlider = new BindableSliderPrefab("Waypoint Curve", this, "WaypointCurve", 0, 1, 0.05);
		WaypointSettings.Insert(m_WaypointCurveSlider);

		// Bullet Camera section
		AddToggleTo(BulletToggles, new CheckBoxPrefab("Bullet Cam", this, "BulletCam"));
		AddToggleTo(BulletToggles, new CheckBoxPrefab("Show Round", this, "BulletCamShowRound"));
		AddToggleTo(BulletToggles, new CheckBoxPrefab("Debug Bullet Path", this, "BulletCamShowPath"));
		BulletSettings.Insert(new BindableSliderPrefab("Bullet Speed %", this, "BulletCamSpeed", 1, 25, 1));
		BulletSettings.Insert(new BindableSliderPrefab("Cam Distance", this, "BulletCamDistance", 0.2, 5, 0.1));

		// Letterbox lives with the other framing looks in Camera Filters (Secondary Camera Settings)
		m_LetterboxButton = new SCPlusWideButton("Letterbox: Off", this, "OnLetterboxClicked");
		CameraFilters.Insert(m_LetterboxButton);

		// Graph Editor section: a dropdown with one row per channel (and Off)
		m_GraphButton = new SCPlusDropdownButton("Channel", this, "OnGraphChannelClicked");
		GraphHeader.Insert(m_GraphButton);
		for (int graph_channel = -1; graph_channel < SCPlusKeyframe.CHANNEL_COUNT; graph_channel++) {
			SCPlusDropdownItem graph_row = new SCPlusDropdownItem(GetGraphChannelName(graph_channel), this, "OnGraphChoiceClicked");
			graph_row.SetTag(graph_channel.ToString());
			GraphList.Insert(graph_row);
			m_GraphRows.Insert(graph_row);
		}

		// Actor track channels (recorded by Auto-Key)
		for (int actor_channel = 0; actor_channel < SCPlusKeyframe.ACTOR_CHANNEL_COUNT; actor_channel++) {
			int actor_tag = 100 + actor_channel; // SimpleCinematicsHudController.ACTOR_GRAPH_BASE
			SCPlusDropdownItem actor_row = new SCPlusDropdownItem(GetGraphChannelName(actor_tag), this, "OnGraphChoiceClicked");
			actor_row.SetTag(actor_tag.ToString());
			GraphList.Insert(actor_row);
			m_GraphRows.Insert(actor_row);
		}

		// Saved sequences
		m_SequenceNameBox = new EditBoxPrefab("Sequence", this, "SequenceName");
		SequenceFile.Insert(m_SequenceNameBox);
		SequenceFile.Insert(new SCPlusButton("Save Sequence", this, "OnSaveSequenceClicked"));
		SequenceFile.Insert(new SCPlusButton("Delete Sequence", this, "OnDeleteSequenceClicked"));
		SequenceFile.Insert(new SCPlusButton("New Sequence", this, "OnNewSequenceClicked"));

		RebuildSequenceList();
		SetSequenceListOpen(false);
		UpdateLetterboxCaption();
		SetGraphListOpen(false);
		OnSequenceEdited();
	}

	override void PropertyChanged(string property_name)
	{
		super.PropertyChanged(property_name);

		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}

		switch (property_name) {
			case "TimelineTime": {
				if (m_SyncingTimeline) {
					break;
				}

				// Ignore echoes of our own updates; only a real scrub pauses playback
				if (Math.AbsFloat(TimelineTime - m_SimpleCinematicsCamera.GetPlayhead()) < 0.001) {
					break;
				}

				m_SimpleCinematicsCamera.Pause();
				m_SimpleCinematicsCamera.SetPlayhead(TimelineTime);
				RefreshCameraSliders();
				UpdateReadout();
				break;
			}

			case "SequenceLength": {
				if (m_SyncingTimeline) {
					break;
				}

				m_Sequence.scp_Length = SequenceLength;
				// The timeline can't be shorter than the last key
				if (m_Sequence.GetLength() > SequenceLength + 0.001) {
					SetBoundFloat(m_LengthSlider, "SequenceLength", m_Sequence.GetLength());
				}

				SyncTimeline();
				break;
			}

			case "KeySpacing": {
				if (m_SyncingTimeline) {
					break;
				}

				// Key Step snaps to whole seconds
				float whole_seconds = Math.Round(KeySpacing);
				if (Math.AbsFloat(whole_seconds - KeySpacing) > 0.0001) {
					SetBoundFloat(m_KeySpacingSlider, "KeySpacing", whole_seconds);
				}

				break;
			}

			case "SequenceLoop": {
				m_Sequence.scp_Loop = SequenceLoop;
				m_SimpleCinematicsCamera.SequenceLoop = SequenceLoop;
				break;
			}

			case "SequenceAimAtTarget": {
				m_SimpleCinematicsCamera.SequenceAimAtTarget = SequenceAimAtTarget;
				break;
			}

			case "SequenceFollowActor": {
				m_Sequence.scp_FollowActor = SequenceFollowActor;
				if (SequenceFollowActor && !m_Sequence.scp_HasActorStart) {
					m_SimpleCinematicsCamera.SetActorStart();
					Notify("Follow Actor: actor start set where you stand now");
				}

				break;
			}

			case "SequenceAutoKey": {
				m_SimpleCinematicsCamera.scp_AutoKey = SequenceAutoKey;
				if (SequenceAutoKey) {
					Notify("Auto-Key on: Play or Record Take records the camera into keys");
				}

				break;
			}

			case "CameraCollision": {
				m_SimpleCinematicsCamera.CameraCollision = CameraCollision;
				break;
			}

			case "ShowPaths": {
				m_SimpleCinematicsCamera.ShowPaths = ShowPaths;
				break;
			}

			case "WaypointCurve": {
				m_SimpleCinematicsCamera.SetWaypointCurve(m_SimpleCinematicsCamera.GetSelectedWaypoint(), WaypointCurve);
				break;
			}

			case "SequenceShowPreview": {
				m_SimpleCinematicsCamera.SequenceShowPreview = SequenceShowPreview;
				break;
			}

			case "PlaybackSpeed": {
				m_SimpleCinematicsCamera.PlaybackSpeed = Math.Clamp(PlaybackSpeed, 0.25, 2);
				break;
			}
		}
	}

	// --- Transport ---

	void TogglePlayPause()
	{
		if (!m_SimpleCinematicsCamera.IsPlaying() && m_Sequence.Count() < 2 && !m_SimpleCinematicsCamera.CanBakeTrack()) {
			if (SequenceAutoKey) {
				Notify("Auto-Key: pick a target first (Follow or Focus), or set two keys");
			} else {
				Notify("Set at least two keys to play");
			}

			return;
		}

		m_SimpleCinematicsCamera.TogglePlayPause();
	}

	void StopPlayback()
	{
		m_SimpleCinematicsCamera.Stop();
		RefreshCameraSliders();
		SyncTimeline();
	}

	void JumpToKey(int index)
	{
		if (!m_Sequence.scp_Keys.IsValidIndex(index)) {
			return;
		}

		m_SimpleCinematicsCamera.Pause();
		m_SimpleCinematicsCamera.SetPlayhead(m_Sequence.scp_Keys[index].scp_KeyTime);
		m_SelectedKey = index;
		RefreshCameraSliders();
		OnSelectionChanged();
	}

	void JumpToNextKey()
	{
		int index = m_Sequence.NextKey(m_SimpleCinematicsCamera.GetPlayhead());
		if (index == -1) {
			Notify("No key after the playhead");
			return;
		}

		JumpToKey(index);
	}

	void JumpToPreviousKey()
	{
		int index = m_Sequence.PreviousKey(m_SimpleCinematicsCamera.GetPlayhead());
		if (index == -1) {
			Notify("No key before the playhead");
			return;
		}

		JumpToKey(index);
	}

	// --- Keys ---

	// Keys every channel from the camera's current view at the playhead, then moves the playhead on by
	// Key Spacing so the next Set Key adds the next key. Keying on top of an existing key updates it; the
	// playhead only moves on when that key is the last one.
	void SetKeyAtPlayhead()
	{
		PushUndo();
		m_SimpleCinematicsCamera.Pause();
		float time = m_SimpleCinematicsCamera.GetPlayhead();
		bool updated_existing = (m_Sequence.FindKeyAt(time) != -1);
		m_SelectedKey = m_Sequence.SetKey(m_SimpleCinematicsCamera.CaptureKey(time));

		bool is_last_key = (m_SelectedKey == m_Sequence.Count() - 1);
		if (KeySpacing > 0 && (!updated_existing || is_last_key)) {
			float next_time = time + KeySpacing;
			if (next_time > m_Sequence.scp_Length) {
				m_Sequence.scp_Length = next_time;
				SetBoundFloat(m_LengthSlider, "SequenceLength", next_time);
			}

			// Don't apply the pose: the camera stays where the user flew it
			m_SimpleCinematicsCamera.SetPlayhead(next_time, false);
		}

		if (updated_existing) {
			Notify(string.Format("Updated key at %1 s", FormatSeconds(time)));
		}

		OnSequenceEdited();
	}

	// Re-keys the selected key from the camera's current view, keeping its time
	void UpdateSelectedKey()
	{
		if (!m_Sequence.scp_Keys.IsValidIndex(m_SelectedKey)) {
			Notify("Select a key first");
			return;
		}

		PushUndo();
		float time = m_Sequence.scp_Keys[m_SelectedKey].scp_KeyTime;
		m_SimpleCinematicsCamera.Pause();
		m_SelectedKey = m_Sequence.SetKey(m_SimpleCinematicsCamera.CaptureKey(time));
		Notify(string.Format("Updated key at %1 s", FormatSeconds(time)));
		OnSequenceEdited();
	}

	// Deletes the selected key, or the key under the playhead if none is selected
	void DeleteSelectedKey()
	{
		int index = m_SelectedKey;
		if (!m_Sequence.scp_Keys.IsValidIndex(index)) {
			index = m_Sequence.FindKeyAt(m_SimpleCinematicsCamera.GetPlayhead());
		}

		if (index == -1) {
			Notify("Select a key, or put the playhead on one, to delete it");
			return;
		}

		PushUndo();
		m_Sequence.scp_Keys.RemoveOrdered(index);
		m_SelectedKey = -1;
		OnSequenceEdited();
	}

	void ClearKeys()
	{
		if (m_Sequence.Count() == 0) {
			return;
		}

		PushUndo();
		m_SimpleCinematicsCamera.Pause();
		m_Sequence.scp_Keys.Clear();
		m_SelectedKey = -1;
		m_SimpleCinematicsCamera.SetPlayhead(0, false);
		OnSequenceEdited();
	}

	void CycleSelectedInterpolation()
	{
		if (!m_Sequence.scp_Keys.IsValidIndex(m_SelectedKey)) {
			Notify("Select a key first");
			return;
		}

		PushUndo();
		SCPlusKeyframe key = m_Sequence.scp_Keys[m_SelectedKey];
		key.scp_Interpolation = (key.scp_Interpolation + 1) % SCPlusKeyframe.INTERPOLATION_COUNT;
		OnSequenceEdited();
	}

	void MoveSelectedKeyToPlayhead()
	{
		if (!m_Sequence.scp_Keys.IsValidIndex(m_SelectedKey)) {
			Notify("Select a key first");
			return;
		}

		PushUndo();
		m_SelectedKey = m_Sequence.MoveKey(m_SelectedKey, m_SimpleCinematicsCamera.GetPlayhead());
		OnSequenceEdited();
	}

	// --- Button handlers ---

	// Left panel buttons (Relay_Command in the layout)
	void OnSetKeyExecute(ButtonCommandArgs args)
	{
		SetFocus(null);
		SetKeyAtPlayhead();
	}

	void OnDeleteKeyExecute(ButtonCommandArgs args)
	{
		SetFocus(null);
		DeleteSelectedKey();
	}

	void OnPlayPauseClicked(SCPlusButton button)
	{
		TogglePlayPause();
	}

	void OnStopClicked(SCPlusButton button)
	{
		StopPlayback();
	}

	void OnPreviousKeyClicked(SCPlusButton button)
	{
		JumpToPreviousKey();
	}

	void OnNextKeyClicked(SCPlusButton button)
	{
		JumpToNextKey();
	}

	void OnSetKeyClicked(SCPlusButton button)
	{
		SetKeyAtPlayhead();
	}

	void OnClearKeysClicked(SCPlusButton button)
	{
		ClearKeys();
	}

	void OnSelectedKeyClicked(SCPlusButton button)
	{
		JumpToKey(m_SelectedKey);
	}

	void OnUpdateKeyClicked(SCPlusButton button)
	{
		UpdateSelectedKey();
	}

	void OnDeleteKeyClicked(SCPlusButton button)
	{
		DeleteSelectedKey();
	}

	void OnCycleInterpolationClicked(SCPlusButton button)
	{
		CycleSelectedInterpolation();
	}

	void OnMoveKeyClicked(SCPlusButton button)
	{
		MoveSelectedKeyToPlayhead();
	}

	void OnKeyClicked(SCPlusButton button)
	{
		JumpToKey(button.GetTag().ToInt());
	}

	void OnUndoClicked(SCPlusButton button)
	{
		Undo();
	}

	void OnRedoClicked(SCPlusButton button)
	{
		Redo();
	}

	void OnLetterboxClicked(SCPlusButton button)
	{
		LetterboxMode = (LetterboxMode + 1) % GetLetterboxCount();
		UpdateLetterboxCaption();
	}

	// Cycles the graph editor through Off and each channel
	// Graph Editor section: the dropdown opens the channel list below it; picking a row shows that channel
	void OnGraphChannelClicked(SCPlusButton button)
	{
		SetGraphListOpen(!m_GraphListOpen);
	}

	void OnGraphChoiceClicked(SCPlusButton button)
	{
		GraphChannel = button.GetTag().ToInt();
		SetGraphListOpen(false);
	}

	protected void SetGraphListOpen(bool open)
	{
		m_GraphListOpen = open;
		ShowSection("GraphListContent", open);
		UpdateGraphCaption();
	}

	static string GetGraphChannelName(int channel)
	{
		if (channel < 0) {
			return "Off";
		}

		if (channel >= 100) {
			return SCPlusKeyframe.ActorChannelName(channel - 100);
		}

		return SCPlusKeyframe.ChannelName(channel);
	}

	// Marks the current choice in a dropdown list
	static string MarkCurrentChoice(string caption, bool current)
	{
		return SCPlusDropdownItem.MakeCaption(caption, current);
	}

	// Dropdown field caption: "Label:   value" (the arrow on the right shows it's a list)
	static string DropdownCaption(string label, string value, bool open)
	{
		return SCPlusDropdownButton.MakeCaption(label, value);
	}

	protected void UpdateGraphCaption()
	{
		if (m_GraphButton) {
			m_GraphButton.SetCaption(DropdownCaption("Channel", GetGraphChannelName(GraphChannel), m_GraphListOpen));
		}

		foreach (SCPlusDropdownItem row: m_GraphRows) {
			int row_channel = row.GetTag().ToInt();
			row.SetCaption(MarkCurrentChoice(GetGraphChannelName(row_channel), row_channel == GraphChannel));
		}
	}

	// --- Framing overlay settings (drawn by SimpleCinematicsModule) ---

	static int GetLetterboxCount()
	{
		return 5;
	}

	// Width / height of the letterboxed frame, 0 = off
	float GetLetterboxRatio()
	{
		switch (LetterboxMode) {
			case 1: return 2.39;
			case 2: return 2.0;
			case 3: return 1.85;
			case 4: return 1.0;
		}

		return 0;
	}

	protected void UpdateLetterboxCaption()
	{
		if (!m_LetterboxButton) {
			return;
		}

		string ratio_name = "Off";
		switch (LetterboxMode) {
			case 1: ratio_name = "2.39:1"; break;
			case 2: ratio_name = "2:1"; break;
			case 3: ratio_name = "1.85:1"; break;
			case 4: ratio_name = "1:1"; break;
		}

		m_LetterboxButton.SetCaption("Letterbox: " + ratio_name);
	}

	// --- Playhead nudging (Left / Right arrows) ---

	void TogglePoseHold()
	{
		HoldPose = !HoldPose;
		NotifyPropertyChanged("HoldPose");
		// The checkbox has its own controller, so make it re-read the value (it didn't follow the hotkey)
		if (m_HoldPoseBox) {
			m_HoldPoseBox.ResetToDefault();
		}
		if (HoldPose) {
			Notify("Hold Pose on: camera keys won't cancel your emote");
		} else {
			Notify("Hold Pose off");
		}
	}

	void NudgePlayhead(float delta)
	{
		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}

		m_SimpleCinematicsCamera.Pause();
		m_SimpleCinematicsCamera.SetPlayhead(m_SimpleCinematicsCamera.GetPlayhead() + delta);
		RefreshCameraSliders();
		SyncTimeline();
	}

	// --- Undo / redo ---

	protected SCPlusUndoState SnapshotKeys()
	{
		return SCPlusUndoState.Capture(m_Sequence);
	}

	// Call before any change to the keys or events
	void PushUndo()
	{
		if (!m_Sequence) {
			return;
		}

		m_UndoStack.Insert(SnapshotKeys());
		if (m_UndoStack.Count() > UNDO_LIMIT) {
			m_UndoStack.RemoveOrdered(0);
		}

		m_RedoStack.Clear();
	}

	void Undo()
	{
		if (m_UndoStack.Count() == 0) {
			Notify("Nothing to undo");
			return;
		}

		m_RedoStack.Insert(SnapshotKeys());
		int last = m_UndoStack.Count() - 1;
		RestoreKeys(m_UndoStack[last]);
		m_UndoStack.Remove(last);
		Notify("Undo");
	}

	void Redo()
	{
		if (m_RedoStack.Count() == 0) {
			Notify("Nothing to redo");
			return;
		}

		m_UndoStack.Insert(SnapshotKeys());
		int last = m_RedoStack.Count() - 1;
		RestoreKeys(m_RedoStack[last]);
		m_RedoStack.Remove(last);
		Notify("Redo");
	}

	protected void RestoreKeys(SCPlusUndoState snapshot)
	{
		m_SimpleCinematicsCamera.Pause();
		snapshot.Apply(m_Sequence);

		if (!m_Sequence.scp_Keys.IsValidIndex(m_SelectedKey)) {
			m_SelectedKey = -1;
		}

		SCPlusOnUndoRestored();

		OnSequenceEdited();
	}

	// Overridden by the event UI so a stale event selection is cleared
	protected void SCPlusOnUndoRestored()
	{
	}

	// --- Saved sequences ---

	void RebuildSequenceList()
	{
		SequenceList.Clear();
		array<string> sequence_names = SCPlusCameraSequence.List();
		foreach (string sequence_name: sequence_names) {
			SCPlusDropdownItem sequence_button = new SCPlusDropdownItem(SCPlusDropdownItem.MakeCaption(sequence_name, sequence_name == m_ActiveSequence), this, "OnSequenceClicked");
			sequence_button.SetTag(sequence_name);
			SequenceList.Insert(sequence_button);
		}
	}

	// Marks the active sequence in the list (captions only, see MarkActivePreset)
	protected void MarkActiveSequence()
	{
		for (int i = 0; i < SequenceList.Count(); i++) {
			SCPlusDropdownItem row = SCPlusDropdownItem.Cast(SequenceList.Get(i));
			if (row) {
				row.SetCaption(SCPlusDropdownItem.MakeCaption(row.GetTag(), row.GetTag() == m_ActiveSequence));
			}
		}
	}

	void OnSequenceClicked(SCPlusButton button)
	{
		SCPlusCameraSequence sequence = SCPlusCameraSequence.Load(button.GetTag());
		if (!sequence) {
			Notify(string.Format("Couldn't load sequence '%1'", button.GetTag()));
			RebuildSequenceList();
			return;
		}

		string world = GetDayZGame().GetWorldNameEx();
		if (sequence.scp_MapName != string.Empty && sequence.scp_MapName != world) {
			Notify(string.Format("'%1' was recorded on %2, not %3", button.GetTag(), sequence.scp_MapName, world));
		}

		LoadSequence(sequence);
		SetSequenceName(button.GetTag());
		m_ActiveSequence = button.GetTag();
		SetSequenceListOpen(false);
		Notify(string.Format("Loaded '%1' (%2 keys)", button.GetTag(), sequence.Count()));
	}

	void OnSequenceDropdownClicked(SCPlusButton button)
	{
		SetSequenceListOpen(!m_SequenceListOpen);
	}

	protected void SetSequenceListOpen(bool open)
	{
		m_SequenceListOpen = open;
		ShowSection("SequenceListContent", open);

		string active = m_ActiveSequence;
		if (active == string.Empty) {
			active = "(unsaved)";
		}

		if (m_SequenceDropdown) {
			m_SequenceDropdown.SetCaption(SCPlusDropdownButton.MakeCaption("Sequence", active));
		}

		MarkActiveSequence();
	}

	void OnSaveSequenceClicked(SCPlusButton button)
	{
		string sequence_name = SCPlusPreset.SanitizeName(SequenceName);
		if (sequence_name == string.Empty) {
			Notify("Type a sequence name first");
			return;
		}

		if (m_Sequence.Count() == 0) {
			Notify("Set at least one key before saving");
			return;
		}

		m_Sequence.scp_Name = sequence_name;
		m_Sequence.scp_MapName = GetDayZGame().GetWorldNameEx();
		m_Sequence.scp_Loop = SequenceLoop;
		if (!m_Sequence.Save()) {
			Notify(string.Format("Couldn't save sequence '%1'", sequence_name));
			return;
		}

		Notify(string.Format("Saved sequence '%1' (%2 keys)", sequence_name, m_Sequence.Count()));
		m_ActiveSequence = sequence_name;
		RebuildSequenceList();
		SetSequenceListOpen(m_SequenceListOpen);
	}

	void OnDeleteSequenceClicked(SCPlusButton button)
	{
		string sequence_name = SCPlusPreset.SanitizeName(SequenceName);
		if (!SCPlusCameraSequence.Delete(sequence_name)) {
			Notify("Click a saved sequence (or type its name) first");
			return;
		}

		Notify(string.Format("Deleted sequence '%1'", sequence_name));
		SetSequenceName(string.Empty);
		if (m_ActiveSequence == sequence_name) {
			m_ActiveSequence = string.Empty;
		}

		RebuildSequenceList();
		SetSequenceListOpen(m_SequenceListOpen);
	}

	void OnNewSequenceClicked(SCPlusButton button)
	{
		SCPlusCameraSequence sequence = new SCPlusCameraSequence();
		sequence.scp_Length = SequenceLength;
		LoadSequence(sequence);
		SetSequenceName(string.Empty);
		m_ActiveSequence = string.Empty;
		SetSequenceListOpen(false);
	}

	protected void LoadSequence(notnull SCPlusCameraSequence sequence)
	{
		m_UndoStack.Clear();
		m_RedoStack.Clear();
		m_Sequence = sequence;
		m_SelectedKey = -1;
		m_SimpleCinematicsCamera.SetSequence(m_Sequence);

		SetBoundFloat(m_LengthSlider, "SequenceLength", m_Sequence.GetLength());
		SequenceLoop = m_Sequence.scp_Loop;
		m_LoopBox.ResetToDefault();
		SequenceFollowActor = m_Sequence.scp_FollowActor;
		if (m_FollowActorBox) {
			m_FollowActorBox.ResetToDefault();
		}

		m_SimpleCinematicsCamera.SequenceLoop = SequenceLoop;

		m_SimpleCinematicsCamera.SetPlayhead(0);
		RefreshCameraSliders();
		OnSequenceEdited();
	}

	// --- Sync helpers ---

	// After any change to keys: rebuild the evaluator, the key list and the readouts
	protected void OnSequenceEdited()
	{
		m_SimpleCinematicsCamera.OnSequenceEdited();

		// The timeline grows to fit keys past the end
		if (m_Sequence.GetLength() > SequenceLength + 0.001) {
			SetBoundFloat(m_LengthSlider, "SequenceLength", m_Sequence.GetLength());
		}

		OnSelectionChanged();
	}

	protected void OnSelectionChanged()
	{
		m_SimpleCinematicsCamera.SetPreviewSelection(m_SelectedKey);
		RebuildKeyList();

		string interpolation = "-";
		string selected_caption = "No key selected";
		if (m_Sequence.scp_Keys.IsValidIndex(m_SelectedKey)) {
			SCPlusKeyframe key = m_Sequence.scp_Keys[m_SelectedKey];
			interpolation = SCPlusKeyframe.InterpolationName(key.scp_Interpolation);
			selected_caption = string.Format("Key %1: %2s", m_SelectedKey + 1, FormatSeconds(key.scp_KeyTime));
		}

		m_InterpolationButton.SetCaption("Curve: " + interpolation);
		m_SelectedKeyButton.SetCaption(selected_caption);
		SyncTimeline();
	}

	protected void RebuildKeyList()
	{
		SequenceKeys.Clear();
		foreach (int i, SCPlusKeyframe key: m_Sequence.scp_Keys) {
			string caption = string.Format("%1.  %2 s   %3", i + 1, FormatSeconds(key.scp_KeyTime), SCPlusKeyframe.InterpolationName(key.scp_Interpolation));
			if (i == m_SelectedKey) {
				caption = string.Format("[ %1 ]", caption);
			}

			SCPlusWideButton key_button = new SCPlusWideButton(caption, this, "OnKeyClicked");
			key_button.SetTag(i.ToString());
			SequenceKeys.Insert(key_button);
		}
	}

	// Keeps the Time slider and readout in step with the camera's playhead. Called every frame by the HUD.
	void SyncTimeline()
	{
		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}

		// Time slider range follows the sequence length
		float length = m_Sequence.GetLength();
		if (m_TimelineSlider && Math.AbsFloat(length - m_TimelineRangeMax) > 0.001) {
			m_TimelineRangeMax = length;
			m_SyncingTimeline = true;
			m_TimelineSlider.SetRange(0, length);
			m_SyncingTimeline = false;
		}

		float playhead = m_SimpleCinematicsCamera.GetPlayhead();
		if (Math.AbsFloat(playhead - TimelineTime) > 0.0005) {
			SetBoundFloat(m_TimelineSlider, "TimelineTime", playhead);
		}

		UpdateReadout();
	}

	protected void UpdateReadout()
	{
		string state = "Paused";
		if (m_SimpleCinematicsCamera.IsPlaying()) {
			state = "Playing";
		}

		string readout = string.Format("%1   %2 / %3 s   %4 keys", state, FormatSeconds(m_SimpleCinematicsCamera.GetPlayhead()), FormatSeconds(m_Sequence.GetLength()), m_Sequence.Count());
		string status = GetStatusMessage();
		if (status != string.Empty) {
			readout = status;
		}

		if (readout != TimelineReadout) {
			TimelineReadout = readout;
			NotifyPropertyChanged("TimelineReadout");
		}
	}

	// Sets a slider-bound float from code without the change being treated as user input
	protected void SetBoundFloat(BindableSliderPrefab slider, string variable_name, float value)
	{
		m_SyncingTimeline = true;
		EnScript.SetClassVar(this, variable_name, 0, value);
		if (slider) {
			slider.RefreshFromBinding();
		}

		m_SyncingTimeline = false;
	}

	// Adds a checkbox to the two-column toggle block: half the row wide, with the label taking most of it
	// (the prefab's own split is 25% label / 75% box, which cuts labels short at half width)
	protected void AddToggle(CheckBoxPrefab box)
	{
		AddToggleTo(SequenceToggles, box);
	}

	// Same two-column checkbox, in another section's collection
	protected void AddToggleTo(ObservableCollection<ref ScriptView> collection, CheckBoxPrefab box)
	{
		collection.Insert(box);
		Widget root = box.GetLayoutRoot();
		if (!root) {
			return;
		}

		float width, height;
		root.GetSize(width, height);
		root.SetSize(0.49, height);

		Widget caption = root.FindAnyWidget("Caption");
		if (caption) {
			caption.GetSize(width, height);
			caption.SetSize(0.72, height);
		}

		Widget check = root.FindAnyWidget("CheckBox");
		if (check) {
			check.GetSize(width, height);
			check.SetSize(0.28, height);
		}
	}

	protected void SetSequenceName(string sequence_name)
	{
		SequenceName = sequence_name;
		if (m_SequenceNameBox) {
			m_SequenceNameBox.GetPrefabController().Value = sequence_name;
			m_SequenceNameBox.GetPrefabController().NotifyPropertyChanged("Value");
			scp_NameSetByScript = true;
		}
	}

	static string FormatSeconds(float seconds)
	{
		int hundredths = Math.Round(seconds * 100);
		int whole = hundredths / 100;
		int fraction = hundredths % 100;
		return string.Format("%1.%2", whole, fraction.ToStringLen(2));
	}

	// Both name boxes' current text, to notice typing even when the click wasn't caught
	string SCPlusGetNameBoxText()
	{
		string text;
		if (m_PresetNameBox && m_PresetNameBox.ContentText) {
			text = m_PresetNameBox.ContentText.GetText();
		}

		text += "|";
		if (m_SequenceNameBox && m_SequenceNameBox.ContentText) {
			text += m_SequenceNameBox.ContentText.GetText();
		}

		return text;
	}

	// True when the screen point is over the preset or sequence name box. A screen-rect test, like the fold
	// sections use, because GetWidgetUnderCursor and GetFocus didn't catch these boxes reliably.
	bool SCPlusIsNameBoxAt(int mouse_x, int mouse_y)
	{
		return SCPlusIsEditBoxAt(m_PresetNameBox, mouse_x, mouse_y) || SCPlusIsEditBoxAt(m_SequenceNameBox, mouse_x, mouse_y);
	}

	protected bool SCPlusIsEditBoxAt(EditBoxPrefab box, int mouse_x, int mouse_y)
	{
		if (!box || !box.ContentText || !box.ContentText.IsVisibleHierarchy()) {
			return false;
		}

		float x, y, width, height;
		box.ContentText.GetScreenPos(x, y);
		box.ContentText.GetScreenSize(width, height);
		return mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height;
	}
}

modded class SimpleCinematicsHud
{
	override void Update(float dt)
	{
		super.Update(dt);
		GetTemplateController().SyncTimeline();
	}
}
