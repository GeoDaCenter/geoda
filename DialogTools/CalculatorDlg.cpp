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

#include <algorithm>
#include <set>
#include <boost/foreach.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "../FramesManager.h"


#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../DataViewer/TimeState.h"
#include "../VarCalc/CalcHelp.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../Project.h"
#include "CalculatorDlg.h"

/*
 Here is the continuous process that will take place:
 1. User modifies expr_t_ctrl
 2. OnExprUpdate is called
 3. Tokenize is called
 4. If an exception is caught, message displayed
 5. If there was an exception, we still want to identify function tokens
   and identifier tokens, so, do the following
   a) Call Parser in identify mode: parser will evaluate, subsituting
      0 values for identifiers, but annotating identifer and function tokens.
   b) parser will proceed until an exeption is thrown
 6. Regardless of exceptions, use info returned from Parser to highlight
    expression text with approriate colors
 7. If exception thrown from lexer, show message
 8. If exception thrown from parser but not lexer, show that message
 9. If no exception thown, then attempt re-evaluate with proper values
    subsituted in for identifiers.  Just the first lines of the table.
 10. If no exception thrown still, then display text in preview and
    enable Assign button
 11. Otherwise disable Assign button.
 
 When Assign button is pressed, run the above process again with the
 additional final step of substituting full table values.
 
 If successful, then show preview along with success message.
 
 */

CalculatorDlg::CalculatorDlg( Project* project_,
	wxWindow* parent,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size, long style)
: wxDialog(parent, wxID_ANY, title, pos, size,
		   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER),
project(project_), table_int(0), table_state(0),
expr_valid(false),
attr_num(wxColour(0,40,200)),
attr_func(*wxBLACK),
attr_unknown_func(*wxRED),
attr_ident(wxColour(0,100,0)),
attr_unknown_ident(*wxRED),
all_init(false)
{
	//attr_ident.SetFontUnderlined(true);
	//attr_unknown_func.SetFontStyle(wxFONTSTYLE_ITALIC);
	//attr_func.SetFontStyle(wxFONTSTYLE_ITALIC);
	if (project) {
		table_int = project->GetTableInt();
		table_state = project->GetTableState();
		table_state->registerObserver(this);
	}
	
	Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler(CalculatorDlg::OnClose) );
	
	expr_t_ctrl = new wxTextCtrl(this, XRCID("ID_EXPRESSION"), wxEmptyString,
								 wxDefaultPosition, wxSize(200, 20));
	
	Connect(XRCID("ID_EXPRESSION"), wxEVT_TEXT,
			wxCommandEventHandler(CalculatorDlg::OnExprUpdate));
	
	wxBoxSizer* sub_top_h_szr_1 = new wxBoxSizer(wxHORIZONTAL);
	sub_top_h_szr_1->Add(expr_t_ctrl, 1, wxEXPAND);
	
	// Create widgets for second horizontal sizer
	select_lbl_s_txt = new wxStaticText(this, XRCID("ID_SELECT_LBL"),
										"Make selection from expression");
	select_btn = new wxButton(this, XRCID("ID_SELECT"), "Select",
							  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	Connect(XRCID("ID_SELECT"), wxEVT_BUTTON,
			wxCommandEventHandler(CalculatorDlg::OnSelect));
	
	wxBoxSizer* sub_top_h_szr_2 = new wxBoxSizer(wxHORIZONTAL);
	sub_top_h_szr_2->Add(select_lbl_s_txt, 0, wxALIGN_CENTER_VERTICAL);
	sub_top_h_szr_2->AddSpacer(3);
	sub_top_h_szr_2->Add(select_btn, 0, wxALIGN_CENTER_VERTICAL);
	
	// Create widgets for third horizontal sizer
	target_lbl_s_txt = new wxStaticText(this, XRCID("ID_TARGET_LBL"), "Target");
	target_choice = new wxChoice(this, XRCID("ID_TARGET"), wxDefaultPosition,
								 wxSize(100,-1));
	new_btn = new wxButton(this, XRCID("ID_NEW"), "New",
						   wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	assign_btn = new wxButton(this, XRCID("ID_ASSIGN"), "Assign To Target",
							  wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	
	Connect(XRCID("ID_TARGET"), wxEVT_CHOICE,
			wxCommandEventHandler(CalculatorDlg::OnTarget));
	
	Connect(XRCID("ID_NEW"), wxEVT_BUTTON,
			wxCommandEventHandler(CalculatorDlg::OnNew));
	Connect(XRCID("ID_ASSIGN"), wxEVT_BUTTON,
			wxCommandEventHandler(CalculatorDlg::OnAssign));
	
	wxBoxSizer* sub_top_h_szr_3 = new wxBoxSizer(wxHORIZONTAL);
	sub_top_h_szr_3->Add(target_lbl_s_txt, 0, wxALIGN_CENTER_VERTICAL);
	sub_top_h_szr_3->AddSpacer(5);
	sub_top_h_szr_3->Add(target_choice, 0, wxALIGN_CENTER_VERTICAL);
	sub_top_h_szr_3->AddSpacer(5);
	sub_top_h_szr_3->Add(new_btn, 0, wxALIGN_CENTER_VERTICAL);
	sub_top_h_szr_3->AddSpacer(5);
	sub_top_h_szr_3->Add(assign_btn, 0, wxALIGN_CENTER_VERTICAL);
	
	
	
	
	// function help tree
	func_help_tree = new wxTreeCtrl(this, XRCID("ID_FUNC_HELP_TREE"),
									wxDefaultPosition, wxSize(200,100),
									wxTR_HIDE_ROOT|wxTR_HAS_BUTTONS);
	InitFuncHelpTree();
	Connect(XRCID("ID_FUNC_HELP_TREE"), wxEVT_TREE_SEL_CHANGED,
			wxTreeEventHandler(CalculatorDlg::OnFuncHelpSel));
	Connect(XRCID("ID_FUNC_HELP_TREE"), wxEVT_TREE_ITEM_ACTIVATED,
			wxTreeEventHandler(CalculatorDlg::OnFuncHelpDClick));
	// function help window
	//html_win = new wxHtmlWindow(this);
	html_win = wxWebView::New(this, XRCID("ID_HTML_WIN"));
	html_win->EnableContextMenu(false);
	
	wxString s;
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<body>";
	s << h_title("Operator Help", 1);
	s << "</body>";
	s << "</html>";
    //html_win->SetPage(s);
	html_win->SetPage(s,"");
	
	wxBoxSizer* help_h_sizer = new wxBoxSizer(wxHORIZONTAL);
	help_h_sizer->Add(func_help_tree, 3,
					  wxEXPAND|wxALIGN_CENTER_HORIZONTAL);
	help_h_sizer->AddSpacer(10);
	help_h_sizer->Add(html_win, 6,
					  wxEXPAND|wxALIGN_CENTER_HORIZONTAL);

	// Left Sub-Top Sizer
	wxBoxSizer* left_sub_top_v_szr = new wxBoxSizer(wxVERTICAL);
	left_sub_top_v_szr->Add(help_h_sizer, 10, wxEXPAND|wxALL, 8);
	
	
	left_sub_top_v_szr->Add(new wxStaticText(this, wxID_ANY, "Expression"), 0,
							 wxTOP|wxLEFT|wxRIGHT, 10);
	
	left_sub_top_v_szr->Add(sub_top_h_szr_1, 3, wxALL|wxEXPAND, 8);
	left_sub_top_v_szr->AddSpacer(3);
	
	msg_s_txt = new wxStaticText(this, XRCID("ID_MESSAGE"), "");
	left_sub_top_v_szr->Add(msg_s_txt, 0, wxLEFT|wxRIGHT, 10);
	left_sub_top_v_szr->AddSpacer(5);
	
	left_sub_top_v_szr->Add(sub_top_h_szr_2, 0, wxALL, 8);
	left_sub_top_v_szr->AddSpacer(5);
	
	left_sub_top_v_szr->Add(sub_top_h_szr_3, 0, wxALL, 8);

	// Right Sub-Top Sizer
	//wxBoxSizer* right_sub_top_v_szr = new wxBoxSizer(wxVERTICAL);
	// Windows for right sub-top sizer
	//right_sub_top_v_szr->Add(func_help_tree, 2,
	//						 wxEXPAND|wxALIGN_CENTER_VERTICAL);
	//right_sub_top_v_szr->AddSpacer(10);
	//right_sub_top_v_szr->Add(html_win, 3,
	//						 wxEXPAND|wxALIGN_CENTER_VERTICAL);
					  
		
	// Sub-Top Sizer
	wxBoxSizer* sub_top_h_szr = new wxBoxSizer(wxHORIZONTAL);
	sub_top_h_szr->Add(left_sub_top_v_szr, 1, wxEXPAND|wxALL, 8);
	//sub_top_h_szr->Add(right_sub_top_v_szr, 5, wxEXPAND|wxALL, 8);
	
	// Top Sizer
	wxBoxSizer* top_v_szr = new wxBoxSizer(wxVERTICAL);
	top_v_szr->Add(sub_top_h_szr, 1, wxEXPAND);
	
	SetSizerAndFit(top_v_szr);

	
	all_init = true;
	select_btn->Enable(false);
	select_lbl_s_txt->Enable(project != 0);
	InitTargetChoice(project != 0);
	
	SetPosition(pos);
	SetTitle(title);
    Centre();
	
	all_init = true;
	wxTreeEvent fh_event;
	OnFuncHelpSel(fh_event);
	
	UpdateQuickParserTable();
}

CalculatorDlg::~CalculatorDlg()
{
	if (table_state) table_state->removeObserver(this);
}

void CalculatorDlg::OnClose(wxCloseEvent& e)
{
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

void CalculatorDlg::OnExprUpdate(wxCommandEvent& e)
{
	ValidateExpression();
}

void CalculatorDlg::OnTarget(wxCommandEvent& e)
{
	if (!all_init) return;
	assign_btn->Enable(expr_valid && IsTargSel());
}

wxString CalculatorDlg::h_title(const wxString& title, int level)
{
	wxString h = "h";
	if (level < 1) {
		h << "1";
	} else if (level > 4) {
		h << "4";
	} else {
		h << level;
	}
	wxString s;
	s << "<" << h << ">";
	s << "<span style=\"color:blue\">" << title << "</span>";
	s << "</" << h << ">";
	return s;
}

void CalculatorDlg::OnFuncHelpSel(wxTreeEvent& ev)
{
	if (!all_init) return;
	
	wxString s;
	
	/*
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<head>";
	s << "  <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\"/>";
	s << "  <title>D3 Hello World</title>";
	s << "  <script type=\"text/javascript\" src=\"http://mbostock.github.com/d3/d3.js?2.4.5\"></script>";
	s << "</head>";
	s << "<body>";
	s << "  <script type=\"text/javascript\">";
	s << "\n";
	s << "d3.select(\"body\").append(\"span\").text(\"Hello, world!\");";
	s << "\n";
	s << "</script>";
	s << "</body>";
	s << "</html>";
	html_win->SetPage(s, "");
	return;
	*/
	
	wxTreeItemId id = func_help_tree->GetSelection();
	wxString sel = func_help_tree->GetItemText(id);
	CalcHelpEntry e = CalcHelp::GetEntry(sel);
	
	s << "<!DOCTYPE html>";
	s << "<html>";
	s << "<body>";	
	if (sel == "") {
		s << h_title("Operator Help", 1);
		// MathML HTML5 example
		/*
		s << "<math>";
		s <<   "<mrow>";
		s <<     "<mrow>";
		s <<       "<msup>";
		s <<         "<mi>a</mi>";
		s <<         "<mn>2</mn>";
		s <<       "</msup>";
		s <<       "<mo>+</mo>";
		s <<       "<msup>";
		s <<         "<mi>b</mi>";
		s <<         "<mn>2</mn>";
		s <<       "</msup>";		
		s <<     "</mrow>";
		s <<     "<mo>=</mo>";
		s <<     "<msup>";
		s <<       "<mi>c</mi>";
		s <<       "<mn>2</mn>";
		s <<     "</msup>";
		s <<   "</mrow>";
		s << "</math>";
		*/
	} else if (sel == "Math" || sel == "Statistics" || sel == "Ordering" ||
			   sel == "Numerical Tests" || sel == "Operators" ||
			   sel == "Weights" || sel == "Rates") {
		s << h_title(sel, 1);
		s << "<p>Expand branch to see list of operators.</p>";
	} else if (sel != "" && e.func == "") {
		s << h_title(sel, 1);
		s << "<p>" << "No help entry for " << sel << "</p>";
	} else {
		s << h_title(e.func, 1);
		s << "<p>" << e.desc << "</p>";
		if (e.syn_args.size() > 0) {
			s << h_title("Syntax", 3);
			if (e.infix && e.syn_args.size() == 1) {
				s << "<p>" << e.func << " " << e.syn_args[0].arg << "</p>";
				if (e.alt_func != "") {
					s << "<p>" << e.alt_func << " " << e.syn_args[0].arg << "</p>";
				}
			} else if (e.infix) {
				s << "<p>";
				for (size_t i=0; i<e.syn_args.size(); ++i) {
					s << "<i>" << e.syn_args[i].arg << "</i>";
					if (i < e.syn_args.size()-1) s << " " << e.func << " ";
				}
				s << "</p>";
				if (e.alt_func != "") {
					s << "<p><i>or</i></p>";
					s << "<p>";
					for (size_t i=0; i<e.syn_args.size(); ++i) {
						s << "<i>" << e.syn_args[i].arg << "</i>";
						if (i < e.syn_args.size()-1) {
							s << " " << e.alt_func << " ";
						}
					}
					s << "</p>";
				}
			} else {
				s << "<p>";
				s << e.func << "(";
				for (size_t i=0; i<e.syn_args.size(); ++i) {
					s << "<i>" << e.syn_args[i].arg << "</i>";
					if (i < e.syn_args.size()-1) s << ", ";
				}
				s << ")</p>";
				if (e.alt_func != "") {
					s << "<p><i>or</i></p>";
					s << "<p>";
					s << e.alt_func << "(";
					for (size_t i=0; i<e.syn_args.size(); ++i) {
						s << "<i>" << e.syn_args[i].arg << "</i>";
						if (i < e.syn_args.size()-1) s << ", ";
					}
					s << ")</p>";
				}
			}
		}
		if (e.args_desc.size() > 0) {
			s << h_title("Parameters", 3);
			for (size_t i=0; i<e.args_desc.size(); ++i) {
				s << "<p><i>" << e.args_desc[i].k <<"</i> &rarr; ";
				s << e.args_desc[i].v << "</p>";
			}
		}
		if (e.exs.size() > 0) {
			s << h_title("Examples", 3);
			for (size_t i=0; i<e.exs.size(); ++i) {
				s << "<p>";
				if (e.exs[i].k != "") s << e.exs[i].k;
				if (e.exs[i].k != "" && e.exs[i].v != "") s << " &rarr; ";
				if (e.exs[i].v != "") s << e.exs[i].v;
				s << "</p>";
			}
		}
	}
	s << "</body>";
	s << "</html>";
	//html_win->SetPage(s);
    html_win->SetPage(s,"");
}

void CalculatorDlg::OnFuncHelpDClick(wxTreeEvent& ev)
{
	if (!all_init) return;
}

void CalculatorDlg::OnNew(wxCommandEvent& e)
{
	if (!all_init || !project) return;
	DataViewerAddColDlg dlg(project, this);
	if (dlg.ShowModal() != wxID_OK) return;
	wxString sel_str = dlg.GetColName();
	InitTargetChoice(true);
	int new_sel = target_choice->FindString(sel_str);
	if (new_sel >= 0) {
		target_choice->SetSelection(new_sel);
	}
	assign_btn->Enable(expr_valid && IsTargSel());
}

void CalculatorDlg::OnAssign(wxCommandEvent& e)
{
	AssignOrSelect(true);
}

void CalculatorDlg::OnSelect(wxCommandEvent& e)
{
	AssignOrSelect(false);
}

void CalculatorDlg::AssignOrSelect(bool assign)
{
	bool select = !assign;
	
	if (!all_init || !project) return;
	ValidateExpression();
	if (!expr_valid) {
		msg_s_txt->SetLabelText("Error: invalid expression");
		Refresh();
		return;
	}
	// we will now do a full evaluation and will print out
	// preview values.
	full_parser_table.clear();
	std::set<wxString>::iterator it;
	for (it = active_ident_set.begin(); it!=active_ident_set.end(); ++it) {
		int col = table_int->FindColId(*it);
		if (col < 0) continue;
		GdaFVSmtPtr val(new GdaFlexValue());
		table_int->GetColData(col, (*val));
		full_parser_table[*it] = val;
		size_t val_obs = (*val).GetObs();
	}
	
	GdaParser parser;
	WeightsManInterface* wmi = 0;
	if (project && project->GetWManInt()) wmi = project->GetWManInt();
	bool parser_success = parser.eval(tokens, &full_parser_table, wmi);
	if (!parser_success) {
		wxString s(parser.GetErrorMsg());
		msg_s_txt->SetLabelText(s);
		Refresh();
		return;
	}
	GdaFVSmtPtr v = parser.GetEvalVal();
	
	int targ_col = -1;
	size_t targ_tms = -1;
	if (assign) {
		// Ensure dimensions are compatiable.
		targ_col = table_int->FindColId(target_choice->GetStringSelection());
		if (targ_col < 0) {
			msg_s_txt->SetLabelText("Error: Target choice not found");
			Refresh();
			return;
		}
		targ_tms = table_int->GetColTimeSteps(targ_col);
	}
	std::valarray<double>& V = (*v).GetValArrayRef();
	size_t V_tms = (*v).GetTms();
	size_t V_obs = (*v).GetObs();
	if (assign && targ_tms < V_tms) {
		msg_s_txt->SetLabelText("Error: Target has too few time periods.");
		Refresh();
		return;
	}
	size_t V_sz = V.size();
	double V_first = V[0];

	size_t obs = table_int->GetNumberRows();
	
	if (assign) {
		std::vector<double> t_vec(obs);
		std::vector<bool> undefined(obs);
		// MMM must consider case of [1 2 3 4 5 ... tms]
		if (V_obs == 1 && V_tms == 1) {
			double val = (*v).GetDouble();
			bool undef = false;
			if (!Gda::IsFinite(val)) {
				val = 0;
				undef = true;
			}
			for (size_t i=0; i<obs; ++i) {
				t_vec[i] = val;
				undefined[i] = undef;
			}
		} else if (V_tms == 1) {
			// fill t_vec from V.
			for (size_t i=0; i<obs; ++i) {
				double val = V[i];
				if (Gda::IsFinite(val)) {
					t_vec[i] = val;
					undefined[i] = false;
				} else {
					t_vec[i] = 0;
					undefined[i] = true;
				}
			}
		}
		for (size_t t=0; t<targ_tms; ++t) {
			if (V_tms > 1) {
				// fill t_vec from V for each time period.
				//std::slice sl(t,obs,Vtms);
				for (size_t i=0; i<obs; ++i) {
					double val = V[t+i*V_tms];
					if (Gda::IsFinite(val)) {
						t_vec[i] = val;
						undefined[i] = false;
					} else {
						t_vec[i] = 0;
						undefined[i] = true;
					}
				}
			}
			table_int->SetColData(targ_col, t, t_vec);
			table_int->SetColUndefined(targ_col, t, undefined);
		}
		
		
		wxString s("Success. First obs. and time value = ");
		s << (*v).GetDouble();
		msg_s_txt->SetLabelText(s);
	} else {
		int t=0;
		wxString t_str = "";
		if (V_tms > 1) {
			// will use current time period
			t = project->GetTimeState()->GetCurrTime();
			t_str = project->GetTimeState()->GetCurrTimeString();
		}
		int num_obs_sel = 0;
		vector<bool> selected(obs);
		if (V_obs == 1) {
			bool val = V[t]!=0 ? true : false;
			for (size_t i=0; i<obs; ++i) {
				selected[i] = val;
			}
			if (val) num_obs_sel = obs;
		} else {
			// fill t_vec from V for time period t.
			for (size_t i=0; i<obs; ++i) {
				selected[i] = V[t+i*V_tms]!=0 ? true : false;
				if (selected[i]) ++num_obs_sel;
			}
		}
		
		HighlightState& hs = *project->GetHighlightState();
		std::vector<bool>& h = hs.GetHighlight();
        bool selection_changed = false;
        
		for (size_t i=0; i<obs; i++) {
			bool sel = selected[i];
			if (sel && !h[i]) {
                h[i] = true;
                selection_changed = true;
			} else if (!sel && h[i]) {
                h[i] = false;
                selection_changed = true;

			}
		}
		hs.SetEventType(HLStateInt::delta);
		hs.notifyObservers();
		
		wxString s;
		s << num_obs_sel << " observation" << (num_obs_sel != 1 ? "s" : "");
		s << " selected";
		if (V_tms == 1) {
			s << ".";
		} else {
			s << " for time period " << t_str << " of result.";
		}
		msg_s_txt->SetLabelText(s);
	}
	Refresh();
}

void CalculatorDlg::ConnectToProject(Project* project_)
{
	project = project_;
	table_state = project->GetTableState();
	table_state->registerObserver(this);
	table_int = project->GetTableInt();
	InitTargetChoice(true);
	select_lbl_s_txt->Enable(true);
	UpdateQuickParserTable();
	ValidateExpression();
}

void CalculatorDlg::DisconnectFromProject()
{
	project = 0;
	if (table_state) table_state->removeObserver(this);
	table_state = 0;
	table_int = 0;
	InitTargetChoice(false);
	select_lbl_s_txt->Enable(false);
	select_btn->Enable(false);
	msg_s_txt->SetLabelText("");
	UpdateQuickParserTable();
	ValidateExpression();
}

void CalculatorDlg::update(TableState* o)
{
	TableState::EventType ev_type = o->GetEventType();
	if (ev_type == TableState::empty ) return;
	InitTargetChoice(true);
	UpdateQuickParserTable();
	ValidateExpression();
}

void CalculatorDlg::InitTargetChoice(bool enable)
{
	if (!all_init) return;
	if (!table_int) enable = false;
	target_lbl_s_txt->Enable(enable);
	target_choice->Enable(enable);
	new_btn->Enable(enable);
	assign_btn->Enable(enable && expr_valid && IsTargSel());
	if (!enable) {
		target_choice->Clear();
		return;
	}
	// rember existing choice
	wxString cur_choice = target_choice->GetStringSelection();
	target_choice->Clear();
	std::vector<wxString> names;
	table_int->FillNumericNameList(names);
	for (size_t i=0; i<names.size(); i++) {
		target_choice->Append(names[i]);
	}
	// restore selection if possible
	target_choice->SetSelection(target_choice->
								FindString(cur_choice));
}

void CalculatorDlg::InitFuncHelpTree()
{
	func_help_tree->DeleteAllItems();
	wxTreeItemId rt_id = func_help_tree->AddRoot("");

	wxTreeItemId ops_id = func_help_tree->AppendItem(rt_id, "Operators");
	func_help_tree->AppendItem(ops_id, "+");
	func_help_tree->AppendItem(ops_id, "-");
	func_help_tree->AppendItem(ops_id, "*");
	func_help_tree->AppendItem(ops_id, "/");
	func_help_tree->AppendItem(ops_id, "^");
	func_help_tree->AppendItem(ops_id, "pow");
	func_help_tree->AppendItem(ops_id, "=");
	func_help_tree->AppendItem(ops_id, "<>");
	func_help_tree->AppendItem(ops_id, "<");
	func_help_tree->AppendItem(ops_id, "<=");
	func_help_tree->AppendItem(ops_id, ">");
	func_help_tree->AppendItem(ops_id, ">=");
	func_help_tree->AppendItem(ops_id, "AND");
	func_help_tree->AppendItem(ops_id, "OR");
	func_help_tree->AppendItem(ops_id, "XOR");
	func_help_tree->AppendItem(ops_id, "NOT");
	//func_help_tree->Expand(ops_id);
	
	wxTreeItemId math_id = func_help_tree->AppendItem(rt_id, "Math");
	func_help_tree->AppendItem(math_id, "sqrt");
	func_help_tree->AppendItem(math_id, "sin");
	func_help_tree->AppendItem(math_id, "cos");
	func_help_tree->AppendItem(math_id, "tan");
	func_help_tree->AppendItem(math_id, "asin");
	func_help_tree->AppendItem(math_id, "acos");
	func_help_tree->AppendItem(math_id, "atan");
	func_help_tree->AppendItem(math_id, "abs");
	func_help_tree->AppendItem(math_id, "ceil");
	func_help_tree->AppendItem(math_id, "floor");
	func_help_tree->AppendItem(math_id, "round");
	func_help_tree->AppendItem(math_id, "log");
	func_help_tree->AppendItem(math_id, "log10");
	func_help_tree->AppendItem(math_id, "log2");
	
	wxTreeItemId stat_id = func_help_tree->AppendItem(rt_id, "Statistics");
	func_help_tree->AppendItem(stat_id, "mean");
	//func_help_tree->AppendItem(stat_id, "median");
	func_help_tree->AppendItem(stat_id, "max");
	func_help_tree->AppendItem(stat_id, "min");
	func_help_tree->AppendItem(stat_id, "sum");
	func_help_tree->AppendItem(stat_id, "stddev");
	func_help_tree->AppendItem(stat_id, "dev_fr_mean");
	func_help_tree->AppendItem(stat_id, "standardize");
	func_help_tree->AppendItem(stat_id, "unif_dist");
	func_help_tree->AppendItem(stat_id, "norm_dist");
	func_help_tree->AppendItem(stat_id, "enumerate");

	wxTreeItemId ord_id = func_help_tree->AppendItem(rt_id, "Ordering");
	func_help_tree->AppendItem(ord_id, "shuffle");
	func_help_tree->AppendItem(ord_id, "rot_down");
	func_help_tree->AppendItem(ord_id, "rot_up");

	wxTreeItemId nt_id = func_help_tree->AppendItem(rt_id, "Numerical Tests");
	func_help_tree->AppendItem(nt_id, "is_defined");
	func_help_tree->AppendItem(nt_id, "is_finite");
	func_help_tree->AppendItem(nt_id, "is_nan");
	func_help_tree->AppendItem(nt_id, "is_pos_inf");
	func_help_tree->AppendItem(nt_id, "is_neg_inf");
	func_help_tree->AppendItem(nt_id, "is_inf");

	wxTreeItemId w_id = func_help_tree->AppendItem(rt_id, "Weights");
	func_help_tree->AppendItem(w_id, "counts");
	func_help_tree->AppendItem(w_id, "lag");
	
	wxTreeItemId rates_id = func_help_tree->AppendItem(rt_id, "Rates");
	func_help_tree->AppendItem(rates_id, "raw_rate");
	func_help_tree->AppendItem(rates_id, "excess_risk");
	func_help_tree->AppendItem(rates_id, "emp_bayes");
	func_help_tree->AppendItem(rates_id, "spatial_rate");
	func_help_tree->AppendItem(rates_id, "sp_emp_bayes");
	func_help_tree->AppendItem(rates_id, "emp_bayes_rt_std");
	
	//func_help_tree->ExpandAll();
}

void CalculatorDlg::UpdateQuickParserTable()
{
	quick_parser_table.clear();
	if (!table_int) return;
	std::vector<wxString> names;
	table_int->FillNumericNameList(names);
	for (size_t i=0; i<names.size(); i++) {
		GdaFVSmtPtr val(new GdaFlexValue(1));	
		quick_parser_table[names[i].ToStdString()] = val;
	}
}

void CalculatorDlg::UpdateFullParserTable()
{
	full_parser_table.clear();
}

void CalculatorDlg::ValidateExpression()
{
	if (!all_init) return;
	expr_valid = false;
	active_ident_set.clear();
	eval_toks.clear();
	bool lexer_success = false;
	wxString expr_str = expr_t_ctrl->GetValue();
	if (expr_t_ctrl->GetValue().IsEmpty()) {
		msg_s_txt->SetLabelText("");
		assign_btn->Enable(false);
		select_btn->Enable(false);
		Refresh();
		return;
	}

	if (lexer_success = lexer.Tokenize(expr_str, tokens)) {
		msg_s_txt->SetLabelText("Lexer success");
	} else {
		msg_s_txt->SetLabelText(lexer.GetErrorMsg());
	}
	

	for (size_t i=0, sz=tokens.size(); i<sz; ++i) {

		if (tokens[i].token == Gda::NUMBER) {
			expr_t_ctrl->SetStyle(tokens[i].start_ind,
								  tokens[i].end_ind,
								  attr_num);
		}
	}
	
	GdaParser parser;
	WeightsManInterface* wmi = 0;
	if (project && project->GetWManInt()) wmi = project->GetWManInt();
	bool parser_success = parser.eval(tokens, &quick_parser_table, wmi);
	if (lexer_success & !parser_success) {
		wxString s(parser.GetErrorMsg());
		msg_s_txt->SetLabelText(s);
	}
	

	eval_toks = parser.GetEvalTokens();
	for (size_t i=0; i<eval_toks.size(); ++i) {

		if (tokens[i].token == Gda::NAME && eval_toks[i].is_ident) {
			active_ident_set.insert(eval_toks[i].string_value);

			expr_t_ctrl->SetStyle(eval_toks[i].start_ind,
								  eval_toks[i].end_ind,
								  eval_toks[i].problem_token ?
								  attr_unknown_ident : attr_ident);
		} else if (tokens[i].token == Gda::NAME && tokens[i].is_func) {
			expr_t_ctrl->SetStyle(eval_toks[i].start_ind,
								  eval_toks[i].end_ind,
								  eval_toks[i].problem_token ?
								  attr_unknown_func : attr_func);
		}
	}
	
	if (!lexer_success || !parser_success) {
		assign_btn->Enable(false);
		select_btn->Enable(false);
		Refresh();
		return;
	}
	
	if (active_ident_set.size() == 0) {
		GdaFVSmtPtr p_val = parser.GetEvalVal();
		wxString s;
		s << (*p_val).GetDouble();
		msg_s_txt->SetLabelText(s);
	} else {
		msg_s_txt->SetLabelText("valid expression");
	}
	assign_btn->Enable(true && IsTargSel());
	select_btn->Enable(true && project);
	expr_valid = true;
	
	Refresh();
}

bool CalculatorDlg::IsTargSel()
{
	if (!all_init) return false;
	return (target_choice->GetSelection() >= 0);
}
