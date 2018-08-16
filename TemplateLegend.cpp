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

#include <wx/colordlg.h>
#include <wx/colourdata.h>
#include <wx/xrc/xmlres.h>
#include <wx/dcgraph.h>
#include <wx/statbox.h>

#include <wx/button.h>

#include "logger.h"
#include "GdaConst.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"
#include "TemplateLegend.h"
#include "Explore/MapNewView.h"


PointRadiusDialog::PointRadiusDialog(const wxString & title, int r)
: wxDialog(NULL, -1, title, wxDefaultPosition, wxSize(250, 160))
{
    
    wxPanel *panel = new wxPanel(this, -1);
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    
    wxBoxSizer *sbox = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *lbl = new wxStaticText(panel, -1, _("Point Radius:"));
    wxString s_radius;
    if (r > 0) s_radius << r;
    else s_radius = "2";
    rb = new wxSpinCtrl(panel, -1, s_radius);
    sbox->Add(lbl);
    sbox->Add(rb);
    wxBoxSizer* panel_v_szr = new wxBoxSizer(wxVERTICAL);
    panel_v_szr->Add(sbox, 1, wxALL|wxEXPAND, 5);
    panel->SetSizer(panel_v_szr);
    
    wxButton *okButton = new wxButton(this, wxID_OK, _("Ok"),
                                      wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, _("Close"),
                                         wxDefaultPosition, wxSize(70, 30));
    
    hbox->Add(okButton, 1);
    hbox->Add(closeButton, 1, wxLEFT, 5);
    
    vbox->Add(panel, 1, wxALL | wxEXPAND, 30);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    
    
    SetSizer(vbox);
    SetAutoLayout(true);
    vbox->Fit(this);
    Centre();
}


int PointRadiusDialog::GetRadius()
{
    return rb->GetValue();
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

GdaLegendLabel::GdaLegendLabel(int _idx, wxString _text, wxPoint _pos, wxSize _sz)
: d_rect(20)
{
    idx = _idx;
    text = _text;
    position = _pos;
    size = _sz;
    isMoving = false;
    tmp_position = position;
    bbox = wxRect(_pos.x, _pos.y + idx * d_rect, _sz.GetWidth(), _sz.GetHeight());
}

GdaLegendLabel::~GdaLegendLabel()
{
    
}

int GdaLegendLabel::getWidth()
{
    return position.x + size.GetWidth();
}

int GdaLegendLabel::getHeight()
{
    return position.y + size.GetHeight();
}

const wxRect& GdaLegendLabel::getBBox()
{
    return bbox;
}

void GdaLegendLabel::move(const wxPoint& new_pos)
{
    tmp_position = new_pos;
}

void GdaLegendLabel::reset()
{
    tmp_position = position;
}

bool GdaLegendLabel::intersect( GdaLegendLabel& another_lbl)
{
    wxRect tmp_bbox(tmp_position.x, tmp_position.y, size.GetWidth(), size.GetHeight());
    return tmp_bbox.Intersects(another_lbl.getBBox());
}

bool  GdaLegendLabel::contains(const wxPoint& cur_pos)
{
    return bbox.Contains(cur_pos);
}

void GdaLegendLabel::draw(wxDC& dc, int cur_idx)
{
    dc.DrawText(text, position.x, position.y + d_rect * cur_idx);
    //dc.DrawRectangle(bbox);
    //bbox = wxRect(position.x, position.y + d_rect * cur_idx, size.GetWidth(), size.GetHeight());
}

void GdaLegendLabel::drawMove(wxDC& dc)
{
    //wxPen pen(*wxBLACK, 1, wxDOT);
    //dc.SetPen(pen);
    //dc.SetBrush(*wxTRANSPARENT_BRUSH);
    //dc.DrawRectangle(position.x, tmp_position.y, size.GetWidth(), size.GetHeight());
    //dc.SetPen(*wxBLACK_PEN);
    dc.DrawText(text, position.x, tmp_position.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////////////////////////
const int TemplateLegend::ID_CATEGORY_COLOR = wxID_HIGHEST + 1;

IMPLEMENT_ABSTRACT_CLASS(TemplateLegend, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateLegend, wxScrolledWindow)
	EVT_MENU(TemplateLegend::ID_CATEGORY_COLOR, TemplateLegend::OnCategoryColor)
    EVT_MENU(XRCID("IDC_CHANGE_POINT_RADIUS"), TemplateLegend::OnChangePointRadius)
	EVT_MOUSE_EVENTS(TemplateLegend::OnEvent)
END_EVENT_TABLE()

TemplateLegend::TemplateLegend(wxWindow *parent,
							   TemplateCanvas* template_canvas_s,
							   const wxPoint& pos, const wxSize& size)
: wxScrolledWindow(parent, wxID_ANY, pos, size,
				   wxBORDER_SUNKEN | wxVSCROLL | wxHSCROLL),
legend_background_color(GdaConst::legend_background_color),
template_canvas(template_canvas_s),
isLeftDown(false),
select_label(NULL),
recreate_labels(false),
isDragDropAllowed(false)
{
	SetBackgroundColour(GdaConst::legend_background_color);
    d_rect = 20;
    px = 10;
    py = 40;
    m_w = 15;
    m_l = 20;
    
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    box->Add(this, 1, wxEXPAND|wxALL);
    parent->SetSizer(box);
}

TemplateLegend::~TemplateLegend()
{
    for (int i=0; i<labels.size(); i++) {
        delete labels[i];
    }
    labels.clear();
}

void TemplateLegend::OnEvent(wxMouseEvent& event)
{
	if (!template_canvas) return;
	
	int cat_clicked = GetCategoryClick(event);
	
    if (event.RightUp()) {
        wxMenu* optMenu = wxXmlResource::Get()->LoadMenu("ID_MAP_VIEW_MENU_LEGEND");
		AddCategoryColorToMenu(optMenu, cat_clicked);
    	wxMenuItem* mi = optMenu->FindItem(XRCID("ID_LEGEND_USE_SCI_NOTATION"));
    	if (mi && mi->IsCheckable()) {
            mi->Check(template_canvas->useScientificNotation);
        }
        PopupMenu(optMenu, event.GetPosition());
        return;
    }
	
    if (isDragDropAllowed == false) {
        if (cat_clicked != -1 && event.LeftUp()) {
            SelectAllInCategory(cat_clicked, event.ShiftDown());
        }
        return;
    }
    
    if (event.LeftDown()) {
        isLeftDown = true;
        for (int i=0;i<labels.size();i++) {
            if (labels[i]->contains(event.GetPosition())){
                select_label = labels[i];
                break;
            }
        }
    } else if (event.Dragging()) {
        if (isLeftDown) {
            isLeftMove = true;
            // moving
            if (select_label) {
                select_label->move(event.GetPosition());
                for (int i=0; i<labels.size(); i++) {
                    if (labels[i] != select_label) {
                        if (select_label->intersect(*labels[i])){
                            LOG_MSG("intersect");
                            // exchange labels only
                            new_order.clear();
                            for (int j=0; j<labels.size();j++) new_order.push_back(j);
                            int from = select_label->idx;
                            int to = labels[i]->idx;
                            // get new order
                            new_order[from] = to;
                            new_order[to] = from;
                            break;
                        }
                    } else {
                        LOG_MSG("other");
                        new_order.clear();
                        for (int j=0; j<labels.size();j++) new_order.push_back(j);
                        
                    }
                }
            }
            Refresh();
        }
    } else if (event.LeftUp()) {
        if (isLeftMove) {
            isLeftMove = false;
            // stop move
            if (select_label) {
                for (int i=0; i<labels.size(); i++) {
                    if (labels[i] != select_label) {
                        if (select_label->intersect(*labels[i])){
                            // exchange labels applying o map
                            int from = select_label->idx;
                            int to = labels[i]->idx;
                            template_canvas->cat_data.ExchangeLabels(from, to);
                            template_canvas->ResetShapes();
                            recreate_labels = true;
                            break;
                        }
                    }
                }
            }
            select_label = NULL;
            Refresh();
        } else {
            // only left click
            if (cat_clicked != -1) {
                SelectAllInCategory(cat_clicked, event.ShiftDown());
            }
        }
        isLeftDown = false;
    }
}

int TemplateLegend::GetCategoryClick(wxMouseEvent& event)
{
	wxPoint pt(event.GetPosition());
	int x, y;
	CalcUnscrolledPosition(pt.x, pt.y, &x, &y);
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	
	int cur_y = py;
	for (int i = 0; i<num_cats; i++) {
		if ((x > px) && (x < px + m_l) &&
			(y > cur_y - 8) && (y < cur_y + m_w))
		{
			return i;
		} else if ((x > px + m_l) &&
				   (y > cur_y - 8) && (y < cur_y + m_w)) {
			return -1;
		}
		cur_y += d_rect;
	}
	
	return -1;
}

void TemplateLegend::AddCategoryColorToMenu(wxMenu* menu, int cat_clicked)
{
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	if (cat_clicked < 0 || cat_clicked >= num_cats) return;
	wxString s;
	s << _("Color for Category");
	wxString cat_label = template_canvas->cat_data.GetCategoryLabel(c_ts, cat_clicked);
	if (!cat_label.IsEmpty())
        s << ": " << cat_label;
    if ( template_canvas->GetShapeType() == TemplateCanvas::points) {
        menu->Prepend(XRCID("IDC_CHANGE_POINT_RADIUS"), _("Change Point Radius"), "");
    }
	menu->Prepend(ID_CATEGORY_COLOR, s, s);
	opt_menu_cat = cat_clicked;
}

void TemplateLegend::OnChangePointRadius(wxCommandEvent& event)
{
    int old_radius = template_canvas->GetPointRadius();
    PointRadiusDialog dlg(_("Change Point Radius"), old_radius);
    if (dlg.ShowModal() == wxID_OK) {
        int new_radius = dlg.GetRadius();
        template_canvas->SetPointRadius(new_radius);
        template_canvas->invalidateBms();
        template_canvas->Refresh();
    }
}

void TemplateLegend::OnCategoryColor(wxCommandEvent& event)
{
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	if (opt_menu_cat < 0 || opt_menu_cat >= num_cats) return;
	
	wxColour col = template_canvas->cat_data.GetCategoryColor(c_ts, opt_menu_cat);
	wxColourData data;
	data.SetColour(col);
	data.SetChooseFull(true);
	int ki;
	for (ki = 0; ki < 16; ki++) {
		wxColour colour(ki * 16, ki * 16, ki * 16);
		data.SetCustomColour(ki, colour);
	}
	
	wxColourDialog dialog(this, &data);
	dialog.SetTitle(_("Choose Cateogry Color"));
	if (dialog.ShowModal() == wxID_OK) {
		wxColourData retData = dialog.GetColourData();
		for (int ts=0; ts<template_canvas->cat_data.GetCanvasTmSteps(); ts++) {
			if (num_cats == template_canvas->cat_data.GetNumCategories(ts)) {
				template_canvas->cat_data.SetCategoryColor(ts, opt_menu_cat,
														   retData.GetColour());
			}
		}
		template_canvas->invalidateBms();
		template_canvas->Refresh();
		Refresh();
	}
}

void TemplateLegend::OnDraw(wxDC& dc)
{
	if (!template_canvas)
        return;
    
    dc.SetFont(*GdaConst::small_font);
    wxString title = template_canvas->GetCategoriesTitle();
    dc.DrawText(title, px, 13);
    
    wxString save_png_ttl = template_canvas->GetVariableNames();
    wxSize title_sz = dc.GetTextExtent(save_png_ttl);

    title_width = title_sz.GetWidth();
    title_height = title_sz.GetHeight();
	
	int time = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int cur_y = py;
	int numRect = template_canvas->cat_data.GetNumCategories(time);
	
    dc.SetPen(*wxBLACK_PEN);
    
    // init labels
    if (recreate_labels || labels.size() != numRect) {
        for (int i=0; i<labels.size(); i++){
            delete labels[i];
        }
        labels.clear();
        
        new_order.clear();
        
        int init_y = py;
        for (int i=0; i<numRect; i++) {
            wxString lbl = template_canvas->cat_data.GetCatLblWithCnt(time, i);
            wxPoint pt( px + m_l + 10, init_y - (m_w / 2));
            wxSize sz = dc.GetTextExtent(lbl);
            labels.push_back(new GdaLegendLabel(i, lbl, pt, sz));
            //init_y += d_rect;
            
            new_order.push_back(i);
        }
        
        recreate_labels = false;
    }
    
	for (int i=0; i<numRect; i++) {
        wxColour clr = template_canvas->cat_data.GetCategoryColor(time, i);
        if (clr.IsOk())
            dc.SetBrush(clr);
        else
            dc.SetBrush(*wxBLACK_BRUSH);
		dc.DrawRectangle(px, cur_y - 8, m_l, m_w);
		cur_y += d_rect;
	}
    
    for (int i=0; i<new_order.size(); i++) {
        GdaLegendLabel* lbl = labels[new_order[i]];
        if (lbl != select_label) {
            lbl->draw(dc, i);
        }
    }
    
    if ( select_label ) {
        select_label->drawMove(dc);
    }
}

void TemplateLegend::Recreate()
{
    recreate_labels = true;
    Refresh();
}

int TemplateLegend::GetDrawingWidth()
{
    int max_width = title_width;
    for (int i=0; i<new_order.size(); i++) {
        GdaLegendLabel* lbl = labels[new_order[i]];
        if (lbl->getWidth() > max_width) {
            max_width = lbl->getWidth();
        }
    }
    return max_width;
}

int TemplateLegend::GetDrawingHeight()
{
    int max_height = title_height;
    if (title_height>0) max_height += 13;
    
    for (int i=0; i<new_order.size(); i++) {
        max_height += d_rect;
    }
    return max_height + 2;
}

void TemplateLegend::RenderToDC(wxDC& dc, double scale)
{
	if (template_canvas == NULL)
        return;

	 dc.SetPen(*wxBLACK_PEN);

    int font_size = 12 * scale;
    wxFont* fnt = wxFont::New(font_size, wxFONTFAMILY_SWISS,
                              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                              wxEmptyString, wxFONTENCODING_DEFAULT);
    dc.SetFont(*fnt);
    
	int cur_y = py;
    int time = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int numRect = template_canvas->cat_data.GetNumCategories(time);
    wxString ttl = template_canvas->GetVariableNames();
    
    int gap = 10;
    if (scale < 1) {
        px = px * scale;
        gap = 10 * scale;
    }
    
    // draw legend title
    if (ttl.IsEmpty()) {
        cur_y = 2 * scale;
    } else {
        dc.DrawText(ttl, px, 2 * scale);
        wxSize title_sz = dc.GetTextExtent(ttl);
        cur_y = 2* scale + title_sz.GetHeight() + gap;
    }
    
    dc.SetPen(*wxBLACK_PEN);
    for (int i=0; i<numRect; i++) {
        wxColour clr = template_canvas->cat_data.GetCategoryColor(time, i);
        if (clr.IsOk()) {
            dc.SetBrush(clr);
        } else {
            dc.SetBrush(*wxBLACK_BRUSH);
        }
        
        int rect_x = px;
        int rect_y = cur_y;
        int rect_w = m_l * scale;
        int rect_h = m_w * scale;
        dc.DrawRectangle(rect_x, rect_y, rect_w, rect_h);
        
		wxString lbl = template_canvas->cat_data.GetCatLblWithCnt(time, i);
		double lbl_x = px + rect_w + gap;
        double lbl_y = cur_y + 2;
        dc.DrawText(lbl, lbl_x, lbl_y);
        

        cur_y += d_rect * scale;
    }
}


void TemplateLegend::SelectAllInCategory(int category,
										 bool add_to_selection)
{
	template_canvas->SelectAllInCategory(category, add_to_selection);
}

