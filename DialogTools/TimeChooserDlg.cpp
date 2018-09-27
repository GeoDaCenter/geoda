/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/TableState.h"
#include "../logger.h"
#include "../DataViewer/TableInterface.h"
#include "TimeChooserDlg.h"

BEGIN_EVENT_TABLE( TimeChooserDlg, wxDialog )
	EVT_CLOSE( TimeChooserDlg::OnClose )
	EVT_SLIDER( XRCID("ID_SLIDER"), TimeChooserDlg::OnMoveSlider )
	EVT_SLIDER( XRCID("ID_SPEED_SLIDER"), TimeChooserDlg::OnMoveSpeedSlider )
	EVT_BUTTON( XRCID("ID_PLAY"), TimeChooserDlg::OnPlayPauseButton )
	EVT_BUTTON( XRCID("ID_STEP_FORWARD"), TimeChooserDlg::OnStepForwardButton )
	EVT_BUTTON( XRCID("ID_STEP_BACK"), TimeChooserDlg::OnStepBackButton )
	EVT_RADIOBUTTON( XRCID("ID_SLOWER"), TimeChooserDlg::OnSpeedSlowerButton )
	EVT_RADIOBUTTON( XRCID("ID_SLOW"), TimeChooserDlg::OnSpeedSlowButton )
	EVT_RADIOBUTTON( XRCID("ID_MEDIUM"), TimeChooserDlg::OnSpeedMediumButton )
	EVT_RADIOBUTTON( XRCID("ID_FAST"), TimeChooserDlg::OnSpeedFastButton )
	EVT_RADIOBUTTON( XRCID("ID_FASTER"), TimeChooserDlg::OnSpeedFasterButton )
	EVT_CHECKBOX( XRCID("ID_LOOP"), TimeChooserDlg::OnLoopCheckBox )
	EVT_CHECKBOX( XRCID("ID_REVERSE"), TimeChooserDlg::OnReverseCheckBox )
	EVT_CHAR_HOOK( TimeChooserDlg::OnKeyEvent )
END_EVENT_TABLE()

TimeChooserTimer::TimeChooserTimer() : time_chooser_dlg(0)
{
}

TimeChooserTimer::TimeChooserTimer(TimeChooserDlg* dlg) :
	time_chooser_dlg(dlg)
{
}

TimeChooserTimer::~TimeChooserTimer()
{
	time_chooser_dlg = 0;
}

void TimeChooserTimer::Notify() {
	if (time_chooser_dlg) time_chooser_dlg->TimerCall();
}

TimeChooserDlg::TimeChooserDlg(wxWindow* parent,
							   FramesManager* frames_manager_s,
							   TimeState* time_state_s,
							   TableState* table_state_s,
							   TableInterface* table_int_s)
: frames_manager(frames_manager_s), time_state(time_state_s),
table_state(table_state_s), table_int(table_int_s),
all_init(false), suspend_notify(false),
suspend_update(false), playing(false), timer(0), delay_ms(1000),
loop(true), forward(true)
{
	wxLogMessage("Open TimeChooserDlg.");
	SetParent(parent);
	wxXmlResource::Get()->LoadDialog(this, GetParent(), "ID_TIME_CHOOSER_DLG");
    SetBackgroundColour(*wxWHITE);
    
	play_button = wxDynamicCast(FindWindow(XRCID("ID_PLAY")), wxButton);
    forward_button = wxDynamicCast(FindWindow(XRCID("ID_STEP_FORWARD")), wxButton);
    backward_button = wxDynamicCast(FindWindow(XRCID("ID_STEP_BACK")), wxButton);
	slider = wxDynamicCast(FindWindow(XRCID("ID_SLIDER")), wxSlider);
	speed_slider = wxDynamicCast(FindWindow(XRCID("ID_SPEED_SLIDER")),
								 wxSlider);
	{
		int sval = 100-(speed_slider->GetValue());
		sval = sval*sval/100;
		delay_ms = min_delay_ms + ((max_delay_ms-min_delay_ms)*sval)/100;	
	}
	cur_txt = wxDynamicCast(FindWindow(XRCID("ID_CUR_TXT")), wxStaticText);
	loop_cb = wxDynamicCast(FindWindow(XRCID("ID_LOOP")), wxCheckBox);
	reverse_cb = wxDynamicCast(FindWindow(XRCID("ID_REVERSE")), wxCheckBox);
	
	int steps = table_int->GetTimeSteps();	
	wxString t_cur;
	t_cur << time_state->GetCurrTimeString();
	cur_txt->SetLabelText(t_cur);
	slider_val = time_state->GetCurrTime();
	
	suspend_notify = true;
	slider->SetRange(0, steps-1);
	slider->SetValue(time_state->GetCurrTime());
	suspend_notify = false;
	
	speed_slider->GetValue();
	
	frames_manager->registerObserver(this);
	time_state->registerObserver(this);
	table_state->registerObserver(this);
	SetMinSize(wxSize(100,50));
	all_init = true;
    
    ToggleButtons( table_int->IsTimeVariant() && steps > 0 ? true : false);
    
}

TimeChooserDlg::~TimeChooserDlg()
{
	if (timer) delete timer; 
	frames_manager->removeObserver(this);
	time_state->removeObserver(this);
	table_state->removeObserver(this);
}

void TimeChooserDlg::ToggleButtons(bool enabled)
{
    play_button->Enable(enabled);
    slider->Enable(enabled);
    speed_slider->Enable(enabled);
    loop_cb->Enable(enabled);
    reverse_cb->Enable(enabled);
    speed_slider->Enable(enabled);
    forward_button->Enable(enabled);
    backward_button->Enable(enabled);
}

void TimeChooserDlg::OnClose(wxCloseEvent& ev)
{
	wxLogMessage("Close TimeChooserDlg::OnClose");
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

void TimeChooserDlg::OnMoveSlider(wxCommandEvent& ev)
{
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int slider_val = slider->GetValue();
	if (slider_val == GetGridBaseTimeStep()) return;
	SetCurTxt(slider_val);
	time_state->SetCurrTime(slider_val);
	if (!suspend_notify) {
		suspend_update = true;
		time_state->notifyObservers();
		suspend_update = false;
	}
	Refresh();
}

void TimeChooserDlg::OnMoveSpeedSlider(wxCommandEvent& ev)
{
	if (!all_init) return;
	UpdateDelayFromSlider();
}

void TimeChooserDlg::ChangeTime(int new_time)
{
	wxLogMessage("In TimeChooserDlg::ChangeTime");
	if (!all_init) return;
	LOG(new_time);
	slider_val = new_time;
	slider->SetValue(slider_val);
	SetCurTxt(slider_val);
	time_state->SetCurrTime(slider_val);
	if (!suspend_notify) {
		suspend_update = true;
		time_state->notifyObservers();
		suspend_update = false;
	}
	Refresh();
}

void TimeChooserDlg::OnPlayPauseButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnPlayPauseButton");
	if (!all_init) return;
	if (playing) {
		// stop playing
		playing = false;
		play_button->SetLabel(">");
		Refresh();
		if (timer) {
			timer->Stop();
			delete timer;
			timer = 0;
		}
	} else {
		// start playing
		int new_slider_val;
		if (forward) {
			new_slider_val = GetSliderTimeStep()+1;
			if (new_slider_val == GetTotalTimeSteps()) new_slider_val=0;
		} else {
			new_slider_val = GetSliderTimeStep()-1;
			if (new_slider_val < 0) new_slider_val = GetTotalTimeSteps()-1;
		}
		ChangeTime(new_slider_val);
		
		playing = true;
		play_button->SetLabel("||");
		Refresh();
		if (!timer) timer = new TimeChooserTimer(this);
		timer->Start(delay_ms);
	}
}

void TimeChooserDlg::OnStepForwardButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnStepForwardButton");
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int new_slider_val = GetSliderTimeStep()+1;
	if (new_slider_val == GetTotalTimeSteps()) new_slider_val=0;
	ChangeTime(new_slider_val);
}

void TimeChooserDlg::OnStepBackButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnStepBackButton");
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int new_slider_val = GetSliderTimeStep()-1;
	if (new_slider_val < 0) new_slider_val = GetTotalTimeSteps()-1;
	ChangeTime(new_slider_val);
}

void TimeChooserDlg::OnSpeedSlowerButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnSpeedSlowerButton");
	if (!all_init) return;
	delay_ms = 3200;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedSlowButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnSpeedSlowButton");
	if (!all_init) return;
	delay_ms = 2200;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedMediumButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnSpeedMediumButton");
	if (!all_init) return;
	delay_ms = 1500;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedFastButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnSpeedFastButton");
	if (!all_init) return;
	delay_ms = 700;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedFasterButton(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnSpeedFasterButton");
	if (!all_init) return;
	delay_ms = 350;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::ChangeSpeed(int delay_ms)
{
	if (!all_init) return;
	if (timer && timer->IsRunning()) {
		timer->Start(delay_ms);
	}
}

void TimeChooserDlg::OnReverseCheckBox(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnReverseCheckBox");
	if (!all_init) return;
	forward = (reverse_cb->GetValue() == 0);
}

void TimeChooserDlg::OnLoopCheckBox(wxCommandEvent& ev)
{
	wxLogMessage("In TimeChooserDlg::OnLoopCheckBox");
	if (!all_init) return;
	loop = (loop_cb->GetValue() == 1);
}

int TimeChooserDlg::GetSliderTimeStep()
{
	if (!all_init) return GetGridBaseTimeStep();
	return slider->GetValue();
}

int TimeChooserDlg::GetGridBaseTimeStep()
{
	return time_state->GetCurrTime();
}

int TimeChooserDlg::GetTotalTimeSteps()
{
	return table_int->GetTimeSteps();
}

void TimeChooserDlg::SetCurTxt(wxInt64 val)
{
	if (!all_init) return;
	wxString t_val;
	t_val << table_int->GetTimeString(val);
	cur_txt->SetLabelText(t_val);
}

void TimeChooserDlg::UpdateDelayFromSlider()
{
	if (!all_init) return;
	int sval = 100-(speed_slider->GetValue());
	sval = sval*sval/100;
	delay_ms = min_delay_ms + ((max_delay_ms-min_delay_ms)*sval)/100;
	ChangeSpeed(delay_ms);	
}

void TimeChooserDlg::TimerCall()
{
	if (!playing) return;
	wxCommandEvent ev;
	int new_time_step;
	if (forward) {
		new_time_step = GetSliderTimeStep() + 1;
		if (!loop && new_time_step >= GetTotalTimeSteps() - 1) {
			OnPlayPauseButton(ev);
			ChangeTime(GetTotalTimeSteps() - 1);
			return;
		}
		if (loop && new_time_step >= GetTotalTimeSteps()) {
			new_time_step = 0;
		}
	} else {
		new_time_step = GetSliderTimeStep() - 1;
		if (!loop && new_time_step <= 0) {
			OnPlayPauseButton(ev);
			ChangeTime(0);
			return;
		}
		if (loop && new_time_step <= -1) {
			new_time_step = GetTotalTimeSteps() - 1;
		}
	}
	ChangeTime(new_time_step);
}

void TimeChooserDlg::update(FramesManager* o)
{	
}

void TimeChooserDlg::update(TimeState* o)
{
	if (suspend_update) return;
    
	suspend_notify = true;
	SetCurTxt(o->GetCurrTime());
	slider->SetValue(o->GetCurrTime());
	slider->Refresh();
	suspend_notify = false;
	Refresh();
}

void TimeChooserDlg::update(TableState* o)
{
	if (o->GetEventType() != TableState::time_ids_add_remove &&
		o->GetEventType() != TableState::time_ids_rename &&
		o->GetEventType() != TableState::time_ids_swap &&
        o->GetEventType() != TableState::cols_delta) return;
	
	int steps = table_int->GetTimeSteps();
    bool is_time = table_int->IsTimeVariant();
    
    if (steps > 1 && is_time) ToggleButtons(true);
    if (steps <=1 || !is_time) ToggleButtons(false);
    
    if (steps > 0) {
        wxString t_cur;
        t_cur << time_state->GetCurrTimeString();
        cur_txt->SetLabelText(t_cur);
        slider_val = time_state->GetCurrTime();
        
        suspend_notify = true;
        slider->SetRange(0, steps-1);
        slider->SetValue(time_state->GetCurrTime());
        suspend_notify = false;
        
        speed_slider->GetValue();
    }
	
	Refresh();
}

void TimeChooserDlg::OnKeyEvent(wxKeyEvent& event)
{
	wxLogMessage("In TimeChooserDlg::OnKeyEvent");
	if (event.GetModifiers() == wxMOD_CMD &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		LOG(del);
		time_state->SetCurrTime(time_state->GetCurrTime() + del);
		if (time_state->GetCurrTime() < 0) {
			time_state->SetCurrTime(table_int->GetTimeSteps()-1);
		} else if (time_state->GetCurrTime() >= table_int->GetTimeSteps()) {
			time_state->SetCurrTime(0);
		}
		time_state->notifyObservers();
		return;
	}
	event.Skip();
}
