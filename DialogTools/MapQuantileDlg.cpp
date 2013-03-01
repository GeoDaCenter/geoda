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

// For compilers that support precompilation, includes <wx/wx.h>.
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/valtext.h>
#include "../GenUtils.h"
#include "MapQuantileDlg.h"

BEGIN_EVENT_TABLE( MapQuantileDlg, wxDialog )
    EVT_BUTTON( wxID_OK, MapQuantileDlg::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, MapQuantileDlg::OnCancelClick )
END_EVENT_TABLE()

MapQuantileDlg::MapQuantileDlg(wxWindow* parent,
							   int min_classes_s,
							   int max_classes_s,
							   int default_classes_s,
							   const wxString& title,
							   const wxString& text)
: min_classes(min_classes_s), max_classes(max_classes_s),
default_classes(default_classes_s)
{
	classes = GenUtils::min<int>(default_classes, max_classes);
	
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_DIALOG_QUANTILE");
	m_classes = wxDynamicCast(FindWindow(XRCID("IDC_EDIT_QUANTILE")),
							  wxSpinCtrl);
	m_classes->SetRange(min_classes, max_classes);

	stat_text = wxDynamicCast(FindWindow(XRCID("IDC_STATIC")), wxStaticText);
	stat_text->SetLabelText(text);
	wxString val;
	val << classes;
	m_classes->SetValue(val);

	SetParent(parent);
	SetTitle(title);
    Centre();
}

void MapQuantileDlg::OnOkClick( wxCommandEvent& event )
{
	classes = m_classes->GetValue();
	if (classes < 1) classes = 1;
	if (classes > max_classes) classes = max_classes;

	event.Skip();
	EndDialog(wxID_OK);
}

void MapQuantileDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}
