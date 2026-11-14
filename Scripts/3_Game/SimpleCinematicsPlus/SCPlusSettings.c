// Per-user camera control settings, saved in the player's DayZ profile.
// Dabs Framework also adds these as a "Simple Cinematics" tab in the vanilla Options menu.
// Field names match the fields on SimpleCinematicsCamera so values can be copied by name.
[RegisterProfileSetting(SCPlusSettings, "Simple Cinematics")]
class SCPlusSettings: ProfileSettings
{
	[RegisterProfileSettingSlider("Movement", "CameraSpeed", "Camera Speed", 0, 200)]
	float CameraSpeed = 100;
	
	[RegisterProfileSettingSlider("Movement", "CameraMouseSensitivity", "Mouse Speed", 0, 100)]
	float CameraMouseSensitivity = 35;
	
	[RegisterProfileSettingSlider("Movement", "CameraMovementSmoothing", "Movement Smoothing", 0, 1)]
	float CameraMovementSmoothing = 0;
	
	[RegisterProfileSettingSlider("Movement", "CameraMouseSmoothing", "Mouse Smoothing", 0, 1)]
	float CameraMouseSmoothing = 0;
	
	[RegisterProfileSettingSlider("View", "CameraViewDistance", "View Distance", 1, 25000)]
	float CameraViewDistance = 3000;
	
	[RegisterProfileSettingSlider("View", "CameraObjectViewDistance", "Object View Distance", 1, 10000)]
	float CameraObjectViewDistance = 700;
	
	// Camera fields that are copied to and from these settings.
	// A function rather than a static variable, so it is never picked up as a saved/serialized field.
	static array<string> GetFields()
	{
		array<string> fields = {
			"CameraSpeed",
			"CameraMouseSensitivity",
			"CameraMovementSmoothing",
			"CameraMouseSmoothing",
			"CameraViewDistance",
			"CameraObjectViewDistance"
		};
		
		return fields;
	}
	
	static SCPlusSettings GetInstance()
	{
		return SCPlusSettings.Cast(GetDayZGame().GetProfileSetting(SCPlusSettings));
	}
	
	// Copies matching float fields from source into these settings
	void CopyFrom(notnull Class source)
	{
		// Hold the array in a local: foreach over a bare function result gets a NULL array
		array<string> fields = GetFields();
		foreach (string field: fields) {
			// GetClassVar's return value isn't used as a success flag (see SCPlusPreset.Capture)
			float value = 0;
			EnScript.GetClassVar(source, field, 0, value);
			EnScript.SetClassVar(this, field, 0, value);
		}
	}
	
	// Copies these settings onto matching float fields of target, notifying it of each change
	void CopyTo(notnull Class target)
	{
		// Hold the array in a local: foreach over a bare function result gets a NULL array
		array<string> fields = GetFields();
		foreach (string field: fields) {
			float value;
			EnScript.GetClassVar(this, field, 0, value);
			EnScript.SetClassVar(target, field, 0, value);
			g_Script.Call(target, "PropertyChanged", field);
		}
	}
}
