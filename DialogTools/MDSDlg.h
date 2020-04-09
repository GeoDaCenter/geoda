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

#ifndef __GEODA_CENTER_MDS_DLG_H__
#define __GEODA_CENTER_MDS_DLG_H__

#include <map>
#include <vector>
#include <wx/dialog.h>
#include <wx/listbox.h>

#include "../VarTools.h"
#include "AbstractClusterDlg.h"

class MDSDlg : public AbstractClusterDlg
{
public:
    MDSDlg(wxFrame *parent, Project* project);
    virtual ~MDSDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );

    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);
    void OnCheckPowerIteration( wxCommandEvent& event );
    void OnMethodChoice(wxCommandEvent &event);
    void InitVariableCombobox(wxListBox* var_box);
    
    virtual wxString _printConfiguration();
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:
    wxChoice* combo_method;
    SimpleReportTextCtrl* m_textbox;
    wxTextCtrl* m_iterations;
    wxTextCtrl* m_eps;
    wxChoice* combo_n;

    wxChoice* m_distance;
    wxCheckBox* chk_poweriteration;
    wxTextCtrl* txt_poweriteration;
    wxStaticText* lbl_poweriteration;
    wxStaticText* txt_usepower;
    wxStaticText* txt_maxit;
    wxStaticText* txt_eps;
    
    DECLARE_EVENT_TABLE()
};

#endif
