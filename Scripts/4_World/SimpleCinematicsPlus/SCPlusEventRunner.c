// Runs event track actions (see SCPlusEvent) for the local player during sequence playback.
//
// Firing: Simple Cinematics locks the player's controls in camera mode, so shots can't come from the fire
// button. The runner raises the weapon through an input override, waits until the raise has finished, then
// fires through the WeaponManager, the same call the fire button ends in. In multiplayer each shot is also
// sent to the server (see SimpleCinematicsModule.SCPlusFireServer) so the server's weapon state and
// other players see it too.

modded class EmoteManager
{
	// Id of the emote playing now, or 0
	int SCPlusGetCurrentEmote()
	{
		if (!m_Callback || !m_bEmoteIsPlaying) {
			return 0;
		}

		return m_CurrentGestureID;
	}

	void SCPlusPlayEmote(int id)
	{
		CreateEmoteCBFromMenu(id);
	}

	void SCPlusEndEmote()
	{
		if (m_Callback) {
			SendEmoteRequestSync(CALLBACK_CMD_END);
		}
	}
}

class SCPlusEventRunner: Managed
{
	// How long a fire event waits for the weapon to come up before giving up
	static const float RAISE_TIMEOUT = 2.0;
	// After the last shot of a fire event the weapon is lowered again after this long (unless a Raise event holds it)
	static const float AUTO_LOWER_DELAY = 1.0;

	protected static ref SCPlusEventRunner s_Instance;

	// Invoked with (PlayerBase player) for every shot fired on the client in multiplayer
	static ref ScriptInvoker OnShotFired = new ScriptInvoker();
	// Invoked with (PlayerBase player, int speed, float angle, float duration) when a move starts (speed 0: it
	// ended) in multiplayer. The server simulates movement from its own copy of the input, so the overrides
	// have to be applied there too, or the player animates on the spot and is pulled back.
	static ref ScriptInvoker OnMoveChanged = new ScriptInvoker();
	protected static ref map<PlayerBase, int> s_ServerMoveTokens;
	// Invoked with (PlayerBase player, vector position, vector orientation) when the actor is put back at the
	// scene start in multiplayer (the server owns the player's position)
	static ref ScriptInvoker OnActorReset = new ScriptInvoker();
	// Invoked with (Object target, vector position, vector orientation) when the scene target is put back at
	// its start in multiplayer (the server owns animals, vehicles and other players)
	static ref ScriptInvoker OnTargetReset = new ScriptInvoker();
	// Invoked with (PlayerBase player, int speed, array<vector> points) when a path starts (speed 0: cancelled)
	// in multiplayer; the server runs its own SCPlusPathFollower
	static ref ScriptInvoker OnPathChanged = new ScriptInvoker();
	// Invoked with (PlayerBase player, float step, float start_time, array<vector> points, array<float> headings)
	// when an actor track replay starts (no points: it stopped) in multiplayer; the server runs its own
	// SCPlusTrackFollower
	static ref ScriptInvoker OnTrackChanged = new ScriptInvoker();

	// Actor track replay (Replay Actor Track): walks the player along the recorded track. While it runs, MOVE
	// and PATH events are skipped (the track already holds that movement).
	protected ref SCPlusTrackFollower m_Track;

	// PATH event: the sequence's waypoints (set by the camera before it runs events) and the active follower
	protected ref array<vector> m_Waypoints;
	protected ref SCPlusPathFollower m_Path;

	void SetWaypoints(array<vector> points)
	{
		m_Waypoints = points;
	}

	static const float PATH_START_SNAP = 3.0; // metres: a first waypoint this close is moved onto the player

	// True while a PATH event steers the player (the module then leaves the aim override to the follower)
	bool IsSteering()
	{
		return m_Path != null || m_Track != null;
	}

	bool IsReplayingTrack()
	{
		return m_Track != null;
	}

	// Starts walking the player along the track samples (step seconds apart) from start_time seconds in
	void StartTrack(array<vector> points, array<float> headings, float step, float start_time)
	{
		PlayerBase player = GetPlayer();
		if (!player || !player.IsAlive() || !points || points.Count() < 2) {
			return;
		}

		CancelActorPreview();
		HumanInputController input = player.GetInputController();
		if (input) {
			EndPath(input, true);
			StopMove(input);
		}

		m_Track = new SCPlusTrackFollower(points, headings, step, start_time);
		if (GetGame().IsMultiplayer()) {
			OnTrackChanged.Invoke(player, step, start_time, points, headings);
		}

		// Print("[SCPlus Track] replay started: " + m_Track.GetSampleCount().ToString() + " samples from " + start_time.ToString() + " s");
	}

	// The path a PATH event walks: it always starts where the player stands now. The first waypoint is put
	// on the player when it's close (it's normally the point dropped on the actor while placing), otherwise
	// the player's position is added in front, so the walk chains from the actor instead of cutting across.
	static array<vector> PathFromPlayer(vector player_position, array<vector> points)
	{
		array<vector> walk_points = new array<vector>();
		if (!points || points.Count() == 0) {
			return walk_points;
		}

		walk_points.Copy(points);
		vector flat = walk_points[0] - player_position;
		flat[1] = 0;
		if (flat.Length() <= PATH_START_SNAP) {
			walk_points.Set(0, player_position);
		} else {
			walk_points.InsertAt(player_position, 0);
		}

		return walk_points;
	}
// Furthest a client may move its own character with an actor reset
	static const float ACTOR_RESET_MAX_DISTANCE = 1000;

	// After a move the controller stays released with zero speed this long, so the engine sees the stop before
	// the camera disables the controller again (handing it back on the same frame left the walk latched on)
	static const float MOVE_SETTLE_TIME = 0.4;
	protected float m_MoveSettle;

	protected int m_ShotsLeft;
	protected float m_ShotInterval;
	protected float m_ShotTimer;
	protected float m_RaiseWait;
	protected bool m_RaiseHeld; // a Raise event keeps the weapon up until a Lower event
	protected bool m_RaiseOverridden;
	protected bool m_RaiseWanted; // re-applied every frame while true
	protected float m_RaiseRetryTimer; // time since the last raise/lower nudge
	protected bool m_RaisePulseOff; // this frame the override is released, so the next one is a fresh press
	protected bool m_LoggedRaised; // last raised state written to the log
	protected float m_LowerElapsed; // time spent trying to lower; the input is handed back after LOWER_GIVE_UP
	static const float LOWER_GIVE_UP = 2.0;

	// Set every frame by the module: Simple Cinematics disables the player's input controller while its camera
	// is open, and a disabled controller ignores the raise override (the log showed no raise/lower ever
	// happening; Fire only worked when the rifle was already up). So during a raise/lower the controller is
	// switched back on, with movement held at zero, and switched off again once the weapon has moved.
	static bool scp_CameraActive;
	protected bool m_ControllerReleased;
	protected float m_LowerTimer = -1;
	protected string m_LastMessage;

	// The controller is released while any of these needs it, so raise/lower and movement don't fight over it
	protected bool m_ReleaseForRaise;

	// MOVE event: the player walks/jogs/sprints through the movement overrides until m_MoveTimeLeft runs out
	protected float m_MoveTimeLeft;
	protected int m_MoveSpeed;
	protected float m_MoveAngle;
	protected bool m_MoveOverridden;

	// STANCE event: the stance is forced until the player gets there (or STANCE_TIMEOUT), then the lock is let go
	static const float STANCE_TIMEOUT = 3.0;
	protected int m_StanceWanted = -1; // SCPlusStance, or -1
	protected float m_StanceTimer;
	protected bool m_StanceForced;

	static SCPlusEventRunner Get()
	{
		if (!s_Instance) {
			s_Instance = new SCPlusEventRunner();
		}

		return s_Instance;
	}

	protected PlayerBase GetPlayer()
	{
		return PlayerBase.Cast(GetGame().GetPlayer());
	}

	// Last problem reported by an event, for the HUD to show (cleared when read)
	string TakeMessage()
	{
		string message = m_LastMessage;
		m_LastMessage = string.Empty;
		return message;
	}

	void Run(notnull SCPlusEvent event_key)
	{
		PlayerBase player = GetPlayer();
		if (!player || !player.IsAlive()) {
			return;
		}

		switch (event_key.scp_Type) {
			case SCPlusEventType.FIRE: {
				m_ShotsLeft += Math.Max(event_key.scp_Count, 1);
				m_ShotInterval = Math.Max(event_key.scp_Interval, 0.05);
				m_ShotTimer = 0;
				m_RaiseWait = 0;
				m_LowerTimer = -1;
				SetRaised(player, true);
				break;
			}

			case SCPlusEventType.RAISE: {
				m_RaiseHeld = true;
				m_LowerTimer = -1;
				SetRaised(player, true);
				break;
			}

			case SCPlusEventType.LOWER: {
				m_RaiseHeld = false;
				m_ShotsLeft = 0;
				SetRaised(player, false);
				break;
			}

			case SCPlusEventType.EMOTE: {
				if (event_key.scp_EmoteId > 0 && player.GetEmoteManager()) {
					player.GetEmoteManager().SCPlusPlayEmote(event_key.scp_EmoteId);
				}

				break;
			}

			case SCPlusEventType.STOP_EMOTE: {
				if (player.GetEmoteManager()) {
					player.GetEmoteManager().SCPlusEndEmote();
				}

				break;
			}

			case SCPlusEventType.MOVE: {
				if (m_Track) {
					break; // the actor track replays this movement
				}

				m_MoveSpeed = Math.Clamp(event_key.scp_Speed, 1, 3);
				m_MoveAngle = event_key.scp_Angle;
				m_MoveTimeLeft = Math.Max(event_key.scp_Duration, 0.1);
				if (GetGame().IsMultiplayer()) {
					OnMoveChanged.Invoke(player, m_MoveSpeed, m_MoveAngle, m_MoveTimeLeft);
				}

				// Print("[SCPlus Events] move speed=" + m_MoveSpeed.ToString() + " angle=" + m_MoveAngle.ToString() + " for " + m_MoveTimeLeft.ToString() + " s");
				break;
			}

			case SCPlusEventType.PATH: {
				if (m_Track) {
					break; // the actor track replays this movement
				}

				if (!m_Waypoints || m_Waypoints.Count() == 0) {
					m_LastMessage = "Path event: place some waypoints first";
					break;
				}

				m_MoveTimeLeft = Math.Min(m_MoveTimeLeft, 0.001); // a running move ends next frame
				// Client and server both walk the same list, which starts at the player
				array<vector> path_points = PathFromPlayer(player.GetPosition(), m_Waypoints);
				m_Path = new SCPlusPathFollower(path_points, event_key.scp_Speed, true);
				if (GetGame().IsMultiplayer()) {
					OnPathChanged.Invoke(player, Math.Clamp(event_key.scp_Speed, 1, 3), path_points);
				}

				// Print("[SCPlus Paths] path started: " + m_Path.GetPointCount().ToString() + " waypoints, speed " + event_key.scp_Speed.ToString());
				break;
			}

			case SCPlusEventType.STANCE: {
				m_StanceWanted = Math.Clamp(event_key.scp_Stance, SCPlusStance.ERECT, SCPlusStance.PRONE);
				m_StanceTimer = 0;
				m_StanceForced = false;
				// Print("[SCPlus Events] stance requested=" + m_StanceWanted.ToString());
				break;
			}
		}
	}

	// Called every frame on the client
	void Update(float dt)
	{
		PlayerBase player = GetPlayer();
		if (!player) {
			return;
		}

		UpdateRaiseState(player, dt);
		UpdateMovement(player, dt);
		UpdatePath(player, dt);
		UpdateTrack(player, dt);
UpdateStance(player, dt);

		if (m_ShotsLeft > 0) {
			UpdateFiring(player, dt);
		} else if (m_LowerTimer >= 0) {
			m_LowerTimer -= dt;
			if (m_LowerTimer < 0 && !m_RaiseHeld) {
				SetRaised(player, false);
			}
		}
	}

	protected void UpdateFiring(PlayerBase player, float dt)
	{
		Weapon_Base weapon = Weapon_Base.Cast(player.GetHumanInventory().GetEntityInHands());
		if (!weapon) {
			m_LastMessage = "Fire event: no weapon in hands";
			m_ShotsLeft = 0;
			return;
		}

		m_ShotTimer -= dt;
		if (m_ShotTimer > 0) {
			return;
		}

		if (!player.GetWeaponManager().CanFire(weapon)) {
			m_RaiseWait += dt;
			if (m_RaiseWait > RAISE_TIMEOUT) {
				m_LastMessage = "Fire event: the weapon couldn't be raised in time";
				m_ShotsLeft = 0;
				m_LowerTimer = AUTO_LOWER_DELAY;
			}

			return;
		}

		player.GetWeaponManager().Fire(weapon);
		if (GetGame().IsMultiplayer()) {
			OnShotFired.Invoke(player);
		}

		m_ShotsLeft--;
		m_ShotTimer = m_ShotInterval;
		if (m_ShotsLeft <= 0) {
			m_LowerTimer = AUTO_LOWER_DELAY;
		}
	}

	// Drives the raise override towards m_RaiseWanted. If the weapon doesn't follow within RAISE_RETRY seconds,
	// the override is released for one frame and applied again, so the engine sees a fresh press. That covers
	// a raise that only reacts to a change in the input (and the toggle-raise control option).
	static const float RAISE_RETRY = 0.35;

	protected void UpdateRaiseState(PlayerBase player, float dt)
	{
		bool raised = player.IsRaised();
		if (raised != m_LoggedRaised) {
			m_LoggedRaised = raised;
			// Print("[SCPlus Events] weapon raised=" + raised.ToString() + " wanted=" + m_RaiseWanted.ToString() + " raise completed=" + player.IsWeaponRaiseCompleted().ToString());
		}

		if (!m_RaiseOverridden && !m_RaiseWanted) {
			return;
		}

		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		if (raised != m_RaiseWanted) {
			ReleaseController(input, true);
		}

		// Never keep the player's raise input locked for long when lowering (e.g. nothing in hands)
		if (!m_RaiseWanted) {
			m_LowerElapsed += dt;
			if (m_LowerElapsed >= LOWER_GIVE_UP || !Weapon_Base.Cast(player.GetHumanInventory().GetEntityInHands())) {
				ApplyRaiseOverrides(player, false);
				ReleaseController(input, false);
				return;
			}
		}

		if (raised == m_RaiseWanted) {
			m_RaiseRetryTimer = 0;
			m_RaisePulseOff = false;
			ReleaseController(input, false);
			if (m_RaiseWanted) {
				input.OverrideRaise(HumanInputControllerOverrideType.ENABLED, true);
			} else {
				// Weapon is down: hand the raise input back to the player
				ApplyRaiseOverrides(player, false);
			}

			return;
		}

		if (m_RaisePulseOff) {
			// Second half of a nudge: press again
			m_RaisePulseOff = false;
			input.OverrideRaise(HumanInputControllerOverrideType.ENABLED, m_RaiseWanted || !raised);
			return;
		}

		m_RaiseRetryTimer += dt;
		if (m_RaiseRetryTimer >= RAISE_RETRY) {
			m_RaiseRetryTimer = 0;
			m_RaisePulseOff = true;
			input.OverrideRaise(HumanInputControllerOverrideType.ENABLED, !m_RaiseWanted);
			// Print("[SCPlus Events] weapon didn't follow (raised=" + raised.ToString() + ", wanted=" + m_RaiseWanted.ToString() + "), nudging the raise input");
			return;
		}

		input.OverrideRaise(HumanInputControllerOverrideType.ENABLED, m_RaiseWanted);
	}

	// Raise/lower's share of the controller (see UpdateControllerRelease)
	protected void ReleaseController(HumanInputController input, bool release)
	{
		m_ReleaseForRaise = release;
		UpdateControllerRelease(input);
	}

	// Turns the player's controller on (if the camera had it off) while raise/lower, a move or a stance change
	// needs it, with movement held at zero unless a move drives it. Puts it back the way the camera wants it
	// once none of them does.
	protected void UpdateControllerRelease(HumanInputController input)
	{
		bool release = m_ReleaseForRaise || m_MoveTimeLeft > 0 || m_MoveSettle > 0 || m_StanceWanted >= 0 || m_Path != null || m_Track != null;
		if (release == m_ControllerReleased) {
			return;
		}

		if (release) {
			if (!scp_CameraActive) {
				return; // controller is already on outside the camera
			}

			input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
			input.SetDisabled(false);
			m_ControllerReleased = true;
			// Print("[SCPlus Events] controller enabled for an event");
			return;
		}

		input.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
		if (scp_CameraActive) {
			input.SetDisabled(true);
		}

		m_ControllerReleased = false;
		// Print("[SCPlus Events] controller handed back to the camera");
	}

	protected void UpdateMovement(PlayerBase player, float dt)
	{
		if (m_MoveTimeLeft <= 0 && !m_MoveOverridden && m_MoveSettle <= 0) {
			return;
		}

		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		if (m_MoveTimeLeft <= 0 && !m_MoveOverridden) {
			// Settling after a move: hold zero speed, then let go
			m_MoveSettle -= dt;
			input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
			if (m_MoveSettle <= 0) {
				m_MoveSettle = 0;
				if (!m_ControllerReleased) {
					input.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
				}

				UpdateControllerRelease(input);
			}

			return;
		}

m_MoveTimeLeft -= dt;
		if (m_MoveTimeLeft > 0 && player.IsAlive()) {
			UpdateControllerRelease(input);
			input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, m_MoveSpeed);
			input.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, m_MoveAngle);
			m_MoveOverridden = true;
			return;
		}

		StopMove(input);
	}

	protected void UpdatePath(PlayerBase player, float dt)
	{
		if (!m_Path) {
			return;
		}

		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		UpdateControllerRelease(input);
		if (!m_Path.Update(player, dt)) {
			// Print("[SCPlus Paths] path finished");
			EndPath(input, false);
		}
	}

	protected void UpdateTrack(PlayerBase player, float dt)
	{
		if (!m_Track) {
			return;
		}

		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		UpdateControllerRelease(input);
		if (!m_Track.Update(player, dt)) {
			// Print("[SCPlus Track] replay finished");
			EndTrack(input, false);
		}
	}

	// notify_server: the replay was cancelled here (a finished one ends on the server by itself)
	protected void EndTrack(HumanInputController input, bool notify_server)
	{
		if (!m_Track) {
			return;
		}

		m_Track = null;
		SCPlusPathFollower.ClearOverrides(input);
		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
		m_MoveSettle = MOVE_SETTLE_TIME; // UpdateMovement hands the controller back once this runs out
		if (notify_server && GetGame().IsMultiplayer() && GetPlayer()) {
			OnTrackChanged.Invoke(GetPlayer(), 0, 0, null, null);
		}
	}

	// notify_server: the path was cancelled here (a finished path ends on the server by itself)
	protected void EndPath(HumanInputController input, bool notify_server)
	{
		if (!m_Path) {
			return;
		}

		m_Path = null;
		SCPlusPathFollower.ClearOverrides(input);
		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
		m_MoveSettle = MOVE_SETTLE_TIME; // UpdateMovement hands the controller back once this runs out
		if (notify_server && GetGame().IsMultiplayer() && GetPlayer()) {
			OnPathChanged.Invoke(GetPlayer(), 0, null);
		}
	}

	protected void StopMove(HumanInputController input)
	{
		m_MoveTimeLeft = 0;
		if (!m_MoveOverridden) {
			return;
		}

		m_MoveOverridden = false;
		if (GetGame().IsMultiplayer() && GetPlayer()) {
			OnMoveChanged.Invoke(GetPlayer(), 0, 0.0, 0.0);
		}

		input.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);
		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
		m_MoveSettle = MOVE_SETTLE_TIME; // UpdateMovement hands the controller back once this runs out
		// Print("[SCPlus Events] move finished");
	}

	protected void UpdateStance(PlayerBase player, float dt)
	{
		if (m_StanceWanted < 0) {
			return;
		}

		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		UpdateControllerRelease(input);

		HumanMovementState state = new HumanMovementState();
		player.GetMovementState(state);
		bool raised = player.IsRaised();
		int target = GetStanceIdx(m_StanceWanted, raised);
		m_StanceTimer += dt;
		HumanCommandMove move = player.GetCommand_Move();

		if (state.m_iStanceIdx == target || m_StanceTimer >= STANCE_TIMEOUT || !player.IsAlive()) {
			if (state.m_iStanceIdx != target) {
				m_LastMessage = "Stance event: the stance didn't change in time";
			}

			if (move && m_StanceForced) {
				move.ForceStance(-1);
			}

			// Print("[SCPlus Events] stance done (stance idx=" + state.m_iStanceIdx.ToString() + ", wanted=" + target.ToString() + ")");
			m_StanceWanted = -1;
			m_StanceForced = false;
			UpdateControllerRelease(input);
			return;
		}

		// Only possible in the move command (not mid-emote, vault, etc.); keep trying until the timeout
		if (move) {
			move.ForceStance(target);
			m_StanceForced = true;
		}
	}

	protected int GetStanceIdx(int stance, bool raised)
	{
		if (stance == SCPlusStance.CROUCH) {
			if (raised) {
				return DayZPlayerConstants.STANCEIDX_RAISEDCROUCH;
			}

			return DayZPlayerConstants.STANCEIDX_CROUCH;
		}

		if (stance == SCPlusStance.PRONE) {
			if (raised) {
				return DayZPlayerConstants.STANCEIDX_RAISEDPRONE;
			}

			return DayZPlayerConstants.STANCEIDX_PRONE;
		}

		if (raised) {
			return DayZPlayerConstants.STANCEIDX_RAISEDERECT;
		}

		return DayZPlayerConstants.STANCEIDX_ERECT;
	}

	protected void SetRaised(PlayerBase player, bool raised)
	{
		m_RaiseWanted = raised;
		m_RaiseRetryTimer = 0;
		m_LowerElapsed = 0;
		m_RaisePulseOff = false;
		// Print("[SCPlus Events] raise requested=" + raised.ToString() + " (currently raised=" + player.IsRaised().ToString() + ")");
		if (raised) {
			ApplyRaiseOverrides(player, true);
		}
		// Lowering: UpdateRaiseState holds the raise input off until the weapon is down, then releases it
	}

	// Raise, and no procedural recoil while the event owns the weapon (the module holds the aim still while
	// the cinematic camera is open). With the cinematic
	// camera active the aim filters don't seem to clear recoil's mouse shift, so after one shot the aim kept
	// climbing until the rifle pointed straight up (zeroing the aim input alone didn't stop it).
	protected void ApplyRaiseOverrides(PlayerBase player, bool enable)
	{
		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		if (enable) {
			input.OverrideRaise(HumanInputControllerOverrideType.ENABLED, true);
			m_RaiseOverridden = true;
			if (player.GetAimingModel()) {
				player.GetAimingModel().SetProceduralRecoilEnabled(false);
			}
		} else {
			input.OverrideRaise(HumanInputControllerOverrideType.DISABLED, false);
			m_RaiseOverridden = false;
			if (player.GetAimingModel()) {
				player.GetAimingModel().SetProceduralRecoilEnabled(true);
			}
		}
	}

	// Stops pending shots and hands the weapon back to the player (playback stopped or camera closed)
	void Reset()
	{
		CancelActorPreview();
		m_ShotsLeft = 0;
		m_RaiseHeld = false;
		m_LowerTimer = -1;
		PlayerBase player = GetPlayer();
		if (player) {
			SetRaised(player, false);
			HumanInputController input = player.GetInputController();
			if (input) {
				EndPath(input, true);
				EndTrack(input, true);
				StopMove(input);
				if (m_StanceWanted >= 0) {
					HumanCommandMove move = player.GetCommand_Move();
					if (move && m_StanceForced) {
						move.ForceStance(-1);
					}

					m_StanceWanted = -1;
					m_StanceForced = false;
					UpdateControllerRelease(input);
				}
			}
		}
	}

	// Stops every event and puts the player back at the scene start (Stop, loop restart)
	void ResetActor(vector position, vector orientation)
	{
		Reset();
		PlayerBase player = GetPlayer();
		if (!player || !player.IsAlive()) {
			return;
		}

		// Print("[SCPlus Events] actor reset to " + position.ToString());
		if (GetGame().IsMultiplayer()) {
			OnActorReset.Invoke(player, position, orientation);
		}

		player.SetPosition(position);
		player.SetOrientation(orientation);
	}

	// Server side of a client's actor reset (multiplayer only)
	static void ResetActorOnServer(PlayerBase player, vector position, vector orientation)
	{
		if (!player || !player.IsAlive() || SCPlusHasNaN(position) || SCPlusHasNaN(orientation)) {
			return;
		}

		if (vector.Distance(player.GetPosition(), position) > ACTOR_RESET_MAX_DISTANCE) {
			return;
		}

		MoveOnServer(player, 0, 0, 0);
		player.SetPosition(position);
		player.SetOrientation(orientation);
	}

	// --- Scrub preview ---
	// While paused with Replay Actor Track on, scrubbing the timeline puts the actor where the track has them at
	// the playhead. The client moves at once; in multiplayer the server is told at most every
	// ACTOR_PREVIEW_INTERVAL seconds (through the actor reset RPC), and always gets the last pose of a drag.
	static const float ACTOR_PREVIEW_INTERVAL = 0.15;
	protected float m_PreviewLastSend = -1000;
	protected bool m_PreviewPending;
	protected vector m_PreviewPosition;
	protected vector m_PreviewOrientation;

	void PreviewActor(vector position, vector orientation)
	{
		PlayerBase player = GetPlayer();
		if (!player || !player.IsAlive() || SCPlusHasNaN(position) || SCPlusHasNaN(orientation)) {
			return;
		}

		player.SetPosition(position);
		player.SetOrientation(orientation);
		if (!GetGame().IsMultiplayer()) {
			return;
		}

		m_PreviewPosition = position;
		m_PreviewOrientation = orientation;
		if (m_PreviewPending) {
			return; // the queued send picks up the newest pose
		}

		float wait = m_PreviewLastSend + ACTOR_PREVIEW_INTERVAL - GetGame().GetTickTime();
		if (wait <= 0) {
			SendActorPreview();
			return;
		}

		m_PreviewPending = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SendActorPreview, Math.Ceil(wait * 1000), false);
	}

	protected void SendActorPreview()
	{
		m_PreviewPending = false;
		m_PreviewLastSend = GetGame().GetTickTime();
		PlayerBase player = GetPlayer();
		if (player && player.IsAlive()) {
			OnActorReset.Invoke(player, m_PreviewPosition, m_PreviewOrientation);
		}
	}

	// Drops a queued preview send (playback, a reset or a track replay takes over the actor)
	void CancelActorPreview()
	{
		if (!m_PreviewPending) {
			return;
		}

		m_PreviewPending = false;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(SendActorPreview);
	}

	// Furthest a scene target may be moved by a reset
	static const float TARGET_RESET_MAX_DISTANCE = 1000;

	// Animals and zombies, vehicles and players can be scene targets; buildings and other static objects can't
	static bool IsResettableTarget(Object target)
	{
		if (!target) {
			return false;
		}

		return target.IsInherited(DayZCreatureAI) || target.IsInherited(Transport) || target.IsInherited(PlayerBase);
	}

	// Puts the scene target back at its start: on the server in multiplayer, directly in single player
	static void ResetTarget(Object target, vector position, vector orientation)
	{
		if (!IsResettableTarget(target)) {
			return;
		}

		// Print(string.Format("[SCPlus Events] scene target %1 reset to %2", target.GetType(), position.ToString()));
		if (GetGame().IsMultiplayer() && GetGame().IsClient()) {
			OnTargetReset.Invoke(target, position, orientation);
			return;
		}

		ResetTargetOnServer(target, position, orientation);
	}

	// Server side of a scene target reset
	static void ResetTargetOnServer(Object target, vector position, vector orientation)
	{
		if (!IsResettableTarget(target) || SCPlusHasNaN(position) || SCPlusHasNaN(orientation)) {
			return;
		}

		if (vector.Distance(target.GetPosition(), position) > TARGET_RESET_MAX_DISTANCE) {
			return;
		}

		PlayerBase player = PlayerBase.Cast(target);
		if (player) {
			if (!player.IsAlive()) {
				return;
			}

			MoveOnServer(player, 0, 0, 0);
		}

		target.SetPosition(position);
		target.SetOrientation(orientation);

		// A vehicle keeps its momentum through a teleport unless it is stopped
		Transport vehicle = Transport.Cast(target);
		if (vehicle) {
			SetVelocity(vehicle, vector.Zero);
			dBodySetAngularVelocity(vehicle, vector.Zero);
			vehicle.Synchronize();
		}
	}

	static bool SCPlusHasNaN(vector v)
	{
		return v[0] != v[0] || v[1] != v[1] || v[2] != v[2];
	}

	// Server side of a client's move (multiplayer only); speed 0 ends it. A safety timer ends it anyway a
	// second after its duration, in case the client's stop never arrives.
	static void MoveOnServer(PlayerBase player, int speed, float angle, float duration)
	{
		if (!player || !player.GetInputController()) {
			return;
		}

		if (!s_ServerMoveTokens) {
			s_ServerMoveTokens = new map<PlayerBase, int>();
		}

		int token = s_ServerMoveTokens.Get(player) + 1;
		s_ServerMoveTokens.Set(player, token);
		HumanInputController input = player.GetInputController();
		if (speed <= 0 || !player.IsAlive()) {
			input.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
			input.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);
			// Keep the token: removing it restarted the count at 1, so the previous move's safety timer
			// (also token 1) stopped the NEXT move a second after the old one should have ended. That made
			// back-to-back Walk -> Jog -> Sprint stall on the server while the client kept moving.
			return;
		}

		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, Math.Clamp(speed, 1, 3));
		input.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, angle);
		int delay_ms = (Math.Clamp(duration, 0.1, 60) + 1) * 1000;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StopMoveOnServer, delay_ms, false, player, token);
	}

	static void StopMoveOnServer(PlayerBase player, int token)
	{
		if (!player || !s_ServerMoveTokens || s_ServerMoveTokens.Get(player) != token) {
			return;
		}

		MoveOnServer(player, 0, 0, 0);
	}

	// Server side of a client's shot (multiplayer only)
	static void FireOnServer(PlayerBase player)
	{
		if (!player || !player.IsAlive() || !player.GetWeaponManager()) {
			return;
		}

		Weapon_Base weapon = Weapon_Base.Cast(player.GetHumanInventory().GetEntityInHands());
		if (!weapon || weapon.IsDamageDestroyed()) {
			return;
		}

		player.GetWeaponManager().Fire(weapon);
	}
}
