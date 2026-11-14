// Waypoint path visuals: flat semi-transparent, slightly emissive strips laid along the path, and a disc on
// each waypoint (a bigger white one on the selected waypoint). World-space models (not overlay lines), so
// walls and terrain hide them like real objects. All local: spawned with ECE_LOCAL, never networked or saved.
// Hidden during playback so they never end up in a shot.
//
// The path is the curve from BuildCurve: each waypoint's corner is rounded by its Curve value (0 = sharp
// corner, 1 = fully rounded), like a spline. The Follow Path event walks the same curve.
//
// Strip colour, per piece:
//  - red: knee or chest height blocked, slope too steep to walk, or deep water
//  - yellow: steep but walkable, something close to either edge, or shallow water
//  - green: everything else
class SCPlusPathVisuals
{
	static const float PIECE_SPACING = 0.8; // strips are 0.8 m long, laid end to end
	static const float LIFT = 0.04; // above the ground so the strips don't flicker into it
	static const int MAX_PIECES = 1000;
	static const float SLOPE_YELLOW = 20; // degrees
	static const float SLOPE_RED = 38;
	static const float WATER_YELLOW = 0.2; // metres
	static const float WATER_RED = 1.1;
	static const float EDGE_OFFSET = 0.35; // side rays for the yellow "obstacle near the edge" check
	static const int REBUILD_INTERVAL_MS = 120; // while dragging, rebuild at most this often
	static const float DEFAULT_CURVE = 1.0;
	static const int CORNER_STEPS = 8;

	protected static ref array<Object> s_Objects = new array<Object>();
	protected static bool s_Shown;
	protected static bool s_Dirty;
	protected static int s_LastBuild;

	// Next Update rebuilds (call after the waypoints change)
	static void MarkDirty()
	{
		s_Dirty = true;
	}

	// Call every frame: shows/hides/rebuilds as needed
	static void Update(bool show, array<vector> points, array<float> curves, int selected)
	{
		if (!show || !points || points.Count() == 0) {
			if (s_Shown) {
				Clear();
			}

			return;
		}

		if (s_Shown && !s_Dirty) {
			return;
		}

		if (s_Shown && GetGame().GetTime() - s_LastBuild < REBUILD_INTERVAL_MS) {
			return;
		}

		Rebuild(points, curves, selected);
	}

	static void Clear()
	{
		foreach (Object path_object: s_Objects) {
			if (path_object) {
				GetGame().ObjectDelete(path_object);
			}
		}

		s_Objects.Clear();
		s_Shown = false;
	}

	static float GetCurve(array<float> curves, int index)
	{
		if (curves && curves.IsValidIndex(index)) {
			return Math.Clamp(curves[index], 0, 1);
		}

		return DEFAULT_CURVE;
	}

	// The path as a polyline: the waypoints, with each inner corner rounded by its Curve value (a quadratic
	// Bezier from a point before the corner to a point after it, the corner itself as the control point)
	static array<vector> BuildCurve(array<vector> points, array<float> curves)
	{
		array<vector> result = new array<vector>();
		if (!points) {
			return result;
		}

		int count = points.Count();
		for (int i = 0; i < count; i++) {
			vector corner = points[i];
			float curve = GetCurve(curves, i);
			if (i == 0 || i == count - 1 || curve < 0.01) {
				result.Insert(corner);
				continue;
			}

			vector before = points[i - 1];
			vector after = points[i + 1];
			float radius = curve * 0.5 * Math.Min(vector.Distance(before, corner), vector.Distance(corner, after));
			if (radius < 0.05) {
				result.Insert(corner);
				continue;
			}

			vector arc_start = corner + (before - corner).Normalized() * radius;
			vector arc_end = corner + (after - corner).Normalized() * radius;
			for (int k = 0; k <= CORNER_STEPS; k++) {
				float t = k;
				t = t / CORNER_STEPS;
				float u = 1 - t;
				result.Insert(arc_start * (u * u) + corner * (2 * u * t) + arc_end * (t * t));
			}
		}

		return result;
	}

	// Points every "spacing" metres along a polyline (always including both ends)
	static array<vector> Resample(array<vector> line, float spacing)
	{
		array<vector> result = new array<vector>();
		if (!line || line.Count() == 0) {
			return result;
		}

		result.Insert(line[0]);
		float carried; // distance walked since the last sample
		for (int i = 0; i + 1 < line.Count(); i++) {
			vector from = line[i];
			vector to = line[i + 1];
			float length = vector.Distance(from, to);
			if (length < 0.0001) {
				continue;
			}

			float walked = spacing - carried;
			while (walked <= length && result.Count() < MAX_PIECES) {
				result.Insert(vector.Lerp(from, to, walked / length));
				walked += spacing;
			}

			carried = length - (walked - spacing);
		}

		vector last = line[line.Count() - 1];
		if (vector.Distance(result[result.Count() - 1], last) > 0.05) {
			result.Insert(last);
		}

		return result;
	}

	protected static void Rebuild(array<vector> points, array<float> curves, int selected)
	{
		Clear();
		s_Dirty = false;
		s_Shown = true;
		s_LastBuild = GetGame().GetTime();

		for (int i = 0; i < points.Count(); i++) {
			string point_type = "SCPlusPathPoint";
			if (i == selected) {
				point_type = "SCPlusPathPointSelected";
			}

			Spawn(point_type, GroundPoint(points[i]), vector.Zero);
		}

		array<vector> samples = Resample(BuildCurve(points, curves), PIECE_SPACING);
		for (int k = 0; k + 1 < samples.Count(); k++) {
			vector piece_start = GroundPoint(samples[k]);
			vector piece_end = GroundPoint(samples[k + 1]);
			vector direction = piece_end - piece_start;
			if (direction.Length() < 0.01) {
				continue;
			}

			vector centre = (piece_start + piece_end) * 0.5;
			vector angles = direction.VectorToAngles();
			Spawn(GetPieceClass(piece_start, piece_end), centre, Vector(angles[0], angles[1], 0));
		}
	}

	// The point on the ground (or the floor it was placed on) plus the lift
	static vector GroundPoint(vector point)
	{
		float surface = GetGame().SurfaceY(point[0], point[2]);
		if (point[1] < surface) {
			point[1] = surface;
		}

		point[1] = point[1] + LIFT;
		return point;
	}

	protected static void Spawn(string type, vector position, vector orientation)
	{
		Object path_object = GetGame().CreateObjectEx(type, position, ECE_LOCAL);
		if (!path_object) {
			return;
		}

		path_object.SetOrientation(orientation);
		path_object.SetPosition(position);
		s_Objects.Insert(path_object);
	}

	protected static string GetPieceClass(vector start, vector end)
	{
		int severity = Severity(start, end);
		if (severity >= 2) {
			return "SCPlusPathRed";
		}

		if (severity == 1) {
			return "SCPlusPathYellow";
		}

		return "SCPlusPathGreen";
	}

	// 0 green, 1 yellow, 2 red
	static int Severity(vector start, vector end)
	{
		int severity;
		vector flat = end - start;
		flat[1] = 0;
		float run = flat.Length();
		if (run > 0.01) {
			float slope = Math.Atan2(Math.AbsFloat(end[1] - start[1]), run) * Math.RAD2DEG;
			if (slope >= SLOPE_RED) {
				return 2;
			}

			if (slope >= SLOPE_YELLOW) {
				severity = 1;
			}
		}

		// Knee and chest height straight along the piece
		if (IsBlocked(start, end, 0.5) || IsBlocked(start, end, 1.3)) {
			return 2;
		}

		// Something close to either edge
		if (run > 0.01) {
			vector side = Vector(-flat[2], 0, flat[0]).Normalized() * EDGE_OFFSET;
			if (IsBlocked(start + side, end + side, 0.5) || IsBlocked(start - side, end - side, 0.5)) {
				severity = 1;
			}
		}

		vector middle = (start + end) * 0.5;
		float water = GetGame().GetWaterDepth(middle);
		if (water >= WATER_RED) {
			return 2;
		}

		if (water >= WATER_YELLOW) {
			severity = 1;
		}

		return severity;
	}

	protected static bool IsBlocked(vector start, vector end, float height)
	{
		vector up = Vector(0, height, 0);
		vector contact_pos, contact_dir;
		int contact_component;
		set<Object> results = new set<Object>();
		return DayZPhysics.RaycastRV(start + up, end + up, contact_pos, contact_dir, contact_component, results, null, GetGame().GetPlayer(), false, false, ObjIntersectGeom, 0);
	}
}
