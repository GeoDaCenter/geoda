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
#include "../logger.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "CatClassifManager.h"

CatClassifManager::CatClassifManager(TableInterface* _table_int,
									 TableState* _table_state,
									 CustomClassifPtree* cc_ptree)
: table_state(_table_state), table_int(_table_int)
{
	BOOST_FOREACH(const CatClassifDef& cc, cc_ptree->GetCatClassifList()) {
		CreateNewClassifState(cc);
	}
	table_state->registerObserver(this);
}

CatClassifManager::~CatClassifManager()
{
	for (std::list<CatClassifState*>::iterator it=classif_states.begin();
		 it != classif_states.end(); it++) {
		(*it)->closeAndDeleteWhenEmpty();
	}
	table_state->removeObserver(this);
}

void CatClassifManager::GetTitles(std::vector<wxString>& titles)
{
	titles.resize(classif_states.size());
	int i=0;
	for (std::list<CatClassifState*>::iterator it=classif_states.begin();
		 it != classif_states.end(); it++) {
		titles[i++] = (*it)->GetTitle();
	}
}

CatClassifState* CatClassifManager::FindClassifState(const wxString& title)
{
	CatClassifState* ccs=0;
	for (std::list<CatClassifState*>::iterator it=classif_states.begin();
		 it != classif_states.end() && !ccs; it++) {
		if (title == (*it)->GetTitle()) ccs = (*it);
	}
	return ccs;
}

CatClassifState* CatClassifManager::CreateNewClassifState(
										const CatClassifDef& cc_data)
{
	CatClassifState* ccs = new CatClassifState;
	ccs->SetCatClassif(cc_data);
	classif_states.push_back(ccs);
	return ccs;
}

void CatClassifManager::RemoveClassifState(CatClassifState* ccs)
{
	ccs->closeAndDeleteWhenEmpty();
	classif_states.remove(ccs);
}

bool CatClassifManager::VerifyAgainstTable()
{
	if (!table_int) return false;
	bool any_changed = false;
	for (std::list<CatClassifState*>::iterator it=classif_states.begin();
		 it != classif_states.end(); it++) {
		CatClassifDef& cc = (*it)->GetCatClassif();
		if (CatClassification::CorrectCatClassifFromTable(cc, table_int)) {
			any_changed = true;
		}
	}
	return any_changed;
}

void CatClassifManager::update(TableState* o)
{
	std::list<CatClassifState*>::iterator i;
	if (o->GetEventType() == TableState::col_rename) {
		if (!o->IsSimpleGroupRename()) return;
		for (i = classif_states.begin(); i != classif_states.end(); ++i) {
			if ((*i)->GetCatClassif().assoc_db_fld_name.CmpNoCase(o->GetOldColName())==0) {
				(*i)->GetCatClassif().assoc_db_fld_name = o->GetNewColName();
			}
		}
	} else if (o->GetEventType() == TableState::cols_delta) {
		const TableDeltaList_type& tdl = o->GetTableDeltaListRef();
		BOOST_FOREACH(const TableDeltaEntry& e, tdl) {
			// remove association for every deleted field
			if (!e.insert) {
				for (i = classif_states.begin(); i!=classif_states.end(); ++i) {
					if ((*i)->GetCatClassif().assoc_db_fld_name.CmpNoCase(e.group_name)==0)
					{
						(*i)->GetCatClassif().assoc_db_fld_name = "";
						(*i)->notifyObservers();
					}
				}
			}
		}
	} else if (o->GetEventType() == TableState::col_data_change) {
		for (i = classif_states.begin(); i != classif_states.end(); ++i) {
			CatClassifDef& cc = (*i)->GetCatClassif();
			if (cc.assoc_db_fld_name == o->GetModifiedColName() &&
				cc.cat_classif_type != CatClassification::custom)
			{
				// reset breaks and notify observers
				int col = -1, tm = 0;
				bool found = table_int->DbColNmToColAndTm(cc.assoc_db_fld_name,
														  col, tm);
				if (!found) continue;
				std::vector<double> v;
                std::vector<bool> v_undef;
				table_int->GetColData(col, tm, v);
                table_int->GetColUndefined(col, tm, v_undef);
                
				int num_obs = table_int->GetNumberRows();
				Gda::dbl_int_pair_vec_type data(num_obs);
                
				for (int ii=0; ii<num_obs; ++ii) {
					data[ii].first = v[ii];
					data[ii].second = ii;
				}
				std::sort(data.begin(), data.end(), Gda::dbl_int_pair_cmp_less);
				CatClassifDef _cc = cc;
				CatClassification::SetBreakPoints(_cc.breaks, _cc.names, data,
												  v_undef, _cc.cat_classif_type,
												  _cc.num_cats);
				if (_cc != cc) {
					cc = cc;
					(*i)->notifyObservers();
				}
			} else if (cc.assoc_db_fld_name == o->GetModifiedColName() &&
					   cc.cat_classif_type == CatClassification::custom) {
				int col = -1, tm = 0;
				bool found = table_int->DbColNmToColAndTm(cc.assoc_db_fld_name,
														  col, tm);
				if (!found) continue;
				CatClassifDef _cc = cc;
				// ensure that breaks and min/max are within new
				// min/max bounds
				double new_min = cc.uniform_dist_min;
				double new_max = cc.uniform_dist_max;
				table_int->GetMinMaxVals(col, tm, new_min, new_max);
				if (cc.uniform_dist_min < new_min) {
					cc.uniform_dist_min = new_min;
				}
				if (cc.uniform_dist_max > new_max) {
					cc.uniform_dist_max = new_max;
				}
				for (int ii=0, sz=cc.breaks.size(); ii<sz; ++ii) {
					if (cc.breaks[ii] < new_min) cc.breaks[ii] = new_min;
					if (cc.breaks[ii] > new_max) cc.breaks[ii] = new_max;
				}
				if (_cc != cc) {
					cc = cc;
					(*i)->notifyObservers();
				}
			}
		}
	}
}
