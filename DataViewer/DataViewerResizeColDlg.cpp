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

#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include "../logger.h"
#include "TableInterface.h"
#include "DataViewerResizeColDlg.h"

BEGIN_EVENT_TABLE( DataViewerResizeColDlg, wxDialog )
    EVT_BUTTON( XRCID("wxID_OK"), DataViewerResizeColDlg::OnOkClick )
END_EVENT_TABLE()

DataViewerResizeColDlg::DataViewerResizeColDlg(wxGrid* grid_s,
										 TableInterface* table_int_s,
										 wxWindow* parent)
: grid(grid_s), table_int(table_int_s)
{
    
    LOG_MSG("Entering DataViewerResizeColDlg::DataViewerResizeColDlg(..)");
	SetParent(parent);
    CreateControls();
    Centre();
    LOG_MSG("Exiting DataViewerResizeColDlg::DataViewerResizeColDlg(..)");
}


void DataViewerResizeColDlg::CreateControls()
{    
    wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_DATA_VIEWER_RESIZE_COL_DLG");
    if (FindWindow(XRCID("ID_TEXT_COL_NUMBER")))
        m_col_id = wxDynamicCast(FindWindow(XRCID("ID_TEXT_COL_NUMBER")),
								 wxTextCtrl);
	if (FindWindow(XRCID("ID_TEXT_COL_WIDTH")))
        m_col_width = wxDynamicCast(FindWindow(XRCID("ID_TEXT_COL_WIDTH")),
									wxTextCtrl);
}

void DataViewerResizeColDlg::OnOkClick( wxCommandEvent& event )
{
	LOG_MSG("Entering DataViewerResizeColDlg::OnOkClick");
	long temp;
	m_col_id->GetValue().ToCLong(&temp);
	id = (int) temp;
	m_col_width->GetValue().ToCLong(&temp);
	width = (int) temp;
	
	if (id < 0 || id >= grid->GetNumberCols()) {
		wxString msg("Error: Col ID not specified or out-of-range.");
		wxMessageDialog dlg (this, msg, "Error", wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
		
	if (width < -1 || width >= 1000) width = 20;

	event.Skip();
	EndDialog(wxID_OK);
	LOG_MSG("Exiting DataViewerResizeColDlg::OnOkClick");
}
