// After the cinematic camera closes, the player's view pitch and the weapon's real aim pitch can disagree,
// so the crosshair/view no longer matches where the rifle points (aiming down sights realigns it, because
// the ironsight camera snaps to the weapon). This eases the view back onto the weapon's aim once, then
// switches itself off until the next camera session, so normal gameplay is untouched.
// Approach adapted from the CASF camera fix iLL shared (camera pitch smoothed toward GetBaseAimingAngleUD).

class SCPlusAimSync
{
	static const float DEADZONE = 1.0; // degrees: closer than this counts as synced
	static const float SMOOTH = 0.25;
	static const float MAX_RATE = 45.0; // degrees per second

	// Set when the SCP camera closes; cleared once view and aim agree
	static bool scp_Pending;
}

modded class DayZPlayerCameraBase
{
	protected float m_SCPlusSyncVelocity[1];

	override float UpdateUDAngleUnlocked(out float pAngle, out float pAngleAdd, float pMin, float pMax, float pDt)
	{
		float result = super.UpdateUDAngleUnlocked(pAngle, pAngleAdd, pMin, pMax, pDt);
		if (!SCPlusAimSync.scp_Pending || !SCPlusSyncAllowed()) {
			m_SCPlusSyncVelocity[0] = 0;
			return result;
		}

		float aim_pitch = m_CommandWeapons.GetBaseAimingAngleUD();
		if (Math.AbsFloat(aim_pitch - pAngle) <= SCPlusAimSync.DEADZONE) {
			m_SCPlusSyncVelocity[0] = 0;
			SCPlusAimSync.scp_Pending = false;
			return result;
		}

		float down_limit, up_limit, left_limit, right_limit;
		m_pPlayer.GetLookLimits(down_limit, up_limit, left_limit, right_limit);
		pMin = Math.Max(pMin, down_limit);
		pMax = Math.Min(pMax, up_limit);

		pAngle = Math.SmoothCD(pAngle, aim_pitch, m_SCPlusSyncVelocity, SCPlusAimSync.SMOOTH, SCPlusAimSync.MAX_RATE, pDt);
		pAngle = Limit(pAngle, pMin, pMax);
		pAngleAdd = Limit(pAngle + pAngleAdd, pMin, pMax) - pAngle;
		return pAngle + pAngleAdd;
	}

	protected bool SCPlusSyncAllowed()
	{
		if (!m_pInput || !m_pPlayer || !m_CommandWeapons) {
			return false;
		}

		if (m_pInput.CameraIsFreeLook() || m_pInput.CameraIsTracking()) {
			return false;
		}

		DayZPlayerImplement player = DayZPlayerImplement.Cast(m_pPlayer);
		if (!player || player.GetInstanceType() != DayZPlayerInstanceType.INSTANCETYPE_CLIENT) {
			return false;
		}

		if (!player.IsAlive() || player.IsUnconscious() || !player.IsRaised()) {
			return false;
		}

		HumanItemAccessor item_accessor = player.GetItemAccessor();
		return item_accessor && item_accessor.IsItemInHandsWeapon();
	}
}
