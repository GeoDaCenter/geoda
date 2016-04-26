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

#ifndef __GEODA_CENTER_DATA_MOVIE_DLG_H__
#define __GEODA_CENTER_DATA_MOVIE_DLG_H__

#include <boost/multi_array.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include "../DataViewer/TableStateObserver.h"
#include "../FramesManagerObserver.h"
#include "../GenUtils.h"

class TableState;
class TimeState;
class FramesManager;
class TableInterface;
class DataMovieDlg;
class DataMovieTimer;

class DataMovieTimer: public wxTimer
{
public:
	DataMovieTimer(DataMovieDlg* dlg);
	DataMovieTimer();
	virtual ~DataMovieTimer();
	
	DataMovieDlg* data_movie_dlg;
	virtual void Notify();
};


class DataMovieDlg : public wxDialog, public FramesManagerObserver,
	public TableStateObserver
{
public:	
	DataMovieDlg(wxWindow* parent, FramesManager* frames_manager,
				 TableState* table_state, TimeState* time_state,
				 TableInterface* table_int,
				 HighlightState* highlight_state);
	virtual ~DataMovieDlg();
	void OnClose(wxCloseEvent& ev);
	void OnMoveSlider(wxCommandEvent& ev);
	void OnMoveSpeedSlider(wxCommandEvent& ev);
	void ChangePosNum(int new_pos_num);
	void StopPlaying();
	void OnPlayPauseButton(wxCommandEvent& ev);
	void OnStepForwardButton(wxCommandEvent& ev);
	void OnStepBackButton(wxCommandEvent& ev);
	void ChangeSpeed(int delay_ms);
	void OnReverseCheckBox(wxCommandEvent& ev);
	void OnLoopCheckBox(wxCommandEvent& ev);
	void InitFieldChoices();
	void OnFieldChoice(wxCommandEvent& ev);
	void OnFieldChoiceTm(wxCommandEvent& ev);
	void OnCumulativeCheckBox(wxCommandEvent& ev);
	void OnAscendingRB(wxCommandEvent& ev);
	void OnDescendingRB(wxCommandEvent& ev);

	void EnableControls(bool enable);
	void InitNewFieldChoice();
	
	void OnKeyEvent(wxKeyEvent& ev);

	int GetSliderPosNum();
	void SetCurTxt(wxInt64 pos);
	
	void UpdateDelayFromSlider();
	void TimerCall();
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return false; }
	
private:
	FramesManager* frames_manager;
	TableState* table_state;
	TimeState* time_state;
	DataMovieTimer* timer;
	bool playing;
	wxString cur_field_choice;
	int cur_field_choice_tm;
	wxChoice* field_choice;
	wxChoice* field_choice_tm;
	wxStaticText* time_label;
	wxSlider* slider;
	wxSlider* speed_slider;
	int speed_slider_val;
	static const int min_delay_ms = 50;
	static const int max_delay_ms = 1000;
	wxButton* play_button;
	wxButton* step_forward_button;
	wxButton* step_back_button;
	wxStaticText* min_label_txt;
	wxStaticText* min_txt;
	wxStaticText* max_label_txt;
	wxStaticText* max_txt;
	wxStaticText* cur_obs_txt;
	wxStaticText* cur_val_txt;
	wxCheckBox* loop_cb;
	wxCheckBox* reverse_cb;
	wxCheckBox* cumulative_cb;
	wxRadioButton* ascending_rb;
	wxRadioButton* descending_rb;
	TableInterface* table_int;
	HighlightState* highlight_state;
	bool all_init;
	bool ignore_slider_event;
	int delay_ms;
	bool forward;
	bool loop;
	bool is_space_time;
	int num_obs;
	bool is_ascending;
	bool is_cumulative;
	
	Gda::dbl_int_pair_vec_type data_sorted;
	
	DECLARE_EVENT_TABLE()
};


#endif
