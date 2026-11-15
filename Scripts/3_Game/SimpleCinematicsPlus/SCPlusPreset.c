class SCPlusPresetValue: Managed
{
	string scp_Name;
	float scp_Value;
	
	// No constructor parameters: the JSON reader has to be able to create these on its own
	static SCPlusPresetValue Create(string name, float value)
	{
		SCPlusPresetValue preset_value = new SCPlusPresetValue();
		preset_value.scp_Name = name;
		preset_value.scp_Value = value;
		return preset_value;
	}
}

// A named "look": lens, filter, blur and colour values captured from the camera.
// Saved as JSON in $profile:SimpleCinematics/Presets/<name>.json
class SCPlusPreset: Managed
{
	static const string DIRECTORY = "$profile:SimpleCinematics\\Presets";
	
	// Presets whose name starts with this are internal (e.g. the auto-saved last look) and hidden from the list
	static const string INTERNAL_PREFIX = "_";
	static const string LAST_LOOK = "_LastLook";
	// Built-in preset (iLL's usual Camera Options, 2026-11-09). Always listed; saving a preset with this
	// name replaces it with your own file.
	static const string BUILTIN_DEFAULT = "Default";
	
	// Only the Camera Options; filters, blur and colour are left as they are
	static SCPlusPreset BuiltInDefault()
	{
		SCPlusPreset preset = new SCPlusPreset();
		preset.scp_Name = BUILTIN_DEFAULT;
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraSpeed", 2.8));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraMouseSensitivity", 0.8));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraMovementSmoothing", 0.929));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraMouseSmoothing", 0.974));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraFOV", 53.95));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraNearPlane", 0.08));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraDOFBlur", 0.09));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraDOFDistance", 2.098));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraViewDistance", 3000));
		preset.scp_Values.Insert(SCPlusPresetValue.Create("CameraObjectViewDistance", 1710.83));
		return preset;
	}
	
	// Camera fields that make up a look.
	// A function rather than a static variable, so it is never picked up as a saved/serialized field.
	static array<string> GetFields()
	{
		array<string> fields = {
			"CameraSpeed",
			"CameraMouseSensitivity",
			"CameraMovementSmoothing",
			"CameraMouseSmoothing",
			"CameraViewDistance",
			"CameraObjectViewDistance",
			"CameraFOV",
			"CameraNearPlane",
			"CameraDOFBlur",
			"CameraDOFDistance",
			"CameraVignette",
			"CameraSharpness",
			"CameraExposure",
			"CameraBlur",
			"CameraRadialBlurX",
			"CameraRadialBlurY",
			"CameraRadialBlurOffsetX",
			"CameraRadialBlurOffsetY",
			"CameraColorizeRed",
			"CameraColorizeGreen",
			"CameraColorizeBlue"
		};
		
		return fields;
	}
	
	string scp_Name;
	ref array<ref SCPlusPresetValue> scp_Values = {};
	
	static SCPlusPreset Capture(string name, notnull Class source)
	{
		SCPlusPreset preset = new SCPlusPreset();
		preset.scp_Name = name;
		array<string> fields = GetFields();
		foreach (string field: fields) {
			// GetClassVar's return value isn't checked: it doesn't appear to be a success flag, and if it
			// isn't, checking it left every captured preset (and the Reset Look defaults) empty
			float value = 0;
			EnScript.GetClassVar(source, field, 0, value);
			preset.scp_Values.Insert(SCPlusPresetValue.Create(field, value));
		}
		
		return preset;
	}
	
	// Writes values onto target and calls target.PropertyChanged for each so effects are applied.
	// Returns how many values were applied.
	int ApplyTo(notnull Class target)
	{
		int applied;
		array<string> fields = GetFields();
		foreach (SCPlusPresetValue preset_value: scp_Values) {
			// Only apply known fields, so an edited JSON file can't poke arbitrary variables
			if (fields.Find(preset_value.scp_Name) == -1) {
				continue;
			}
			
			float value = preset_value.scp_Value;
			EnScript.SetClassVar(target, preset_value.scp_Name, 0, value);
			g_Script.Call(target, "PropertyChanged", preset_value.scp_Name);
			applied++;
		}
		
		return applied;
	}
	
	bool IsInternal()
	{
		return scp_Name.IndexOf(INTERNAL_PREFIX) == 0;
	}
	
	// --- File handling ---
	
	static string SanitizeName(string name)
	{
		string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";
		string result;
		for (int i = 0; i < name.Length(); i++) {
			string character = name[i];
			if (allowed.Contains(character)) {
				result += character;
			}
		}
		
		result = result.Trim();
		if (result.Length() > 48) {
			result = result.Substring(0, 48);
		}
		
		return result;
	}
	
	static string GetFilePath(string name)
	{
		return string.Format("%1\\%2.json", DIRECTORY, SanitizeName(name));
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
		scp_Name = SanitizeName(scp_Name);
		if (scp_Name == string.Empty) {
			return false;
		}
		
		EnsureDirectory();
		
		string json;
		JsonSerializer serializer = new JsonSerializer();
		// Serialize through a typed variable: WriteToString(this, ...) uses the parameter's static type and writes "{}"
		SCPlusPreset data = this;
		if (!serializer.WriteToString(data, true, json)) {
			ErrorEx(string.Format("[SimpleCinematics] Failed to serialize preset %1", scp_Name));
			return false;
		}
		
		// Keep the version being replaced (internal "_" presets such as _LastLook are skipped)
		SCPlusBackups.Keep("Presets", GetFilePath(scp_Name), scp_Name);
		
		FileHandle handle = OpenFile(GetFilePath(scp_Name), FileMode.WRITE);
		if (!handle) {
			ErrorEx(string.Format("[SimpleCinematics] Could not open %1 for writing", GetFilePath(scp_Name)));
			return false;
		}
		
		FPrint(handle, json);
		CloseFile(handle);
		return true;
	}
	
	static SCPlusPreset Load(string name)
	{
		string file_path = GetFilePath(name);
		if (!File.Exists(file_path)) {
			if (name == BUILTIN_DEFAULT) {
				return BuiltInDefault();
			}
			
			return null;
		}
		
		SCPlusPreset preset = new SCPlusPreset();
		string json_error;
		JsonSerializer serializer = new JsonSerializer();
		if (!serializer.ReadFromString(preset, File.ReadAllText(file_path), json_error)) {
			ErrorEx(string.Format("[SimpleCinematics] Failed to read preset %1: %2", file_path, json_error));
			return null;
		}
		
		if (!preset.scp_Values) {
			preset.scp_Values = new array<ref SCPlusPresetValue>();
		}
		
		// PrintFormat("[SimpleCinematicsPlus] Loaded preset %1 with %2 values", name, preset.scp_Values.Count());
		
		return preset;
	}
	
	static bool Delete(string name)
	{
		string file_path = GetFilePath(name);
		if (!File.Exists(file_path)) {
			return false;
		}
		
		SCPlusBackups.Keep("Presets", file_path, SanitizeName(name));
		return DeleteFile(file_path);
	}
	
	// Names of user-visible presets, sorted
	static array<string> List()
	{
		array<string> names = {};
		if (FileExist(DIRECTORY)) {
			array<string> files = Directory.EnumerateFiles(DIRECTORY, "*.json");
			foreach (string file: files) {
				string name = File.GetNameWithoutExtension(file);
				if (name.IndexOf(INTERNAL_PREFIX) == 0) {
					continue;
				}
				
				names.Insert(name);
			}
		}
		
		names.Sort();
		// The built-in Default is always first
		int existing = names.Find(BUILTIN_DEFAULT);
		if (existing != -1) {
			names.RemoveOrdered(existing);
		}
		
		names.InsertAt(BUILTIN_DEFAULT, 0);
		return names;
	}
}
