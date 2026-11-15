// Actor marker: a small ground cue (disc plus floating diamond) that helpers drop where they should
// walk, jog or sprint to. It is a normal item: anyone holding one drops it (inventory drag or the drop
// key) and picks it up again with F. Spawn it with COT's object spawner (search "SCPlusActorMarker").
//
// Every player sees markers, except the filming player while their camera plays a sequence back:
// on that client only, every marker is made invisible locally, wherever it is (ground, hands, another
// player's inventory), so markers never end up in a played-back recording. While flying or editing the
// camera they stay visible for framing (iLL's choice, 2026-11-12). Nothing is hidden on the server or for
// other players. Five colours (Green, Yellow, Red, Blue, White) give a route order.
class SCPlusActorMarker: ItemBase
{
	override void EEInit()
	{
		super.EEInit();
		// Clients and offline/listen play only; a dedicated server renders nothing
		if (!GetGame().IsDedicatedServer()) {
			SCPlusActorMarkers.Get().Register(this);
		}
	}

	override void EEDelete(EntityAI parent)
	{
		if (!GetGame().IsDedicatedServer()) {
			SCPlusActorMarkers.Get().Unregister(this);
		}
		super.EEDelete(parent);
	}

	override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
	{
		super.EEItemLocationChanged(oldLoc, newLoc);
		if (!GetGame().IsDedicatedServer()) {
			SCPlusActorMarkers.Get().Apply(this);
		}
	}
}

class SCPlusActorMarker_Green: SCPlusActorMarker {}
class SCPlusActorMarker_Yellow: SCPlusActorMarker {}
class SCPlusActorMarker_Red: SCPlusActorMarker {}
class SCPlusActorMarker_Blue: SCPlusActorMarker {}
class SCPlusActorMarker_White: SCPlusActorMarker {}

// Client-side registry. The module reports playback start/stop through SetPlaybackActive; the tick
// also requires SCPlusEventRunner.scp_CameraActive (set every frame by the module), so closing the
// camera always brings the markers back even if a stop was missed.
class SCPlusActorMarkers: Managed
{
	static const int TICK_MS = 100;

	protected static ref SCPlusActorMarkers s_Instance;

	protected ref array<SCPlusActorMarker> scp_Markers = new array<SCPlusActorMarker>();
	protected bool scp_Hidden;
	protected bool scp_Ticking;
	protected bool scp_PlaybackActive;

	static SCPlusActorMarkers Get()
	{
		if (!s_Instance) {
			s_Instance = new SCPlusActorMarkers();
		}
		return s_Instance;
	}

	void Register(SCPlusActorMarker marker)
	{
		if (!marker || scp_Markers.Find(marker) != -1) {
			return;
		}
		scp_Markers.Insert(marker);
		Apply(marker);
		if (!scp_Ticking) {
			scp_Ticking = true;
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Tick, TICK_MS, true);
		}
	}

	void Unregister(SCPlusActorMarker marker)
	{
		int i = scp_Markers.Find(marker);
		if (i != -1) {
			scp_Markers.Remove(i);
		}
		if (scp_Markers.Count() == 0 && scp_Ticking) {
			scp_Ticking = false;
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Tick);
		}
	}

	void Apply(SCPlusActorMarker marker)
	{
		if (marker) {
			marker.SetInvisible(scp_Hidden);
		}
	}

	void SetPlaybackActive(bool playing)
	{
		scp_PlaybackActive = playing;
		Tick();
	}

	bool IsHidden()
	{
		return scp_Hidden;
	}

	protected void Tick()
	{
		bool hide = scp_PlaybackActive && SCPlusEventRunner.scp_CameraActive;
		bool changed = hide != scp_Hidden;
		scp_Hidden = hide;
		// While hidden, re-apply every tick: vanilla hand/action code can toggle visibility of held items
		if (!changed && !hide) {
			return;
		}
		for (int i = scp_Markers.Count() - 1; i >= 0; i--) {
			SCPlusActorMarker marker = scp_Markers[i];
			if (!marker) {
				scp_Markers.Remove(i);
				continue;
			}
			marker.SetInvisible(hide);
		}
		if (changed) {
			// Print("[SCPlus Markers] " + scp_Markers.Count().ToString() + " marker(s) " + hide.ToString());
		}
	}
}
