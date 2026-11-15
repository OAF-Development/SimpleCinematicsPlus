// Steers a player along the path curve for the PATH event. The same follower runs on the client (for
// prediction) and on the server (which owns the movement), each from its own copy of the player's position.
//
// Pure pursuit: the follower tracks how far along the path the player is (the closest point on the curve,
// only ever moving forward) and steers at a point LOOKAHEAD metres further on. That cuts smoothly through
// the dense points of a rounded corner instead of chasing each one. The player never stops to turn; on a
// sharp turn they drop to walking pace. The aim direction that turns the body was found in game to be -1;
// it's still checked once and flipped if the gap clearly grows.
class SCPlusPathFollower: Managed
{
	static const float LOOKAHEAD_WALK = 1.6; // metres ahead on the path to steer at
	static const float LOOKAHEAD_JOG = 2.4;
	static const float LOOKAHEAD_SPRINT = 3.4;
	static const float ARRIVE = 0.7; // metres from the end that count as arrived
	static const float SLOW_TURN = 50; // degrees: walk while turning this sharply
	static const float AIM_GAIN = 0.25;
	static const float AIM_MAX = 0.08; // radians per tick
	static const float MIN_SPEED = 0.8; // m/s, for the give-up time
	static const int MAX_POINTS = 600;
	static const int SEARCH_SEGMENTS = 12; // how far ahead the closest-point search looks

	protected ref array<vector> m_Points = new array<vector>();
	protected ref array<float> m_Distances = new array<float>(); // path length up to each point
	protected float m_Length;
	protected int m_Speed;
	protected int m_Segment; // segment the player is on
	protected float m_Progress; // metres along the path
	protected float m_Lookahead;
	protected float m_AimSign = -1;
	protected bool m_SignChecked;
	protected float m_SignWatch = -1;
	protected float m_SignStartDiff;
	protected float m_Elapsed;
	protected float m_TimeLimit;
	protected bool m_Verbose;
	protected float m_LogTimer;

	void SCPlusPathFollower(array<vector> points, int speed, bool verbose)
	{
		if (points) {
			for (int i = 0; i < points.Count() && i < MAX_POINTS; i++) {
				m_Points.Insert(points[i]);
			}
		}

		m_Speed = Math.Clamp(speed, 1, 3);
		m_Verbose = verbose;
		m_Lookahead = LOOKAHEAD_WALK;
		if (m_Speed == 2) {
			m_Lookahead = LOOKAHEAD_JOG;
		} else if (m_Speed >= 3) {
			m_Lookahead = LOOKAHEAD_SPRINT;
		}

		m_Distances.Insert(0);
		for (int k = 0; k + 1 < m_Points.Count(); k++) {
			m_Length += FlatDistance(m_Points[k], m_Points[k + 1]);
			m_Distances.Insert(m_Length);
		}

		m_TimeLimit = m_Length / MIN_SPEED + 15;
	}

	int GetPointCount()
	{
		return m_Points.Count();
	}

	static float FlatDistance(vector a, vector b)
	{
		vector flat = b - a;
		flat[1] = 0;
		return flat.Length();
	}

	// The point "distance" metres along the path
	protected vector PointAt(float distance)
	{
		int last = m_Points.Count() - 1;
		if (distance >= m_Length) {
			return m_Points[last];
		}

		for (int i = Math.Max(m_Segment, 0); i < last; i++) {
			if (m_Distances[i + 1] >= distance) {
				float segment_length = m_Distances[i + 1] - m_Distances[i];
				if (segment_length < 0.0001) {
					return m_Points[i + 1];
				}

				return vector.Lerp(m_Points[i], m_Points[i + 1], (distance - m_Distances[i]) / segment_length);
			}
		}

		return m_Points[last];
	}

	// Moves m_Segment / m_Progress to the closest point on the path ahead of where the player was
	protected void UpdateProgress(vector position)
	{
		int last = m_Points.Count() - 1;
		float best = 1000000;
		int best_segment = m_Segment;
		float best_progress = m_Progress;
		int search_end = Math.Min(m_Segment + SEARCH_SEGMENTS, last);
		for (int i = m_Segment; i < search_end; i++) {
			vector a = m_Points[i];
			vector b = m_Points[i + 1];
			vector ab = b - a;
			ab[1] = 0;
			vector ap = position - a;
			ap[1] = 0;
			float length_sq = vector.Dot(ab, ab);
			float t = 0;
			if (length_sq > 0.000001) {
				t = Math.Clamp(vector.Dot(ap, ab) / length_sq, 0, 1);
			}

			vector closest = a + ab * t;
			closest[1] = position[1];
			float distance = vector.Distance(closest, position);
			if (distance < best) {
				best = distance;
				best_segment = i;
				best_progress = m_Distances[i] + (m_Distances[i + 1] - m_Distances[i]) * t;
			}
		}

		if (best_progress >= m_Progress) {
			m_Segment = best_segment;
			m_Progress = best_progress;
		}
	}

	// Steers the player for one frame; false once the end is reached (or it gave up)
	bool Update(PlayerBase player, float dt)
	{
		HumanInputController input = player.GetInputController();
		if (!input || !player.IsAlive() || m_Points.Count() == 0) {
			return false;
		}

		m_Elapsed += dt;
		if (m_Elapsed > m_TimeLimit) {
			// Print("[SCPlus Paths] gave up at " + m_Progress.ToString() + " of " + m_Length.ToString() + " m (took too long)");
			return false;
		}

		vector position = player.GetPosition();
		if (m_Points.Count() > 1) {
			UpdateProgress(position);
		}

		vector end_point = m_Points[m_Points.Count() - 1];
		if (FlatDistance(position, end_point) < ARRIVE && m_Progress > m_Length - m_Lookahead - ARRIVE) {
			return false;
		}

		vector to_target = PointAt(m_Progress + m_Lookahead) - position;
		to_target[1] = 0;
		if (to_target.Length() < 0.05) {
			return false;
		}

		vector target_angles = to_target.VectorToAngles();
		vector body = player.GetOrientation();
		float diff = target_angles[0] - body[0];
		while (diff > 180) {
			diff -= 360;
		}

		while (diff < -180) {
			diff += 360;
		}

		UpdateSignCheck(Math.AbsFloat(diff), dt);

		float aim = Math.Clamp(diff * Math.DEG2RAD * AIM_GAIN, -AIM_MAX, AIM_MAX) * m_AimSign;
		input.OverrideAimChangeX(HumanInputControllerOverrideType.ENABLED, aim);
		input.OverrideAimChangeY(HumanInputControllerOverrideType.ENABLED, 0);

		float speed = m_Speed;
		if (Math.AbsFloat(diff) > SLOW_TURN) {
			speed = 1;
		}

		input.OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, speed);
		input.OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, 0);

		if (m_Verbose) {
			m_LogTimer -= dt;
			if (m_LogTimer <= 0) {
				m_LogTimer = 1;
				float off_path = FlatDistance(position, PointAt(m_Progress));
				// Print("[SCPlus Paths] " + m_Progress.ToString() + "/" + m_Length.ToString() + " m, off path " + off_path.ToString() + " m, turn=" + diff.ToString() + " sign=" + m_AimSign.ToString());
			}
		}

		return true;
	}

	// The first real turn shows which aim direction turns the body towards the target
	protected void UpdateSignCheck(float abs_diff, float dt)
	{
		if (m_SignChecked) {
			return;
		}

		if (m_SignWatch < 0) {
			if (abs_diff > 15) {
				m_SignWatch = 0;
				m_SignStartDiff = abs_diff;
			}

			return;
		}

		m_SignWatch += dt;
		if (m_SignWatch < 0.4) {
			return;
		}

		m_SignChecked = true;
		if (abs_diff > m_SignStartDiff + 5) {
			m_AimSign = -m_AimSign;
			// Print("[SCPlus Paths] turning the wrong way, flipped the aim direction");
		}
	}

	// Hands the aim and movement inputs back (speed is left at zero override: the caller decides)
	static void ClearOverrides(HumanInputController input)
	{
		if (!input) {
			return;
		}

		input.OverrideAimChangeX(HumanInputControllerOverrideType.DISABLED, 0);
		input.OverrideAimChangeY(HumanInputControllerOverrideType.DISABLED, 0);
		input.OverrideMovementAngle(HumanInputControllerOverrideType.DISABLED, 0);
	}

	// --- Server side (multiplayer) ---
	protected static ref map<PlayerBase, ref SCPlusPathFollower> s_ServerPaths;
	protected static bool s_ServerTicking;
	protected static int s_LastServerTick;

	// speed 0 (or no points) stops the player's path
	static void PathOnServer(PlayerBase player, int speed, array<vector> points)
	{
		if (!player) {
			return;
		}

		if (!s_ServerPaths) {
			s_ServerPaths = new map<PlayerBase, ref SCPlusPathFollower>();
		}

		if (s_ServerPaths.Contains(player)) {
			s_ServerPaths.Remove(player);
			StopOnServer(player);
		}

		if (speed <= 0 || !points || points.Count() == 0 || !player.IsAlive()) {
			return;
		}

		// Only paths that start near the player (a client can only steer its own character anyway)
		if (vector.Distance(player.GetPosition(), points[0]) > 1000) {
			return;
		}

		s_ServerPaths.Set(player, new SCPlusPathFollower(points, speed, false));
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

		ClearOverrides(input);
		input.OverrideMovementSpeed(HumanInputControllerOverrideType.DISABLED, 0);
	}

	protected static void ServerTick()
	{
		int now = GetGame().GetTime();
		float dt = (now - s_LastServerTick) / 1000.0;
		s_LastServerTick = now;
		if (!s_ServerPaths || s_ServerPaths.Count() == 0) {
			return;
		}

		array<PlayerBase> finished = new array<PlayerBase>();
		foreach (PlayerBase player, SCPlusPathFollower follower: s_ServerPaths) {
			if (!player || !follower || !follower.Update(player, dt)) {
				finished.Insert(player);
			}
		}

		foreach (PlayerBase done: finished) {
			s_ServerPaths.Remove(done);
			if (done) {
				StopOnServer(done);
			}
		}
	}
}
