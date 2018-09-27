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

#ifndef __GEODA_CENTER_VIZ3_SCATTERPLOT_H__
#define __GEODA_CENTER_VIZ3_SCATTERPLOT_H__

class MyGLCanvas;
class myOGLManager;

// The main frame class
class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);
    
    void OnQuit(wxCommandEvent& event);
#if wxUSE_LOGWINDOW
    void OnLogWindow(wxCommandEvent& event);
#endif // wxUSE_LOGWINDOW
    void SetOGLString(const wxString& ogls)
    { m_OGLString = ogls; }
    bool OGLAvailable();
    
private:
#if wxUSE_LOGWINDOW
    wxLogWindow* m_LogWin;
#endif // wxUSE_LOGWINDOW
    wxString     m_OGLString;
    MyGLCanvas*  m_mycanvas;
    
    wxDECLARE_EVENT_TABLE();
};


// The canvas window
class MyGLCanvas : public wxGLCanvas
{
public:
    MyGLCanvas(MyFrame* parent, const wxGLAttributes& canvasAttrs);
    ~MyGLCanvas();
    
    //Used just to know if we must end now because OGL 3.2 isn't available
    bool OglCtxAvailable()
    {return m_oglContext != NULL;}
    
    //Init the OpenGL stuff
    bool oglInit();
    
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouse(wxMouseEvent& event);
    
private:
    // Members
    MyFrame*      m_parent;
    wxGLContext*  m_oglContext;
    myOGLManager* m_oglManager;
    int           m_winHeight; // We use this var to know if we have been sized
    
    wxDECLARE_EVENT_TABLE();
};


// IDs for the controls and the menu commands
enum
{
    Pyramid_Quit = wxID_EXIT,
    Pyramid_LogW = wxID_HIGHEST + 10
};
#endif

