/////////////////////////////////////////////////////////////////////////////
// Name:        ogl.h
// Purpose:     OGL main include
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id: ogl.h,v 1.1 2007/03/28 15:15:47 frm Exp $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_OGL_H_
#define _OGL_OGL_H_


#define WXDLLIMPEXP_OGL



#include "basic.h"      // Basic shapes
#include "basicp.h"
#include "lines.h"      // Lines and splines
#include "linesp.h"
#include "divided.h"    // Vertically-divided rectangle
#include "composit.h"   // Composite images
#include "canvas.h"     // wxShapeCanvas for displaying objects
#include "ogldiag.h"    // wxDiagram

#include "bmpshape.h"
#include "constrnt.h"
#include "drawn.h"
#include "drawnp.h"
#include "mfutils.h"
#include "misc.h"

// TODO: replace with wxModule implementation
extern WXDLLIMPEXP_OGL void wxOGLInitialize();
extern WXDLLIMPEXP_OGL void wxOGLCleanUp();

#endif
 // _OGL_OGL_H_
