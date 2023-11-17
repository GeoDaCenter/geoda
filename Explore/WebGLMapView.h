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

#include "../HighlightStateObserver.h"
#include "../Project.h"
#include "../TemplateFrame.h"

class WebGLMapFrame : public TemplateFrame, public HighlightStateObserver {
 public:
  explicit WebGLMapFrame(wxFrame* parent, Project* project, OGRLayer* layer, const wxString& title = _("WebGL Map"),
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         const int style = wxDEFAULT_FRAME_STYLE);
  virtual ~WebGLMapFrame();

  virtual void update(HLStateInt* o);

 private:
  wxWebView* m_browser;
  wxString custom_scheme;
  HLStateInt* highlight_state;

  // constant string for in-memory file name
  static const wxString memory_arrow_file_name;

  // constant string for arrow file name
  static const wxString arrow_file_name;

  // constant string for javascript bundle.js
  static const wxString bundle_js_file_name;

  // constant string for index.html
  static const wxString index_html_file_name;

  // constant string for default custom schema
  static const wxString default_custom_schema;

  void OnIdle(wxIdleEvent& WXUNUSED(evt));
  void OnHandleWebMessage(const wxWebViewEvent& event);
  void CreateMemoryFiles(OGRLayer* layer);

  DECLARE_CLASS(WebGLMapFrame)
  DECLARE_EVENT_TABLE()
};
#endif
