modded class BindableSliderPrefab
{
	// Re-reads the bound variable, e.g. after a preset changed it behind the slider's back
	void RefreshFromBinding()
	{
		// GetClassVar's return value isn't checked: it doesn't appear to be a success flag, and if it isn't,
		// checking it made this return early so sliders never showed values set by presets or Reset Look
		float value;
		EnScript.GetClassVar(m_BindingContext, m_BindingName, 0, value);
		
		m_PrefabBaseController.CalculatedValue = value;
		m_PrefabBaseController.NotifyPropertyChanged("CalculatedValue");
	}
	
	// Changes the slider's range (e.g. the Time slider following the sequence length)
	void SetRange(float min, float max)
	{
		m_Min = min;
		m_Max = Math.Max(max, min + 0.01);
		RefreshFromBinding();
	}
	
	// True while the box next to the slider is ticked (it's then the arrow-key "quick slider")
	bool IsQuickSliderActive()
	{
		return m_IsBinding;
	}
	
	// Camera Speed: while the HUD is shown the slider only changes with its box ticked, so a stray drag, arrow
	// click or wheel notch over the panel can't change the speed mid-shot. With the HUD hidden it's unlocked, so
	// the mouse wheel still changes the speed while shooting live. scp_LockActive is set every frame by the module.
	bool scp_RequireTick;
	static bool scp_LockActive;
	
	bool IsLockedByTick()
	{
		return scp_RequireTick && scp_LockActive && !m_IsBinding;
	}
	
	override void PrefabPropertyChanged(string property_name)
	{
		if (property_name == "Value" && IsLockedByTick()) {
			// Put the slider back on the bound value without writing anything to the camera. The notifies don't
			// go back to the controller, so this doesn't loop.
			float bound;
			EnScript.GetClassVar(m_BindingContext, m_BindingName, 0, bound);
			m_PrefabBaseController.CalculatedValue = bound;
			m_PrefabBaseController.NotifyPropertyChanged("CalculatedValue", false);
			m_PrefabBaseController.Value = Math.InverseLerp(m_Min, m_Max, bound);
			m_PrefabBaseController.NotifyPropertyChanged("Value", false);
			return;
		}
		
		super.PrefabPropertyChanged(property_name);
	}
}
