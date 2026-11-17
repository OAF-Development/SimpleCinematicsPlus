// Actor track in the HUD (Waypoint Editor section and the timeline):
//  - Auto-Key records the actor's walk as keys (see SimpleCinematicsCamera, "Actor track")
//  - "Replay Actor Track" walks the actor along them during playback
//  - "Clear Actor Track" deletes them
//  - the Graph Editor edits them (Channel: Actor X / Height / Z / Heading, see SimpleCinematicsHudTimelineGraph)
//  - the timeline shows them as small green diamonds above the camera keys

modded class SimpleCinematicsHudController
{
	bool SequenceReplayActor; // saved with the sequence (scp_ReplayActor)
	protected ref CheckBoxPrefab m_ReplayActorBox;

	override void SetupSequencer()
	{
		super.SetupSequencer();
		m_ReplayActorBox = new CheckBoxPrefab("Replay Actor Track", this, "SequenceReplayActor");
		AddToggleTo(WaypointToggles, m_ReplayActorBox);
		WaypointTools.Insert(new SCPlusButton("Set Actor Key", this, "OnSetActorKeyClicked"));
		WaypointTools.Insert(new SCPlusButton("Clear Actor Track", this, "OnClearActorTrackClicked"));
	}

	// Keys where the actor stands and faces now, at the playhead (one undo step)
	void OnSetActorKeyClicked(SCPlusButton button)
	{
		if (!m_Sequence || !m_SimpleCinematicsCamera) {
			return;
		}

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.IsAlive()) {
			Notify("No actor to key");
			return;
		}

		vector position = player.GetPosition();
		vector body_orientation = player.GetOrientation();
		if (SCPlusEventRunner.SCPlusHasNaN(position) || SCPlusEventRunner.SCPlusHasNaN(body_orientation)) {
			Notify("Couldn't read the actor's position");
			return;
		}

		m_SimpleCinematicsCamera.Pause();
		float time = m_SimpleCinematicsCamera.GetPlayhead();
		array<ref SCPlusKeyframe> keys = m_Sequence.scp_ActorKeys;

		// Keep the heading continuous with the key before it (or after it), so the track doesn't spin round
		float heading = body_orientation[0];
		int neighbour = -1;
		bool replaced = false;
		foreach (int i, SCPlusKeyframe existing: keys) {
			if (Math.AbsFloat(existing.scp_KeyTime - time) <= SCPlusCameraSequence.KEY_TOLERANCE) {
				replaced = true;
				continue;
			}

			if (existing.scp_KeyTime < time || neighbour == -1) {
				neighbour = i;
			}
		}

		if (neighbour != -1) {
			vector neighbour_orientation = keys[neighbour].scp_Orientation;
			heading = SCPlusSequenceEvaluator.UnwrapAngle(heading, neighbour_orientation[0]);
		}

		PushUndo();
		SCPlusKeyframe actor_key = SCPlusKeyframe.Create(time, position, Vector(heading, 0, 0), 70, 0, 0);
		m_SelectedActorKey = SCPlusCameraSequence.InsertSorted(keys, actor_key);
		m_SimpleCinematicsCamera.SetActorPreviewSelection(m_SelectedActorKey);
		OnSequenceEdited();

		string action = "Set";
		if (replaced) {
			action = "Updated";
		}

		string hint = string.Empty;
		if (m_Sequence.HasActorTrack() && !m_Sequence.scp_ReplayActor) {
			hint = ". Tick Replay Actor Track to walk it in playback";
		}

		Notify(string.Format("%1 actor key at %2 s (%3 keys)%4", action, FormatSeconds(time), keys.Count(), hint));
	}

	override void PropertyChanged(string property_name)
	{
		super.PropertyChanged(property_name);
		if (property_name != "SequenceReplayActor" || !m_Sequence) {
			return;
		}

		m_Sequence.scp_ReplayActor = SequenceReplayActor;
		if (!SequenceReplayActor) {
			Notify("Replay Actor Track off: Auto-Key records the actor again");
			return;
		}

		if (!m_Sequence.HasActorTrack()) {
			Notify("Replay Actor Track: no track yet. Record an Auto-Key take with the actor moving first");
			return;
		}

		Notify(string.Format("Replay Actor Track on: the actor walks the %1-key track during playback", m_Sequence.scp_ActorKeys.Count()));
	}

	void OnClearActorTrackClicked(SCPlusButton button)
	{
		if (!m_Sequence || m_Sequence.scp_ActorKeys.Count() == 0) {
			Notify("No actor track to clear");
			return;
		}

		if (!SCPlusConfirm("clear-actor-track", "Click Clear Actor Track again to delete the recorded actor track (Ctrl+Z undoes)")) {
			return;
		}

		PushUndo();
		m_Sequence.scp_ActorKeys.Clear();
		m_SelectedActorKey = -1;
		m_SimpleCinematicsCamera.SetActorPreviewSelection(-1);
		OnSequenceEdited();
		Notify("Actor track cleared");
	}

	override protected void LoadSequence(notnull SCPlusCameraSequence sequence)
	{
		m_SelectedActorKey = -1;
		super.LoadSequence(sequence);
		SyncReplayActorBox();
		if (m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.SetActorPreviewSelection(-1);
		}
	}

	override protected void RestoreKeys(SCPlusUndoState snapshot)
	{
		super.RestoreKeys(snapshot);
		if (m_Sequence && !m_Sequence.scp_ActorKeys.IsValidIndex(m_SelectedActorKey)) {
			m_SelectedActorKey = -1;
			m_SimpleCinematicsCamera.SetActorPreviewSelection(-1);
		}
	}

	protected void SyncReplayActorBox()
	{
		if (!m_Sequence) {
			return;
		}

		SequenceReplayActor = m_Sequence.scp_ReplayActor;
		if (m_ReplayActorBox) {
			m_ReplayActorBox.ResetToDefault();
		}
	}
}

modded class SimpleCinematicsHud
{
	static const int SCPLUS_COLOR_ACTOR_KEY = 0xFF50DC78;
	static const float SCPLUS_ACTOR_LANE_OFFSET = -11; // px above the key track

	override protected void DrawTimeline(SimpleCinematicsHudController controller)
	{
		super.DrawTimeline(controller);

		SCPlusCameraSequence sequence = controller.GetSequence();
		if (!sequence || !sequence.scp_ActorKeys || sequence.scp_ActorKeys.Count() == 0) {
			return;
		}

		float length = sequence.GetLength();
		float width, height;
		m_TimelinePanel.GetScreenSize(width, height);
		float lane_y = height * 0.62 + SCPLUS_ACTOR_LANE_OFFSET;
		float left = SCPLUS_TIMELINE_MARGIN;
		float right = width - SCPLUS_TIMELINE_MARGIN;
		int selected = controller.GetSelectedActorKey();

		foreach (int i, SCPlusKeyframe key: sequence.scp_ActorKeys) {
			float x = TimelineTimeToX(key.scp_KeyTime, length, width);
			if (x < left - 1 || x > right + 1) {
				continue;
			}

			int color = SCPLUS_COLOR_ACTOR_KEY;
			if (i == selected) {
				color = SCPLUS_COLOR_KEY_SELECTED;
			}

			DrawDiamond(x, lane_y, 3, color);
		}
	}
}
