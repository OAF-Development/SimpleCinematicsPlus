// Replays the Auto-Key actor track: walks the player along the recorded (and maybe edited) track so they are
// where the track says at each moment of playback. Like SCPlusPathFollower it runs on the client (prediction)
// and on the server (which owns the movement), each from its own copy of the samples.
//
// DayZ only animates a player from movement input, so the actor can't be placed exactly; the follower picks
// walk / jog / sprint from the speed the track needs plus how far behind or ahead the actor is, and steers at
// a point a little further along the track. Expect the actor within a few tenths of a metre on a steady walk,
// more on sudden speed changes (the gaits are fixed speeds). Where the track stands still the actor stops
// and turns to the recorded heading.
//
// The track arrives as evenly spaced samples (position, heading) built by the camera from the actor keys.

// Turns the body towards a heading through the aim override, settling which sign of the override turns
// it which way from how the body really turns (see SCPlusPathFollower for why it can't be assumed).
// Once settled on this machine, later steering starts with that sign.
class SCPlusSteering: Managed
{
	static const float AIM_GAIN = 0.2;
	static const float AIM_MAX = 0.06; // radians per tick
	static const float AIM_DEADZONE = 1.5; // degrees
	static const float AIM_SMOOTH = 0.35;
	static const int SIGN_SAMPLES = 12;
	static const float SIGN_MIN_TURN = 0.4;

	protected static float s_SettledSign;

	protected float m_AimSign = 1;
	protected bool m_SignChecked;
	protected int m_SignGood;
	protected int m_SignBad;
	protected float m_LastHeading;
	protected bool m_HasLastHeading;
	protected float m_LastDiff;
	protected float m_Aim;

	void SCPlusSteering()
	{
		if (s_SettledSign != 0) {
			m_AimSign = s_SettledSign;
			m_SignChecked = true;
		}
	}

	static float WrapDiff(float diff)
	{
		while (diff > 180) {
			diff -= 360;
		}

		while (diff < -180) {
			diff += 360;
		}

		return diff;
	}

	// diff: degrees from the body's heading to the wanted heading. Applies the aim override.
	void Steer(HumanInputController input, float heading, float diff, float gain_scale = 1)
	{
		UpdateSignCheck(heading, diff);
		float wanted = 0;
		if (Math.AbsFloat(diff) > AIM_DEADZONE) {
			wanted = Math.Clamp(diff * Math.DEG2RAD * AIM_GAIN * gain_scale, -AIM_MAX, AIM_MAX) * m_AimSign;
		}

		m_Aim = Math.Lerp(m_Aim, wanted, AIM_SMOOTH);
		input.OverrideAimChangeX(HumanInputControllerOverrideType.ENABLED, m_Aim);
		input.OverrideAimChangeY(HumanInputControllerOverrideType.ENABLED, 0);
	}

	protected void UpdateSignCheck(float heading, float diff)
	{
		if (m_SignChecked) {
			return;
		}

		if (m_HasLastHeading && Math.AbsFloat(m_LastDiff) > 4) {
			float turned = WrapDiff(heading - m_LastHeading);
			if (Math.AbsFloat(turned) >= SIGN_MIN_TURN) {
				if ((turned > 0) == (m_LastDiff > 0)) {
					m_SignGood++;
				} else {
					m_SignBad++;
				}
			}
		}

		m_LastHeading = heading;
		m_HasLastHeading = true;
		m_LastDiff = diff;
		if (m_SignGood + m_SignBad < SIGN_SAMPLES) {
			return;
		}

		m_SignChecked = true;
		if (m_SignBad > m_SignGood * 2) {
			m_AimSign = -m_AimSign;
			m_Aim = 0;
		}

		s_SettledSign = m_AimSign;
	}
}

class SCPlusTrackFollower: Managed
{
	// Ground speeds of the gaits (m/s, erect, flat ground; estimates, the position term corrects for them)
	static const float WALK_SPEED = 1.6;
	static const float JOG_SPEED = 4.2;
	static const float SPRINT_SPEED = 6.4;
	static const float GEAR_HYSTERESIS = 0.3; // m/s either side of the switch point, so the gait doesn't flicker
	static const float CATCH_UP_GAIN = 1.2; // extra m/s per metre behind the track
	static const float STILL_SPEED = 0.25; // m/s: slower than this the track counts as standing
	static const float STILL_RADIUS = 0.45; // metres: this close to a standing track the actor stops
	static const float LEAD_TIME = 0.5; // seconds ahead on the track to steer at
	static const float LEAD_DISTANCE = 1.2; // metres ahead at least
	static const float SLOW_TURN = 50; // degrees: walk while turning this sharply
	static const float END_GRACE = 2.0; // seconds after the track ends to finish the last turn
	static const int MAX_SAMPLES = 1500;

	protected ref array<vector> m_Points = new array<vector>();
	protected ref array<float> m_Headings = new array<float>();
	protected float m_Step;
	protected float m_Time;
	protected float m_EndTime;
	protected int m_Gear;
	protected ref SCPlusSteering m_Steering = new SCPlusSteering();

	void SCPlusTrackFollower(array<vector> points, array<float> headings, float step, float start_time)
	{
		if (points) {
			for (int i = 0; i < points.Count() && i < MAX_SAMPLES; i++) {
				m_Points.Insert(points[i]);
				float heading = 0;
				if (headings && headings.IsValidIndex(i)) {
					heading = headings[i];
				}

				m_Headings.Insert(heading);
			}
		}

		m_Step = Math.Max(step, 0.02);
		m_Time = Math.Max(start_time, 0);
		m_EndTime = Math.Max(m_Points.Count() - 1, 0) * m_Step;
	}

	int GetSampleCount()
	{
		return m_Points.Count();
	}

	protected vector PositionAt(float time)
	{
		int last = m_Points.Count() - 1;
		float index = Math.Clamp(time / m_Step, 0, last);
		int low = Math.Floor(index);
		if (low >= last) {
			return m_Points[last];
		}

		return vector.Lerp(m_Points[low], m_Points[low + 1], index - low);
	}

	protected float HeadingAt(float time)
	{
		int last = m_Headings.Count() - 1;
		int nearest = Math.Clamp(Math.Round(time / m_Step), 0, last);
		return m_Headings[nearest];
	}

	static float Flat(vector v)
	{
		v[1] = 0;
		return v.Length();
	}

	// Gait 0 stop, 1 walk, 2 jog, 3 sprint for a wanted ground speed, with hysteresis around the switch points
	protected int PickGear(float wanted)
	{
		array<float> speeds = {0, WALK_SPEED, JOG_SPEED, SPRINT_SPEED};
		int gear = m_Gear;
		while (gear < 3 && wanted > (speeds[gear] + speeds[gear + 1]) / 2 + GEAR_HYSTERESIS) {
			gear++;
		}

		while (gear > 0 && wanted < (speeds[gear - 1] + speeds[gear]) / 2 - GEAR_HYSTERESIS) {
			gear--;
		}

		return gear;
	}

	// Steers the player for one frame; false once the track is over
	bool Update(PlayerBase player, float dt)
	{
		HumanInputController input = player.GetInputController();
		if (!input || !player.IsAlive() || m_Points.Count() < 2) {
			return false;
		}

		m_Time += dt;
		if (m_Time > m_EndTime + END_GRACE) {
			return false;
		}

		vector position = player.GetPosition();
		vector body = player.GetOrientation();
		vector expected = PositionAt(m_Time);
		vector velocity = PositionAt(m_Time + 0.25) - PositionAt(m_Time - 0.25);
		velocity[1] = 0;
		float track_speed = velocity.Length() / 0.5;
		if (m_Time > m_EndTime) {
			track_speed = 0;
		}

		vector error = expected - position;
		error[1] = 0;
		float error_length = error.Length();

		if (track_speed < STILL_SPEED && error_length < STILL_RADIUS) {
			// Standing on the track: stop and face the recorded heading
			m_Gear = 0;
			input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, 0);
			input.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, 0);
			float stand_diff = SCPlusSteering.WrapDiff(HeadingAt(m_Time) - body[0]);
			m_Steering.Steer(input, body[0], stand_diff, 0.6);
			if (m_Time > m_EndTime && Math.AbsFloat(stand_diff) < 4) {
				return false;
			}

			return true;
		}

		// Behind (positive) or ahead (negative) of where the track is now, along its direction
		float along = error_length;
		if (track_speed >= STILL_SPEED) {
			vector direction = velocity.Normalized();
			along = vector.Dot(error, direction);
		}

		// Steer at a point a little further on (or straight at the track when it stands still)
		float lead = LEAD_TIME;
		if (track_speed > 0.1) {
			lead = Math.Max(LEAD_TIME, LEAD_DISTANCE / track_speed);
		}

		vector aim_point = PositionAt(m_Time + lead);
		if (Flat(aim_point - position) < 0.3) {
			aim_point = expected;
		}

		vector to_aim = aim_point - position;
		to_aim[1] = 0;
		float diff = 0;
		if (to_aim.Length() > 0.05) {
			vector aim_angles = to_aim.VectorToAngles();
			diff = SCPlusSteering.WrapDiff(aim_angles[0] - body[0]);
		}

		m_Steering.Steer(input, body[0], diff);

		float wanted = Math.Clamp(track_speed + CATCH_UP_GAIN * along, 0, SPRINT_SPEED + 1);
		m_Gear = PickGear(wanted);
		int gear = m_Gear;
		if (gear > 1 && Math.AbsFloat(diff) > SLOW_TURN) {
			gear = 1;
		}

		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, gear);
		input.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, 0);
		return true;
	}

	// --- Server side (multiplayer) ---
	protected static ref map<PlayerBase, ref SCPlusTrackFollower> s_ServerTracks;
	protected static bool s_ServerTicking;
	protected static int s_LastServerTick;

	// No points stops the player's track
	static void TrackOnServer(PlayerBase player, float step, float start_time, array<vector> points, array<float> headings)
	{
		if (!player) {
			return;
		}

		if (!s_ServerTracks) {
			s_ServerTracks = new map<PlayerBase, ref SCPlusTrackFollower>();
		}

		if (s_ServerTracks.Contains(player)) {
			s_ServerTracks.Remove(player);
			StopOnServer(player);
		}

		if (!points || points.Count() < 2 || !player.IsAlive()) {
			return;
		}

		// Only tracks that start near the player (a client can only steer its own character anyway)
		if (vector.Distance(player.GetPosition(), points[0]) > 1000) {
			return;
		}

		s_ServerTracks.Set(player, new SCPlusTrackFollower(points, headings, step, start_time));
		if (!s_ServerTicking) {
			s_ServerTicking = true;
			s_LastServerTick = GetGame().GetTime();
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ServerTick, 20, true);
		}
	}

	protected static void StopOnServer(PlayerBase player)
	{
		HumanInputController input = player.GetInputController();
		if (!input) {
			return;
		}

		SCPlusPathFollower.ClearOverrides(input);
		input.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
	}

	protected static void ServerTick()
	{
		int now = GetGame().GetTime();
		float dt = (now - s_LastServerTick) / 1000.0;
		s_LastServerTick = now;
		if (!s_ServerTracks || s_ServerTracks.Count() == 0) {
			return;
		}

		array<PlayerBase> finished = new array<PlayerBase>();
		foreach (PlayerBase player, SCPlusTrackFollower follower: s_ServerTracks) {
			if (!player || !follower || !follower.Update(player, dt)) {
				finished.Insert(player);
			}
		}

		foreach (PlayerBase done: finished) {
			s_ServerTracks.Remove(done);
			if (done) {
				StopOnServer(done);
			}
		}
	}
}
