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

#ifndef __GEODA_CENTER_DATA_VIEWER_H__
#define __GEODA_CENTER_DATA_VIEWER_H__

#include <wx/frame.h>
#include <wx/grid.h>
#include "../FramesManagerObserver.h"

class DbfGridTableBase;
class FramesManager;

class DataViewerFrame : public wxFrame, public FramesManagerObserver
{
public:
	DataViewerFrame(wxFrame *parent, FramesManager* frames_manager,
					const wxString& dbf_fname );
	DataViewerFrame(wxFrame *parent, FramesManager* frames_manager,
					const wxString& dbf_sp_fname, const wxString& dbf_tm_fname,
					int sp_tbl_sp_col, int tm_tbl_sp_col, int tm_tbl_tm_col);
	virtual ~DataViewerFrame();
	
	// Some Default File-menu options for when DataViewer
	// is loaded as a stand-alone program.
	void LoadDefaultMenus();
	
	void OnQuit( wxCommandEvent& ev );
	void About( wxCommandEvent& ev );
	void OnOpenFile( wxCommandEvent& ev );
	void OnPrintTable( wxCommandEvent& ev );
	void OnMoveSelectedToTop( wxCommandEvent& ev );
	static void SortEvent( wxGridEvent& ev, wxGrid* grid,
						  DbfGridTableBase* grid_base );
	void OnSortEvent( wxGridEvent& ev );
	static void ColSizeEvent( wxGridSizeEvent& ev, wxGrid* grid,
							 DbfGridTableBase* grid_base );	
	void OnColSizeEvent( wxGridSizeEvent& ev );
	static void ColMoveEvent( wxGridEvent& ev, wxGrid* grid,
							 DbfGridTableBase* grid_base );	
	void OnColMoveEvent( wxGridEvent& ev );
	void OnMoveCol( wxCommandEvent& ev );
	void OnColResize( wxCommandEvent& ev );
	static void LabelLeftClickEvent( wxGridEvent& ev, wxGrid* grid,
									DbfGridTableBase* grid_base );
	void OnLabelLeftClickEvent( wxGridEvent& ev);
	static void LabelLeftDClickEvent(wxGridEvent& ev, wxGrid* grid,
									 DbfGridTableBase* grid_base);	
	void OnLabelLeftDClickEvent( wxGridEvent& ev);
	void OnCellChanged( wxGridEvent& ev );
	void OnAddCol( wxCommandEvent& ev );
	void OnDeleteCol( wxCommandEvent& ev );
	void OnRandomColOrder( wxCommandEvent& ev );
	void OnPrintGridInfo( wxCommandEvent& ev );
	void OnSaveDbf( wxCommandEvent& ev );
	void OnSaveDbfAs( wxCommandEvent& ev );
	void OnShowDbfInfo( wxCommandEvent& ev );
	void OnEditFieldProperties( wxCommandEvent& ev );
	void OnMergeTable( wxCommandEvent& ev );
	void OnChooseTimeStep( wxCommandEvent& ev );
	void OnShowTimeChooser( wxCommandEvent& ev );

	/** Implementation of FramesManagerObserver interface */
	virtual void update(FramesManager* o);
	
protected:
	FramesManager* frames_manager;
	
private:
	wxGrid* grid;
	DbfGridTableBase* grid_base;
	wxString cur_dbf_fname;
	wxString cur_dbf_tm_fname;
	
	DECLARE_EVENT_TABLE()
};


#endif
