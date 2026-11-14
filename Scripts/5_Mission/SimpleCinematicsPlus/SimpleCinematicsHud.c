modded class SimpleCinematicsHudController
{
	ref ObservableCollection<ref ScriptView> PresetHeader = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> PresetControls = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> PresetList = new ObservableCollection<ref ScriptView>(this);
	
	// Date Controls: on a server the date is also sent to the server (the original only set it on this
	// client, so the server's clock sync put it back within a minute). Lock Time keeps it there.
	bool LockTime;
	
	// Bound to the preset name edit box
	string PresetName;
	protected ref EditBoxPrefab m_PresetNameBox;
	// Set when script (not the player) changes a name box, so the HUD doesn't take it as typing
	bool scp_NameSetByScript;
	
	// Presets dropdown: a header row that opens and closes the list of saved presets
	protected ref SCPlusWideButton m_PresetDropdown;
	protected string m_ActivePreset;
	protected bool m_PresetListOpen;
	
	// Short message shown in the left panel header (the notification popup is easy to miss behind the HUD)
	protected string m_StatusMessage;
	protected int m_StatusUntil;
	
	// Full replacement of the original SetCamera (super is NOT called, it would build every slider twice).
	// Changes vs. the original:
	//  - mouse speed slider bound to CameraMouseSensitivity (was "CameraMouseSpeed", which doesn't exist)
	//  - hour and minute sliders start at 0 (were 1, so midnight and :00 were unreachable)
	//  - adds the Presets section
	// If Simple Cinematics changes its SetCamera, re-copy it here and re-apply these changes.
	override void SetCamera(SimpleCinematicsCamera camera)
	{
		m_SimpleCinematicsCamera = camera;
		m_World = GetGame().GetWorld();
		m_Weather = GetGame().GetWeather();
		
		CameraSpeedSlider = new BindableSliderPrefab("Camera Speed", m_SimpleCinematicsCamera, "CameraSpeed", 0, 200);
		CameraSensitivitySlider = new BindableSliderPrefab("Camera Mouse Speed", m_SimpleCinematicsCamera, "CameraMouseSensitivity", 0, 100);
		CameraMovementSmoothness = new BindableSliderPrefab("Movement Smoothing", m_SimpleCinematicsCamera, "CameraMovementSmoothing", 0, 1, 0.01);
		CameraMouseSmoothness = new BindableSliderPrefab("Mouse Smoothing", m_SimpleCinematicsCamera, "CameraMouseSmoothing", 0, 1, 0.01);
		CameraDOFSlider = new BindableSliderPrefab("DOF Dist", m_SimpleCinematicsCamera, "CameraDOFDistance", 0.1, 100);
		CameraFOVSlider = new BindableSliderPrefab("FOV", m_SimpleCinematicsCamera, "CameraFOV", 0.1, 180);
		
		CameraOptions.Insert(CameraSpeedSlider);
		CameraOptions.Insert(CameraSensitivitySlider);
		CameraOptions.Insert(CameraMovementSmoothness);
		CameraOptions.Insert(CameraMouseSmoothness);
		CameraOptions.Insert(CameraFOVSlider);
		
		CameraOptions.Insert(new BindableSliderPrefab("Near Plane", m_SimpleCinematicsCamera, "CameraNearPlane"));
		CameraOptions.Insert(new BindableSliderPrefab("DOF Blur", m_SimpleCinematicsCamera, "CameraDOFBlur", 0, 10, 0.01));
		CameraOptions.Insert(CameraDOFSlider);
		CameraOptions.Insert(new BindableSliderPrefab("View Distance", m_SimpleCinematicsCamera, "CameraViewDistance", 1, 25000));
		CameraOptions.Insert(new BindableSliderPrefab("Object View Distance", m_SimpleCinematicsCamera, "CameraObjectViewDistance", 1, 10000));
		
		CameraFilters.Insert(new BindableSliderPrefab("Vignette", m_SimpleCinematicsCamera, "CameraVignette", 0, 1));
		CameraFilters.Insert(new BindableSliderPrefab("Sharpness", m_SimpleCinematicsCamera, "CameraSharpness"));
		CameraFilters.Insert(new BindableSliderPrefab("Exposure", m_SimpleCinematicsCamera, "CameraExposure", 0, 5));
		
		CameraBlur.Insert(new BindableSliderPrefab("Gaussian Blur", m_SimpleCinematicsCamera, "CameraBlur", 0, 1));
		
		//CameraBlur.Insert(new CheckBoxPrefab("Lock Radial Blur", m_SimpleCinematicsCamera, "LockCameraRadialBlur"));
		CameraBlur.Insert(new BindableSliderPrefab("Radial Blur X", m_SimpleCinematicsCamera, "CameraRadialBlurX", 0, 1));
		CameraBlur.Insert(new BindableSliderPrefab("Radial Blur Y", m_SimpleCinematicsCamera, "CameraRadialBlurY", 0, 1));
		
		//CameraBlur.Insert(new CheckBoxPrefab("Lock Radial Blur Offset", m_SimpleCinematicsCamera, "LockCameraRadialBlurOffset"));
		CameraBlur.Insert(new BindableSliderPrefab("Radial Blur Offset X", m_SimpleCinematicsCamera, "CameraRadialBlurOffsetX", 0, 1));
		CameraBlur.Insert(new BindableSliderPrefab("Radial Blur Offset Y", m_SimpleCinematicsCamera, "CameraRadialBlurOffsetY", 0, 1));
		
		CameraColor.Insert(new BindableSliderPrefab("Red", m_SimpleCinematicsCamera, "CameraColorizeRed", 0, 1));
		CameraColor.Insert(new BindableSliderPrefab("Green", m_SimpleCinematicsCamera, "CameraColorizeGreen", 0, 1));
		CameraColor.Insert(new BindableSliderPrefab("Blue", m_SimpleCinematicsCamera, "CameraColorizeBlue", 0, 1));
		
		m_PresetDropdown = new SCPlusWideButton("Preset", this, "OnPresetDropdownClicked");
		PresetHeader.Insert(m_PresetDropdown);
		
		m_PresetNameBox = new EditBoxPrefab("Name", this, "PresetName");
		PresetControls.Insert(m_PresetNameBox);
		PresetControls.Insert(new SCPlusButton("Save Preset", this, "OnSavePresetClicked"));
		PresetControls.Insert(new SCPlusButton("Delete Preset", this, "OnDeletePresetClicked"));
		PresetControls.Insert(new SCPlusButton("Reset Look", this, "OnResetLookClicked"));
		RebuildPresetList();
		SetPresetListOpen(false);
		
		// The sequencer is set up by the SetCamera override in SimpleCinematicsHudSequencer.c
		// (that file compiles after this one, so its methods can't be called from here)
		
		int year, month, day, hour, minute;
		m_World.GetDate(year, month, day, hour, minute);
		m_Year = year;
		m_Month = month;
		m_Day = day;
		m_Hour = hour;
		m_Minute = minute;
		
		m_Rain = m_Weather.GetRain().GetActual();
		m_Fog = m_Weather.GetFog().GetActual();
		m_Overcast = m_Weather.GetOvercast().GetActual();
		m_Snow = m_Weather.GetSnowfall().GetActual();
		m_Snowscale = m_Weather.GetSnowflakeScale();
		m_Wind = m_Weather.GetWindMagnitude().GetActual();
		m_Winddir = m_Weather.GetWindDirection().GetActual() * Math.RAD2DEG;
		
		m_DynFogHeightBias = m_Weather.GetDynVolFogHeightBias();
		m_DynFogHeightDensity = m_Weather.GetDynVolFogHeightDensity();
		m_DynFogDistanceDensity = m_Weather.GetDynVolFogDistanceDensity();
				
		DateControls.Insert(new BindableSliderPrefab("Year", this, "m_Year", 1970, 2038, 1));
		DateControls.Insert(new BindableSliderPrefab("Month", this, "m_Month", 1, 12, 1));
		DateControls.Insert(new BindableSliderPrefab("Day", this, "m_Day", 1, 31, 1));
		DateControls.Insert(new BindableSliderPrefab("Hour", this, "m_Hour", 0, 23, 1));
		DateControls.Insert(new BindableSliderPrefab("Minute", this, "m_Minute", 0, 59, 1));
		DateControls.Insert(new CheckBoxPrefab("Lock Time (stops the clock)", this, "LockTime"));
		
		WeatherControls.Insert(new BindableSliderPrefab("Rain", this, "m_Rain", 0, 1));
		WeatherControls.Insert(new BindableSliderPrefab("Snow", this, "m_Snow", 0, 1));
		WeatherControls.Insert(new BindableSliderPrefab("Snow Scale", this, "m_Snowscale", 0, 1));
		WeatherControls.Insert(new BindableSliderPrefab("Fog", this, "m_Fog", 0, 1));
		WeatherControls.Insert(new BindableSliderPrefab("Overcast", this, "m_Overcast", 0, 1));
		WeatherControls.Insert(new BindableSliderPrefab("Wind Direction", this, "m_Winddir", 0, 360));
		WeatherControls.Insert(new BindableSliderPrefab("Wind Speed", this, "m_Wind", 0, 100));
		WeatherControls.Insert(new BindableSliderPrefab("Dynamic Fog Distance Density", this, "m_DynFogDistanceDensity"));
		WeatherControls.Insert(new BindableSliderPrefab("Dynamic Fog Height Bias", this, "m_DynFogHeightBias"));
		WeatherControls.Insert(new BindableSliderPrefab("Dynamic Fog Height Density", this, "m_DynFogHeightDensity"));
	}
	
	override void PropertyChanged(string property_name)
	{
		super.PropertyChanged(property_name);
		
		switch (property_name) {
			case "m_Year":
			case "m_Month":
			case "m_Day":
			case "m_Hour":
			case "m_Minute":
			case "LockTime": {
				// Sliders report every frame while dragged, so send once they settle
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SendDateToServer);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(SendDateToServer, 300);
				break;
			}
		}
	}
	
	void SendDateToServer()
	{
		if (!GetGame().IsMultiplayer()) {
			return; // offline the local SetDate is all there is
		}
		
		Param6<bool, int, int, int, int, int> date_params = new Param6<bool, int, int, int, int, int>(LockTime, m_Year, m_Month, m_Day, m_Hour, m_Minute);
		GetRPCManager().SendRPC("SimpleCinematicsPlus", "SCPlusSetDateServer", date_params, true);
	}
	
	// --- Presets ---
	
	void RebuildPresetList()
	{
		PresetList.Clear();
		array<string> preset_names = SCPlusPreset.List();
		foreach (string preset_name: preset_names) {
			SCPlusWideButton preset_button = new SCPlusWideButton(preset_name, this, "OnPresetClicked");
			preset_button.SetTag(preset_name);
			PresetList.Insert(preset_button);
		}
	}
	
	void OnPresetClicked(SCPlusButton button)
	{
		SCPlusPreset preset = SCPlusPreset.Load(button.GetTag());
		if (!preset) {
			Notify(string.Format("Couldn't load preset '%1'", button.GetTag()));
			RebuildPresetList();
			return;
		}
		
		int applied = m_SimpleCinematicsCamera.ApplyPreset(preset);
		RefreshCameraSliders();
		if (applied == 0) {
			Notify(string.Format("Preset '%1' has no saved values; set the sliders and save it again", button.GetTag()));
		} else {
			Notify(string.Format("Applied preset '%1' (%2 values)", button.GetTag(), applied));
		}
		
		SetPresetName(button.GetTag());
		m_ActivePreset = button.GetTag();
		SetPresetListOpen(false);
	}
	
	void OnPresetDropdownClicked(SCPlusButton button)
	{
		SetPresetListOpen(!m_PresetListOpen);
	}
	
	protected void SetPresetListOpen(bool open)
	{
		m_PresetListOpen = open;
		ShowSection("PresetListContent", open);
		
		string active = m_ActivePreset;
		if (active == string.Empty) {
			active = "(none)";
		}
		
		string sign = "+";
		if (open) {
			sign = "-";
		}
		
		if (m_PresetDropdown) {
			m_PresetDropdown.SetCaption(string.Format("Preset: %1   [%2]", active, sign));
		}
	}
	
	// Shows or hides a named section of the HUD layout
	protected void ShowSection(string widget_name, bool show)
	{
		if (!m_LayoutRoot) {
			return;
		}
		
		Widget section = m_LayoutRoot.FindAnyWidget(widget_name);
		if (section) {
			section.Show(show);
		}
	}
	
	void OnSavePresetClicked(SCPlusButton button)
	{
		string preset_name = SCPlusPreset.SanitizeName(PresetName);
		if (preset_name == string.Empty || preset_name.IndexOf(SCPlusPreset.INTERNAL_PREFIX) == 0) {
			Notify("Type a preset name first (letters, numbers, spaces, - and _; can't start with _)");
			return;
		}
		
		if (!SCPlusPreset.Capture(preset_name, m_SimpleCinematicsCamera).Save()) {
			Notify(string.Format("Couldn't save preset '%1'", preset_name));
			return;
		}
		
		Notify(string.Format("Saved preset '%1'", preset_name));
		m_ActivePreset = preset_name;
		RebuildPresetList();
		SetPresetListOpen(m_PresetListOpen);
	}
	
	void OnDeletePresetClicked(SCPlusButton button)
	{
		string preset_name = SCPlusPreset.SanitizeName(PresetName);
		if (preset_name == SCPlusPreset.BUILTIN_DEFAULT && !File.Exists(SCPlusPreset.GetFilePath(preset_name))) {
			Notify("The built-in Default preset can't be deleted (saving over it makes your own version)");
			return;
		}
		
		if (!SCPlusPreset.Delete(preset_name)) {
			Notify("Click a preset (or type its name) before deleting");
			return;
		}
		
		Notify(string.Format("Deleted preset '%1'", preset_name));
		SetPresetName(string.Empty);
		if (m_ActivePreset == preset_name) {
			m_ActivePreset = string.Empty;
		}
		
		RebuildPresetList();
		SetPresetListOpen(m_PresetListOpen);
	}
	
	void OnResetLookClicked(SCPlusButton button)
	{
		int applied = m_SimpleCinematicsCamera.ResetLook();
		RefreshCameraSliders();
		Notify(string.Format("Reset to defaults (%1 values), Camera Options from the Default preset", applied));
	}
	
	// Sync camera sliders after their values were changed from code (preset load, reset)
	void RefreshCameraSliders()
	{
		foreach (BindableSliderPrefab bindable_slider: m_BindableSliders) {
			if (bindable_slider && bindable_slider.GetBindingContext() == m_SimpleCinematicsCamera) {
				bindable_slider.RefreshFromBinding();
			}
		}
	}
	
	// The edit box has its own controller, so push the text into it; its binding writes PresetName back
	protected void SetPresetName(string preset_name)
	{
		PresetName = preset_name;
		if (m_PresetNameBox) {
			m_PresetNameBox.GetPrefabController().Value = preset_name;
			m_PresetNameBox.GetPrefabController().NotifyPropertyChanged("Value");
			scp_NameSetByScript = true;
		}
	}
	
	protected void Notify(string text)
	{
		m_StatusMessage = text;
		m_StatusUntil = GetGame().GetTime() + 3000;
		NotificationSystem.AddNotificationExtended(3, "Simple Cinematics", text);
	}
	
	protected string GetStatusMessage()
	{
		if (m_StatusMessage == string.Empty || GetGame().GetTime() > m_StatusUntil) {
			return string.Empty;
		}
		
		return m_StatusMessage;
	}
	
	// Used by the hotkey table in SimpleCinematicsModule
	BindableSliderPrefab GetHotkeySlider(string slider_name)
	{
		switch (slider_name) {
			case "CameraSpeedSlider": return CameraSpeedSlider;
			case "CameraSensitivitySlider": return CameraSensitivitySlider;
			case "CameraMovementSmoothness": return CameraMovementSmoothness;
			case "CameraMouseSmoothness": return CameraMouseSmoothness;
			case "CameraDOFSlider": return CameraDOFSlider;
			case "CameraFOVSlider": return CameraFOVSlider;
		}
		
		return null;
	}
	
}

modded class SimpleCinematicsHud
{
	// Same layout as Simple Cinematics, plus the Presets section
	override string GetLayoutFile()
	{
		return "SimpleCinematicsPlus/GUI/layouts/hud/hud.layout";
	}
	
	// --- Fold / unfold: click a section's title bar to hide or show its contents ---
	
	protected ref array<ref SCPlusFoldSection> m_FoldSections;
	protected bool m_FoldMouseWasDown;
	
	override void Update(float dt)
	{
		super.Update(dt);
		UpdateFoldSections();
		UpdateTypingState();
	}
	
	// Backup for SimpleCinematicsCamera.IsTypingInUI: GetFocus alone didn't stop WASD moving the camera while
	// typing a name. A click on a name box starts typing; a click anywhere else, Enter or Escape ends it.
	// The keys are edge-detected: KeyState can stay set after a press, which used to clear typing every frame.
	protected bool m_TypingMouseWasDown;
	protected bool m_TypingEndKeyWasDown;
	protected bool m_TypingTextKnown;
	protected string m_TypingLastText;
	
	protected void UpdateTypingState()
	{
		SimpleCinematicsHudController controller = GetTemplateController();
		if (!controller || !GetLayoutRoot() || !GetLayoutRoot().IsVisible() || !GetGame().GetUIManager().IsCursorVisible()) {
			SetTypingInHud(false, "hud or cursor hidden");
			return;
		}
		
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		if (mouse_down && !m_TypingMouseWasDown) {
			int mouse_x, mouse_y;
			GetMousePos(mouse_x, mouse_y);
			bool on_box = controller.SCPlusIsNameBoxAt(mouse_x, mouse_y) || IsTextBox(GetWidgetUnderCursor());
			SetTypingInHud(on_box, "click");
		}
		
		m_TypingMouseWasDown = mouse_down;
		
		// Text that changes without script setting it means the player is typing, even if the click was missed
		string name_text = controller.SCPlusGetNameBoxText();
		if (controller.scp_NameSetByScript || !m_TypingTextKnown) {
			controller.scp_NameSetByScript = false;
			m_TypingTextKnown = true;
		} else if (name_text != m_TypingLastText) {
			SetTypingInHud(true, "name text changed");
		}
		
		m_TypingLastText = name_text;
		bool end_key_down = KeyState(KeyCode.KC_RETURN) != 0 || KeyState(KeyCode.KC_NUMPADENTER) != 0 || KeyState(KeyCode.KC_ESCAPE) != 0;
		if (end_key_down && !m_TypingEndKeyWasDown) {
			SetTypingInHud(false, "enter or escape");
		}
		
		m_TypingEndKeyWasDown = end_key_down;
	}
	
	protected void SetTypingInHud(bool typing, string reason)
	{
		if (SimpleCinematicsCamera.scp_TypingInHud == typing) {
			return;
		}
		
		SimpleCinematicsCamera.scp_TypingInHud = typing;
		Print("[SCPlus Typing] " + typing + " (" + reason + ")");
	}
	
	protected bool IsTextBox(Widget widget)
	{
		while (widget) {
			if (EditBoxWidget.Cast(widget) || MultilineEditBoxWidget.Cast(widget)) {
				return true;
			}
			
			widget = widget.GetParent();
		}
		
		return false;
	}
	
	protected void InitFoldSections()
	{
		m_FoldSections = new array<ref SCPlusFoldSection>();
		AddFoldSection("PresetsFrame", "PresetsText", "Presets", "PresetHeaderContent,PresetListContent,PresetsContent");
		AddFoldSection("SequencerFrame", "SequencerText", "Sequencer", "SequenceHeaderContent,SequenceListContent,SequenceTransportContent,SequenceKeyToolsContent,SequenceSettingsContent,SequenceTogglesContent,SequenceSettingsContent2,SequenceFileContent");
		AddFoldSection("CameraOptionsFrame", "CameraOptionsText", "Camera Options", "CameraOptionsContent");
		AddFoldSection("CameraFiltersFrame", "CameraFiltersText", "Camera Filters", "CameraFiltersContent");
		AddFoldSection("CameraColorFrame", "CameraColorText", "Camera Color", "CameraColorContent");
		AddFoldSection("CameraBlurControlsFrame", "CameraBlurControlsText", "Camera Blur", "CameraBlurContent");
		AddFoldSection("WeatherControlsFrame", "WeatherControlsText", "Weather Controls", "WeatherControlsContent");
		AddFoldSection("WeatherControlsFrame0", "WeatherControlsText0", "Date Controls", "WeatherControlsContent0");
		
		AddFoldSection("GraphEditorFrame", "GraphEditorText", "Graph Editor", "GraphHeaderContent,GraphListContent");
		AddFoldSection("EventsFrame", "EventsText", "Events", "EventHeaderContent,EventListContent,EventToolsContent");
		AddFoldSection("FocusTargetFrame", "FocusTargetCaption", "Focus & Follow Target", "FocusTargetMainPanel,FollowTargetMainPanel");
	}
	
	// content_names: comma-separated widget names
	protected SCPlusFoldSection AddFoldSection(string header_name, string title_name, string title, string content_names)
	{
		array<string> names = new array<string>();
		content_names.Split(",", names);
		Widget header = GetLayoutRoot().FindAnyWidget(header_name);
		TextWidget title_widget = TextWidget.Cast(GetLayoutRoot().FindAnyWidget(title_name));
		if (!header || !title_widget) {
			return null;
		}
		
		SCPlusFoldSection section = new SCPlusFoldSection();
		section.m_Header = header;
		section.m_Title = title_widget;
		section.m_TitleText = title;
		foreach (string content_name: names) {
			Widget content = GetLayoutRoot().FindAnyWidget(content_name);
			if (content) {
				section.m_Contents.Insert(content);
			}
		}
		
		section.UpdateTitle();
		m_FoldSections.Insert(section);
		return section;
	}
	
	protected void UpdateFoldSections()
	{
		if (!GetLayoutRoot()) {
			return;
		}
		
		if (!m_FoldSections) {
			InitFoldSections();
		}
		
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = mouse_down && !m_FoldMouseWasDown;
		m_FoldMouseWasDown = mouse_down;
		if (!pressed || !GetLayoutRoot().IsVisible() || !GetGame().GetUIManager().IsCursorVisible()) {
			return;
		}
		
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		foreach (SCPlusFoldSection section: m_FoldSections) {
			if (section.IsUnderMouse(mouse_x, mouse_y)) {
				section.Toggle();
				return;
			}
		}
	}
}

// One foldable HUD section: its title bar and the widgets it hides
class SCPlusFoldSection: Managed
{
	Widget m_Header;
	TextWidget m_Title;
	string m_TitleText;
	ref array<Widget> m_Contents = new array<Widget>();
	ref array<bool> m_WasVisible = new array<bool>();
	bool m_Folded;
	
	bool IsUnderMouse(int mouse_x, int mouse_y)
	{
		if (!m_Header || !m_Header.IsVisible()) {
			return false;
		}
		
		float x, y, width, height;
		m_Header.GetScreenPos(x, y);
		m_Header.GetScreenSize(width, height);
		return mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height;
	}
	
	// Folding remembers which parts were showing (e.g. a closed dropdown list stays closed on unfold)
	void Toggle()
	{
		m_Folded = !m_Folded;
		if (m_Folded) {
			m_WasVisible.Clear();
			foreach (Widget content: m_Contents) {
				m_WasVisible.Insert(content.IsVisible());
				content.Show(false);
			}
		} else {
			foreach (int i, Widget folded_content: m_Contents) {
				bool show = true;
				if (m_WasVisible.IsValidIndex(i)) {
					show = m_WasVisible[i];
				}
				
				folded_content.Show(show);
			}
		}
		
		UpdateTitle();
	}
	
	void UpdateTitle()
	{
		string sign = "-";
		if (m_Folded) {
			sign = "+";
		}
		
		m_Title.SetText(string.Format("%1   [%2]", m_TitleText, sign));
	}
}
