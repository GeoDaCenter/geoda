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

#include <vector>
#include <wx/wx.h>
#include "../DataViewer/DataSource.h"
#include "../DataViewer/TableInterface.h"
#include "../Project.h"
#include "ProjectInfoDlg.h"

ProjectInfoDlg::ProjectInfoDlg(Project* project)
: wxDialog(NULL, wxID_ANY, _("Project Information"), wxDefaultPosition,
           wxSize(250, 150), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxLogMessage("Open ProjectInfoDlg.");
    std::vector<wxString> key;
    std::vector<wxString> val;
	key.push_back("Project Title");
	val.push_back(project->GetProjectTitle());
	
	key.push_back("Project File");
	wxString pfp = project->GetProjectFullPath();
	if (pfp.IsEmpty()) {
		val.push_back("N/A");
	} else {
		val.push_back(pfp);
	}
		
	IDataSource* ds = project->GetDataSource();
	
	key.push_back("Data Source Type");
	val.push_back(GdaConst::datasrc_type_to_str[ds->GetType()]);
	
	if (FileDataSource* d = dynamic_cast<FileDataSource*>(ds)) {
		key.push_back("Data Source File");
		val.push_back(d->GetFilePath());
	} else if (DBDataSource* d = dynamic_cast<DBDataSource*>(ds)) {
		key.push_back("Database Name");
		val.push_back(d->GetDBName());
		key.push_back("Database Host");
		val.push_back(d->GetDBHost());
		key.push_back("Database Port");
		val.push_back(d->GetDBPort());
		key.push_back("Database User");
		val.push_back(d->GetDBUser());
	} else if (WebServiceDataSource* d =
			   dynamic_cast<WebServiceDataSource*>(ds)) {
		key.push_back("Web File Service URL");
		val.push_back(d->GetURL());
	}
	
	key.push_back("Layer Name");
	wxString layername;
	if (ProjectConfiguration* pc = project->GetProjectConf()) {
		if (LayerConfiguration* lc = pc->GetLayerConfiguration()) {
			layername = lc->GetName();
		}
	}
	val.push_back(layername);
	
	TableInterface* table_int = project->GetTableInt();
	key.push_back("Number Records/Observations");
	val.push_back(wxString::Format("%d", table_int->GetNumberRows()));
	
	int table_cols = table_int->GetNumberCols();
	int grp_cnt = 0;
	int fld_cnt = 0;
	int plhdr_cnt = 0;
	for (int c=0; c<table_cols; ++c) {
		if (table_int->IsColTimeVariant(c)) {
			++grp_cnt;
			for (int t=0, ts=table_int->GetColTimeSteps(c); t<ts; ++t) {
				if (table_int->GetColType(c,t) != GdaConst::placeholder_type) {
					++fld_cnt;
				} else {
					++plhdr_cnt;
				}
			}
		} else {
			++fld_cnt;
		}
	}
	
	key.push_back("Number Data Source Fields");
	val.push_back(wxString::Format("%d", fld_cnt));
	
	key.push_back("Number Table Columns");
	val.push_back(wxString::Format("%d", table_cols));
	
	key.push_back("Number Table Groups");
	val.push_back(wxString::Format("%d", grp_cnt));
    
    if (project->IsTableOnlyProject() == false) {
        key.push_back("Map boundary");
        double minx = 0, miny = 0,  maxx = 0,  maxy = 0;
        project->GetMapExtent(minx, miny, maxx, maxy);
        val.push_back(wxString::Format("Lower left: %f, %f Upper right: %f, %f", minx, miny, maxx, maxy));

        key.push_back("CRS (proj4 format)");
        OGRSpatialReference* sr = project->GetSpatialReference();
        wxString str_crs = "Unknown";
        if (sr) {
            char* tmp = new char[1024];
            if (sr->exportToProj4(&tmp) == OGRERR_NONE) {
                str_crs = tmp;
            }
            delete[] tmp;
        }
        val.push_back(str_crs);

        key.push_back("CRS (EPSG code)");
        wxString str_EPSG = "Unknown";
        if (sr) {
            int epsg_code = sr->GetEPSGGeogCS();
            if (epsg_code > -1) {
                str_EPSG = wxString::Format("%d", epsg_code);
            }
        }
        val.push_back(str_EPSG);
    }

    wxPanel* panel = new wxPanel(this, -1);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    wxFlexGridSizer* gbox = new wxFlexGridSizer(12, 2, 5, 5);
    gbox->SetFlexibleDirection(wxBOTH);
    gbox->AddGrowableCol(1, 1);
	for (int i=0, sz=key.size(); i<sz; ++i) {
        wxString s = key[i] + ": ";
		wxStaticText* st = new wxStaticText(panel, wxID_ANY, s);
        wxTextCtrl* txt = new wxTextCtrl(panel, wxID_ANY, val[i],
                                         wxDefaultPosition, wxSize(400, -1),
                                         wxTE_READONLY);
        wxBoxSizer* txtbox = new wxBoxSizer(wxHORIZONTAL);
        txtbox->Add(txt, 1, wxEXPAND);
        gbox->Add(st);
        gbox->Add(txtbox, 1, wxEXPAND | wxALL);
	}
	
	panel->SetSizerAndFit(gbox);

	wxButton* ok_btn = new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition,
									wxDefaultSize, wxBU_EXACTFIT);
	
	hbox->Add(ok_btn, 1, wxLEFT, 15);
	vbox->Add(panel, 1, wxALL | wxEXPAND, 15);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
	
	SetSizer(vbox);
	vbox->Fit(this);
	
	Center();
}
