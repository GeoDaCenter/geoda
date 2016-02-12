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

#ifndef __GEODA_CENTER_TIME_CHOOSER_DLG_H__
#define __GEODA_CENTER_TIME_CHOOSER_DLG_H__

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include "../FramesManagerObserver.h"
#include "../DataViewer/TimeStateObserver.h"
#include "../DataViewer/TableStateObserver.h"

class FramesManager;
class TimeState;
class TimeChooserDlg;

class TimeChooserTimer : public wxTimer
{
public:
	TimeChooserTimer(TimeChooserDlg* dlg);
	TimeChooserTimer();
	virtual ~TimeChooserTimer();
	
	TimeChooserDlg* time_chooser_dlg;
	virtual void Notify();
};


class TimeChooserDlg : public wxDialog, public FramesManagerObserver,
	public TimeStateObserver, public TableStateObserver
{
public:	
	TimeChooserDlg(wxWindow* parent, FramesManager* frames_manager,
				   TimeState* time_state, TableState* table_state,
				   TableInterface* table_int);
	virtual ~TimeChooserDlg();
	void OnClose(wxCloseEvent& ev);
	void OnMoveSlider(wxCommandEvent& ev);
	void OnMoveSpeedSlider(wxCommandEvent& ev);
	void ChangeTime(int new_time);
	void OnPlayPauseButton(wxCommandEvent& ev);
	void OnStepForwardButton(wxCommandEvent& ev);
	void OnStepBackButton(wxCommandEvent& ev);
	void OnSpeedSlowerButton(wxCommandEvent& ev);
	void OnSpeedSlowButton(wxCommandEvent& ev);
	void OnSpeedMediumButton(wxCommandEvent& ev);
	void OnSpeedFastButton(wxCommandEvent& ev);
	void OnSpeedFasterButton(wxCommandEvent& ev);
	void ChangeSpeed(int delay_ms);
	void OnReverseCheckBox(wxCommandEvent& ev);
	void OnLoopCheckBox(wxCommandEvent& ev);
	void OnKeyEvent(wxKeyEvent& ev);
    void ToggleButtons(bool enabled);

	
	int GetSliderTimeStep();
	int GetGridBaseTimeStep();
	int GetTotalTimeSteps();
	void SetCurTxt(wxInt64 val);
	
	void UpdateDelayFromSlider();
	void TimerCall();
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
protected:
	TableInterface* table_int;
	FramesManager* frames_manager;
	TimeState* time_state;
	TableState* table_state;
	TimeChooserTimer* timer;
	bool playing;
	wxSlider* slider;
	int slider_val;
	wxSlider* speed_slider;
	int speed_slider_val;
	static const int min_delay_ms = 200;
	static const int max_delay_ms = 3000;
	wxButton* play_button;
    wxButton* forward_button;
    wxButton* backward_button;
	wxStaticText* cur_txt;
	wxCheckBox* loop_cb;
	wxCheckBox* reverse_cb;
	bool all_init;
	bool suspend_notify;
	bool suspend_update;
	int delay_ms;
	bool forward;
	bool loop;
	
	DECLARE_EVENT_TABLE()
};


#endif
