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

#include "SpregEngineDlg.h"

#include <wx/wx.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "../Regression/SpregEngine.h"

using namespace SpregEngine;

namespace {

wxString FormatSize(long long bytes)
{
	if (bytes < 0) return _( "unknown" );
	if (bytes < 1024) {
		wxString text;
		text << bytes << " B";
		return text;
	}
	if (bytes < 1024 * 1024) return wxString::Format("%.1f kB", bytes / 1024.0);
	if (bytes < 1024LL * 1024 * 1024) return wxString::Format("%.1f MB", bytes / 1048576.0);
	return wxString::Format("%.2f GB", bytes / 1073741824.0);
}

/** The engine GeoDa would use right now, empty when there is none. */
wxString CurrentEngineDir()
{
	SpregEngine::Manifest manifest;
	wxString err;
	if (!manifest.Read(SpregEngine::ShippedManifestPath(), err)) return wxEmptyString;
	return SpregEngine::Discover(manifest).dir;
}

/**
 * Drives a wxProgressDialog from the installer's progress callback.  The
 * progress dialog is what keeps the application responsive while a 150 MB
 * download and an unpack are running, and its cancel button is what stops them.
 */
class ProgressDlgSink : public ProgressSink {
public:
	explicit ProgressDlgSink(wxProgressDialog* dialog)
		: dialog_(dialog), cancelled_(false) {}

	virtual bool OnProgress(long long done, long long total, const wxString& phase)
	{
		if (!dialog_) return true;                 // no UI: never cancel
		bool alive;
		if (total > 0) {
			const int value = static_cast<int>((done * 1000) / total);
			alive = dialog_->Update(value < 0 ? 0 : (value > 1000 ? 1000 : value), phase);
		} else {
			alive = dialog_->Pulse(phase);
		}
		if (!alive) cancelled_ = true;
		return !cancelled_;
	}

	virtual bool Cancelled() { return cancelled_; }

private:
	wxProgressDialog* dialog_;
	bool cancelled_;
};

} // namespace

SpregEngineDlg::SpregEngineDlg(wxWindow* parent, wxWindowID id,
							   const wxString& title, const wxPoint& pos,
							   const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	CreateControls();
	RefreshStatus();
}

bool SpregEngineDlg::EngineAvailable()
{
	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) return false;
	return Discover(manifest).installed;
}

void SpregEngineDlg::CreateControls()
{
	wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

	wxStaticText* intro = new wxStaticText(this, wxID_ANY,
		_("GeoDa's own engine covers classical regression, spatial lag and spatial "
		  "error.  The advanced models - regimes, spatial Durbin and SLX, GMM and "
		  "instrumental variables, probit, specification search - run in a separate "
		  "engine built on the Python package spreg.\n\n"
		  "It is downloaded on demand and kept in your own GeoDa folder; nothing is "
		  "installed system wide and nothing changes in GeoDa's own engine."));
	intro->Wrap(600);
	top->Add(intro, 0, wxALL, 12);

	wxStaticLine* line = new wxStaticLine(this);
	top->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

	status_text_ = new wxStaticText(this, wxID_ANY, "");
	top->Add(status_text_, 0, wxALL, 12);

	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
	install_button_ = new wxButton(this, wxID_ANY, _("&Install engine"));
	install_file_button_ = new wxButton(this, wxID_ANY, _("Install from &file..."));
	test_button_ = new wxButton(this, wxID_ANY, _("&Test engine"));
	remove_button_ = new wxButton(this, wxID_ANY, _("&Remove engine"));
	close_button_ = new wxButton(this, wxID_CANCEL, _("Close"));
	buttons->Add(install_button_, 0, wxRIGHT, 6);
	buttons->Add(install_file_button_, 0, wxRIGHT, 6);
	buttons->Add(test_button_, 0, wxRIGHT, 6);
	buttons->Add(remove_button_, 0, wxRIGHT, 6);
	buttons->AddStretchSpacer();
	buttons->Add(close_button_, 0);
	top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

	log_text_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
							   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
	top->Add(log_text_, 1, wxEXPAND | wxALL, 12);

	SetSizer(top);

	install_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnInstall, this);
	install_file_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnInstallFromFile, this);
	test_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnTest, this);
	remove_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnRemove, this);
	close_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnClose, this);

	Centre();
}

void SpregEngineDlg::RefreshStatus()
{
	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) {
		status_text_->SetLabel(wxString::Format(
			_("This installation of GeoDa has no engine manifest:\n%s\n\n%s"),
			ShippedManifestPath(), err));
		EnableActions(false);
		return;
	}

	const Status status = Discover(manifest);
	wxString text;
	if (status.installed) {
		text = wxString::Format(
			_("Engine installed: spreg %s\n%s\n%s on disk"),
			status.version, status.dir, FormatSize(EngineSize(status.dir)));
	} else {
		const Artifact* artifact = manifest.ForThisPlatform();
		text = _("No engine installed.");
		if (artifact) {
			text += wxString::Format(
				_("\n\nInstalling it downloads %s for %s and unpacks to %s."),
				FormatSize(artifact->size_bytes), PlatformKey(), FormatSize(artifact->unpacked_bytes));
		} else {
			text += wxString::Format(
				_("\n\nThere is no engine archive for %s (%s) yet."),
				PlatformKey(), manifest.spreg);
		}
		if (!status.problem.IsEmpty()) text += "\n\n" + status.problem;
	}
	status_text_->SetLabel(text);
	status_text_->Wrap(600);

	const bool installed = status.installed;
	const bool can_install = manifest.ForThisPlatform() != NULL;
	install_button_->Enable(can_install);
	install_file_button_->Enable(true);
	test_button_->Enable(installed);
	remove_button_->Enable(installed);
	Layout();
}

void SpregEngineDlg::EnableActions(bool enable)
{
	install_button_->Enable(enable);
	install_file_button_->Enable(enable);
	test_button_->Enable(enable);
	remove_button_->Enable(enable);
	close_button_->Enable(enable);
}

void SpregEngineDlg::AppendLine(const wxString& line)
{
	log_text_->AppendText(line + "\n");
}

void SpregEngineDlg::ShowText(const wxString& text, bool append)
{
	if (!append) log_text_->SetValue("");
	log_text_->AppendText(text);
	if (!text.EndsWith("\n")) log_text_->AppendText("\n");
}

void SpregEngineDlg::OnInstall(wxCommandEvent& WXUNUSED(event))
{
	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) {
		wxMessageBox(err, _("Engine manifest"), wxOK | wxICON_ERROR, this);
		return;
	}
	const Artifact* artifact = manifest.ForThisPlatform();
	if (!artifact) {
		wxMessageBox(wxString::Format(
			_("There is no engine archive for %s yet.  You can still install one "
			  "from a file, if you have it."), PlatformKey()),
			_("No engine for this platform"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	if (wxMessageBox(wxString::Format(
			_("Download and install the regression engine?\n\n"
			  "spreg %s for %s\n%s to download, %s on disk, in\n%s\n\n"
			  "The download is verified against a checksum that GeoDa ships."),
			manifest.spreg, PlatformKey(), FormatSize(artifact->size_bytes),
			FormatSize(artifact->unpacked_bytes), EnginesRoot()),
		_("Install engine"), wxYES_NO | wxICON_QUESTION, this) != wxYES) {
		return;
	}

	EnableActions(false);
	{
		wxProgressDialog progress(_("Installing the regression engine"),
								  _("Preparing..."), 1000, this,
								  wxPD_APP_MODAL | wxPD_CAN_ABORT
								  | wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE);
		ProgressDlgSink sink(&progress);

		wxString installed_dir;
		const bool ok = InstallFromUrl(manifest, *artifact, &sink, err, installed_dir);
		if (!ok) {
			EnableActions(true);
			ShowText(wxString::Format(_("Installation failed.\n\n%s"), err));
			RefreshStatus();
			wxMessageBox(err, _("Installation failed"), wxOK | wxICON_ERROR, this);
			return;
		}
		ShowText(wxString::Format(_("Installed spreg %s into\n%s"), manifest.spreg,
								  installed_dir));
	}

	// prove it works before telling the user it does
	wxString probe;
	if (ProbeEngine(CurrentEngineDir(), probe, err)) {
		AppendLine(_("\nThe engine answers:"));
		AppendLine(probe);
	} else {
		AppendLine(wxString::Format(_("\nThe engine was installed but does not run:\n%s"), err));
	}
	EnableActions(true);
	RefreshStatus();
}

void SpregEngineDlg::OnInstallFromFile(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog dialog(this, _("Select an engine archive"),
						wxEmptyString, wxEmptyString,
						_("Engine archives (*.zip)|*.zip|All files (*.*)|*.*"),
						wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dialog.ShowModal() != wxID_OK) return;

	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) {
		wxMessageBox(err, _("Engine manifest"), wxOK | wxICON_ERROR, this);
		return;
	}
	const Artifact* artifact = manifest.ForThisPlatform();

	wxString warning;
	if (!artifact) {
		warning = wxString::Format(
			_("The manifest has no engine for %s, so the archive cannot be checked "
			  "against a known checksum.  Install it anyway?"), PlatformKey());
	} else {
		warning = wxString::Format(
			_("Install this archive as the engine for %s?\n\nThe checksum in GeoDa's "
			  "manifest will be checked first, and the installation is refused "
			  "unless it matches."), PlatformKey());
	}
	if (wxMessageBox(warning, _("Install from file"), wxYES_NO | wxICON_QUESTION, this)
		!= wxYES) {
		return;
	}

	// an archive the manifest knows about gets verified; anything else is
	// installed only after the warning above, with an empty checksum
	Artifact fallback;
	if (!artifact) {
		fallback.key = PlatformKey();
		fallback.built = false;      // no size or checksum to check
		artifact = &fallback;
	}

	EnableActions(false);
	wxString installed_dir;
	const bool ok = InstallFromZip(manifest, *artifact, dialog.GetPath(), NULL, err,
									installed_dir);
	if (!ok) {
		EnableActions(true);
		ShowText(wxString::Format(_("Installation failed.\n\n%s"), err));
		RefreshStatus();
		wxMessageBox(err, _("Installation failed"), wxOK | wxICON_ERROR, this);
		return;
	}
	ShowText(wxString::Format(_("Installed into\n%s"), installed_dir));
	EnableActions(true);
	RefreshStatus();
}

void SpregEngineDlg::OnTest(wxCommandEvent& WXUNUSED(event))
{
	const wxString dir = CurrentEngineDir();
	EnableActions(false);

	wxString output, err;
	if (ProbeEngine(dir, output, err)) {
		ShowText(_("The engine works.  It reports:\n\n") + output);
	} else {
		ShowText(_("The engine does not work:\n\n") + err);
	}
	EnableActions(true);
}

void SpregEngineDlg::OnRemove(wxCommandEvent& WXUNUSED(event))
{
	const wxString dir = CurrentEngineDir();
	if (wxMessageBox(wxString::Format(_("Remove the regression engine?\n\n%s\n\n"
										"GeoDa's own regression models are not affected."),
									  dir),
					 _("Remove engine"), wxYES_NO | wxICON_QUESTION, this) != wxYES) {
		return;
	}
	wxString err;
	if (Remove(dir, err)) {
		ShowText(wxString::Format(_("Removed %s"), dir));
	} else {
		ShowText(wxString::Format(_("Could not remove the engine.\n\n%s"), err));
	}
	RefreshStatus();
}

void SpregEngineDlg::OnClose(wxCommandEvent& WXUNUSED(event))
{
	EndModal(wxID_OK);
}
