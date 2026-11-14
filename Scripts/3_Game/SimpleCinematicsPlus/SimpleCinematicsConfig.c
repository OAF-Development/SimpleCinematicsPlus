// Fixes for Simple Cinematics' synced config, without editing the original mod.
modded class SimpleCinematicsConfig
{
	override bool Read(Serializer serializer, int version)
	{
		// The original Read() appends the synced users onto the field's default list,
		// so the hardcoded default ID ended up in front of the server's list on every client.
		// Remember how many defaults were there and drop them once the synced list is read.
		int default_count;
		if (AvailableUsers) {
			default_count = AvailableUsers.Count();
		}
		
		if (!super.Read(serializer, version)) {
			return false;
		}
		
		for (int i = 0; i < default_count && AvailableUsers.Count() > 0; i++) {
			AvailableUsers.RemoveOrdered(0);
		}
		
		return true;
	}
	
	bool IsUserAllowed(PlayerIdentity identity)
	{
		// Offline / singleplayer has no identity, always allow
		if (!GetGame().IsMultiplayer()) {
			return true;
		}
		
		if (!identity || !AvailableUsers) {
			return false;
		}
		
		return AvailableUsers.Find(identity.GetPlainId()) != -1;
	}
}
