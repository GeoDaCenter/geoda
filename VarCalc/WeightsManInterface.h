/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_WEIGHTS_MAN_INTERFACE_H__
#define __GEODA_CENTER_WEIGHTS_MAN_INTERFACE_H__

#include <list>
#include <set>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "WeightsMetaInfo.h"
#include "GdaFlexValue.h"
class GalWeight;
class GeoDaWeight;
class ProgressDlg;


// ToDo: There is no need to have a "WeightsManInterface" since there is only
// one implementation: WeightsNewManager, which should be used instead of an
// interface.

class WeightsManInterface
{
public:
	virtual void GetIds(std::vector<boost::uuids::uuid>& ids) const = 0;
	virtual boost::uuids::uuid FindIdByMetaInfo(const WeightsMetaInfo& wmi) const = 0;
	virtual boost::uuids::uuid FindIdByFilename(const wxString& file) const = 0;
	virtual boost::uuids::uuid FindIdByTitle(const wxString& title) const = 0;
	virtual boost::uuids::uuid RequestWeights(const WeightsMetaInfo& wmi) = 0;
	virtual bool WeightsExists(boost::uuids::uuid w_uuid) const = 0;
	virtual WeightsMetaInfo GetMetaInfo(boost::uuids::uuid w_uuid) const = 0;
	virtual wxString GetShortDispName(boost::uuids::uuid w_uuid) const = 0;
	virtual wxString GetLongDispName(boost::uuids::uuid w_uuid) const = 0;
	virtual std::list<boost::uuids::uuid> GetIds() const = 0;
	virtual WeightsMetaInfo::SymmetryEnum IsSym(boost::uuids::uuid w_uuid) const = 0;
	virtual WeightsMetaInfo::SymmetryEnum CheckSym(boost::uuids::uuid w_uuid,
												   ProgressDlg* p_dlg=0) = 0;
	virtual bool Lag(boost::uuids::uuid w_uuid, const GdaFlexValue& data,
					 GdaFlexValue& result) = 0;
	virtual bool GetCounts(boost::uuids::uuid w_uuid,
						   std::vector<long>& counts) = 0;
	virtual void GetNbrsExclCores(boost::uuids::uuid w_uuid,
								  const std::set<long>& cores,
								  std::set<long>& nbrs) = 0;
	virtual void Remove(boost::uuids::uuid w_uuid) = 0;
	virtual wxString RecNumToId(boost::uuids::uuid w_uuid, long rec_num) = 0;
	virtual GalWeight* GetGal(boost::uuids::uuid w_uuid) = 0;
    virtual GeoDaWeight* GetWeights(boost::uuids::uuid w_uuid) = 0;
	virtual boost::uuids::uuid GetDefault() const = 0;
	virtual void MakeDefault(boost::uuids::uuid w_uuid) = 0;
	virtual boost::uuids::uuid FindByTitle(const wxString& s) const = 0;
	virtual wxString SuggestTitleFromFileName(const wxString& fname) const = 0;
	virtual wxString GetTitle(boost::uuids::uuid w_uuid) const = 0;
	virtual void SetTitle(boost::uuids::uuid w_uuid, const wxString& s) = 0;
	virtual bool IsValid(boost::uuids::uuid w_uuid) = 0;	
};

#endif
