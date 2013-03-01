/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <wx/sizer.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../logger.h"
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
	LOG_MSG("In TimeChooserTimer::TimeChooserTimer");
}

TimeChooserTimer::TimeChooserTimer(TimeChooserDlg* dlg) :
	time_chooser_dlg(dlg)
{
	LOG_MSG("In TimeChooserTimer::TimeChooserTimer");
}

TimeChooserTimer::~TimeChooserTimer()
{
	LOG_MSG("In TimeChooserTimer::~TimeChooserTimer");
	time_chooser_dlg = 0;
}

void TimeChooserTimer::Notify() {
	LOG_MSG("In TimeChooserTimer::Notify");
	if (time_chooser_dlg) time_chooser_dlg->TimerCall();
}

TimeChooserDlg::TimeChooserDlg(wxWindow* parent,
							   FramesManager* frames_manager_s,
							   DbfGridTableBase* grid_base_s)
: frames_manager(frames_manager_s), grid_base(grid_base_s),
all_init(false), suspend_notify(false), suspend_update(false),
playing(false), timer(0), delay_ms(1000),
loop(true), forward(true)
{
	LOG_MSG("Entering TimeChooserDlg::TimeChooserDlg");
	SetParent(parent);
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_TIME_CHOOSER_DLG");
	play_button = wxDynamicCast(FindWindow(XRCID("ID_PLAY")), wxButton);
	slider = wxDynamicCast(FindWindow(XRCID("ID_SLIDER")), wxSlider);
	speed_slider = wxDynamicCast(FindWindow(XRCID("ID_SPEED_SLIDER")),
								 wxSlider);
	{
		int sval = 100-(speed_slider->GetValue());
		sval = sval*sval/100;
		delay_ms = min_delay_ms + ((max_delay_ms-min_delay_ms)*sval)/100;	
	}
	min_txt = wxDynamicCast(FindWindow(XRCID("ID_MIN_TXT")), wxStaticText);
	max_txt = wxDynamicCast(FindWindow(XRCID("ID_MAX_TXT")), wxStaticText);
	cur_txt = wxDynamicCast(FindWindow(XRCID("ID_CUR_TXT")), wxStaticText);
	loop_cb = wxDynamicCast(FindWindow(XRCID("ID_LOOP")), wxCheckBox);
	reverse_cb = wxDynamicCast(FindWindow(XRCID("ID_REVERSE")), wxCheckBox);
	
	int steps = grid_base->time_ids.size();	
	wxString t_min;
	wxString t_max;
	wxString t_cur;
	t_min << grid_base->time_ids[0];
	t_max << grid_base->time_ids[steps-1];
	t_cur << grid_base->time_ids[grid_base->curr_time_step];
	min_txt->SetLabelText(t_min);
	max_txt->SetLabelText(t_max);
	cur_txt->SetLabelText(t_cur);
	slider_val = grid_base->curr_time_step;
	
	suspend_notify = true;
	slider->SetRange(0, steps-1);
	slider->SetValue(grid_base->curr_time_step);
	suspend_notify = false;
	
	speed_slider->GetValue();
	
	frames_manager->registerObserver(this);
	SetMinSize(wxSize(100,50));
	all_init = true;
	LOG_MSG("Exiting TimeChooserDlg::TimeChooserDlg");
}

TimeChooserDlg::~TimeChooserDlg()
{
	if (timer) delete timer; 
	frames_manager->removeObserver(this);
}

void TimeChooserDlg::OnClose(wxCloseEvent& ev)
{
	LOG_MSG("Entering TimeChooserDlg::OnClose");
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
	LOG_MSG("Exiting TimeChooserDlg::OnClose");
}

void TimeChooserDlg::OnMoveSlider(wxCommandEvent& ev)
{
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int slider_val = slider->GetValue();
	if (slider_val == GetGridBaseTimeStep()) return;
	SetCurTxt(slider_val);
	grid_base->curr_time_step = slider_val;
	if (!suspend_notify) {
		suspend_update = true;
		frames_manager->notifyObservers();
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
	LOG_MSG("In TimeChooserDlg::ChangeTime");
	if (!all_init) return;
	LOG(new_time);
	slider_val = new_time;
	slider->SetValue(slider_val);
	SetCurTxt(slider_val);
	grid_base->curr_time_step = slider_val;
	if (!suspend_notify) {
		suspend_update = true;
		frames_manager->notifyObservers();
		suspend_update = false;
	}
	Refresh();
}

void TimeChooserDlg::OnPlayPauseButton(wxCommandEvent& ev)
{
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
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int new_slider_val = GetSliderTimeStep()+1;
	if (new_slider_val == GetTotalTimeSteps()) new_slider_val=0;
	ChangeTime(new_slider_val);
}

void TimeChooserDlg::OnStepBackButton(wxCommandEvent& ev)
{
	if (!all_init) return;
	if (playing) OnPlayPauseButton(ev);
	int new_slider_val = GetSliderTimeStep()-1;
	if (new_slider_val < 0) new_slider_val = GetTotalTimeSteps()-1;
	ChangeTime(new_slider_val);
}

void TimeChooserDlg::OnSpeedSlowerButton(wxCommandEvent& ev)
{
	if (!all_init) return;
	delay_ms = 3200;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedSlowButton(wxCommandEvent& ev)
{
	if (!all_init) return;
	delay_ms = 2200;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedMediumButton(wxCommandEvent& ev)
{
	if (!all_init) return;
	delay_ms = 1500;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedFastButton(wxCommandEvent& ev)
{
	if (!all_init) return;
	delay_ms = 700;
	//ChangeSpeed(delay_ms);
}

void TimeChooserDlg::OnSpeedFasterButton(wxCommandEvent& ev)
{
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
	if (!all_init) return;
	forward = (reverse_cb->GetValue() == 0);
}

void TimeChooserDlg::OnLoopCheckBox(wxCommandEvent& ev)
{
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
	return grid_base->curr_time_step;
}

int TimeChooserDlg::GetTotalTimeSteps()
{
	return grid_base->time_steps;
}

void TimeChooserDlg::SetCurTxt(wxInt64 val)
{
	if (!all_init) return;
	wxString t_val;
	t_val << grid_base->time_ids[val];
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
	if (suspend_update) return;
	LOG_MSG("Entering TimeChooserDlg::update(FramesManager* o)");
	suspend_notify = true;
	LOG(slider->GetValue());
	SetCurTxt(grid_base->curr_time_step);
	slider->SetValue(grid_base->curr_time_step);
	slider->Refresh();
	LOG(slider->GetValue());
	suspend_notify = false;
	Refresh();
	LOG_MSG("Exiting TimeChooserDlg::update(FramesManager* o)");
}

void TimeChooserDlg::OnKeyEvent(wxKeyEvent& event)
{
	if (event.GetModifiers() == wxMOD_CMD &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		LOG(del);
		grid_base->curr_time_step = grid_base->curr_time_step + del;
		if (grid_base->curr_time_step < 0) {
			grid_base->curr_time_step = grid_base->time_steps-1;
		} else if (grid_base->curr_time_step >= grid_base->time_steps) {
			grid_base->curr_time_step = 0;
		}
		frames_manager->notifyObservers();
		return;
	}
	event.Skip();
}
