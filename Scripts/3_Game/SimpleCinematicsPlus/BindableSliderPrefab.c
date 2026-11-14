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
}
