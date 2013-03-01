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

#ifndef __GEODA_CENTER_CAT_CLASSIF_DLG_H__
#define __GEODA_CENTER_CAT_CLASSIF_DLG_H__

#include <boost/multi_array.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include "../TemplateCanvas.h"
#include "../TemplateFrame.h"
#include "../DataViewer/TableStateObserver.h"
#include "../Explore/CatClassification.h"
#include "../Explore/CatClassifManager.h"
#include "../Explore/CatClassifStateObserver.h"
#include "../FramesManagerObserver.h"
#include "../GenUtils.h"
#include "../GeoDaConst.h"

class CatClassifCanvas;
class CatClassifFrame;
class CatClassifPanel;
class DbfGridTableBase;
class FramesManager;
class HighlightState;
class TableState;

class CatClassifHistCanvas : public TemplateCanvas {
public:
	CatClassifHistCanvas(wxWindow *parent, TemplateFrame* t_frame,
						 Project* project,
						 const wxPoint& pos = wxDefaultPosition,
						 const wxSize& size = wxDefaultSize);
	virtual ~CatClassifHistCanvas();
	virtual void DisplayRightClickMenu(const wxPoint& pos);
	virtual void update(HighlightState* o);
	virtual wxString GetCanvasTitle();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void DetermineMouseHoverObjects();
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
private:
	virtual void PopulateCanvas();
	
public:
	void InitIntervals();
	void UpdateIvalSelCnts();
	static const int max_intervals;
	static const int default_intervals;

	void ChangeData(GeoDa::dbl_int_pair_vec_type* new_data);
	void ChangeBreaks(std::vector<double>* new_breaks,
					  std::vector<wxColour>* new_colors);
	void ChangeColorScheme(std::vector<wxColour>* new_colors);
	static void InitRandNormData(GeoDa::dbl_int_pair_vec_type& rn_data);
	
private:
	virtual void UpdateStatusBar();
	
	Project* project;
	HighlightState* highlight_state;
	int num_obs;
	GeoDa::dbl_int_pair_vec_type* data;
	GeoDa::dbl_int_pair_vec_type default_data;
	
	AxisScale axis_scale_y;
	MyAxis* y_axis;
	
	std::vector<wxColour>* colors; // size = cur_num_intervals
	std::vector<wxColour> default_colors;
	std::vector<double>* breaks; // size = cur_num_intervals-1
	std::vector<double> default_breaks;
	double max_num_obs_in_ival;
	int cur_intervals;
	std::vector<int> ival_obs_cnt; // size = cur_num_intervals
	std::vector<int> ival_obs_sel_cnt;  // size = cur_num_intervals
	std::vector<int> obs_id_to_ival; // size = num_obs
	std::vector<std::list<int> > ival_to_obs_ids;
	
	static const double left_pad_const;
	static const double right_pad_const;
	static const double interval_width_const;
	static const double interval_gap_const;
	
	DECLARE_EVENT_TABLE()
};


class CatClassifPanel: public wxPanel, public TableStateObserver
{
public:
    CatClassifPanel(Project* project,
					CatClassifHistCanvas* hist_canvas,
					wxWindow* parent, wxWindowID id = wxID_ANY,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxSYSTEM_MENU);
	virtual ~CatClassifPanel();
	
	CatClassifState* PromptNew(const CatClassifDef& ccd,
							   const wxString& suggested_title = wxEmptyString,
							   const wxString& field_name = wxEmptyString,
							   int field_tm = 0);
	void EditExisting(const wxString& cat_classif_title,
					  const wxString& field_name = wxEmptyString,
					  int field_tm = 0);
	
	void OnKillFocusEvent(wxFocusEvent& event);
	void OnCurCatsChoice(wxCommandEvent& event);
	void OnColorScheme(wxCommandEvent& event);
	void OnNumCatsSpinCtrl(wxSpinEvent& event);
	void InitFieldChoices();
	void OnFieldChoice(wxCommandEvent& ev);
	void OnFieldChoiceTm(wxCommandEvent& ev);
	void InitNewFieldChoice();
	void InitCurCatsChoices();
	void OnBrkSlider(wxCommandEvent& event);
	void OnScrollThumbRelease(wxScrollEvent& event);
	void OnCatBut(wxMouseEvent& event);
	void OnCatTxt(wxCommandEvent& event);
	void OnBrkRad(wxCommandEvent& event);
	void OnBrkTxtEnter(wxCommandEvent& event);
	void OnButtonCopyFromExisting(wxCommandEvent& event);
	void OnButtonChangeTitle(wxCommandEvent& event);
	void OnButtonNew(wxCommandEvent& event);
	void OnButtonDelete(wxCommandEvent& event);
	void OnButtonClose(wxCommandEvent& event);
	bool IsDuplicateTitle(const wxString& title);
	wxString GetDefaultTitle(const wxString& field_name = wxEmptyString,
							 int field_tm = 0);
	void EnableControls(bool enable);
	void ResetValuesToDefault();
	void InitFromCCData();
	bool IsOkToDelete(const wxString& custom_cat_title);
	
	void CopyFromExisting(CatClassification::CatClassifType new_theme);
	void InitSliderFromBreak(int brk);
	void ChangeNumCats(int new_num_cats);
	int GetActiveBrkRadio();
	void UpdateBrkTxtRad(int active_brk);
	void UpdateBrkSliderRanges();
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	
	int num_obs;
	GeoDa::dbl_int_pair_vec_type data;
	CatClassifDef cc_data;
	
	static const int max_intervals;
	static const int default_intervals;
	int cur_intervals;
	
	CatClassifFrame* template_frame;
private:
	Project* project;
	DbfGridTableBase* grid_base;
	CatClassifHistCanvas* hist_canvas;
	CatClassifManager* cat_classif_manager;
	TableState* table_state;
	
	void ShowNumCategories(int num_cats);
	
	void UpdateCCState();
	CatClassifState* cc_state;
	wxChoice* cur_cats_choice;
	wxButton* copy_from_existing_button;
	wxButton* change_title_button;
	wxButton* delete_button;
	wxSpinCtrl* num_cats_spin_ctrl;
	wxStaticText* min_lbl;
	wxStaticText* max_lbl;
	wxChoice* color_scheme;
	wxString cur_field_choice;
	int cur_field_choice_tm;
	wxChoice* field_choice;
	wxChoice* field_choice_tm;
	wxSlider* brk_slider;
	int last_brk_slider_pos;
	std::vector<wxStaticBitmap*> cat_but;
	std::vector<wxTextCtrl*> cat_txt;
	std::vector<wxRadioButton*> brk_rad;
	std::vector<wxStaticText*> brk_lbl;
	std::vector<wxTextCtrl*> brk_txt;
	std::vector<double> brk_slider_min;
	std::vector<double> brk_slider_max;
	bool all_init;
	
	DECLARE_EVENT_TABLE()
};

class CatClassifFrame : public TemplateFrame
{
public:
    CatClassifFrame(wxFrame *parent, Project* project,
					const wxString& title = "Category Editor",
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GeoDaConst::cat_classif_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~CatClassifFrame();
	
	void OnActivate(wxActivateEvent& event);

	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);

	void OnThemeless(wxCommandEvent& event);
	void OnQuantile(wxCommandEvent& event);
	void OnPercentile(wxCommandEvent& event);
	void OnHinge15(wxCommandEvent& event);
	void OnHinge30(wxCommandEvent& event);
	void OnStdDevMap(wxCommandEvent& event);
	void OnUniqueValues(wxCommandEvent& event);
	void OnNaturalBreaks(wxCommandEvent& event);
	void OnEqualIntervals(wxCommandEvent& event);
	
	CatClassifState* PromptNew(const CatClassifDef& ccd,
							   const wxString& suggested_title = wxEmptyString,
							   const wxString& field_name = wxEmptyString,
							   int field_tm = 0);
	void EditExisting(const wxString& cat_classif_title,
					  const wxString& field_name = wxEmptyString,
					  int field_tm = 0);
	
private:
	void ChangeThemeType(CatClassification::CatClassifType new_theme);
	
	wxSplitterWindow* splitter;
	CatClassifHistCanvas* canvas;
	CatClassifPanel* panel;
	
	DECLARE_EVENT_TABLE()
};

#endif
