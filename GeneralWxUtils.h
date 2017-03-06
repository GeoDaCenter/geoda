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

class GeneralWxUtils	{
public:
	static wxOperatingSystemId GetOsId();
	static wxString LogOsId();
	static bool isMac();
	static bool isMac106();
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
};

#endif
