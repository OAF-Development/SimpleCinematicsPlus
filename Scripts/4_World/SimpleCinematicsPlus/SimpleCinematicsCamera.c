modded class SimpleCinematicsCamera
{
	// Look values the camera started with, used by ResetLook()
	protected ref SCPlusPreset m_DefaultLook;

	// True while applying saved values, so loading doesn't immediately trigger a save
	protected bool m_SuppressSave;

	// Delay before writing settings to disk after the last change
	static const int SCPLUS_SAVE_DELAY_MS = 1000;

	// Last camera position/orientation without NaN; restored if a frame produces NaN (a NaN transform turns the
	// screen white and throws every frame)
	protected vector scp_LastGoodPosition;
	protected vector scp_LastGoodOrientation;
	protected bool scp_HasGoodTransform;

	// Camera collision (free flying only; playback follows the keys exactly). The camera stops this far from
	// walls, objects and the ground, and slides along them instead of passing through.
	bool CameraCollision; // off by default (the HUD's checkbox sets it)

	// Waypoint path visuals (hidden while playing)
	bool ShowPaths;

	array<vector> GetWaypoints()
	{
		if (!m_Sequence) {
			return null;
		}

		return m_Sequence.scp_Waypoints;
	}

	protected int scp_SelectedWaypoint = -1;

	int GetSelectedWaypoint()
	{
		if (!m_Sequence || !m_Sequence.scp_Waypoints.IsValidIndex(scp_SelectedWaypoint)) {
			return -1;
		}

		return scp_SelectedWaypoint;
	}

	void SelectWaypoint(int index)
	{
		scp_SelectedWaypoint = index;
		SCPlusPathVisuals.MarkDirty();
	}

	array<float> GetWaypointCurves()
	{
		if (!m_Sequence) {
			return null;
		}

		return m_Sequence.scp_WaypointCurves;
	}

	// The walkable path: the waypoints with their corners rounded (what the visuals show and PATH follows)
	array<vector> GetPathCurve()
	{
		if (!m_Sequence) {
			return new array<vector>();
		}

		return SCPlusPathVisuals.BuildCurve(m_Sequence.scp_Waypoints, m_Sequence.scp_WaypointCurves);
	}

	// Keeps one curve value per waypoint (older sequences have none)
	protected void SCPlusSyncCurveCount()
	{
		array<float> curves = m_Sequence.scp_WaypointCurves;
		while (curves.Count() < m_Sequence.scp_Waypoints.Count()) {
			curves.Insert(SCPlusPathVisuals.DEFAULT_CURVE);
		}

		while (curves.Count() > m_Sequence.scp_Waypoints.Count()) {
			curves.Remove(curves.Count() - 1);
		}
	}

	// Adds a waypoint after the selected one (or at the end) and selects it. The first click on an empty path
	// also drops point 1 where the player stands, so every path starts at the actor and chains from there.
	void AddWaypoint(vector position)
	{
		if (!m_Sequence) {
			return;
		}

		SCPlusSyncCurveCount();
		if (m_Sequence.scp_Waypoints.Count() == 0) {
			PlayerBase actor = PlayerBase.Cast(GetGame().GetPlayer());
			if (actor) {
				m_Sequence.scp_Waypoints.Insert(actor.GetPosition());
				m_Sequence.scp_WaypointCurves.Insert(SCPlusPathVisuals.DEFAULT_CURVE);
				SelectWaypoint(0);
			}
		}

		int index = m_Sequence.scp_Waypoints.Count();
		int selected = GetSelectedWaypoint();
		if (selected >= 0 && selected < index - 1) {
			index = selected + 1;
			m_Sequence.scp_Waypoints.InsertAt(position, index);
			m_Sequence.scp_WaypointCurves.InsertAt(SCPlusPathVisuals.DEFAULT_CURVE, index);
		} else {
			m_Sequence.scp_Waypoints.Insert(position);
			m_Sequence.scp_WaypointCurves.Insert(SCPlusPathVisuals.DEFAULT_CURVE);
		}

		SelectWaypoint(index);
		// Print("[SCPlus Paths] waypoint " + (index + 1).ToString() + " at " + position.ToString());
	}

	void MoveWaypoint(int index, vector position)
	{
		if (!m_Sequence || !m_Sequence.scp_Waypoints.IsValidIndex(index)) {
			return;
		}

		m_Sequence.scp_Waypoints.Set(index, position);
		SCPlusPathVisuals.MarkDirty();
	}

	// Deletes the selected waypoint (or the last one); the one before it becomes selected
	bool RemoveWaypoint()
	{
		if (!m_Sequence || m_Sequence.scp_Waypoints.Count() == 0) {
			return false;
		}

		SCPlusSyncCurveCount();
		int index = GetSelectedWaypoint();
		if (index < 0) {
			index = m_Sequence.scp_Waypoints.Count() - 1;
		}

		m_Sequence.scp_Waypoints.RemoveOrdered(index);
		m_Sequence.scp_WaypointCurves.RemoveOrdered(index);
		SelectWaypoint(Math.Min(index, m_Sequence.scp_Waypoints.Count()) - 1);
		return true;
	}

	void ClearWaypoints()
	{
		if (!m_Sequence) {
			return;
		}

		m_Sequence.scp_Waypoints.Clear();
		m_Sequence.scp_WaypointCurves.Clear();
		SelectWaypoint(-1);
	}

	float GetWaypointCurve(int index)
	{
		if (!m_Sequence) {
			return SCPlusPathVisuals.DEFAULT_CURVE;
		}

		return SCPlusPathVisuals.GetCurve(m_Sequence.scp_WaypointCurves, index);
	}

	void SetWaypointCurve(int index, float value)
	{
		if (!m_Sequence || !m_Sequence.scp_Waypoints.IsValidIndex(index)) {
			return;
		}

		SCPlusSyncCurveCount();
		m_Sequence.scp_WaypointCurves.Set(index, Math.Clamp(value, 0, 1));
		SCPlusPathVisuals.MarkDirty();
	}
static const float SCPLUS_COLLISION_MARGIN = 0.2;

	protected vector SCPlusCollide(vector from, vector to)
	{
		vector result = to;
		vector travel = to - from;
		float travel_length = travel.Length();
		if (travel_length > 0.0001) {
			vector direction = travel.Normalized();
			vector contact_pos, contact_dir;
			int contact_component;
			set<Object> results = new set<Object>();
			Object ignore = GetGame().GetPlayer();
			vector ray_end = to + direction * SCPLUS_COLLISION_MARGIN;
			if (DayZPhysics.RaycastRV(from, ray_end, contact_pos, contact_dir, contact_component, results, null, ignore, false, false, ObjIntersectGeom, 0)) {
				vector normal = contact_dir.Normalized();
				if (SCPlusIsNaN(normal) || normal.Length() < 0.5) {
					result = contact_pos - direction * SCPLUS_COLLISION_MARGIN;
				} else {
					// Push the destination back out to the margin along the surface normal: the camera slides
					float depth = vector.Dot(to - contact_pos, normal);
					if (depth < SCPLUS_COLLISION_MARGIN) {
						result = to + normal * (SCPLUS_COLLISION_MARGIN - depth);
					}
				}
			}
		}

		float ground = GetGame().SurfaceY(result[0], result[2]) + SCPLUS_COLLISION_MARGIN;
		if (result[1] < ground) {
			result[1] = ground;
		}

		if (SCPlusIsNaN(result)) {
			return from;
		}

		return result;
	}

	protected float scp_NaNLogTimer;
	protected int scp_NaNFrames;

	static bool SCPlusIsNaNFloat(float value)
	{
		return value != value;
	}

	// A NaN in any of these makes every following frame NaN too (they multiply the movement each frame)
	protected void SCPlusRepairNaNState(vector transform[4], float time_slice)
	{
		scp_NaNFrames++;
		string bad;
		if (SCPlusIsNaN(transform[0]) || SCPlusIsNaN(transform[1]) || SCPlusIsNaN(transform[2])) {
			bad += " rotation";
		}

		if (SCPlusIsNaN(transform[3])) {
			bad += " position";
		}

		if (SCPlusIsNaNFloat(CameraSpeed)) {
			bad += " CameraSpeed";
			CameraSpeed = 100;
		}

		if (SCPlusIsNaNFloat(CameraMovementSmoothing)) {
			bad += " CameraMovementSmoothing";
			CameraMovementSmoothing = 0;
		}

		if (SCPlusIsNaNFloat(CameraMouseSmoothing)) {
			bad += " CameraMouseSmoothing";
			CameraMouseSmoothing = 0;
		}

		if (SCPlusIsNaNFloat(CameraMouseSensitivity)) {
			bad += " CameraMouseSensitivity";
			CameraMouseSensitivity = 35;
		}

		if (SCPlusIsNaN(FollowedTargetDeltaPos)) {
			bad += " FollowedTargetDeltaPos";
			FollowedTargetDeltaPos = vector.Zero;
		}

		if (SCPlusIsNaN(linearVelocity) || SCPlusIsNaN(angularVelocity)) {
			bad += " velocity";
		}

		if (!scp_HasGoodTransform || SCPlusIsNaN(scp_LastGoodPosition)) {
			bad += " (no good transform to restore)";
		}

		scp_NaNLogTimer -= time_slice;
		if (scp_NaNLogTimer <= 0) {
			// Print(string.Format("[SCPlus Camera] NaN camera transform (%1 frames):%2. Restored the last good one.", scp_NaNFrames, bad));
			scp_NaNLogTimer = 1;
			scp_NaNFrames = 0;
		}
	}

	static bool SCPlusIsNaN(vector v)
	{
		return v[0] != v[0] || v[1] != v[1] || v[2] != v[2];
	}

	// --- Sequencer ---
	// The sequence itself is owned by the HUD; the camera evaluates and plays it
	protected SCPlusCameraSequence m_Sequence;
	protected ref SCPlusSequenceEvaluator m_Evaluator;
	protected ref SCPlusSequenceEvaluator scp_ActorEvaluator; // the actor track's curves (null without a track)
	protected int scp_PreviewActorSelected = -1;
	protected float m_Playhead;
	protected bool m_Playing;
	protected float m_SpectatorTimer;
	protected bool m_ReachedEnd;
	protected bool m_PlaybackJustStarted; // the first frame also runs events exactly at the start time

	bool SequenceLoop;
	float PlaybackSpeed = 1; // 0.25x .. 2x
	bool SequenceAimAtTarget;
	bool SequenceShowPreview; // off by default (the HUD's "Show Motion Path" checkbox sets it)

	// Show Motion Path, filled by DrawSequencePreview each editing frame and drawn by the module on its overlay
	// canvas (Shape lines don't render in the retail game, which is why the path never showed). World space.
	static ref array<vector> scp_PreviewFrom = new array<vector>();
	static ref array<vector> scp_PreviewTo = new array<vector>();
	static ref array<int> scp_PreviewColor = new array<int>();
	static ref array<float> scp_PreviewWidth = new array<float>();
	static ref array<vector> scp_PreviewDots = new array<vector>();
	static ref array<int> scp_PreviewDotColor = new array<int>();
	static ref array<float> scp_PreviewDotSize = new array<float>(); // metres

	static void SCPlusClearPreview()
	{
		scp_PreviewFrom.Clear();
		scp_PreviewTo.Clear();
		scp_PreviewColor.Clear();
		scp_PreviewWidth.Clear();
		scp_PreviewDots.Clear();
		scp_PreviewDotColor.Clear();
		scp_PreviewDotSize.Clear();
	}

	protected static void SCPlusPreviewLine(vector from, vector to, int color, float width)
	{
		scp_PreviewFrom.Insert(from);
		scp_PreviewTo.Insert(to);
		scp_PreviewColor.Insert(color);
		scp_PreviewWidth.Insert(width);
	}

	protected static void SCPlusPreviewDot(vector position, int color, float size)
	{
		scp_PreviewDots.Insert(position);
		scp_PreviewDotColor.Insert(color);
		scp_PreviewDotSize.Insert(size);
	}
	protected int m_PreviewSelected = -1;

	// Invoked with (SimpleCinematicsCamera camera, bool playing) when playback starts or stops
	ref ScriptInvoker OnPlaybackStateChanged = new ScriptInvoker();

	static const int PREVIEW_COLOR_CURVE = 0xFFFFC832;
	static const int PREVIEW_COLOR_KEY = 0xC8FFFFFF;
	static const int PREVIEW_COLOR_SELECTED = 0xFF32C8FF;
	static const int PREVIEW_COLOR_PLAYHEAD = 0xFFFF3232;
	static const float PREVIEW_SAMPLE_STEP = 0.1; // seconds between curve samples

	// True while a text box has keyboard focus, so typing doesn't fly the camera or fire hotkeys
	// Set by the HUD when a text box is clicked (cleared by a click elsewhere, Enter or Escape)
	static bool scp_TypingInHud;
	protected float scp_TypingLogTimer;

	static bool IsTypingInUI()
	{
		if (scp_TypingInHud) {
			return true;
		}

		Widget focus = GetFocus();
		return focus && (focus.IsInherited(EditBoxWidget) || focus.IsInherited(MultilineEditBoxWidget));
	}

	// Fix: the original crashed when given null
	override void SetFollowTarget(Object target)
	{
		if (!target) {
			FollowedTarget = null;
			FollowedTargetDeltaPos = vector.Zero;
			return;
		}

		super.SetFollowTarget(target);
	}

	override void PropertyChanged(string property_name)
	{
		super.PropertyChanged(property_name);
		ScheduleSave();
	}

	// Restores the user's saved control settings and their last look. Call once after creating the camera.
	void ApplySavedState()
	{
		m_SuppressSave = true;

		m_DefaultLook = SCPlusPreset.Capture("_Default", this);

		SCPlusSettings settings = SCPlusSettings.GetInstance();
		if (settings) {
			settings.CopyTo(this);
		}

		// First run (no last look saved yet) starts from the built-in Default preset
		SCPlusPreset last_look = SCPlusPreset.Load(SCPlusPreset.LAST_LOOK);
		if (!last_look) {
			last_look = SCPlusPreset.Load(SCPlusPreset.BUILTIN_DEFAULT);
		}

		if (last_look) {
			last_look.ApplyTo(this);
		}

		m_SuppressSave = false;
	}

	// Both return how many values were applied
	int ApplyPreset(notnull SCPlusPreset preset)
	{
		return preset.ApplyTo(this);
	}

	int ResetLook()
	{
		// The mod's own defaults for every value, then the Default preset's Camera Options on top
		int applied;
		if (m_DefaultLook) {
			applied = m_DefaultLook.ApplyTo(this);
		}

		SCPlusPreset default_preset = SCPlusPreset.Load(SCPlusPreset.BUILTIN_DEFAULT);
		if (default_preset) {
			default_preset.ApplyTo(this);
		}

		return applied;
	}

	// Debounced: sliders fire every frame while dragged, so wait until they settle
	protected void ScheduleSave()
	{
		if (m_SuppressSave || !GetGame()) {
			return;
		}

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SaveState);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(SaveState, SCPLUS_SAVE_DELAY_MS);
	}

	// Writes control settings to the profile and the current look to the _LastLook preset
	void SaveState()
	{
		if (GetGame()) {
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SaveState);
		}

		SCPlusSettings settings = SCPlusSettings.GetInstance();
		if (settings) {
			settings.CopyFrom(this);
			settings.Save();
		}

		SCPlusPreset.Capture(SCPlusPreset.LAST_LOOK, this).Save();
	}

	// ---------------------------------------------------------------------
	// Sequencer
	// ---------------------------------------------------------------------

	void SetSequence(SCPlusCameraSequence sequence)
	{
		Pause();
		m_Sequence = sequence;
		scp_SceneTarget = null;
		scp_SelectedWaypoint = -1;
		SCPlusPathVisuals.MarkDirty();
		OnSequenceEdited();
	}

	// Call after any change to the sequence's keys
	void OnSequenceEdited()
	{
		m_Evaluator = null;
		if (m_Sequence && m_Sequence.Count() > 0) {
			m_Evaluator = new SCPlusSequenceEvaluator(m_Sequence);
		}

		scp_ActorEvaluator = null;
		if (m_Sequence && m_Sequence.scp_ActorKeys && m_Sequence.scp_ActorKeys.Count() > 0) {
			scp_ActorEvaluator = new SCPlusSequenceEvaluator(m_Sequence, m_Sequence.scp_ActorKeys);
		}
	}

	SCPlusSequenceEvaluator GetActorEvaluator()
	{
		return scp_ActorEvaluator;
	}

	void SetActorPreviewSelection(int key_index)
	{
		scp_PreviewActorSelected = key_index;
	}

	// --- Actor track ---
	// Auto-Key also records where the player (the actor) goes, into the sequence's actor keys. With Replay
	// Actor Track on, playback walks the actor along those keys (SCPlusTrackFollower) and Follow Actor / Aim
	// At Target use the track instead of the live character, so the framing is exactly what was keyed.
	static const float ACTOR_REPLAY_STEP = 0.1; // seconds between the samples handed to the follower
	static const int ACTOR_REPLAY_MAX_SAMPLES = 1500;

	bool IsActorTrackActive()
	{
		return m_Sequence && m_Sequence.scp_ReplayActor && m_Sequence.HasActorTrack() && scp_ActorEvaluator != null;
	}

	// Where the actor track puts the actor (feet) at a time, and their heading
	vector GetActorTrackPosition(float time)
	{
		if (!scp_ActorEvaluator) {
			return vector.Zero;
		}

		return Vector(scp_ActorEvaluator.EvaluateChannelAt(0, time), scp_ActorEvaluator.EvaluateChannelAt(1, time), scp_ActorEvaluator.EvaluateChannelAt(2, time));
	}

	// Starts the actor walking the track from "time" (playback start and each loop)
	protected void StartActorReplay(float time)
	{
		if (!IsActorTrackActive()) {
			return;
		}

		float end = m_Sequence.scp_ActorKeys[m_Sequence.scp_ActorKeys.Count() - 1].scp_KeyTime;
		float step = Math.Max(ACTOR_REPLAY_STEP, end / (ACTOR_REPLAY_MAX_SAMPLES - 1));
		int count = Math.Ceil(end / step) + 1;
		array<vector> points = new array<vector>();
		array<float> headings = new array<float>();
		for (int i = 0; i < count; i++) {
			float sample_time = Math.Min(i * step, end);
			points.Insert(GetActorTrackPosition(sample_time));
			headings.Insert(scp_ActorEvaluator.EvaluateChannelAt(3, sample_time));
		}

		SCPlusEventRunner.Get().StartTrack(points, headings, step, time);
	}

	void SetPreviewSelection(int key_index)
	{
		m_PreviewSelected = key_index;
	}

	// A key holding the camera's current view, for the given time
	SCPlusKeyframe CaptureKey(float time)
	{
		return SCPlusKeyframe.Create(time, GetPosition(), GetOrientation(), CameraFOV, CameraDOFDistance, CameraDOFBlur);
	}

	SCPlusSequenceEvaluator GetEvaluator()
	{
		return m_Evaluator;
	}

	float GetPlayhead()
	{
		return m_Playhead;
	}

	float GetSequenceLength()
	{
		if (!m_Sequence) {
			return 0;
		}

		return m_Sequence.GetLength();
	}

	bool IsPlaying()
	{
		return m_Playing;
	}

	// Moves the playhead and puts the camera where the sequence says it should be at that time
	void SetPlayhead(float time, bool apply_pose = true)
	{
		m_Playhead = Math.Clamp(time, 0, GetSequenceLength());
		if (apply_pose && m_Evaluator) {
			ApplySequencePose(m_Playhead);
			FinishPose();
		}

		if (apply_pose) {
			PreviewActorAt(m_Playhead);
		}
	}

	// Scrub preview: while paused with Replay Actor Track on, the actor stands where the track has them
	void PreviewActorAt(float time)
	{
		if (m_Playing || scp_Baking || !IsActorTrackActive()) {
			return;
		}

		vector position = GetActorTrackPosition(time);
		float heading = scp_ActorEvaluator.EvaluateChannelAt(3, time);
		heading = heading - Math.Round(heading / 360) * 360;
		SCPlusEventRunner.Get().PreviewActor(position, Vector(heading, 0, 0));
	}

	bool Play()
	{
		if (m_Playing) {
			return true;
		}

		if (!m_Sequence) {
			return false;
		}

		bool can_play_keys = m_Evaluator != null && m_Sequence.Count() >= 2;
		if (scp_AutoKey) {
			// Auto-Key: plays (or, with no keys, holds the framing on the target) from the start and records it
			if (!can_play_keys && !CanBakeTrack()) {
				return false;
			}

			StartBake();
		} else if (!can_play_keys) {
			return false;
		}

		// Like most editors, pressing play at the end starts again from the beginning
		if (m_Playhead >= GetSequenceLength() - SCPlusCameraSequence.KEY_TOLERANCE) {
			m_Playhead = 0;
		}

		// Starting from the top: the first take remembers where the actor stands, later takes put them back there
		if (m_Playhead <= SCPlusCameraSequence.KEY_TOLERANCE) {
			if (m_Sequence.scp_HasActorStart) {
				ResetActor();
			} else {
				SetActorStart();
			}
		}

		StartActorReplay(m_Playhead);
		m_Playing = true;
		m_PlaybackJustStarted = true;
		m_SpectatorTimer = 0;
		linearVelocity = vector.Zero;
		angularVelocity = vector.Zero;
		OnPlaybackStateChanged.Invoke(this, true);
		return true;
	}

	void Pause()
	{
		if (!m_Playing) {
			return;
		}

		m_Playing = false;
		// Stopping by hand cancels pending shots; reaching the end lets the last burst finish
		if (!m_ReachedEnd) {
			SCPlusEventRunner.Get().Reset();
		}
		EndBake();
		FinishPose();
		OnPlaybackStateChanged.Invoke(this, false);
	}

	// Stop = pause and rewind to the start
	void Stop()
	{
		Pause();
		SetPlayhead(0);
		ResetActor();
	}

	// --- Auto-Key (see SCPlusBake) ---
	// With Auto-Key on, Play and Record Take record what the camera does into keys, like Blender's Bake Action:
	//  - with 2+ keys: the keys play as usual (with Follow Actor / Aim At Target) and the result is baked
	//  - with fewer: the camera holds its current framing on the target (followed target, else focus target,
	//    else the actor with Follow Actor on), aims at the focus target, and that is baked
	// Reaching the end commits the keys (the HUD replaces the sequence's keys, one undo step). Stopping early
	// throws the recording away, except with no keys, where stopping is how the shot ends.
	bool scp_AutoKey;
	protected bool scp_Baking;
	protected bool scp_BakeTrack;
	protected Object scp_BakeAnchor;
	protected Object scp_BakeAim;
	protected vector scp_BakeOffset;
	protected vector scp_BakeOrientation;
	protected float scp_BakeLength;
	protected float scp_BakeStep;
	protected float scp_BakeNextSample;
	protected vector scp_TrackSmoothed;
	protected bool scp_TrackSmoothedValid;
	protected ref array<ref SCPlusKeyframe> scp_BakeSamples;
	protected ref array<ref SCPlusKeyframe> scp_BakeResult;
	// Actor track recorded alongside (not while the track itself is being replayed: that keeps the edit)
	static const float ACTOR_MIN_MOVE = 0.5; // metres the actor must move (or ACTOR_MIN_TURN degrees turn) to replace the track
	static const float ACTOR_MIN_TURN = 20;
	protected bool scp_BakeRecordActor;
	protected ref array<ref SCPlusKeyframe> scp_BakeActorSamples;
	protected ref array<ref SCPlusKeyframe> scp_BakeActorResult;
	protected string scp_BakeMessage;
	protected float scp_BakeRecordedLength;

	bool IsBaking()
	{
		return scp_Baking;
	}

	Object GetBakeAnchor()
	{
		if (FollowedTarget) {
			return FollowedTarget;
		}

		if (SelectedTarget) {
			return SelectedTarget;
		}

		if (m_Sequence && m_Sequence.scp_FollowActor) {
			return GetGame().GetPlayer();
		}

		return null;
	}

	// True when Play can bake a shot with no keys
	bool CanBakeTrack()
	{
		return scp_AutoKey && m_Sequence && GetBakeAnchor() != null;
	}

	protected void StartBake()
	{
		scp_BakeResult = null;
		scp_BakeMessage = string.Empty;
		scp_BakeSamples = new array<ref SCPlusKeyframe>();
		scp_BakeActorResult = null;
		scp_BakeActorSamples = new array<ref SCPlusKeyframe>();
		scp_BakeRecordActor = GetGame().GetPlayer() != null && !IsActorTrackActive();
		scp_BakeTrack = m_Sequence.Count() < 2 || !m_Evaluator;

		if (scp_BakeTrack) {
			scp_BakeAnchor = GetBakeAnchor();
			scp_BakeAim = SelectedTarget;
			scp_BakeOffset = GetPosition() - scp_BakeAnchor.GetPosition();
			scp_BakeOrientation = GetOrientation();
			scp_TrackSmoothedValid = false;
			scp_BakeLength = m_Sequence.GetLength();
			foreach (SCPlusEvent event_key: m_Sequence.scp_Events) {
				scp_BakeLength = Math.Max(scp_BakeLength, event_key.GetEnd());
			}

			m_Playhead = 0;
		} else {
			// The first sample is the sequence's own start pose
			SetPlayhead(0);
			scp_BakeLength = GetSequenceLength();
		}

		scp_BakeStep = SCPlusBake.GetStep(scp_BakeLength);
		scp_Baking = true;
		scp_BakeNextSample = 0;
		RecordBakeSample(0, true);
		// Print(string.Format("[SCPlus AutoKey] bake started (%1, %2 s)", scp_BakeTrack, scp_BakeLength));
	}

	// No keys: keep the offset from the anchor (smoothed like Follow Actor) and aim at the focus target
	protected void ApplyTrackPose()
	{
		if (!scp_BakeAnchor) {
			return;
		}

		vector anchor = scp_BakeAnchor.GetPosition();
		if (SCPlusIsNaN(anchor)) {
			return;
		}

		scp_TrackSmoothed = SCPlusSmooth(scp_TrackSmoothed, anchor, scp_TrackSmoothedValid, SCPLUS_FOLLOW_SMOOTH_TIME);
		SetPosition(scp_TrackSmoothed + scp_BakeOffset);
		if (scp_BakeAim) {
			SCPlusSafeLookAt(GetSmoothedAimPoint(scp_BakeAim));
		} else {
			SetOrientation(scp_BakeOrientation);
		}
	}

	protected void RecordBakeSample(float time, bool force)
	{
		if (!scp_Baking || !scp_BakeSamples) {
			return;
		}

		if (!force && time < scp_BakeNextSample) {
			return;
		}

		int count = scp_BakeSamples.Count();
		if (count > 0 && time <= scp_BakeSamples[count - 1].scp_KeyTime + 0.001) {
			return;
		}

		SCPlusKeyframe sample = CaptureKey(time);
		if (SCPlusIsNaN(sample.scp_Position) || SCPlusIsNaN(sample.scp_Orientation)) {
			return;
		}

		scp_BakeSamples.Insert(sample);
		scp_BakeNextSample = time + scp_BakeStep;
		RecordActorSample(time);
	}

	protected void RecordActorSample(float time)
	{
		if (!scp_BakeRecordActor || !scp_BakeActorSamples) {
			return;
		}

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) {
			return;
		}

		vector position = player.GetPosition();
		vector body_orientation = player.GetOrientation();
		if (SCPlusIsNaN(position) || SCPlusIsNaN(body_orientation)) {
			return;
		}

		// Heading only: pitch / roll of the body are noise for a walking actor
		scp_BakeActorSamples.Insert(SCPlusKeyframe.Create(time, position, Vector(body_orientation[0], 0, 0), 70, 0, 0));
	}

	// True when the recorded actor moved or turned enough to be worth a track
	protected bool ActorSamplesMoved()
	{
		int count = scp_BakeActorSamples.Count();
		if (count < 2) {
			return false;
		}

		SCPlusKeyframe first_sample = scp_BakeActorSamples[0];
		vector first = first_sample.scp_Position;
		vector first_orientation = first_sample.scp_Orientation;
		float first_heading = first_orientation[0];
		foreach (SCPlusKeyframe sample: scp_BakeActorSamples) {
			vector offset = sample.scp_Position - first;
			offset[1] = 0;
			vector sample_orientation = sample.scp_Orientation;
			float turn = SCPlusSequenceEvaluator.UnwrapAngle(sample_orientation[0], first_heading) - first_heading;
			if (offset.Length() >= ACTOR_MIN_MOVE || Math.AbsFloat(turn) >= ACTOR_MIN_TURN) {
				return true;
			}
		}

		return false;
	}

	// Called by Pause: keeps the recording if the shot finished
	protected void EndBake()
	{
		if (!scp_Baking) {
			return;
		}

		RecordBakeSample(m_Playhead, true);
		scp_Baking = false;

		int count = scp_BakeSamples.Count();
		float recorded = 0;
		if (count > 0) {
			recorded = scp_BakeSamples[count - 1].scp_KeyTime;
		}

		bool complete = m_ReachedEnd || (scp_BakeTrack && recorded >= 0.5);
		if (complete && count >= 2) {
			scp_BakeResult = SCPlusBake.Decimate(scp_BakeSamples);
			scp_BakeRecordedLength = recorded;
			if (scp_BakeRecordActor && scp_BakeActorSamples && ActorSamplesMoved()) {
				scp_BakeActorResult = SCPlusBake.DecimateActor(scp_BakeActorSamples);
			}
			// Print(string.Format("[SCPlus AutoKey] baked %1 samples into %2 keys over %3 s", count, scp_BakeResult.Count(), recorded));
		} else {
			scp_BakeResult = null;
			scp_BakeMessage = "Auto-Key: stopped before the end, nothing baked (keys unchanged)";
			// Print("[SCPlus AutoKey] bake discarded (stopped early)");
		}

		scp_BakeSamples = null;
		scp_BakeActorSamples = null;
		scp_BakeRecordActor = false;
		scp_BakeTrack = false;
		scp_BakeAnchor = null;
		scp_BakeAim = null;
	}

	// The finished bake for the HUD to commit (null if none); ClearBakeResult once it is used
	array<ref SCPlusKeyframe> GetBakeResult()
	{
		return scp_BakeResult;
	}

	// The actor track recorded with the finished bake (null if none, or the actor didn't move)
	array<ref SCPlusKeyframe> GetBakeActorResult()
	{
		return scp_BakeActorResult;
	}

	float GetBakeRecordedLength()
	{
		return scp_BakeRecordedLength;
	}

	string GetBakeMessage()
	{
		return scp_BakeMessage;
	}

	void ClearBakeResult()
	{
		scp_BakeResult = null;
		scp_BakeActorResult = null;
		scp_BakeMessage = string.Empty;
	}

	// Remembers where the player stands now as the scene start (saved with the sequence)
	bool SetActorStart()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !m_Sequence) {
			return false;
		}

		m_Sequence.scp_HasActorStart = true;
		m_Sequence.scp_ActorStartPos = player.GetPosition();
		m_Sequence.scp_ActorStartOri = player.GetOrientation();
		// Print("[SCPlus Events] actor start set at " + m_Sequence.scp_ActorStartPos.ToString());

		// The scene starts here for the target too, unless its start was set by hand already
		if (!m_Sequence.scp_HasTargetStart) {
			SetTargetStart();
		}

		return true;
	}

	// Puts the player back at the scene start (if one is set), stops their events, and puts the scene target back
	void ResetActor()
	{
		if (!m_Sequence) {
			return;
		}

		if (m_Sequence.scp_HasActorStart) {
			SCPlusEventRunner.Get().ResetActor(m_Sequence.scp_ActorStartPos, m_Sequence.scp_ActorStartOri);
		}

		ResetSceneTarget();
	}

	// --- Scene target: the animal, vehicle or other player picked with Follow / Focus ---
	protected Object scp_SceneTarget;
	static const float SCENE_TARGET_PICKUP_DISTANCE = 100; // a re-picked target this close to the saved start is adopted

	// The Follow target, else the Focus target, if it can be reset (not your own character, not a building)
	Object GetSceneTargetCandidate()
	{
		Object own = GetGame().GetPlayer();
		if (FollowedTarget && FollowedTarget != own && SCPlusEventRunner.IsResettableTarget(FollowedTarget)) {
			return FollowedTarget;
		}

		if (SelectedTarget && SelectedTarget != own && SCPlusEventRunner.IsResettableTarget(SelectedTarget)) {
			return SelectedTarget;
		}

		return null;
	}

	// Remembers where the scene target stands now (saved with the sequence). False if there is none.
	bool SetTargetStart()
	{
		Object target = GetSceneTargetCandidate();
		if (!target || !m_Sequence) {
			return false;
		}

		scp_SceneTarget = target;
		m_Sequence.scp_HasTargetStart = true;
		m_Sequence.scp_TargetStartPos = target.GetPosition();
		m_Sequence.scp_TargetStartOri = target.GetOrientation();
		m_Sequence.scp_TargetType = target.GetType();
		// Print(string.Format("[SCPlus Events] scene target start: %1 at %2", m_Sequence.scp_TargetType, m_Sequence.scp_TargetStartPos.ToString()));
		return true;
	}

	void ClearTargetStart()
	{
		scp_SceneTarget = null;
		if (m_Sequence) {
			m_Sequence.scp_HasTargetStart = false;
		}
	}

	protected void ResetSceneTarget()
	{
		if (!m_Sequence || !m_Sequence.scp_HasTargetStart) {
			return;
		}

		// After a restart (or a loaded sequence) the object is gone: adopt the picked target if it is the same kind
		// of thing near the saved start
		if (!scp_SceneTarget) {
			Object candidate = GetSceneTargetCandidate();
			if (candidate && candidate.GetType() == m_Sequence.scp_TargetType && vector.Distance(candidate.GetPosition(), m_Sequence.scp_TargetStartPos) <= SCENE_TARGET_PICKUP_DISTANCE) {
				scp_SceneTarget = candidate;
			}
		}

		if (!scp_SceneTarget) {
			return;
		}

		SCPlusEventRunner.ResetTarget(scp_SceneTarget, m_Sequence.scp_TargetStartPos, m_Sequence.scp_TargetStartOri);
	}

	// Looks at a point without LookAt's NaN when the point is straight above/below or on the camera
	void SCPlusSafeLookAt(vector target)
	{
		vector direction = target - GetPosition();
		if (SCPlusIsNaN(direction) || direction.Length() < 0.05) {
			return;
		}

		vector angles = direction.VectorToAngles();
		float pitch = angles[1];
		if (pitch > 180) {
			pitch = pitch - 360;
		}

		SetOrientation(Vector(angles[0], Math.Clamp(pitch, -89, 89), 0));
	}

	void TogglePlayPause()
	{
		if (m_Playing) {
			Pause();
		} else {
			Play();
		}
	}

	protected void UpdatePlayback(float time_slice)
	{
		float length = GetSequenceLength();
		float speed = PlaybackSpeed;
		if (scp_Baking) {
			// Baked keys must line up with the actor's events, which always run in real time
			speed = 1;
			if (scp_BakeTrack) {
				length = scp_BakeLength;
			}
		}

		float previous = m_Playhead;
		m_Playhead += time_slice * speed;

		bool reached_end;
		if (m_Playhead >= length) {
			if (SequenceLoop && !scp_Baking && length > 0) {
				// Events up to the end, then from the start of the next loop
				RunEvents(previous, length, false);
				ResetActor();
				previous = -1;
				m_Playhead = Math.FMod(m_Playhead, length);
				StartActorReplay(m_Playhead);
			} else {
				m_Playhead = length;
				reached_end = true;
			}
		}

		RunEvents(previous, m_Playhead, previous < 0 || m_PlaybackJustStarted);
		bool first_frame = m_PlaybackJustStarted;
		m_PlaybackJustStarted = false;

		// Smooth follow / aim during playback; the first frame (and a loop's actor reset) snaps
		scp_PoseTimeSlice = time_slice;
		if (first_frame || previous < 0) {
			scp_PoseTimeSlice = 0;
		}

		if (scp_BakeTrack) {
			ApplyTrackPose();
		} else {
			ApplySequencePose(m_Playhead);
		}

		scp_PoseTimeSlice = 0;
		RecordBakeSample(m_Playhead, reached_end);

		// Fast camera moves need the server to stream the world in sooner than the usual 5 seconds
		m_SpectatorTimer += time_slice;
		if (m_SpectatorTimer >= 0.5) {
			m_SpectatorTimer = 0;
			GetGame().UpdateSpectatorPosition(GetPosition());
		}

		if (reached_end) {
			m_ReachedEnd = true;
			Pause();
			m_ReachedEnd = false;
		}
	}

	// --- Bullet Cam (see SCPlusBulletCam) ---
	// Set by the module each frame: the mouse wheel is busy (quick slider ticked, or cursor over a panel)
	bool scp_BlockWheelSpeed;

	protected ref array<vector> m_BulletPath;
	protected float m_BulletTime;
	protected float m_BulletHold;
	protected float m_BulletStreamTimer;
	protected Object m_BulletModel;
	protected vector m_BulletSavedPosition;
	protected vector m_BulletSavedOrientation;

	bool IsFollowingBullet()
	{
		return m_BulletPath != null;
	}

	// Returns true while the bullet cam owns the camera this frame
	protected bool UpdateBulletCam(float time_slice)
	{
		if (!m_BulletPath) {
			if (!SCPlusBulletCam.HasShot()) {
				return false;
			}

			string ammo_type;
			bool hit;
			m_BulletPath = SCPlusBulletCam.TakeShot(ammo_type, hit);
			if (!m_BulletPath || m_BulletPath.Count() < 2) {
				m_BulletPath = null;
				return false;
			}

			m_BulletTime = 0;
			m_BulletHold = 0;
			m_BulletStreamTimer = 0;
			m_BulletSavedPosition = GetPosition();
			m_BulletSavedOrientation = GetOrientation();
			m_BulletModel = SCPlusBulletCam.CreateBulletModel(ammo_type, m_BulletPath[0]);
		}

		int last = m_BulletPath.Count() - 1;
		float flight_time = last * SCPlusBulletCam.STEP;
		if (m_BulletTime < flight_time) {
			m_BulletTime = Math.Min(m_BulletTime + time_slice * SCPlusBulletCam.scp_SpeedFactor, flight_time);
		} else {
			m_BulletHold += time_slice;
			if (m_BulletHold >= SCPlusBulletCam.scp_HoldTime) {
				EndBulletCam();
				return true;
			}
		}

		// Position on the path and the direction of travel there
		float steps = m_BulletTime / SCPlusBulletCam.STEP;
		int index = Math.Clamp(Math.Floor(steps), 0, last - 1);
		float fraction = Math.Clamp(steps - index, 0, 1);
		vector bullet_position = vector.Lerp(m_BulletPath[index], m_BulletPath[index + 1], fraction);
		vector travel = vector.Direction(m_BulletPath[index], m_BulletPath[index + 1]);
		travel.Normalize();

		float distance = SCPlusBulletCam.scp_Distance;
		SetPosition(bullet_position - travel * distance + Vector(0, distance * 0.12, 0));
		LookAt(bullet_position + travel * 4);

		if (m_BulletModel) {
			m_BulletModel.SetPosition(bullet_position);
			float model_scale = m_BulletModel.GetScale();
			m_BulletModel.SetDirection(travel);
			m_BulletModel.SetScale(model_scale); // re-apply in case setting the direction resets the scale
		}

		// Long shots need the world streamed in around the bullet
		m_BulletStreamTimer += time_slice;
		if (m_BulletStreamTimer >= 0.5) {
			m_BulletStreamTimer = 0;
			GetGame().UpdateSpectatorPosition(GetPosition());
		}

		return true;
	}

	void EndBulletCam()
	{
		if (!m_BulletPath) {
			return;
		}

		m_BulletPath = null;
		if (m_BulletModel) {
			GetGame().ObjectDelete(m_BulletModel);
			m_BulletModel = null;
		}

		// A playing sequence simply carries on from where it was; otherwise go back to where the camera was
		if (!m_Playing) {
			SetPosition(m_BulletSavedPosition);
			SetOrientation(m_BulletSavedOrientation);
		}

		GetGame().UpdateSpectatorPosition(GetPosition());
	}

	// Runs the events in (from, to]. include_from also runs events exactly at "from" (start of playback or a loop).
	protected void RunEvents(float from, float to, bool include_from)
	{
		if (!m_Sequence || !m_Sequence.scp_Events) {
			return;
		}

		SCPlusEventRunner.Get().SetWaypoints(GetPathCurve());
		foreach (SCPlusEvent event_key: m_Sequence.scp_Events) {
			bool after_from = event_key.scp_Time > from || (include_from && event_key.scp_Time >= from);
			if (after_from && event_key.scp_Time <= to) {
				SCPlusEventRunner.Get().Run(event_key);
			}
		}
	}

	// Sets position, rotation and lens straight from the curves. The lens is applied directly rather than
	// through PropertyChanged so playback doesn't schedule a settings save every frame.
	protected void ApplySequencePose(float time)
	{
		if (!m_Evaluator) {
			return;
		}

		vector position, pose_orientation;
		float fov, dof_distance, dof_blur;
		m_Evaluator.Evaluate(time, position, pose_orientation, fov, dof_distance, dof_blur);
		position = position + GetSmoothedFollowOffset();
		if (SCPlusIsNaN(position) || SCPlusIsNaN(pose_orientation)) {
			return;
		}

		SetPosition(position);
		if (SequenceAimAtTarget && SelectedTarget) {
			SCPlusSafeLookAt(GetSmoothedAimPoint(SelectedTarget));
		} else {
			SetOrientation(pose_orientation);
		}

		CameraFOV = fov;
		SetFOV(CameraFOV * Math.DEG2RAD);
		CameraDOFDistance = dof_distance;
		CameraDOFBlur = dof_blur;
		SetFocus(CameraDOFDistance, CameraDOFBlur / 4);
	}

	// Follow Actor: while playing, how far the actor has moved from the actor start. Keys are made with the
	// actor standing at the start, so adding this keeps the framing as the actor walks, runs or follows a path.
	// Network position corrections, the actor reset and the walk cycle make the raw positions twitch from frame
	// to frame, and the camera copied every twitch. Both the follow offset and the Aim At Target point are
	// smoothed (about a tenth of a second), and snap instead when they jump more than SCPLUS_SNAP_DISTANCE.
	static const float SCPLUS_FOLLOW_SMOOTH_TIME = 0.12;
	static const float SCPLUS_AIM_SMOOTH_TIME = 0.15;
	static const float SCPLUS_SNAP_DISTANCE = 3;
	protected float scp_PoseTimeSlice; // 0 = snap (scrubbing, first frame)
	protected vector scp_FollowSmoothed;
	protected bool scp_FollowSmoothedValid;
	protected vector scp_AimSmoothed;
	protected bool scp_AimSmoothedValid;

	protected vector SCPlusSmooth(vector current, vector target, inout bool valid, float smooth_time)
	{
		if (!valid || scp_PoseTimeSlice <= 0 || vector.Distance(current, target) > SCPLUS_SNAP_DISTANCE) {
			valid = true;
			return target;
		}

		float blend = 1 - Math.Pow(2.71828, -scp_PoseTimeSlice / smooth_time);
		return vector.Lerp(current, target, blend);
	}

	vector GetSmoothedFollowOffset()
	{
		vector raw = GetActorFollowOffset();
		scp_FollowSmoothed = SCPlusSmooth(scp_FollowSmoothed, raw, scp_FollowSmoothedValid, SCPLUS_FOLLOW_SMOOTH_TIME);
		return scp_FollowSmoothed;
	}

	vector GetSmoothedAimPoint(notnull Object target)
	{
		vector raw = GetTargetLookPosition(target);
		// Replaying the actor track: aim where the track has the actor (at the live head height above the feet)
		if (m_Playing && target == GetGame().GetPlayer() && IsActorTrackActive()) {
			raw = GetActorTrackPosition(m_Playhead) + (raw - target.GetPosition());
		}

		if (SCPlusIsNaN(raw)) {
			return scp_AimSmoothed;
		}

		scp_AimSmoothed = SCPlusSmooth(scp_AimSmoothed, raw, scp_AimSmoothedValid, SCPLUS_AIM_SMOOTH_TIME);
		return scp_AimSmoothed;
	}

	vector GetActorFollowOffset()
	{
		if (!m_Playing || !m_Sequence || !m_Sequence.scp_FollowActor || !m_Sequence.scp_HasActorStart) {
			return vector.Zero;
		}

		// Replaying the actor track: follow the track itself, not the live character
		if (IsActorTrackActive()) {
			vector track_offset = GetActorTrackPosition(m_Playhead) - m_Sequence.scp_ActorStartPos;
			if (!SCPlusIsNaN(track_offset)) {
				return track_offset;
			}
		}

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) {
			return vector.Zero;
		}

		vector offset = player.GetPosition() - m_Sequence.scp_ActorStartPos;
		if (SCPlusIsNaN(offset)) {
			return vector.Zero;
		}

		return offset;
	}

	// After playback or a scrub: re-anchor follow, push the lens through the normal path so sliders and the saved look agree
	protected void FinishPose()
	{
		if (FollowedTarget) {
			FollowedTargetDeltaPos = GetPosition() - FollowedTarget.GetPosition();
		}

		PropertyChanged("CameraFOV");
		PropertyChanged("CameraDOFDistance");
		GetGame().UpdateSpectatorPosition(GetPosition());
	}

	static vector GetTargetLookPosition(notnull Object target)
	{
		PlayerBase player = PlayerBase.Cast(target);
		if (player) {
			int head_bone = player.GetBoneIndexByName("Head");
			if (head_bone >= 0) {
				return player.GetBonePositionWS(head_bone);
			}

			return player.GetPosition() + Vector(0, 1.6, 0);
		}

		return target.GetPosition();
	}

	// Draws the camera's motion path, keys (with their look direction) and the playhead in the world.
	// Like a motion path in Blender or Maya, so the move can be judged without playing it.
	protected void DrawSequencePreview()
	{
		if (!SequenceShowPreview || !m_Sequence || !m_Evaluator) {
			return;
		}

		vector position, pose_orientation, previous_position;
		float fov, dof_distance, dof_blur;
		// Curve first, so the keys and the playhead draw on top of it
		if (m_Sequence.Count() >= 2) {
			float start = m_Sequence.scp_Keys[0].scp_KeyTime;
			float end = m_Sequence.scp_Keys[m_Sequence.Count() - 1].scp_KeyTime;
			float step_count = Math.Clamp(Math.Ceil((end - start) / PREVIEW_SAMPLE_STEP), 2, 600);
			for (int step = 0; step <= step_count; step++) {
				float fraction = step;
				fraction = fraction / step_count; // float division, step / step_count alone would be integer maths
				m_Evaluator.Evaluate(Math.Lerp(start, end, fraction), position, pose_orientation, fov, dof_distance, dof_blur);
				if (step > 0) {
					SCPlusPreviewLine(previous_position, position, PREVIEW_COLOR_CURVE, 2);
				}

				previous_position = position;
			}
		}

		// Keys, each with a short line for its look direction
		foreach (int i, SCPlusKeyframe key: m_Sequence.scp_Keys) {
			int color = PREVIEW_COLOR_KEY;
			if (i == m_PreviewSelected) {
				color = PREVIEW_COLOR_SELECTED;
			}

			SCPlusPreviewLine(key.scp_Position, key.scp_Position + key.scp_Orientation.AnglesToVector() * 1.5, color, 2);
			SCPlusPreviewDot(key.scp_Position, color, 0.25);
		}

		// Playhead marker
		if (m_Sequence.Count() > 0) {
			m_Evaluator.Evaluate(m_Playhead, position, pose_orientation, fov, dof_distance, dof_blur);
			SCPlusPreviewDot(position, PREVIEW_COLOR_PLAYHEAD, 0.18);
		}
	}

	static const int PREVIEW_COLOR_ACTOR = 0xFF50DC78;
	static const int PREVIEW_COLOR_ACTOR_KEY = 0xC8B4FFC8;

	// Show Motion Path for the actor track: a green line just above the ground, a dot and a heading line per key
	protected void DrawActorTrackPreview()
	{
		if (!SequenceShowPreview || !m_Sequence || !scp_ActorEvaluator || m_Sequence.scp_ActorKeys.Count() == 0) {
			return;
		}

		vector lift = Vector(0, 0.1, 0);
		int key_count = m_Sequence.scp_ActorKeys.Count();
		if (key_count >= 2) {
			float start = m_Sequence.scp_ActorKeys[0].scp_KeyTime;
			float end = m_Sequence.scp_ActorKeys[key_count - 1].scp_KeyTime;
			float step_count = Math.Clamp(Math.Ceil((end - start) / PREVIEW_SAMPLE_STEP), 2, 600);
			vector previous_position;
			for (int step = 0; step <= step_count; step++) {
				float fraction = step;
				fraction = fraction / step_count;
				vector position = GetActorTrackPosition(Math.Lerp(start, end, fraction)) + lift;
				if (step > 0) {
					SCPlusPreviewLine(previous_position, position, PREVIEW_COLOR_ACTOR, 2);
				}

				previous_position = position;
			}
		}

		foreach (int i, SCPlusKeyframe key: m_Sequence.scp_ActorKeys) {
			int color = PREVIEW_COLOR_ACTOR_KEY;
			if (i == scp_PreviewActorSelected) {
				color = PREVIEW_COLOR_SELECTED;
			}

			vector key_orientation = key.scp_Orientation;
			vector heading = Vector(key_orientation[0], 0, 0);
			SCPlusPreviewLine(key.scp_Position + lift, key.scp_Position + lift + heading.AnglesToVector() * 0.8, color, 2);
			SCPlusPreviewDot(key.scp_Position + lift, color, 0.15);
		}

		if (key_count > 0) {
			SCPlusPreviewDot(GetActorTrackPosition(m_Playhead) + lift, PREVIEW_COLOR_PLAYHEAD, 0.12);
		}
	}

	// Full replacement of the original EOnFrame (super is NOT called). Changes vs. the original:
	//  - cam_speed is always initialised (was undefined when Boost_Multiplier was 0)
	//  - removed reads of SCCameraZoomIn/Out/ZoomReset, which aren't declared inputs and did nothing
	//  - no movement while typing in a HUD text box
	//  - uses SelectedTarget instead of the original's private IsTargeting flag (same meaning)
	// If Simple Cinematics changes its EOnFrame, re-copy it here and re-apply these changes.
	override void EOnFrame(IEntity other, float timeSlice)
	{
		// Motion path is rebuilt every editing frame below; empty during Bullet Cam and playback
		SCPlusClearPreview();
		SCPlusPathVisuals.Update(ShowPaths && !m_Playing && !IsFollowingBullet(), GetWaypoints(), GetWaypointCurves(), GetSelectedWaypoint());

// Bullet Cam takes over while it follows a shot (a playing sequence waits, frozen)
		if (UpdateBulletCam(timeSlice)) {
			return;
		}

		// Sequence playback takes over the camera completely
		if (m_Playing) {
			UpdatePlayback(timeSlice);
			return;
		}

		DrawSequencePreview();
		DrawActorTrackPreview();

		vector transform[4];
		GetTransform(transform);

		Input input = GetGame().GetInput();
		vector movement;
		bool typing = IsTypingInUI();
		if (!KeyState(KeyCode.KC_LCONTROL) && !typing) {
			movement = Vector(input.LocalValue("SCCameraRight") - input.LocalValue("SCCameraLeft"), input.LocalValue("SCCameraUp") - input.LocalValue("SCCameraDown"), input.LocalValue("SCCameraForward") - input.LocalValue("SCCameraBackward"));
		}

		// Diagnostics for the typing guard: log (at most once a second) when movement keys are held while typing
		// (blocked) and when the camera moves while a name box has text focus by GetFocus but the flag is off
		scp_TypingLogTimer = Math.Max(scp_TypingLogTimer - timeSlice, 0);
		if (scp_TypingLogTimer == 0) {
			float raw_move = Math.AbsFloat(input.LocalValue("SCCameraRight")) + Math.AbsFloat(input.LocalValue("SCCameraLeft")) + Math.AbsFloat(input.LocalValue("SCCameraUp")) + Math.AbsFloat(input.LocalValue("SCCameraDown")) + Math.AbsFloat(input.LocalValue("SCCameraForward")) + Math.AbsFloat(input.LocalValue("SCCameraBackward"));
			if (raw_move > 0 && typing) {
				// Print("[SCPlus Typing] movement keys held while typing: blocked");
				scp_TypingLogTimer = 1;
			} else if (raw_move > 0 && GetGame().GetUIManager().IsCursorVisible()) {
				// Print("[SCPlus Typing] camera moving with cursor shown, typing flag off");
				scp_TypingLogTimer = 1;
			}
		}

		float yawDiff = input.LocalValue("UAAimLeft") - input.LocalValue("UAAimRight");
		float pitchDiff = input.LocalValue("UAAimDown") - input.LocalValue("UAAimUp");
		float speedInc = input.LocalValue("UACameraToolSpeedIncrease" ) - input.LocalValue("UACameraToolSpeedDecrease");
		if (scp_BlockWheelSpeed) {
			speedInc = 0;
		}

		vector current_position = GetPosition();
		float current_altitude = current_position[1] - GetGame().SurfaceY(current_position[0], current_position[2]);

		bool shouldRoll = false;
		bool decreaseSpeeds = input.LocalValue("SCCameraSlow");
		bool increaseSpeeds = input.LocalValue("SCCameraTurbo");

		if (MoveEnabled) {
			// declared outside the boost block so it always has a value, even with Boost_Multiplier at 0
			float cam_speed = CameraSpeed;
			if (Boost_Multiplier > 0) {
				CameraSpeed += Math.Clamp(timeSlice * 40.0 * CameraSpeed * speedInc / Boost_Multiplier, -Boost_Multiplier, Boost_Multiplier);

				cam_speed = CameraSpeed;
				if (decreaseSpeeds) {
					cam_speed = cam_speed * 0.1;
				}

				if (increaseSpeeds) {
					cam_speed = (cam_speed * Boost_Multiplier) * (0.2 + (transform[3][1])/600) ;
				}
			}

			linearVelocity = linearVelocity * CameraMovementSmoothing;
			for (int i = 0; i < 3; i++) {
				linearVelocity += transform[i] * movement[i];
			}

			// Handle follow target offset
			if (FollowedTarget) {
				FollowedTargetDeltaPos = FollowedTargetDeltaPos + (linearVelocity * timeSlice * Math.Lerp(cam_speed, cam_speed * 0.01, CameraMovementSmoothing));
				transform[3] = FollowedTarget.GetPosition() + FollowedTargetDeltaPos;
			} else {
				transform[3] = transform[3] + (linearVelocity * timeSlice * Math.Lerp(cam_speed, cam_speed * 0.01, CameraMovementSmoothing));
			}

		}

		if (CameraCollision && MoveEnabled && !SCPlusIsNaN(transform[3]) && !SCPlusIsNaN(current_position)) {
			vector collided = SCPlusCollide(current_position, transform[3]);
			if (vector.Distance(collided, transform[3]) > 0.0001) {
				transform[3] = collided;
				linearVelocity = linearVelocity * 0.5;
				if (FollowedTarget) {
					FollowedTargetDeltaPos = collided - FollowedTarget.GetPosition();
				}
			}
		}

		if (SCPlusIsNaN(transform[0]) || SCPlusIsNaN(transform[1]) || SCPlusIsNaN(transform[2]) || SCPlusIsNaN(transform[3])) {
			// Logged at most once a second (it used to print every frame, which itself caused hitches), with
			// what was bad, and any NaN setting that would make the next frame NaN again is put back to default
			SCPlusRepairNaNState(transform, timeSlice);
			linearVelocity = vector.Zero;
			angularVelocity = vector.Zero;
			if (scp_HasGoodTransform) {
				SetPosition(scp_LastGoodPosition);
				SetOrientation(scp_LastGoodOrientation);
				if (FollowedTarget) {
					FollowedTargetDeltaPos = scp_LastGoodPosition - FollowedTarget.GetPosition();
				}
			}

			return;
		}

		SetTransform(transform);
		scp_LastGoodPosition = transform[3];
		scp_LastGoodOrientation = GetOrientation();
		scp_HasGoodTransform = !SCPlusIsNaN(scp_LastGoodOrientation);

		if (input.LocalValue("SCIncreaseFOV") || input.LocalValue("SCDecreaseFOV")) {
			return;
		}

		if ((input.LocalValue("UATempRaiseWeapon") || !GetGame().GetUIManager().IsCursorVisible()) && LookEnabled && !SelectedTarget) {

			angularVelocity = angularVelocity * CameraMouseSmoothing;
			float temp_cam_rot_speed = Math.Lerp(CameraMouseSensitivity, CameraMouseSensitivity * 0.05, CameraMouseSmoothing);
			angularVelocity[0] = angularVelocity[0] + (yawDiff * temp_cam_rot_speed * 10);
			angularVelocity[1] = angularVelocity[1] + (pitchDiff * temp_cam_rot_speed * 10);

			if (shouldRoll) {
				angularVelocity[2] = angularVelocity[2] + (speedInc * temp_cam_rot_speed * 10);
			}

			orientation = GetOrientation();
			for (int j = 0; j < 3; j++) {
				orientation[j] = orientation[j] - (angularVelocity[j] * timeSlice);
			}

			orientation[1] = Math.Clamp(orientation[1], -85, 85);

			SetOrientation(orientation);
		}


		if (SelectedTarget) {
			vector position = SelectedTarget.GetPosition();
			if (SelectedTarget.IsInherited(PlayerBase)) {
				PlayerBase player = PlayerBase.Cast(SelectedTarget);
				int head_bone = player.GetBoneIndexByName("Head");
				if (head_bone >= 0) {
					position = player.GetBonePositionWS(head_bone);
				} else {
					position = position + Vector(0, 1.6, 0);
				}
			}

			SCPlusSafeLookAt(position);
		}
	}
}
