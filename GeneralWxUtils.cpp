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

#include "GeneralWxUtils.h"
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/platinfo.h>
#include <wx/log.h>
#include <wx/window.h>

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
	int cnt = menu->GetMenuItemCount();
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
	int menuCnt = menu->GetMenuItemCount();
	int subMenuCnt = subMenu->GetMenuItemCount();
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



