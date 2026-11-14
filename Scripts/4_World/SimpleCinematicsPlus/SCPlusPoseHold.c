// Pose Hold: keeps a full-body emote (vanilla or SurvivorAnimations) playing while the cinematic
// camera is active.
//
// Why poses dropped to idle: Simple Cinematics disables the player's input controller in camera mode,
// but vanilla EmoteManager.InterruptGestureCheck reads the raw move/stance inputs (UAMoveForward,
// UAMoveBack, UAMoveLeft/Right, UATurnLeft/Right, UAStand, UACrouch, UAProne, UAGetOver) straight from
// the input API. Flying the camera with W/A/S/D presses those same keys, so the emote was ended as if
// the player had walked off.
//
// The check runs on the client only, so the flag here is client-side state set by the module each frame.
class SCPlusPoseHold
{
	// True while the camera is active and the HUD's "Hold Pose" box is ticked
	static bool scp_Active;
	
	static bool IsHolding()
	{
		return scp_Active;
	}
}

modded class EmoteManager
{
	override bool InterruptGestureCheck()
	{
		if (SCPlusPoseHold.IsHolding()) {
			return false;
		}
		
		return super.InterruptGestureCheck();
	}
}
