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

#ifndef __GEODA_CENTER_QUANTILELISA_DLG_H__
#define __GEODA_CENTER_QUANTILELISA_DLG_H__

#include <map>
#include <vector>
#include <wx/wx.h> 
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/string.h>

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../DataViewer/TableInterface.h"
#include "AbstractClusterDlg.h"

class QuantileLisaDlg : public AbstractClusterDlg
{
public:
    QuantileLisaDlg(wxFrame *parent, Project* project);
    virtual ~QuantileLisaDlg();
    
    void CreateControls();
    
    void OnOK( wxCommandEvent& event );
    void OnChangeQuantiles(wxKeyEvent& event);
    void OnCloseClick( wxCommandEvent& event );
    void OnClose(wxCloseEvent& ev);

    virtual wxString _printConfiguration() {return wxEmptyString;}
    virtual void update(TableState* o);

protected:
    wxTextCtrl* txt_quantiles;
    wxChoice* cho_quantile;
    wxTextCtrl* txt_output_field;

    DECLARE_EVENT_TABLE()
};

#endif
