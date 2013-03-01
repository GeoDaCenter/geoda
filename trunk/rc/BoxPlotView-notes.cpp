// Trying to understand and fix update, selection and brushing code for BoxPlotView:

// BoxPlotView methods of interest:
BEGIN_EVENT_TABLE(BoxPlotCanvas, wxScrolledWindow)
  EVT_SIZE(BoxPlotCanvas::OnSize)
  EVT_MOUSE_EVENTS(BoxPlotCanvas::OnEvent)
  EVT_MOUSE_CAPTURE_LOST(BoxPlotCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

// Template Canvas methods of interest:
// None of the events below should get called for BoxPlotCanvas
// since the event table sends events to wxScrolledWindow, not
// TemplateCanvas
BEGIN_EVENT_TABLE(TemplateCanvas, wxScrolledWindow)
  EVT_SIZE(TemplateCanvas::OnSize)
  EVT_PAINT(TemplateCanvas::OnPaint)
  EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
  EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
  EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

// wxMouseEvent notes:
// LeftDown(): true when the left button is first pressed down
// LeftIsDown(): true while the left button is down. During a mouse dragging
//  operation, this will continue to return true, while LeftDown() is false.
// RightDown/RightIsDown: similar to Left.
// Moving(): returns true when mouse is moved, but no buttons are pressed.
// Dragging(): returns true when mouse is moved and at least one mouse button is
//   pressed.
// CmdDown(): Checks if MetaDown under Mac and ControlDown on other platforms.
// ButtonDClick(int but = wxMOUSE_BTN_ANY): checks for double click of any
//   button. Can also specify wxMOUSE_BTN_LEFT / RIGHT / MIDDLE.  Or
//   LeftDCLick(), etc.
// LeftUp(): returns true at the moment the button changed to up.

/*
OnDraw calls Draw and happens when Refresh is sent or a Paint Event is sent.
Right now on Linux, a single resize event results in two OnDraw calls.
This is wasteful.
 */

/*
Note: wxGridEx::Refresh() is being called even though table is hidden and
whenever a single selection event happens in another view.
 */

/*
Let's add a state variable to TemplateCanvas called selection_outline_visible
We will then have a methods:

Assumption is that gSelect1 and gSelect2 reflect the desired new selection.

NOTE: Must review purpose of 	canvas_overlay.Reset(); // OVERLAY CODE
in TemplateCanvas.cpp

DrawSelectionOutline()
if (selection_outline_visible) {
  EraseSelectionOutline()
}
selection_outline_visible = true;

EraseSelectionOutline()
  selection_outline_visible = false;
  we are using an overlay, so just do this:
  overlaydc.Clear();

Whenever OnDraw() is called, immediately call EraseSelectionOutline();


 */
