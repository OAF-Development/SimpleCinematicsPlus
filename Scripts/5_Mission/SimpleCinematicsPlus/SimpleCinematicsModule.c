modded class SimpleCinematicsModule
{
	override void Init()
	{
		super.Init();
		BuildHotkeyTables();
		
		// Event track shots fired on a client are repeated on the server (multiplayer only)
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusFireServer", this);
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusSetDateServer", this);
		SCPlusEventRunner.OnShotFired.Insert(SendShotToServer);
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusMoveServer", this);
		SCPlusEventRunner.OnMoveChanged.Insert(SendMoveToServer);
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusActorResetServer", this);
		SCPlusEventRunner.OnActorReset.Insert(SendActorResetToServer);
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusTargetResetServer", this);
		SCPlusEventRunner.OnTargetReset.Insert(SendTargetResetToServer);
		GetRPCManager().AddRPC("SimpleCinematicsPlus", "SCPlusPathServer", this);
		SCPlusEventRunner.OnPathChanged.Insert(SendPathToServer);
	}
	
	protected void SendPathToServer(PlayerBase player, int speed, array<vector> points)
	{
		array<vector> sent = new array<vector>();
		if (points) {
			sent.Copy(points);
		}
		
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusPathServer", new Param2<int, ref array<vector>>(speed, sent), true, null, player);
	}
	
	void SCPlusPathServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !GetGame().IsMultiplayer() || !sender) {
			return;
		}
		
		// A client can only steer its own character
		PlayerBase player = PlayerBase.Cast(target);
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != sender.GetId()) {
			return;
		}
		
		// Same allow-list as Simple Cinematics' own server controls (date, weather): on a live server only
		// allowed users can drive their character, teleport it back to the actor start, or fire through events
		if (!IsSenderAllowed(sender)) {
			return;
		}
		
		Param2<int, ref array<vector>> data;
		if (!ctx.Read(data)) {
			return;
		}
		
		SCPlusPathFollower.PathOnServer(player, data.param1, data.param2);
	}

	protected void SendTargetResetToServer(Object target, vector position, vector orientation)
	{
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusTargetResetServer", new Param2<vector, vector>(position, orientation), true, null, target);
	}
	
	void SCPlusTargetResetServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !GetGame().IsMultiplayer() || !sender) {
			return;
		}
		
		// Moving animals, vehicles or other players needs the same allow-list as SC's server controls
		if (!IsSenderAllowed(sender) || !SCPlusEventRunner.IsResettableTarget(target)) {
			return;
		}
		
		Param2<vector, vector> data;
		if (!ctx.Read(data)) {
			return;
		}
		
		// Print(string.Format("[SCPlus Events] %1 reset scene target %2", GetSenderId(sender), target.GetType()));
		SCPlusEventRunner.ResetTargetOnServer(target, data.param1, data.param2);
	}
	
	protected void SendActorResetToServer(PlayerBase player, vector position, vector orientation)
	{
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusActorResetServer", new Param2<vector, vector>(position, orientation), true, null, player);
	}
	
	void SCPlusActorResetServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !GetGame().IsMultiplayer() || !sender) {
			return;
		}
		
		// A client can only move its own character
		PlayerBase player = PlayerBase.Cast(target);
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != sender.GetId()) {
			return;
		}
		
		// Same allow-list as Simple Cinematics' own server controls (date, weather): on a live server only
		// allowed users can drive their character, teleport it back to the actor start, or fire through events
		if (!IsSenderAllowed(sender)) {
			return;
		}
		
		Param2<vector, vector> data;
		if (!ctx.Read(data)) {
			return;
		}
		
		SCPlusEventRunner.ResetActorOnServer(player, data.param1, data.param2);
	}

	protected void SendMoveToServer(PlayerBase player, int speed, float angle, float duration)
	{
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusMoveServer", new Param3<int, float, float>(speed, angle, duration), true, null, player);
	}
	
	void SCPlusMoveServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !GetGame().IsMultiplayer() || !sender) {
			return;
		}
		
		// A client can only move its own character
		PlayerBase player = PlayerBase.Cast(target);
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != sender.GetId()) {
			return;
		}
		
		// Same allow-list as Simple Cinematics' own server controls (date, weather): on a live server only
		// allowed users can drive their character, teleport it back to the actor start, or fire through events
		if (!IsSenderAllowed(sender)) {
			return;
		}
		
		Param3<int, float, float> data;
		if (!ctx.Read(data)) {
			return;
		}
		
		SCPlusEventRunner.MoveOnServer(player, data.param1, data.param2, data.param3);
	}
	
	protected void SendShotToServer(PlayerBase player)
	{
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusFireServer", new Param1<int>(0), true, null, player);
	}
	
	void SCPlusFireServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !GetGame().IsMultiplayer() || !sender) {
			return;
		}
		
		// A client can only fire its own character
		PlayerBase player = PlayerBase.Cast(target);
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != sender.GetId()) {
			return;
		}
		
		// Same allow-list as Simple Cinematics' own server controls (date, weather): on a live server only
		// allowed users can drive their character, teleport it back to the actor start, or fire through events
		if (!IsSenderAllowed(sender)) {
			return;
		}
		
		SCPlusEventRunner.FireOnServer(player);
	}
	
	// Hotkeys that nudge a HUD slider: input name, slider name (see GetHotkeySlider), step.
	// Held hotkeys apply their step every frame, press hotkeys once per press.
	protected static ref array<ref Param3<string, string, float>> s_SliderHotkeys;
	protected static ref array<ref Param3<string, string, float>> s_SliderPressHotkeys;
	
	protected static void BuildHotkeyTables()
	{
		if (s_SliderHotkeys) {
			return;
		}
		
		s_SliderHotkeys = new array<ref Param3<string, string, float>>();
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCCameraSensUp", "CameraSensitivitySlider", 0.15));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCCameraSensDown", "CameraSensitivitySlider", -0.15));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCMovementSmoothUp", "CameraMovementSmoothness", 0.02));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCMovementSmoothDown", "CameraMovementSmoothness", -0.02));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCMouseSmoothUp", "CameraMouseSmoothness", 0.02));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCMouseSmoothDown", "CameraMouseSmoothness", -0.02));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCIncreaseDOF", "CameraDOFSlider", 0.03));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCDecreaseDOF", "CameraDOFSlider", -0.03));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCIncreaseFOV", "CameraFOVSlider", 0.15));
		s_SliderHotkeys.Insert(new Param3<string, string, float>("SCDecreaseFOV", "CameraFOVSlider", -0.15));
		
		s_SliderPressHotkeys = new array<ref Param3<string, string, float>>();
		s_SliderPressHotkeys.Insert(new Param3<string, string, float>("UAZoomInOptics", "CameraSpeedSlider", 0.05));
		s_SliderPressHotkeys.Insert(new Param3<string, string, float>("UAZoomOutOptics", "CameraSpeedSlider", -0.05));
	}
	
	// Full replacement of the original OnUpdate (super is NOT called). Changes vs. the original:
	//  - hotkeys come from the table above instead of 14 copy-pasted blocks; the original checked
	//    SCMovementSmoothUp twice, so "decrease movement smoothing" never worked
	//  - hotkeys no longer return early, which used to skip target picking that frame
	//  - clicking empty sky while picking a target no longer indexes an empty set
	//  - works offline (the original returned whenever IsServer(), which is true in singleplayer)
	//  - ignores hotkeys while typing in a HUD text box
	// If Simple Cinematics changes its OnUpdate, re-copy it here and re-apply these changes.
	override void OnUpdate(float timeslice)
	{
		if (GetGame().IsServer() && GetGame().IsMultiplayer()) {
			return;
		}
		
		SimpleCinematicsConfig cfg = SimpleCinematicsConfig.Cast(GetDayZGame().GetMissionSetting(SimpleCinematicsConfig));
		if (!cfg) {
			return;
		}
		
		m_Player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!m_Player) {
			return;
		}
		
		if (!cfg.IsUserAllowed(m_Player.GetIdentity())) {
			return;
		}
		
		DrawFramingOverlay();
		
		// Record Take and key help requests from the HUD buttons, then the take countdown
		if (SCPlusUIRequests.scp_RecordTake) {
			SCPlusUIRequests.scp_RecordTake = false;
			StartTake();
		}
		
		if (SCPlusUIRequests.scp_ToggleKeyHelp) {
			SCPlusUIRequests.scp_ToggleKeyHelp = false;
			if (m_Active) {
				SCPlusKeyHelp.Toggle();
			}
		}
		
		UpdateTake(timeslice);
		
		// Event track: pending shots and weapon raise/lower
		SCPlusEventRunner.scp_CameraActive = m_Active;
		SCPlusEventRunner.Get().Update(timeslice);
		string event_message = SCPlusEventRunner.Get().TakeMessage();
		if (event_message != string.Empty && m_SimpleCinematicsHud) {
			m_SimpleCinematicsHud.GetTemplateController().ShowMessage(event_message);
		}
		
		// Pose Hold: tell EmoteManager (4_World) whether camera keys may cancel the emote
		SCPlusPoseHold.scp_Active = m_Active && (!m_SimpleCinematicsHud || m_SimpleCinematicsHud.GetTemplateController().HoldPose);
		
		// Bullet Cam settings for Weapon_Base / the camera (4_World)
		SCPlusBulletCam.scp_Armed = false;
		if (m_Active && m_SimpleCinematicsHud) {
			SimpleCinematicsHudController bullet_settings = m_SimpleCinematicsHud.GetTemplateController();
			SCPlusBulletCam.scp_Armed = bullet_settings.BulletCam;
			SCPlusBulletCam.scp_ShowBullet = bullet_settings.BulletCamShowRound;
			SCPlusBulletCam.scp_ShowPath = bullet_settings.BulletCamShowPath;
			if (!bullet_settings.BulletCamShowPath) {
				SCPlusBulletCam.ClearDebugShot();
			}
			SCPlusBulletCam.scp_SpeedFactor = Math.Clamp(bullet_settings.BulletCamSpeed, 1, 25) / 100;
			SCPlusBulletCam.scp_Distance = Math.Clamp(bullet_settings.BulletCamDistance, 0.2, 5);
		}
		
		// Keep the character's head out of freelook while filming, so it settles facing forward instead of
		// staying turned to wherever it was looking when the camera opened
		if (m_Active) {
			ReleasePlayerHead();
		}
		
		// Hold the character's own look/aim still while filming. Without this the mouse movement used to
		// steer the cinematic camera also turned the player's camera and aim, so on exit the head and rifle
		// snapped to wherever you had been looking.
		LockPlayerAim(m_Active);
		
		// Don't treat typing in a HUD text box (e.g. a preset name) as hotkeys
		if (SimpleCinematicsCamera.IsTypingInUI()) {
			return;
		}
				
		Input input = GetGame().GetInput();
		
		if (input.LocalPress("SCCameraToggle")) {
			CameraToggle();
		}
		
		if (input.LocalPress("SCCameraHudToggle")) {
			CameraHudToggle();
		}
		
		if (input.LocalPress("SCCameraToggleCursor") && m_SimpleCinematicsHud && m_SimpleCinematicsHud.GetLayoutRoot().IsVisible()) {
			ToggleCursor();
		}
		
		if (input.LocalValue("SCQuickLeft")) {
			QuickMoveLeft(timeslice);
		}
		
		if (input.LocalValue("SCQuickRight")) {
			QuickMoveRight(timeslice);
		}
		
		// Sequencer hotkeys
		if (m_Active && m_SimpleCinematicsHud) {
			SimpleCinematicsHudController sequencer = m_SimpleCinematicsHud.GetTemplateController();
			if (input.LocalPress("SCSeqSetKey")) {
				sequencer.SetKeyAtPlayhead();
			}
			
			if (input.LocalPress("SCSeqPlayPause")) {
				// During a take countdown, play / pause cancels the take
				if (m_TakeState == SCPLUS_TAKE_COUNTDOWN) {
					FinishTake("Take cancelled");
				} else {
					sequencer.TogglePlayPause();
				}
			}
			
			// F1: key cheat-sheet (a raw key, like Ctrl+Z, so no keybind is added or changed)
			if (KeyPressedOnce(KeyCode.KC_F1, m_KeyHelpKeyWasDown)) {
				SCPlusKeyHelp.Toggle();
			}
			
			if (input.LocalPress("SCSeqNextKey")) {
				sequencer.JumpToNextKey();
			}
			
			if (input.LocalPress("SCSeqPreviousKey")) {
				sequencer.JumpToPreviousKey();
			}
			
			if (input.LocalPress("SCSeqDeleteKey")) {
				// Delete removes the selected event marker if there is one, otherwise the key
				if (sequencer.HasSelectedEvent()) {
					sequencer.DeleteSelectedEvent();
				} else {
					sequencer.DeleteSelectedKey();
				}
			}
			
			if (input.LocalPress("SCPlusTogglePoseHold")) {
				sequencer.TogglePoseHold();
			}
			
			// Ctrl+Z / Ctrl+Y: undo / redo key edits
			bool ctrl_down = KeyState(KeyCode.KC_LCONTROL) != 0 || KeyState(KeyCode.KC_RCONTROL) != 0;
			if (KeyPressedOnce(KeyCode.KC_Z, m_UndoKeyWasDown) && ctrl_down) {
				sequencer.Undo();
			}
			
			if (KeyPressedOnce(KeyCode.KC_Y, m_RedoKeyWasDown) && ctrl_down) {
				sequencer.Redo();
			}
			
			// Left / Right arrows step the playhead back / forward 0.1 s (1 s with Shift). Simple Cinematics also uses
			// the arrows to nudge a "quick slider" (the box ticked next to a slider), so they only move the
			// playhead while no quick slider is ticked.
			BindableSliderPrefab quick_slider = BindableSliderPrefab.Cast(sequencer.GetActiveQuickSlider());
			bool quick_slider_active = quick_slider && quick_slider.IsQuickSliderActive();
			float nudge = 0.1;
			if (KeyState(KeyCode.KC_LSHIFT) != 0) {
				nudge = 1.0;
			}
			
			if (KeyPressedOnce(KeyCode.KC_LEFT, m_NudgeBackWasDown) && !quick_slider_active) {
				sequencer.NudgePlayhead(-nudge);
			}
			
			if (KeyPressedOnce(KeyCode.KC_RIGHT, m_NudgeForwardWasDown) && !quick_slider_active) {
				sequencer.NudgePlayhead(nudge);
			}
			
			// Mouse wheel: steps the ticked quick slider. With no box ticked it changes Camera Speed as before,
			// except while the cursor is over a scrolling panel, where the wheel only scrolls the panel.
			bool over_panel = IsCursorOverScrollPanel();
			if (m_SimpleCinematicsCamera) {
				m_SimpleCinematicsCamera.scp_BlockWheelSpeed = quick_slider_active || over_panel;
			}
			
			int wheel = GetMouseState(MouseState.WHEEL);
			if (quick_slider_active && !over_panel && wheel != 0) {
				for (int notch = 0; notch < Math.AbsInt(wheel); notch++) {
					if (wheel > 0) {
						quick_slider.OnButtonUp(null);
					} else {
						quick_slider.OnButtonDown(null);
					}
				}
			}
		} else if (m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.scp_BlockWheelSpeed = false;
		}
		
		if (m_SimpleCinematicsHud) {
			BuildHotkeyTables();
			SimpleCinematicsHudController controller = m_SimpleCinematicsHud.GetTemplateController();
			foreach (Param3<string, string, float> hotkey: s_SliderHotkeys) {
				if (input.LocalValue(hotkey.param1)) {
					NudgeSlider(controller.GetHotkeySlider(hotkey.param2), hotkey.param3);
				}
			}
			
			// These are the mouse wheel (Camera Speed). Skip them while the wheel belongs to a ticked slider or a panel.
			bool wheel_taken = m_SimpleCinematicsCamera && m_SimpleCinematicsCamera.scp_BlockWheelSpeed;
			foreach (Param3<string, string, float> press_hotkey: s_SliderPressHotkeys) {
				if (!wheel_taken && input.LocalPress(press_hotkey.param1)) {
					NudgeSlider(controller.GetHotkeySlider(press_hotkey.param2), press_hotkey.param3);
				}
			}
		}
		
		if (m_Active && m_SimpleCinematicsHud && m_SimpleCinematicsCamera) {
			HandleWaypointEditing(m_SimpleCinematicsHud.GetTemplateController(), input);
		}
		
		if (m_Active && m_SimpleCinematicsHud && GetGame().GetUIManager().IsCursorVisible() && input.LocalPress("UAFire")) {
			HandleTargetPick(m_SimpleCinematicsHud.GetTemplateController());
		}
	}
	
	// Edge detection for raw keys that aren't declared inputs
	protected bool m_UndoKeyWasDown;
	protected bool m_RedoKeyWasDown;
	protected bool m_NudgeBackWasDown;
	protected bool m_NudgeForwardWasDown;
	protected bool m_KeyHelpKeyWasDown;
	
	protected bool KeyPressedOnce(KeyCode key, inout bool was_down)
	{
		bool down = KeyState(key) != 0;
		bool pressed = down && !was_down;
		was_down = down;
		return pressed;
	}
	
	// --- Framing overlay: letterbox bars and a rule-of-thirds grid ---
	// Lives in its own widget tree, created before the HUD so the HUD draws on top of it,
	// and stays visible when the HUD is hidden (for capturing).
	protected Widget m_OverlayRoot;
	protected CanvasWidget m_OverlayCanvas;
	
	protected void CreateFramingOverlay()
	{
		if (m_OverlayRoot) {
			return;
		}
		
		m_OverlayRoot = GetGame().GetWorkspace().CreateWidgets("SimpleCinematicsPlus/GUI/layouts/hud/overlay.layout");
		if (m_OverlayRoot) {
			m_OverlayCanvas = CanvasWidget.Cast(m_OverlayRoot.FindAnyWidget("SCPlusOverlayCanvas"));
			m_OverlayText = TextWidget.Cast(m_OverlayRoot.FindAnyWidget("SCPlusOverlayText"));
		}
	}
	
	protected void DestroyFramingOverlay()
	{
		if (m_OverlayRoot) {
			m_OverlayRoot.Unlink();
		}
		
		m_OverlayRoot = null;
		m_OverlayCanvas = null;
		m_OverlayText = null;
	}
	
	// --- Record Take: a clean, hands-off take for screen recording ---
	// Hides the HUD, cursor, thirds grid, waypoint paths and actor markers, rewinds (putting the actor back),
	// counts down 3-2-1 in the middle of the screen, holds a clean second of pre-roll, then plays the sequence
	// once (loop off). Everything is restored when playback ends or is paused. Play / Pause cancels the countdown.
	static const int SCPLUS_TAKE_IDLE = 0;
	static const int SCPLUS_TAKE_COUNTDOWN = 1;
	static const int SCPLUS_TAKE_PLAYING = 2;
	static const float SCPLUS_TAKE_COUNT = 3; // seconds of 3-2-1
	static const float SCPLUS_TAKE_PREROLL = 1; // clean seconds between the countdown and playback
	
	protected TextWidget m_OverlayText;
	protected int m_TakeState;
	protected float m_TakeTimer;
	protected bool m_TakeHudWasVisible;
	protected bool m_TakeCursorWasVisible;
	protected bool m_TakeThirdsWasOn;
	protected bool m_TakeLoopWasOn;
	
	protected void StartTake()
	{
		if (!m_Active || !m_SimpleCinematicsCamera || !m_SimpleCinematicsHud || m_TakeState != SCPLUS_TAKE_IDLE) {
			return;
		}
		
		SimpleCinematicsHudController controller = m_SimpleCinematicsHud.GetTemplateController();
		if (!controller.SCPlusHasSequenceKeys()) {
			controller.ShowMessage("Set at least two keys before recording a take");
			return;
		}
		
		SCPlusKeyHelp.Show(false);
		m_TakeHudWasVisible = m_SimpleCinematicsHud.GetLayoutRoot().IsVisible();
		m_TakeCursorWasVisible = GetGame().GetUIManager().IsCursorVisible();
		m_TakeThirdsWasOn = controller.ShowThirdsGrid;
		m_TakeLoopWasOn = m_SimpleCinematicsCamera.SequenceLoop;
		
		controller.ShowThirdsGrid = false;
		m_SimpleCinematicsCamera.SequenceLoop = false;
		m_SimpleCinematicsCamera.Stop();
		m_SimpleCinematicsHud.GetLayoutRoot().Show(false);
		GetGame().GetUIManager().ShowCursor(false);
		SCPlusActorMarkers.Get().SetPlaybackActive(true);
		
		m_TakeState = SCPLUS_TAKE_COUNTDOWN;
		m_TakeTimer = SCPLUS_TAKE_COUNT + SCPLUS_TAKE_PREROLL;
		// Print("[SCPlus Take] countdown started");
	}
	
	protected void UpdateTake(float timeslice)
	{
		if (m_TakeState != SCPLUS_TAKE_COUNTDOWN) {
			return;
		}
		
		if (!m_Active || !m_SimpleCinematicsCamera) {
			FinishTake("Take cancelled");
			return;
		}
		
		m_TakeTimer -= timeslice;
		float counting = m_TakeTimer - SCPLUS_TAKE_PREROLL;
		if (m_OverlayText) {
			m_OverlayText.Show(counting > 0);
			if (counting > 0) {
				int shown = Math.Ceil(counting);
				m_OverlayText.SetText(shown.ToString());
			}
		}
		
		if (m_TakeTimer > 0) {
			return;
		}
		
		m_TakeState = SCPLUS_TAKE_PLAYING;
		if (!m_SimpleCinematicsCamera.Play()) {
			FinishTake("Couldn't start playback (set at least two keys)");
			return;
		}
		
		// Print("[SCPlus Take] playing");
	}
	
	protected void FinishTake(string message)
	{
		if (m_TakeState == SCPLUS_TAKE_IDLE) {
			return;
		}
		
		m_TakeState = SCPLUS_TAKE_IDLE;
		if (m_OverlayText) {
			m_OverlayText.Show(false);
		}
		
		if (m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.SequenceLoop = m_TakeLoopWasOn;
		}
		
		SCPlusActorMarkers.Get().SetPlaybackActive(false);
		if (m_SimpleCinematicsHud) {
			SimpleCinematicsHudController controller = m_SimpleCinematicsHud.GetTemplateController();
			controller.ShowThirdsGrid = m_TakeThirdsWasOn;
			if (m_Active) {
				m_SimpleCinematicsHud.GetLayoutRoot().Show(m_TakeHudWasVisible);
				GetGame().GetUIManager().ShowCursor(m_TakeCursorWasVisible && m_TakeHudWasVisible);
			}
			
			controller.ShowMessage(message);
		}
		
		// Print("[SCPlus Take] " + message);
	}
	
	// Bullet Cam "Show Path (debug)" lines, projected onto the overlay canvas. Each line is drawn in short
	// pieces so the parts behind the camera can be skipped.
	protected void DrawBulletDebugLines()
	{
		int count = SCPlusBulletCam.scp_DebugFrom.Count();
		for (int i = 0; i < count; i++) {
			vector from = SCPlusBulletCam.scp_DebugFrom[i];
			vector to = SCPlusBulletCam.scp_DebugTo[i];
			int color = SCPlusBulletCam.scp_DebugColor[i];
			float pieces = 40;
			vector previous = GetGame().GetScreenPos(from);
			for (int piece = 1; piece <= pieces; piece++) {
				vector current = GetGame().GetScreenPos(vector.Lerp(from, to, piece / pieces));
				if (previous[2] > 0 && current[2] > 0) {
					m_OverlayCanvas.DrawLine(previous[0], previous[1], current[0], current[1], 3, color);
				}
				
				previous = current;
			}
		}
	}
	
	protected void DrawFramingOverlay()
	{
		if (!m_OverlayCanvas) {
			return;
		}
		
		m_OverlayCanvas.Clear();
		DrawBulletDebugLines();
		if (!m_Active || !m_SimpleCinematicsHud) {
			return;
		}
		
		SimpleCinematicsHudController controller = m_SimpleCinematicsHud.GetTemplateController();
		float ratio = controller.GetLetterboxRatio();
		bool thirds = controller.ShowThirdsGrid;
		if (ratio <= 0 && !thirds) {
			return;
		}
		
		float width, height;
		m_OverlayRoot.GetScreenSize(width, height);
		if (width <= 0 || height <= 0) {
			return;
		}
		
		// Frame inside the bars
		float frame_x = 0;
		float frame_y = 0;
		float frame_w = width;
		float frame_h = height;
		if (ratio > 0) {
			if (ratio > width / height) {
				frame_h = width / ratio;
				frame_y = (height - frame_h) / 2;
				m_OverlayCanvas.DrawLine(0, frame_y / 2, width, frame_y / 2, frame_y, 0xFF000000);
				m_OverlayCanvas.DrawLine(0, height - frame_y / 2, width, height - frame_y / 2, frame_y, 0xFF000000);
			} else {
				frame_w = height * ratio;
				frame_x = (width - frame_w) / 2;
				m_OverlayCanvas.DrawLine(frame_x / 2, 0, frame_x / 2, height, frame_x, 0xFF000000);
				m_OverlayCanvas.DrawLine(width - frame_x / 2, 0, width - frame_x / 2, height, frame_x, 0xFF000000);
			}
		}
		
		if (thirds) {
			int grid_color = 0x66FFFFFF;
			for (int i = 1; i <= 2; i++) {
				float gx = frame_x + frame_w * i / 3;
				float gy = frame_y + frame_h * i / 3;
				m_OverlayCanvas.DrawLine(gx, frame_y, gx, frame_y + frame_h, 1, grid_color);
				m_OverlayCanvas.DrawLine(frame_x, gy, frame_x + frame_w, gy, 1, grid_color);
			}
		}
	}
	
	protected void NudgeSlider(BindableSliderPrefab slider, float delta)
	{
		if (!slider) {
			return;
		}
		
		// Setting CalculatedValue flows through SliderPrefab, which clamps and writes back to the camera
		slider.GetPrefabController().CalculatedValue = slider.GetPrefabController().CalculatedValue + delta;
		slider.GetPrefabController().NotifyPropertyChanged("CalculatedValue");
	}
	
	// --- Waypoint editing (Place Waypoints on, cursor shown) ---
	// Click a waypoint disc to select it and drag it; click the ground to add a waypoint after the selected one.
	// P toggles Place Waypoints. P is read as a raw key (like Ctrl+Z), so no keybind is added or changed.
	static const float WAYPOINT_PICK_PIXELS = 22;
	protected int m_DraggedWaypoint = -1;
	protected bool m_WaypointKeyWasDown;
	
	protected void HandleWaypointEditing(SimpleCinematicsHudController controller, Input input)
	{
		bool p_pressed = KeyPressedOnce(KeyCode.KC_P, m_WaypointKeyWasDown);
		bool ctrl_held = KeyState(KeyCode.KC_LCONTROL) != 0 || KeyState(KeyCode.KC_RCONTROL) != 0;
		if (p_pressed && !ctrl_held && !SimpleCinematicsCamera.IsTypingInUI()) {
			controller.PlaceWaypoints = !controller.PlaceWaypoints;
			controller.NotifyPropertyChanged("PlaceWaypoints");
			if (controller.PlaceWaypoints) {
				controller.ShowMessage("Place Waypoints on (P)");
			} else {
				controller.ShowMessage("Place Waypoints off (P)");
			}
		}
		
		if (!controller.PlaceWaypoints || !GetGame().GetUIManager().IsCursorVisible() || m_SimpleCinematicsCamera.IsPlaying()) {
			m_DraggedWaypoint = -1;
			return;
		}
		
		if (m_DraggedWaypoint >= 0) {
			if (input.LocalValue("UAFire") <= 0) {
				m_DraggedWaypoint = -1;
				return;
			}
			
			set<Object> drag_hits;
			vector drag_position = MousePosToRay(drag_hits, GetGame().GetPlayer());
			if (drag_position.Length() > 0.001) {
				m_SimpleCinematicsCamera.MoveWaypoint(m_DraggedWaypoint, drag_position);
			}
			
			return;
		}
		
		if (!input.LocalPress("UAFire") || IsCursorOverScrollPanel()) {
			return;
		}
		
		int picked = PickWaypointAtCursor();
		if (picked >= 0) {
			m_SimpleCinematicsCamera.SelectWaypoint(picked);
			controller.SCPlusSyncWaypointCurve();
			m_DraggedWaypoint = picked;
			return;
		}
		
		set<Object> ray_hits;
		vector waypoint = MousePosToRay(ray_hits, GetGame().GetPlayer());
		if (waypoint.Length() < 0.001) {
			controller.ShowMessage("Nothing under the cursor to place a waypoint on");
			return;
		}
		
		m_SimpleCinematicsCamera.AddWaypoint(waypoint);
		controller.SCPlusSyncWaypointCurve();
	}
	
	// Index of the waypoint disc under the cursor (within WAYPOINT_PICK_PIXELS), or -1
	protected int PickWaypointAtCursor()
	{
		array<vector> points = m_SimpleCinematicsCamera.GetWaypoints();
		if (!points) {
			return -1;
		}
		
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		int best = -1;
		float best_distance = WAYPOINT_PICK_PIXELS;
		for (int i = 0; i < points.Count(); i++) {
			vector screen = GetGame().GetScreenPos(SCPlusPathVisuals.GroundPoint(points[i]));
			if (screen[2] <= 0) {
				continue; // behind the camera
			}
			
			float dx = screen[0] - mouse_x;
			float dy = screen[1] - mouse_y;
			float distance = Math.Sqrt(dx * dx + dy * dy);
			if (distance < best_distance) {
				best_distance = distance;
				best = i;
			}
		}
		
		return best;
	}
	
	protected void HandleTargetPick(SimpleCinematicsHudController controller)
	{
		// Place Waypoints: clicks edit the path instead (HandleWaypointEditing)
		if (controller.PlaceWaypoints) {
			return;
		}

if (!controller.SelectingTargetFocusState && !controller.SelectingTargetFollowState) {
			return;
		}
		
		set<Object> collisions;
		CurrentMousePosition = MousePosToRay(collisions);
		
		// Clicking empty sky returns no objects. Indexing the empty set used to throw
		if (!collisions || collisions.Count() == 0 || !collisions[0]) {
			return;
		}
		
		Object hit = collisions[0];
		if (controller.SelectingTargetFocusState) {
			m_SimpleCinematicsCamera.SelectTarget(hit);
			controller.SelectingTargetFocusState = false;
			controller.NotifyPropertyChanged("SelectingTargetFocusState");
			controller.NotifyPropertyChanged("m_SimpleCinematicsCamera.SelectedTarget");
		}
		
		if (controller.SelectingTargetFollowState) {
			m_SimpleCinematicsCamera.SetFollowTarget(hit);
			controller.SelectingTargetFollowState = false;
			controller.NotifyPropertyChanged("SelectingTargetFollowState");
			controller.NotifyPropertyChanged("m_SimpleCinematicsCamera.FollowedTarget");
		}
	}
	
	// True when the HUD was hidden because playback started, so it can be shown again afterwards
	protected bool m_HudHiddenForPlayback;
	
	override void CameraToggle()
	{
		bool had_camera = (m_SimpleCinematicsCamera != null);
		
		// Closing the camera stops any sequence playback first
		if (m_Active && m_SimpleCinematicsCamera && m_SimpleCinematicsCamera.IsPlaying()) {
			m_SimpleCinematicsCamera.Pause();
		}
		
		// About to close: flush any pending debounced save
		if (m_Active && m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.SaveState();
		}
		
		// Opening: create the overlay first so the HUD (created by super) draws above it
		if (!had_camera) {
			CreateFramingOverlay();
		}
		
		super.CameraToggle();
		
		if (had_camera && !m_SimpleCinematicsCamera) {
			SCPlusPathVisuals.Clear();
			DestroyFramingOverlay();
		}
		
		if (!m_Active) {
			SCPlusKeyHelp.Show(false);
			FinishTake("Take cancelled");
		}
		
		// Closing the camera hands the weapon back to the player
		// Switch conflicting inputs off while filming, back on when the camera closes (keybinds untouched).
		// Re-enable a moment later so the key that closed the camera can't also trigger a COT action.
		if (m_Active) {
			SetConflictingInputsDisabled(true);
		} else {
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(RestoreConflictingInputs, 300, false);
		}
		
		if (!m_Active) {
			SCPlusEventRunner.Get().Reset();
			ReleasePlayerHead();
			SCPlusAimSync.scp_Pending = true; // ease the view back onto the weapon's aim
			if (m_SimpleCinematicsCamera) {
				m_SimpleCinematicsCamera.EndBulletCam();
			}
		}
		
		// Camera was just created: restore saved settings and last look, then sync the HUD sliders
		if (!had_camera && m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.OnPlaybackStateChanged.Insert(OnPlaybackStateChanged);
			m_SimpleCinematicsCamera.ApplySavedState();
			if (m_SimpleCinematicsHud) {
				m_SimpleCinematicsHud.GetTemplateController().RefreshCameraSliders();
			}
		}
	}
	
	// Inputs that fight the cinematic camera. Looked up by name at runtime, so a missing mod's input is
	// simply skipped. ForceDisable is a runtime switch only: the player's saved keybinds never change.
	// True while the cursor is shown and over a scrollable panel (or anything inside one)
	protected bool IsCursorOverScrollPanel()
	{
		if (!GetGame().GetUIManager().IsCursorVisible()) {
			return false;
		}
		
		Widget hovered = GetWidgetUnderCursor();
		while (hovered) {
			if (ScrollWidget.Cast(hovered)) {
				return true;
			}
			
			hovered = hovered.GetParent();
		}
		
		return false;
	}
	
	protected array<string> GetConflictingInputNames()
	{
		array<string> names = {
			// Vanilla freelook: Left Alt, which SC also uses to slow the camera
			"UALookAround",
			// COT free camera (Insert and Ctrl + wheel overlap SC's keys)
			"UACameraToolToggleCamera",
			"UACameraToolZoomForwards",
			"UACameraToolZoomBackwards",
			// (UACameraToolSpeedIncrease/Decrease stay on: Simple Cinematics reads them for Camera Speed)
			// COT keys that would act on the world while filming (H teleport, Delete removes the object under the cursor)
			"UATeleportModuleTeleportCursor",
			"UAObjectModuleDeleteOnCursor",
			// COT sidebar (Y, same as SC's HUD toggle)
			"UACOTToggleButtons"
		};
		return names;
	}
	
	protected void RestoreConflictingInputs()
	{
		// The camera was reopened before this ran: keep the inputs off
		if (m_Active) {
			return;
		}
		
		SetConflictingInputsDisabled(false);
	}
	
	void SetConflictingInputsDisabled(bool disabled)
	{
		UAInputAPI input_api = GetUApi();
		if (!input_api) {
			return;
		}
		
		array<string> input_names = GetConflictingInputNames();
		foreach (string input_name: input_names) {
			UAInput blocked_input = input_api.GetInputByName(input_name);
			if (blocked_input) {
				blocked_input.ForceDisable(disabled);
			}
		}
		
		input_api.UpdateControls();
	}
	
	// Left Alt is both SC's "slow" modifier and vanilla's freelook key. The player's own camera keeps
	// running under the cinematic camera, so freelook can latch and leave the head stuck at its old
	// offset after exit. Clear the latch and any forced freelook when handing control back.
	protected bool m_PlayerAimLocked;
	
	protected void LockPlayerAim(bool locked)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.GetInputController()) {
			return;
		}
		
		HumanInputController hic = player.GetInputController();
		if (locked) {
			hic.OverrideAimChangeX(HumanInputControllerOverrideType.ENABLED, 0);
			hic.OverrideAimChangeY(HumanInputControllerOverrideType.ENABLED, 0);
			m_PlayerAimLocked = true;
		} else if (m_PlayerAimLocked) {
			hic.OverrideAimChangeX(HumanInputControllerOverrideType.DISABLED, 0);
			hic.OverrideAimChangeY(HumanInputControllerOverrideType.DISABLED, 0);
			m_PlayerAimLocked = false;
		}
	}
	
	protected void ReleasePlayerHead()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) {
			return;
		}
		
		HumanInputController hic = player.GetInputController();
		if (hic) {
			hic.ResetFreeLookToggle();
		}
		
		DayZPlayerCameraBase player_camera = DayZPlayerCameraBase.Cast(player.GetCurrentCamera());
		if (player_camera) {
			player_camera.ForceFreelook(false);
		}
	}
	
	protected void OnPlaybackStateChanged(SimpleCinematicsCamera camera, bool playing)
	{
		// Actor markers are hidden from this client only while a sequence plays back
		SCPlusActorMarkers.Get().SetPlaybackActive(playing);
		
		// Auto-Key: a finished bake becomes the sequence's keys
		if (!playing && m_SimpleCinematicsHud) {
			m_SimpleCinematicsHud.GetTemplateController().SCPlusCommitBake();
		}
		
		// A Record Take ends when its playback stops (end reached, paused or camera closed)
		if (!playing && m_TakeState == SCPLUS_TAKE_PLAYING) {
			FinishTake("Take finished");
			return;
		}
		
		if (!m_SimpleCinematicsHud) {
			return;
		}
		
		SimpleCinematicsHudController controller = m_SimpleCinematicsHud.GetTemplateController();
		if (playing) {
			if (controller.SequenceHideHud && m_SimpleCinematicsHud.GetLayoutRoot().IsVisible()) {
				m_HudHiddenForPlayback = true;
				m_SimpleCinematicsHud.GetLayoutRoot().Show(false);
				GetGame().GetUIManager().ShowCursor(false);
			}
			
			return;
		}
		
		if (m_HudHiddenForPlayback) {
			m_HudHiddenForPlayback = false;
			if (m_Active) {
				m_SimpleCinematicsHud.GetLayoutRoot().Show(true);
				GetGame().GetUIManager().ShowCursor(true);
			}
		}
		
		controller.RefreshCameraSliders();
		controller.SyncTimeline();
	}
	
	void FlushSavedState()
	{
		if (m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.SaveState();
		}
	}
	
	// Fix: the original handlers didn't check who sent the RPC, so any client could change
	// server weather or time whenever the Allow flags were on
	override void SetWeatherServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!IsSenderAllowed(sender)) {
			PrintFormat("[SimpleCinematicsPlus] Rejected weather change from unauthorized sender %1", GetSenderId(sender));
			return;
		}
		
		super.SetWeatherServer(type, ctx, sender, target);
	}
	
	override void SetDateServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!IsSenderAllowed(sender)) {
			PrintFormat("[SimpleCinematicsPlus] Rejected date change from unauthorized sender %1", GetSenderId(sender));
			return;
		}
		
		super.SetDateServer(type, ctx, sender, target);
	}
	
	// Date Controls from a client: set the server's date, and with Lock Time keep putting it back so the
	// clock doesn't move on while filming
	protected bool m_DateLocked;
	protected int m_LockedYear, m_LockedMonth, m_LockedDay, m_LockedHour, m_LockedMinute;
	
	void SCPlusSetDateServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server) {
			return;
		}
		
		Param6<bool, int, int, int, int, int> date_params;
		if (!ctx.Read(date_params)) {
			return;
		}
		
		if (!IsSenderAllowed(sender)) {
			PrintFormat("[SimpleCinematicsPlus] Rejected date change from unauthorized sender %1", GetSenderId(sender));
			return;
		}
		
		SimpleCinematicsConfig config = SimpleCinematicsConfig.Cast(GetDayZGame().GetMissionSetting(SimpleCinematicsConfig));
		if (!config || !config.AllowDaytimeControl) {
			return;
		}
		
		m_LockedYear = date_params.param2;
		m_LockedMonth = date_params.param3;
		m_LockedDay = date_params.param4;
		m_LockedHour = date_params.param5;
		m_LockedMinute = date_params.param6;
		GetGame().GetWorld().SetDate(m_LockedYear, m_LockedMonth, m_LockedDay, m_LockedHour, m_LockedMinute);
		
		m_DateLocked = date_params.param1;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ReapplyLockedDate);
		if (m_DateLocked) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ReapplyLockedDate, 5000, true);
		}
	}
	
	protected void ReapplyLockedDate()
	{
		if (m_DateLocked) {
			GetGame().GetWorld().SetDate(m_LockedYear, m_LockedMonth, m_LockedDay, m_LockedHour, m_LockedMinute);
		}
	}
	
	protected bool IsSenderAllowed(PlayerIdentity sender)
	{
		SimpleCinematicsConfig config = SimpleCinematicsConfig.Cast(GetDayZGame().GetMissionSetting(SimpleCinematicsConfig));
		return config && config.IsUserAllowed(sender);
	}
	
	protected static string GetSenderId(PlayerIdentity sender)
	{
		if (!sender) {
			return "<no identity>";
		}
		
		return sender.GetPlainId();
	}
}
