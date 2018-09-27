//
//  AbstractCanvas.hpp
//  GeoDa
//
//  Created by Xun Li on 8/17/18.
//

#ifndef AbstractCanvas_hpp
#define AbstractCanvas_hpp

#include <stdio.h>
#include <wx/wx.h>

/**
 * Abstract class for all drawing classes: maps and plots
 *
 * Implemented a basic double buffered drawing canvas
 *
 * Inherited classes can take care of the drawing function: DoDraw(dc),
 * to drawing to a buffer, then to the screen, during EVT_PAINT event.
 *
 *  Inherited classes can also use
 *  wxBufferedDC to direct drawing to a buffer,
 *  then the screen outside EVT_PAINT event.
 *
 */
/*
class AbstractCanvas : public wxWindow
{
    DECLARE_CLASS(AbstractCanvas)
    
    wxColour m_bgColor;
    int m_bufferWidth;
    int m_bufferHeight;
    
    wxBitmap* p_buffer;
    wxBitmap* p_drawingBackupBuffer;
    
    bool m_reInitBuffer;
    
public:
    AbstractCanvas(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint &pos=wxDefaultPosition,
                   const wxSize &size=wxDefaultSize,
                   long style=0);
    
    ~AbstractCanvas ();
    
    void InitBuffer();
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnIdle(wxIdleEvent& event);
    virtual void OnSize(wxSizeEvent& event);
    
    DECLARE_EVENT_TABLE()
};
*/
#endif /* AbstractCanvas_hpp */
