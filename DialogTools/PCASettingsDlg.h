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

#ifndef __GEODA_CENTER_PCA_SET_DLG_H__
#define __GEODA_CENTER_PCA_SET_DLG_H__


#include <vector>
#include <wx/combobox.h>
#include <wx/gauge.h>

#include "../DataViewer/TableStateObserver.h"
#include "../GeneralWxUtils.h"
#include "../FramesManager.h"
#include "../FramesManagerObserver.h"
#include "../Project.h"
#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class PCASettingsDlg : public AbstractClusterDlg
{
public:
    PCASettingsDlg(wxFrame* parent, Project* project);
    virtual ~PCASettingsDlg();
    
    void CreateControls();

    void OnOK( wxCommandEvent& event );
    void OnSave( wxCommandEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);

    virtual wxString _printConfiguration();
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
private:
    wxChoice* combo_n;
    wxChoice* combo_method;
    wxButton* saveButton;
    SimpleReportTextCtrl* m_textbox;
    
    unsigned int row_lim;
    unsigned int col_lim;
    std::vector<float> scores;
    float thresh95;
    
    DECLARE_EVENT_TABLE()
};

#endif
