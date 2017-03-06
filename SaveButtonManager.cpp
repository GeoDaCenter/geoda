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

#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include "DataViewer/TableState.h"
#include "ShapeOperations/WeightsManState.h"
#include "GeneralWxUtils.h"
#include "GeoDa.h"
#include "logger.h"
#include "SaveButtonManager.h"

SaveButtonManager::SaveButtonManager(TableState* table_state_,
									 WeightsManState* w_man_state_)
: table_state(table_state_), w_man_state(w_man_state_),
metadata_chgs_since_last_save(false), db_chgs_since_last_save(false),
allow_enable_save(false)
{
	table_state->registerObserver(this);
	w_man_state->registerObserver(this);
}

SaveButtonManager::~SaveButtonManager()
{
	table_state->removeObserver(this);
	w_man_state->removeObserver(this);
}

void SaveButtonManager::SetAllowEnableSave(bool enable)
{
	allow_enable_save = enable;
	UpdateSaveMenuItems();
}

bool SaveButtonManager::IsAllowEnableSave()
{
	return allow_enable_save;
}

bool SaveButtonManager::IsSaveNeeded()
{
	return IsMetaDataSaveNeeded() || IsDbSaveNeeded();
}

bool SaveButtonManager::IsMetaDataSaveNeeded()
{
	return metadata_chgs_since_last_save;
}

bool SaveButtonManager::IsDbSaveNeeded()
{
	return db_chgs_since_last_save;
}

void SaveButtonManager::SetMetaDataSaveNeeded(bool save_needed)
{
	metadata_chgs_since_last_save = save_needed;
	UpdateSaveMenuItems();
}

void SaveButtonManager::SetDbSaveNeeded(bool save_needed)
{
	db_chgs_since_last_save = save_needed;
	UpdateSaveMenuItems();
}

void SaveButtonManager::update(TableState* o)
{
	bool md_chgs_prev = metadata_chgs_since_last_save;
	bool db_chgs_prev = db_chgs_since_last_save;
	
	TableState::EventType type = o->GetEventType();
	
	if (type == TableState::cols_delta) {
		// must inspect TableDeltaEntry list to determine if
		// meta-data changed, or db-change.
		const TableDeltaList_type& tdl = o->GetTableDeltaListRef();
		BOOST_FOREACH(const TableDeltaEntry& e, tdl) {
			metadata_chgs_since_last_save = true;
			if (e.change_to_db) db_chgs_since_last_save = true;
		}
	}
	if (type == TableState::col_rename) {
		// must determine if group or simple group rename		
		if (o->IsSimpleGroupRename()) {
			db_chgs_since_last_save = true;
		} else {
			metadata_chgs_since_last_save = true;
		}
	}
	
	if (type == TableState::col_data_change ||
		type == TableState::col_properties_change) {
		db_chgs_since_last_save = true;
	}
	
	if (type == TableState::col_disp_decimals_change ||
		type == TableState::col_order_change ||
		type == TableState::time_ids_add_remove ||
		type == TableState::time_ids_rename ||
		type == TableState::time_ids_swap) {
		metadata_chgs_since_last_save = true;
	}
	
	if (!md_chgs_prev && !db_chgs_prev &&
		(metadata_chgs_since_last_save || db_chgs_since_last_save)) {
		UpdateSaveMenuItems();
	}
}

void SaveButtonManager::update(WeightsManState* o)
{
	bool md_chgs_prev = metadata_chgs_since_last_save;
	metadata_chgs_since_last_save = true;
	if (!md_chgs_prev && metadata_chgs_since_last_save) {
		UpdateSaveMenuItems();
	}
}

void SaveButtonManager::UpdateSaveMenuItems()
{
	GdaFrame* gda_frame = GdaFrame::GetGdaFrame();
	if (!gda_frame) return;
	wxMenuBar* mb = gda_frame->GetMenuBar();
	bool enable_save = IsSaveNeeded() && IsAllowEnableSave();
	gda_frame->EnableTool(XRCID("ID_SAVE_PROJECT"), enable_save);
	GeneralWxUtils::EnableMenuItem(mb, XRCID("ID_SAVE_PROJECT"),
								   enable_save);
}
