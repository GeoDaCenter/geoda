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

#ifndef __GEODA_CENTER_CLUSTERMATCH_MAP_VIEW_H__
#define __GEODA_CENTER_CLUSTERMATCH_MAP_VIEW_H__

#include <vector>
#include "MapNewView.h"
#include "../GdaConst.h"

using namespace std;

class ClusterMatchSelectDlg : public AbstractClusterDlg
{
public:
    ClusterMatchSelectDlg(wxFrame *parent, Project* project);
    virtual ~ClusterMatchSelectDlg();
   
    void CreateControls();
    
    virtual void update(TableState* o);
    
    void OnVarSelect( wxCommandEvent& event );
    void OnOK( wxCommandEvent& event );
    void OnClickClose( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);

    virtual wxString _printConfiguration();

    GeoDaWeight* CreateQueenWeights();

protected:
    wxPanel *panel;
    wxBoxSizer *container;
    wxTextCtrl* m_textbox;
    wxArrayInt var_selections;
    
    DECLARE_EVENT_TABLE()
};

#endif
