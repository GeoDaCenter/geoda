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

#ifndef __GEODA_CENTER_WEIGHTS_MANAGER_H__
#define __GEODA_CENTER_WEIGHTS_MANAGER_H__

#include <list>
#include <map>
#include <vector>
#include "WeightsManPtree.h"
#include "../VarCalc/WeightsManInterface.h"
class GeoDaWeight;
class GalWeight;
class GwtWeight;
class GalElement;
class GwtElement;
class ProgressDlg;
class TableInterface;
class WeightsManState;
class Project;

class WeightsNewManager : public WeightsManInterface
{
public:
	WeightsNewManager(WeightsManState* w_man_state,
					  TableInterface* table_int);
    ~WeightsNewManager();
    
	void Init(const std::list<WeightsPtreeEntry>& entries);
	std::list<WeightsPtreeEntry> GetPtreeEntries() const;
	bool AssociateGal(boost::uuids::uuid w_uuid, GalWeight* gw);
	
	// Implementation of WeightsManInterface
	virtual void GetIds(std::vector<boost::uuids::uuid>& ids) const;
	virtual boost::uuids::uuid FindIdByMetaInfo(const WeightsMetaInfo& wmi) const;
	virtual boost::uuids::uuid FindIdByFilename(const wxString& file) const;
	virtual boost::uuids::uuid FindIdByTitle(const wxString& title) const;
	virtual boost::uuids::uuid RequestWeights(const WeightsMetaInfo& wmi);
	virtual bool WeightsExists(boost::uuids::uuid) const;
	virtual WeightsMetaInfo GetMetaInfo(boost::uuids::uuid w_uuid) const;
	virtual wxString GetShortDispName(boost::uuids::uuid w_uuid) const;
	virtual wxString GetLongDispName(boost::uuids::uuid w_uuid) const;
	virtual std::list<boost::uuids::uuid> GetIds() const;
	virtual WeightsMetaInfo::SymmetryEnum IsSym(boost::uuids::uuid w_uuid) const;
	virtual WeightsMetaInfo::SymmetryEnum CheckSym(boost::uuids::uuid w_uuid,
												   ProgressDlg* p_dlg=0);
	virtual bool Lag(boost::uuids::uuid w_uuid, const GdaFlexValue& data,
					 GdaFlexValue& result);
	virtual bool GetCounts(boost::uuids::uuid w_uuid,
						   std::vector<long>& counts);
	virtual void GetNbrsExclCores(boost::uuids::uuid w_uuid,
								  const std::set<long>& cores,
								  std::set<long>& nbrs);	
	virtual void Remove(boost::uuids::uuid w_uuid);
	virtual wxString RecNumToId(boost::uuids::uuid w_uuid, long rec_num);
	virtual GalWeight* GetGal(boost::uuids::uuid w_uuid);
	virtual GeoDaWeight* GetWeights(boost::uuids::uuid w_uuid);
	virtual boost::uuids::uuid GetDefault() const;
	virtual void MakeDefault(boost::uuids::uuid w_uuid);
	virtual boost::uuids::uuid FindByTitle(const wxString& s) const;
	virtual wxString SuggestTitleFromFileName(const wxString& fname) const;
	virtual wxString GetTitle(boost::uuids::uuid w_uuid) const;
	virtual void SetTitle(boost::uuids::uuid w_uuid, const wxString& s);
	virtual bool IsValid(boost::uuids::uuid w_uuid);
	
private:
	struct Entry {
		Entry() : gal_weight(0), geoda_weight(0) {}
		Entry(const WeightsPtreeEntry& e) : gal_weight(0), geoda_weight(0), wpte(e) {}
		WeightsPtreeEntry wpte;
		GalWeight* gal_weight;
        GeoDaWeight* geoda_weight;
		std::vector<wxString> rec_num_to_id;
	};
	typedef std::map<boost::uuids::uuid, Entry> EmType;
	typedef EmType::const_iterator EmTypeCItr;
	// entry_map and uuid_order must be kept perfectly in sync
	EmType entry_map;
	std::list<boost::uuids::uuid> uuid_order;
	
	boost::uuids::uuid FindUuid(const WeightsMetaInfo& wmi) const;
	GalElement* GetGalElemArray(boost::uuids::uuid w_uuid);
	bool InitRecNumToIdMap(boost::uuids::uuid w_uuid);
	TableInterface* table_int;
	WeightsManState* w_man_state;
};


namespace GdaWeightsTools {
	bool CheckWeightSymmetry(GeoDaWeight* w, ProgressDlg* p_dlg=0);
	void DumpWeight(GeoDaWeight* w);
	
	bool CheckGalSymmetry(GalWeight* w, ProgressDlg* p_dlg=0);
	bool CheckGwtSymmetry(GwtWeight* w, ProgressDlg* p_dlg=0);
	void DumpGal(GalWeight* w);
	void DumpGwt(GwtWeight* w);
}

#endif
