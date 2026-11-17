// A Dabs ButtonPrefab that calls a function when clicked.
// Constructed like any prefab: caption, the object to call, and the name of the function.
// The function receives this button, so callers can read GetCaption() or GetTag().
class SCPlusButton: ButtonPrefab
{
	protected string m_Tag;
	
	void SetTag(string tag)
	{
		m_Tag = tag;
	}
	
	string GetTag()
	{
		return m_Tag;
	}
	
	string GetCaption()
	{
		return m_PrefabBaseController.Caption;
	}
	
	void SetCaption(string caption)
	{
		m_PrefabBaseController.Caption = caption;
		m_PrefabBaseController.NotifyPropertyChanged("Caption", false);
	}
	
	// Relay_Command from DabsFramework/gui/Layouts/prefabs/ButtonPrefab.layout
	bool ButtonExecute(ButtonCommandArgs args)
	{
		if (m_BindingContext && m_BindingName != string.Empty) {
			g_Script.Call(m_BindingContext, m_BindingName, this);
		}
		
		// The prefab's ButtonWidget is two-way bound to Value, so every click latches it "on" (highlighted).
		// Release it once the click has been fully processed.
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Call(ReleaseButton);
		return true;
	}
	
	void ~SCPlusButton()
	{
		if (GetGame()) {
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ReleaseButton);
		}
	}
	
	protected void ReleaseButton()
	{
		m_PrefabBaseController.Value = false;
		m_PrefabBaseController.NotifyPropertyChanged("Value");
		
		if (GetLayoutRoot()) {
			ButtonWidget button_widget = ButtonWidget.Cast(GetLayoutRoot().FindAnyWidget("Button"));
			if (button_widget) {
				button_widget.SetState(false);
			}
		}
		
		SetFocus(null);
	}
	
	// The binding name is a function, not a variable, so don't write anything back
	override void PrefabPropertyChanged(string property_name)
	{
	}
	
	override bool GetDefaultValue(Class binding_context, string binding_name)
	{
		return false;
	}
}

// Full-width version, used for list rows (the key list in the left panel)
class SCPlusWideButton: SCPlusButton
{
	override string GetLayoutFile()
	{
		return "SimpleCinematicsPlus/GUI/layouts/prefabs/SCPlusWideButton.layout";
	}
}

// Dropdown field (Presets, Sequence, Channel, Event Type): a dark combo box with the label and the current
// choice on the left and an arrow on the right. Clicking it opens or closes its SCPlusDropdownItem list.
class SCPlusDropdownButton: SCPlusWideButton
{
	override string GetLayoutFile()
	{
		return "SimpleCinematicsPlus/GUI/layouts/prefabs/SCPlusDropdownHeader.layout";
	}
	
	static string MakeCaption(string label, string value)
	{
		return string.Format("  %1:   %2", label, value);
	}
}

// One row of an open dropdown list: compact, left aligned, the current choice marked
class SCPlusDropdownItem: SCPlusWideButton
{
	override string GetLayoutFile()
	{
		return "SimpleCinematicsPlus/GUI/layouts/prefabs/SCPlusDropdownItem.layout";
	}
	
	static string MakeCaption(string text, bool current)
	{
		if (current) {
			return "  >  " + text;
		}
		
		return "      " + text;
	}
}
