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



#include <fstream>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h> 
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/progdlg.h>
#include <cpl_error.h>

#include "../ShapeOperations/WeightsManager.h"
#include "../Project.h"
#include "../GenUtils.h"
#include "../logger.h"
#include "../GdaException.h"
#include "../GeneralWxUtils.h"
#include "../rc/GeoDaIcon-16x16.xpm"
#include "SaveAsDlg.h"



BEGIN_EVENT_TABLE( SaveAsDlg, wxDialog )
    EVT_BUTTON( XRCID("IDC_OPEN_IASC"), SaveAsDlg::OnBrowseProjectFileBtn )
    EVT_BUTTON( XRCID("IDC_OPEN_DS_PATH"), SaveAsDlg::OnBrowseDatasourceBtn )
    EVT_BUTTON( wxID_OK, SaveAsDlg::OnOkClick )
    EVT_CHECKBOX( XRCID("ID_SEL_PRJ_CHECKBOX"), SaveAsDlg::OnProjectCheck)
    EVT_CHECKBOX( XRCID("ID_SEL_DS_CHECKBOX"), SaveAsDlg::OnDatasourceCheck)
END_EVENT_TABLE()

SaveAsDlg::SaveAsDlg(wxWindow* parent,
                     Project* project,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos,
                     const wxSize& size )
: project_p(project)
{
	proj_file_path = wxFileName("");
    ds_file_path = wxFileName("");
   
    wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_SAVE_AS_DLG");
    FindWindow(XRCID("wxID_OK"))->Enable(false);
    
	m_chk_create_datasource = XRCCTRL(*this, "ID_SEL_DS_CHECKBOX",wxCheckBox);
	m_datasource_path_txt = XRCCTRL(*this, "IDC_FIELD_DS_PATH", wxTextCtrl);
    m_browse_datasource_btn = XRCCTRL(*this, "IDC_OPEN_DS_PATH",wxBitmapButton);
    m_chk_create_datasource->SetValue(false);
    m_datasource_path_txt->Disable();
    m_browse_datasource_btn->Disable();
    
	m_chk_create_project = XRCCTRL(*this, "ID_SEL_PRJ_CHECKBOX",wxCheckBox);
	m_project_path_txt = XRCCTRL(*this, "IDC_FIELD_ASC", wxTextCtrl);
    m_browse_project_btn = XRCCTRL(*this, "IDC_OPEN_IASC",wxBitmapButton);
    m_chk_create_project->SetValue(true);
    m_project_path_txt->Enable();
    m_browse_project_btn->Enable();

	SetIcon(wxIcon(GeoDaIcon_16x16_xpm));

    SetParent(parent);
	SetPosition(pos);
	Centre();
}

void SaveAsDlg::OnProjectCheck( wxCommandEvent& event )
{
    if ( !m_chk_create_project->IsChecked() &&
         !m_chk_create_datasource->IsChecked() ) {
        FindWindow(XRCID("wxID_OK"))->Enable(false);
    }
    bool flag = m_chk_create_project->IsChecked();
    m_project_path_txt->Enable(flag);
    m_browse_project_btn->Enable(flag);
}

void SaveAsDlg::OnDatasourceCheck( wxCommandEvent& event )
{
    if ( !m_chk_create_project->IsChecked() &&
         !m_chk_create_datasource->IsChecked() ) {
        FindWindow(XRCID("wxID_OK"))->Enable(false);
    }
    bool flag = m_chk_create_datasource->IsChecked();
    m_datasource_path_txt->Enable(flag);
    m_browse_datasource_btn->Enable(flag);
}

void SaveAsDlg::OnBrowseDatasourceBtn ( wxCommandEvent& event )
{
    wxString msg;
    IDataSource* datasource = project_p->GetDataSource();
    if ( datasource == NULL ) {
        msg = "Datasource in project is not valid.";
        wxMessageDialog dlg(this, msg , "Error", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    GdaConst::DataSourceType ds_type = datasource->GetType();
    wxString ds_format = IDataSource::GetDataTypeNameByGdaDSType(ds_type);
    
    if (ds_type != GdaConst::ds_dbf &&
        ds_type != GdaConst::ds_shapefile &&
        ds_type != GdaConst::ds_gml &&
        ds_type != GdaConst::ds_kml &&
        ds_type != GdaConst::ds_geo_json &&
        ds_type != GdaConst::ds_mapinfo &&
        ds_type != GdaConst::ds_sqlite &&
        ds_type != GdaConst::ds_gpkg &&
        ds_type != GdaConst::ds_csv)
    {
        msg << "Save is not supported on current data source type: "
        << ds_format << ". Please try to use \"File->Save As\" other data source. "
        << "However, the project file can still be saved as other project file.";
		wxMessageDialog dlg (this, msg, "Warning", wxOK | wxICON_INFORMATION);
		dlg.ShowModal();
        m_datasource_path_txt->SetValue("");
        m_chk_create_datasource->SetValue(false);
        return;
    }
    wxString ds_path = project_p->GetDataSource()->GetOGRConnectStr();
    wxString suffix = ds_path.AfterLast('.');
    if ( suffix.empty() ) {
        msg << "The original datasource " << ds_path << " is not a valid file."
        "GeoDa \"Save\" only works on file datasource.";
        wxMessageDialog dlg(this, msg , "Warning", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        return;
    }
    wxString wildcard;
    wildcard << ds_format << "  (*." << suffix << ")|*." << suffix;
    wxFileDialog dlg(this, "Save As Datasource", "", "", wildcard,
                     wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;  
    ds_file_path = dlg.GetPath();
    ds_file_path.SetExt(suffix);
    m_datasource_path_txt->SetValue(ds_file_path.GetFullPath());
    FindWindow(XRCID("wxID_OK"))->Enable(true);
}

void SaveAsDlg::OnBrowseProjectFileBtn ( wxCommandEvent& event )
{
    wxString wildcard = "GeoDa Project (*.gda)|*.gda";
    wxFileDialog dlg(this, "GeoDa Project File to Open", "", "", wildcard,
                     wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;  
    proj_file_path = dlg.GetPath();
    proj_file_path.SetExt("gda");
    m_project_path_txt->SetValue(proj_file_path.GetFullPath());
    FindWindow(XRCID("wxID_OK"))->Enable(true);
}

void SaveAsDlg::OnOkClick( wxCommandEvent& event )
{
	LOG_MSG("Entering SaveAsDlg::OnOkClick");
    try {
        wxString project_fname = m_project_path_txt->GetValue().Trim();
        wxString datasource_fname = m_datasource_path_txt->GetValue().Trim();
      
        bool bSaveProject = m_chk_create_project->IsChecked();
        bool bSaveDatasource = m_chk_create_datasource->IsChecked();
       
        wxString msg;
        
        if ( bSaveProject && !bSaveDatasource ) {
            // SaveAs project only
            if (!project_fname.empty()) {
				project_p->SpecifyProjectConfFile(project_fname);
				project_p->SaveProjectConf();
            } else {
                msg = "Project file path is empty.";
            }
        } else if ( !bSaveProject && bSaveDatasource ) {
            // SaveAs datasource  only
            if (!datasource_fname.empty()) {
                bool is_update = false;
                project_p->SaveDataSourceAs(datasource_fname, is_update);
            } else {
                msg = "Datasource path is empty.";
            }
        } else if ( bSaveProject && bSaveDatasource ) {
            // SaveAs project + datasource
            if ( project_fname.empty() || datasource_fname.empty() ) {
                msg = "Please provide paths for both Project file and "
                "Datasource.";
            } else {
				project_p->SpecifyProjectConfFile(project_fname);
                bool is_update = false;
                project_p->SaveDataSourceAs(datasource_fname, is_update);
                
                wxFileName new_proj_fname(project_fname);
                wxString proj_title = new_proj_fname.GetName();
                wxFileName new_ds_fname(datasource_fname);
                wxString layer_name = new_ds_fname.GetName();
                
                IDataSource* datasource = new FileDataSource(datasource_fname);
                LayerConfiguration* layer_conf =
                    new LayerConfiguration(layer_name, datasource);
                
                VarOrderPtree* var_order = layer_conf->GetVarOrderPtree();
                var_order->ReInitFromTableInt(project_p->GetTableInt());
                CustomClassifPtree* cc = layer_conf->GetCustClassifPtree();
                cc->SetCatClassifList(project_p->GetCatClassifManager());
                WeightsManPtree* spatial_weights =
                    layer_conf->GetWeightsManPtree();
				WeightsNewManager* wnm =
					(WeightsNewManager*) project_p->GetWManInt();
                spatial_weights->
                    SetWeightsMetaInfoList(wnm->GetPtreeEntries());
                ProjectConfiguration* project_conf =
                    new ProjectConfiguration(proj_title, layer_conf);
                project_conf->Save(project_fname);
                
                delete project_conf;
            }
        } else if ( !bSaveProject && !bSaveDatasource ) {
            // SaveAs nothing
            return;
        }
        
        if ( !msg.empty() ) {
            wxMessageDialog dlg(this, msg , "Info", wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
            return;
        }
        msg = "Saved successfully.";
        wxMessageDialog dlg(this, msg , "Info", wxOK | wxICON_INFORMATION);
        dlg.ShowModal();
        EndDialog(wxID_OK);
        
    } catch (GdaException& e) {
        wxMessageDialog dlg (this, e.what(), "Error", wxOK | wxICON_ERROR);
        dlg.ShowModal();
        return;
    }
	LOG_MSG("Exiting SaveAsDlg::OnOkClick");
}
