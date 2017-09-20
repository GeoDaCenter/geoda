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
#ifndef __GEODA_CENTER_GEOCODING_DLG_H__
#define __GEODA_CENTER_GEOCODING_DLG_H__


#include <vector>
#include <wx/combobox.h>
#include <wx/gauge.h>

#include "../FramesManager.h"
#include "../FramesManagerObserver.h"
#include "../Project.h"

class FramesManager;
class TableInterface;
class Project;

using namespace std;

class GeocodingDlg : public wxDialog, public FramesManagerObserver, public TableStateObserver
{
public:
    GeocodingDlg(wxWindow* parent, Project* p,
                 const wxString& title = _("Geocoding Setting Dialog"),
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize );
    
    ~GeocodingDlg();
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    virtual void update(TableState* o);
    /** This method is only here temporarily until all observer classes
     support dynamic time changes such as swap, rename and add/remove. */
    virtual bool AllowTimelineChanges() { return true; }
    /** Does this observer allow data modifications to named group. */
    virtual bool AllowGroupModify(const wxString& grp_nm) { return true; }
    /** Does this observer allow Table/Geometry row additions and deletions. */
    virtual bool AllowObservationAddDelete(){ return true; }
  
    void Init();
    
    void CreateControls();
    void run();
    
    void OnClose(wxCloseEvent& ev);
    void OnOkClick( wxCommandEvent& event );
    void OnStopClick( wxCommandEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    
protected:
    wxFrame *parent;
    Project* project;
    TableInterface* table_int;
    FramesManager* frames_manager;
    TableState* table_state;
    
    boost::thread* t;
    bool stop;
   
    wxChoice* m_choice_vars;
    wxTextCtrl* m_lat;
    wxTextCtrl* m_lng;
    wxTextCtrl* m_google_input;
   
    wxButton *okButton;
    wxButton *stopButton;
    wxButton *closeButton;
    
    wxGauge* m_prg;
    
	DECLARE_EVENT_TABLE()
};

#endif
