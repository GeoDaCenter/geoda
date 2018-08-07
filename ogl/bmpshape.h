/////////////////////////////////////////////////////////////////////////////
// Name:        bmpshape.h
// Purpose:     wxBitmapShape
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: bmpshape.h,v 1.1 2007/03/28 15:15:47 frm Exp $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_BITMAP_H_
#define _OGL_BITMAP_H_


class WXDLLIMPEXP_OGL wxBitmapShape: public wxRectangleShape
{
    DECLARE_DYNAMIC_CLASS(wxBitmapShape)
public:
    wxBitmapShape();
    ~wxBitmapShape();
    
    void OnDraw(wxDC& dc);
    
#if wxUSE_PROLOGIO
    // I/O
    void WriteAttributes(wxExpr *clause);
    void ReadAttributes(wxExpr *clause);
#endif
    
    // Does the copying for this object
    void Copy(wxShape& copy);
    
    void SetScale(double s) { scale_factor = s;}
    
    void SetSize(double w, double h, bool recursive = true);
    inline wxBitmap& GetBitmap() const { return (wxBitmap&) m_bitmap; }
    void SetBitmap(const wxBitmap& bm);
    inline void SetFilename(const wxString& f) { m_filename = f; };
    inline wxString GetFilename() const { return m_filename; }
    
private:
    double        m_aspectratio;
    wxImage       m_imgmap;
    wxImage       m_imglegend;
    wxBitmap      m_bitmap;
    wxString      m_filename;
    double        scale_factor;
};

#endif
// _OGL_BITMAP_H_
