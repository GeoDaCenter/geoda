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

#ifndef __GEODA_CENTER_WEBGL_MAP_VIEW_H__
#define __GEODA_CENTER_WEBGL_MAP_VIEW_H__

#include <ogrsf_frmts.h>
#include <wx/frame.h>
#include <wx/webview.h>
#include <wx/wx.h>

#include <vector>

#include "../TemplateFrame.h"

class WebGLMapFrame : public TemplateFrame {
 public:
  explicit WebGLMapFrame(wxFrame* parent, Project* project, const std::vector<OGRFeature*>& features,
                         const wxString& title = _("WebGL Map"), const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize, const int style = wxDEFAULT_FRAME_STYLE);
  virtual ~WebGLMapFrame();

 private:
  wxWebView* m_browser;

  void OnIdle(wxIdleEvent& WXUNUSED(evt));
  void CreateMemoryFiles(const std::vector<OGRFeature*>& features);

  DECLARE_CLASS(WebGLMapFrame)
  DECLARE_EVENT_TABLE()
};
#endif