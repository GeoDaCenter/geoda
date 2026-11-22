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
class wxGauge;
class wxStaticText;
class wxTextCtrl;

/**
 * Installs the engine that provides the advanced regression models (regimes,
 * spatial Durbin and SLX, GMM and instrumental variables, probit) and reports
 * how the download is getting on.
 *
 * It is opened from the Regression dialog's "Install Spreg" button: the engine
 * only means anything in that context, so it is not a menu entry of its own.
 * There is one action - install - and nothing else to get wrong; removing the
 * engine, or installing one from a file, are administrator jobs that the
 * README describes.
 *
 * See Regression/SpregEngine.h for the mechanics (download, checksum, atomic
 * unpack, running the solver) and spreg_engine/README.md for the whole story.
 */
class SpregEngineDlg: public wxDialog
{
public:
	SpregEngineDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
				   const wxString& title = _("Install Spreg"),
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxSize(560, 400));

	/** True when an engine is installed, asked and answered. */
	bool EngineInstalled() const { return installed_; }

	/** Is there a usable engine right now?  Cheap: it reads a directory. */
	static bool EngineAvailable();

private:
	void CreateControls();
	void UpdateState();
	void SetBusy(bool busy);
	void Log(const wxString& text, bool append = true);

	void OnInstall( wxCommandEvent& event );
	void OnClose( wxCommandEvent& event );

	wxStaticText* status_text_;
	wxGauge* gauge_;
	wxTextCtrl* log_text_;
	wxButton* install_button_;
	wxButton* close_button_;

	bool installed_;
	bool busy_;
};

#endif
