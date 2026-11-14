modded class MissionGameplay
{
	// Save camera settings and the last look if the player leaves with the camera still open
	override void OnMissionFinish()
	{
		SimpleCinematicsModule module = SimpleCinematicsModule.Cast(GetModuleManager().GetModule(SimpleCinematicsModule));
		if (module) {
			module.FlushSavedState();
			module.SetConflictingInputsDisabled(false);
		}
		
		super.OnMissionFinish();
	}
}
