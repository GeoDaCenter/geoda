/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/colourdata.h>
#include <wx/colordlg.h>
#include <wx/filename.h>
#include <wx/textdlg.h>
//#include <wx/valnum.h> // causes compile problem on OSX with wxWidgets 2.9.2
#include <wx/valtext.h>
#include <wx/xrc/xmlres.h>
#include "../FramesManager.h"
#include "../GeoDaConst.h"
#include "../GeneralWxUtils.h"
#include "../Project.h"
#include "../DataViewer/DbfGridTableBase.h"
#include "../DataViewer/TableState.h"
#include "../Explore/CatClassifState.h"
#include "../Generic/HighlightState.h"
#include "../ShapeOperations/ShapeUtils.h"
#include "../logger.h"
#include "CatClassifDlg.h"


BEGIN_EVENT_TABLE(CatClassifHistCanvas, TemplateCanvas)
	EVT_PAINT(TemplateCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(TemplateCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(TemplateCanvas::OnMouseEvent)
	EVT_MOUSE_CAPTURE_LOST(TemplateCanvas::OnMouseCaptureLostEvent)
END_EVENT_TABLE()

const int CatClassifHistCanvas::max_intervals = 10;
const int CatClassifHistCanvas::default_intervals = 6;
const double CatClassifHistCanvas::left_pad_const = 0;
const double CatClassifHistCanvas::right_pad_const = 0;
const double CatClassifHistCanvas::interval_width_const = 10;
const double CatClassifHistCanvas::interval_gap_const = 0;

CatClassifHistCanvas::CatClassifHistCanvas(wxWindow *parent,
								   TemplateFrame* t_frame,
								   Project* project_s,
								   const wxPoint& pos, const wxSize& size)
: TemplateCanvas(parent, pos, size, false, true),
project(project_s), num_obs(project_s->GetNumRecords()),
highlight_state(project_s->highlight_state),
y_axis(0), data(0), default_data(project_s->GetNumRecords()),
breaks(0), default_breaks(default_intervals-1),
colors(0), default_colors(default_intervals)
{
	using namespace Shapefile;
	LOG_MSG("Entering CatClassifHistCanvas::CatClassifHistCanvas");
	template_frame = t_frame;

	cur_intervals = default_intervals;
	
	InitRandNormData(default_data);
	// equal spacing between -3 sd and +3 sd
	for (int i=0; i<default_intervals-1; i++) {
		default_breaks[i] = ((double) i+1)/((double) default_intervals);
		default_breaks[i] *= 6.0;
		default_breaks[i] -= 3.0;
	}
	CatClassification::PickColorSet(default_colors,
									CatClassification::diverging_color_scheme,
									default_intervals, false);
	data = &default_data;
	breaks = &default_breaks;
	colors = &default_colors;
	
	obs_id_to_ival.resize(num_obs);
	
	InitIntervals();
	
	highlight_color = GeoDaConst::highlight_color;
	
	fixed_aspect_ratio_mode = false;
	use_category_brushes = false;
	selectable_shps_type = rectangles;
	
	PopulateCanvas();
	
	highlight_state->registerObserver(this);
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);  // default style
	LOG_MSG("Exiting CatClassifHistCanvas::CatClassifHistCanvas");
}

CatClassifHistCanvas::~CatClassifHistCanvas()
{
	LOG_MSG("Entering CatClassifHistCanvas::~CatClassifHistCanvas");
	highlight_state->removeObserver(this);
	LOG_MSG("Exiting CatClassifHistCanvas::~CatClassifHistCanvas");
}

void CatClassifHistCanvas::DisplayRightClickMenu(const wxPoint& pos)
{
	LOG_MSG("Entering CatClassifHistCanvas::DisplayRightClickMenu");
	wxMenu* optMenu;
	optMenu = wxXmlResource::Get()->
		LoadMenu("ID_CAT_CLASSIF_HIST_VIEW_MENU_OPTIONS");
	SetCheckMarks(optMenu);
	template_frame->UpdateContextMenuItems(optMenu);
	// The position passed in is the mouse position relative to
	// the canvas window, not the parent frame.  This is corrected
	// by adding the position of the canvas relative to its parent
	template_frame->PopupMenu(optMenu,
							  wxPoint(GetPosition().x + pos.x, pos.y));
	template_frame->UpdateOptionMenuItems();
	LOG_MSG("Exiting CatClassifHistCanvas::DisplayRightClickMenu");
}


void CatClassifHistCanvas::SetCheckMarks(wxMenu* menu)
{
	// Update the checkmarks and enable/disable state for the
	// following menu items if they were specified for this particular
	// view in the xrc file.  Items that cannot be enable/disabled,
	// or are not checkable do not appear.
}

void CatClassifHistCanvas::DetermineMouseHoverObjects()
{
	total_hover_obs = 0;
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (selectable_shps[i]->pointWithin(sel1)) {
			hover_obs[total_hover_obs++] = i;
			if (total_hover_obs == max_hover_obs) break;
		}
	}
}

// The following function assumes that the set of selectable objects
// all rectangles.
void CatClassifHistCanvas::UpdateSelection(bool shiftdown, bool pointsel)
{
	bool rect_sel = (!pointsel && (brushtype == rectangle));
	
	std::vector<bool>& hs = highlight_state->GetHighlight();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
	int total_newly_selected = 0;
	int total_newly_unselected = 0;
	
	int total_sel_shps = selectable_shps.size();
	
	wxPoint lower_left;
	wxPoint upper_right;
	if (rect_sel) {
		GenUtils::StandardizeRect(sel1, sel2, lower_left, upper_right);
	}
	if (!shiftdown) {
		bool any_selected = false;
		for (int i=0; i<total_sel_shps; i++) {
			MyRectangle* rec = (MyRectangle*) selectable_shps[i];
			if ((pointsel && rec->pointWithin(sel1)) ||
				(rect_sel &&
				 GenUtils::RectsIntersect(rec->lower_left, rec->upper_right,
										  lower_left, upper_right))) {
					 //LOG_MSG(wxString::Format("ival %d selected", i));
					 any_selected = true;
					 break;
				 } else {
					 //LOG_MSG(wxString::Format("ival %d not selected", i));
					 //LOG_MSG(wxString::Format(""));
				 }
		}
		if (!any_selected) {
			highlight_state->SetEventType(HighlightState::unhighlight_all);
			highlight_state->notifyObservers();
			return;
		}
	}
	
	for (int i=0; i<total_sel_shps; i++) {
		MyRectangle* rec = (MyRectangle*) selectable_shps[i];
		bool selected = ((pointsel && rec->pointWithin(sel1)) ||
						 (rect_sel &&
						  GenUtils::RectsIntersect(rec->lower_left,
												   rec->upper_right,
												   lower_left, upper_right)));
		bool all_sel = (ival_obs_cnt[i] == ival_obs_sel_cnt[i]);
		if (pointsel && all_sel && selected) {
			// unselect all in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
				 it != ival_to_obs_ids[i].end(); it++) {
				nuh[total_newly_unselected++] = (*it);
			}
		} else if (!all_sel && selected) {
			// select currently unselected in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
				 it != ival_to_obs_ids[i].end(); it++) {
				if (hs[*it]) continue;
				nh[total_newly_selected++] = (*it);
			}
		} else if (!selected && !shiftdown) {
			// unselect all selected in ival
			for (std::list<int>::iterator it=ival_to_obs_ids[i].begin();
				 it != ival_to_obs_ids[i].end(); it++) {
				if (!hs[*it]) continue;
				nuh[total_newly_unselected++] = (*it);
			}
		}
	}
	
	if (total_newly_selected > 0 || total_newly_unselected > 0) {
		highlight_state->SetTotalNewlyHighlighted(total_newly_selected);
		highlight_state->SetTotalNewlyUnhighlighted(total_newly_unselected);
		NotifyObservables();
	}
	UpdateStatusBar();
}

void CatClassifHistCanvas::DrawSelectableShapes(wxMemoryDC &dc)
{
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_cnt[i] == 0) continue;
		selectable_shps[i]->paintSelf(dc);
	}
}

void CatClassifHistCanvas::DrawHighlightedShapes(wxMemoryDC &dc)
{
	dc.SetPen(wxPen(highlight_color));
	dc.SetBrush(wxBrush(highlight_color, wxBRUSHSTYLE_CROSSDIAG_HATCH));
	
	for (int i=0, iend=selectable_shps.size(); i<iend; i++) {
		if (ival_obs_sel_cnt[i] == 0) continue;
		double s = (((double) ival_obs_sel_cnt[i]) /
					((double) ival_obs_cnt[i]));
		MyRectangle* rec = (MyRectangle*) selectable_shps[i];
		dc.DrawRectangle(rec->lower_left.x, rec->lower_left.y,
						 rec->upper_right.x - rec->lower_left.x,
						 (rec->upper_right.y - rec->lower_left.y)*s);
	}	
}

/** Override of TemplateCanvas method. */
void CatClassifHistCanvas::update(HighlightState* o)
{
	LOG_MSG("Entering CatClassifHistCanvas::update");
	
	int total = highlight_state->GetTotalNewlyHighlighted();
	std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
	
	HighlightState::EventType type = highlight_state->GetEventType();
	layer0_valid = false;
	layer1_valid = false;
	layer2_valid = false;
	
	UpdateIvalSelCnts();
	
	Refresh();
	
	LOG_MSG("Entering CatClassifHistCanvas::update");	
}

wxString CatClassifHistCanvas::GetCanvasTitle()
{
	wxString s;
	s << "Category Editor";
	return s;
}

void CatClassifHistCanvas::PopulateCanvas()
{
	LOG_MSG("Entering CatClassifHistCanvas::PopulateCanvas");
	BOOST_FOREACH( MyShape* shp, background_shps ) { delete shp; }
	background_shps.clear();
	BOOST_FOREACH( MyShape* shp, selectable_shps ) { delete shp; }
	selectable_shps.clear();
	BOOST_FOREACH( MyShape* shp, foreground_shps ) { delete shp; }
	foreground_shps.clear();
	
	double x_min = 0;
	double x_max = left_pad_const + right_pad_const
	+ interval_width_const * cur_intervals + 
	+ interval_gap_const * (cur_intervals-1);
	
	// orig_x_pos is the center of each histogram bar
	std::vector<double> orig_x_pos(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		orig_x_pos[i] = left_pad_const + interval_width_const/2.0
		+ i * (interval_width_const + interval_gap_const);
	}
	
	shps_orig_xmin = x_min;
	shps_orig_xmax = x_max;
	shps_orig_ymin = 0;
	shps_orig_ymax = max_num_obs_in_ival;
	
	axis_scale_y = AxisScale(0, shps_orig_ymax, 5);
	shps_orig_ymax = axis_scale_y.scale_max;
	y_axis = new MyAxis("Frequency", axis_scale_y,
						wxRealPoint(0,0), wxRealPoint(0, shps_orig_ymax),
						-9, 0);
	background_shps.push_back(y_axis);

	virtual_screen_marg_top = 25;
	virtual_screen_marg_bottom = 25+32;
	virtual_screen_marg_left = 25+35;
	virtual_screen_marg_right = 25;
	
	selectable_shps.resize(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		double x0 = orig_x_pos[i] - interval_width_const/2.0;
		double x1 = orig_x_pos[i] + interval_width_const/2.0;
		double y0 = 0;
		double y1 = ival_obs_cnt[i];
		selectable_shps[i] = new MyRectangle(wxRealPoint(x0, 0),
											 wxRealPoint(x1, y1));
		selectable_shps[i]->setPen((*colors)[i]);
		selectable_shps[i]->setBrush((*colors)[i]);
		
		if (i<cur_intervals-1) {
			MyText* brk = new MyText(GenUtils::DblToStr((*breaks)[i]),
									 *GeoDaConst::small_font,
									 wxRealPoint(x1, y0), 90,
									 MyText::right, MyText::v_center, 0, 10);
			background_shps.push_back(brk);
		}
	}
	
	ResizeSelectableShps();
	
	LOG_MSG("Exiting CatClassifHistCanvas::PopulateCanvas");
}

/** based on data and breaks and cur_intervals,
 populate obs_id_to_ival, ival_to_obs_id, ival_obs_cnt and ival_obs_sel_cnt
 Note: this algorithm is based on the theme == quantile case
 in the CatClassification::PopulateCatClassifData method. */ 
void CatClassifHistCanvas::InitIntervals()
{
	std::vector<bool>& hs = highlight_state->GetHighlight();
	
	ival_obs_cnt.resize(cur_intervals);
	ival_obs_sel_cnt.resize(cur_intervals);
	ival_to_obs_ids.clear();
	ival_to_obs_ids.resize(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		ival_obs_cnt[i] = 0;
		ival_obs_sel_cnt[i] = 0;
	}
	
	if (cur_intervals == 1) {
		for (int i=0; i<num_obs; i++) {
			obs_id_to_ival[i] = 0;
			ival_to_obs_ids[0].push_front(i);
		}		
		ival_obs_cnt[0] = num_obs;
		ival_obs_sel_cnt[0] = highlight_state->GetTotalHighlighted();
	} else {
		int num_breaks = breaks->size();
		int num_breaks_lower = num_breaks/2;
		
		double val;
		int ind;
		for (int i=0; i<num_obs; i++) {
			val = (*data)[i].first;
			ind = (*data)[i].second;
			bool found = false;
			int cat = num_breaks; // last cat by default
			for (int j=0; j<num_breaks_lower; j++) {
				if (val < (*breaks)[j]) {
					found = true;
					cat = j;
					break;
				}
			}
			if (!found) {
				for (int j=num_breaks_lower; j<num_breaks; j++) {
					if (val <= (*breaks)[j]) {
						cat = j;
						break;
					}
				}
			}
			// at this point we know that index ind belongs to category cat
			obs_id_to_ival[ind] = cat;
			ival_to_obs_ids[cat].push_front(ind);
			ival_obs_cnt[cat]++;
			if (hs[ind]) ival_obs_sel_cnt[cat]++;
		}
	}
	
	max_num_obs_in_ival = 0;
	for (int i=0; i<cur_intervals; i++) {
		if (ival_obs_cnt[i] > max_num_obs_in_ival) {
			max_num_obs_in_ival = ival_obs_cnt[i];
		}
	}
	
	LOG_MSG("InitIntervals: ");
	LOG_MSG(wxString::Format("max_num_obs_in_ival: %f", max_num_obs_in_ival));
	for (int i=0; i<cur_intervals; i++) {
		LOG_MSG(wxString::Format("ival_obs_cnt[%d] = %d",
								 i, ival_obs_cnt[i]));
	}
}

void CatClassifHistCanvas::UpdateIvalSelCnts()
{
	HighlightState::EventType type = highlight_state->GetEventType();
	if (type == HighlightState::unhighlight_all) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_sel_cnt[i] = 0;
		}
	} else if (type == HighlightState::delta) {
		std::vector<int>& nh = highlight_state->GetNewlyHighlighted();
		std::vector<int>& nuh = highlight_state->GetNewlyUnhighlighted();
		int nh_cnt = highlight_state->GetTotalNewlyHighlighted();
		int nuh_cnt = highlight_state->GetTotalNewlyUnhighlighted();
		
		for (int i=0; i<nh_cnt; i++) {
			ival_obs_sel_cnt[obs_id_to_ival[nh[i]]]++;
		}
		for (int i=0; i<nuh_cnt; i++) {
			ival_obs_sel_cnt[obs_id_to_ival[nuh[i]]]--;
		}
	} else if (type == HighlightState::invert) {
		for (int i=0; i<cur_intervals; i++) {
			ival_obs_sel_cnt[i] = 
			ival_obs_cnt[i] - ival_obs_sel_cnt[i];
		}
	}
}

void CatClassifHistCanvas::ChangeData(GeoDa::dbl_int_pair_vec_type* new_data)
{
	data = new_data;
	InitIntervals();
	PopulateCanvas();
}

void CatClassifHistCanvas::ChangeBreaks(std::vector<double>* new_breaks,
										std::vector<wxColour>* new_colors)
{
	breaks = new_breaks;
	colors = new_colors;
	cur_intervals = breaks->size() + 1;
	InitIntervals();
	PopulateCanvas();
}

void CatClassifHistCanvas::ChangeColorScheme(std::vector<wxColour>* new_colors)
{
	colors = new_colors;
	InitIntervals();
	PopulateCanvas();
}

void CatClassifHistCanvas::InitRandNormData(GeoDa::dbl_int_pair_vec_type& rnd)
{
	int n_obs = rnd.size();
	// Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	boost::normal_distribution<> norm_dist(0.0, 1.0);
	boost::variate_generator<boost::mt19937&,
	boost::normal_distribution<> > X(rng, norm_dist);
	for (int i=0; i<n_obs; i++) {
		rnd[i].first = X();
		rnd[i].second = i;
	}
	std::sort(rnd.begin(), rnd.end(),
			  GeoDa::dbl_int_pair_cmp_less);
}

void CatClassifHistCanvas::UpdateStatusBar()
{
	wxStatusBar* sb = template_frame->GetStatusBar();
	if (!sb) return;
	if (total_hover_obs == 0) {
		sb->SetStatusText("");
		return;
	}
	int ival = hover_obs[0];
	wxString s;
	s << "bin: " << ival+1 << ", range: ";
	if (cur_intervals <= 1) {
		s << "(-inf, inf)";
	} else if (ival == 0) {
		s << "(-inf, " << (*breaks)[ival] << ")";
	} else if (ival == cur_intervals-1) {
		s << "[" << (*breaks)[ival-1] << ", inf)";
	} else {
		int num_breaks = cur_intervals-1;
		int num_breaks_lower = num_breaks/2;
		wxString a;
		wxString b;
		if (ival < num_breaks_lower) {
			a = "(";
			b = "]";
		} else if (ival == num_breaks_lower) {
			a = "[";
			b = "]";
		} else {
			a = "[";
			b = ")";
		}
		s << a << (*breaks)[ival-1] << ", " << (*breaks)[ival] << b;
	}
	s << ", #obs: " << ival_obs_cnt[ival];
	double p = 100.0*((double) ival_obs_cnt[ival])/((double) num_obs);
	s << ", %tot: ";
	s << wxString::Format("%.1f", p);
	s << "%, #sel: " << ival_obs_sel_cnt[ival];
	
	sb->SetStatusText(s);
}

const int CatClassifPanel::max_intervals = 10;
const int CatClassifPanel::default_intervals = 6;

typedef std::pair<int, wxWindow*> int_win_pair_type;
typedef std::vector<int_win_pair_type> int_win_pair_vec_type;
bool int_win_pair_less(const int_win_pair_type& ind1,
					   const int_win_pair_type& ind2)
{
	return ind1.first < ind2.first;
}

BEGIN_EVENT_TABLE(CatClassifPanel, wxPanel)
	EVT_CHOICE(XRCID("ID_CUR_CATS_CHOICE"), CatClassifPanel::OnCurCatsChoice)
	EVT_CHOICE(XRCID("ID_COLOR_SCHEME"), CatClassifPanel::OnColorScheme)
	EVT_SPINCTRL(XRCID("ID_NUM_CATS"), CatClassifPanel::OnNumCatsSpinCtrl)
	EVT_CHOICE( XRCID("ID_PREVIEW_VAR"), CatClassifPanel::OnFieldChoice)
	EVT_CHOICE( XRCID("ID_PREVIEW_VAR_TM"), CatClassifPanel::OnFieldChoiceTm)
	EVT_SLIDER(XRCID("ID_BRK_SLIDER"), CatClassifPanel::OnBrkSlider)
	EVT_SCROLL_THUMBRELEASE(CatClassifPanel::OnScrollThumbRelease)
	EVT_TEXT(XRCID("ID_CAT_TXT"), CatClassifPanel::OnCatTxt)
	EVT_RADIOBUTTON(XRCID("ID_BRK_RAD"), CatClassifPanel::OnBrkRad)	
	EVT_TEXT_ENTER(XRCID("ID_BRK_TXT"), CatClassifPanel::OnBrkTxtEnter)
	EVT_BUTTON(XRCID("ID_COPY_FROM_EXISTING"),
			   CatClassifPanel::OnButtonCopyFromExisting)
	EVT_BUTTON(XRCID("ID_CHANGE_TITLE"),
			   CatClassifPanel::OnButtonChangeTitle)
	EVT_BUTTON(XRCID("wxID_NEW"), CatClassifPanel::OnButtonNew)
	EVT_BUTTON(XRCID("wxID_DELETE"), CatClassifPanel::OnButtonDelete)
	EVT_BUTTON(XRCID("wxID_CLOSE"), CatClassifPanel::OnButtonClose)
END_EVENT_TABLE()

CatClassifPanel::CatClassifPanel(Project* project_s,
								 CatClassifHistCanvas* hist_canvas_s,
								 wxWindow* parent, wxWindowID id,
								 const wxPoint& pos, const wxSize& size,
								 long style)
: project(project_s), grid_base(project_s->GetGridBase()),
hist_canvas(hist_canvas_s),
cat_classif_manager(project_s->GetCatClassifManager()), cc_state(0),
table_state(project_s->GetTableState()),
cur_field_choice(""), cur_field_choice_tm(0),
data(project_s->GetNumRecords()), cur_intervals(default_intervals),
num_obs(project_s->GetNumRecords()), all_init(false)
{
	SetParent(parent);
	
	CatClassifHistCanvas::InitRandNormData(data);
	cc_data.cat_classif_type = CatClassification::custom;
	cc_data.color_scheme = CatClassification::diverging_color_scheme;
	CatClassification::ChangeNumCats(default_intervals, cc_data);
	
	//Begin Creating Controls
	
	wxXmlResource::Get()->LoadPanel(this, GetParent(), "ID_CAT_CLASSIF");
	
	num_cats_spin_ctrl = wxDynamicCast(FindWindow(XRCID("ID_NUM_CATS")),
									   wxSpinCtrl);
	cur_cats_choice = wxDynamicCast(FindWindow(XRCID("ID_CUR_CATS_CHOICE")),
									wxChoice);
	copy_from_existing_button =
		wxDynamicCast(FindWindow(XRCID("ID_COPY_FROM_EXISTING")), wxButton);
	change_title_button =
		wxDynamicCast(FindWindow(XRCID("ID_CHANGE_TITLE")), wxButton);
	delete_button =	wxDynamicCast(FindWindow(XRCID("wxID_DELETE")), wxButton);
	min_lbl = wxDynamicCast(FindWindow(XRCID("ID_MIN_LBL")), wxStaticText);
	max_lbl = wxDynamicCast(FindWindow(XRCID("ID_MAX_LBL")), wxStaticText);
	color_scheme = wxDynamicCast(FindWindow(XRCID("ID_COLOR_SCHEME")),
								 wxChoice);
	color_scheme->SetSelection(1);
	field_choice = wxDynamicCast(FindWindow(XRCID("ID_PREVIEW_VAR")), wxChoice);
	field_choice->Clear();
	field_choice->Append("normal distribution");
	field_choice->SetSelection(0);
	field_choice_tm = wxDynamicCast(FindWindow(XRCID("ID_PREVIEW_VAR_TM")),
								   wxChoice);
	field_choice_tm->Show(grid_base->IsTimeVariant());
	
	brk_slider = wxDynamicCast(FindWindow(XRCID("ID_BRK_SLIDER")), wxSlider);
	
	int_win_pair_vec_type cat_but_srt_vec;
	int_win_pair_vec_type cat_txt_srt_vec;
	int_win_pair_vec_type brk_rad_srt_vec;
	int_win_pair_vec_type brk_lbl_srt_vec;
	int_win_pair_vec_type brk_txt_srt_vec;
	
	wxWindowList& win_list = GetChildren();
	for (wxWindowList::iterator it = win_list.begin(); it != win_list.end();
		 it++) {
		if ((*it)->GetId() == XRCID("ID_CAT_BUT")) {
			cat_but_srt_vec.push_back(std::make_pair((*it)->GetPosition().y,
													 (*it)));
		} else if ((*it)->GetId() == XRCID("ID_CAT_TXT")) {
			cat_txt_srt_vec.push_back(std::make_pair((*it)->GetPosition().y,
													 (*it)));
		} else if ((*it)->GetId() == XRCID("ID_BRK_RAD")) {
			brk_rad_srt_vec.push_back(std::make_pair((*it)->GetPosition().y,
													 (*it)));
		} else if ((*it)->GetId() == XRCID("ID_BRK_LBL")) {
			brk_lbl_srt_vec.push_back(std::make_pair((*it)->GetPosition().y,
													 (*it)));
		} else if ((*it)->GetId() == XRCID("ID_BRK_TXT")) {
			brk_txt_srt_vec.push_back(std::make_pair((*it)->GetPosition().y,
													 (*it)));
		}
	}
		
	std::sort(cat_but_srt_vec.begin(), cat_but_srt_vec.end(),
			  int_win_pair_less);
	cat_but.resize(cat_but_srt_vec.size());
	for (int i=0, iend=cat_but_srt_vec.size(); i<iend; i++) {
		cat_but[i] = (wxStaticBitmap*) cat_but_srt_vec[i].second;
		cat_but[i]->Bind(wxEVT_LEFT_UP, &CatClassifPanel::OnCatBut, this);
	}
	
	std::sort(cat_txt_srt_vec.begin(), cat_txt_srt_vec.end(),
			  int_win_pair_less);
	cat_txt.resize(cat_txt_srt_vec.size());
	for (int i=0, iend=cat_txt_srt_vec.size(); i<iend; i++) {
		cat_txt[i] = (wxTextCtrl*) cat_txt_srt_vec[i].second;
	}
	for (int i=0; i<cc_data.num_cats; i++) {
		cc_data.names[i] = cat_txt[i]->GetValue();
	}
	
	std::sort(brk_rad_srt_vec.begin(), brk_rad_srt_vec.end(),
			  int_win_pair_less);
	brk_rad.resize(brk_rad_srt_vec.size());
	for (int i=0, iend=brk_rad_srt_vec.size(); i<iend; i++) {
		brk_rad[i] = (wxRadioButton*) brk_rad_srt_vec[i].second;
	}	
	
	std::sort(brk_lbl_srt_vec.begin(), brk_lbl_srt_vec.end(),
			  int_win_pair_less);
	brk_lbl.resize(brk_lbl_srt_vec.size());
	for (int i=0, iend=brk_lbl_srt_vec.size(); i<iend; i++) {
		brk_lbl[i] = (wxStaticText*) brk_lbl_srt_vec[i].second;
		wxString s;
		s << "break " << i+1 << ":";
		brk_lbl[i]->SetLabelText(s);
	}
	
	std::sort(brk_txt_srt_vec.begin(), brk_txt_srt_vec.end(),
			  int_win_pair_less);
	brk_txt.resize(brk_txt_srt_vec.size());
	for (int i=0, iend=brk_txt_srt_vec.size(); i<iend; i++) {
		brk_txt[i] = (wxTextCtrl*) brk_txt_srt_vec[i].second;
		brk_txt[i]->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
		//brk_txt[i]->SetValidator(wxFloatingPointValidator<double> validator);
		brk_txt[i]->Bind(wxEVT_KILL_FOCUS, &CatClassifPanel::OnKillFocusEvent,
						 this);
	}
	brk_slider->SetRange(0, 1000);
	brk_slider_min.resize(brk_txt.size());
	brk_slider_max.resize(brk_txt.size());
	
	//End Creating Controls
	//wxString str;
	//CatClassification::PrintCatClassifDef(cc_data, str);
	//LOG(str);
	all_init = true;
	InitFieldChoices();
	ResetValuesToDefault();
	EnableControls(false);
	
	InitCurCatsChoices();
	wxString cc_str_sel = cur_cats_choice->GetStringSelection();
	cc_state = cat_classif_manager->FindClassifState(cc_str_sel);
	if (cc_state) {
		cc_data = cc_state->GetCatClassif();
		InitFromCCData();
		EnableControls(true);
	}
	
	table_state->registerObserver(this);
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
}

CatClassifPanel::~CatClassifPanel()
{
	table_state->removeObserver(this);
}

CatClassifState* CatClassifPanel::PromptNew(const CatClassifDef& ccd,
											const wxString& suggested_title,
											const wxString& field_name,
											int field_tm)
{
	if (!all_init) return 0;
	wxString msg;
	msg << "New Custom Categories Title:";
	wxString new_title = (suggested_title.IsEmpty() ?
						  GetDefaultTitle(field_name, field_tm) :
						  suggested_title);
	bool retry = true;
	bool success = false;
	while (retry) {
		wxTextEntryDialog dlg(this, msg, "New Categories Title");
		dlg.SetValue(new_title);
		if (dlg.ShowModal() == wxID_OK) {
			new_title = dlg.GetValue();
			new_title.Trim(false);
			new_title.Trim(true);
			if (new_title.IsEmpty()) {
				retry = false;
			} else if (IsDuplicateTitle(new_title)) {
				wxString es;
				es << "Categories title \"" << new_title << "\" already ";
				es << "exists. Please choose a different title.";
				wxMessageDialog ed(NULL, es, "Error", wxOK | wxICON_ERROR);
				ed.ShowModal();
			} else {
				success = true;
				cc_data = ccd;
				cc_data.title = new_title;
				cc_data.cat_classif_type = CatClassification::custom;
				int f_sel = field_choice->FindString(field_name);
				if (f_sel != wxNOT_FOUND) {
					field_choice->SetSelection(f_sel);
					cur_field_choice = field_name;
					cc_data.default_var = field_name;
					if (grid_base->IsColTimeVariant(field_name)) {
						field_choice_tm->SetSelection(field_tm);
						cur_field_choice_tm = field_tm;
						cc_data.default_var_tm = field_tm;
					} else {
						cur_field_choice_tm = 0;
					}
				}
				InitNewFieldChoice();
				InitFromCCData();
				cc_state = cat_classif_manager->CreateNewClassifState(cc_data);
				cur_cats_choice->Append(new_title);
				cur_cats_choice->SetSelection(cur_cats_choice->GetCount()-1);
				EnableControls(true);
				retry = false;
			}
		} else {
			retry = false;
		}
	}
	return success ? cc_state : 0;
}

void CatClassifPanel::EditExisting(const wxString& cat_classif_title,
								   const wxString& field_name,
								   int field_tm)
{
	
}

void CatClassifPanel::OnKillFocusEvent(wxFocusEvent& event)
{
	LOG_MSG("In CatClassifPanel::OnKillFocusEvent");
	wxWindow* w = (wxWindow*) (event.GetEventObject());
	if (wxTextCtrl* tc = dynamic_cast<wxTextCtrl*>(w)) {
		LOG(tc->GetValue());
		int obj_id = -1;
		for (int i=0; i<cc_data.breaks.size() && obj_id<0; i++) {
			if (tc == brk_txt[i]) obj_id = i;
		}
		if (obj_id != -1) {
			LOG_MSG(wxString::Format("Focus left brk_text[%d]", obj_id));
			wxString s(brk_txt[obj_id]->GetValue());
			double val;
			if (s.ToDouble(&val)) {
				if (val != cc_data.breaks[obj_id]) {
					int nbrk = CatClassification::ChangeBreakValue(obj_id, val,
																   cc_data);
					UpdateCCState();
					UpdateBrkTxtRad(nbrk);
					hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
					UpdateBrkSliderRanges();
					double new_min = brk_slider_min[0];
					double new_max = brk_slider_max[0];
					for (int i=0; i<cc_data.breaks.size(); i++) {
						if (brk_slider_min[i] < new_min) {
							new_min = brk_slider_min[i];
						}
						if (brk_slider_max[i] > new_max) {
							new_max = brk_slider_max[i];
						}
					}
					min_lbl->SetLabelText(GenUtils::DblToStr(new_min));
					max_lbl->SetLabelText(GenUtils::DblToStr(new_max));
					InitSliderFromBreak(nbrk);
					Refresh();
				}
			} else {
				wxString o;
				o << cc_data.breaks[obj_id];
				brk_txt[obj_id]->ChangeValue(o);
			}
		}
	}
	event.Skip();
}

void CatClassifPanel::OnCurCatsChoice(wxCommandEvent& event)
{
	if (!all_init) return;
	wxString cc_str_sel = cur_cats_choice->GetStringSelection();
	cc_state = cat_classif_manager->FindClassifState(cc_str_sel);
	if (cc_state) {
		cc_data = cc_state->GetCatClassif();
		int col = grid_base->FindColId(cc_data.default_var);
		if (col >= 0) {
			cur_field_choice = cc_data.default_var;
			if (grid_base->IsColTimeVariant(col)) {
				field_choice_tm->Enable(true);
				field_choice_tm->SetSelection(cc_data.default_var_tm);
			} else {
				field_choice_tm->Enable(false);
			}
		}
		InitFromCCData();
		InitNewFieldChoice();
		EnableControls(true);
	}
}

void CatClassifPanel::OnColorScheme(wxCommandEvent& event)
{
	if (!all_init) return;
	if (color_scheme->GetSelection() == 0) {
		cc_data.color_scheme = CatClassification::sequential_color_scheme;
	} else if (color_scheme->GetSelection() == 1) {
		cc_data.color_scheme = CatClassification::diverging_color_scheme;
	} else if (color_scheme->GetSelection() == 2) {
		cc_data.color_scheme = CatClassification::qualitative_color_scheme;
	} else {
		cc_data.color_scheme = CatClassification::custom_color_scheme;
	}
	if (cc_data.color_scheme != CatClassification::custom_color_scheme) {
		CatClassification::PickColorSet(cc_data.colors,
										cc_data.color_scheme,
										cc_data.num_cats, false);
		for (int i=0; i<cc_data.colors.size(); i++) {
			cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
		}
		UpdateCCState();
		hist_canvas->ChangeColorScheme(&cc_data.colors);
		Refresh();
	}
}

void CatClassifPanel::OnNumCatsSpinCtrl(wxSpinEvent& event)
{
	if (!all_init) return;
	int cats = num_cats_spin_ctrl->GetValue();
	if (cats < 1 || cats > max_intervals) return;
	ChangeNumCats(cats);
}

/**
 Assumptions: all widgets are initialized
 Purpose: Initialize field_choice and field_choice_tm
 widgets.  If any current selections, then these are remembered
 and reselected. Note: InitFieldChoices is based on methods
 in DataMovieDlg class
 */
void CatClassifPanel::InitFieldChoices()
{
	LOG_MSG("Entering CatClassifPanel::InitFieldChoices");
	if (!all_init) return;
	wxString cur_fc_str = field_choice->GetStringSelection();
	int cur_fc_tm_id = field_choice_tm->GetSelection();
	field_choice->Clear();
	field_choice_tm->Clear();
	std::vector<wxString> times;
	grid_base->GetTimeStrings(times);
	for (int i=0; i<times.size(); i++) {
		field_choice_tm->Append(times[i]);
	}
	if (cur_fc_tm_id != wxNOT_FOUND) {
		field_choice_tm->SetSelection(cur_fc_tm_id);
	}
	std::vector<wxString> names;
	grid_base->FillNumericNameList(names);
	field_choice->Append("normal distribution");
	for (int i=0; i<names.size(); i++) {
		field_choice->Append(names[i]);
	}
	field_choice->SetSelection(field_choice->FindString(cur_fc_str));
	field_choice_tm->Enable(grid_base->IsColTimeVariant(cur_fc_str));
	if (grid_base->IsColTimeVariant(cur_fc_str) &&
		field_choice_tm->GetSelection() == wxNOT_FOUND) {
		field_choice_tm->SetSelection(0);
	}
	
	if (field_choice->FindString(cur_fc_str) != wxNOT_FOUND) {
		cur_field_choice = cur_fc_str;
		if (grid_base->IsColTimeVariant(cur_fc_str)) {
			cur_field_choice_tm = field_choice_tm->GetSelection();
		} else {
			cur_field_choice_tm = 0;
		}
	} else {
		// default to normal distribution
		cur_field_choice = "normal distribution";
		field_choice->SetSelection(0);
	}
	LOG(grid_base->IsColTimeVariant(cur_fc_str));
	LOG_MSG("Exiting CatClassifPanel::InitFieldChoices");
}

void CatClassifPanel::OnFieldChoice(wxCommandEvent& ev)
{
	LOG_MSG("Entering CatClassifPanel::OnFieldChoice");
	wxString cur_fc_str = field_choice->GetStringSelection();
	bool is_tm_var = grid_base->IsColTimeVariant(cur_fc_str);
	field_choice_tm->Enable(is_tm_var);
	if (is_tm_var && field_choice_tm->GetSelection() == wxNOT_FOUND) {
		field_choice_tm->SetSelection(0);
	}
	InitFieldChoices();
	InitNewFieldChoice();
	
	// MMM is new field choice exists in Table Data, then make this
	// the new default_var for current custom category
	// Also update default_var_tm if needed
	
	LOG_MSG("Exiting CatClassifPanel::OnFieldChoice");
}

void CatClassifPanel::OnFieldChoiceTm(wxCommandEvent& ev)
{
	LOG_MSG("Entering CatClassifPanel::OnFieldChoiceTm");
	InitFieldChoices();
	InitNewFieldChoice();
	
	// MMM is new field choice exists in Table Data, then make this
	// the new default_var for current custom category
	
	LOG_MSG("Exiting CatClassifPanel::OnFieldChoiceTm");
}

/** A new field_choice / field_choice_tm combination has been selected.  If
 cur_field_choice str is normal distribution, then resample from a
 normal distribution, otherwise get data from table update.
 */
void CatClassifPanel::InitNewFieldChoice()
{
	LOG_MSG("Entering CatClassifPanel::InitNewFieldChoice");
	if (cur_field_choice.IsEmpty()) {
		cur_field_choice = "normal distribution";
	}
	if (cur_field_choice == "normal distribution") {
		CatClassifHistCanvas::InitRandNormData(data);
	} else {
		int col = grid_base->FindColId(cur_field_choice);
		std::vector<double> dd;
		grid_base->GetColData(col, cur_field_choice_tm, dd);
		for (int i=0; i<num_obs; i++) {
			data[i].first = dd[i];
			data[i].second = i;
		}
		std::sort(data.begin(), data.end(), GeoDa::dbl_int_pair_cmp_less);
	}
	LOG(data[0].first);
	LOG(data[num_obs-1].first);
	hist_canvas->ChangeData(&data);
	LOG_MSG("Exiting CatClassifPanel::InitNewFieldChoice");
}

void CatClassifPanel::InitCurCatsChoices()
{
	LOG_MSG("Entering CatClassifPanel::InitCutCatsChoices");
	wxString cur_str = cur_cats_choice->GetStringSelection();
	cur_cats_choice->Clear();
	std::vector<wxString> titles;
	cat_classif_manager->GetTitles(titles);
	for (int i=0, ie=titles.size(); i<ie; i++) {
		cur_cats_choice->Append(titles[i]);
	}
	if (cur_cats_choice->GetCount() == 0) return;
	int sel = cur_cats_choice->FindString(cur_str);
	if (sel != wxNOT_FOUND) {
		cur_cats_choice->SetSelection(sel);
	} else {
		cur_cats_choice->SetSelection(0);
	}
	LOG_MSG("Exiting CatClassifPanel::InitCutCatsChoices");
}


void CatClassifPanel::OnBrkSlider(wxCommandEvent& event)
{
	if (!all_init) return;
	if (last_brk_slider_pos == brk_slider->GetValue()) return;
	last_brk_slider_pos = brk_slider->GetValue();
	//LOG_MSG("In CatClassifPanel::OnBrkSlider");
	int brk = GetActiveBrkRadio();
	if (brk < 0 || brk > cur_intervals-2) return;
	double sr = ((double) (brk_slider->GetMax()-brk_slider->GetMin()));
	if (sr == 0) return;
	double sd = ((double) (brk_slider->GetValue()-brk_slider->GetMin()));
	double r = brk_slider_max[brk]-brk_slider_min[brk];
	double v = brk_slider_min[brk] + (sd/sr)*r;
	LOG(v);
	cc_data.breaks[brk] = v;
	brk_txt[brk]->ChangeValue(GenUtils::DblToStr(v));
	int nbrk = CatClassification::ChangeBreakValue(brk, v, cc_data);
	// refresh just the radio buttons and break txt
	UpdateBrkTxtRad(nbrk);
	UpdateCCState();
	hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
}

void CatClassifPanel::OnScrollThumbRelease(wxScrollEvent& event)
{
	//LOG_MSG("In CatClassifPanel::OnScrollThumbRelease");
	InitSliderFromBreak(GetActiveBrkRadio());
	UpdateBrkSliderRanges();
}

void CatClassifPanel::OnCatBut(wxMouseEvent& event)
{
	if (!all_init) return;
	int obj_id = -1;
	
	wxStaticBitmap* obj = (wxStaticBitmap*) event.GetEventObject();
	for (int i=0, iend=cat_but.size(); i<iend && obj_id==-1; i++) {
		if (obj == cat_but[i]) obj_id = i;
	}

	//int pos_x; int pos_y;
	//event.GetPosition(&pos_x, &pos_y);
	//for (int i=0, iend=cat_but.size(); i<iend && obj_id==-1; i++) {
	//	int x, y, w, h;
	//	cat_but[i]->GetPosition(&x, &y);
	//	cat_but[i]->GetSize(&w, &h);
	//	if ((pos_x >= x && pos_x <= x+w) &&
	//		(pos_y >= y && pos_y <= y+h)) obj_id = i;
	//}
	LOG(obj_id);
	if (obj_id < 0) return;
	
	wxColour col = cc_data.colors[obj_id];
	wxColourData data;
	data.SetColour(col);
	data.SetChooseFull(true);
	int ki;
	for (ki = 0; ki < 16; ki++) {
		wxColour colour(ki * 16, ki * 16, ki * 16);
		data.SetCustomColour(ki, colour);
	}
	
	wxColourDialog dialog(this, &data);
	dialog.SetTitle("Choose Cateogry Color");
	if (dialog.ShowModal() != wxID_OK) return;
	wxColourData retData = dialog.GetColourData();
	if (cc_data.colors[obj_id] == retData.GetColour()) return;
	color_scheme->SetSelection(3);
	cc_data.color_scheme = CatClassification::custom_color_scheme;
	cc_data.colors[obj_id] = retData.GetColour();
	cat_but[obj_id]->SetBackgroundColour(retData.GetColour());
	UpdateCCState();
	hist_canvas->ChangeColorScheme(&cc_data.colors);
	Refresh();
}

void CatClassifPanel::OnCatTxt(wxCommandEvent& event)
{
	if (!all_init) return;
	wxTextCtrl* obj = (wxTextCtrl*) event.GetEventObject();
	int obj_id = -1;
	for (int i=0, iend=cat_txt.size(); i<iend && obj_id==-1; i++) {
		if (obj == cat_txt[i]) obj_id = i;
	}
	LOG(obj_id);
	if (obj_id < 0) return;
	cc_data.names[obj_id] = cat_txt[obj_id]->GetValue();
	UpdateCCState();
}

void CatClassifPanel::OnBrkRad(wxCommandEvent& event)
{
	if (!all_init) return;
	wxRadioButton* obj = (wxRadioButton*) event.GetEventObject();
	int obj_id = -1;
	for (int i=0, iend=brk_rad.size(); i<iend && obj_id==-1; i++) {
		if (obj == brk_rad[i]) obj_id = i;
	}
	LOG(obj_id);
	if (obj_id < 0) return;
	InitSliderFromBreak(obj_id);
}

void CatClassifPanel::OnBrkTxtEnter(wxCommandEvent& event)
{
	if (!all_init) return;
	wxTextCtrl* obj = (wxTextCtrl*) event.GetEventObject();
	int obj_id = -1;
	for (int i=0, iend=brk_txt.size(); i<iend && obj_id==-1; i++) {
		if (obj == brk_txt[i]) obj_id = i;
	}
	LOG_MSG("In CatClassifPanel::OnBrkTxtEnter");
	LOG(obj_id);
	if (obj_id < 0) return;
	wxString s(brk_txt[obj_id]->GetValue());
	double val;
	if (s.ToDouble(&val)) {
		if (val != cc_data.breaks[obj_id]) {
			int nbrk = CatClassification::ChangeBreakValue(obj_id, val,
														   cc_data);
			UpdateBrkTxtRad(nbrk);
			UpdateCCState();
			hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
			InitSliderFromBreak(nbrk);
		}
	} else {
		wxString o;
		o << cc_data.breaks[obj_id];
		brk_txt[obj_id]->ChangeValue(o);
	}
}

void CatClassifPanel::OnButtonCopyFromExisting(wxCommandEvent& event)
{
	if (!all_init) return;
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("ID_CAT_CLASSIF_POPUP");
	wxWindow* win = (wxWindow*) event.GetEventObject();
	PopupMenu(menu, win->GetPosition());
}

void CatClassifPanel::OnButtonChangeTitle(wxCommandEvent& event)
{
	if (!all_init) return;
	wxString msg;
	wxString cur_title = cur_cats_choice->GetStringSelection();
	wxString new_title = cur_title;
	msg << "Change title \"" << cur_title << "\" to";
	bool retry = true;
	while (retry) {
		wxTextEntryDialog dlg(this, msg, "Change Categories Title");
		dlg.SetValue(new_title);
		if (dlg.ShowModal() == wxID_OK) {
			new_title = dlg.GetValue();
			new_title.Trim(false);
			new_title.Trim(true);
			if (new_title.IsEmpty() || new_title == cur_title) {
				retry = false;
			} else if (IsDuplicateTitle(new_title)) {
				wxString es;
				es << "Categories title \"" << new_title << "\" already ";
				es << "exists. Please choose a different title.";
				wxMessageDialog ed(NULL, es, "Error", wxOK | wxICON_ERROR);
				ed.ShowModal();
			} else {
				int sel = cur_cats_choice->GetSelection();
				cur_cats_choice->SetString(sel, new_title);
				cc_data.title = new_title;
				UpdateCCState();
				retry = false;
			}
		} else {
			retry = false;
		}
	}
}

void CatClassifPanel::OnButtonNew(wxCommandEvent& event)
{
	if (!all_init) return;
	wxString msg;
	msg << "New Custom Categories Title:";
	wxString new_title = GetDefaultTitle();
	bool retry = true;
	while (retry) {
		wxTextEntryDialog dlg(this, msg, "New Categories Title");
		dlg.SetValue(new_title);
		if (dlg.ShowModal() == wxID_OK) {
			new_title = dlg.GetValue();
			new_title.Trim(false);
			new_title.Trim(true);
			if (new_title.IsEmpty()) {
				retry = false;
			} else if (IsDuplicateTitle(new_title)) {
				wxString es;
				es << "Categories title \"" << new_title << "\" already ";
				es << "exists. Please choose a different title.";
				wxMessageDialog ed(NULL, es, "Error", wxOK | wxICON_ERROR);
				ed.ShowModal();
			} else {
				cc_data.title = new_title;
				cc_state = cat_classif_manager->CreateNewClassifState(cc_data);
				cur_cats_choice->Append(new_title);
				cur_cats_choice->SetSelection(cur_cats_choice->GetCount()-1);
				EnableControls(true);
				retry = false;
			}
		} else {
			retry = false;
		}
	}
}

void CatClassifPanel::OnButtonDelete(wxCommandEvent& event)
{
	if (!all_init) return;
	wxString custom_cat_title = cur_cats_choice->GetStringSelection();
	if (IsOkToDelete(custom_cat_title)) {
		cat_classif_manager->RemoveClassifState(cc_state);
		cc_state = 0;
		cur_cats_choice->Delete(cur_cats_choice->GetSelection());
		if (cur_cats_choice->GetCount() > 0) {
			cur_cats_choice->SetSelection(0);
			cc_state = cat_classif_manager->
				FindClassifState(cur_cats_choice->GetStringSelection());
			cc_data = cc_state->GetCatClassif();
			InitFromCCData();
		} else {
			ResetValuesToDefault();
			CatClassifHistCanvas::InitRandNormData(data);
			hist_canvas->ChangeData(&data);
			hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
			EnableControls(false);
		}
	} else {
		wxString es;
		es << "Categories \"" << custom_cat_title << "\" is currently ";
		es << "in use by another view. Please close or change all views ";
		es << "using this custom categories before deleting.";
		wxMessageDialog ed(NULL, es, "Error", wxOK | wxICON_ERROR);
		ed.ShowModal();
	}
}

void CatClassifPanel::OnButtonClose(wxCommandEvent& event)
{
	template_frame->Close();
}

bool CatClassifPanel::IsDuplicateTitle(const wxString& title)
{
	wxString ttl(title);
	ttl.Trim(false);
	ttl.Trim(true);
	int num_cl = cur_cats_choice->GetCount();
	for (int i=0; i<num_cl; i++) {
		if (ttl == cur_cats_choice->GetString(i)) return true;
	}
	return false;
}

wxString CatClassifPanel::GetDefaultTitle(const wxString& field_name,
										  int field_tm)
{
	int max_tries = 500;
	int cur_try = 1;
	wxString ret_title_base("Custom Breaks");
	if (grid_base->ColNameExists(field_name)) {
		ret_title_base << " (" << field_name;
		if (grid_base->IsColTimeVariant(field_name)) {
			ret_title_base << ", " << grid_base->GetTimeString(field_tm);
		}
		ret_title_base << ")";
	}
	wxString ret_title = ret_title_base;
	while (IsDuplicateTitle(ret_title) && cur_try <= max_tries) {
		ret_title = ret_title_base;
		ret_title << " " << cur_try++;
	}
	return ret_title;
}

void CatClassifPanel::EnableControls(bool enable)
{
	cur_cats_choice->Enable(enable);
	copy_from_existing_button->Enable(enable);
	change_title_button->Enable(enable);
	delete_button->Enable(enable);
	num_cats_spin_ctrl->Enable(enable);
	color_scheme->Enable(enable);
	field_choice->Enable(enable);
	field_choice_tm->Enable(enable);
	brk_slider->Enable(enable);
	for (int i=0; i<cat_but.size(); i++) {
		cat_but[i]->Enable(enable);
		cat_txt[i]->Enable(enable);
	}
	for (int i=0; i<brk_rad.size(); i++) {
		brk_rad[i]->Enable(enable);
		brk_txt[i]->Enable(enable);
	}
}

/** Reset cc_data and all controls according to default_intervals.
 Assumes controls have already been created, but assumes nothing about
 current values. */
void CatClassifPanel::ResetValuesToDefault()
{
	cur_intervals = default_intervals;
	CatClassification::ChangeNumCats(default_intervals, cc_data);
	num_cats_spin_ctrl->SetValue(default_intervals);

	CatClassification::PickColorSet(cc_data.colors,
									CatClassification::diverging_color_scheme,
									default_intervals, false);
	for (int i=0; i<default_intervals; i++) {
		cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
	}
	for (int i=0, iend=cat_txt.size(); i<iend; i++) {
		wxString s;
		s << "category " << i+1;
		cat_txt[i]->ChangeValue(s);
	}
	for (int i=0; i<default_intervals; i++) {
		wxString s;
		s << "category " << i+1;
		cc_data.names[i] = s;
	}
	for (int i=0, iend=brk_txt.size(); i<iend; i++) {
		brk_txt[i]->ChangeValue("3.0");
	}
	// equal spacing between -3 sd and +3 sd
	for (int i=0; i<default_intervals-1; i++) {
		double brk = ((double) i+1)/((double) default_intervals);
		brk *= 6.0;
		brk -= 3.0;
		wxString s;
		s << brk;
		brk_txt[i]->ChangeValue(s);
		cc_data.breaks[i] = brk;
	}
	min_lbl->SetLabelText("-4");
	max_lbl->SetLabelText("4");
	brk_slider->SetValue(250);
	last_brk_slider_pos = 250;
	for (int i=0; i<brk_txt.size(); i++) {
		brk_slider_min[i] = -4;
		brk_slider_max[i] = 4;
	}
	ShowNumCategories(default_intervals);
	UpdateBrkTxtRad(0);
	InitSliderFromBreak(0);
	field_choice->SetSelection(0);
	Refresh();
}


/** Initialize all controls and CatClassifHistogram according to current
 cc_data and preview_variable */
void CatClassifPanel::InitFromCCData()
{
	CatClassification::PickColorSet(cc_data.colors, cc_data.color_scheme,
									cc_data.breaks.size()+1);
	if (cc_data.color_scheme == CatClassification::sequential_color_scheme) {
		color_scheme->SetSelection(0);
	} else if (cc_data.color_scheme ==
			   CatClassification::diverging_color_scheme) {
		color_scheme->SetSelection(1);
	} else if (cc_data.color_scheme ==
			   CatClassification::qualitative_color_scheme) {
		color_scheme->SetSelection(2);
	} else {
		color_scheme->SetSelection(3);
	}
	cur_intervals = cc_data.breaks.size()+1;
	num_cats_spin_ctrl->SetValue(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
		cat_txt[i]->ChangeValue(cc_data.names[i]);
	}
	for (int i=0; i<cur_intervals-1; i++) {
		wxString s;
		s << cc_data.breaks[i];
		brk_txt[i]->ChangeValue(s);
	}
	int f_sel = field_choice->FindString(cc_data.default_var);
	if (f_sel != wxNOT_FOUND) {
		field_choice->SetSelection(f_sel);
		cur_field_choice = cc_data.default_var;
		if (grid_base->IsColTimeVariant(cc_data.default_var)) {
			field_choice_tm->SetSelection(cc_data.default_var_tm);
			cur_field_choice_tm = cc_data.default_var_tm;
		} else {
			cur_field_choice_tm = 0;
		}
	}
	ShowNumCategories(cur_intervals);
	InitSliderFromBreak(0);
	for (int i=0; i<cur_intervals-1; i++) {
		brk_slider_min[i] = brk_slider_min[0];
		brk_slider_max[i] = brk_slider_max[0];
	}
	UpdateBrkTxtRad(0);
	hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
	Refresh();
}

/** Return true if and only if current category has no interested clients. */
bool CatClassifPanel::IsOkToDelete(const wxString& custom_cat_title)
{
	CatClassifState* ccs =
		cat_classif_manager->FindClassifState(custom_cat_title);
	if (!ccs) return false;
	return ccs->GetNumberObservers() == 0;
}

void CatClassifPanel::CopyFromExisting(
								CatClassification::CatClassifType new_theme)
{
	LOG_MSG("Entering CatClassifPanel::CopyFromExisting");
	// cur_intervals = num_cats
	// new_theme is new theme
	// data is data
	// will set just breaks and also color scheme accordingly
	std::vector<double> new_breaks;
	CatClassification::SetBreakPoints(new_breaks, data, new_theme,
									  cur_intervals);
	if (new_theme != CatClassification::custom &&
		cc_data.color_scheme != CatClassification::custom_color_scheme) {
		if (new_theme == CatClassification::quantile ||
			new_theme == CatClassification::natural_breaks ||
			new_theme == CatClassification::equal_intervals) {
			cc_data.color_scheme = CatClassification::sequential_color_scheme;
			color_scheme->SetSelection(0);
		} else if (new_theme == CatClassification::hinge_15 ||
				   new_theme == CatClassification::hinge_30 ||
				   new_theme == CatClassification::percentile ||
				   new_theme == CatClassification::stddev) {
			cc_data.color_scheme = CatClassification::diverging_color_scheme;
			color_scheme->SetSelection(1);
		} else if (new_theme == CatClassification::unique_values) {
			cc_data.color_scheme = CatClassification::qualitative_color_scheme;
			color_scheme->SetSelection(2);
		}
	}
	cur_intervals = new_breaks.size()+1;
	for (int i=0; i<new_breaks.size(); i++) {
		LOG(new_breaks[i]);
	}
	CatClassification::ChangeNumCats(cur_intervals, cc_data);
	wxString ncs;
	ncs << cur_intervals;
	num_cats_spin_ctrl->SetValue(ncs);
	ShowNumCategories(cur_intervals);
	for (int i=0; i<cur_intervals; i++) {
		cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
	}
	for (int i=0; i<new_breaks.size(); i++) {
		cc_data.breaks[i] = new_breaks[i];
		brk_txt[i]->ChangeValue(GenUtils::DblToStr(new_breaks[i]));
	}

	UpdateBrkSliderRanges();
	int cur_brk_rad = GetActiveBrkRadio();
	if (cur_brk_rad >= cc_data.num_cats-1) {
		brk_rad[cur_brk_rad]->SetValue(0);
		if (cc_data.num_cats-2 >= 0) {
			brk_rad[cc_data.num_cats-2]->SetValue(1);
		} else {
			brk_rad[0]->SetValue(1);
		}
	}
	InitSliderFromBreak(GetActiveBrkRadio());
	UpdateCCState();
	hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
	Refresh();
	
	LOG_MSG("Exiting CatClassifPanel::CopyFromExisting");
}

/** If brk==-1, then disable slider */
void CatClassifPanel::InitSliderFromBreak(int brk)
{
	if (!all_init) return;
	if (brk < 0 || brk > cc_data.num_cats-2) {
		min_lbl->SetLabelText(" ");
		max_lbl->SetLabelText(" ");
		brk_slider->SetValue((brk_slider->GetMax()-brk_slider->GetMin())/2);
		brk_slider->Disable();
		return;
	}
	double min;
	double max;
	if (cc_data.num_cats <= 1) {
		min = data[0].first;
		max = data[num_obs-1].first;
	} else {
		min = cc_data.breaks[0];
		max = cc_data.breaks[cc_data.num_cats-2];
	}
	double range = max-min;
	double sl_min;
	double sl_max;
	bool center = false;
	if (min == 0 && max == 0) {
		sl_min = -1;
		sl_max = 1;
		center = true;
	} else if (range == 0) {
		double mag = min;
		if (mag < 0) mag = -mag;
		sl_min = min - mag/2.0;
		sl_max = min + mag/2.0;
		center = true;
	} else {
		sl_min = min - range/2.0;
		sl_max = max + range/2.0;
	}
	brk_slider_min[brk] = sl_min;
	brk_slider_max[brk] = sl_max;
	min_lbl->SetLabelText(GenUtils::DblToStr(sl_min));
	max_lbl->SetLabelText(GenUtils::DblToStr(sl_max));
	if (center) {
		brk_slider->SetValue((brk_slider->GetMax()-brk_slider->GetMin())/2);
		brk_slider->Enable();
	} else {
		double sl_pos_min = (double) brk_slider->GetMin();
		double sl_pos_max = (double) brk_slider->GetMax();
		double diff = cc_data.breaks[brk] - sl_min;
		double p = sl_pos_min + (sl_pos_max-sl_pos_min)*(diff/(sl_max-sl_min));
		brk_slider->SetValue((int) p);
	}
}

void CatClassifPanel::ChangeNumCats(int new_num_cats)
{
	if (!all_init) return;
	if (new_num_cats < 1) new_num_cats = 1;
	if (new_num_cats > max_intervals) new_num_cats = max_intervals;
	if (new_num_cats == cc_data.num_cats) return;
	int old_num_cats = cc_data.num_cats;
	CatClassification::ChangeNumCats(new_num_cats, cc_data);
	UpdateCCState();
	cur_intervals = new_num_cats;
	ShowNumCategories(new_num_cats);
	for (int i=0; i<cc_data.colors.size(); i++) {
		cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
	}
	for (int i=0; i<cc_data.num_cats; i++) {
		cat_but[i]->SetBackgroundColour(cc_data.colors[i]);
		cat_txt[i]->ChangeValue(cc_data.names[i]);
	}
	for (int i=0; i<cc_data.num_cats-1; i++) {
		brk_txt[i]->ChangeValue(GenUtils::DblToStr(cc_data.breaks[i]));
	}
	int cur_brk_rad = GetActiveBrkRadio();
	if (cur_brk_rad >= cc_data.num_cats-1) {
		brk_rad[cur_brk_rad]->SetValue(0);
		if (cc_data.num_cats-2 >= 0) {
			brk_rad[cc_data.num_cats-2]->SetValue(1);
		} else {
			brk_rad[0]->SetValue(1);
		}
	}
	InitSliderFromBreak(GetActiveBrkRadio());
	UpdateCCState();
	hist_canvas->ChangeBreaks(&cc_data.breaks, &cc_data.colors);
	UpdateBrkSliderRanges();
	Refresh();
}

int CatClassifPanel::GetActiveBrkRadio()
{
	if (!all_init) return -1;
	for (int i=0; i<brk_rad.size(); i++) {
		if (brk_rad[i]->GetValue() != 0) return i;
	}
	return -1;
}

void CatClassifPanel::UpdateBrkTxtRad(int active_brk)
{
	for (int i=0; i<cc_data.breaks.size(); i++) {
		brk_txt[i]->ChangeValue(GenUtils::DblToStr(cc_data.breaks[i]));
	}
	for (int i=0; i<brk_rad.size(); i++) {
		brk_rad[i]->SetValue(0);
	}
	if (active_brk >= 0) brk_rad[active_brk]->SetValue(1);
}

void CatClassifPanel::UpdateBrkSliderRanges()
{
	if (!all_init) return;
	if (cc_data.num_cats-1 <= 0) return;
	double min;
	double max;
	if (cc_data.num_cats <= 1) {
		min = data[0].first;
		max = data[num_obs-1].first;
	} else {
		min = cc_data.breaks[0];
		max = cc_data.breaks[cc_data.num_cats-2];
	}
	double range = max-min;
	double sl_min;
	double sl_max;
	if (min == 0 && max == 0) {
		sl_min = -1;
		sl_max = 1;
	} else if (range == 0) {
		double mag = min;
		if (mag < 0) mag = -mag;
		sl_min = min - mag/2.0;
		sl_max = min + mag/2.0;
	} else {
		sl_min = min - range/2.0;
		sl_max = max + range/2.0;
	}
	for (int i=0; i<cc_data.breaks.size(); i++) {
		brk_slider_min[i] = sl_min;
		brk_slider_max[i] = sl_max;
	}
}

void CatClassifPanel::update(TableState* o)
{
	LOG_MSG("In DataMovieDlg::update(TableState* o)");
	InitFieldChoices();
	InitNewFieldChoice();
}

/** Show / Hide color, category name, and break widget objects
	according to number of passed in categories */
void CatClassifPanel::ShowNumCategories(int num_cats)
{
	if (num_cats < 1 || num_cats > max_intervals) return;
	for (int i=0; i<max_intervals; i++) {
		bool show = i<num_cats;
		cat_but[i]->Show(show);
		cat_txt[i]->Show(show);
	}
	for (int i=0; i<max_intervals-1; i++) {
		bool show = i<num_cats-1;
		brk_rad[i]->Show(show);
		brk_lbl[i]->Show(show);
		brk_txt[i]->Show(show);
	}
}

/** If cc_state !=0, copy over current cc_data values and call
 notifyObservers. */
void CatClassifPanel::UpdateCCState()
{
	if (!cc_state) return;
	cc_state->SetCatClassif(cc_data);
	cc_state->notifyObservers();
}

BEGIN_EVENT_TABLE(CatClassifFrame, TemplateFrame)
	EVT_ACTIVATE(CatClassifFrame::OnActivate)
END_EVENT_TABLE()

CatClassifFrame::CatClassifFrame(wxFrame *parent, Project* project,
								 const wxString& title, const wxPoint& pos,
								 const wxSize& size, const long style)
: TemplateFrame(parent, project, title, pos, size, style)
{
	LOG_MSG("Entering CatClassifFrame::CatClassifFrame");
	wxSplitterWindow* splitter = new wxSplitterWindow(this);
	
	canvas = new CatClassifHistCanvas(splitter, this, project);
	template_canvas = canvas;
	panel = new CatClassifPanel(project, canvas, splitter, wxID_ANY);
	
	canvas->template_frame = this;
	panel->template_frame = this;
	splitter->SplitVertically(panel, canvas);
	splitter->SetMinimumPaneSize(20);
	SetMinSize(wxSize(300,500));
	
	//template_canvas->SetScrollRate(1,1);
	DisplayStatusBar(true);
	SetTitle(template_canvas->GetCanvasTitle());
	Show(true);
	LOG_MSG("Exiting CatClassifFrame::CatClassifFrame");
}

///MMM: Sort out in all Frames: what should be in the destructor?
CatClassifFrame::~CatClassifFrame()
{
	if (HasCapture()) ReleaseMouse();
	DeregisterAsActive();
	frames_manager->removeObserver(this);
}

void CatClassifFrame::OnActivate(wxActivateEvent& event)
{
	LOG_MSG("In CatClassifFrame::OnActivate");
	if (event.GetActive()) {
		RegisterAsActive("CatClassifFrame", GetTitle());
	}
    if ( event.GetActive() && template_canvas ) template_canvas->SetFocus();
}


/** Implementation of FramesManagerObserver interface */
void CatClassifFrame::update(FramesManager* o)
{
	LOG_MSG("In CatClassifFrame::update(FramesManager* o)");
	template_canvas->TitleOrTimeChange();
	//UpdateTitle();
}

void CatClassifFrame::OnThemeless(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::no_theme);
}

void CatClassifFrame::OnHinge15(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_15);
}

void CatClassifFrame::OnHinge30(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::hinge_30);
}

void CatClassifFrame::OnQuantile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::quantile);
}

void CatClassifFrame::OnPercentile(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::percentile);
}

void CatClassifFrame::OnStdDevMap(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::stddev);
}

void CatClassifFrame::OnUniqueValues(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::unique_values);
}

void CatClassifFrame::OnNaturalBreaks(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::natural_breaks);
}

void CatClassifFrame::OnEqualIntervals(wxCommandEvent& event)
{
	ChangeThemeType(CatClassification::equal_intervals);
}

void CatClassifFrame::ChangeThemeType(
							CatClassification::CatClassifType new_theme)
{
	LOG_MSG("In CatClassifFrame::ChangeThemeType");
	panel->CopyFromExisting(new_theme);
}

CatClassifState* CatClassifFrame::PromptNew(const CatClassifDef& ccd,
											const wxString& suggested_title,
											const wxString& field_name,
											int field_tm)
{
	return panel->PromptNew(ccd, suggested_title, field_name, field_tm);
}

void CatClassifFrame::EditExisting(const wxString& cat_classif_title,
								   const wxString& field_name, int field_tm)
{
	panel->EditExisting(cat_classif_title, field_name, field_tm);
}
