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

#ifndef __GEODA_CENTER_SCHCCLUSTER_DLG_H___
#define __GEODA_CENTER_SCHCCLUSTER_DLG_H___

#include <vector>
#include <map>

#include "../FramesManager.h"
#include "../VarTools.h"
#include "../logger.h"
#include "AbstractClusterDlg.h"
#include "HClusterDlg.h"

struct GdaNode;
class Project;
class TableInterface;

class SCHCDlg : public HClusterDlg
{
public:
    SCHCDlg(wxFrame *parent, Project* project);
    virtual ~SCHCDlg();
    
protected:
    virtual bool Run(vector<wxInt64>& clusters);

    DECLARE_EVENT_TABLE()
};

#endif
