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
#include <wx/filename.h>
#include <wx/gauge.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>

#include "../Regression/SpregEngine.h"

using namespace SpregEngine;

namespace {

wxString FormatSize(long long bytes)
{
	if (bytes < 0) return _("unknown");
	if (bytes < 1024) {
		wxString text;
		text << bytes << " B";
		return text;
	}
	if (bytes < 1024 * 1024) return wxString::Format("%.1f kB", bytes / 1024.0);
	if (bytes < 1024LL * 1024 * 1024) return wxString::Format("%.1f MB", bytes / 1048576.0);
	return wxString::Format("%.2f GB", bytes / 1073741824.0);
}

/**
 * Moves the dialog's gauge while the installer works.
 *
 * wxYieldIfNeeded() is what makes that visible: the download and the unpack
 * run on this thread, so without it nothing would be repainted until they are
 * over.  It is safe here because the dialog disables its buttons first, so
 * there is nothing to click while it yields.
 */
class GaugeSink : public ProgressSink {
public:
	GaugeSink(wxGauge* gauge, wxStaticText* status)
		: gauge_(gauge), status_(status), cancelled_(false) {}

	virtual bool OnProgress(long long done, long long total, const wxString& phase)
	{
		if (status_) status_->SetLabel(phase);
		if (gauge_) {
			if (total > 0) {
				int value = static_cast<int>((done * 1000) / total);
				if (value < 0) value = 0;
				if (value > 1000) value = 1000;
				if (value != gauge_->GetValue()) gauge_->SetValue(value);
			} else {
				gauge_->Pulse();
			}
		}
		wxYieldIfNeeded();
		return !cancelled_;
	}

	virtual bool Cancelled() { return cancelled_; }

private:
	wxGauge* gauge_;
	wxStaticText* status_;
	bool cancelled_;
};

} // namespace

SpregEngineDlg::SpregEngineDlg(wxWindow* parent, wxWindowID id,
							   const wxString& title, const wxPoint& pos,
							   const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  status_text_(NULL), gauge_(NULL), log_text_(NULL), install_button_(NULL),
	  close_button_(NULL), installed_(false), busy_(false)
{
	CreateControls();
	UpdateState();
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
		_("The advanced regression models - regimes, spatial Durbin, GMM and "
		  "instrumental variables, probit - are estimated by spreg, a Python "
		  "package. GeoDa downloads an engine for it once, keeps it in your own "
		  "GeoDa folder and uses it from then on.\n\n"
		  "Nothing is installed system wide, and GeoDa's own regression models "
		  "are not affected either way."));
	intro->Wrap(520);
	top->Add(intro, 0, wxALL, 12);

	status_text_ = new wxStaticText(this, wxID_ANY, "");
	top->Add(status_text_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

	gauge_ = new wxGauge(this, wxID_ANY, 1000);
	top->Add(gauge_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

	log_text_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
							   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
	top->Add(log_text_, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);

	wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
	install_button_ = new wxButton(this, wxID_ANY, _("&Install Engine"));
	close_button_ = new wxButton(this, wxID_CANCEL, _("Close"));
	buttons->Add(install_button_, 0, wxRIGHT, 8);
	buttons->AddStretchSpacer();
	buttons->Add(close_button_, 0);
	top->Add(buttons, 0, wxEXPAND | wxALL, 12);

	SetSizer(top);
	install_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnInstall, this);
	close_button_->Bind(wxEVT_BUTTON, &SpregEngineDlg::OnClose, this);
	Centre();
}

void SpregEngineDlg::UpdateState()
{
	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) {
		// the application was not packaged completely
		status_text_->SetLabel(_("This installation of GeoDa cannot download the engine."));
		Log(err);
		install_button_->Enable(false);
		return;
	}

	const Status status = Discover(manifest);
	installed_ = status.installed;
	if (installed_) {
		status_text_->SetLabel(wxString::Format(_("spreg %s is installed:\n%s"),
												status.version, status.dir));
		install_button_->SetLabel(_("&Reinstall Engine"));
	} else {
		const Artifact* artifact = manifest.ForThisPlatform();
		if (artifact) {
			status_text_->SetLabel(wxString::Format(
				_("Downloading the engine takes %s and unpacks to %s."),
				FormatSize(artifact->size_bytes), FormatSize(artifact->unpacked_bytes)));
		} else {
			status_text_->SetLabel(wxString::Format(
				_("There is no engine published for %s yet."), PlatformKey()));
			install_button_->Enable(false);
		}
		if (!status.problem.IsEmpty()) Log(status.problem);
	}
	Layout();
}

void SpregEngineDlg::SetBusy(bool busy)
{
	busy_ = busy;
	install_button_->Enable(!busy);
	close_button_->Enable(!busy);
	gauge_->SetValue(0);
}

void SpregEngineDlg::Log(const wxString& text, bool append)
{
	if (!append) log_text_->SetValue("");
	log_text_->AppendText(text);
	if (!text.EndsWith("\n")) log_text_->AppendText("\n");
}

void SpregEngineDlg::OnInstall(wxCommandEvent& WXUNUSED(event))
{
	if (busy_) return;

	Manifest manifest;
	wxString err;
	if (!manifest.Read(ShippedManifestPath(), err)) {
		wxMessageBox(err, _("Install Spreg"), wxOK | wxICON_ERROR, this);
		return;
	}
	const Artifact* artifact = manifest.ForThisPlatform();
	if (!artifact) {
		wxMessageBox(wxString::Format(
			_("There is no engine published for %s yet."), PlatformKey()),
			_("Install Spreg"), wxOK | wxICON_INFORMATION, this);
		return;
	}

	SetBusy(true);
	Log(wxString::Format(_("Downloading %s and unpacking it into\n%s"),
						 FormatSize(artifact->size_bytes), EnginesRoot()), false);

	GaugeSink sink(gauge_, status_text_);
	wxString installed_dir;
	const bool ok = InstallFromUrl(manifest, *artifact, &sink, err, installed_dir);

	if (!ok) {
		SetBusy(false);
		status_text_->SetLabel(_("The engine could not be installed."));
		Log(err);
		wxLogMessage("Spreg: the engine could not be installed: %s", err);
		wxMessageBox(err, _("Install Spreg"), wxOK | wxICON_ERROR, this);
		UpdateState();
		return;
	}

	// tell the user it works, not that it was downloaded
	wxString answer;
	if (ProbeEngine(installed_dir, answer, err)) {
		Log(wxString::Format(_("\nInstalled into\n%s\n\nThe engine reports:\n%s"),
							 installed_dir, answer));
		status_text_->SetLabel(_("spreg is installed and ready."));
		installed_ = true;
	} else {
		Log(wxString::Format(_("\nThe engine was installed but does not run:\n%s"), err));
		wxLogMessage("Spreg: the engine was installed but does not run: %s", err);
		wxMessageBox(err, _("Install Spreg"), wxOK | wxICON_ERROR, this);
		status_text_->SetLabel(_("The engine was installed but does not run."));
	}
	SetBusy(false);
	UpdateState();
	Layout();
}

void SpregEngineDlg::OnClose(wxCommandEvent& WXUNUSED(event))
{
	if (busy_) return;
	EndModal(installed_ ? wxID_OK : wxID_CANCEL);
}
