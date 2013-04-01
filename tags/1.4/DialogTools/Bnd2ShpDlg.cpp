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

#include <fstream>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/xrc/xmlres.h>

#include "Bnd2ShpDlg.h"
#include "../ShapeOperations/shp2gwt.h"

IMPLEMENT_CLASS( Bnd2ShpDlg, wxDialog )

BEGIN_EVENT_TABLE( Bnd2ShpDlg, wxDialog )
    EVT_BUTTON( XRCID("ID_CREATE"), Bnd2ShpDlg::OnCreateClick )
    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), Bnd2ShpDlg::OnCOpenIascClick )
    EVT_BUTTON( XRCID("IDC_OPEN_OSHP"), Bnd2ShpDlg::OnCOpenOshpClick )
    EVT_BUTTON( XRCID("IDCANCEL"), Bnd2ShpDlg::OnCancelClick )
END_EVENT_TABLE()

Bnd2ShpDlg::Bnd2ShpDlg( )
{
}

Bnd2ShpDlg::Bnd2ShpDlg( wxWindow* parent, wxWindowID id,
					   const wxString& caption, const wxPoint& pos,
					   const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(false);
	FindWindow(XRCID("ID_CREATE"))->Enable(false);
}

bool Bnd2ShpDlg::Create( wxWindow* parent, wxWindowID id,
						const wxString& caption, const wxPoint& pos,
						const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();
    return true;
}

void Bnd2ShpDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_CONVERT_BOUNDARY_TO_SHP");
    m_inputfile = XRCCTRL(*this, "IDC_FIELD_ASC", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_FIELD_SHP", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
}

void Bnd2ShpDlg::OnCreateClick( wxCommandEvent& event )
{
	wxString m_iASC = m_inputfile->GetValue();
	wxString m_oSHP = m_outputfile->GetValue();

	if (!CreateSHPfromBoundary(m_iASC, m_oSHP))
	{
		wxMessageBox("Fail in reading the input file!");
			return;
	}

	event.Skip(); // wxDialog::OnOK(event);
}

void Bnd2ShpDlg::OnCOpenIascClick( wxCommandEvent& event )
{
    wxFileDialog dlg ( this, "Input ASCII file", "", "",
					  "ASCII files (*.*)|*.*" );

	wxString	m_path = wxEmptyString;
	bool m_polyid = false;

    if (dlg.ShowModal() == wxID_OK)
    {
		m_path  = dlg.GetPath();
		wxString onlyFileName = dlg.GetPath();
		wxString InFile = m_path;
		wxString m_iASC = m_path;
		m_inputfile->SetValue(InFile);
		
		fn = dlg.GetFilename();
		int j = fn.Find('.', true);
		if (j >= 0) fn = fn.Left(j);
        
		std::ifstream ias;
		ias.open(m_iASC.mb_str());
		char name[1000];

		ias.getline(name,100);
		//wxString tok = wxString::Format("%100s", name);
		wxString tok = wxString(name, wxConvUTF8);
		wxString ID_name= wxEmptyString; 
		long nRows;

		int pos = tok.Find(',');
		if( pos >= 0)
		{
			wxString tl = tok.Left(pos);
			//nRows = (double) atof(tl.ToAscii());
			tl.ToCLong(&nRows);
			ID_name = tok.Right(tok.Length()-pos-1);
		}
		else
		{
			wxMessageBox("The first line should have comma separated "
						 "number of rows and ID name!");
			return;
		}

		ID_name.Trim(true);
		ID_name.Trim(false);

		if (nRows < 1)
		{
			wxMessageBox("Wrong number of rows!");
	 			return;
		}
		else if(ID_name == wxEmptyString)
		{
			wxMessageBox("ID is not specified!");
			return;
		}

		FindWindow(XRCID("IDC_OPEN_OSHP"))->Enable(true);
		FindWindow(XRCID("IDC_FIELD_SHP"))->Enable(true);

    }

}

void Bnd2ShpDlg::OnCOpenOshpClick( wxCommandEvent& event )
{
	wxFileDialog dlg
                 (
                    this,
                    "Output Shp file",
                    wxEmptyString,
                    fn + ".shp",
                    "Shp files (*.shp)|*.shp",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

	wxString	m_path = wxEmptyString;


    if (dlg.ShowModal() == wxID_OK)
    {

		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);

		FindWindow(XRCID("ID_CREATE"))->Enable(true);
	}
}

void Bnd2ShpDlg::OnCancelClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);	
}
