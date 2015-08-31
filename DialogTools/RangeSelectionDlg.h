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

#ifndef __GEODA_CENTER_RANGE_SELECTION_DLG_H__
#define __GEODA_CENTER_RANGE_SELECTION_DLG_H__

#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h> 
#include "../FramesManagerObserver.h"
#include "../DataViewer/TableStateObserver.h"
#include "../ShapeOperations/WeightsManStateObserver.h"

class FramesManager;
class TableState;
class Project;
class TableInterface;
class WeightsManState;
class WeightsManInterface;

class RangeSelectionDlg: public wxDialog, public FramesManagerObserver,
public TableStateObserver, public WeightsManStateObserver
{    
public:
    RangeSelectionDlg( wxWindow* parent, Project* project,
					   FramesManager* frames_manager, TableState* table_state,
					   const wxString& title = "Selection Tool", 
					   const wxPoint& pos = wxDefaultPosition );
	virtual ~RangeSelectionDlg();

	/** Load XML resources and initialize local-variable widget pointers.
	 Initialize widgets with defaults. */
    void CreateControls();
	void OnFieldChoice( wxCommandEvent& event );
	void OnFieldChoiceTm( wxCommandEvent& event );
	void OnRangeTextChange( wxCommandEvent& event );
	void OnSelRangeClick( wxCommandEvent& event );
	void OnSelUndefClick( wxCommandEvent& event );
	void OnInvertSelClick( wxCommandEvent& event );
	void OnRandomSelClick( wxCommandEvent& event );
	void OnClearSelClick( wxCommandEvent& event );
	void OnAddNeighsToSelClick( wxCommandEvent& event );
	void OnAddField( wxCommandEvent& event );
	void OnSaveFieldChoice( wxCommandEvent& event );
	void OnSaveFieldChoiceTm( wxCommandEvent& event );
	void OnSelCheckBox( wxCommandEvent& event );
	void OnUnselCheckBox( wxCommandEvent& event );
	void OnSelUnselTextChange( wxCommandEvent& event);
	void OnApplySaveClick( wxCommandEvent& event );
	void OnCloseClick( wxCommandEvent& event );
    
    void OnSetNewSelect(wxCommandEvent& event );
    void OnSetSubSelect(wxCommandEvent& event );
    void OnSetAppendSelect(wxCommandEvent& event );

	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; }
	
	/** Implementation of WeightsManStateObserver interface */
	virtual void update(WeightsManState* o);
	virtual int numMustCloseToRemove(boost::uuids::uuid id) const {
		return 0; }
	virtual void closeObserver(boost::uuids::uuid id) {};

private:
	/** Should only be called by update and constructor. */
	void RefreshColIdMap();
	/** Refresh selection vars list and associated time list,
	 enabling widgets as needed. */
	
	/** Refresh m_weights_choice and weights_ids */
	void RefreshWeightsIds();
	boost::uuids::uuid GetWeightsId();
	
	void InitSelectionVars();
	/** Refresh save vars list and associated time list,
	 enabling widgets as needed. */
	void InitSaveVars();
	/** Generic helper method called only by InitSelectionVars
	 and InitSaveVars */
	void InitVars(wxChoice* field, wxChoice* field_tm);
	/** Is Table currently time variant */
	
	bool IsTimeVariant();
	
	/** If Variable choice is valid, enable select all in range and
	 undefined buttons. Also update text to current variable choice. */
	void CheckRangeButtonSettings();
	/** Check that everything is ok to enable the Apply button and
	 enable/disable as needed. */
	void CheckApplySaveSettings();
	
	/** Get the current selection variable col id.  -1 if not found */
	int GetSelColInt();
	/** Get the current selection variable time id.  0 by default */
	int GetSelColTmInt();
	/** Get the current save variable col id.  -1 if not found */
	int GetSaveColInt();
	/** Get the current save variable time id.  0 by default */
	int GetSaveColTmInt();
	
	wxChoice* m_field_choice;
	wxChoice* m_field_choice_tm;
	wxTextCtrl* m_min_text;
	wxStaticText* m_field_static_txt;
	wxStaticText* m_field2_static_txt;
	wxTextCtrl* m_max_text;
	wxButton* m_sel_range_button;
	wxButton* m_sel_undef_button;
	wxButton* m_invert_sel_button;
	wxTextCtrl* m_num_to_rand_sel_txt;
	wxButton* m_random_sel_button;
	wxButton* m_clear_sel_button;
	wxButton* m_add_neighs_to_sel_button;
	wxChoice* m_weights_choice;
	wxChoice* m_save_field_choice;
	wxChoice* m_save_field_choice_tm;
	wxCheckBox* m_sel_check_box;	
	wxTextCtrl* m_sel_val_text;
	wxCheckBox* m_unsel_check_box;	
	wxTextCtrl* m_unsel_val_text;
	wxButton* m_apply_save_button;
    wxRadioButton* m_radio_newselect;
    wxRadioButton* m_radio_subselect;
    wxRadioButton* m_radio_appendselect;
	
	bool m_selection_made; // true once a selection has been made
	bool all_init;

	// col_id_map[i] is a map from the i'th item in the fields drop-down
	// to the actual col_id_map.  Items in the fields dropdown are in the
	// order displayed in wxGrid
	std::vector<int> col_id_map;
	TableInterface* table_int;
	Project* project;
	FramesManager* frames_manager;
	TableState* table_state;
	WeightsManInterface* w_man_int;
	WeightsManState* w_man_state;
	std::vector<boost::uuids::uuid> weights_ids;

	// The last mapped col_id for which selection was applied.
	// This value is used for the save results apply funciton.
	int current_sel_mcol;
	
	DECLARE_EVENT_TABLE()
};

#endif
