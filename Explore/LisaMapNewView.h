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

#ifndef __GEODA_CENTER_LISA_MAP_NEW_VIEW_H__
#define __GEODA_CENTER_LISA_MAP_NEW_VIEW_H__

#include "MapNewView.h"
#include "AbstractClusterMap.h"
#include "../GdaConst.h"

class LisaMapFrame;
class LisaMapCanvas;
class LisaCoordinator;

class LisaMapCanvas : public AbstractMapCanvas
{
	DECLARE_CLASS(LisaMapCanvas)
public:	
    LisaMapCanvas(wxWindow *parent, TemplateFrame* t_frame,
                  Project* project,
                  LisaCoordinator* lisa_coordinator,
                  CatClassification::CatClassifType theme_type,
                  bool isClust,
                  bool isBivariate,
                  bool isEBRate,
                  wxString menu_xrcid,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);
	virtual ~LisaMapCanvas();
    
    virtual void SetLabelsAndColorForClusters(int& num_cats, int t,
                                              CatClassifData& cat_data);

	virtual wxString GetCanvasTitle();
    virtual wxString GetVariableNames();
    bool is_diff;
    
protected:
	LisaCoordinator* lisa_coord;
	bool is_bi; // true = Bivariate, false = Univariate
	bool is_rate; // true = Moran Empirical Bayes Rate Smoothing
	
    wxString str_highhigh;
    wxString str_highlow;
    wxString str_lowlow;
    wxString str_lowhigh;
    
	DECLARE_EVENT_TABLE()
};

class LisaMapFrame : public AbstractMapFrame
{
	DECLARE_CLASS(LisaMapFrame)
public:
    LisaMapFrame(wxFrame *parent, Project* project,
                 LisaCoordinator* lisa_coordinator,
                 bool isClusterMap,
                 bool isBivariate,
                 bool isEBRate,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = GdaConst::map_default_size,
                 const long style = wxDEFAULT_FRAME_STYLE);
    virtual ~LisaMapFrame();
	
    virtual CatClassification::CatClassifType GetThemeType();
    virtual TemplateCanvas* CreateMapCanvas(wxPanel* rpanel);
    virtual void OnSaveResult(wxCommandEvent& event);
    virtual void OnShowAsConditionalMap(wxCommandEvent& event);

protected:
	LisaCoordinator* lisa_coord;
    CatClassification::CatClassifType theme_type;
    bool is_clust;
    bool is_bivariate;
    bool is_ebrate;
    
    DECLARE_EVENT_TABLE()
};

#endif
