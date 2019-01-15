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

#ifndef __GEODA_CENTER_MULTIVAR_SETTINGS_DLG_H___
#define __GEODA_CENTER_MULTIVAR_SETTINGS_DLG_H___

#include <vector>
#include <map>

#include <boost/uuid/uuid.hpp>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include "../VarTools.h"
#include "../Explore/CatClassification.h"
#include "../VarCalc/WeightsMetaInfo.h"
#include "../FramesManagerObserver.h"

class Project;
class TableInterface;

////////////////////////////////////////////////////////////////////////////
//
// class MultiVariableSettingsDlg
//
////////////////////////////////////////////////////////////////////////////
class MultiVariableSettingsDlg : public wxDialog
{
public:
    MultiVariableSettingsDlg(Project* project);
    virtual ~MultiVariableSettingsDlg();
    
    void CreateControls();
    bool Init();
   
    void OnOK( wxCommandEvent& event );
    void OnClose( wxCommandEvent& event );

    void InitVariableCombobox(wxListBox* var_box);
    void InitWeightsCombobox(wxChoice* weights_ch);
    
    boost::uuids::uuid GetWeightsId();
    
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
private:
    bool has_time;
    Project* project;
    TableInterface* table_int;
    std::vector<wxString> tm_strs;
    std::vector<boost::uuids::uuid> weights_ids;
    
    wxListBox* combo_var;
    wxChoice* combo_weights;

    wxArrayString var_items;
	std::map<wxString, wxString> name_to_nm;
	std::map<wxString, int> name_to_tm_id;
};

#endif
