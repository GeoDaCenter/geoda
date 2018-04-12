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

#ifndef __GEODA_CENTER_MAP_QUANTILE_DLG_H__
#define __GEODA_CENTER_MAP_QUANTILE_DLG_H__

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

class NumCategoriesDlg: public wxDialog
{    
    DECLARE_EVENT_TABLE()

public:
    NumCategoriesDlg( wxWindow* parent,
				   int min_categories_s,
				   int max_categories_s,
				   int default_categories_s,
				   const wxString& title = _("Number of Categories"),
				   const wxString& text = _("Categories"));
	
	void OnSpinCtrl( wxSpinEvent& event );
    void OnOkClick( wxCommandEvent& event );
	int GetNumCategories();
	
private:
	int categories;
	int min_categories;
	int max_categories;
	int default_categories;
	wxStaticText* stat_text;
    wxSpinCtrl* m_categories;
};

#endif

