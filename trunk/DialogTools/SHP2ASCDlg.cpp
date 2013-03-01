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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/valtext.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces

#include "../ShapeOperations/shp.h"
#include "../ShapeOperations/ShapeFileTypes.h"
#include "../ShapeOperations/ShapeFileHdr.h"
#include "SHP2ASCDlg.h"

using namespace ShapeFileTypes;

BEGIN_EVENT_TABLE( SHP2ASCDlg, wxDialog )
    EVT_BUTTON( XRCID("IDOK_ADD"), SHP2ASCDlg::OnOkAddClick )
    EVT_BUTTON( XRCID("IDC_OPEN_OASC"), SHP2ASCDlg::OnCOpenOascClick )
    EVT_BUTTON( XRCID("IDOKDONE"), SHP2ASCDlg::OnOkdoneClick )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO1"), SHP2ASCDlg::OnCRadio1Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO2"), SHP2ASCDlg::OnCRadio2Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO3"), SHP2ASCDlg::OnCRadio3Selected )
    EVT_RADIOBUTTON( XRCID("IDC_RADIO4"), SHP2ASCDlg::OnCRadio4Selected )
    EVT_BUTTON( XRCID("IDC_OPEN_ISHP"), SHP2ASCDlg::OnCOpenIshpClick )
END_EVENT_TABLE()

bool CreateASCBoundary(char* ishp, char* oasc, char* orasc, int field,
					   int type, bool isR)
{
	if(isR && !orasc)
		return false;

	ofstream ascr;
	if (isR) {
		ascr.open(orasc); // produce bounding box file.
	}

	if(isR) {
		if(!ascr.is_open())
			return false;
	}

	ofstream asc(oasc);

	if(!asc.is_open())
		return false;

	iShapeFile    shx(wxString(ishp), "shx");
	char          hsx[ 2*GeoDaConst::ShpHeaderSize ];
	shx.read((char *) &hsx[0], 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hdx(hsx);
	long          offset, contents, *OffsetIx;
	long n = (hdx.Length() - GeoDaConst::ShpHeaderSize) / 4;
	OffsetIx= new long [ n ];

	if (n < 1 || OffsetIx == NULL)
		return false;

	for (long rec= 0; rec < n; ++rec) {
		offset= ReadBig(shx);
		contents= ReadBig(shx);
		offset *= 2;
		OffsetIx[rec]= offset;
	}
	shx.close();

	wxString name = wxString::Format("%s",ishp);
	name = name+ ".dbf";
	iDBF tb(name);

	if(!tb.IsConnectedToFile()) {
		wxMessageBox("Can't find a DBF file.");
		return false;
	}

	switch(type) {
		case 1:
		case 3:
			break;
		case 2:
		case 4:
			asc << n << "," << tb.GetFieldName(field) << endl;
			if(isR) ascr << n << "," << tb.GetFieldName(field) << endl;
			break;
		default:
			asc.close();
		if(isR)
			ascr.close();
		return false;
	}

	double* polyid = NULL;
	charPtr* temp_y = NULL;
	int col_type;

	if (tb.GetFieldType(field) == 'N' || tb.GetFieldType(field) == 'D') {
		polyid = new double[n];
		col_type = 1;
		tb.GetDblDataArray(wxString(tb.GetFieldName(field)),polyid);
	} else if (tb.GetFieldType(field) == 'C') {
		temp_y = new charPtr [n + 1];
		col_type = 0;
		tb.GetStrDataArray(wxString(tb.GetFieldName(field)),temp_y);
	} else {
		wxMessageBox("the file is Unsupported type!");
		tb.close();
		return false;
	}
	tb.close();

	iShapeFile    shp(wxString(ishp), "shp");
	char          hs[ 2*GeoDaConst::ShpHeaderSize ];
	shp.read(hs, 2*GeoDaConst::ShpHeaderSize);
	ShapeFileHdr        hd(hs);

	if (ShapeFileTypes::ShapeType(hd.FileShape()) == ShapeFileTypes::POLYGON)  
	{
		Box pBox;
		for (long rec= 0; rec < n; ++rec)  
		{
			if(col_type == 0)
				asc << temp_y[rec]; 
			else
				asc << polyid[rec];

			shp.seekg(OffsetIx[rec]+12, ios::beg);
#ifdef WORDS_BIGENDIAN
			char r[32], p;
			double m1, m2, n1, n2;
			shp.read((char *)r, sizeof(double) * 4);
			SWAP(r[0], r[7], p);
			SWAP(r[1], r[6], p);
			SWAP(r[2], r[5], p);
			SWAP(r[3], r[4], p);
			memcpy(&m1, &r[0], sizeof(double));
			SWAP(r[8], r[15], p);
			SWAP(r[9], r[14], p);
			SWAP(r[10], r[13], p);
			SWAP(r[11], r[12], p);
			memcpy(&m2, &r[8], sizeof(double));
			SWAP(r[16], r[23], p);
			SWAP(r[17], r[22], p);
			SWAP(r[18], r[21], p);
			SWAP(r[19], r[20], p);
			memcpy(&n1, &r[16], sizeof(double));
			SWAP(r[24], r[31], p);
			SWAP(r[25], r[30], p);
			SWAP(r[26], r[29], p);
			SWAP(r[27], r[28], p);
			memcpy(&n2, &r[24], sizeof(double));
			BasePoint p1 = BasePoint(m1, m2);
			BasePoint p2 = BasePoint(n1, n2);
			pBox = Box(p1, p2);
#else
			shp >> pBox;
#endif
			shp.seekg(OffsetIx[rec]+12, ios::beg);
			BoundaryShape t;
			t.ReadShape(shp);    

			int n_po  = t.GetNumPoints()-1;

			switch(type)
			{
			case 1:
			case 2:
				asc << "," << n_po << endl;
				break;
			case 3:
			case 4:
				asc << endl;
				break;
			default:

				return false;
			}

			if(isR) {
				if(col_type == 0)
					ascr << temp_y[rec]; 
				else
					ascr << polyid[rec];

				ascr << "," << wxString::Format("%.10f", pBox._min().x);
				ascr << "," << wxString::Format("%.10f", pBox._min().y);
				ascr << "," << wxString::Format("%.10f", pBox._max().x);
				ascr << "," << wxString::Format("%.10f", pBox._max().y);
				ascr << endl;		
			}

			BasePoint *Points;
			Points = t.GetPoints();

			for(int p=0; p<n_po; p++)
			{
				asc << wxString::Format("%.10f", Points[p].x);
				asc << ",";
				asc << wxString::Format("%.10f", Points[p].y);
				asc << endl;
			}
			switch(type)
			{
			case 3:
			case 4:
					asc << wxString::Format("%.10f", Points[0].x);
					asc << ",";
					asc << wxString::Format("%.10f", Points[0].y);
					asc << endl;
				break;
			case 1:
			case 2:
				break;
			default:
				return false;
			}
		}
	}

	if(polyid)
		delete [] polyid;
	polyid = NULL;
	if(temp_y)
		delete [] temp_y;
	temp_y = NULL;

	asc.close();
	if(isR)
		ascr.close();
	shp.close();    

	return true;	
}

SHP2ASCDlg::SHP2ASCDlg( )
{
}

SHP2ASCDlg::SHP2ASCDlg( wxWindow* parent, wxWindowID id,
						 const wxString& caption, const wxPoint& pos,
						 const wxSize& size, long style )
{
	type = -1;
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_OASC"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_ASC"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR"))->Enable(false);
	FindWindow(XRCID("IDOK_ADD"))->Enable(false);
}

bool SHP2ASCDlg::Create( wxWindow* parent, wxWindowID id,
						 const wxString& caption, const wxPoint& pos,
						 const wxSize& size, long style )
{
    SetParent(parent);
    CreateControls();
    Centre();

	m_ra1->SetValue(true);
	type = 1;

    return true;
}


void SHP2ASCDlg::CreateControls()
{
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_CONVERT_SHP2ASC");
    m_inputfile = XRCCTRL(*this, "IDC_FIELD_SHP", wxTextCtrl);
	m_inputfile->SetMaxLength(0);
    m_outputfile = XRCCTRL(*this, "IDC_FIELD_ASC", wxTextCtrl);
	m_outputfile->SetMaxLength(0);
    m_X = XRCCTRL(*this, "IDC_KEYVAR", wxChoice);
    m_check = XRCCTRL(*this, "IDC_CHECK1", wxCheckBox);
    m_ra1 = XRCCTRL(*this, "IDC_RADIO1", wxRadioButton);
    m_ra1a = XRCCTRL(*this, "IDC_RADIO2", wxRadioButton);
    m_ra2 = XRCCTRL(*this, "IDC_RADIO3", wxRadioButton);
    m_ra2a = XRCCTRL(*this, "IDC_RADIO4", wxRadioButton);

	if (m_ra1->GetValue()) type = 1;
	if (m_ra1a->GetValue()) type = 2;
	if (m_ra2->GetValue()) type = 3;
	if (m_ra2a->GetValue()) type = 4;
}

void SHP2ASCDlg::OnOkAddClick( wxCommandEvent& event )
{
	if(type == -1) {
		wxMessageBox("Select options!");
		return;
	}

	wxString m_ishp = m_inputfile->GetValue();
	wxString m_oasc = m_outputfile->GetValue();
	wxString DirName;
	wxString RName;
	wxString RNameExt;

	int pos = m_ishp.Find('.',true);
	if (pos > 0) 
		DirName = m_ishp.Left(pos);
	else 
		DirName = m_ishp;

	char buf_ifl[512];
	strcpy( buf_ifl, (const char*)DirName.mb_str(wxConvUTF8) );
	char* ifl = buf_ifl;

	char buf_ofl[512];
	strcpy( buf_ofl, (const char*)m_oasc.mb_str(wxConvUTF8) );
	char* ofl = buf_ofl;

	pos = m_oasc.Find('.',true);
	if (pos > 0) {
		RName = m_oasc.Left(pos);
		RNameExt = m_oasc.Right(m_oasc.Length()-pos);
	} else {
		RName = m_oasc;
		RNameExt = wxEmptyString;
	}

	bool m_isR = m_check->GetValue();

	if(m_isR) {
		wxString orf = RName+ "_r" +RNameExt;
		char buf_orfl[512];
		strcpy( buf_orfl, (const char*)orf.mb_str(wxConvUTF8) );
		char* orfl = buf_orfl;

		if(!CreateASCBoundary(ifl, ofl, orfl,
							  m_X->GetSelection(), type, m_isR)) {
			wxMessageBox("Can't write output file!");
			return;
		}
			
	} else {
		if(!CreateASCBoundary(ifl, ofl, NULL,
							  m_X->GetSelection(), type, m_isR)) {
			wxMessageBox("Can't write output file!");
			return;
		}
	}

	event.Skip();
}

void SHP2ASCDlg::OnCOpenOascClick( wxCommandEvent& event )
{
    wxFileDialog dlg
                 (
                    this,
                    "Output ASCII file",
                    wxEmptyString,
                    fn + ".txt",
                    "ASCII files (*.txt)|*.txt",
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT
                 );

	
	wxString	m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);
		FindWindow(XRCID("IDOK_ADD"))->Enable(true);
	}
}

void SHP2ASCDlg::OnOkdoneClick( wxCommandEvent& event )
{
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void SHP2ASCDlg::OnCRadio1Selected( wxCommandEvent& event )
{
    type = 1;
}

void SHP2ASCDlg::OnCRadio2Selected( wxCommandEvent& event )
{
    type = 2;
}

void SHP2ASCDlg::OnCRadio3Selected( wxCommandEvent& event )
{
    type = 3;
}

void SHP2ASCDlg::OnCRadio4Selected( wxCommandEvent& event )
{
    type = 4;
}


void SHP2ASCDlg::OnCOpenIshpClick( wxCommandEvent& event )
{
    wxFileDialog dlg( this, "Input Shp file", "", "",
					 "Shp files (*.shp)|*.shp" );

	
	wxString	m_path = wxEmptyString;
	bool m_polyid = false;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString onlyFileName = dlg.GetPath();
		wxString InFile = m_path;
		wxString m_ishp = m_path;
		m_inputfile->SetValue(InFile);
		fn = dlg.GetFilename();
		int j = fn.Find('.', true);
		if (j >= 0) fn = fn.Left(j);

		m_X->Clear();

		wxString DirName;
		int pos = m_ishp.Find('.',true);
		DirName = pos >= 0 ? m_ishp.Left(pos) : m_ishp;
		DirName = DirName + ".dbf";
		
		iDBF tb(DirName);
		
		if(!tb.IsConnectedToFile())
		{
			wxMessageBox("DBF file failed!");
			return;
		}
		int numfields = tb.GetNumOfField();

		if(numfields == 0)
		{
			wxMessageBox("No fields found!");
			tb.close();
			return;
		}

		for(int i=0; i<numfields; i++)
		{
		   
			m_X->Append(wxString::Format("%s",tb.GetFieldName(i)));
		}
		tb.close();

		m_X->SetSelection(0);

	
		FindWindow(XRCID("IDC_OPEN_OASC"))->Enable(true);
		FindWindow(XRCID("IDC_FIELD_ASC"))->Enable(true);
		FindWindow(XRCID("IDC_KEYVAR"))->Enable(true);	
	}
}
