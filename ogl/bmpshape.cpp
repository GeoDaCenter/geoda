/////////////////////////////////////////////////////////////////////////////
// Name:        bmpshape.cpp
// Purpose:     Bitmap shape class
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: bmpshape.cpp,v 1.1 2007/03/28 15:15:56 frm Exp $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if wxUSE_PROLOGIO
#include "wx/deprecated/wxexpr.h"
#endif

#include "ogl.h"


/*
 * Bitmap object
 *
 */

IMPLEMENT_DYNAMIC_CLASS(wxBitmapShape, wxRectangleShape)

wxBitmapShape::wxBitmapShape():wxRectangleShape(100.0, 50.0)
{
    m_filename = wxEmptyString;
    scale_factor = 1.0;
}

wxBitmapShape::~wxBitmapShape()
{
}

void wxBitmapShape::OnDraw(wxDC& dc)
{
    if (!m_bitmap.Ok())
        return;
    
    int x, y;
    x = WXROUND(m_xpos - m_width/2.0);
    y = WXROUND(m_ypos - m_height/2.0);
    
    dc.DrawBitmap(m_imgmap, x, y);
}

void wxBitmapShape::SetSize(double w, double h, bool WXUNUSED(recursive))
{
    // save aspect ratio
    if (w/h > m_aspectratio) {
        w = m_aspectratio * h;
    } else {
        h = w / m_aspectratio;
    }
    
    if (m_bitmap.Ok())
    {
        m_imgmap.Destroy();
        m_imgmap = m_bitmap.ConvertToImage();
        if (w != m_width || h != m_height) {
            m_imgmap.Rescale(w, h);
        }
    }
    
    SetAttachmentSize(w, h);
    
    m_width = w;
    m_height = h;
    SetDefaultRegionSize();
}

#if wxUSE_PROLOGIO
void wxBitmapShape::WriteAttributes(wxExpr *clause)
{
    // Can't really save the bitmap; so instantiate the bitmap
    // at a higher level in the application, from a symbol library.
    wxRectangleShape::WriteAttributes(clause);
    clause->AddAttributeValueString(_T("filename"), m_filename);
}

void wxBitmapShape::ReadAttributes(wxExpr *clause)
{
    wxRectangleShape::ReadAttributes(clause);
    clause->GetAttributeValue(_T("filename"), m_filename);
}
#endif

// Does the copying for this object
void wxBitmapShape::Copy(wxShape& copy)
{
    wxRectangleShape::Copy(copy);
    
    wxASSERT( copy.IsKindOf(CLASSINFO(wxBitmapShape)) ) ;
    
    wxBitmapShape& bitmapCopy = (wxBitmapShape&) copy;
    
    bitmapCopy.m_bitmap = m_bitmap;
    bitmapCopy.SetFilename(m_filename);
}

void wxBitmapShape::SetBitmap(const wxBitmap& bm)
{
    m_bitmap = bm;
    if (m_bitmap.Ok()) {
        double bm_w = m_bitmap.GetWidth();
        double bm_h = m_bitmap.GetHeight();
        m_aspectratio = bm_w / bm_h;
        SetSize(bm_w/scale_factor, bm_h/scale_factor);
    }
}
