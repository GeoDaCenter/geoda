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

#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/filedlg.h>
#include "../FramesManager.h"
#include "../GeoDaConst.h"
#include "../DialogTools/OpenSpaceTimeDlg.h"
#include "../DialogTools/TimeChooserDlg.h"
#include "DataViewerApp.h"

extern void DataViewerInitXmlResource();

IMPLEMENT_APP( DataViewerApp )

bool DataViewerApp::OnInit()
{
	GeoDaConst::init();
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxXPMHandler);
	wxXmlResource::Get()->InitAllHandlers();
	DataViewerInitXmlResource();
	
	frames_manager = new FramesManager;
	
	OpenSpaceTimeDlg dlg(true);
	if (dlg.ShowModal() != wxID_OK) return false;
	DataViewerFrame* frame =
		new DataViewerFrame(0, frames_manager,
							dlg.time_invariant_dbf_name.GetFullPath(),
							dlg.time_variant_dbf_name.GetFullPath(),
							dlg.sp_table_space_col,
							dlg.tm_table_space_col,
							dlg.tm_table_time_col);
	frame->LoadDefaultMenus();
	frame->Show(true);
	
	frames_manager->closeAndDeleteWhenEmpty();
	
	//TimeChooserDlg* time_chooser = new TimeChooserDlg(0, frames_manager);
	//time_chooser->Show(true);
	
	
	//wxFileDialog dlg(0, "Choose DBF file", "", "",
	//				 "DBF files (*.dbf)|*.dbf");
	//if (dlg.ShowModal() != wxID_OK) return false;
	//DataViewerFrame* frame = new DataViewerFrame(0, frames_manager,
	//  dlg.GetPath());
	//frame->LoadDefaultMenus();
	//frame->Show(true);
	return true;
}

