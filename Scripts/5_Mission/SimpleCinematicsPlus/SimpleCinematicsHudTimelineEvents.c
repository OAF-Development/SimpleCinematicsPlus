// Event track UI (must compile after SimpleCinematicsHudSequencer.c and SimpleCinematicsHudTimeline.c):
//  - its own "Events" HUD section: an "Event Type" dropdown listing every event type, then Add Event
//    (at the playhead) and Delete Event; the waypoint buttons go in the "Waypoint Editor" section
//  - the timeline bar shows events as small orange markers under the key track; click one to select it and
//    jump to it, drag it to retime it (0.1 s snapping, Ctrl for none); Delete removes the selected event
//  - "Emote (current)" stores whatever emote the player is playing now, so start it from the wheel first

modded class SimpleCinematicsHudController
{
	// The types offered by the "Add Event" button, in click order
	static const int SCPLUS_EVENT_CHOICES = 16;
	
	protected int m_SelectedEvent = -1;
	protected int m_EventChoice;
	protected ref SCPlusDropdownButton m_EventTypeButton;
	protected ref array<SCPlusDropdownItem> m_EventRows = new array<SCPlusDropdownItem>();
	protected bool m_EventListOpen;
	protected bool m_EventDragUndoPushed;
	
	// "Events" section: the event type dropdown, its list, and Add / Delete Event
	ref ObservableCollection<ref ScriptView> EventHeader = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> EventList = new ObservableCollection<ref ScriptView>(this);
	ref ObservableCollection<ref ScriptView> EventTools = new ObservableCollection<ref ScriptView>(this);
	
	override void SetupSequencer()
	{
		super.SetupSequencer();
		
		m_EventTypeButton = new SCPlusDropdownButton("Event Type", this, "OnEventTypeClicked");
		EventHeader.Insert(m_EventTypeButton);
		for (int event_choice = 0; event_choice < SCPLUS_EVENT_CHOICES; event_choice++) {
			SCPlusDropdownItem event_row = new SCPlusDropdownItem(GetEventChoiceName(event_choice), this, "OnEventChoiceClicked");
			event_row.SetTag(event_choice.ToString());
			EventList.Insert(event_row);
			m_EventRows.Insert(event_row);
		}
		
		EventTools.Insert(new SCPlusButton("Add Event", this, "OnAddEventClicked"));
		EventTools.Insert(new SCPlusButton("Delete Event", this, "OnDeleteEventClicked"));
		EventTools.Insert(new SCPlusButton("Set Actor Start", this, "OnSetActorStartClicked"));
		EventTools.Insert(new SCPlusButton("Set Target Start", this, "OnSetTargetStartClicked"));
		// Waypoint Editor section
		WaypointTools.Insert(new SCPlusButton("Delete Point", this, "OnDeleteLastWaypointClicked"));
		WaypointTools.Insert(new SCPlusButton("Clear Path", this, "OnClearPathClicked"));
		SetEventListOpen(false);
	}
	
	// Public wrapper so the module can show event runner problems
	void ShowMessage(string text)
	{
		Notify(text);
	}
	
	int GetSelectedEvent()
	{
		return m_SelectedEvent;
	}
	
	bool HasSelectedEvent()
	{
		return m_Sequence && m_Sequence.scp_Events.IsValidIndex(m_SelectedEvent);
	}
	
	override protected void LoadSequence(notnull SCPlusCameraSequence sequence)
	{
		m_SelectedEvent = -1;
		super.LoadSequence(sequence);
	}
	
	// Selecting a key deselects the event
	override void JumpToKey(int index)
	{
		m_SelectedEvent = -1;
		super.JumpToKey(index);
	}
	
	protected string GetEventChoiceName(int choice)
	{
		switch (choice) {
			case 0: return "Fire 1 shot";
			case 1: return "Fire 3-shot burst";
			case 2: return "Fire 10 shots";
			case 3: return "Raise weapon";
			case 4: return "Lower weapon";
			case 5: return "Emote (current)";
			case 6: return "Stop emote";
			case 7: return "Walk 3 s";
			case 8: return "Jog 3 s";
			case 9: return "Sprint 3 s";
			case 10: return "Crouch";
			case 11: return "Stand";
			case 12: return "Prone";
			case 13: return "Walk path";
			case 14: return "Jog path";
			case 15: return "Sprint path";
}
		
		return "-";
	}
	
	// Builds the event for a menu choice; null (with a message) if it can't be made right now
	protected SCPlusEvent CreateEventForChoice(int choice)
	{
		SCPlusEvent event_key = new SCPlusEvent();
		switch (choice) {
			case 0: {
				event_key.scp_Type = SCPlusEventType.FIRE;
				event_key.scp_Count = 1;
				break;
			}
			
			case 1: {
				event_key.scp_Type = SCPlusEventType.FIRE;
				event_key.scp_Count = 3;
				event_key.scp_Interval = 0.09;
				break;
			}
			
			case 2: {
				event_key.scp_Type = SCPlusEventType.FIRE;
				event_key.scp_Count = 10;
				event_key.scp_Interval = 0.1;
				break;
			}
			
			case 3: {
				event_key.scp_Type = SCPlusEventType.RAISE;
				break;
			}
			
			case 4: {
				event_key.scp_Type = SCPlusEventType.LOWER;
				break;
			}
			
			case 5: {
				PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
				int emote_id;
				if (player && player.GetEmoteManager()) {
					emote_id = player.GetEmoteManager().SCPlusGetCurrentEmote();
				}
				
				if (emote_id <= 0) {
					Notify("Start the emote from the emote wheel first, then add the event");
					return null;
				}
				
				event_key.scp_Type = SCPlusEventType.EMOTE;
				event_key.scp_EmoteId = emote_id;
				break;
			}
			
			case 6: {
				event_key.scp_Type = SCPlusEventType.STOP_EMOTE;
				break;
			}
			
			case 7: {
				event_key.scp_Type = SCPlusEventType.MOVE;
				event_key.scp_Speed = 1;
				event_key.scp_Duration = 3;
				break;
			}
			
			case 8: {
				event_key.scp_Type = SCPlusEventType.MOVE;
				event_key.scp_Speed = 2;
				event_key.scp_Duration = 3;
				break;
			}
			
			case 9: {
				event_key.scp_Type = SCPlusEventType.MOVE;
				event_key.scp_Speed = 3;
				event_key.scp_Duration = 3;
				break;
			}
			
			case 10: {
				event_key.scp_Type = SCPlusEventType.STANCE;
				event_key.scp_Stance = SCPlusStance.CROUCH;
				break;
			}
			
			case 11: {
				event_key.scp_Type = SCPlusEventType.STANCE;
				event_key.scp_Stance = SCPlusStance.ERECT;
				break;
			}
			
			case 12: {
				event_key.scp_Type = SCPlusEventType.STANCE;
				event_key.scp_Stance = SCPlusStance.PRONE;
				break;
			}
			
			case 13: {
				event_key.scp_Type = SCPlusEventType.PATH;
				event_key.scp_Speed = 1;
				break;
			}
			
			case 14: {
				event_key.scp_Type = SCPlusEventType.PATH;
				event_key.scp_Speed = 2;
				break;
			}
			
			case 15: {
				event_key.scp_Type = SCPlusEventType.PATH;
				event_key.scp_Speed = 3;
				break;
			}
}
		
		return event_key;
	}
	
	void AddEventAtPlayhead()
	{
		SCPlusEvent event_key = CreateEventForChoice(m_EventChoice);
		if (!event_key) {
			return;
		}
		
		PushUndo();
		event_key.scp_Time = m_SimpleCinematicsCamera.GetPlayhead();
		m_SelectedEvent = m_Sequence.AddEvent(event_key);
		Notify(string.Format("Added \"%1\" at %2s", event_key.GetCaption(), FormatSeconds(event_key.scp_Time)));
		OnSequenceEdited();
		SCPlusWarnIfClash(m_SelectedEvent);
	}
	
	void DeleteSelectedEvent()
	{
		if (!HasSelectedEvent()) {
			Notify("Click an event marker on the timeline to select it first");
			return;
		}
		
		PushUndo();
		m_Sequence.scp_Events.RemoveOrdered(m_SelectedEvent);
		m_SelectedEvent = -1;
		OnSequenceEdited();
	}
	
	// Timeline: clicking an event marker selects it and moves the playhead to it
	void SelectEvent(int index)
	{
		if (!m_Sequence.scp_Events.IsValidIndex(index)) {
			return;
		}
		
		m_SelectedEvent = index;
		m_SelectedKey = -1;
		m_SimpleCinematicsCamera.Pause();
		m_SimpleCinematicsCamera.SetPlayhead(m_Sequence.scp_Events[index].scp_Time);
		RefreshCameraSliders();
		OnSelectionChanged();
		Notify(string.Format("Event: %1 at %2s", m_Sequence.scp_Events[index].GetCaption(), FormatSeconds(m_Sequence.scp_Events[index].scp_Time)));
	}
	
	// Timeline: dragging an event marker
	void DragEvent(int index, float time)
	{
		if (!m_Sequence.scp_Events.IsValidIndex(index)) {
			return;
		}
		
		// One undo step per drag, taken on its first move
		if (!m_EventDragUndoPushed) {
			PushUndo();
			m_EventDragUndoPushed = true;
		}
		
		m_SelectedEvent = m_Sequence.MoveEvent(index, Math.Max(time, 0));
	}
	
	void FinishEventDrag()
	{
		m_EventDragUndoPushed = false;
		OnSequenceEdited();
		SCPlusWarnIfClash(m_SelectedEvent);
	}
	
	// Tells the user when an event will cut another short or be skipped (also drawn red on the timeline)
	void SCPlusWarnIfClash(int index)
	{
		if (!m_Sequence) {
			return;
		}
		
		int other = m_Sequence.FindClash(index);
		if (other == -1) {
			return;
		}
		
		SCPlusEvent clash = m_Sequence.scp_Events[other];
		Notify(string.Format("Clashes with \"%1\" at %2s: one of them may be cut short or skipped", clash.GetCaption(), FormatSeconds(clash.scp_Time)));
	}
	
	override protected void SCPlusOnUndoRestored()
	{
		if (!m_Sequence.scp_Events.IsValidIndex(m_SelectedEvent)) {
			m_SelectedEvent = -1;
		}
	}
	
	protected void UpdateEventCaption()
	{
		if (m_EventTypeButton) {
			m_EventTypeButton.SetCaption(DropdownCaption("Event Type", GetEventChoiceName(m_EventChoice), m_EventListOpen));
		}
		
		foreach (SCPlusDropdownItem row: m_EventRows) {
			int row_choice = row.GetTag().ToInt();
			row.SetCaption(MarkCurrentChoice(GetEventChoiceName(row_choice), row_choice == m_EventChoice));
		}
	}
	
	// Events section: the dropdown opens the event type list below it; picking a row selects that type
	void OnEventTypeClicked(SCPlusButton button)
	{
		SetEventListOpen(!m_EventListOpen);
	}
	
	void OnEventChoiceClicked(SCPlusButton button)
	{
		m_EventChoice = button.GetTag().ToInt();
		SetEventListOpen(false);
	}
	
	protected void SetEventListOpen(bool open)
	{
		m_EventListOpen = open;
		ShowSection("EventListContent", open);
		UpdateEventCaption();
	}
	
	void OnAddEventClicked(SCPlusButton button)
	{
		AddEventAtPlayhead();
	}
	
	// Remembers where the player stands now; Stop and each loop put them back here
	void OnSetActorStartClicked(SCPlusButton button)
	{
		if (!m_SimpleCinematicsCamera || !m_SimpleCinematicsCamera.SetActorStart()) {
			Notify("No player to set the actor start from");
			return;
		}
		
		Notify("Actor start set here (save the sequence to keep it)");
	}
	
	// Remembers where the Follow / Focus target stands now; Play from the start, Stop and each loop put it back
	void OnSetTargetStartClicked(SCPlusButton button)
	{
		if (!m_SimpleCinematicsCamera || !m_Sequence) {
			return;
		}
		
		if (m_SimpleCinematicsCamera.SetTargetStart()) {
			Notify(string.Format("Target start set: %1 (save the sequence to keep it)", m_Sequence.scp_TargetType));
			return;
		}
		
		// No target picked: a second click clears the saved one
		if (m_Sequence.scp_HasTargetStart) {
			m_SimpleCinematicsCamera.ClearTargetStart();
			Notify("No target picked: target start cleared");
			return;
		}
		
		Notify("Pick an animal, vehicle or player with Follow or Focus first");
	}
	
	void OnDeleteLastWaypointClicked(SCPlusButton button)
	{
		if (!m_SimpleCinematicsCamera || !m_SimpleCinematicsCamera.RemoveWaypoint()) {
			Notify("No waypoints to delete");
			return;
		}
		
		SCPlusSyncWaypointCurve();
	}
	
	void OnClearPathClicked(SCPlusButton button)
	{
		if (m_SimpleCinematicsCamera) {
			m_SimpleCinematicsCamera.ClearWaypoints();
			Notify("Path cleared");
		}
	}
	
	void OnDeleteEventClicked(SCPlusButton button)
	{
		DeleteSelectedEvent();
	}
}

modded class SimpleCinematicsHud
{
	static const int SCPLUS_COLOR_EVENT = 0xFFFF8C32;
	static const int SCPLUS_COLOR_EVENT_SELECTED = 0xFF32C8FF;
	static const float SCPLUS_EVENT_LANE_OFFSET = 14; // px below the key track
	
	protected bool m_EventMouseWasDown;
	protected int m_EventDragIndex = -1;
	protected bool m_EventDragMoved;
	protected int m_EventDragStartX;
	
	override protected void UpdateTimelineMouse(SimpleCinematicsHudController controller)
	{
		bool mouse_down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = mouse_down && !m_EventMouseWasDown;
		m_EventMouseWasDown = mouse_down;
		
		SCPlusCameraSequence sequence = controller.GetSequence();
		float length = sequence.GetLength();
		float panel_x, panel_y, width, height;
		m_TimelinePanel.GetScreenPos(panel_x, panel_y);
		m_TimelinePanel.GetScreenSize(width, height);
		int mouse_x, mouse_y;
		GetMousePos(mouse_x, mouse_y);
		float local_x = mouse_x - panel_x;
		float local_y = mouse_y - panel_y;
		
		// An event drag in progress owns the mouse until the button is released
		if (m_EventDragIndex != -1) {
			if (!mouse_down) {
				if (m_EventDragMoved) {
					controller.FinishEventDrag();
				}
				
				m_EventDragIndex = -1;
				return;
			}
			
			if (!m_EventDragMoved && Math.AbsInt(mouse_x - m_EventDragStartX) < 4) {
				return;
			}
			
			m_EventDragMoved = true;
			float time = TimelineXToTime(local_x, length, width);
			if (!KeyState(KeyCode.KC_LCONTROL)) {
				time = Math.Round(time * 10) / 10;
			}
			
			controller.DragEvent(m_EventDragIndex, time);
			m_EventDragIndex = controller.GetSelectedEvent();
			return;
		}
		
		// A new click on an event marker starts an event click / drag instead of a scrub
		bool in_lane = local_x >= 0 && local_x <= width && local_y >= height * 0.62 + 8 && local_y <= height;
		if (pressed && in_lane && GetGame().GetUIManager().IsCursorVisible()) {
			int hit = FindTimelineEventAt(sequence, local_x, length, width);
			if (hit != -1) {
				controller.SelectEvent(hit);
				m_EventDragIndex = controller.GetSelectedEvent();
				m_EventDragMoved = false;
				m_EventDragStartX = mouse_x;
				return;
			}
		}
		
		super.UpdateTimelineMouse(controller);
	}
	
	protected int FindTimelineEventAt(SCPlusCameraSequence sequence, float local_x, float length, float width)
	{
		int best = -1;
		float best_distance = SCPLUS_KEY_HIT;
		foreach (int i, SCPlusEvent event_key: sequence.scp_Events) {
			float distance = Math.AbsFloat(TimelineTimeToX(event_key.scp_Time, length, width) - local_x);
			if (distance <= best_distance) {
				best = i;
				best_distance = distance;
			}
		}
		
		return best;
	}
	
	override protected void DrawTimeline(SimpleCinematicsHudController controller)
	{
		super.DrawTimeline(controller);
		
		SCPlusCameraSequence sequence = controller.GetSequence();
		if (sequence.scp_Events.Count() == 0) {
			return;
		}
		
		float length = sequence.GetLength();
		float width, height;
		m_TimelinePanel.GetScreenSize(width, height);
		float lane_y = height * 0.62 + SCPLUS_EVENT_LANE_OFFSET;
		float left = SCPLUS_TIMELINE_MARGIN;
		float right = width - SCPLUS_TIMELINE_MARGIN;
		int selected = controller.GetSelectedEvent();
		
		foreach (int i, SCPlusEvent event_key: sequence.scp_Events) {
			float x = TimelineTimeToX(event_key.scp_Time, length, width);
			if (x < left - 1 || x > right + 1) {
				continue;
			}
			
			int color = SCPLUS_COLOR_EVENT;
			if (i == selected) {
				color = SCPLUS_COLOR_EVENT_SELECTED;
			}
			
			// Small upward triangle, pointing at the time on the key track
			for (float row = 0; row <= 5; row += 1) {
				m_TimelineCanvas.DrawLine(x - row - 0.5, lane_y - 3 + row, x + row + 0.5, lane_y - 3 + row, 1, color);
			}
		}
	}
}
