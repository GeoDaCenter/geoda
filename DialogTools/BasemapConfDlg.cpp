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

#include "BasemapConfDlg.h"

#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include <string>

#include "../GdaConst.h"
#include "../Project.h"
#include "../ShapeOperations/OGRDataAdapter.h"

BEGIN_EVENT_TABLE(BasemapConfDlg, wxDialog)
EVT_BUTTON(wxID_OK, BasemapConfDlg::OnOkClick)
EVT_BUTTON(XRCID("ID_NOKIA_RESET"), BasemapConfDlg::OnResetClick)
END_EVENT_TABLE()

BasemapConfDlg::BasemapConfDlg(wxWindow* parent, Project* _p, wxWindowID id, const wxString& title, const wxPoint& pos,
                               const wxSize& size) {
  wxLogMessage("Open BasemapConfDlg.");
  p = _p;

  basemap_resources = wxString::FromUTF8(GdaConst::gda_basemap_sources.mb_str());
  std::vector<wxString> items = OGRDataAdapter::GetInstance().GetHistory("gda_basemap_sources");
  if (items.size() > 0) {
    basemap_resources = items[0];
  }

  wxString encoded_str = wxString::FromUTF8((const char*)basemap_resources.mb_str());
  if (encoded_str.IsEmpty() == false) {
    basemap_resources = encoded_str;
  }

  wxXmlResource::Get()->LoadDialog(this, GetParent(), "IDD_BASEMAP_CONF_DLG");
  FindWindow(XRCID("wxID_OK"))->Enable(true);
  m_txt_stadia_key = XRCCTRL(*this, "IDC_STADIA_KEY", wxTextCtrl);
  m_txt_nokia_uname = XRCCTRL(*this, "IDC_NOKIA_USERNAME", wxTextCtrl);
  m_txt_nokia_key = XRCCTRL(*this, "IDC_NOKIA_KEY", wxTextCtrl);
  m_txt_basemap = XRCCTRL(*this, "IDC_BASEMAP_SOURCE", wxTextCtrl);
  m_txt_basemap->SetValue(basemap_resources);

  SetParent(parent);
  SetPosition(pos);
  Centre();
}

void BasemapConfDlg::OnOkClick(wxCommandEvent& event) {
  wxLogMessage("BasemapConfDlg: Click OK Button.");

  wxString stadie_key(m_txt_stadia_key->GetValue().Trim());
  if (!stadie_key.empty()) {
    OGRDataAdapter::GetInstance().AddEntry("stadia_key", stadie_key);
  }

  wxString nokia_uname(m_txt_nokia_uname->GetValue().Trim());
  wxString nokia_key(m_txt_nokia_key->GetValue().Trim());
  if (!nokia_uname.empty() && !nokia_key.empty()) {
    OGRDataAdapter::GetInstance().AddEntry("nokia_user", nokia_uname);
    OGRDataAdapter::GetInstance().AddEntry("nokia_key", nokia_key);
  }

  if (m_txt_basemap->GetValue() != basemap_resources) {
    OGRDataAdapter::GetInstance().AddEntry("gda_basemap_sources", m_txt_basemap->GetValue());
  }
  EndDialog(wxID_OK);
}

void BasemapConfDlg::OnResetClick(wxCommandEvent& event) {
  wxLogMessage("BasemapConfDlg: Click Reset Button.");

  m_txt_basemap->SetValue(GdaConst::gda_basemap_sources);
  OGRDataAdapter::GetInstance().AddEntry("gda_basemap_sources", GdaConst::gda_basemap_sources);

  EndDialog(wxID_OK);
}
