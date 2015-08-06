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

#ifndef __GEODA_CENTER_CALC_HELP_H__
#define __GEODA_CENTER_CALC_HELP_H__

#include <vector>
#include <map>
#include <wx/string.h>

// Need a way to indicate optional arguments
struct CalcHelpEntry {
	struct StrPair {
		StrPair(wxString k_, wxString v_) : k(k_), v(v_) {}
		wxString k;
		wxString v;
	};
	struct ArgPair {
		ArgPair(wxString arg_, bool opt=false) : arg(arg_), optional(opt) {}
		wxString arg;
		bool optional;
	};
	
	CalcHelpEntry() : infix(false) {}
	wxString func;
	wxString alt_func;
	wxString desc;
	bool infix;
	std::vector<ArgPair> syn_args;
	std::vector<StrPair> args_desc;
	std::vector<StrPair> exs;
	
	friend bool operator<(const CalcHelpEntry& lh, const CalcHelpEntry& rh);
};

/** Everything in CalcHelp should be static. */
class CalcHelp {
public:
	// This should be called only once in GdaApp::OnInit()
	static void init();
	static bool HasEntry(const wxString& f);
	static CalcHelpEntry GetEntry(const wxString& f);
	
	static std::map<wxString, CalcHelpEntry> dict;
};

#endif
