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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/valtext.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/xrc/xmlres.h> // XRC XML resouces

#include "../ShapeOperations/ShapeFileTypes.h"
#include "../ShapeOperations/ShapeFileHdr.h"

#include "../ShapeOperations/OGRDatasourceProxy.h"
#include "../ShapeOperations/OGRLayerProxy.h"
#include "SHP2ASCDlg.h"
#include "ConnectDatasourceDlg.h"

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

bool SHP2ASCDlg::CreateASCBoundary(wxString oasc, wxString orasc, int field,
                                   int type, bool isR)
{
	if(isR && !orasc)
		return false;

	ofstream ascr;
	if (isR) {
		//ascr.open(orasc); // produce bounding box file.
		ascr.open(GET_ENCODED_FILENAME(orasc));
	}

	if(isR) {
		if(!ascr.is_open())
			return false;
	}

	ofstream asc;
	asc.open(GET_ENCODED_FILENAME(oasc));

	if(!asc.is_open())
		return false;

    int n = ogr_layer->GetNumRecords();
    string field_name = ogr_layer->GetFieldName(field).ToStdString();
    
	switch(type) {
		case 1:
		case 3:
			break;
		case 2:
		case 4:
			asc << n << "," << field_name << endl;
			if(isR) ascr << n << "," << field_name << endl;
			break;
		default:
			asc.close();
		if(isR)
			ascr.close();
		return false;
	}

	double* polyid = NULL;
	string* temp_y = NULL;
	int col_type;

    if (ogr_layer->GetFieldType(field) == GdaConst::long64_type ||
        ogr_layer->GetFieldType(field) == GdaConst::double_type ) {
		polyid = new double[n];
		col_type = 1;
        for (int i=0; i<n; i++) {
            polyid[i] = ogr_layer->data[i]->GetFieldAsInteger64(field);
        }
        
	} else if (ogr_layer->GetFieldType(field) == GdaConst::string_type) {
		temp_y = new string[n + 1];
		col_type = 0;
        for (int i=0; i<n; i++) {
            temp_y[i] = ogr_layer->data[i]->GetFieldAsString(field);
        }
        
	} else {
		wxMessageBox(_("This file type is not supported."));
		return false;
	}

	if (ogr_layer->GetShapeType() == wkbPolygon ||
		ogr_layer->GetShapeType() == wkbMultiPolygon ) {
        
		for (long rec= 0; rec < n; ++rec)
		{
			if(col_type == 0)
				asc << temp_y[rec]; 
			else
				asc << polyid[rec]; 

			int n_po  = 0;
            OGRGeometry* geom = ogr_layer->data[rec]->GetGeometryRef();
            OGRPolygon* poly = dynamic_cast<OGRPolygon*>(geom);
            OGRMultiPolygon* multi_poly = NULL;
           
            // if a OGRPolygon, build a OGRMultiPolygon
            if ( poly != NULL ) {
                multi_poly = new OGRMultiPolygon();
                multi_poly->addGeometry(poly);
            } else {
                multi_poly = dynamic_cast<OGRMultiPolygon*>(geom);
            }
           
            int num_sub_polys = multi_poly->getNumGeometries();
            
            for ( int i=0; i < num_sub_polys; i++) {
                OGRPolygon* sub_poly =
                    (OGRPolygon*)multi_poly->getGeometryRef(i);
                OGRLinearRing* ext_ring = sub_poly->getExteriorRing();
                n_po += ext_ring->getNumPoints();
                int num_inner_rings = sub_poly->getNumInteriorRings();
                for (int j=0; j < num_inner_rings; j++) {
                    OGRLinearRing* inn_ring = sub_poly->getInteriorRing(j);
                    n_po += inn_ring->getNumPoints();
                }
            }
            
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
                
                OGREnvelope pEnvelope;
                multi_poly->getEnvelope(&pEnvelope);

				ascr << "," << wxString::Format("%.10f", pEnvelope.MinX);
				ascr << "," << wxString::Format("%.10f", pEnvelope.MinY);
				ascr << "," << wxString::Format("%.10f", pEnvelope.MaxX);
				ascr << "," << wxString::Format("%.10f", pEnvelope.MaxY);
				ascr << endl;		
			}
            
            OGRLinearRing* ext_ring = NULL;
            
            for ( int i=0; i < num_sub_polys; i++) {
                OGRPolygon* sub_poly =
                    (OGRPolygon*)multi_poly->getGeometryRef(i);
                ext_ring = sub_poly->getExteriorRing();
                for (int i=0; i < ext_ring->getNumPoints(); i++) {
                    asc << wxString::Format("%.10f", ext_ring->getX(i));
                    asc << ",";
                    asc << wxString::Format("%.10f", ext_ring->getY(i));
                    asc << endl;
                }
                int num_inner_rings = sub_poly->getNumInteriorRings();
                for (int i=0; i < num_inner_rings; i++) {
                    OGRLinearRing* inn_ring = sub_poly->getInteriorRing(i);
                    for (int j=0; j < inn_ring->getNumPoints(); j++) {
                        asc << wxString::Format("%.10f", inn_ring->getX(j));
                        asc << ",";
                        asc << wxString::Format("%.10f", inn_ring->getY(j));
                        asc << endl;
                    }
                }
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

	return true;	
}

SHP2ASCDlg::SHP2ASCDlg( )
: ogr_ds(NULL), ogr_layer(NULL)
{
}

SHP2ASCDlg::SHP2ASCDlg( wxWindow* parent, wxWindowID id,
						 const wxString& caption, const wxPoint& pos,
						 const wxSize& size, long style )
: ogr_ds(NULL), ogr_layer(NULL)
{
	type = -1;
    Create(parent, id, caption, pos, size, style);

	FindWindow(XRCID("IDC_OPEN_OASC"))->Enable(false);
	FindWindow(XRCID("IDC_FIELD_ASC"))->Enable(false);
	FindWindow(XRCID("IDC_KEYVAR"))->Enable(false);
	FindWindow(XRCID("IDOK_ADD"))->Enable(false);
}

SHP2ASCDlg::~SHP2ASCDlg()
{
    if ( ogr_ds ) {
        delete ogr_ds;
        ogr_ds = NULL;
    }
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
    wxLogMessage("In SHP2ASCDlg::OnOkAddClick()");
	if(type == -1) {
		wxMessageBox(_("Please select an option."));
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

	wxString ifl = DirName;
	wxString ofl = m_oasc;

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

		if(!CreateASCBoundary(ofl, orf, m_X->GetSelection(), type, m_isR)) {
			wxMessageBox(_("Can't write output file!"));
			return;
		}
			
	} else {
		if(!CreateASCBoundary(ofl, wxEmptyString, m_X->GetSelection(),
                              type, m_isR)) {
			wxMessageBox(_("Can't write output file!"));
			return;
		}
	}

    wxMessageDialog dlg (this, _("Export shape to boundary successfully."),
                         _("Info"), wxOK | wxICON_INFORMATION);
    dlg.ShowModal();
    
	event.Skip();
}

void SHP2ASCDlg::OnCOpenOascClick( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCOpenOascClick()");
    
    wxFileDialog dlg(this,
                     _("Output ASCII file"),
                     wxEmptyString,
                     fn + ".txt",
                     "ASCII files (*.txt)|*.txt",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	
	wxString m_path = wxEmptyString;

    if (dlg.ShowModal() == wxID_OK) {
		m_path  = dlg.GetPath();
		wxString OutFile = m_path;
		m_outputfile->SetValue(OutFile);
		FindWindow(XRCID("IDOK_ADD"))->Enable(true);
	}
}

void SHP2ASCDlg::OnOkdoneClick( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnOkdoneClick()");
    
	event.Skip();
	EndDialog(wxID_CANCEL);
}

void SHP2ASCDlg::OnCRadio1Selected( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCRadio1Selected()");
    
    type = 1;
}

void SHP2ASCDlg::OnCRadio2Selected( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCRadio2Selected()");
    type = 2;
}

void SHP2ASCDlg::OnCRadio3Selected( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCRadio3Selected()");
    type = 3;
}

void SHP2ASCDlg::OnCRadio4Selected( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCRadio4Selected()");
    type = 4;
}


void SHP2ASCDlg::OnCOpenIshpClick( wxCommandEvent& event )
{
    wxLogMessage("In SHP2ASCDlg::OnCOpenIshpClick()");
    try{
        ConnectDatasourceDlg dlg(this);
        if (dlg.ShowModal() != wxID_OK) return;
        
        wxString proj_title = dlg.GetProjectTitle();
        wxString layer_name = dlg.GetLayerName();
        IDataSource* datasource = dlg.GetDataSource();
        wxString ds_name = datasource->GetOGRConnectStr();
        GdaConst::DataSourceType ds_type = datasource->GetType();
        
        ogr_ds = new OGRDatasourceProxy(ds_name, ds_type, true);
        ogr_layer = ogr_ds->GetLayerProxy(layer_name.ToStdString());
        ogr_layer->ReadData();

        bool is_valid_layer = true;
        
        if (ogr_layer->IsTableOnly()) {
            wxMessageBox(_("This is not a shape datasource. Please open a valid shape datasource, e.g. ESRI Shapefile, PostGIS layer..."));
            is_valid_layer = false;
        }
        if (ogr_layer->GetNumFields() == 0){
            wxMessageBox(_("No fields found!"));
            is_valid_layer = false;
        }
        if ( !is_valid_layer) {
            delete ogr_ds;
            ogr_ds = NULL;
            return;
        }
        
        m_X->Clear();
        for (int i=0; i<ogr_layer->GetNumFields(); i++){
            m_X->Append(wxString::Format("%s",ogr_layer->GetFieldName(i)));
        }
        m_X->SetSelection(0);
        m_inputfile->SetValue(layer_name);
        
        FindWindow(XRCID("IDC_OPEN_OASC"))->Enable(true);
        FindWindow(XRCID("IDC_FIELD_ASC"))->Enable(true);
        FindWindow(XRCID("IDC_KEYVAR"))->Enable(true);
    } catch (GdaException& e) {
        wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
        return;
    }
}
