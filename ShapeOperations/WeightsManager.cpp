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


#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <wx/msgdlg.h>
#include "../DialogTools/ProgressDlg.h"
#include "../GenUtils.h"
#include "../DataViewer/TableInterface.h"
#include "WeightsManState.h"
#include "GeodaWeight.h"
#include "GalWeight.h"
#include "GwtWeight.h"
#include "WeightUtils.h"
#include "WeightsManager.h"
#include "../Project.h"
#include "../SaveButtonManager.h"
#include "../logger.h"
#include "../VarCalc/GdaLexer.h"


WeightsNewManager::WeightsNewManager(WeightsManState* w_man_state_,
									 TableInterface* table_int_)
: w_man_state(w_man_state_), table_int(table_int_)
{
}

WeightsNewManager::~WeightsNewManager()
{
    
	for (EmTypeCItr it=entry_map.begin(); it != entry_map.end(); ++it) {
        Entry e = it->second;
        if (e.gal_weight) {
            delete e.gal_weight;
            e.gal_weight = NULL;
        }
        if (e.geoda_weight) {
            delete e.geoda_weight;
            e.geoda_weight = NULL;
        }
	}
}

void WeightsNewManager::Init(const std::list<WeightsPtreeEntry>& entries)
{
	entry_map.clear();
	uuid_order.clear();
	// We will ignore existing titles from disk and instead
	// construct titles based on filename.
	std::set<wxString> titles;
	
	BOOST_FOREACH(const WeightsPtreeEntry& pte, entries) {
		wxString title = wxFileName(pte.wmi.filename).GetName();
		if (title.IsEmpty()) title = "weights";
		if (titles.find(title.Lower()) != titles.end()) {
			bool done = false;
			for (int i=0; i<1000 && !done; ++i) {
				wxString tmp = title;
				tmp << "_" << i+1;
				if (titles.find(tmp.Lower()) == titles.end()) {
					done = true;
					title = tmp;
				}
			}
		}
		titles.insert(title.Lower());
		Entry e(pte);
		e.wpte.title = title;
		boost::uuids::uuid u = boost::uuids::random_generator()();
		entry_map[u] = e;
		uuid_order.push_back(u);
	}
}

std::list<WeightsPtreeEntry> WeightsNewManager::GetPtreeEntries() const
{
	std::list<WeightsPtreeEntry> p;
	BOOST_FOREACH(const boost::uuids::uuid& w_uuid, uuid_order) {
		p.push_back(entry_map.find(w_uuid)->second.wpte);
	}
	return p;
}

/**
 This should only be used by CreatingWeights to associate a newly created
 gal with a newly added weights. If a GalWeight already exists, then it
 will be first deleted.  WeightsNewManager assumes ownership of GalWeight!
 If flase is returned, the caller should free GalWeight.
 */
bool WeightsNewManager::AssociateGal(boost::uuids::uuid w_uuid, GalWeight* gw)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return false;
	if (it->second.gal_weight != 0) {
		delete it->second.gal_weight; it->second.gal_weight = 0;
	}
	it->second.gal_weight = gw;
	if (w_man_state) w_man_state->notifyObservers();
	return true;
}



void WeightsNewManager::GetIds(std::vector<boost::uuids::uuid>& ids) const
{
	ids.clear();
	BOOST_FOREACH(boost::uuids::uuid u, uuid_order) ids.push_back(u);
}

boost::uuids::uuid WeightsNewManager::FindIdByMetaInfo(const WeightsMetaInfo& wmi) const
{
	return FindUuid(wmi);
}

boost::uuids::uuid WeightsNewManager::FindIdByFilename(const wxString& file) const
{
	for (EmTypeCItr it=entry_map.begin(); it != entry_map.end(); ++it) {
		if (it->second.wpte.wmi.filename.CmpNoCase(file) == 0) return it->first;
	}
	return boost::uuids::nil_uuid();
}

boost::uuids::uuid WeightsNewManager::FindIdByTitle(const wxString& title) const
{
	for (EmTypeCItr it=entry_map.begin(); it != entry_map.end(); ++it) {
		if (it->second.wpte.title.CmpNoCase(title) == 0) return it->first;
	}
	return boost::uuids::nil_uuid();
}

boost::uuids::uuid WeightsNewManager::RequestWeights(const WeightsMetaInfo& wmi)
{
    //XXX: seems no need to check again
	//boost::uuids::uuid u = FindUuid(wmi);
	//if (!u.is_nil()) return u;
    boost::uuids::uuid u = boost::uuids::random_generator()();
	WeightsPtreeEntry pte(wmi);
	Entry e(pte);
	e.wpte.title = SuggestTitleFromFileName(wmi.filename);
	entry_map[u] = e;
	uuid_order.push_back(u);
	if (w_man_state) {
		w_man_state->SetAddEvtTyp(u);
		w_man_state->notifyObservers();
	}
	return u;
}

bool WeightsNewManager::WeightsExists(boost::uuids::uuid w_uuid) const
{
	return entry_map.find(w_uuid) != entry_map.end();
}

WeightsMetaInfo WeightsNewManager::GetMetaInfo(boost::uuids::uuid w_uuid) const
{
	if (!WeightsExists(w_uuid)) return WeightsMetaInfo();
	EmTypeCItr ci = entry_map.find(w_uuid);
	return (*ci).second.wpte.wmi;
}

wxString WeightsNewManager::GetShortDispName(boost::uuids::uuid w_uuid) const
{
	return GetTitle(w_uuid);
	//EmTypeCItr ci = entry_map.find(w_uuid);
	//if (ci == entry_map.end()) return "";
	//const WeightsPtreeEntry& pte = ci->second.wpte;
	//if (!pte.title.IsEmpty()) return pte.title;
	//if (!pte.wmi.filename.IsEmpty()) {
	//	return wxFileName(pte.wmi.filename).GetName();
	//}
	//return wxString(boost::uuids::to_string(w_uuid));
}

wxString WeightsNewManager::GetLongDispName(boost::uuids::uuid w_uuid) const
{
	return GetTitle(w_uuid);
	//EmTypeCItr ci = entry_map.find(w_uuid);
	//if (ci == entry_map.end()) return "";
	//const WeightsPtreeEntry& pte = ci->second.wpte;
	//if (!pte.title.IsEmpty()) return pte.title;
	//if (!pte.wmi.filename.IsEmpty()) {
	//	return wxFileName(pte.wmi.filename).GetName();
	//}
	//return wxString(boost::uuids::to_string(w_uuid));
}

std::list<boost::uuids::uuid> WeightsNewManager::GetIds() const
{
	return uuid_order;
}

WeightsMetaInfo::SymmetryEnum
	WeightsNewManager::IsSym(boost::uuids::uuid w_uuid) const
{
	EmTypeCItr it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return WeightsMetaInfo::SYM_unknown;
	return it->second.wpte.wmi.sym_type;
}

WeightsMetaInfo::SymmetryEnum
	WeightsNewManager::CheckSym(boost::uuids::uuid w_uuid, ProgressDlg* p_dlg)
{
	if (IsSym(w_uuid) != WeightsMetaInfo::SYM_unknown) return IsSym(w_uuid);
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return WeightsMetaInfo::SYM_unknown;
	Entry& e = it->second;
	GalWeight* w = GetGal(w_uuid);
	if (w == 0 || w->gal == 0) {
		e.wpte.wmi.sym_type = WeightsMetaInfo::SYM_unknown;
	} else if (GdaWeightsTools::CheckGalSymmetry(w, p_dlg)) {
		e.wpte.wmi.sym_type = WeightsMetaInfo::SYM_symmetric;
	} else {
		e.wpte.wmi.sym_type = WeightsMetaInfo::SYM_asymmetric;
	}
	// if (w_man_state) w_man_state->notifyObservers();
	// should notify SaveButtonManager that meta-data changed.
	return e.wpte.wmi.sym_type;
}

bool WeightsNewManager::Lag(boost::uuids::uuid w_uuid, const GdaFlexValue& data,
							GdaFlexValue& result)
{
	if (!WeightsExists(w_uuid)) {
		return false;
	}
	if (data.GetObs() < 1) {
		return false;
	}
	if (data.GetObs() == 1) {
		// Will assume this is part of initial syntax checking stage and will
		// return original data;
		result = data;
		return true;
	}
	GalWeight* gw = GetGal(w_uuid);
	if (!gw || !gw->gal || !(gw->num_obs = data.GetObs())) {
		return false;
	}
	GalElement* W = gw->gal;
	const std::valarray<double>& x = data.GetConstValArrayRef();
	result.SetSize(data.GetObs(), data.GetTms());
	std::valarray<double>& y = result.GetValArrayRef();
	for (size_t t=0, tms=data.GetTms(); t<tms; ++t) {
		for (size_t i=0, obs=data.GetObs(); i<obs; ++i) {
			double s = 0;
			size_t nbrs = W[i].Size();
			for (size_t n=0; n<nbrs; ++n) {
				s += x[W[i][n]*tms+t];
			}
			y[i*tms+t] = s / ((double) nbrs);
		}
	}
	return true;
}

bool WeightsNewManager::GetCounts(boost::uuids::uuid w_uuid,
								  std::vector<long>& counts)
{
	counts.resize(table_int->GetNumberRows());
	GalWeight* gw = GetGal(w_uuid);
	if (gw == 0) {
		for (size_t i=0, sz=counts.size(); i<sz; ++i) {
			counts[i] = 0;
		}
		return false;
	}
	for (size_t i=0, sz=counts.size(); i<sz; ++i) {
		counts[i] = gw->gal[i].Size();
	}
	return true;
}

void WeightsNewManager::GetNbrsExclCores(boost::uuids::uuid w_uuid,
										 const std::set<long>& cores,
										 std::set<long>& nbrs)
{
	using namespace std;
	nbrs.clear();
	GalWeight* gw = GetGal(w_uuid);
	if (!gw || !gw->gal) return;
	GalElement* W = gw->gal;
	long num_obs = table_int->GetNumberRows();
	BOOST_FOREACH(long c, cores) {
		if (c >= num_obs || c < 0) continue;
		BOOST_FOREACH(long n, W[c].GetNbrs()) {
			if (cores.find(n) == cores.end()) nbrs.insert(n);
		}
	}
}

void WeightsNewManager::Remove(boost::uuids::uuid w_uuid)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return;
	if (it->second.gal_weight) delete it->second.gal_weight;
	entry_map.erase(it);
	for (std::list<boost::uuids::uuid>::iterator it=uuid_order.begin();
		 it != uuid_order.end(); ++it) {
		if (w_uuid == (*it)) {
			uuid_order.erase(it);
			break;
		}
	}
	if (w_man_state) {
		w_man_state->SetRemoveEvtTyp(w_uuid);
		w_man_state->notifyObservers();
	}
}

wxString WeightsNewManager::RecNumToId(boost::uuids::uuid w_uuid, long rec_num)
{
	if (!InitRecNumToIdMap(w_uuid)) {
		wxString r;
		r << rec_num+1;
		return r;
	}
	EmType::iterator it = entry_map.find(w_uuid);
	return it->second.rec_num_to_id[rec_num];
}

/** If gal_weight doesn't yet exist, then create it from meta-data if
 possible. */
GalWeight* WeightsNewManager::GetGal(boost::uuids::uuid w_uuid)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return 0;
	Entry& e = it->second;
    wxString tmpName = e.wpte.wmi.filename;
    if (e.gal_weight) {
        return e.gal_weight;
    }
	
	// Load file for first use
	wxFileName t_fn(e.wpte.wmi.filename);
	wxString ext = t_fn.GetExt().Lower();
	if (ext != "gal" && ext != "gwt") {
		return 0;
	}
	GalElement* gal=0;
	if (ext == "gal") {
		gal = WeightUtils::ReadGal(e.wpte.wmi.filename, table_int);
	} else { // ext == "gwt"
		gal = WeightUtils::ReadGwtAsGal(e.wpte.wmi.filename, table_int);
	}
	if (gal != 0) {
		GalWeight* w = new GalWeight();
		w->num_obs = table_int->GetNumberRows();
		w->wflnm = e.wpte.wmi.filename;
        w->id_field = e.wpte.wmi.id_var;
		w->title = e.wpte.title;
		w->gal = gal;
		e.gal_weight = w;
	}
	return e.gal_weight;
}

GeoDaWeight* WeightsNewManager::GetWeights(boost::uuids::uuid w_uuid)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return 0;
	Entry& e = it->second;
    wxString tmpName = e.wpte.wmi.filename;
    
    wxFileName t_fn(tmpName);
    wxString ext = t_fn.GetExt().Lower();
    if (ext != "gal" && ext != "gwt") {
        return 0;
    }
    
	if (ext == "gal" && e.gal_weight) return e.gal_weight;
	
	// Load file for first use
	
	if (ext == "gal") {
        GalElement* gal = WeightUtils::ReadGal(e.wpte.wmi.filename, table_int);
    	if (gal != 0) {
    		GalWeight* w = new GalWeight();
    		w->num_obs = table_int->GetNumberRows();
    		w->wflnm = e.wpte.wmi.filename;
            w->id_field = e.wpte.wmi.id_var;
    		w->title = e.wpte.title;
    		w->gal = gal;
    		e.geoda_weight = (GeoDaWeight*)w;
    	}
        
	} else { // ext == "gwt"
        GwtElement* gwt = WeightUtils::ReadGwt(e.wpte.wmi.filename, table_int);
    	if (gwt != 0) {
    		GwtWeight* w = new GwtWeight();
    		w->num_obs = table_int->GetNumberRows();
    		w->wflnm = e.wpte.wmi.filename;
            w->id_field = e.wpte.wmi.id_var;
    		w->title = e.wpte.title;
    		w->gwt = gwt;
    		e.geoda_weight = (GeoDaWeight*)w;
    	}
	}
	return e.geoda_weight;
}

boost::uuids::uuid WeightsNewManager::GetDefault() const
{
	for (EmTypeCItr it=entry_map.begin(); it!=entry_map.end(); ++it) {
		if (it->second.wpte.is_default) return it->first;
	}
	if (uuid_order.empty()) {
		return boost::uuids::nil_uuid();
	} else {
		return uuid_order.front();
	}
}

void WeightsNewManager::MakeDefault(boost::uuids::uuid w_uuid)
{
	for (EmType::iterator it=entry_map.begin(); it!=entry_map.end(); ++it) {
		if (it->first == w_uuid) {
			it->second.wpte.is_default = true;
		} else {
			it->second.wpte.is_default = false;
		}
	}
}

boost::uuids::uuid WeightsNewManager::FindByTitle(const wxString& s) const
{
	for (EmTypeCItr it=entry_map.begin(); it!=entry_map.end(); ++it) {
		if (it->second.wpte.title.CmpNoCase(s) == 0) return it->first;
	}
	return boost::uuids::nil_uuid();
}

/** Suggests a new, unique title based on a filename. */
wxString WeightsNewManager::SuggestTitleFromFileName(const wxString& fname) const
{
	std::set<wxString> titles;
	BOOST_FOREACH(const boost::uuids::uuid& w_uuid, uuid_order) {
		titles.insert(GetTitle(w_uuid).Lower());
	}
	wxString title = wxFileName(fname).GetName();
	if (title.IsEmpty()) title = "weights";
	if (titles.find(title.Lower()) != titles.end()) {
		bool done = false;
		for (int i=0; i<1000 && !done; ++i) {
			wxString tmp = title;
			tmp << "_" << i+1;
			if (titles.find(tmp.Lower()) == titles.end()) {
				done = true;
				title = tmp;
			}
		}
	}
	return title;
}

wxString WeightsNewManager::GetTitle(boost::uuids::uuid w_uuid) const
{
	EmTypeCItr it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return "";
	return it->second.wpte.title;
}

void WeightsNewManager::SetTitle(boost::uuids::uuid w_uuid, const wxString& s)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return;
	it->second.wpte.title = s;
	if (w_man_state) {
		w_man_state->SetNameChangeEvtTyp(w_uuid);
		w_man_state->notifyObservers();
	}
}

bool WeightsNewManager::IsValid(boost::uuids::uuid w_uuid)
{
	// some logic to validate weight.  Does it load, are the dimensions correct?
	GalWeight* gw = GetGal(w_uuid);
	if (!gw || !gw->gal) return false;
	return true;
}


boost::uuids::uuid WeightsNewManager::FindUuid(const WeightsMetaInfo& wmi) const
{
	for (EmTypeCItr it=entry_map.begin(); it != entry_map.end(); ++it) {
		if (it->second.wpte.wmi == wmi)
            return it->first;
	}
	return boost::uuids::nil_uuid();
}

GalElement* WeightsNewManager::GetGalElemArray(boost::uuids::uuid w_uuid)
{
	GalWeight* gw = GetGal(w_uuid);
	if (!gw) return 0;
	return gw->gal;
}

/** If false then w_uuid could not be found. */
bool WeightsNewManager::InitRecNumToIdMap(boost::uuids::uuid w_uuid)
{
	EmType::iterator it = entry_map.find(w_uuid);
	if (it == entry_map.end()) return false;
	Entry& e = it->second;
	if (e.rec_num_to_id.size() > 0) return true;
	e.rec_num_to_id.resize(table_int->GetNumberRows());
	int col = table_int->FindColId(e.wpte.wmi.id_var);
	if (col >= 0) {
		if (table_int->GetColType(col) == GdaConst::string_type) {
			std::vector<wxString> v;
			table_int->GetColData(col, 0, v);
			for (size_t i=0, sz=v.size(); i<sz; i++) {
				e.rec_num_to_id[i] = v[i];
			}
		} else if (table_int->GetColType(col) == GdaConst::double_type) {
			std::vector<double> v;
			table_int->GetColData(col, 0, v);
			for (size_t i=0, sz=v.size(); i<sz; i++) {
				e.rec_num_to_id[i] << v[i];
			}
		} else {
			std::vector<wxInt64> v;
			table_int->GetColData(col, 0, v);
			for (size_t i=0, sz=v.size(); i<sz; i++) {
				e.rec_num_to_id[i] << v[i];
			}
		}
		return true;
	}
	for (size_t i=0, sz=e.rec_num_to_id.size(); i<sz; i++) {
		e.rec_num_to_id[i] << (i+1);
	}
	return false;
}


bool GdaWeightsTools::CheckWeightSymmetry(GeoDaWeight* w, ProgressDlg* p_dlg)
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

bool GdaWeightsTools::CheckGalSymmetry(GalWeight* w, ProgressDlg* p_dlg)
{
    int obs = w->num_obs;
	int update_ival = (obs > 100 ? obs/100 : 1);
	
	GalElement* gal = w->gal;
	int tenth = GenUtils::max(1, obs/10);
	for (int i=0; i<obs; i++) {
		if (p_dlg && (i % tenth == 0)) {
			p_dlg->ValueUpdate(i/ (double) obs);
		}
		const GalElement& elm_i = gal[i];
		// for each neighbor j of i, check that i is a neighbor of j
		for (int j=0, sz_i=elm_i.Size(); j<sz_i; j++) {
			bool found = false;
			const GalElement& elm_j = gal[elm_i[j]];
			long sz_j = elm_j.Size();
			int k = 0;
			while (!found && k < sz_j) {
				if (elm_j[k++] == i) found = true;
			}
			if (!found) {
				p_dlg->ValueUpdate(1);
                
				return false;
			}
		}
	}
	p_dlg->ValueUpdate(1);
	return true;
}

bool GdaWeightsTools::CheckGwtSymmetry(GwtWeight* w, ProgressDlg* p_dlg)
{
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
				return false;
			}
		}
	}
	p_dlg->ValueUpdate(1);
	return true;
}

void GdaWeightsTools::DumpWeight(GeoDaWeight* w)
{
	if (w->weight_type == GeoDaWeight::gal_type) {
		DumpGal((GalWeight*) w);
	} else {
		DumpGwt((GwtWeight*) w);
	}
}

void GdaWeightsTools::DumpGal(GalWeight* w)
{
	GalElement* gal = w->gal;
	int obs = w->num_obs;
	for (int i=0; i<obs; i++) {
		const GalElement& elm_i = gal[i];
		wxString msg("");
		msg << i << ":";
		for (int j=0, jend=elm_i.Size(); j<jend; j++) {
			msg << " " << elm_i[j];
		}
	}
}

void GdaWeightsTools::DumpGwt(GwtWeight* w)
{
	GwtElement* gwt = w->gwt;
	int obs = w->num_obs;
	for (int i=0; i<obs; i++) {
		GwtNeighbor* data_i = gwt[i].dt();
		wxString msg("");
		msg << i << ":";
		for (int j=0, jend=gwt[i].Size(); j<jend; j++) {
			msg << " (" << data_i[j].nbx << ", " << data_i[j].weight << ")";
		}
	}
}

