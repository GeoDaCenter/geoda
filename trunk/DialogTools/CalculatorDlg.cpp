/**
 * GeoDa TM, Copyright (C) 2011-2014 by Luc Anselin - all rights reserved
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
#include "../ShapeOperations/DbfFile.h"
#include "../DataViewer/DbfTable.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../DataViewer/TimeState.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
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


BEGIN_EVENT_TABLE( CalculatorDlg, wxDialog )
	EVT_CLOSE( CalculatorDlg::OnClose )
	EVT_TEXT( XRCID("ID_EXPRESSION"), CalculatorDlg::OnExprUpdate )
	EVT_CHOICE( XRCID("ID_TARGET"), CalculatorDlg::OnTarget )
	EVT_BUTTON( XRCID("ID_NEW"), CalculatorDlg::OnNew )
	EVT_BUTTON( XRCID("ID_ASSIGN"), CalculatorDlg::OnAssign )
END_EVENT_TABLE()

CalculatorDlg::CalculatorDlg( Project* project_,
	wxWindow* parent,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size, long style)
: project(project_), table_int(0), table_state(0),
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
	CreateControls();
	SetPosition(pos);
	SetTitle(title);
    Centre();
	
	all_init = true;
	
	UpdateQuickParserTable();
}

CalculatorDlg::~CalculatorDlg()
{
	LOG_MSG("In ~CalculatorDlg::CalculatorDlg");
	if (table_state) table_state->removeObserver(this);
}

void CalculatorDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "ID_CALCULATOR_DLG");
	
	expr_t_ctrl = 
		wxDynamicCast(FindWindow(XRCID("ID_EXPRESSION")), wxTextCtrl);
	msg_s_txt =	wxDynamicCast(FindWindow(XRCID("ID_MESSAGE")), wxStaticText);
	target_lbl_s_txt =
		wxDynamicCast(FindWindow(XRCID("ID_TARGET_LBL")), wxStaticText);
	target_choice =
		wxDynamicCast(FindWindow(XRCID("ID_TARGET")), wxChoice);
	new_btn = wxDynamicCast(FindWindow(XRCID("ID_NEW")), wxButton);
	assign_btn = wxDynamicCast(FindWindow(XRCID("ID_ASSIGN")), wxButton);
	all_init = true;
	
	InitTargetChoice(project != 0);
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
	if (!all_init || !project) return;
	ValidateExpression();
	if (!expr_valid) {
		msg_s_txt->SetLabelText("Assign failed: invalid expression");
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
		LOG(val_obs);
	}
	
	GdaParser parser;
	bool parser_success = parser.eval(tokens, &full_parser_table);
	if (!parser_success) {
		wxString s(parser.GetErrorMsg());
		msg_s_txt->SetLabelText(s);
		Refresh();
		return;
	}
	GdaFVSmtPtr v = parser.GetEvalVal();
	
	// Ensure dimensions are compatiable.
	int targ_col = table_int->FindColId(target_choice->GetStringSelection());
	if (targ_col < 0) {
		msg_s_txt->SetLabelText("Error: Target choice not found");
		Refresh();
		return;
	}
	size_t targ_tms = table_int->GetColTimeSteps(targ_col);
	std::valarray<double>& V = (*v).GetValArrayRef();
	size_t V_tms = (*v).GetTms();
	size_t V_obs = (*v).GetObs();
	if (targ_tms < V_tms) {
		msg_s_txt->SetLabelText("Error: Target has too few time periods.");
		Refresh();
		return;
	}
	size_t V_sz = V.size();
	double V_first = V[0];
	LOG(V_tms);
	LOG(V_obs);
	LOG(V_first);
	size_t obs = table_int->GetNumberRows();
	std::vector<double> t_vec(obs);
	std::vector<bool> undefined(obs);
	if (V_obs == 1) {
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
	Refresh();
	return;
}

void CalculatorDlg::ConnectToProject(Project* project_)
{
	LOG_MSG("In CalculatorDlg::ConnectToProject");
	project = project_;
	table_state = project->GetTableState();
	table_state->registerObserver(this);
	table_int = project->GetTableInt();
	InitTargetChoice(true);
}

void CalculatorDlg::DisconnectFromProject()
{
	LOG_MSG("In CalculatorDlg::DisconnectFromProject");
	project = 0;
	if (table_state) table_state->removeObserver(this);
	table_state = 0;
	table_int = 0;
	InitTargetChoice(false);
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
		Refresh();
		return;
	}

	if (lexer_success = lexer.Tokenize(expr_str, tokens)) {
		msg_s_txt->SetLabelText("Lexer success");
	} else {
		msg_s_txt->SetLabelText(lexer.GetErrorMsg());
	}
	
	LOG_MSG("Tokens from lexer");
	for (size_t i=0, sz=tokens.size(); i<sz; ++i) {
		LOG_MSG("token: " + tokens[i].ToStr());
		if (tokens[i].token == Gda::NUMBER) {
			expr_t_ctrl->SetStyle(tokens[i].start_ind,
								  tokens[i].end_ind,
								  attr_num);
		}
	}
	
	GdaParser parser;
	bool parser_success = parser.eval(tokens, &quick_parser_table);
	if (lexer_success & !parser_success) {
		wxString s(parser.GetErrorMsg());
		msg_s_txt->SetLabelText(s);
	}
	
	LOG_MSG("Tokens from parser");
	eval_toks = parser.GetEvalTokens();
	for (size_t i=0; i<eval_toks.size(); ++i) {
		LOG_MSG("eval token: " + tokens[i].ToStr());
		if (tokens[i].token == Gda::NAME && eval_toks[i].is_ident) {
			active_ident_set.insert(eval_toks[i].string_value);
			LOG_MSG(eval_toks[i].string_value);
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
	expr_valid = true;
	
	Refresh();
}

bool CalculatorDlg::IsTargSel()
{
	if (!all_init) return false;
	return (target_choice->GetSelection() >= 0);
}

