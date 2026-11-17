class CfgPatches
{
	class SimpleCinematicsPlus_Scripts
	{
		units[] = {"SCPlusActorMarker_Green", "SCPlusActorMarker_Yellow", "SCPlusActorMarker_Red", "SCPlusActorMarker_Blue", "SCPlusActorMarker_White"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DF_Scripts", "SimpleCinematics_Scripts"};
	};
};
class CfgMods
{
	class SimpleCinematicsPlus
	{
		name = "Simple Cinematics Plus";
		dir = "SimpleCinematicsPlus";
		credits = "Utilizes dependencies from Dabs Framework and Simple Cinematics - Credits to Dab and contributors";
		author = "iLLMannerSevs";
		inputs = "SimpleCinematicsPlus/Scripts/Data/Inputs.xml";
        version = "1.0.0";
        type = "mod";
		dependencies[] = {"Game", "World", "Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"SimpleCinematicsPlus/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"SimpleCinematicsPlus/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"SimpleCinematicsPlus/Scripts/5_Mission"};
			};
		};
	};
};
class CfgVehicles
{
	class HouseNoDestruct;
	// Bare projectile (no casing) shown by Bullet Cam; spawned locally, never saved or networked
	class SCPlusBulletModel: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_bullet.p3d";
	};
	// Waypoint path visuals (see SCPlusPathVisuals); spawned locally, never saved or networked
	class SCPlusPathGreen: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_path_green.p3d";
	};
	class SCPlusPathYellow: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_path_yellow.p3d";
	};
	class SCPlusPathRed: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_path_red.p3d";
	};
	class SCPlusPathPoint: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_path_point.p3d";
	};
	class SCPlusPathPointSelected: HouseNoDestruct
	{
		scope = 1;
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_path_point_selected.p3d";
	};
	// Actor marker: a small ground cue for helpers (see 4_World/SimpleCinematicsPlus/SCPlusActorMarker.c).
	// Visible to everyone, hidden locally on the filming client while its camera is open.
	class ItemBase;
	class SCPlusActorMarker: ItemBase
	{
		scope = 2;
		displayName = "Actor Marker";
		descriptionShort = "Drop it where an actor should move to. Everyone sees it, but it never shows in the Simple Cinematics camera.";
		model = "SimpleCinematicsPlus\Scripts\Data\Models\scp_actor_marker.p3d";
		weight = 100;
		itemSize[] = {1,1};
		rotationFlags = 16;
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(0.15,1,0.25,1,co)"};
	};
	class SCPlusActorMarker_Green: SCPlusActorMarker
	{
		scope = 2;
		displayName = "Actor Marker (Green)";
	};
	class SCPlusActorMarker_Yellow: SCPlusActorMarker
	{
		scope = 2;
		displayName = "Actor Marker (Yellow)";
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,0.85,0.1,1,co)"};
	};
	class SCPlusActorMarker_Red: SCPlusActorMarker
	{
		scope = 2;
		displayName = "Actor Marker (Red)";
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,0.15,0.1,1,co)"};
	};
	class SCPlusActorMarker_Blue: SCPlusActorMarker
	{
		scope = 2;
		displayName = "Actor Marker (Blue)";
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(0.15,0.5,1,1,co)"};
	};
	class SCPlusActorMarker_White: SCPlusActorMarker
	{
		scope = 2;
		displayName = "Actor Marker (White)";
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(1,1,1,1,co)"};
	};
};
