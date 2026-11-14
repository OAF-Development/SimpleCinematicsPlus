// Keeps the last few versions of a saved file before it is overwritten or deleted:
//   $profile:SimpleCinematics\Backups\<kind>\<name>.1.json (newest) .. <name>.5.json (oldest)
// Used for sequences and presets. Internal files (names starting with "_") are never backed up.
class SCPlusBackups
{
	static const string ROOT = "$profile:SimpleCinematics\\Backups";
	static const int KEEP = 5;
	
	static string GetBackupPath(string kind, string name, int slot)
	{
		return string.Format("%1\\%2\\%3.%4.json", ROOT, kind, name, slot);
	}
	
	// Copies file_path into slot 1, moving older copies down a slot. Returns false if nothing was kept.
	static bool Keep(string kind, string file_path, string name)
	{
		if (name == string.Empty || name.IndexOf("_") == 0 || !FileExist(file_path)) {
			return false;
		}
		
		if (!FileExist("$profile:SimpleCinematics")) {
			MakeDirectory("$profile:SimpleCinematics");
		}
		
		if (!FileExist(ROOT)) {
			MakeDirectory(ROOT);
		}
		
		string folder = string.Format("%1\\%2", ROOT, kind);
		if (!FileExist(folder)) {
			MakeDirectory(folder);
		}
		
		// Oldest falls off the end, then every copy moves down one slot
		string oldest = GetBackupPath(kind, name, KEEP);
		if (FileExist(oldest)) {
			DeleteFile(oldest);
		}
		
		for (int slot = KEEP - 1; slot >= 1; slot--) {
			string from = GetBackupPath(kind, name, slot);
			if (!FileExist(from)) {
				continue;
			}
			
			string to = GetBackupPath(kind, name, slot + 1);
			if (FileExist(to)) {
				DeleteFile(to);
			}
			
			CopyFile(from, to);
			DeleteFile(from);
		}
		
		bool kept = CopyFile(file_path, GetBackupPath(kind, name, 1));
		Print(string.Format("[SCPlus Backups] %1 '%2' -> slot 1 (%3)", kind, name, kept));
		return kept;
	}
}
