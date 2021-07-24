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

#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/platinfo.h>
#include <wx/log.h>
#include <wx/window.h>
#include <wx/xrc/xmlres.h>
#include <wx/wfstream.h>
#include <wx/colordlg.h>
#include <wx/txtstrm.h>

#include "FramesManagerObserver.h"
#include "FramesManager.h"
#include "DialogTools/AddIdVariable.h"
#include "GeneralWxUtils.h"
#include "Project.h"

////////////////////////////////////////////////////////////////////////
//
// SimpleReportTextCtrl
//
////////////////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE(SimpleReportTextCtrl, wxTextCtrl)
EVT_CONTEXT_MENU(SimpleReportTextCtrl::OnContextMenu)
END_EVENT_TABLE()

void SimpleReportTextCtrl::OnContextMenu(wxContextMenuEvent& event)
{
    wxMenu* menu = new wxMenu;
    // Some standard items
    menu->Append(XRCID("SAVE_SIMPLE_REPORT"), _("Save"));
    menu->AppendSeparator();
    menu->Append(wxID_UNDO, _("Undo"));
    menu->Append(wxID_REDO, _("Redo"));
    menu->AppendSeparator();
    menu->Append(wxID_CUT, _("Cut"));
    menu->Append(wxID_COPY, _("Copy"));
    menu->Append(wxID_PASTE, _("Paste"));
    menu->Append(wxID_CLEAR, _("Delete"));
    menu->AppendSeparator();
    menu->Append(wxID_SELECTALL, _("Select All"));
    
    // Add any custom items here
    Connect(XRCID("SAVE_SIMPLE_REPORT"), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(SimpleReportTextCtrl::OnSaveClick));
    
    PopupMenu(menu);
}

void SimpleReportTextCtrl::OnSaveClick( wxCommandEvent& event )
{
    wxLogMessage("In SimpleReportTextCtrl::OnSaveClick()");
    wxFileDialog dlg( this, "Save Results", wxEmptyString,
                     wxEmptyString,
                     "TXT files (*.txt)|*.txt",
                     wxFD_SAVE );
    if (dlg.ShowModal() != wxID_OK) return;
    
    wxFileName new_txt_fname(dlg.GetPath());
    wxString new_txt = new_txt_fname.GetFullPath();
    wxFFileOutputStream output(new_txt);
    if (output.IsOk()) {
        wxTextOutputStream txt_out( output );
        txt_out << this->GetValue();
        txt_out.Flush();
        output.Close();
    }
}

////////////////////////////////////////////////////////////////////////
//
// ScrolledDetailMsgDialog
//
////////////////////////////////////////////////////////////////////////

ScrolledDetailMsgDialog::ScrolledDetailMsgDialog(const wxString & title, const wxString & msg, const wxString & details, const wxSize &size, const wxArtID & art_id)
: wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    
    wxPanel *panel = new wxPanel(this, -1);
    
    wxBoxSizer *vbox0 = new wxBoxSizer(wxVERTICAL);
    wxStaticText *st = new wxStaticText(panel, wxID_ANY, msg, wxDefaultPosition, wxDefaultSize, wxTE_WORDWRAP);
    tc = new SimpleReportTextCtrl(panel, XRCID("ID_TEXTCTRL_1"), details, wxDefaultPosition, wxSize(-1, 300));
    vbox0->Add(st, 0, wxBOTTOM, 10);
    vbox0->Add(tc, 1, wxEXPAND);
    
    wxBoxSizer *hbox0 = new wxBoxSizer(wxHORIZONTAL);
    wxBitmap save = wxArtProvider::GetBitmap(wxART_WARNING);
    wxStaticBitmap *warn = new wxStaticBitmap(panel, wxID_ANY, save);
    hbox0->Add(warn, 0, wxRIGHT, 5);
    hbox0->Add(vbox0, 1);
    
    panel->SetSizer(hbox0);
    
    wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    wxButton *saveButton = new wxButton(this, XRCID("SAVE_DETAILS"), _("Save Details"), wxDefaultPosition, wxSize(130, -1));
    wxButton *okButton = new wxButton(this, wxID_OK, _("Ok"), wxDefaultPosition, wxSize(110, -1));
    hbox1->Add(saveButton, 1, wxRIGHT, 30);
    hbox1->Add(okButton, 1);
    
    Connect(XRCID("SAVE_DETAILS"), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ScrolledDetailMsgDialog::OnSaveClick));
    
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(panel, 1, wxEXPAND | wxALL, 20);
    vbox->Add(hbox1, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
    
    SetSizer(vbox);
    
    Centre();
    ShowModal();
    
    Destroy(); 
}

void ScrolledDetailMsgDialog::OnSaveClick( wxCommandEvent& event )
{
    wxLogMessage("In ScrolledDetailMsgDialog::OnSaveClick()");
    wxFileDialog dlg( this, "Save results", wxEmptyString,
                     wxEmptyString,
                     "TXT files (*.txt)|*.txt",
                     wxFD_SAVE );
    if (dlg.ShowModal() != wxID_OK) return;
    
    wxFileName new_txt_fname(dlg.GetPath());
    wxString new_txt = new_txt_fname.GetFullPath();
    wxFFileOutputStream output(new_txt);
    if (output.IsOk()) {
        wxTextOutputStream txt_out( output );
        txt_out << tc->GetValue();
        txt_out.Flush();
        output.Close();
    }
}

////////////////////////////////////////////////////////////////////////
//
// Other functions
//
////////////////////////////////////////////////////////////////////////
wxOperatingSystemId GeneralWxUtils::GetOsId()
{
	static wxOperatingSystemId osId =
		wxPlatformInfo::Get().GetOperatingSystemId();
	return osId;
}

wxString GeneralWxUtils::LogOsId()
{
    wxString oslog;
  
    int os = 0;
    
    if (isMac()) {
        os = 1;
    } else if (isWindows()) {
        os = 2;
    } else if (isUnix()) {
        os = 3;
    }
    
	int majorVsn = 0;
	int minorVsn = 0;
	wxGetOsVersion(&majorVsn, &minorVsn);
    
    oslog = wxString::Format("\nos: %d-%d-%d", os, majorVsn, minorVsn);
    return oslog;
}

bool GeneralWxUtils::isMac()
{
	static bool r = (GetOsId() & wxOS_MAC ? true : false);
	return r;
}

bool GeneralWxUtils::isMac106()
{
	static bool r = (GetOsId() & wxOS_MAC ? true : false);

	int majorVsn = 0;
	int minorVsn = 0;
	wxGetOsVersion(&majorVsn, &minorVsn);

	r = r & (minorVsn == 6);

	return r;
}

bool GeneralWxUtils::isMac1014plus()
{
    static bool r = (GetOsId() & wxOS_MAC ? true : false);

    int majorVsn = 0;
    int minorVsn = 0;
    wxGetOsVersion(&majorVsn, &minorVsn);

    r = r & (majorVsn > 10 || minorVsn >= 14);

    return r;
}

bool GeneralWxUtils::isWindows()
{
	static bool r = (GetOsId() & wxOS_WINDOWS ? true : false);
	return r;
}

bool GeneralWxUtils::isUnix()
{
	static bool r = (GetOsId() & wxOS_UNIX ? true : false);
	return r;
}

bool GeneralWxUtils::isXP()
{
	static bool r = (GetOsId() & wxOS_WINDOWS ? true : false);
	if (r) {
		int majorVsn = 0;
		int minorVsn = 0;
		wxGetOsVersion(&majorVsn, &minorVsn);
		r = r && (majorVsn == 5);
		// This includes Windows 2000 (5.0),
		//    XP (5.1) and Windows Server 2003 (5.3)
	}
	return r;
}

bool GeneralWxUtils::isVista()
{
	static bool r = (GetOsId() & wxOS_WINDOWS ? true : false);
	if (r) {
		int majorVsn = 0;
		int minorVsn = 0;
		wxGetOsVersion(&majorVsn, &minorVsn);
		r = r && (majorVsn == 6) && (minorVsn == 0);
		// This includes Windows Server 2008 and Vista
	}
	return r;
}

bool GeneralWxUtils::isX86()
{
#if defined(_WIN64) || defined(__amd64__)
	return false;
#else
	return true;
#endif
}

bool GeneralWxUtils::isX64()
{
    return !isX86();
}

bool GeneralWxUtils::isDebug()
{
#if defined(__WXDEBUG__) || defined(DEBUG)
	return true;
#else
	return false;
#endif
}

bool GeneralWxUtils::isBigEndian()
{
	static bool r =
		(wxPlatformInfo::Get().GetEndianness() & wxENDIAN_BIG);
	return r;
}

bool GeneralWxUtils::isLittleEndian()
{
	static bool r =
		(wxPlatformInfo::Get().GetEndianness() & wxENDIAN_LITTLE);
	return r;
}

/*
 * ReplaceMenu: finds MenuBar menu with given title and replaces with newMenu
 *   additionally, the previous menu is deleted.  Returns true on success.
 */
bool GeneralWxUtils::ReplaceMenu(wxMenuBar* mb, const wxString& title,
								 wxMenu* newMenu)
{
	//int menu_count = mb->GetMenuCount();
	//LOG(menu_count);
	//for (int i=0; i<menu_count; i++) {
	//	LOG_MSG(mb->GetMenuLabelText(i));
	//}
	
	int m_ind = mb->FindMenu(title);
	if (m_ind == wxNOT_FOUND) {
		delete newMenu;
		return false;
	}
	//wxMenu* prev_opt_menu = mb->GetMenu(m_ind);
	//mb->Replace(m_ind, newMenu, title);
    wxMenu* prev_opt_menu = mb->Remove(m_ind);
    mb->Insert(m_ind, newMenu, title);
	
	// The following line shouldn't be needed, but on wxWidgets 2.9.2, the
	// menu label is set to empty after Replace is called.
	//mb->SetMenuLabel(m_ind, title);
	if (prev_opt_menu) delete prev_opt_menu;
	return true;
}

/*
 * EnableMenuAll: Given a menubar pointer and top-level menu title,
 *   this method traverses every menu item in the menu tree (menu items
 *   include actual clickable menu items and submenus) and either enables
 *   or disables the item according to the value of bool enable.  This
 *   is useful in situations where one wants to disable everything in
 *   a menu except for one deeply-nested menu item (see EnableMenuItem).
 *   Boolean value 'true' is returned iff the top-level menu is found.
 *   The enable/disable state of the actual top-level menu is not changed.
 */
bool GeneralWxUtils::EnableMenuAll(wxMenuBar* mb, const wxString& title,
								   bool enable)
{
	if (!mb ) return false;
	int mPos = mb->FindMenu(title);
	if (mPos == wxNOT_FOUND) return false;
	wxMenu* menu = mb->GetMenu(mPos);
	GeneralWxUtils::EnableMenuRecursive(menu, enable);
	return true;
}

/*
 * This is intended to be a helper function to EnableMenuAll with
 * the menubar argument, but can be called directly on an valid wxMenu.
 */
void GeneralWxUtils::EnableMenuRecursive(wxMenu* menu, bool enable)
{
	if (!menu) return;
	int cnt = (int)menu->GetMenuItemCount();
	for (int i=0; i<cnt; i++) {
		wxMenuItem* mItem = menu->FindItemByPosition(i);
		//wxString msg("menu item '");
		//msg << mItem->GetItemLabelText() << "' enable = " << enable;
		//LOG_MSG(msg);
		mItem->Enable(enable);
		//wxString msg2("menu item '");
		//msg2 << mItem->GetItemLabelText() << "' IsEnabled() = ";
		//msg2 << mItem->IsEnabled();
		//LOG_MSG(msg2);
		GeneralWxUtils::EnableMenuRecursive(mItem->GetSubMenu(), enable);
	}
}

/*
 * EnableMenuItem:  This method assumes there are no submenus within
 *                  submenus.
 */
bool GeneralWxUtils::EnableMenuItem(wxMenuBar* mb, const wxString& menuTitle,
									int id, bool enable)
{
	if (!mb) return false;
	wxMenu* menu = NULL;
	wxMenuItem* mItem =	mb->FindItem(id);
	menu=mb->GetMenu(mb->FindMenu(menuTitle));
	if (!mItem || !menu) return false;
	//LOG_MSG("item title:") + mItem->GetText();
	mItem->Enable(enable);
	// Check if embedded in a submenu and enable as needed.
	wxMenu* subMenu = NULL;
	menu->FindItem(id, &subMenu);
	if (menu == subMenu) return true;
	int menuCnt = (int)menu->GetMenuItemCount();
	int subMenuCnt = (int)subMenu->GetMenuItemCount();
	int subMenuIndex = wxNOT_FOUND;
	int i = 0;
	while (subMenuIndex == wxNOT_FOUND && i < menuCnt) {
		mItem = menu->FindItemByPosition(i);
		if (mItem->IsSubMenu() && mItem->GetSubMenu() == subMenu)
			subMenuIndex = i;
		i++;
	}
	if (enable) {
		menu->FindItemByPosition(subMenuIndex)->Enable(true);
		return true;
	}
	bool anyEnabled = false;
	// Check if there are any other items enabled in the submenu.
	for (i = 0; i<subMenuCnt; i++) {
		mItem = subMenu->FindItemByPosition(i);
		if (!mItem->IsSeparator())
			anyEnabled = anyEnabled || mItem->IsEnabled();
	}
	if (!anyEnabled)
		menu->FindItemByPosition(subMenuIndex)->Enable(false);
	return true;
}

bool GeneralWxUtils::EnableMenuItem(wxMenuBar* m, int id, bool enable)
{
	if (!m) return false;
	wxMenuItem* mi = m->FindItem(id);
	if (mi) {
		mi->Enable(enable);
		return true;
	}
	return false;
}

bool GeneralWxUtils::EnableMenuItem(wxMenu* m, int id, bool enable)
{
	if (!m) return false;
	wxMenuItem* mi = m->FindItem(id);
	if (mi) {
		mi->Enable(enable);
		return true;
	}
	return false;
}

bool GeneralWxUtils::CheckMenuItem(wxMenuBar* m, int id, bool check)
{
	if (!m) return false;
	wxMenuItem* mi = m->FindItem(id);
	if (mi && mi->IsCheckable()) {
		mi->Check(check);
		return true;
	}
	return false;
}

bool GeneralWxUtils::CheckMenuItem(wxMenu* menu, int id, bool check)
{
	if (!menu) return false;
	wxMenuItem* mItem =	menu->FindItem(id);
	if (!mItem) return false;
	if (!mItem->IsCheckable()) return false;
	mItem->Check(check);
	return true;
}

bool GeneralWxUtils::SetMenuItemText(wxMenu* menu, int id,
									 const wxString& text)
{
	if (!menu) return false;
	wxMenuItem* mItem =	menu->FindItem(id);
	if (!mItem) return false;
	mItem->SetItemLabel(text);
	return true;
}


wxMenu* GeneralWxUtils::FindMenu(wxMenuBar* mb,	const wxString& menuTitle)
{
	if (!mb) return 0;
	int menu = mb->FindMenu(menuTitle);
	if (menu == wxNOT_FOUND) return 0;
	return mb->GetMenu(menu);
}

wxColour GeneralWxUtils::PickColor(wxWindow *parent, wxColour& col)
{
    wxColourData data;
    data.SetColour(col);
    data.SetChooseFull(true);
    int ki;
    for (ki = 0; ki < 16; ki++) {
        wxColour colour(ki * 16, ki * 16, ki * 16);
        data.SetCustomColour(ki, colour);
    }
    
    wxColourDialog dialog(parent, &data);
    dialog.SetTitle(_("Choose Cateogry Color"));
    if (dialog.ShowModal() == wxID_OK) {
        wxColourData retData = dialog.GetColourData();
        return retData.GetColour();
    }
    return col;
}

void GeneralWxUtils::SaveWindowAsImage(wxWindow *win, wxString title)
{
    //Create a DC for the whole screen area
    wxWindowDC dcScreen(win);
    
    //Get the size of the screen/DC
    wxCoord screenWidth, screenHeight;
    dcScreen.GetSize(&screenWidth, &screenHeight);
    
    //Create a Bitmap that will later on hold the screenshot image
    //Note that the Bitmap must have a size big enough to hold the screenshot
    //-1 means using the current default colour depth
    wxSize new_sz = win->FromDIP(wxSize(screenWidth, screenHeight));
    wxBitmap screenshot(new_sz);
    
    //Create a memory DC that will be used for actually taking the screenshot
    wxMemoryDC memDC;
    //Tell the memory DC to use our Bitmap
    //all drawing action on the memory DC will go to the Bitmap now
    memDC.SelectObject(screenshot);
    //Blit (in this case copy) the actual screen on the memory DC
    //and thus the Bitmap

    memDC.Blit( 0, //Copy to this X coordinate
               0, //Copy to this Y coordinate
               screenWidth, //Copy this width
               screenHeight, //Copy this height
               &dcScreen, //From where do we copy?
               0, //What's the X offset in the original DC?
               0  //What's the Y offset in the original DC?
               );
    //Select the Bitmap out of the memory DC by selecting a new
    //uninitialized Bitmap
    memDC.SelectObject(wxNullBitmap);
    
    //Our Bitmap now has the screenshot, so let's save it :-)
    wxString default_fname(title);
    wxString filter = "BMP|*.bmp|PNG|*.png";
    wxFileDialog dialog(NULL, _("Save Image to File"), wxEmptyString,
                        default_fname, filter,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK) return;
    wxFileName fname = wxFileName(dialog.GetPath());
    wxString str_fname = fname.GetPathWithSep() + fname.GetName();
    
    if (dialog.GetFilterIndex() == 0) {
        screenshot.SaveFile(str_fname + ".bmp", wxBITMAP_TYPE_BMP);
    } else if (dialog.GetFilterIndex() == 1) {
        screenshot.SaveFile(str_fname + ".png", wxBITMAP_TYPE_PNG);
    }
}

///////////////////////////////////////////////////////////////////////////////
// TransparentSettingDialog
///////////////////////////////////////////////////////////////////////////////
TransparentSettingDialog::TransparentSettingDialog(
	wxWindow * parent,
	double _transparency,
	wxWindowID id,
	const wxString & caption,
	const wxPoint & position,
	const wxSize & size,
	long style )                          
: wxDialog( parent, id, caption, position, size, style),
transparency(_transparency)
{
    wxLogMessage("Open TransparentSettingDialog");   
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);
    
    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    
    // A text control for the user’s name
    int trasp_scale = 100 * transparency;

	wxBoxSizer* subSizer = new wxBoxSizer(wxHORIZONTAL);
    slider = new wxSlider(this, wxID_ANY, trasp_scale, 0, 100,
                          wxDefaultPosition, wxSize(200, -1),
                          wxSL_HORIZONTAL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"1.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);
    subSizer->Add(slider, 0, wxALIGN_CENTER_VERTICAL|wxALL);
	subSizer->Add(new wxStaticText(this, wxID_ANY,"0.0"), 0,
                  wxALIGN_CENTER_VERTICAL|wxALL);

	boxSizer->Add(subSizer);
    wxString txt_transparency = wxString::Format(_("Current Opacity: %.2f"), 1.0 - transparency);
    
    slider_text = new wxStaticText(
		this,
		wxID_ANY,
		txt_transparency,
		wxDefaultPosition,
		wxSize(100, -1));
    boxSizer->Add(slider_text, 0, wxGROW|wxALL, 5);
    boxSizer->Add(new wxButton(this, wxID_OK, _("OK")), 0, wxALIGN_CENTER|wxALL, 10);
    
    topSizer->Fit(this);
    
    slider->Bind(wxEVT_SLIDER, &TransparentSettingDialog::OnSliderChange, this);
}

TransparentSettingDialog::~TransparentSettingDialog()
{
}

void TransparentSettingDialog::OnSliderChange( wxCommandEvent & event )
{
    int val = event.GetInt();
    double trasp = 1.0 - val / 100.0;
    slider_text->SetLabel(wxString::Format(_("Current Transparency: %.2f"), trasp));
	transparency = trasp;
}

double TransparentSettingDialog::GetTransparency()
{
	return transparency;
}

///////////////////////////////////////////////////////////////////////////////
// CheckSpatialRefDialog
///////////////////////////////////////////////////////////////////////////////

CheckSpatialRefDialog::CheckSpatialRefDialog(wxWindow * parent,
                                             const wxString& msg,
                                             wxWindowID id,
                                             const wxString & caption,
                                             const wxPoint & position,
                                             const wxSize & size,
                                             long style)
: wxDialog(parent, id, caption, position, size, style)
{
    wxLogMessage("Open TransparentSettingDialog");
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer* boxSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(boxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 20);

    wxBitmap bmp = wxArtProvider::GetBitmap(wxART_WARNING);
    wxStaticBitmap *warning_icon = new wxStaticBitmap(this, wxID_ANY, bmp);
    wxStaticText* textctrl = new wxStaticText(this, wxID_ANY, msg);

    wxBoxSizer *msgbox = new wxBoxSizer(wxHORIZONTAL);
    msgbox->Add(warning_icon, 0, wxRIGHT, 10);
    msgbox->Add(textctrl, 1, wxEXPAND);

    cb = new wxCheckBox(this, wxID_ANY, _("Don't ask again"));

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton *yes_btn = new wxButton(this, wxID_OK, _("Yes"));
    wxButton *no_btn = new wxButton(this, wxID_CANCEL, _("No"));
    hbox->Add(yes_btn, 1, wxALL, 5);
    hbox->Add(no_btn, 1, wxALL, 5);
    
    boxSizer->Add(msgbox, 1, wxEXPAND | wxALL, 10);
    boxSizer->Add(cb, 0, wxALIGN_CENTER | wxTOP, 20);
    boxSizer->Add(hbox, 0, wxALIGN_CENTER | wxALL, 10);
    
    topSizer->Fit(this);
    no_btn->SetDefault();
}

bool CheckSpatialRefDialog::IsCheckAgain()
{
    return !cb->GetValue();
}

////////////////////////////////////////////////////////////////////////
// SelectWeightsIdDialog
////////////////////////////////////////////////////////////////////////

SelectWeightsIdDialog::SelectWeightsIdDialog(wxWindow * parent, Project* project,
                                             wxWindowID id,
                                             const wxString & caption,
                                             const wxPoint & position,
                                             const wxSize & size,
                                             long style)
: wxDialog(parent, id, caption, position, size, style),
project(project)
{
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(topSizer);

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* textctrl = new wxStaticText(this, wxID_ANY, _("Select ID Variable"));
    m_id_field = new wxChoice (this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
    InitVariableChoice(); // fill content for id dropdown list
    wxButton* btn_add_id_var = new wxButton(this, wxID_ANY, _("Add ID Variable..."));

    hbox->Add(textctrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    hbox->Add(m_id_field, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);
    hbox->Add(btn_add_id_var, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer *btnbox = new wxBoxSizer(wxHORIZONTAL);
    wxButton *yes_btn = new wxButton(this, wxID_OK, _("OK"));
    wxButton *no_btn = new wxButton(this, wxID_CANCEL, _("Cancel"));
    btnbox->Add(yes_btn, 1, wxALL, 5);
    btnbox->Add(no_btn, 1, wxALL, 5);

    topSizer->Add(hbox, 0, wxEXPAND | wxALL, 20);
    topSizer->Add(btnbox, 0, wxALIGN_CENTER | wxALL, 10);
    topSizer->Fit(this);

    //events
    btn_add_id_var->Bind(wxEVT_BUTTON, &SelectWeightsIdDialog::OnAddIDVariable, this);
    m_id_field->Bind(wxEVT_CHOICE, &SelectWeightsIdDialog::OnIdVariableSelected, this);
}

wxString SelectWeightsIdDialog::GetIDVariable()
{
    return m_id_field->GetStringSelection();
}

void SelectWeightsIdDialog::OnAddIDVariable(wxCommandEvent& evt)
{
    TableInterface* table_int = project->GetTableInt();
    AddIdVariable dlg(table_int, this);
    if (dlg.ShowModal() == wxID_OK) {
        // We know that the new id has been added to the the table in memory
        InitVariableChoice();
        m_id_field->SetSelection(0);
        OnIdVariableSelected(evt);
    }
}

void SelectWeightsIdDialog::InitVariableChoice()
{
    TableInterface* table_int = project->GetTableInt();
    col_id_map.clear();
    table_int->FillColIdMap(col_id_map);

    m_id_field->Clear();
    for (int i=0, iend=(int)col_id_map.size(); i<iend; i++) {
        int col = col_id_map[i];
        if (table_int->GetColType(col) == GdaConst::long64_type ||
            table_int->GetColType(col) == GdaConst::string_type) {
            if (!table_int->IsColTimeVariant(col)) {
                m_id_field->Append(table_int->GetColName(col));
            }
        }
    }
    m_id_field->SetSelection(-1);
}

void SelectWeightsIdDialog::OnIdVariableSelected(wxCommandEvent& evt)
{
    if (m_id_field->GetSelection() < 0) return;

    wxString sel_var = m_id_field->GetStringSelection();
    TableInterface* table_int = project->GetTableInt();
    if (table_int->CheckID(sel_var) == false) {
        m_id_field->SetSelection(-1);
    }
}


BEGIN_EVENT_TABLE( SummaryDialog, wxFrame)
    EVT_CLOSE( SummaryDialog::OnClose )
    EVT_MOUSE_EVENTS(SummaryDialog::OnMouseEvent)
END_EVENT_TABLE()

SummaryDialog::~SummaryDialog( )
{
    frames_manager->removeObserver(this);
}

SummaryDialog::SummaryDialog( wxWindow* parent, Project* project,
                                           wxString showText,
                                           wxWindowID id,
                                           const wxString& caption,
                                           const wxPoint& pos,
                                           const wxSize& size, long style )
:wxFrame(parent, id, caption, pos, size, style)
{
    wxLogMessage("Open SummaryDialog.");
    SetParent(parent);
    frames_manager = project->GetFramesManager();
    
    results = showText;
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    CreateControls();
    Centre();
    m_textbox->AppendText(results);
    
    frames_manager->registerObserver(this);
}

void SummaryDialog::update(FramesManager* o)
{
}

void SummaryDialog::CreateControls()
{
    wxPanel *panel = new wxPanel(this, -1);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    m_textbox = new wxTextCtrl(panel, XRCID("ID_TEXTCTRL"), "", wxDefaultPosition, wxSize(620,560), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxTE_RICH2);
    
    if (GeneralWxUtils::isWindows()) {
        wxFont font(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
    } else {
        wxFont font(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_textbox->SetFont(font);
        
    }
    vbox->Add(m_textbox, 1, wxEXPAND|wxALL);
    panel->SetSizer(vbox);
    
    Center();
}

void SummaryDialog::AddNewReport(const wxString report)
{
    results = report + results;
    m_textbox->SetValue(results);
}

void SummaryDialog::SetReport(const wxString report)
{
    results = report;
    m_textbox->SetValue(results);
}

void SummaryDialog::OnClose( wxCloseEvent& event )
{
    wxLogMessage("In SummaryDialog::OnClose()");
    Destroy();
    event.Skip();
}
