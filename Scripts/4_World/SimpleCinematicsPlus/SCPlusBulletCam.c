// Bullet Cam: when armed, the next shot fired by the local player is followed by the cinematic camera from the
// muzzle to where it lands, in slow motion, then the camera goes back to where it was (or the sequence carries on).
//
// DayZ bullets are simulated by the engine and are not script objects, so the flight is re-simulated here from
// the barrel's memory points, the ammo's config speed and air friction, and gravity, with raycasts to find the hit.
// The path matches the real shot closely at normal ranges; it ignores wind and ricochets.

modded class Weapon_Base
{
	override void EEFired(int muzzleType, int mode, string ammoType)
	{
		super.EEFired(muzzleType, mode, ammoType);
		
		if (!GetGame().IsClient() && GetGame().IsMultiplayer()) {
			return;
		}
		
		if (SCPlusBulletCam.scp_Armed && GetHierarchyRootPlayer() && GetHierarchyRootPlayer() == GetGame().GetPlayer()) {
			SCPlusBulletCam.OnLocalShot(this, ammoType);
		}
	}
}

class SCPlusBulletCam
{
	static const float STEP = 1.0 / 120.0; // simulation step, seconds
	static const float MAX_FLIGHT_TIME = 6.0; // 720 steps
	static const float GRAVITY = 9.81;
	
	// Set every frame by the module from the HUD settings
	static bool scp_Armed;
	static float scp_SpeedFactor = 0.04; // fraction of real time
	static float scp_Distance = 1.0; // metres behind the bullet
	static bool scp_ShowBullet = true;
	static bool scp_ShowPath; // debug: draw the barrel and aim lines for each shot
	protected static ref array<Shape> s_PathShapes;
	// Same lines kept as data, so the HUD overlay can draw them too (debug Shapes may not render in retail)
	static ref array<vector> scp_DebugFrom = new array<vector>();
	static ref array<vector> scp_DebugTo = new array<vector>();
	static ref array<int> scp_DebugColor = new array<int>();
	static float scp_HoldTime = 1.5; // real seconds to hold on the impact
	
	// The shot waiting to be followed (taken by the camera)
	protected static ref array<vector> s_Path;
	protected static string s_AmmoType;
	protected static bool s_Hit;
	
	static bool HasShot()
	{
		return s_Path != null;
	}
	
	// Hands the pending shot to the camera: path points STEP seconds apart, whether it ended in a hit
	static array<vector> TakeShot(out string ammo_type, out bool hit)
	{
		array<vector> path = s_Path;
		ammo_type = s_AmmoType;
		hit = s_Hit;
		s_Path = null;
		return path;
	}
	
	static void OnLocalShot(notnull Weapon_Base weapon, string ammo_type)
	{
		if (s_Path) {
			return; // one bullet at a time
		}
		
		vector muzzle = GetBarrelPoint(weapon, "usti hlavne");
		vector breech = GetBarrelPoint(weapon, "konec hlavne");
		vector direction = vector.Direction(breech, muzzle);
		if (direction.Length() < 0.01) {
			// Weapon without barrel memory points: use its forward axis
			muzzle = weapon.GetPosition();
			direction = weapon.GetDirection();
		}
		
		direction.Normalize();
		vector barrel_direction = direction;
		
		// The barrel memory points aren't reliable on every weapon (mirrored pitch, a few degrees of heading off),
		// which grows into a big miss on long shots. The engine's aim angles (the green debug line) are what the
		// shot actually follows, so the path uses them fully. The barrel is only a fallback without an aim.
		PlayerBase shooter = PlayerBase.Cast(weapon.GetHierarchyRootPlayer());
		if (shooter && shooter.GetCommandModifier_Weapons()) {
			direction = GetAimDirection(shooter);
		}
		
		Print("[SCPlus BulletCam] barrel " + barrel_direction.ToString() + " -> path " + direction.ToString() + " | player dir " + weapon.GetHierarchyRootPlayer().GetDirection().ToString());
		
		if (scp_ShowPath) {
			DrawDebugShot(weapon, muzzle, direction);
		}
		
		float speed = GetGame().ConfigGetFloat("CfgAmmo " + ammo_type + " typicalSpeed");
		if (speed <= 0) {
			speed = 800;
		}
		
		float air_friction = GetGame().ConfigGetFloat("CfgAmmo " + ammo_type + " airFriction");
		
		s_Path = Simulate(muzzle, direction * speed, air_friction, weapon.GetHierarchyRootPlayer(), s_Hit);
		s_AmmoType = ammo_type;
	}
	
	// Where the player is aiming: body heading plus the aim offsets, as one direction
	protected static vector GetAimDirection(notnull PlayerBase player)
	{
		HumanCommandWeapons weapons = player.GetCommandModifier_Weapons();
		float yaw = player.GetOrientation()[0] + weapons.GetBaseAimingAngleLR();
		float pitch = weapons.GetBaseAimingAngleUD();
		return Vector(yaw, pitch, 0).AnglesToVector();
	}
	
	// World position of a barrel memory point; prefers the memory LOD, falls back to the named selection
	protected static vector GetBarrelPoint(Weapon_Base weapon, string point)
	{
		if (weapon.MemoryPointExists(point)) {
			return weapon.ModelToWorld(weapon.GetMemoryPointPos(point));
		}
		
		return weapon.ModelToWorld(weapon.GetSelectionPositionMS(point));
	}
	
	protected static array<vector> Simulate(vector position, vector velocity, float air_friction, Object ignore, out bool hit)
	{
		array<vector> path = {position};
		hit = false;
		int steps = 720; // MAX_FLIGHT_TIME / STEP
		for (int i = 0; i < steps; i++) {
			// Drag along the velocity (airFriction is negative in config) plus gravity
			vector acceleration = velocity * (air_friction * velocity.Length());
			acceleration[1] = acceleration[1] - GRAVITY;
			velocity = velocity + acceleration * STEP;
			vector next = position + velocity * STEP;
			
			vector contact_pos, contact_dir;
			int contact_component;
			set<Object> results = new set<Object>();
			if (DayZPhysics.RaycastRV(position, next, contact_pos, contact_dir, contact_component, results, null, ignore, false, false, ObjIntersectFire, 0)) {
				path.Insert(contact_pos);
				hit = true;
				return path;
			}
			
			path.Insert(next);
			position = next;
			if (position[1] < GetGame().SurfaceY(position[0], position[2]) - 1) {
				break;
			}
		}
		
		return path;
	}
	
	// Local bare-projectile model (SCP's own, no casing), scaled from the calibre in the ammo name; null if off
	static Object CreateBulletModel(string ammo_type, vector position)
	{
		if (!scp_ShowBullet) {
			return null;
		}
		
		Object model = GetGame().CreateObjectEx("SCPlusBulletModel", position, ECE_LOCAL);
		if (model) {
			model.SetScale(GetCalibreMM(ammo_type) / 7.62);
		}
		
		return model;
	}
	
	// Calibre in mm from names like Bullet_762x39, Bullet_9x19, Bullet_308Win, Bullet_45ACP, Bullet_22
	static float GetCalibreMM(string ammo_type)
	{
		string name = ammo_type;
		if (name.IndexOf("Bullet_") == 0) {
			name = name.Substring(7, name.Length() - 7);
		}
		
		string digits;
		for (int i = 0; i < name.Length(); i++) {
			string character = name.Get(i);
			if (character.ToAscii() < 48 || character.ToAscii() > 57) {
				break;
			}
			
			digits += character;
		}
		
		float value = digits.ToFloat();
		if (value <= 0) {
			return 7.62;
		}
		
		bool metric = name.Contains("x");
		if (metric) {
			// 762x39 -> 7.62, 545x39 -> 5.45, 9x19 -> 9, 12x70 (gauge) stays 12
			while (value > 20) {
				value = value / 10;
			}
			
			return value;
		}
		
		// Imperial: 308 -> .308 in, 45 -> .45 in, 22 -> .22 in
		while (value >= 1) {
			value = value / 10;
		}
		
		return Math.Clamp(value * 25.4, 4, 20);
	}
	
	// Debug lines for the last shot (kept until the next shot, or until Show Path is unticked):
	//   red    = the direction Bullet Cam follows (barrel heading, aiming pitch), from the muzzle
	//   green  = the player's aim direction (heading + aiming angles), from the muzzle
	//   yellow = barrel memory points themselves (breech -> muzzle), to check they sit on the barrel
	// Compare with the bullet hole the real shot leaves.
	protected static void DrawDebugShot(Weapon_Base weapon, vector muzzle, vector barrel_direction)
	{
		ClearDebugShot();
		s_PathShapes = new array<Shape>();
		
		vector breech = GetBarrelPoint(weapon, "konec hlavne");
		s_PathShapes.Insert(DrawLine(breech, muzzle, 0xFFFFFF00));
		s_PathShapes.Insert(DrawLine(muzzle, muzzle + barrel_direction * 300, 0xFFFF0000));
		
		PlayerBase player = PlayerBase.Cast(weapon.GetHierarchyRootPlayer());
		if (player && player.GetCommandModifier_Weapons()) {
			HumanCommandWeapons weapons = player.GetCommandModifier_Weapons();
			float pitch = weapons.GetBaseAimingAngleUD();
			vector aim_direction = GetAimDirection(player);
			s_PathShapes.Insert(DrawLine(muzzle, muzzle + aim_direction * 300, 0xFF00FF00));
			Print("[SCPlus BulletCam] barrel dir " + barrel_direction.ToString() + " aim dir " + aim_direction.ToString() + " aimLR " + weapons.GetBaseAimingAngleLR() + " aimUD " + pitch);
		}
	}
	
	protected static Shape DrawLine(vector from, vector to, int color)
	{
		scp_DebugFrom.Insert(from);
		scp_DebugTo.Insert(to);
		scp_DebugColor.Insert(color);
		vector points[2];
		points[0] = from;
		points[1] = to;
		return Shape.CreateLines(color, ShapeFlags.NOZBUFFER, points, 2);
	}
	
	static void ClearDebugShot()
	{
		scp_DebugFrom.Clear();
		scp_DebugTo.Clear();
		scp_DebugColor.Clear();
		if (!s_PathShapes) {
			return;
		}
		
		foreach (Shape shape: s_PathShapes) {
			if (shape) {
				shape.Destroy();
			}
		}
		
		s_PathShapes = null;
	}
}
