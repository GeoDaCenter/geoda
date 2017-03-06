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
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TimeState.h"
#include "../DataViewer/TableState.h"
#include "../HighlightState.h"
#include "../logger.h"
#include "DataMovieDlg.h"

BEGIN_EVENT_TABLE( DataMovieDlg, wxDialog )
	EVT_CLOSE( DataMovieDlg::OnClose )
	EVT_SLIDER( XRCID("ID_SLIDER"), DataMovieDlg::OnMoveSlider )
	EVT_SLIDER( XRCID("ID_SPEED_SLIDER"), DataMovieDlg::OnMoveSpeedSlider )
	EVT_BUTTON( XRCID("ID_PLAY"), DataMovieDlg::OnPlayPauseButton )
	EVT_BUTTON( XRCID("ID_STEP_FORWARD"), DataMovieDlg::OnStepForwardButton )
	EVT_BUTTON( XRCID("ID_STEP_BACK"), DataMovieDlg::OnStepBackButton )
	EVT_CHECKBOX( XRCID("ID_LOOP"), DataMovieDlg::OnLoopCheckBox )
	EVT_CHECKBOX( XRCID("ID_REVERSE"), DataMovieDlg::OnReverseCheckBox )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE"), DataMovieDlg::OnFieldChoice )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE_TM"), DataMovieDlg::OnFieldChoiceTm )
	EVT_CHECKBOX( XRCID("ID_CUMULATIVE"), DataMovieDlg::OnCumulativeCheckBox)
	EVT_RADIOBUTTON( XRCID("ID_ASCENDING"), DataMovieDlg::OnAscendingRB )
	EVT_RADIOBUTTON( XRCID("ID_DESCENDING"), DataMovieDlg::OnDescendingRB )
	EVT_CHAR_HOOK( DataMovieDlg::OnKeyEvent )
END_EVENT_TABLE()

DataMovieTimer::DataMovieTimer() : data_movie_dlg(0)
{
}

DataMovieTimer::DataMovieTimer(DataMovieDlg* dlg) :
	data_movie_dlg(dlg)
{
}

DataMovieTimer::~DataMovieTimer()
{
	data_movie_dlg = 0;
}

void DataMovieTimer::Notify()
{
	if (data_movie_dlg) data_movie_dlg->TimerCall();
}

DataMovieDlg::DataMovieDlg(wxWindow* parent,
						   FramesManager* frames_manager_s,
						   TableState* table_state_s,
						   TimeState* time_state_s,
						   TableInterface* table_int_s,
						   HighlightState* highlight_state_s)
: frames_manager(frames_manager_s), table_state(table_state_s),
time_state(time_state_s), table_int(table_int_s),
highlight_state(highlight_state_s),
playing(false), timer(0), delay_ms(333),
loop(true), forward(true), is_space_time(time_state_s->GetTimeSteps() > 1),
num_obs(table_int->GetNumberRows()),
cur_field_choice(""), cur_field_choice_tm(0),
is_ascending(true), is_cumulative(true),
all_init(false)
{
	wxLogMessage("Open DataMovieDlg.");
	SetParent(parent);
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_DATA_MOVIE_DLG");
	
	field_choice = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE")),
								 wxChoice);
	field_choice_tm = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE_TM")),
									wxChoice);
	time_label = wxDynamicCast(FindWindow(XRCID("ID_TIME_LABEL")),
							   wxStaticText);
	if (!is_space_time) {
		field_choice_tm->Show(false);
		time_label->Show(false);
	}
	
	play_button = wxDynamicCast(FindWindow(XRCID("ID_PLAY")), wxButton);
	step_forward_button = wxDynamicCast(FindWindow(XRCID("ID_STEP_FORWARD")),
										wxButton);
	step_back_button = wxDynamicCast(FindWindow(XRCID("ID_STEP_BACK")),
										 wxButton);
	slider = wxDynamicCast(FindWindow(XRCID("ID_SLIDER")), wxSlider);
	speed_slider = wxDynamicCast(FindWindow(XRCID("ID_SPEED_SLIDER")),
								 wxSlider);
	{
		int sval = 100-(speed_slider->GetValue());
		sval = sval*sval/100;
		delay_ms = min_delay_ms + ((max_delay_ms-min_delay_ms)*sval)/100;	
		//speed_slider->GetValue();
	}
	min_txt = wxDynamicCast(FindWindow(XRCID("ID_MIN_TXT")), wxStaticText);
	min_label_txt = wxDynamicCast(FindWindow(XRCID("ID_MIN_LABEL")),
								  wxStaticText);
	max_txt = wxDynamicCast(FindWindow(XRCID("ID_MAX_TXT")), wxStaticText);
	max_label_txt = wxDynamicCast(FindWindow(XRCID("ID_MAX_LABEL")),
								  wxStaticText);
	cur_obs_txt = wxDynamicCast(FindWindow(XRCID("ID_CUR_OBS_TXT")),
								wxStaticText);
	cur_val_txt = wxDynamicCast(FindWindow(XRCID("ID_CUR_VAL_TXT")),
								wxStaticText);
	loop_cb = wxDynamicCast(FindWindow(XRCID("ID_LOOP")), wxCheckBox);
	reverse_cb = wxDynamicCast(FindWindow(XRCID("ID_REVERSE")), wxCheckBox);
	cumulative_cb = wxDynamicCast(FindWindow(XRCID("ID_CUMULATIVE")),
								   wxCheckBox);
	ascending_rb = wxDynamicCast(FindWindow(XRCID("ID_ASCENDING")),
								 wxRadioButton);
	descending_rb = wxDynamicCast(FindWindow(XRCID("ID_DESCENDING")),
								  wxRadioButton);
	
	ignore_slider_event = true;
	// value 0 represents nothing selected and value num_obs represents
	// everything selected.
	slider->SetRange(0, num_obs);
	slider->SetValue(0);
	ignore_slider_event = false;
	
	all_init = true; // all widget pointers are initialized
	
	InitFieldChoices();
	
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	SetMinSize(wxSize(100,50));
}

DataMovieDlg::~DataMovieDlg()
{
	if (timer) delete timer; 
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
}

void DataMovieDlg::OnClose(wxCloseEvent& ev)
{
	wxLogMessage("Close DataMovieDlg::OnClose");
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

void DataMovieDlg::OnMoveSlider(wxCommandEvent& ev)
{
	wxLogMessage("In DataMovieDlg::OnMoveSlider");
	if (!all_init || ignore_slider_event) return;
	if (playing) StopPlaying();
	ChangePosNum(GetSliderPosNum());
	Refresh();
}

void DataMovieDlg::OnMoveSpeedSlider(wxCommandEvent& ev)
{
	wxLogMessage("In DataMovieDlg::OnMoveSpeedSlider");
	if (!all_init) return;
	UpdateDelayFromSlider();
}

void DataMovieDlg::ChangePosNum(int new_pos_num)
{
	if (!all_init) return;
	int slider_val = slider->GetValue();
	slider->SetValue(new_pos_num);
	SetCurTxt(new_pos_num);
	if (new_pos_num == 0) {
		highlight_state->SetEventType(HLStateInt::unhighlight_all);
		highlight_state->notifyObservers();
		Refresh();
		return;
	}
	std::vector<bool>& hs = highlight_state->GetHighlight();
    bool selection_changed = false;
	
	// the passed-in new_pos_num refers to the observations starting
	// from 1 to num_obs, rather than 0 to num_obs-1.
	int pos = new_pos_num-1;
	if (is_cumulative) {
		for (int i=0; i<num_obs; i++) {
			int id = data_sorted[i].second;
			if (i > pos && hs[id]) {
                hs[id] = false;
                selection_changed = true;
			} else if (i <= pos && !hs[id]) {
                hs[id] = true;
                selection_changed = true;
			} 
		}
	} else {
		for (int i=0; i<num_obs; i++) {
			int id = data_sorted[i].second;
			if (hs[id] && i != pos) {
                hs[id] = false;
                selection_changed = true;
			} else if (i == pos && !hs[id]) {
                hs[id] = true;
                selection_changed = true;
			}
		}
	}
    if (selection_changed) {
		highlight_state->SetEventType(HLStateInt::delta);
		highlight_state->notifyObservers();
	}
	Refresh();
}

void DataMovieDlg::StopPlaying()
{
	playing = false;
	play_button->SetLabel(">");
	Refresh();
	if (timer) {
		timer->Stop();
		delete timer;
		timer = 0;
	}
}

void DataMovieDlg::OnPlayPauseButton(wxCommandEvent& ev)
{
    wxLogMessage("DataMovieDlg::OnPlayPauseButton");
	if (!all_init) return;
	if (playing) {
		// stop playing
		StopPlaying();
	} else {
		// start playing
		int new_slider_val;
		if (forward) {
			new_slider_val = GetSliderPosNum()+1;
			if (new_slider_val == num_obs+1) new_slider_val=0;
		} else {
			new_slider_val = GetSliderPosNum()-1;
			if (new_slider_val < 0) new_slider_val = num_obs;
		}
		ChangePosNum(new_slider_val);
		
		playing = true;
		play_button->SetLabel("||");
		Refresh();
		if (!timer) timer = new DataMovieTimer(this);
		timer->Start(delay_ms);
	}
}

void DataMovieDlg::OnStepForwardButton(wxCommandEvent& ev)
{
    wxLogMessage("DataMovieDlg::OnStepForwardButton");
	if (!all_init) return;
	if (playing) StopPlaying();
	int new_slider_val = GetSliderPosNum()+1;
	if (new_slider_val == num_obs+1) new_slider_val=0;
	ChangePosNum(new_slider_val);
}

void DataMovieDlg::OnStepBackButton(wxCommandEvent& ev)
{
    wxLogMessage("DataMovieDlg::OnStepBackButton");
	if (!all_init) return;
	if (playing) StopPlaying();
	int new_slider_val = GetSliderPosNum()-1;
	if (new_slider_val < 0) new_slider_val = num_obs;
	ChangePosNum(new_slider_val);
}

void DataMovieDlg::ChangeSpeed(int delay_ms)
{
	if (!all_init) return;
	if (timer && timer->IsRunning()) {
		timer->Start(delay_ms);
	}
}

void DataMovieDlg::OnReverseCheckBox(wxCommandEvent& ev)
{
    wxLogMessage("DataMovieDlg::OnReverseCheckBox");
	if (!all_init) return;
	forward = (reverse_cb->GetValue() == 0);
}

void DataMovieDlg::OnLoopCheckBox(wxCommandEvent& ev)
{
    wxLogMessage("DataMovieDlg::OnLoopCheckBox");
	if (!all_init) return;
	loop = (loop_cb->GetValue() == 1);
}

/**
 Assumptions: all widgets are initialized
 Purpose: Initialize field_choice and field_choice_tm
 widgets.  If any current selections, then these are remembered
 and reselected.
 */
void DataMovieDlg::InitFieldChoices()
{
	if (!all_init) return;
	wxString cur_fc_str = field_choice->GetStringSelection();
	int cur_fc_tm_id = field_choice_tm->GetSelection();
	field_choice->Clear();
	field_choice_tm->Clear();
	std::vector<wxString> times;
	table_int->GetTimeStrings(times);
	for (int i=0; i<times.size(); i++) {
		field_choice_tm->Append(times[i]);
	}
	if (cur_fc_tm_id != wxNOT_FOUND) {
		field_choice_tm->SetSelection(cur_fc_tm_id);
	}
	std::vector<wxString> names;
	table_int->FillNumericNameList(names);
	for (int i=0; i<names.size(); i++) {
		field_choice->Append(names[i]);
	}
	field_choice->SetSelection(field_choice->FindString(cur_fc_str));
	field_choice_tm->Enable(table_int->IsColTimeVariant(cur_fc_str));
	if (table_int->IsColTimeVariant(cur_fc_str) &&
		field_choice_tm->GetSelection() == wxNOT_FOUND) {
		field_choice_tm->SetSelection(0);
	}
	
	if (field_choice->FindString(cur_fc_str) != wxNOT_FOUND) {
		cur_field_choice = cur_fc_str;
		if (table_int->IsColTimeVariant(cur_fc_str)) {
			cur_field_choice_tm = field_choice_tm->GetSelection();
		} else {
			cur_field_choice_tm = 0;
		}
	} else {
		cur_field_choice = "";
		if (playing) StopPlaying();
	}
	
	EnableControls(field_choice->FindString(cur_fc_str) != wxNOT_FOUND);
}

void DataMovieDlg::OnFieldChoice(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataMovieDlg::OnFieldChoice");
	wxString cur_fc_str = field_choice->GetStringSelection();
	bool is_tm_var = table_int->IsColTimeVariant(cur_fc_str);
	field_choice_tm->Enable(is_tm_var);
	if (is_tm_var && field_choice_tm->GetSelection() == wxNOT_FOUND) {
		field_choice_tm->SetSelection(0);
	}
	InitFieldChoices();
	InitNewFieldChoice();
	ChangePosNum(GetSliderPosNum());
}

void DataMovieDlg::OnFieldChoiceTm(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataMovieDlg::OnFieldChoiceTm");
	InitFieldChoices();
	InitNewFieldChoice();
	ChangePosNum(GetSliderPosNum());
}

void DataMovieDlg::OnCumulativeCheckBox(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataMovieDlg::OnCumulativeCheckBox");
	if (!all_init) return;
	is_cumulative = cumulative_cb->GetValue() == 1;
}

void DataMovieDlg::OnAscendingRB(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataMovieDlg::OnAscendingRB");
	if (!all_init) return;
	is_ascending = true;
	min_label_txt->SetLabel("min:");
	max_label_txt->SetLabel("max:");
	InitNewFieldChoice();
	ChangePosNum(GetSliderPosNum());
}

void DataMovieDlg::OnDescendingRB(wxCommandEvent& ev)
{
	wxLogMessage("Entering DataMovieDlg::OnDescendingRB");
	if (!all_init) return;
	is_ascending = false;
	min_label_txt->SetLabel("max:");
	max_label_txt->SetLabel("min:");
	InitNewFieldChoice();
	ChangePosNum(GetSliderPosNum());
}

/** If true, then a valid Variable is selected in field_choice, and
 the slider, time, forward/reverse/play buttons should all be
 enabled.  Otherwise, these controls should be disabled */
void DataMovieDlg::EnableControls(bool enable)
{
	if (!all_init) return;
	if (is_space_time && field_choice && field_choice_tm) {
		wxString cur_fc_str = field_choice->GetStringSelection();
		bool is_tm_var = table_int->IsColTimeVariant(cur_fc_str);
		field_choice_tm->Enable(is_tm_var);
	}
	play_button->Enable(enable);
	step_forward_button->Enable(enable);
	step_back_button->Enable(enable);
	slider->Enable(enable);
	if (!enable) {
		ignore_slider_event = true;
		slider->SetValue(0);
		min_txt->SetLabelText(" ");
		max_txt->SetLabelText(" ");
		cur_obs_txt->SetLabelText(" ");
		cur_val_txt->SetLabelText(" ");
		ignore_slider_event = false;
	}
}

/** A new field_choice / field_choice_tm combination has been selected.  If
 cur_field_choice str is empty, then reset values to blank, otherwise
 get data from table and set min/max values.
 */
void DataMovieDlg::InitNewFieldChoice()
{
	if (cur_field_choice.IsEmpty()) {
		EnableControls(false);
		return;
	}
	data_sorted.resize(num_obs);
	int col = table_int->FindColId(cur_field_choice);
	std::vector<double> dd;
	table_int->GetColData(col, cur_field_choice_tm, dd);
	for (int i=0; i<num_obs; i++) {
		data_sorted[i].first = dd[i];
		data_sorted[i].second = i;
	}
	if (is_ascending) {
		std::sort(data_sorted.begin(), data_sorted.end(),
				  Gda::dbl_int_pair_cmp_less);
	} else {
		std::sort(data_sorted.begin(), data_sorted.end(),
				  Gda::dbl_int_pair_cmp_greater);
	}
	min_txt->SetLabelText(GenUtils::DblToStr(data_sorted[0].first));
	max_txt->SetLabelText(GenUtils::DblToStr(data_sorted[num_obs-1].first));
}

int DataMovieDlg::GetSliderPosNum()
{
	if (!all_init) return 0;
	return slider->GetValue();
}

void DataMovieDlg::SetCurTxt(wxInt64 pos)
{
	if (!all_init) return;
	wxString t_obs;
	wxString t_val;
	if (pos > 0) {
		t_obs << data_sorted[pos-1].second+1;
		t_val << GenUtils::DblToStr(data_sorted[pos-1].first);
	} else {
		t_obs << "     ";
		t_val << "     ";
	}
	cur_obs_txt->SetLabelText(t_obs);
	cur_val_txt->SetLabelText(t_val);
}

void DataMovieDlg::UpdateDelayFromSlider()
{
	if (!all_init) return;
	int sval = 100-(speed_slider->GetValue());
	sval = sval*sval/100;
	delay_ms = min_delay_ms + ((max_delay_ms-min_delay_ms)*sval)/100;
	ChangeSpeed(delay_ms);	
}

void DataMovieDlg::TimerCall()
{
	if (!playing) return;
	wxCommandEvent ev;
	int new_time_step;
	if (forward) {
		new_time_step = GetSliderPosNum() + 1;
		if (!loop && new_time_step >= num_obs - 1) {
			OnPlayPauseButton(ev);
			ChangePosNum(num_obs - 1);
			return;
		}
		if (loop && new_time_step >= num_obs) {
			new_time_step = 0;
		}
	} else {
		new_time_step = GetSliderPosNum() - 1;
		if (!loop && new_time_step <= 0) {
			OnPlayPauseButton(ev);
			ChangePosNum(0);
			return;
		}
		if (loop && new_time_step <= -1) {
			new_time_step = num_obs - 1;
		}
	}
	ChangePosNum(new_time_step);
}

/** FramesManager calls update when time changes, but this dialog does
 not care about time changes, so ignore */
void DataMovieDlg::update(FramesManager* o)
{
}

void DataMovieDlg::update(TableState* o)
{
	InitFieldChoices();
}

void DataMovieDlg::OnKeyEvent(wxKeyEvent& event)
{
    wxLogMessage("In DataMovieDlg::OnKeyEvent");
    
	if (event.GetModifiers() == wxMOD_CMD &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		int del = (event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
		time_state->SetCurrTime(time_state->GetCurrTime() + del);
		if (time_state->GetCurrTime() < 0) {
			time_state->SetCurrTime(time_state->GetTimeSteps()-1);
		} else if (time_state->GetCurrTime() >= time_state->GetTimeSteps()) {
			time_state->SetCurrTime(0);
		}
		time_state->notifyObservers();
		return;
	}
	event.Skip();
}
