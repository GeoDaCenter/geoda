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

#ifndef __GEODA_CENTER_GENERAL_WX_UTILS_H__
#define __GEODA_CENTER_GENERAL_WX_UTILS_H__

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/colour.h>

#include "FramesManagerObserver.h"
#include "DialogTools/VariableSettingsDlg.h"

class Project;

class GeneralWxUtils	{
public:
	static wxOperatingSystemId GetOsId();
	static wxString LogOsId();
	static bool isMac();
	static bool isMac106();
    static bool isMac1014plus();
	static bool isWindows();
	static bool isUnix();
	static bool isXP();
	static bool isVista();
	static bool isX86();
	static bool isX64();
	static bool isDebug();
	static bool isBigEndian();
	static bool isLittleEndian();
	static bool ReplaceMenu(wxMenuBar* mb, const wxString& title,
							wxMenu* newMenu); 
	static bool EnableMenuAll(wxMenuBar* mb, const wxString& title,
							  bool enable);
	static void EnableMenuRecursive(wxMenu* menu, bool enable);
	static bool EnableMenuItem(wxMenuBar* mb, const wxString& menuTitle,
							   int id, bool enable);
	static bool EnableMenuItem(wxMenuBar* m, int id, bool enable);
	static bool EnableMenuItem(wxMenu* m, int id, bool enable);
	static bool CheckMenuItem(wxMenuBar* m, int id, bool check);
	static bool CheckMenuItem(wxMenu* menu, int id, bool check);
	static bool SetMenuItemText(wxMenu* menu, int id, const wxString& text);
	static wxMenu* FindMenu(wxMenuBar* mb, const wxString& menuTitle);
    static wxColour PickColor(wxWindow* parent, wxColour& col);
    static void SaveWindowAsImage(wxWindow* win, wxString title);
    //static std::set<wxString> GetFieldNamesFromTable(TableInterface* table);
};

class SimpleReportTextCtrl : public wxTextCtrl
{
public:
    SimpleReportTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "",
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style =  wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_READONLY | wxTE_RICH | wxTE_RICH2, const wxValidator& validator = wxDefaultValidator,
                         const wxString& name = wxTextCtrlNameStr)
    : wxTextCtrl(parent, id, value, pos, size, style, validator, name)
    {
        if (GeneralWxUtils::isWindows()) {
            wxFont font(8,wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            SetFont(font);
        } else {
            wxFont font(12,wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            SetFont(font);
        }
    }
protected:
    void OnContextMenu(wxContextMenuEvent& event);
    void OnSaveClick( wxCommandEvent& event );
    DECLARE_EVENT_TABLE()
};

class SummaryDialog : public wxFrame, public FramesManagerObserver
{
    DECLARE_EVENT_TABLE()

public:
    SummaryDialog() {}
    SummaryDialog(wxWindow* parent, Project* project, wxString showText,
        wxWindowID id = wxID_ANY,
        const wxString& caption = _("Summary"),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(680, 480),
        long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX);
    virtual ~SummaryDialog();
    
    /** Implementation of FramesManagerObserver interface */
    virtual void update(FramesManager* o);
    
    void CreateControls();
    void OnClose(wxCloseEvent& event);
    void AddNewReport(const wxString report);
    void SetReport(const wxString report);
    
    SimpleReportTextCtrl* m_textbox;
    wxString results;
    
private:
    FramesManager* frames_manager;
};

class ScrolledDetailMsgDialog : public wxDialog
{
public:
    ScrolledDetailMsgDialog(const wxString & title, const wxString & msg, const wxString & details, const wxSize &size = wxSize(540, 280), const wxArtID & art_id =  wxART_WARNING);
   
    SimpleReportTextCtrl *tc;
    
    void OnSaveClick( wxCommandEvent& event );
};

class TransparentSettingDialog: public wxDialog
{
	double transparency;
    wxSlider* slider;
    wxStaticText* slider_text;
	void OnSliderChange(wxCommandEvent& event );
public:
    TransparentSettingDialog ();
    TransparentSettingDialog (wxWindow * parent,
		double trasp,
		wxWindowID id=wxID_ANY,
		const wxString & caption="Transparent Setting Dialog",
		const wxPoint & pos = wxDefaultPosition,
		const wxSize & size = wxDefaultSize,
		long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~TransparentSettingDialog ();

	double GetTransparency();
};

// Prompt user that spatial objects are not projected or with unknown projection
// The distance computation could be wrong. Ask user to continue proceeding or
// quit the application
class CheckSpatialRefDialog : public wxDialog
{
    wxCheckBox *cb;

public:
    CheckSpatialRefDialog(wxWindow * parent, const wxString& msg,
                          wxWindowID id=wxID_ANY,
                          const wxString & caption="Warning",
                          const wxPoint & pos = wxDefaultPosition,
                          const wxSize & size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~CheckSpatialRefDialog() {}

    bool IsCheckAgain();
};

// Prompt user to select an ID variable for weights creation
// Allow user to "Add ID variable" in table
class SelectWeightsIdDialog : public wxDialog
{
    Project* project;
    wxChoice * m_id_field;

    // col_id_map[i] is a map from the i'th numeric item in the
    // fields drop-down to the actual col_id_map.  Items
    // in the fields dropdown are in the order displayed
    // in wxGrid
    std::vector<int> col_id_map;
    
public:
    SelectWeightsIdDialog(wxWindow * parent, Project* project,
                          wxWindowID id=wxID_ANY,
                          const wxString & caption="Select ID Variable Dialog",
                          const wxPoint & pos = wxDefaultPosition,
                          const wxSize & size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE );
    virtual ~SelectWeightsIdDialog() {}

    wxString GetIDVariable();
    
protected:
    void InitVariableChoice();
    void OnIdVariableSelected(wxCommandEvent& evt);
    void OnAddIDVariable(wxCommandEvent& evt);
};
#endif
