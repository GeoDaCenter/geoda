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

#include <wx/colordlg.h>
#include <wx/colourdata.h>
#include <wx/xrc/xmlres.h>
#include "logger.h"
#include "GeoDaConst.h"
#include "TemplateCanvas.h"
#include "TemplateFrame.h"
#include "TemplateLegend.h"

const int TemplateLegend::ID_CATEGORY_COLOR = wxID_HIGHEST + 1;

IMPLEMENT_ABSTRACT_CLASS(TemplateLegend, wxScrolledWindow)

BEGIN_EVENT_TABLE(TemplateLegend, wxScrolledWindow)
	EVT_MENU(TemplateLegend::ID_CATEGORY_COLOR, TemplateLegend::OnCategoryColor)
	EVT_MOUSE_EVENTS(TemplateLegend::OnEvent)
END_EVENT_TABLE()

TemplateLegend::TemplateLegend(wxWindow *parent,
							   TemplateCanvas* template_canvas_s,
							   const wxPoint& pos, const wxSize& size)
: wxScrolledWindow(parent, wxID_ANY, pos, size,
				   wxBORDER_SUNKEN | wxVSCROLL | wxHSCROLL),
legend_background_color(GeoDaConst::legend_background_color),
template_canvas(template_canvas_s)
{
	SetBackgroundColour(GeoDaConst::legend_background_color);
    d_rect = 20;
    px = 10;
    py = 40;
    m_w = 15;
    m_l = 20;
}

TemplateLegend::~TemplateLegend()
{
}

void TemplateLegend::OnEvent(wxMouseEvent& event)
{
	if (!template_canvas) return;
	
	int cat_clicked = GetCategoryClick(event);
	
    if (event.RightUp()) {
        LOG_MSG("MapNewLegend::OnEvent, event.RightUp() == true");
        wxMenu* optMenu =
			wxXmlResource::Get()->LoadMenu("ID_MAP_VIEW_MENU_LEGEND");
		AddCategoryColorToMenu(optMenu, cat_clicked);
        PopupMenu(optMenu, event.GetPosition());
        return;
    }
	
    if (event.LeftDown()) {
        LOG_MSG("TemplateLegend::OnEvent, event.LeftDown() == true");
		if (cat_clicked != -1) {
			SelectAllInCategory(cat_clicked, event.ShiftDown());
		}
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
	s << "Color for Category";
	wxString cat_label = template_canvas->cat_data.GetCategoryLabel(c_ts,
																cat_clicked);
	if (!cat_label.IsEmpty()) s << ": " << cat_label;
	menu->Prepend(ID_CATEGORY_COLOR, s, s);
	opt_menu_cat = cat_clicked;
}

void TemplateLegend::OnCategoryColor(wxCommandEvent& event)
{
	int c_ts = template_canvas->cat_data.GetCurrentCanvasTmStep();
	int num_cats = template_canvas->cat_data.GetNumCategories(c_ts);
	if (opt_menu_cat < 0 || opt_menu_cat >= num_cats) return;
	
	wxColour col = template_canvas->cat_data.GetCategoryColor(c_ts,
															  opt_menu_cat);
	wxColourData data;
	data.SetColour(col);
	data.SetChooseFull(true);
	int ki;
	for (ki = 0; ki < 16; ki++) {
		wxColour colour(ki * 16, ki * 16, ki * 16);
		data.SetCustomColour(ki, colour);
	}
	
	wxColourDialog dialog(this, &data);
	dialog.SetTitle("Choose Cateogry Color");
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
	if (!template_canvas) return;
    dc.SetFont(*GeoDaConst::small_font);
    dc.DrawText(template_canvas->GetCategoriesTitle(), px, 13);
	
	int time = template_canvas->cat_data.GetCurrentCanvasTmStep();
    int cur_y = py;
	int numRect = template_canvas->cat_data.GetNumCategories(time);
	
    dc.SetPen(*wxBLACK_PEN);
	for (int i=0; i<numRect; i++) {
		dc.SetBrush(template_canvas->cat_data.GetCategoryColor(time, i));
		dc.DrawText(template_canvas->cat_data.GetCatLblWithCnt(time, i),
					(px + m_l + 10), cur_y - (m_w / 2));
		dc.DrawRectangle(px, cur_y - 8, m_l, m_w);
		cur_y += d_rect;
	}
}

void TemplateLegend::SelectAllInCategory(int category,
										 bool add_to_selection)
{
	template_canvas->SelectAllInCategory(category, add_to_selection);
}

