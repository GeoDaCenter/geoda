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

#ifndef __GEODA_CENTER_TEMPLATE_FRAME_H__
#define __GEODA_CENTER_TEMPLATE_FRAME_H__

#include <set>
#include <wx/frame.h>
#include "FramesManagerObserver.h"
#include "DataViewer/TableStateObserver.h"
#include "DataViewer/TimeStateObserver.h"

class FramesManager;
class TableState;
class TimeState;
class Project;
class TemplateCanvas;
class TemplateLegend;

/**
 * Common template frame
 */
class TemplateFrame: public wxFrame, public FramesManagerObserver,
	public TableStateObserver, public TimeStateObserver
{
public:
	TemplateFrame(wxFrame *parent, Project* project, const wxString& title,
				  const wxPoint& pos, const wxSize& size, const long style);
	virtual ~TemplateFrame();

	virtual void MapMenus();
	void RegisterAsActive(const wxString& name,
						  const wxString& title = "GeoDa");
	void DeregisterAsActive();
	static wxString GetActiveName();
	static TemplateFrame* GetActiveFrame();
	virtual void OnActivate(wxActivateEvent& event) {}
	
public:
	static bool GetColorFromUser(wxWindow* parent,
								 const wxColour& cur_color,
								 wxColour& ret_color,
								 const wxString& title = "Choose A Color");
	void OnKeyEvent(wxKeyEvent& event);
	virtual void ExportImage(TemplateCanvas* canvas, const wxString& type);
    virtual void OnChangeMapTransparency();
	virtual void OnSaveCanvasImageAs(wxCommandEvent& event);
	virtual void OnCopyLegendToClipboard(wxCommandEvent& event);
	virtual void OnCopyImageToClipboard(wxCommandEvent& event);
    
	virtual void OnLegendUseScientificNotation(wxCommandEvent& event);
	virtual void OnLegendBackgroundColor(wxCommandEvent& event);
	virtual void OnCanvasBackgroundColor(wxCommandEvent& event);
	virtual void OnSelectableFillColor(wxCommandEvent& event);
	virtual void OnSelectableOutlineColor(wxCommandEvent& event);
	virtual void OnUserBackgroundColorVisible(wxCommandEvent& event);
	virtual void OnSelectableOutlineVisible(wxCommandEvent& event);
	virtual void OnHighlightColor(wxCommandEvent& event);
	virtual void OnSelectWithRect(wxCommandEvent& event);
	virtual void OnSelectWithCircle(wxCommandEvent& event);
	virtual void OnSelectWithLine(wxCommandEvent& event);
	virtual void OnSelectionMode(wxCommandEvent& event);
	virtual void OnResetMap(wxCommandEvent& event);
	virtual void OnRefreshMap(wxCommandEvent& event);
	virtual void OnFitToWindowMode(wxCommandEvent& event);
	virtual void OnFixedAspectRatioMode(wxCommandEvent& event);
	virtual void OnSetDisplayPrecision(wxCommandEvent& event);
	virtual void OnZoomMode(wxCommandEvent& event);
	virtual void OnZoomOutMode(wxCommandEvent& event);
	virtual void OnPanMode(wxCommandEvent& event);
	virtual void OnPrintCanvasState(wxCommandEvent& event);
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);
	virtual void UpdateTitle();
	virtual void OnTimeSyncVariable(int var_index);
	virtual void OnFixedScaleVariable(int var_index);
	virtual void OnPlotsPerView(int plots_per_view);
	virtual void OnPlotsPerViewOther();
	virtual void OnPlotsPerViewAll();
	virtual bool IsStatusBarVisible() { return is_status_bar_visible; }
	virtual void OnDisplayStatusBar(wxCommandEvent& event) {
		DisplayStatusBar(!IsStatusBarVisible());
    }
	virtual void DisplayStatusBar(bool show);
	/** Called by TemplateCanvas to determine if TemplateFrame will
	 generate the Status Bar String. */
	virtual bool GetStatusBarStringFromFrame();
	/** Set to true if TemplateFrame implements GetUpdateStatusBarString. */
	virtual void SetGetStatusBarStringFromFrame(bool get_sb_string);
	virtual wxString GetUpdateStatusBarString(const std::vector<int>& hover_obs,
                                              int total_hover_obs);
	virtual Project* GetProject() { return project; }
	/** return value can be null */
	virtual TemplateLegend* GetTemplateLegend() { return template_legend; }
	
	/** Default Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	/** Default Implementation of TableStateObserver interface */
	virtual void update(TableState* o);
	/** Default Implementation of TimeStateObserver interface */
	virtual void update(TimeState* o);
	/** Default Implementation of TableStateObserver interface.  Indicates if
	 frame currently handle changes to time-line.  This is a function
	 of private boolean variables depends_on_non_simple_groups 
	 and supports_timeline_changes.  If supports_timeline_changes is true,
	 then return value is true and depends_on_non_simple_groups is ignored. */
	virtual bool AllowTimelineChanges();
	virtual bool AllowGroupModify(const wxString& grp_nm);
	virtual bool AllowObservationAddDelete() { return false; }
	
	/** Indicate that current view depends on group */
	virtual void AddGroupDependancy(const wxString& grp_nm);
	/** Indicate that current view no longer depends on group */
	virtual void RemoveGroupDependancy(const wxString& grp_nm);
	virtual void ClearAllGroupDependencies();
	
	virtual void SetDependsOnNonSimpleGroups(bool v) {
		depends_on_non_simple_groups = v; }
	
private:
	static TemplateFrame* activeFrame;
	static wxString activeFrName;

protected:
	Project* project;
	TemplateCanvas* template_canvas;
	TemplateLegend* template_legend; // optional
	FramesManager* frames_manager;
	TableState* table_state;
	TimeState* time_state;
	bool is_status_bar_visible;
	bool get_status_bar_string_from_frame;
	
	/** True iff frame depend on multi-time-period variables currently. 
	 If supports_timeline_changes is true, then no need to update this
	 variable. */
	bool depends_on_non_simple_groups;
	/** True iff frame can handle time-line changes such as add/delete/swap. */
	bool supports_timeline_changes;

	std::set<wxString> grp_dependencies;
	
	DECLARE_CLASS(TemplateFrame)
	DECLARE_EVENT_TABLE()
};

#endif
