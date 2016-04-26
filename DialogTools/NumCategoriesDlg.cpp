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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>
#include "../GenUtils.h"
#include "NumCategoriesDlg.h"

BEGIN_EVENT_TABLE( NumCategoriesDlg, wxDialog )
	EVT_SPINCTRL( XRCID("ID_NUM_CATEGORIES_SPIN"),
				 NumCategoriesDlg::OnSpinCtrl )
    EVT_BUTTON( wxID_OK, NumCategoriesDlg::OnOkClick )
END_EVENT_TABLE()

NumCategoriesDlg::NumCategoriesDlg(wxWindow* parent,
							   int min_categories_s,
							   int max_categories_s,
							   int default_categories_s,
							   const wxString& title,
							   const wxString& text)
: min_categories(min_categories_s), max_categories(max_categories_s),
default_categories(default_categories_s)
{
	categories = GenUtils::min<int>(default_categories, max_categories);
	
	wxXmlResource::Get()->LoadDialog(this, GetParent(),"ID_NUM_CATEGORIES_DLG");
	m_categories = wxDynamicCast(FindWindow(XRCID("ID_NUM_CATEGORIES_SPIN")), wxSpinCtrl);
	m_categories->SetRange(min_categories, max_categories);

	stat_text = wxDynamicCast(FindWindow(XRCID("IDC_STATIC")), wxStaticText);
	stat_text->SetLabelText(text);
	m_categories->SetValue(categories);

	SetParent(parent);
	SetTitle(title);
    Centre();
}

void NumCategoriesDlg::OnSpinCtrl( wxSpinEvent& event )
{
	categories = m_categories->GetValue();
	if (categories < 1) categories = 1;
	if (categories > max_categories) categories = max_categories;
}

void NumCategoriesDlg::OnOkClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_OK);
}

int NumCategoriesDlg::GetNumCategories()
{
	return categories;
}
