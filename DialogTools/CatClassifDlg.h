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
#include "../GdaConst.h"

class CatClassifCanvas;
class CatClassifFrame;
class CatClassifPanel;
class TableInterface;
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
	virtual void update(HLStateInt* o);
	virtual wxString GetCanvasTitle();
	virtual void SetCheckMarks(wxMenu* menu);
	virtual void DetermineMouseHoverObjects(wxPoint pt);
	virtual void UpdateSelection(bool shiftdown = false,
								 bool pointsel = false);
	virtual void DrawSelectableShapes(wxMemoryDC &dc);
	virtual void DrawHighlightedShapes(wxMemoryDC &dc);
	
protected:
	virtual void PopulateCanvas();
    void GetBarPositions(std::vector<double>& x_center_pos,
                         std::vector<double>& x_left_pos,
                         std::vector<double>& x_right_pos);
    
public:
	void InitIntervals();
	void UpdateIvalSelCnts();
	static const int max_intervals;
	static const int default_intervals;
	static const double default_min;
	static const double default_max;

	void ChangeAll(Gda::dbl_int_pair_vec_type* new_data,
				   std::vector<double>* new_breaks,
				   std::vector<wxColour>* new_colors);
	static void InitRandNormData(Gda::dbl_int_pair_vec_type& rn_data);
	static void InitUniformData(Gda::dbl_int_pair_vec_type& data,
								double min, double max);
	
protected:
	virtual void UpdateStatusBar();
	
	int num_obs;
	Gda::dbl_int_pair_vec_type* data;
	Gda::dbl_int_pair_vec_type default_data;
	
	AxisScale axis_scale_y;
	AxisScale axis_scale_x;
    
	GdaAxis* y_axis;
	GdaAxis* x_axis;
	
    double max_val;
    double min_val;
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
					wxWindow* parent,
					wxChoice* preview_var_choice,
					wxChoice* preview_var_tm_choice,
					wxCheckBox* sync_vars_chk,
                    bool _useScientificNotation=false,
					wxWindowID id = wxID_ANY,
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize,
					long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE);
	virtual ~CatClassifPanel();
	
	CatClassifState* PromptNew(const CatClassifDef& ccd,
							   const wxString& suggested_title = wxEmptyString,
							   const wxString& field_name = wxEmptyString,
							   int field_tm = 0,
                               bool prompt_title_dlg = true);

	// Top level user actions
	void OnCurCatsChoice(wxCommandEvent& event);
	void OnBreaksChoice(wxCommandEvent& event);
	void OnColorSchemeChoice(wxCommandEvent& event);
	void OnNumCatsChoice(wxCommandEvent& event);
	void OnAssocVarChoice(wxCommandEvent& ev);
	void OnAssocVarTmChoice(wxCommandEvent& ev);
	void OnPreviewVarChoice(wxCommandEvent& ev);
	void OnPreviewVarTmChoice(wxCommandEvent& ev);
	void OnSyncVarsChk(wxCommandEvent& ev);
	void OnUnifDistMinEnter(wxCommandEvent& event);
	void OnUnifDistMinKillFocus(wxFocusEvent& event);
	void OnUnifDistMaxEnter(wxCommandEvent& event);
	void OnUnifDistMaxKillFocus(wxFocusEvent& event);
	void OnAutomaticLabelsCb(wxCommandEvent& event);
	void OnBrkRad(wxCommandEvent& event);
	void OnBrkTxtEnter(wxCommandEvent& event);
	void OnBrkSlider(wxCommandEvent& event);
	void OnScrollThumbRelease(wxScrollEvent& event);
	void OnKillFocusEvent(wxFocusEvent& event);
	void OnCategoryColorButton(wxMouseEvent& event);
	void OnCategoryTitleText(wxCommandEvent& event);
	void OnButtonChangeTitle(wxCommandEvent& event);
	void OnButtonNew(wxCommandEvent& event);
	void OnButtonDelete(wxCommandEvent& event);
	void OnButtonClose(wxCommandEvent& event);

    void OnSaveCategories(wxCommandEvent& event);
    void SaveCategories(const wxString& title,
                        const wxString& label,
                        const wxString& field_default);
    
	void ResetValuesToDefault();
	void EnableControls(bool enable);
	void InitFromCCData();
	void InitAssocVarChoices();
	void InitPreviewVarChoices();
	void InitCurCatsChoices();
	
	int GetNumCats();
	void SetNumCats(int num_cats);
	void ShowNumCategories(int num_cats);
	bool IsAutomaticLabels();
	void SetAutomaticLabels(bool auto_labels);
	CatClassification::ColorScheme GetColorSchemeChoice();
	void SetColorSchemeChoice(CatClassification::ColorScheme cs);
	CatClassification::BreakValsType GetBreakValsTypeChoice();
	void SetBreakValsTypeChoice(CatClassification::BreakValsType bvt);
	wxString GetAssocDbFldNm();
	wxString GetAssocVarChoice();
	wxString GetAssocVarTmChoice();
	int GetAssocVarTmAsInt();
	wxString GetPreviewDbFldNm();
	wxString GetPreviewVarChoice();
	wxString GetPreviewVarTmChoice();
	int GetPreviewVarTmAsInt();
	bool IsSyncVars();
	void SetSyncVars(bool sync_assoc_and_prev_vars);
	bool IsUnifDistMode() { return unif_dist_mode; }
	void SetUnifDistMode(bool enable) { unif_dist_mode = enable; }
	void ShowUnifDistMinMax(bool show);
	void SetUnifDistMinMaxTxt(double min, double max);
	int GetActiveBrkRadio();
	void SetActiveBrkRadio(int);
	void SetBrkTxtFromVec(const std::vector<double>& brks);	
	double GetBrkSliderMin();
	double GetBrkSliderMax();	
	void SetSliderFromBreak(int brk);
	bool IsDuplicateTitle(const wxString& title);
	wxString GetDefaultTitle(const wxString& field_name = wxEmptyString,
							 int field_tm = 0);
	
	bool IsOkToDelete(const wxString& custom_cat_title);
	void UpdateCCState();
	
	/** Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	virtual bool AllowTimelineChanges() { return true; }
	virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
	virtual bool AllowObservationAddDelete() { return true; } //MMM

	CatClassifFrame* template_frame;	
private:
    bool useScientificNotation;
	Project* project;
	TableInterface* table_int;
	CatClassifHistCanvas* hist_canvas;
	CatClassifManager* cat_classif_manager;
	TableState* table_state;
	
	CatClassifState* cc_state;
	wxChoice* cur_cats_choice;
	wxChoice* breaks_choice;
    wxButton* save_categories_button;

	wxButton* change_title_button;
	wxButton* delete_button;
	wxChoice* num_cats_choice;
	wxStaticText* min_lbl;
	wxStaticText* max_lbl;
	wxChoice* color_scheme;
	wxChoice* assoc_var_choice;
	wxChoice* assoc_var_tm_choice;
	wxChoice* preview_var_choice;
	wxChoice* preview_var_tm_choice;
	wxCheckBox* sync_vars_chk;
	wxStaticText* unif_dist_min_lbl;
	wxTextCtrl* unif_dist_min_txt;
	wxStaticText* unif_dist_max_lbl;
	wxTextCtrl* unif_dist_max_txt;
	wxCheckBox* auto_labels_cb;
	wxSlider* brk_slider;
	int last_brk_slider_pos;
	std::vector<wxStaticBitmap*> cat_color_button;
	std::vector<wxTextCtrl*> cat_title_txt;
	std::vector<wxRadioButton*> brk_rad;
	std::vector<wxStaticText*> brk_lbl;
	std::vector<wxTextCtrl*> brk_txt;
	
	int num_obs;
	Gda::dbl_int_pair_vec_type data;
    std::vector<bool> data_undef;
	CatClassifDef cc_data;
	Gda::dbl_int_pair_vec_type preview_data;
	
	static const wxString unif_dist_txt;
	static const int max_intervals;
	static const int default_intervals;
	static const double default_min;
	static const double default_max;
	
	bool all_init;
	bool unif_dist_mode;
	
	DECLARE_EVENT_TABLE()
};


class CatClassifFrame : public TemplateFrame
{
public:
    CatClassifFrame(wxFrame *parent, Project* project,
                    bool useScientificNotation = false,
					const wxString& title = _("Category Editor"),
					const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = GdaConst::cat_classif_default_size,
					const long style = wxDEFAULT_FRAME_STYLE);
	virtual ~CatClassifFrame();
	
	void OnActivate(wxActivateEvent& event);
	void OnPreviewVarChoice(wxCommandEvent& event);
	void OnPreviewVarTmChoice(wxCommandEvent& event);
	void OnSyncVarsChk(wxCommandEvent& event);
	
	/** Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	
	CatClassifState* PromptNew(const CatClassifDef& ccd,
							   const wxString& suggested_title = wxEmptyString,
							   const wxString& field_name = wxEmptyString,
							   int field_tm = 0,
                               bool prompt_title_dlg = true);
	
private:
	CatClassifHistCanvas* canvas;
	CatClassifPanel* panel;

	DECLARE_EVENT_TABLE()
};

#endif
