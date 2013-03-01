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

#ifndef __GEODA_TEMPLATE_FRAME_H__
#define __GEODA_TEMPLATE_FRAME_H__

#include <list>
#include <wx/frame.h>
#include "FramesManagerObserver.h"
class FramesManager;
class Project;
class TemplateCanvas;
class TemplateLegend;

/**
 * Common template frame
 */
class TemplateFrame: public wxFrame, public FramesManagerObserver
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
	
public:
	static bool GetColorFromUser(wxWindow* parent,
								 const wxColour& cur_color,
								 wxColour& ret_color,
								 const wxString& title = "Choose A Color");
	
	void OnKeyEvent(wxKeyEvent& event);
	virtual void ExportImage(TemplateCanvas* canvas, const wxString& type);
	virtual void OnSaveCanvasImageAs(wxCommandEvent& event);
	virtual void OnCopyLegendToClipboard(wxCommandEvent& event);
	virtual void OnCopyImageToClipboard(wxCommandEvent& event);
	virtual void OnLegendBackgroundColor(wxCommandEvent& event);
	virtual void OnCanvasBackgroundColor(wxCommandEvent& event);
	virtual void OnSelectableFillColor(wxCommandEvent& event);
	virtual void OnSelectableOutlineColor(wxCommandEvent& event);
	virtual void OnSelectableOutlineVisible(wxCommandEvent& event);
	virtual void OnHighlightColor(wxCommandEvent& event);
	virtual void OnSelectWithRect(wxCommandEvent& event);
	virtual void OnSelectWithCircle(wxCommandEvent& event);
	virtual void OnSelectWithLine(wxCommandEvent& event);
	virtual void OnSelectionMode(wxCommandEvent& event);
	virtual void OnFitToWindowMode(wxCommandEvent& event);
	virtual void OnFixedAspectRatioMode(wxCommandEvent& event);
	virtual void OnZoomMode(wxCommandEvent& event);
	virtual void OnPanMode(wxCommandEvent& event);
	virtual void OnPrintCanvasState(wxCommandEvent& event);
	virtual void UpdateOptionMenuItems();
	virtual void UpdateContextMenuItems(wxMenu* menu);
	virtual void UpdateTitle() {};
	virtual void OnTimeSyncVariable(int var_index);
	virtual void OnFixedScaleVariable(int var_index);
	virtual void OnPlotsPerView(int plots_per_view);
	virtual void OnPlotsPerViewOther();
	virtual void OnPlotsPerViewAll();
	virtual bool IsStatusBarVisible() { return is_status_bar_visible; }
	virtual void OnDisplayStatusBar(wxCommandEvent& event) {
		DisplayStatusBar(!IsStatusBarVisible()); }
	virtual void DisplayStatusBar(bool show);
	virtual Project* GetProject() { return project; }
	/** return value can be null */
	virtual TemplateLegend* GetTemplateLegend() { return template_legend; }
	
	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
private:
	static TemplateFrame* activeFrame;
	static wxString activeFrName;
protected:
	Project* project;
	TemplateCanvas* template_canvas;
	TemplateLegend* template_legend; // optional
	FramesManager* frames_manager;
	bool is_status_bar_visible;

	DECLARE_CLASS(TemplateFrame)
	DECLARE_EVENT_TABLE()
};

#endif
