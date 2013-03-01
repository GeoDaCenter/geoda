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

#include "../logger.h"
#include "CatClassifManager.h"

CatClassifManager::CatClassifManager()
{
	
}

CatClassifManager::~CatClassifManager()
{
	for (std::list<CatClassifState*>::iterator it=classif_states.begin();
		 it != classif_states.end(); it++) {
		(*it)->closeAndDeleteWhenEmpty();
	}
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

