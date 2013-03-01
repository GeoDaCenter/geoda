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

#include <wx/msgdlg.h>
#include "../DialogTools/ProgressDlg.h"
#include "../GenUtils.h"
#include "GalWeight.h"
#include "GwtWeight.h"
#include "WeightsManager.h"
#include "../logger.h"

const int MAX_OPEN_WEIGHTS = 30;

WeightsManager::WeightsManager(int obs)
: observations(obs), current_weight(-1), num_weights(0),
is_default_weight_set(false),
weights(MAX_OPEN_WEIGHTS)
{
}

WeightsManager::~WeightsManager()
{
	clean();
}

bool WeightsManager::clean()
{
	for (int i=0; i<num_weights; i++) {
		if (weights[i]) delete weights[i]; weights[i] = 0;
	}
	weights.clear();
	is_default_weight_set = false;
	current_weight = -1;
	num_weights = 0;
	return true;
}

bool WeightsManager::AddWeightFile(GeoDaWeight* weight, bool set_as_default)
{
	if (weight && (num_weights < MAX_OPEN_WEIGHTS)) {
		if (num_weights == 0) {
			observations = weight->num_obs;
		}
		if (weight->num_obs != observations) {
			wxMessageBox(wxString::Format("Error: attempted to open weights "
										  "file with %d observations when "
										  "currently open DBF file has %d "
										  "observations.", weight->num_obs,
										  observations));
			return false;
		}
		is_default_weight_set = set_as_default;
		for (int i=0; i<num_weights; i++) {
			if (weight->wflnm == weights.at(i)->wflnm) {
				current_weight = i;
				return true;
			}
		}
		weights.at(num_weights) = weight;
		current_weight = num_weights;
		num_weights++;
	} else {
		wxMessageBox(wxString::Format("Error: Can't open more than %d weight "
									  "files.", MAX_OPEN_WEIGHTS));
		return false;
	}
	return true;
}

bool WeightsManager::IsGalWeight(int pos)
{
	return (GetGalWeight(pos) != NULL);
}

bool WeightsManager::IsGwtWeight(int pos)
{
	return (GetGwtWeight(pos) != NULL);
}

GeoDaWeight* WeightsManager::GetWeight(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return NULL;
	return weights.at(pos);
}

GeoDaWeight* WeightsManager::GetCurrWeight()
{
	if ((current_weight < 0) || (current_weight >= num_weights)) {
		return NULL;
	}
	return weights.at(current_weight);
}

GalWeight* WeightsManager::GetGalWeight(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return NULL;
	GeoDaWeight* w = weights.at(pos);
	if (w->weight_type != GeoDaWeight::gal_type) return NULL;
	if (pos != current_weight) is_default_weight_set = false;
	current_weight = pos;
	return (GalWeight*) w;
}

GwtWeight* WeightsManager::GetGwtWeight(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return NULL;
	GeoDaWeight* w = weights.at(pos);
	if (w->weight_type != GeoDaWeight::gwt_type) return NULL;
	if (pos != current_weight) is_default_weight_set = false;
	current_weight = pos;
	return (GwtWeight*) w;
}

wxString WeightsManager::GetWFilename(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return wxEmptyString;
	GeoDaWeight* w = weights.at(pos);
	if (!w) return wxEmptyString;
	return w->wflnm;
}

bool WeightsManager::SetCurrWeightInd(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return false;
	current_weight = pos;
	return true;
}

wxString WeightsManager::GetCurrWFilename()
{
	return GetWFilename(current_weight);
}

wxString WeightsManager::GetCurrWeightTitle()
{
	return GenUtils::GetFileName(GetCurrWFilename());
}

bool WeightsManager::IsWSymmetric(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return false;
	GeoDaWeight* w = weights.at(pos);
	if (!w) return false;
	return w->is_symmetric;
}

void WeightsManager::SetWSymmetric(int pos, bool symmetric)
{
	if ((pos < 0) || (pos >= num_weights)) return;
	GeoDaWeight* w = weights.at(pos);
	if (!w) return;
	w->is_symmetric = symmetric;
}

bool WeightsManager::IsWSymmetricValid(int pos)
{
	if ((pos < 0) || (pos >= num_weights)) return false;
	GeoDaWeight* w = weights.at(pos);
	if (!w) return false;
	return w->symmetry_checked;
}

void WeightsManager::SetWSymmetricValid(int pos, bool valid)
{
	if ((pos < 0) || (pos >= num_weights)) return;
	GeoDaWeight* w = weights.at(pos);
	if (!w) return;
	w->symmetry_checked = valid;
}


bool WeightsManager::CheckWeightSymmetry(GeoDaWeight* w, ProgressDlg* p_dlg)
{
	if (!w->symmetry_checked) {
		if (w->weight_type == GeoDaWeight::gal_type) {
			w->is_symmetric = CheckGalSymmetry((GalWeight*) w, p_dlg);
		} else {
			w->is_symmetric = CheckGwtSymmetry((GwtWeight*) w, p_dlg);
		}
		w->symmetry_checked = true;
	}
	return w->is_symmetric;
}

bool WeightsManager::CheckGalSymmetry(GalWeight* w, ProgressDlg* p_dlg)
{
	LOG_MSG("Entering WeightsManager::CheckGalSymmetry");
	
	int obs = w->num_obs;
	int update_ival = (obs > 100 ? obs/100 : 1);
	
	GalElement* gal = w->gal;
	int tenth = GenUtils::max(1, obs/10);
	for (int i=0; i<obs; i++) {
		if (p_dlg && (i % tenth == 0)) {
			p_dlg->ValueUpdate(i/ (double) obs);
		}
		long* data_i = gal[i].dt();
		long size_i = gal[i].Size();
		// for each neighbor j of i, check that i is a neighbor of j
		for (int j=0; j<size_i; j++) {
			bool found = false;
			long size_j = gal[data_i[j]].Size();
			long* data_j = gal[data_i[j]].dt();
			int k = 0;
			while (!found && k < size_j) {
				if (data_j[k++] == i) found = true;
			}
			if (!found) {
				p_dlg->ValueUpdate(1);
				LOG_MSG(wxString::Format("Non-symmetric GAL file.  Observation "
										 "%d is a neighbor of %d, but %d is not"
										 " a neighbor of %d", data_i[j], i,
										 i, data_i[j]));
				return false;
			}
		}
	}
	p_dlg->ValueUpdate(1);
	LOG_MSG("Exiting WeightsManager::CheckGalSymmetry");
	return true;
}

bool WeightsManager::CheckGwtSymmetry(GwtWeight* w, ProgressDlg* p_dlg)
{
	LOG_MSG("Entering WeightsManager::CheckGwtSymmetry");	
	int obs = w->num_obs;
	int update_ival = (obs > 100 ? obs/100 : 1);
	
	GwtElement* gwt = w->gwt;
	int tenth = GenUtils::max(1, obs/10);
	for (int i=0; i<obs; i++) {
		if (p_dlg && (i % tenth == 0)) {
			p_dlg->ValueUpdate(i/ (double) obs);
		}
		GwtNeighbor* data_i = gwt[i].dt();
		long size_i = gwt[i].Size();
		// for each neighbor j of i, check that i is a neighbor of j
		for (int j=0; j<size_i; j++) {
			bool found = false;
			long size_j = gwt[data_i[j].nbx].Size();
			GwtNeighbor* data_j = gwt[data_i[j].nbx].dt();
			int k = 0;
			while (!found && k < size_j) {
				if (data_j[k++].nbx == i) found = true;
			}
			if (!found) {
				p_dlg->ValueUpdate(1);
				LOG_MSG(wxString::Format("Non-symmetric GWT file.  Observation "
										 "%d is a neighbor of %d, but %d is not"
										 " a neighbor of %d", data_i[j].nbx, i,
										 i, data_i[j].nbx));
				return false;
			}
		}
	}
	p_dlg->ValueUpdate(1);
	LOG_MSG("Exiting WeightsManager::CheckGwtSymmetry");
	return true;
}

void WeightsManager::DumpWeight(GeoDaWeight* w)
{
	if (w->weight_type == GeoDaWeight::gal_type) {
		DumpGal((GalWeight*) w);
	} else {
		DumpGwt((GwtWeight*) w);
	}
}

void WeightsManager::DumpGal(GalWeight* w)
{
	LOG_MSG("Entering WeightsManager::DumpGal");
	GalElement* gal = w->gal;
	int obs = w->num_obs;
	for (int i=0; i<obs; i++) {
		long* data_i = gal[i].dt();
		wxString msg("");
		msg << i << ":";
		for (int j=0, jend=gal[i].Size(); j<jend; j++) {
			msg << " " << data_i[j];
		}
		LOG_MSG(msg);
	}
	LOG_MSG("Exiting WeightsManager::DumpGal");
}

void WeightsManager::DumpGwt(GwtWeight* w)
{
	LOG_MSG("Entering WeightsManager::DumpGwt");
	GwtElement* gwt = w->gwt;
	int obs = w->num_obs;
	for (int i=0; i<obs; i++) {
		GwtNeighbor* data_i = gwt[i].dt();
		wxString msg("");
		msg << i << ":";
		for (int j=0, jend=gwt[i].Size(); j<jend; j++) {
			msg << " (" << data_i[j].nbx << ", " << data_i[j].weight << ")";
		}
		LOG_MSG(msg);
	}
	LOG_MSG("Exiting WeightsManager::DumpGwt");	
}

