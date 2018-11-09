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

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/dcmemory.h>
#include <wx/dataobj.h>
#include <wx/dcsvg.h>
#include <wx/dcps.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/xrc/xmlres.h>
#include "DataViewer/TableInterface.h"
#include "DataViewer/TableState.h"
#include "DataViewer/TimeState.h"
#include "FramesManager.h"
#include "TemplateFrame.h"
#include "TemplateCanvas.h"
#include "TemplateLegend.h"
#include "Explore/MapNewView.h"
#include "Explore/CartogramNewView.h"
#include "Explore/ConditionalMapView.h"
#include "Explore/LisaScatterPlotView.h"
#include "Explore/PCPNewView.h"
#include "Explore/ScatterNewPlotView.h"
#include "Explore/MapLayoutView.h"
#include "DialogTools/AdjustYAxisDlg.h"


#include "rc/GeoDaIcon-16x16.xpm"
#include "GeneralWxUtils.h"
#include "GeoDa.h"
#include "Project.h"
#include "logger.h"

IMPLEMENT_CLASS(TemplateFrame, wxFrame)

BEGIN_EVENT_TABLE(TemplateFrame, wxFrame)
	EVT_CHAR_HOOK(TemplateFrame::OnKeyEvent)
END_EVENT_TABLE()

TemplateFrame* TemplateFrame::activeFrame = 0;
wxString TemplateFrame::activeFrName = wxEmptyString;

TemplateFrame::TemplateFrame(wxFrame *parent, Project* project_s,
							 const wxString& title,
							 const wxPoint& pos,
							 const wxSize& size, const long style)
: wxFrame(parent, wxID_ANY, title, pos, size, style),
	template_canvas(0), template_legend(0), project(project_s),
	frames_manager(project_s->GetFramesManager()),
	table_state(project_s->GetTableState()),
	time_state(project_s->GetTimeState()),
	is_status_bar_visible(false),
	get_status_bar_string_from_frame(false),
	supports_timeline_changes(false),
	depends_on_non_simple_groups(true), toolbar(NULL)
{
	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	time_state->registerObserver(this);
}

TemplateFrame::~TemplateFrame()
{
	LOG_MSG("Called TemplateFrame::~TemplateFrame()");
	if (HasCapture()) ReleaseMouse();
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
	time_state->removeObserver(this);
}

void TemplateFrame::OnSelectWithRect(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithRect");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::rectangle);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectWithCircle(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithCircle");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::circle);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectWithLine(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectWithLine");
	if (!template_canvas) return;
	template_canvas->SetBrushType(TemplateCanvas::line);
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectionMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectionMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::select);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnResetMap(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnResetMap");
	if (!template_canvas) return;
	template_canvas->ResetShapes();
	UpdateOptionMenuItems();
}

void TemplateFrame::OnRefreshMap(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnResetMap");
	if (!template_canvas) return;
	template_canvas->ReDraw();
}


void TemplateFrame::OnFitToWindowMode(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnFitToWindowMode");
	if (!template_canvas) return;
	template_canvas->SetFitToWindowMode(
				!template_canvas->GetFitToWindowMode());
	UpdateOptionMenuItems();
	LOG_MSG("Exiting TemplateFrame::OnFitToWindowMode");
}

void TemplateFrame::OnFixedAspectRatioMode(wxCommandEvent& event)
{
	if (!template_canvas) return;
	template_canvas->SetFixedAspectRatioMode(
				!template_canvas->GetFixedAspectRatioMode());	
	UpdateOptionMenuItems();
}

void TemplateFrame::OnSetDisplayPrecision(wxCommandEvent& event)
{
	if (!template_canvas) return;
    
    AxisLabelPrecisionDlg dlg(template_canvas->axis_display_precision, this);
    if (dlg.ShowModal () != wxID_OK)
        return;
    int def_precision = dlg.precision;
    template_canvas->SetDisplayPrecision(def_precision);
    
	UpdateOptionMenuItems();
}

void TemplateFrame::OnZoomMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnZoomMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::zoom);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnZoomOutMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnZoomMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::zoomout);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnPanMode(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnPanMode");
	if (!template_canvas) return;
	template_canvas->SetMouseMode(TemplateCanvas::pan);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnPrintCanvasState(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnPrintCanvasState");
	if (template_canvas) {
	}
}

void TemplateFrame::UpdateOptionMenuItems()
{
	if (template_canvas == 0) return;
	wxMenuBar* mb = GdaFrame::GetGdaFrame()->GetMenuBar();
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_RECT"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::rectangle);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_CIRCLE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::circle);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECT_WITH_LINE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::line);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECTION_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::select);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_FIT_TO_WINDOW_MODE"),
								  template_canvas->GetFitToWindowMode());	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								  template_canvas->GetFixedAspectRatioMode());	
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_ZOOM_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::zoom);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_PAN_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::pan);
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
								  template_canvas->
									IsSelectableOutlineVisible());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_CANVAS_BACKGROUND_COLOR"),
								  template_canvas->
									IsUserBackgroundColorVisible());
	GeneralWxUtils::CheckMenuItem(mb, XRCID("ID_DISPLAY_STATUS_BAR"),
								  IsStatusBarVisible());
}

void TemplateFrame::UpdateContextMenuItems(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_DISPLAY_STATUS_BAR"),IsStatusBarVisible());
	if (template_canvas == 0) return;
	// or are not checkable do not appear.
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_RECT"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::rectangle);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_CIRCLE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::circle);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECT_WITH_LINE"),
								  template_canvas->GetBrushType() ==
								  TemplateCanvas::line);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTION_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::select);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_FIT_TO_WINDOW_MODE"),
								  template_canvas->GetFitToWindowMode());	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_FIXED_ASPECT_RATIO_MODE"),
								  template_canvas->GetFixedAspectRatioMode());	
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_ZOOM_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::zoom);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_PAN_MODE"),
								  template_canvas->GetMouseMode() ==
								  TemplateCanvas::pan);
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_SELECTABLE_OUTLINE_VISIBLE"),
								  template_canvas->
									IsSelectableOutlineVisible());
	GeneralWxUtils::CheckMenuItem(menu, XRCID("ID_CANVAS_BACKGROUND_COLOR"),
								  template_canvas->
									IsUserBackgroundColorVisible());
	
}

void TemplateFrame::UpdateTitle()
{
	if (template_canvas) SetTitle(template_canvas->GetCanvasTitle());
	if (GetActiveFrame()==this) GdaFrame::GetGdaFrame()->SetTitle(GetTitle());
}

void TemplateFrame::OnTimeSyncVariable(int var_index)
{
	if (!template_canvas) return;
	template_canvas->TimeSyncVariableToggle(var_index);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnFixedScaleVariable(int var_index)
{
	if (!template_canvas) return;
	template_canvas->FixedScaleVariableToggle(var_index);
	UpdateOptionMenuItems();
}

void TemplateFrame::OnPlotsPerView(int plots_per_view)
{
	if (!template_canvas) return;
	template_canvas->PlotsPerView(plots_per_view);
	UpdateOptionMenuItems();
	UpdateTitle();
}

void TemplateFrame::OnPlotsPerViewOther()
{
	if (!template_canvas) return;
	template_canvas->PlotsPerViewOther();
	UpdateOptionMenuItems();
	UpdateTitle();
}

void TemplateFrame::OnPlotsPerViewAll()
{
	if (!template_canvas) return;
	template_canvas->PlotsPerViewAll();
	UpdateOptionMenuItems();
	UpdateTitle();
}

void TemplateFrame::DisplayStatusBar(bool show)
{
	LOG_MSG("Entering TemplateFrame::DisplayStatusBar");
	wxStatusBar* sb = 0;
	if (!is_status_bar_visible && show) {
		is_status_bar_visible = true;
		if (!GetStatusBar()) {
			sb = new wxStatusBar(this);
			SetStatusBar(sb);
		}
		SendSizeEvent();
	} else if (is_status_bar_visible && !show) {
		is_status_bar_visible = false;
		sb = GetStatusBar();
		if (sb) {
			SetStatusBar(0);
			delete sb;
		}
		SendSizeEvent();
	}
	LOG_MSG("Exiting TemplateFrame::DisplayStatusBar");
}

bool TemplateFrame::GetStatusBarStringFromFrame()
{
	return get_status_bar_string_from_frame;
}

void TemplateFrame::SetGetStatusBarStringFromFrame(bool get_sb_string)
{
	get_status_bar_string_from_frame = get_sb_string;
}

wxString TemplateFrame::GetUpdateStatusBarString(const std::vector<int>& hover_obs, int total_hover_obs)
{
	return "";
}

void TemplateFrame::RegisterAsActive(const wxString& name,
									 const wxString& title)
{
	if (!GdaFrame::GetGdaFrame()) {
		// ~GdaFrame() has been called, program is exiting
		return;
	}
	if (!activeFrame || activeFrame != this) {
		if (activeFrame && activeFrame != this)
			activeFrame->DeregisterAsActive();
		MapMenus();
		// Enable the Close menu item.  This saves including this code in every
		// single MapMenus call by all classes that inherit this class.
		GeneralWxUtils::EnableMenuItem(GdaFrame::GetGdaFrame()->GetMenuBar(),
									   GdaFrame::GetGdaFrame()->GetMenuBar()->GetMenuLabelText(0),
									   wxID_CLOSE, true);
		activeFrame = this;
		activeFrName = name;
		GdaFrame::GetGdaFrame()->SetTitle(title);
	}
}

void TemplateFrame::DeregisterAsActive()
{
	LOG_MSG("In TemplateFrame::DeregisterAsActive");
	if (!GdaFrame::GetGdaFrame()) {
		// ~GdaFrame() has been called, program is exiting
		return;
	}
	if (activeFrame == this) {
		activeFrame = NULL;
		activeFrName = wxEmptyString;
		// restore to a default state.
		GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
	}
}

wxString TemplateFrame::GetActiveName()
{
	return activeFrName;
}

TemplateFrame* TemplateFrame::GetActiveFrame()
{
	return activeFrame;
}

void TemplateFrame::MapMenus()
{
}

void TemplateFrame::OnKeyEvent(wxKeyEvent& event)
{
	//LOG_MSG("In TemplateFrame::OnKeyEvent");
	if (event.GetModifiers() == wxMOD_CMD &&
		project && project->GetTimeState() &&
		project->GetTableInt()->IsTimeVariant() &&
		(event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_RIGHT)) {
		TimeState* time_state = project->GetTimeState();
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


void TemplateFrame::ExportImage(TemplateCanvas* canvas, const wxString& type)
{
	wxLogMessage("Entering TemplateFrame::ExportImage");
    wxBitmap* main_map = template_canvas->GetPrintLayer();
    int canvas_width = main_map->GetWidth();
    int canvas_height = main_map->GetHeight();
    double scale = template_canvas->GetContentScaleFactor();
    
    if (GdaConst::enable_high_dpi_support) {
        canvas_width = canvas_width / scale;
        canvas_height = canvas_height / scale;
    }
    
    if (template_legend ) {
        // with legend
        // try to keep maplayout dialog fixed size
        int dlg_width = 900;
        int dlg_height = dlg_width * canvas_height / (double)canvas_width + 160;
        wxSize sz(dlg_width, dlg_height);
        wxString title = project->GetProjectTitle();
        MapCanvas* map_canvas = dynamic_cast<MapCanvas*>(template_canvas);
        CanvasLayoutDialog* lo_dlg;
        if (map_canvas) {
            lo_dlg = new MapLayoutDialog(title, template_legend, template_canvas, _("Map Layout Preview"), wxDefaultPosition, sz);
        } else {
            lo_dlg = new CanvasLayoutDialog(title, template_legend, template_canvas, _("Canvas Layout Preview"), wxDefaultPosition, sz);
        }
        lo_dlg->ShowModal();
        delete lo_dlg;
        
    } else {
        // without legend
        int default_width = canvas_width*2;
        int default_height = canvas_height*2;
        
        CanvasExportSettingDialog setting_dlg(default_width, default_height, _("Image Dimension Setting"));
        if (setting_dlg.ShowModal() == wxID_OK) {
            int out_res_x = setting_dlg.GetMapWidth();
            int out_res_y = setting_dlg.GetMapHeight();
            double canvas_scale = (double) out_res_x / canvas_width;
            
            
            wxString default_fname(project->GetProjectTitle());
            wxString filter ="BMP|*.bmp|PNG|*.png|SVG|*.svg";
            int filter_index = 1;
            wxFileDialog dialog(canvas, _("Save Image to File"), wxEmptyString,
                                default_fname, filter,
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            dialog.SetFilterIndex(filter_index);
            if (dialog.ShowModal() != wxID_OK) {
                return;
            }
            wxFileName fname = wxFileName(dialog.GetPath());
            wxString str_fname = fname.GetPathWithSep() + fname.GetName();
            
            switch (dialog.GetFilterIndex()) {
                case 0:
                {
                    wxLogMessage("BMP selected");
                    wxBitmap canvas_bm;
                    canvas_bm.CreateScaled(canvas_width, canvas_height, 32, canvas_scale);
                    wxMemoryDC canvas_dc(canvas_bm);
                    canvas_dc.SetBackground(*wxWHITE_BRUSH);
                    canvas_dc.Clear();
                    template_canvas->RenderToDC(canvas_dc, out_res_x, out_res_y);
                    canvas_bm.SaveFile(str_fname + ".bmp", wxBITMAP_TYPE_PNG);
                }
                    break;
                case 1:
                {
                    wxLogMessage("PNG selected");
                    wxBitmap canvas_bm;
                    canvas_bm.CreateScaled(canvas_width, canvas_height, 32, canvas_scale);
                    wxMemoryDC canvas_dc(canvas_bm);
                    canvas_dc.SetBackground(*wxWHITE_BRUSH);
                    canvas_dc.Clear();
                    template_canvas->RenderToDC(canvas_dc, out_res_x, out_res_y);
                    canvas_bm.SaveFile(str_fname + ".png", wxBITMAP_TYPE_PNG);
                }
                    break;
                    
                case 2:
                {
                    wxLogMessage("SVG selected");
                    wxSVGFileDC dc(str_fname + ".svg", out_res_x, out_res_y);
                    template_canvas->RenderToSVG(dc, out_res_x, out_res_y);
                }
                    break;
                default:
                {
                }
                    break;
            }
        }
    }
    
	wxLogMessage("Exiting TemplateFrame::ExportImage");
}

void TemplateFrame::OnChangeMapTransparency()
{
    // should be overrided.
}

void TemplateFrame::OnSaveCanvasImageAs(wxCommandEvent& event)
{
	if (!template_canvas) return;
    
	ExportImage(template_canvas, activeFrName);
}

void TemplateFrame::OnCopyLegendToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnCopyLegendToClipboard");
	if (!template_legend) return;
    wxSize sz = template_legend->GetVirtualSize();
	
    wxBitmap bitmap(sz.x, sz.y);
	
    wxMemoryDC dc;
    dc.SelectObject(bitmap);
	
    wxBrush brush;
    brush.SetColour(template_legend->GetBackgroundColour());
    dc.SetBrush(brush);
	dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
	dc.SetPen(*wxBLACK_PEN);
    template_legend->OnDraw(dc);
    dc.SelectObject(wxNullBitmap);
	
    if (!wxTheClipboard->Open()) {
        wxMessageBox("Can't open clipboard.");
    } else {
        wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
        wxTheClipboard->Close();
    }
	LOG_MSG("Exiting TemplateFrame::OnCopyLegendToClipboard");
}


void TemplateFrame::OnCopyImageToClipboard(wxCommandEvent& event)
{
	LOG_MSG("Entering TemplateFrame::OnCopyImageToClipboard");
	if (!template_canvas) return;
	wxSize sz = template_canvas->GetVirtualSize();
		
	wxBitmap bitmap( sz.x, sz.y );
	
	wxMemoryDC dc;
	dc.SelectObject( bitmap );
	dc.DrawBitmap(*template_canvas->GetLayer2(), 0, 0);
	dc.SelectObject( wxNullBitmap );

	if ( !wxTheClipboard->Open() ) {
		wxMessageBox("Can't open clipboard.");
	} else {
		wxTheClipboard->AddData(new wxBitmapDataObject(bitmap));
		wxTheClipboard->Close();
	}
	LOG_MSG("Exiting TemplateFrame::OnCopyImageToClipboard");
}

void TemplateFrame::OnLegendUseScientificNotation(wxCommandEvent& event)
{
    bool flag = template_canvas->useScientificNotation;
  
    
    template_canvas->SetScientificNotation(!flag);
    if (MapCanvas* canvas = dynamic_cast<MapCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    } else if (CartogramNewCanvas* canvas = dynamic_cast<CartogramNewCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    } else if (CartogramNewCanvas* canvas = dynamic_cast<CartogramNewCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    } else if (ConditionalMapCanvas* canvas = dynamic_cast<ConditionalMapCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    } else if (PCPCanvas* canvas = dynamic_cast<PCPCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    } else if (ScatterNewPlotCanvas* canvas = dynamic_cast<ScatterNewPlotCanvas*>(template_canvas)) {
        canvas->CreateAndUpdateCategories();
    }
    template_legend->Recreate();
    UpdateOptionMenuItems();
}

void TemplateFrame::OnLegendBackgroundColor(wxCommandEvent& event)
{
	if (!template_legend) return;
    wxColour col = template_legend->GetBackgroundColour();
	
    wxColourData data;
    data.SetColour(col);
    data.SetChooseFull(true);
    for (int i = 0; i < 16; i++) {
        wxColour colour(i*16, i*16, i*16);
        data.SetCustomColour(i, colour);
    }
	
    wxColourDialog dialog(this, &data);
    dialog.SetTitle("Legend Background Color");
    if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        col = retData.GetColour();
        template_legend->SetBackgroundColour(col);
        template_legend->Refresh();
    }
    UpdateOptionMenuItems();
}


bool TemplateFrame::GetColorFromUser(wxWindow* parent,
									 const wxColour& cur_color,
									 wxColour& ret_color,
									 const wxString& title)
{
	wxColourData data;
	data.SetColour(cur_color);
	data.SetChooseFull(true);
	for (int i=0; i<16; i++) {
		wxColour color(i*16, i*16, i*16);
		data.SetCustomColour(i, color);
	}
	
	wxColourDialog dialog(parent, &data);
	dialog.SetTitle(title);
	if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        ret_color = retData.GetColour();
		return true;
    }
	return false;
}

void TemplateFrame::OnCanvasBackgroundColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnCanvasBackgroundColor");
	if (!template_canvas) return;
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->canvas_background_color,
						  new_color,
						  "Background Color") ) {
		template_canvas->SetCanvasBackgroundColor(new_color);
	}
    UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectableFillColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineColor");
	if (!template_canvas) return;
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->selectable_fill_color,
						  new_color,
						  "Fill Color") ) {
		template_canvas->SetSelectableFillColor(new_color);
	}
    UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectableOutlineColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineColor");
	if (!template_canvas) return;
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->selectable_outline_color,
						  new_color,
						  "Outline Color") ) {
		template_canvas->SetSelectableOutlineColor(new_color);
	}
    UpdateOptionMenuItems();
}

void TemplateFrame::OnUserBackgroundColorVisible(wxCommandEvent& event)
{
	if (!template_canvas) return;
	template_canvas->SetBackgroundColorVisible(
						!template_canvas->user_canvas_background_color);
    UpdateOptionMenuItems();
}

void TemplateFrame::OnSelectableOutlineVisible(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnSelectableOutlineVisible");
	if (!template_canvas) return;
	template_canvas->SetSelectableOutlineVisible(
						!template_canvas->selectable_outline_visible);
    UpdateOptionMenuItems();
}

void TemplateFrame::OnHighlightColor(wxCommandEvent& event)
{
	LOG_MSG("Called TemplateFrame::OnHighlightColor");
	if (!template_canvas) return;
	wxColour new_color;
	if ( GetColorFromUser(this,
						  template_canvas->highlight_color,
						  new_color,
						  "Highlight Color") ) {
		template_canvas->SetHighlightColor(new_color);
	}
    UpdateOptionMenuItems();
}

void TemplateFrame::update(FramesManager* o)
{
}

void TemplateFrame::update(TableState* o)
{
}

void TemplateFrame::update(TimeState* o)
{
}

bool TemplateFrame::AllowTimelineChanges()
{
	if (supports_timeline_changes) return true;
	return !depends_on_non_simple_groups;
}

bool TemplateFrame::AllowGroupModify(const wxString& grp_nm)
{
	return (grp_dependencies.find(grp_nm) == grp_dependencies.end());
}

void TemplateFrame::AddGroupDependancy(const wxString& grp_nm)
{
	grp_dependencies.insert(grp_nm);
}

void TemplateFrame::RemoveGroupDependancy(const wxString& grp_nm)
{
	grp_dependencies.erase(grp_nm);
}

void TemplateFrame::ClearAllGroupDependencies()
{
	grp_dependencies.clear();
}

int TemplateFrame::GetCurrentCanvasTimeStep()
{
    return template_canvas ? template_canvas->cat_data.GetCurrentCanvasTmStep() : -1;
}

