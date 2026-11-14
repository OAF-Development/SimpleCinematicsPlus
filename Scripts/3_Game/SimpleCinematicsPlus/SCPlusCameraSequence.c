// Camera sequencing, modelled on Blender / Unreal / Maya:
//  - a sequence has a timeline (seconds) and keys placed at points on it
//  - each key stores every camera channel (position, rotation, FOV, DOF), like "key all"
//  - the curve between two keys uses the interpolation of the key on the left
//    (Bezier with auto-clamped handles, Linear, or Constant / stepped)

enum SCPlusInterpolation
{
	BEZIER,   // smooth, auto-clamped handles (Blender's default): no overshoot, eases into extremes and ends
	LINEAR,
	CONSTANT, // hold the value until the next key
	EASE_IN,  // starts slow, arrives at full speed
	EASE_OUT, // leaves at full speed, slows into the next key
	EASE_IN_OUT // slow at both ends (stops at each key)
}

class SCPlusKeyframe: Managed
{
	float scp_KeyTime; // seconds on the timeline
	vector scp_Position;
	vector scp_Orientation; // yaw, pitch, roll in degrees, as returned by GetOrientation()
	float scp_FOV = 70;
	float scp_DOFDistance;
	float scp_DOFBlur;
	int scp_Interpolation = SCPlusInterpolation.BEZIER;
	
	// Graph editor handles. A channel whose bit is set in scp_ManualMask uses the slopes stored here
	// (value per second) instead of the automatic, clamped ones. Channel order: see GetChannel.
	int scp_ManualMask;
	ref array<float> scp_InTangents = {};
	ref array<float> scp_OutTangents = {};
	
	static const int CHANNEL_COUNT = 9;
	
	// 0-2 position x/y/z, 3-5 rotation yaw/pitch/roll, 6 FOV, 7 DOF distance, 8 DOF blur
	float GetChannel(int channel)
	{
		if (channel < 3) {
			return scp_Position[channel];
		}
		
		if (channel < 6) {
			return scp_Orientation[channel - 3];
		}
		
		switch (channel) {
			case 6: return scp_FOV;
			case 7: return scp_DOFDistance;
		}
		
		return scp_DOFBlur;
	}
	
	void SetChannel(int channel, float value)
	{
		if (channel < 3) {
			scp_Position[channel] = value;
			return;
		}
		
		if (channel < 6) {
			scp_Orientation[channel - 3] = value;
			return;
		}
		
		switch (channel) {
			case 6: {
				scp_FOV = Math.Clamp(value, 1, 179);
				return;
			}
			
			case 7: {
				scp_DOFDistance = Math.Max(value, 0);
				return;
			}
		}
		
		scp_DOFBlur = Math.Max(value, 0);
	}
	
	static string ChannelName(int channel)
	{
		switch (channel) {
			case 0: return "Position X";
			case 1: return "Height";
			case 2: return "Position Z";
			case 3: return "Yaw";
			case 4: return "Pitch";
			case 5: return "Roll";
			case 6: return "FOV";
			case 7: return "DOF Distance";
		}
		
		return "DOF Blur";
	}
	
	bool HasManualTangent(int channel)
	{
		return (scp_ManualMask & (1 << channel)) != 0 && scp_InTangents && scp_OutTangents && scp_InTangents.Count() == CHANNEL_COUNT && scp_OutTangents.Count() == CHANNEL_COUNT;
	}
	
	void SetManualTangent(int channel, float in_slope, float out_slope)
	{
		if (!scp_InTangents) {
			scp_InTangents = new array<float>();
		}
		
		if (!scp_OutTangents) {
			scp_OutTangents = new array<float>();
		}
		
		while (scp_InTangents.Count() < CHANNEL_COUNT) {
			scp_InTangents.Insert(0);
		}
		
		while (scp_OutTangents.Count() < CHANNEL_COUNT) {
			scp_OutTangents.Insert(0);
		}
		
		scp_InTangents[channel] = in_slope;
		scp_OutTangents[channel] = out_slope;
		scp_ManualMask = scp_ManualMask | (1 << channel);
	}
	
	void ClearManualTangent(int channel)
	{
		if ((scp_ManualMask & (1 << channel)) != 0) {
			scp_ManualMask = scp_ManualMask - (1 << channel);
		}
	}
	
	static SCPlusKeyframe Create(float time, vector position, vector orientation, float fov, float dof_distance, float dof_blur, int interpolation = SCPlusInterpolation.BEZIER)
	{
		SCPlusKeyframe keyframe = new SCPlusKeyframe();
		keyframe.scp_KeyTime = time;
		keyframe.scp_Position = position;
		keyframe.scp_Orientation = orientation;
		keyframe.scp_FOV = fov;
		keyframe.scp_DOFDistance = dof_distance;
		keyframe.scp_DOFBlur = dof_blur;
		keyframe.scp_Interpolation = interpolation;
		return keyframe;
	}
	
	static const int INTERPOLATION_COUNT = 6;
	
	static string InterpolationName(int interpolation)
	{
		switch (interpolation) {
			case SCPlusInterpolation.LINEAR: return "Linear";
			case SCPlusInterpolation.CONSTANT: return "Constant";
			case SCPlusInterpolation.EASE_IN: return "Ease In";
			case SCPlusInterpolation.EASE_OUT: return "Ease Out";
			case SCPlusInterpolation.EASE_IN_OUT: return "Ease In/Out";
		}
		
		return "Bezier";
	}
	
	SCPlusKeyframe Clone()
	{
		SCPlusKeyframe copy = Create(scp_KeyTime, scp_Position, scp_Orientation, scp_FOV, scp_DOFDistance, scp_DOFBlur, scp_Interpolation);
		copy.scp_ManualMask = scp_ManualMask;
		if (scp_InTangents) {
			copy.scp_InTangents.Copy(scp_InTangents);
		}
		
		if (scp_OutTangents) {
			copy.scp_OutTangents.Copy(scp_OutTangents);
		}
		
		return copy;
	}
}

// A named timeline of keys. Saved as JSON in $profile:SimpleCinematics/Sequences/<name>.json
// Event track: actions fired at an exact time during playback (Unreal Sequencer style event keys).
// The actions themselves are run by SCPlusEventRunner (4_World).
enum SCPlusEventType
{
	FIRE, // fire the weapon in hand; scp_Count shots, scp_Interval seconds apart (raises it first if needed)
	RAISE, // raise the weapon and keep it raised
	LOWER, // lower the weapon
	EMOTE, // play emote scp_EmoteId (vanilla or SurvivorAnimations)
	STOP_EMOTE, // end the current emote
	MOVE, // walk/jog/sprint (scp_Speed 1/2/3) for scp_Duration seconds at scp_Angle degrees from the body's facing
	STANCE, // change stance to scp_Stance (SCPlusStance)
	PATH // walk/jog/sprint (scp_Speed) along the sequence's waypoints
}

enum SCPlusStance
{
	ERECT,
	CROUCH,
	PRONE
}

class SCPlusEvent: Managed
{
	static const int TYPE_COUNT = 8;
	
	float scp_Time; // seconds on the timeline
	int scp_Type = SCPlusEventType.FIRE;
	int scp_Count = 1;
	float scp_Interval = 0.12;
	int scp_EmoteId;
	int scp_Speed = 1; // MOVE: 1 walk, 2 jog, 3 sprint
	float scp_Duration = 3; // MOVE: seconds
	float scp_Angle; // MOVE: degrees relative to the body's facing (0 forward)
	int scp_Stance = SCPlusStance.ERECT; // STANCE
	
	// Timed events run for scp_Duration seconds and are drawn as bars on the timeline
	bool IsTimed()
	{
		return scp_Type == SCPlusEventType.MOVE;
	}
	
	float GetEnd()
	{
		if (IsTimed()) {
			return scp_Time + scp_Duration;
		}
		
		return scp_Time;
	}
	
	SCPlusEvent Clone()
	{
		SCPlusEvent copy = new SCPlusEvent();
		copy.scp_Time = scp_Time;
		copy.scp_Type = scp_Type;
		copy.scp_Count = scp_Count;
		copy.scp_Interval = scp_Interval;
		copy.scp_EmoteId = scp_EmoteId;
		copy.scp_Speed = scp_Speed;
		copy.scp_Duration = scp_Duration;
		copy.scp_Angle = scp_Angle;
		copy.scp_Stance = scp_Stance;
		return copy;
	}
	
	string GetCaption()
	{
		switch (scp_Type) {
			case SCPlusEventType.FIRE: {
				if (scp_Count == 1) {
					return "Fire 1 shot";
				}
				
				return string.Format("Fire %1 shots", scp_Count);
			}
			
			case SCPlusEventType.RAISE: return "Raise weapon";
			case SCPlusEventType.LOWER: return "Lower weapon";
			case SCPlusEventType.EMOTE: return string.Format("Emote %1", scp_EmoteId);
			case SCPlusEventType.STOP_EMOTE: return "Stop emote";
			case SCPlusEventType.MOVE: {
				string gait = "Walk";
				if (scp_Speed == 2) {
					gait = "Jog";
				} else if (scp_Speed >= 3) {
					gait = "Sprint";
				}
				
				string move_caption = string.Format("%1 %2 s", gait, scp_Duration.ToString());
				if (Math.AbsFloat(scp_Angle) > 0.5) {
					move_caption += string.Format(" at %1 deg", Math.Round(scp_Angle));
				}
				
				return move_caption;
			}
			
			case SCPlusEventType.PATH: {
				if (scp_Speed == 2) {
					return "Jog path";
				}
				
				if (scp_Speed >= 3) {
					return "Sprint path";
				}
				
				return "Walk path";
			}
			
			case SCPlusEventType.STANCE: {
				if (scp_Stance == SCPlusStance.CROUCH) {
					return "Crouch";
				}
				
				if (scp_Stance == SCPlusStance.PRONE) {
					return "Prone";
				}
				
				return "Stand";
			}
		}
		
		return "Event";
	}
}

class SCPlusCameraSequence: Managed
{
	static const string DIRECTORY = "$profile:SimpleCinematics\\Sequences";
	
	// Keys closer together than this are treated as the same frame (one frame at 60 fps)
	static const float KEY_TOLERANCE = 0.0166;
	
	string scp_Name;
	string scp_MapName; // map the sequence was recorded on
	float scp_Length = 10; // timeline length in seconds
	bool scp_Loop;
	// Where the player (actor) stands when the scene starts; playback puts them back there on Stop and on each loop
	bool scp_HasActorStart;
	// During playback the camera moves with the actor: keyed positions are offset by how far the actor
	// has moved from the actor start (keys are made with the actor standing at the start)
	bool scp_FollowActor;
	// Scene target (an animal, vehicle or another player picked with Follow / Focus): where it stood when the
	// scene started, so Play from the start, Stop and each loop put it back there too. Only the place is saved;
	// the object itself is picked again after a restart (see SimpleCinematicsCamera.ResetSceneTarget).
	bool scp_HasTargetStart;
	vector scp_TargetStartPos;
	vector scp_TargetStartOri;
	string scp_TargetType;
	vector scp_ActorStartPos;
	vector scp_ActorStartOri;
	// Waypoint path the actor can follow (PATH event); world positions in placement order
	ref array<vector> scp_Waypoints = new array<vector>();
	// Corner rounding per waypoint, 0 (sharp) to 1 (fully rounded); missing entries use the default
	ref array<float> scp_WaypointCurves = new array<float>();
ref array<ref SCPlusKeyframe> scp_Keys = {}; // always sorted by scp_KeyTime
	ref array<ref SCPlusEvent> scp_Events = {}; // always sorted by scp_Time
	
	int Count()
	{
		return scp_Keys.Count();
	}
	
	// The timeline always covers every key
	float GetLength()
	{
		float length = Math.Max(scp_Length, 0.1);
		if (scp_Keys.Count() > 0) {
			length = Math.Max(length, scp_Keys[scp_Keys.Count() - 1].scp_KeyTime);
		}
		
		if (scp_Events.Count() > 0) {
			length = Math.Max(length, scp_Events[scp_Events.Count() - 1].scp_Time);
		}
		
		return length;
	}
	
	int FindKeyAt(float time)
	{
		foreach (int i, SCPlusKeyframe key: scp_Keys) {
			if (Math.AbsFloat(key.scp_KeyTime - time) <= KEY_TOLERANCE) {
				return i;
			}
		}
		
		return -1;
	}
	
	// Inserts a key, replacing any key already on that frame. Returns its index.
	int SetKey(notnull SCPlusKeyframe key)
	{
		key.scp_KeyTime = Math.Max(key.scp_KeyTime, 0);
		int existing = FindKeyAt(key.scp_KeyTime);
		if (existing != -1) {
			// Keep the interpolation the user chose for that key, like re-keying in Blender
			key.scp_Interpolation = scp_Keys[existing].scp_Interpolation;
			scp_Keys.RemoveOrdered(existing);
		}
		
		for (int i = 0; i < scp_Keys.Count(); i++) {
			if (scp_Keys[i].scp_KeyTime > key.scp_KeyTime) {
				scp_Keys.InsertAt(key, i);
				return i;
			}
		}
		
		return scp_Keys.Insert(key);
	}
	
	// Moves a key to a new time (replacing any key there) and returns its new index
	int MoveKey(int index, float time)
	{
		if (!scp_Keys.IsValidIndex(index)) {
			return -1;
		}
		
		SCPlusKeyframe key = scp_Keys[index];
		scp_Keys.RemoveOrdered(index);
		key.scp_KeyTime = time;
		
		int existing = FindKeyAt(time);
		if (existing != -1) {
			scp_Keys.RemoveOrdered(existing);
		}
		
		for (int i = 0; i < scp_Keys.Count(); i++) {
			if (scp_Keys[i].scp_KeyTime > key.scp_KeyTime) {
				scp_Keys.InsertAt(key, i);
				return i;
			}
		}
		
		return scp_Keys.Insert(key);
	}
	
	// Index of the first key after time, or -1
	int NextKey(float time)
	{
		foreach (int i, SCPlusKeyframe key: scp_Keys) {
			if (key.scp_KeyTime > time + KEY_TOLERANCE) {
				return i;
			}
		}
		
		return -1;
	}
	
	// Index of the last key before time, or -1
	int PreviousKey(float time)
	{
		for (int i = scp_Keys.Count() - 1; i >= 0; i--) {
			if (scp_Keys[i].scp_KeyTime < time - KEY_TOLERANCE) {
				return i;
			}
		}
		
		return -1;
	}
	
	// --- Events ---
	
	// Inserts an event in time order (several events may share a time) and returns its index
	int AddEvent(notnull SCPlusEvent event_key)
	{
		event_key.scp_Time = Math.Max(event_key.scp_Time, 0);
		for (int i = 0; i < scp_Events.Count(); i++) {
			if (scp_Events[i].scp_Time > event_key.scp_Time) {
				scp_Events.InsertAt(event_key, i);
				return i;
			}
		}
		
		return scp_Events.Insert(event_key);
	}
	
	// Moves an event to a new time and returns its new index
	int MoveEvent(int index, float time)
	{
		if (!scp_Events.IsValidIndex(index)) {
			return -1;
		}
		
		SCPlusEvent event_key = scp_Events[index];
		scp_Events.RemoveOrdered(index);
		event_key.scp_Time = time;
		return AddEvent(event_key);
	}
	
	// Index of an event that clashes with the event at index, or -1. Two events clash when one would
	// cut the other short or be skipped by the runner:
	//  - a timed move (MOVE) is still running when a stance change, another move/path or an emote starts
	//  - a stance change lands within RAISE_SETTLE seconds of a raise / lower (the weapon change is still busy)
	static const float RAISE_SETTLE = 1.0;
	
	int FindClash(int index)
	{
		if (!scp_Events.IsValidIndex(index)) {
			return -1;
		}
		
		SCPlusEvent a = scp_Events[index];
		foreach (int other, SCPlusEvent b: scp_Events) {
			if (other == index) {
				continue;
			}
			
			if (EventsClash(a, b) || EventsClash(b, a)) {
				return other;
			}
		}
		
		return -1;
	}
	
	// Does "later" interrupt "first"?
	protected static bool EventsClash(SCPlusEvent first, SCPlusEvent later)
	{
		float gap = later.scp_Time - first.scp_Time;
		if (gap < 0) {
			return false;
		}
		
		if (first.IsTimed() && gap < first.scp_Duration - KEY_TOLERANCE) {
			switch (later.scp_Type) {
				case SCPlusEventType.MOVE:
				case SCPlusEventType.PATH:
				case SCPlusEventType.STANCE:
				case SCPlusEventType.EMOTE:
				case SCPlusEventType.STOP_EMOTE: {
					return true;
				}
			}
		}
		
		if ((first.scp_Type == SCPlusEventType.RAISE || first.scp_Type == SCPlusEventType.LOWER) && later.scp_Type == SCPlusEventType.STANCE) {
			return gap < RAISE_SETTLE;
		}
		
		return false;
	}
	
	// --- File handling ---
	
	static string GetFilePath(string name)
	{
		return string.Format("%1\\%2.json", DIRECTORY, SCPlusPreset.SanitizeName(name));
	}
	
	static void EnsureDirectory()
	{
		if (!FileExist("$profile:SimpleCinematics")) {
			MakeDirectory("$profile:SimpleCinematics");
		}
		
		if (!FileExist(DIRECTORY)) {
			MakeDirectory(DIRECTORY);
		}
	}
	
	bool Save()
	{
		scp_Name = SCPlusPreset.SanitizeName(scp_Name);
		if (scp_Name == string.Empty) {
			return false;
		}
		
		EnsureDirectory();
		
		string json;
		JsonSerializer serializer = new JsonSerializer();
		// Serialize through a typed variable: WriteToString(this, ...) uses the parameter's static type and writes "{}"
		SCPlusCameraSequence data = this;
		if (!serializer.WriteToString(data, true, json)) {
			ErrorEx(string.Format("[SimpleCinematicsPlus] Failed to serialize sequence %1", scp_Name));
			return false;
		}
		
		// Keep the version being replaced (Backups\Sequences\<name>.1.json is the newest)
		SCPlusBackups.Keep("Sequences", GetFilePath(scp_Name), scp_Name);
		
		FileHandle handle = OpenFile(GetFilePath(scp_Name), FileMode.WRITE);
		if (!handle) {
			ErrorEx(string.Format("[SimpleCinematicsPlus] Could not open %1 for writing", GetFilePath(scp_Name)));
			return false;
		}
		
		FPrint(handle, json);
		CloseFile(handle);
		return true;
	}
	
	static SCPlusCameraSequence Load(string name)
	{
		string file_path = GetFilePath(name);
		if (!File.Exists(file_path)) {
			return null;
		}
		
		SCPlusCameraSequence sequence = new SCPlusCameraSequence();
		string json_error;
		JsonSerializer serializer = new JsonSerializer();
		if (!serializer.ReadFromString(sequence, File.ReadAllText(file_path), json_error)) {
			ErrorEx(string.Format("[SimpleCinematicsPlus] Failed to read sequence %1: %2", file_path, json_error));
			return null;
		}
		
		if (!sequence.scp_Events) {
			sequence.scp_Events = new array<ref SCPlusEvent>();
		}
		
		// Sort events too (older sequence files have none)
		array<ref SCPlusEvent> loaded_events = sequence.scp_Events;
		sequence.scp_Events = new array<ref SCPlusEvent>();
		foreach (SCPlusEvent loaded_event: loaded_events) {
			sequence.AddEvent(loaded_event);
		}
		
		if (!sequence.scp_Keys) {
			sequence.scp_Keys = new array<ref SCPlusKeyframe>();
		}
		
		// Re-insert so a hand-edited file still ends up sorted
		array<ref SCPlusKeyframe> loaded_keys = sequence.scp_Keys;
		sequence.scp_Keys = new array<ref SCPlusKeyframe>();
		foreach (SCPlusKeyframe key: loaded_keys) {
			sequence.SetKey(key);
		}
		
		return sequence;
	}
	
	static bool Delete(string name)
	{
		string file_path = GetFilePath(name);
		if (!File.Exists(file_path)) {
			return false;
		}
		
		// A deleted sequence can be recovered from Backups\Sequences\<name>.1.json
		SCPlusBackups.Keep("Sequences", file_path, SCPlusPreset.SanitizeName(name));
		return DeleteFile(file_path);
	}
	
	static array<string> List()
	{
		array<string> names = {};
		if (!FileExist(DIRECTORY)) {
			return names;
		}
		
		array<string> files = Directory.EnumerateFiles(DIRECTORY, "*.json");
		foreach (string file: files) {
			names.Insert(File.GetNameWithoutExtension(file));
		}
		
		names.Sort();
		return names;
	}
}

// Evaluates a sequence at any time. Each of the 9 channels (position xyz, rotation xyz, FOV, DOF distance,
// DOF blur) is its own curve, like F-curves in Blender. Rotation is unwrapped so the camera always takes the
// short way round between keys.
class SCPlusSequenceEvaluator: Managed
{
	static const int CHANNEL_COUNT = 9;
	
	protected ref array<float> m_Times = {};
	protected ref array<int> m_Interpolation = {};
	// m_Values[channel * key_count + key], m_Tangents the same layout (value per second)
	protected ref array<float> m_Values = {};
	protected ref array<float> m_Tangents = {}; // incoming slope at each key
	protected ref array<float> m_OutTangents = {}; // outgoing slope at each key
	protected int m_KeyCount;
	
	void SCPlusSequenceEvaluator(notnull SCPlusCameraSequence sequence)
	{
		m_KeyCount = sequence.Count();
		if (m_KeyCount == 0) {
			return;
		}
		
		m_Values.Resize(CHANNEL_COUNT * m_KeyCount);
		m_Tangents.Resize(CHANNEL_COUNT * m_KeyCount);
		m_OutTangents.Resize(CHANNEL_COUNT * m_KeyCount);
		
		vector previous_orientation;
		for (int k = 0; k < m_KeyCount; k++) {
			SCPlusKeyframe key = sequence.scp_Keys[k];
			m_Times.Insert(key.scp_KeyTime);
			m_Interpolation.Insert(key.scp_Interpolation);
			
			vector orientation = key.scp_Orientation;
			if (k > 0) {
				orientation[0] = UnwrapAngle(orientation[0], previous_orientation[0]);
				orientation[2] = UnwrapAngle(orientation[2], previous_orientation[2]);
			}
			
			previous_orientation = orientation;
			
			m_Values[0 * m_KeyCount + k] = key.scp_Position[0];
			m_Values[1 * m_KeyCount + k] = key.scp_Position[1];
			m_Values[2 * m_KeyCount + k] = key.scp_Position[2];
			m_Values[3 * m_KeyCount + k] = orientation[0];
			m_Values[4 * m_KeyCount + k] = orientation[1];
			m_Values[5 * m_KeyCount + k] = orientation[2];
			m_Values[6 * m_KeyCount + k] = key.scp_FOV;
			m_Values[7 * m_KeyCount + k] = key.scp_DOFDistance;
			m_Values[8 * m_KeyCount + k] = key.scp_DOFBlur;
		}
		
		for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
			for (int i = 0; i < m_KeyCount; i++) {
				SCPlusKeyframe tangent_key = sequence.scp_Keys[i];
				if (tangent_key.HasManualTangent(channel)) {
					m_Tangents[channel * m_KeyCount + i] = tangent_key.scp_InTangents[channel];
					m_OutTangents[channel * m_KeyCount + i] = tangent_key.scp_OutTangents[channel];
				} else {
					float auto_tangent = AutoClampedTangent(channel, i);
					m_Tangents[channel * m_KeyCount + i] = auto_tangent;
					m_OutTangents[channel * m_KeyCount + i] = auto_tangent;
				}
			}
		}
	}
	
	int GetKeyCount()
	{
		return m_KeyCount;
	}
	
	float GetKeyTime(int key)
	{
		return m_Times[key];
	}
	
	// Key value as the curves see it (rotation unwrapped)
	float GetKeyValue(int channel, int key)
	{
		return m_Values[channel * m_KeyCount + key];
	}
	
	float GetInTangent(int channel, int key)
	{
		return m_Tangents[channel * m_KeyCount + key];
	}
	
	float GetOutTangent(int channel, int key)
	{
		return m_OutTangents[channel * m_KeyCount + key];
	}
	
	// One channel of the curve at a time (before the first key / after the last key it holds)
	float EvaluateChannelAt(int channel, float time)
	{
		if (m_KeyCount == 0) {
			return 0;
		}
		
		int segment = 0;
		if (time <= m_Times[0]) {
			time = m_Times[0];
		} else {
			while (segment < m_KeyCount - 1 && time >= m_Times[segment + 1]) {
				segment++;
			}
		}
		
		return EvaluateChannel(channel, segment, time);
	}
	
	// Blender-style auto-clamped handle: flat at the first and last key and at any local peak or dip
	// (so the curve never overshoots a key), otherwise the slope between the neighbouring keys.
	protected float AutoClampedTangent(int channel, int i)
	{
		if (i == 0 || i == m_KeyCount - 1) {
			return 0;
		}
		
		float previous = m_Values[channel * m_KeyCount + i - 1];
		float current = m_Values[channel * m_KeyCount + i];
		float next = m_Values[channel * m_KeyCount + i + 1];
		if ((current - previous) * (next - current) <= 0) {
			return 0;
		}
		
		float span = m_Times[i + 1] - m_Times[i - 1];
		if (span <= 0) {
			return 0;
		}
		
		return (next - previous) / span;
	}
	
	protected float EvaluateChannel(int channel, int segment, float time)
	{
		int channel_offset = channel * m_KeyCount;
		float v0 = m_Values[channel_offset + segment];
		if (segment >= m_KeyCount - 1) {
			return v0;
		}
		
		float v1 = m_Values[channel_offset + segment + 1];
		float t0 = m_Times[segment];
		float span = m_Times[segment + 1] - t0;
		if (span <= 0) {
			return v1;
		}
		
		float s = Math.Clamp((time - t0) / span, 0, 1);
		switch (m_Interpolation[segment]) {
			case SCPlusInterpolation.CONSTANT: {
				return v0;
			}
			
			case SCPlusInterpolation.LINEAR: {
				return Math.Lerp(v0, v1, s);
			}
			
			case SCPlusInterpolation.EASE_IN: {
				return Math.Lerp(v0, v1, s * s);
			}
			
			case SCPlusInterpolation.EASE_OUT: {
				return Math.Lerp(v0, v1, 1 - (1 - s) * (1 - s));
			}
			
			case SCPlusInterpolation.EASE_IN_OUT: {
				return Math.Lerp(v0, v1, s * s * (3 - 2 * s));
			}
		}
		
		// Cubic Hermite (equivalent to a Bezier with handles at 1/3 of the segment)
		float m0 = m_OutTangents[channel_offset + segment] * span;
		float m1 = m_Tangents[channel_offset + segment + 1] * span;
		float s2 = s * s;
		float s3 = s2 * s;
		return (2 * s3 - 3 * s2 + 1) * v0 + (s3 - 2 * s2 + s) * m0 + (3 * s2 - 2 * s3) * v1 + (s3 - s2) * m1;
	}
	
	// Before the first key the camera holds the first key, after the last key it holds the last key
	void Evaluate(float time, out vector position, out vector orientation, out float fov, out float dof_distance, out float dof_blur)
	{
		if (m_KeyCount == 0) {
			return;
		}
		
		int segment = 0;
		if (time <= m_Times[0]) {
			time = m_Times[0];
		} else {
			while (segment < m_KeyCount - 1 && time >= m_Times[segment + 1]) {
				segment++;
			}
		}
		
		position = Vector(EvaluateChannel(0, segment, time), EvaluateChannel(1, segment, time), EvaluateChannel(2, segment, time));
		orientation = Vector(EvaluateChannel(3, segment, time), EvaluateChannel(4, segment, time), EvaluateChannel(5, segment, time));
		orientation[1] = Math.Clamp(orientation[1], -89, 89);
		fov = Math.Clamp(EvaluateChannel(6, segment, time), 1, 179);
		dof_distance = Math.Max(EvaluateChannel(7, segment, time), 0);
		dof_blur = Math.Max(EvaluateChannel(8, segment, time), 0);
	}
	
	// Shifts angle by whole turns so it is within 180 degrees of reference_angle
	static float UnwrapAngle(float angle, float reference_angle)
	{
		while (angle - reference_angle > 180) {
			angle -= 360;
		}
		
		while (angle - reference_angle < -180) {
			angle += 360;
		}
		
		return angle;
	}
}

// One undo step: the keys and events as they were before an edit (see the sequencer's PushUndo)
class SCPlusUndoState: Managed
{
	ref array<ref SCPlusKeyframe> scp_Keys = new array<ref SCPlusKeyframe>();
	ref array<ref SCPlusEvent> scp_Events = new array<ref SCPlusEvent>();
	
	static SCPlusUndoState Capture(notnull SCPlusCameraSequence sequence)
	{
		SCPlusUndoState state = new SCPlusUndoState();
		foreach (SCPlusKeyframe key: sequence.scp_Keys) {
			state.scp_Keys.Insert(key.Clone());
		}
		
		foreach (SCPlusEvent event_key: sequence.scp_Events) {
			state.scp_Events.Insert(event_key.Clone());
		}
		
		return state;
	}
	
	void Apply(notnull SCPlusCameraSequence sequence)
	{
		sequence.scp_Keys.Clear();
		foreach (SCPlusKeyframe key: scp_Keys) {
			sequence.scp_Keys.Insert(key.Clone());
		}
		
		sequence.scp_Events.Clear();
		foreach (SCPlusEvent event_key: scp_Events) {
			sequence.scp_Events.Insert(event_key.Clone());
		}
	}
}

// Auto-Key: turns a dense recording of the camera into the fewest keys that still follow it within a small
// tolerance, like Blender's Bake Action followed by Decimate. The result is ordinary, editable keys.
class SCPlusBake
{
	static const float SAMPLE_STEP = 0.1; // seconds between samples (longer bakes use a coarser step, see GetStep)
	static const int MAX_SAMPLES = 600;
	static const float POSITION_TOLERANCE = 0.04; // metres
	static const float ANGLE_TOLERANCE = 0.6; // degrees
	static const float FOV_TOLERANCE = 0.3;
	static const float DOF_DISTANCE_TOLERANCE = 0.25;
	static const float DOF_BLUR_TOLERANCE = 0.05;
	static const float MAX_TURN = 90; // degrees one segment may turn, so the curves never take the long way round
	static const float MAX_GAP = 4; // seconds between keys at most
	
	static float GetStep(float length)
	{
		return Math.Max(SAMPLE_STEP, length / MAX_SAMPLES);
	}
	
	// Angles continuous from sample to sample (350 then 370, not 350 then 10)
	static void Unwrap(notnull array<ref SCPlusKeyframe> samples)
	{
		for (int i = 1; i < samples.Count(); i++) {
			vector previous = samples[i - 1].scp_Orientation;
			vector current = samples[i].scp_Orientation;
			for (int axis = 0; axis < 3; axis++) {
				float delta = current[axis] - previous[axis];
				while (delta > 180) {
					current[axis] = current[axis] - 360;
					delta = delta - 360;
				}
				
				while (delta < -180) {
					current[axis] = current[axis] + 360;
					delta = delta + 360;
				}
			}
			
			samples[i].scp_Orientation = current;
		}
	}
	
	// How far the sample is from a straight line between a and b, in tolerances (above 1 = too far)
	protected static float Error(SCPlusKeyframe a, SCPlusKeyframe b, SCPlusKeyframe sample)
	{
		float fraction = 0;
		float span = b.scp_KeyTime - a.scp_KeyTime;
		if (span > 0.0001) {
			fraction = (sample.scp_KeyTime - a.scp_KeyTime) / span;
		}
		
		vector position = vector.Lerp(a.scp_Position, b.scp_Position, fraction);
		float error = vector.Distance(position, sample.scp_Position) / POSITION_TOLERANCE;
		
		vector orientation = vector.Lerp(a.scp_Orientation, b.scp_Orientation, fraction);
		for (int axis = 0; axis < 3; axis++) {
			error = Math.Max(error, Math.AbsFloat(orientation[axis] - sample.scp_Orientation[axis]) / ANGLE_TOLERANCE);
		}
		
		error = Math.Max(error, Math.AbsFloat(Math.Lerp(a.scp_FOV, b.scp_FOV, fraction) - sample.scp_FOV) / FOV_TOLERANCE);
		error = Math.Max(error, Math.AbsFloat(Math.Lerp(a.scp_DOFDistance, b.scp_DOFDistance, fraction) - sample.scp_DOFDistance) / DOF_DISTANCE_TOLERANCE);
		error = Math.Max(error, Math.AbsFloat(Math.Lerp(a.scp_DOFBlur, b.scp_DOFBlur, fraction) - sample.scp_DOFBlur) / DOF_BLUR_TOLERANCE);
		return error;
	}
	
	protected static bool NeedsSplit(SCPlusKeyframe a, SCPlusKeyframe b)
	{
		if (b.scp_KeyTime - a.scp_KeyTime > MAX_GAP) {
			return true;
		}
		
		vector turn = b.scp_Orientation - a.scp_Orientation;
		return Math.AbsFloat(turn[0]) > MAX_TURN || Math.AbsFloat(turn[1]) > MAX_TURN || Math.AbsFloat(turn[2]) > MAX_TURN;
	}
	
	protected static float WrapAngle(float angle, float low)
	{
		while (angle >= low + 360) {
			angle = angle - 360;
		}
		
		while (angle < low) {
			angle = angle + 360;
		}
		
		return angle;
	}
	
	// Keeps the first and last sample and every sample the line between its neighbours misses (Ramer-Douglas-Peucker)
	static array<ref SCPlusKeyframe> Decimate(notnull array<ref SCPlusKeyframe> samples)
	{
		array<ref SCPlusKeyframe> result = new array<ref SCPlusKeyframe>();
		int count = samples.Count();
		if (count == 0) {
			return result;
		}
		
		Unwrap(samples);
		
		array<bool> keep = new array<bool>();
		for (int i = 0; i < count; i++) {
			keep.Insert(i == 0 || i == count - 1);
		}
		
		array<int> stack = new array<int>();
		stack.Insert(0);
		stack.Insert(count - 1);
		while (stack.Count() >= 2) {
			int b = stack[stack.Count() - 1];
			int a = stack[stack.Count() - 2];
			stack.RemoveOrdered(stack.Count() - 1);
			stack.RemoveOrdered(stack.Count() - 1);
			if (b - a < 2) {
				continue;
			}
			
			int worst = -1;
			float worst_error = 1;
			for (int j = a + 1; j < b; j++) {
				float error = Error(samples[a], samples[b], samples[j]);
				if (error > worst_error) {
					worst_error = error;
					worst = j;
				}
			}
			
			if (worst == -1 && NeedsSplit(samples[a], samples[b])) {
				worst = (a + b) / 2;
			}
			
			if (worst != -1) {
				keep[worst] = true;
				stack.Insert(a);
				stack.Insert(worst);
				stack.Insert(worst);
				stack.Insert(b);
			}
		}
		
		for (int k = 0; k < count; k++) {
			if (!keep[k]) {
				continue;
			}
			
			SCPlusKeyframe key = samples[k].Clone();
			vector orientation = key.scp_Orientation;
			orientation[0] = WrapAngle(orientation[0], 0);
			orientation[1] = WrapAngle(orientation[1], -180);
			orientation[2] = WrapAngle(orientation[2], -180);
			key.scp_Orientation = orientation;
			key.scp_Interpolation = SCPlusInterpolation.BEZIER;
			result.Insert(key);
		}
		
		return result;
	}
}
