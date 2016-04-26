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

#include <vector>
#include <boost/foreach.hpp>
#include <wx/msgdlg.h>
#include <wx/arrstr.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../GeoDa.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../logger.h"
#include "TimeEditorDlg.h"

BEGIN_EVENT_TABLE( TimeEditorDlg, wxDialog )
	EVT_CLOSE( TimeEditorDlg::OnClose )
	EVT_BUTTON( XRCID("ID_NEW_BTN"), TimeEditorDlg::OnNewButton )
	EVT_BUTTON( XRCID("ID_BACK_BTN"), TimeEditorDlg::OnBackButton )
	EVT_BUTTON( XRCID("ID_FORWARD_BTN"), TimeEditorDlg::OnForwardButton )
	EVT_BUTTON( XRCID("ID_DELETE_BTN"), TimeEditorDlg::OnDeleteButton )	
	EVT_SIZE( TimeEditorDlg::OnSize )
	EVT_LIST_END_LABEL_EDIT( XRCID("ID_TIME_ID_LIST"),
							TimeEditorDlg::OnEndEditItem )
	EVT_LIST_ITEM_ACTIVATED( XRCID("ID_TIME_ID_LIST"),
							TimeEditorDlg::OnItemActivate )
	EVT_LIST_ITEM_SELECTED( XRCID("ID_TIME_ID_LIST"),
						   TimeEditorDlg::OnItemSelection )
	EVT_LIST_ITEM_DESELECTED( XRCID("ID_TIME_ID_LIST"),
							 TimeEditorDlg::OnItemSelection )
END_EVENT_TABLE()

TimeEditorDlg::TimeEditorDlg(wxWindow* parent,
							 FramesManager* frames_manager_s,
							 TableState* table_state_s,
							 TableInterface* table_int_s)
: frames_manager(frames_manager_s), table_state(table_state_s),
table_int(table_int_s), all_init(false), lc(0),
ignore_time_id_update(false)
{
	SetParent(parent);
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_TIME_EDITOR");
	lc = wxDynamicCast(FindWindow(XRCID("ID_TIME_ID_LIST")), wxListCtrl);
	lc->InsertColumn(0, "", wxLIST_FORMAT_CENTER, 150);
	InitFromTable();
	UpdateButtons();
	
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	all_init = true;
}

TimeEditorDlg::~TimeEditorDlg()
{
	LOG_MSG("In TimeEditorDlg::~TimeEditorDlg");
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
}

void TimeEditorDlg::InitFromTable()
{
	lc->DeleteAllItems();
	std::vector<wxString> tm_strs;
	table_int->GetTimeStrings(tm_strs);
	int t=0;
	BOOST_FOREACH(const wxString& s, tm_strs) lc->InsertItem(t++, s);
}

void TimeEditorDlg::OnClose(wxCloseEvent& ev)
{
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

void TimeEditorDlg::OnNewButton(wxCommandEvent& ev)
{
	if (table_state->GetNumDisallowTimelineChanges() > 0) {
		wxString msg = table_state->GetDisallowTimelineChangesMsg();
		wxMessageDialog dlg (this, msg, "Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	int id = GetListSel();
	bool end_insert = id < 0;
	if (end_insert) id = lc->GetItemCount();
	wxString label;
	if (id == 0) {
		label = GetNewPrependTimeLabel();
	} else {
		label = GetNewAppendTimeLabel();
	}
	table_int->InsertTimeStep(id, label);
	if (!end_insert) SelectItem(id);
	if (lc->GetItemCount() == 2) {
		GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
	}
	UpdateButtons();
}

void TimeEditorDlg::OnBackButton(wxCommandEvent& ev)
{
	int sel = GetListSel();
	if (sel <= 0 || sel >= lc->GetItemCount()) return;
	if (table_state->GetNumDisallowTimelineChanges() > 0) {
		wxString msg = table_state->GetDisallowTimelineChangesMsg();
		wxMessageDialog dlg (this, msg, "Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	table_int->SwapTimeSteps(sel-1, sel);
	SelectItem(sel-1);
	UpdateButtons();
}

void TimeEditorDlg::OnForwardButton(wxCommandEvent& ev)
{
	int sel = GetListSel();
	if (sel < 0 || sel >= lc->GetItemCount()-1) return;
	if (table_state->GetNumDisallowTimelineChanges() > 0) {
		wxString msg = table_state->GetDisallowTimelineChangesMsg();
		wxMessageDialog dlg (this, msg, "Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	table_int->SwapTimeSteps(sel, sel+1);
	SelectItem(sel+1);
	UpdateButtons();
}

void TimeEditorDlg::OnDeleteButton(wxCommandEvent& ev)
{
	int sel = GetListSel();
	if (sel < 0) return;
	if (table_state->GetNumDisallowTimelineChanges() > 0) {
		wxString msg = table_state->GetDisallowTimelineChangesMsg();
		wxMessageDialog dlg (this, msg, "Warning",
							 wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
		return;
	}
	table_int->RemoveTimeStep(sel);
	if (lc->GetItemCount() == 1) {
		GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
	}
	UpdateButtons();
}

void TimeEditorDlg::OnSize(wxSizeEvent& ev)
{
	wxDialog::OnSize(ev);
	if (lc) {
		int w, h;
		lc->GetClientSize(&w, &h);
		lc->SetColumnWidth(0, w);
	}
}

void TimeEditorDlg::OnEndEditItem(wxListEvent& ev)
{
	using namespace std;
	LOG_MSG(wxString::Format("In TimeEditorDlg::OnEndEditItem, item %d",
							 (int) ev.GetItem().GetId()));
	// ensure that new time label is unique and not empty
	int id = ev.GetItem().GetId();
	wxString item_new = ev.GetText();
	item_new.Trim(true);
	item_new.Trim(false);
	LOG(item_new);
	bool is_dup = false;
	for (int t=0, tms=table_int->GetTimeSteps(); t<tms && !is_dup; ++t) {
		if (t != id && table_int->GetTimeString(t) == item_new) is_dup = true;
	}
	
	bool is_disallow = table_state->GetNumDisallowTimelineChanges() > 0;
	
	if (item_new.IsEmpty() || is_dup || is_disallow) {
		if (is_disallow && !(item_new.IsEmpty() || is_dup)) {
			wxString msg = table_state->GetDisallowTimelineChangesMsg();
			wxMessageDialog dlg(this, msg, "Warning",
								wxOK | wxICON_INFORMATION);
			dlg.ShowModal();
		}
		LOG_MSG("Restoring item to original.");
		ev.Veto();
		return;
	}

	// item_new is valid, proceed with rename
	ignore_time_id_update = true;
	table_int->RenameTimeStep(id, item_new);
}

void TimeEditorDlg::OnItemActivate(wxListEvent& ev)
{
	lc->EditLabel(ev.GetItem().GetId());
	UpdateButtons();
}

void TimeEditorDlg::OnItemSelection( wxListEvent& event )
{
	if (!all_init) return;
	UpdateButtons();
}

void TimeEditorDlg::update(FramesManager* o)
{	
}

void TimeEditorDlg::update(TableState* o)
{
	TableState::EventType type = o->GetEventType();
	if (!ignore_time_id_update &&
		(type == TableState::time_ids_add_remove ||
		 type == TableState::time_ids_rename ||
		 type == TableState::time_ids_swap))
	{
		InitFromTable();
	}
	ignore_time_id_update = false;
	UpdateButtons();
}

wxString TimeEditorDlg::GetNewPrependTimeLabel()
{
	int cnt = 1;
	wxString s;
	for (int i=0; i<10000; i++) {
		s = "time ";
		s << cnt--;
		if (lc->FindItem(-1, s) == wxNOT_FOUND) return s;
	}
	return s;
}

wxString TimeEditorDlg::GetNewAppendTimeLabel()
{
	int cnt = 1;
	wxString s;
	for (int i=0; i<10000; i++) {
		s = "time ";
		s << cnt++;
		if (lc->FindItem(-1, s) == wxNOT_FOUND) return s;
	}
	return s;
}

int TimeEditorDlg::GetListSel()
{
	if (!all_init || !lc) return -1;
	long item = -1;
	for ( ;; ) {
		item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if ( item == -1 ) break;
		return item;
	}
	return -1;
}

void TimeEditorDlg::SelectItem(int i)
{
	if (!lc || i<0 || i>=lc->GetItemCount()) return;
	lc->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void TimeEditorDlg::UpdateButtons()
{
	if (!lc) return;
	int sel = GetListSel();
	wxWindow* w;
	w = FindWindow(XRCID("ID_NEW_BTN"));
	if (w) w->Enable(true);
	w = FindWindow(XRCID("ID_BACK_BTN"));
	if (w) w->Enable(sel > 0);
	w = FindWindow(XRCID("ID_FORWARD_BTN"));
	if (w) w->Enable(sel >= 0 && sel < lc->GetItemCount()-1);
	w = FindWindow(XRCID("ID_DELETE_BTN"));
	if (w) w->Enable(sel >= 0 && sel < lc->GetItemCount() &&
					 lc->GetItemCount() > 1);
}
