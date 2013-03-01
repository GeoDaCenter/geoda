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

#ifndef __GEODA_CENTER_TEMPLATE_LEGEND_H__
#define __GEODA_CENTER_TEMPLATE_LEGEND_H__

#include <wx/menu.h>
#include <wx/scrolwin.h>
#include <wx/dc.h>

class TemplateCanvas;
class TemplateFrame;

class TemplateLegend: public wxScrolledWindow
{
public:
	TemplateLegend(wxWindow *parent, TemplateCanvas* template_canvas,
				   const wxPoint& pos, const wxSize& size);
	virtual ~TemplateLegend();
	
	void OnCategoryColor(wxCommandEvent& event);
	void OnEvent(wxMouseEvent& event);
	virtual void OnDraw(wxDC& dc);
	
	wxColour legend_background_color;
	TemplateCanvas* template_canvas;
	
protected:
	void SelectAllInCategory(int category, bool add_to_selection = false);
	int GetCategoryClick(wxMouseEvent& event);
	void AddCategoryColorToMenu(wxMenu* menu, int cat_clicked);
	
	int px, py, m_w, m_l; 
	int d_rect; 
	bool all_init;
	int opt_menu_cat; // last category added to Legend menu
	
	static const int ID_CATEGORY_COLOR;
	
	DECLARE_ABSTRACT_CLASS(TemplateLegend)
	DECLARE_EVENT_TABLE()
};

#endif
