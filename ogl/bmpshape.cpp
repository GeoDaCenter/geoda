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
#include "../TemplateCanvas.h"
#include "../TemplateLegend.h"
#include "../Explore/MapNewView.h"

/*
 * Bitmap object
 *
 */

IMPLEMENT_DYNAMIC_CLASS(wxBitmapShape, wxRectangleShape)

wxBitmapShape::wxBitmapShape():wxRectangleShape(100.0, 50.0)
{
    transparent_bg = true;
	scaled_basemap = true;
    m_filename = wxEmptyString;
    tcanvas = NULL;
    tlegend = NULL;
}

wxBitmapShape::~wxBitmapShape()
{
}

void wxBitmapShape::OnDraw(wxDC& dc)
{
    if (!m_imgmap.Ok())
        return;
    
    wxSize sz = dc.GetSize();
    
    
    int x, y;
    x = WXROUND(m_xpos - m_width/2.0);
    y = WXROUND(m_ypos - m_height/2.0);
    
	if (tcanvas) {
		dc.DrawBitmap(m_imgmap, x, y);
	} else if (tlegend) {
		if (transparent_bg) {
			m_imgmap.SetMaskColour(255, 255, 255);
		}
		dc.DrawBitmap(m_imgmap, x, y, transparent_bg);
	}
}

void wxBitmapShape::SetSize(double w, double h, bool WXUNUSED(recursive))
{
    if (tcanvas) {
        if (MapCanvas* canvas = dynamic_cast<MapCanvas*>(tcanvas)) {
            wxBitmap bm(w, h);
            wxMemoryDC dc;
            dc.SelectObject(bm);
            tcanvas->RenderToDC(dc, w, h);
            dc.SelectObject(wxNullBitmap);
            
            m_imgmap.Destroy();
            m_imgmap = bm.ConvertToImage();
        
        } else {
            int canvas_width, canvas_height;
            tcanvas->GetClientSize(&canvas_width, &canvas_height);
            double scale_factor = w / (double)canvas_width;
            wxBitmap bm;
            bm.CreateScaled(canvas_width, canvas_height, 32, scale_factor);
            wxMemoryDC dc;
            dc.SelectObject(bm);
            dc.SetBackground(*wxWHITE_BRUSH);
            dc.Clear();

            tcanvas->RenderToDC(dc, w, h);
            dc.SelectObject(wxNullBitmap);
            
            m_imgmap.Destroy();
            m_imgmap = bm.ConvertToImage();
        }
        
    } else if (tlegend) {
        int legend_width = tlegend->GetDrawingWidth();
        int legend_height = tlegend->GetDrawingHeight();
        double scale_factor = w / (double)legend_width;
        wxBitmap bm(w, h);
        wxMemoryDC dc;
        dc.SelectObject(bm);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        tlegend->RenderToDC(dc, scale_factor);
        dc.SelectObject(wxNullBitmap);
        
        m_imgmap.Destroy();
        m_imgmap = bm.ConvertToImage();
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
    clause->AddAttributeValueString("filename", m_filename);
}

void wxBitmapShape::ReadAttributes(wxExpr *clause)
{
    wxRectangleShape::ReadAttributes(clause);
    clause->GetAttributeValue("filename", m_filename);
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
        SetSize(bm_w, bm_h);
    }
}

void wxBitmapShape::SetCanvas(TemplateCanvas* _tcanvas)
{
    tcanvas = _tcanvas;
    if (tcanvas) {
        wxSize sz = tcanvas->GetClientSize();
        wxBitmap bm(sz.GetWidth(), sz.GetHeight());
        wxMemoryDC dc;
        dc.SelectObject(bm);
        tcanvas->RenderToDC(dc, m_width, m_height);
        dc.SelectObject(wxNullBitmap);
        
        m_imgmap.Destroy();
        m_imgmap = bm.ConvertToImage();
        
        SetSize(sz.GetWidth(), sz.GetHeight());
    }
}

void wxBitmapShape::SetCanvas(TemplateLegend* _tlegend)
{
    tlegend = _tlegend;
    if (tlegend) {
        int legend_width = tlegend->GetDrawingWidth();
        int legend_height = tlegend->GetDrawingHeight();
        wxBitmap bm(legend_width, legend_height);
        wxMemoryDC dc;
        dc.SelectObject(bm);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        tlegend->RenderToDC(dc, 1);
        dc.SelectObject(wxNullBitmap);
        
        m_imgmap.Destroy();
        m_imgmap = bm.ConvertToImage();
        SetSize(legend_width, legend_height);
    }
}

void wxBitmapShape::SetTransparentBG(bool flag)
{
	transparent_bg = flag;
}

void wxBitmapShape::SetScaledBasemap(bool flag)
{
	scaled_basemap = flag;
	if (MapCanvas* canvas = dynamic_cast<MapCanvas*>(tcanvas)) {
		int w = m_width;
		int h = m_height;
		wxBitmap bm(w, h);
		wxMemoryDC dc;
		dc.SelectObject(bm);
		canvas->print_detailed_basemap = !flag;
		canvas->RenderToDC(dc, w, h);
		dc.SelectObject(wxNullBitmap);
            
		m_imgmap.Destroy();
		m_imgmap = bm.ConvertToImage();
		
	}
}
