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

#ifndef __GEODA_CENTER_SPREG_ENGINE_DLG_H__
#define __GEODA_CENTER_SPREG_ENGINE_DLG_H__

#include <wx/dialog.h>
#include <wx/string.h>

class wxButton;
class wxStaticText;
class wxTextCtrl;

/**
 * Installs, tests and removes the Python regression engine that provides the
 * advanced models (regimes, spatial Durbin/SLX, GMM and IV, probit, ...).
 *
 * GeoDa ships the solver and the engine manifest, and downloads the engine
 * itself only when someone asks for it, so that the installer stays as it is
 * and no Python is bundled.  See Regression/SpregEngine.h for the mechanics and
 * spreg_engine/README.md for the whole story.
 */
class SpregEngineDlg: public wxDialog
{
public:
	SpregEngineDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
				   const wxString& title = _("Advanced Regression Engine"),
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxSize(640, 470));

	/** Runs a job through the engine; used by the regression dialog later. */
	static bool EngineAvailable();

private:
	void CreateControls();
	void RefreshStatus();
	void ShowText(const wxString& text, bool append = false);
	void AppendLine(const wxString& line);
	void EnableActions(bool enable);

	void OnInstall( wxCommandEvent& event );
	void OnInstallFromFile( wxCommandEvent& event );
	void OnTest( wxCommandEvent& event );
	void OnRemove( wxCommandEvent& event );
	void OnClose( wxCommandEvent& event );

	wxStaticText* status_text_;
	wxTextCtrl* log_text_;
	wxButton* install_button_;
	wxButton* install_file_button_;
	wxButton* test_button_;
	wxButton* remove_button_;
	wxButton* close_button_;
};

#endif
