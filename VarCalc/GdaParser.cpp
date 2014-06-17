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

#include <math.h>
#include "../logger.h"
#include "GdaParser.h"

GdaParser::GdaParser()
{
}

bool GdaParser::eval(const std::vector<GdaTokenDetails>& tokens_,
					 std::map<wxString, GdaFVSmtPtr> * table_)
{
	tokens = tokens_;
	table = table_;
	tok_i = 0;
	bool success = false;
	eval_toks.clear();
	try {
		error_msg = "";
		eval_val = expression();
		success = true;
	}
	catch (GdaParserException e) {
		error_msg = e.what();
	}
	catch (std::exception e) {
		error_msg = e.what();
	}
	return success;
}

GdaFVSmtPtr GdaParser::expression()
{
	using namespace std;
	GdaFVSmtPtr left = mult_expr();
	
	for (;;) {
		if (curr_token() == Gda::PLUS) {
			inc_token(); // consume '+'
			GdaFVSmtPtr right = mult_expr();
			wxString ss;
			ss << "GdaParser + op: operand " << (*left).ToStr();
			ss << ", operand " << (*right).ToStr();
			LOG_MSG(ss);
			(*left) += (*right);
		} else if (curr_token() == Gda::MINUS) {
			inc_token(); // consume '-'
			(*left) -= (*mult_expr());
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::mult_expr()
{
	GdaFVSmtPtr left = pow_expr();
	
	for (;;) {
		if (curr_token() == Gda::MUL) {
			inc_token(); // consume '*'
			(*left) *= (*pow_expr());
		} else if (curr_token() == Gda::DIV) {
			inc_token(); // consume '/'
			(*left) /= (*pow_expr());
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::pow_expr()
{
	GdaFVSmtPtr left = func_expr();
	if (curr_token() == Gda::POW) {
		inc_token(); // consume '^'
		GdaFVSmtPtr right = expression();
		(*left) ^= (*right);
		return left;
	} else {
		return left;
	}
}

GdaFVSmtPtr GdaParser::func_expr()
{
	if (curr_token() != Gda::NAME ||
		(curr_token() == Gda::NAME && next_token() != Gda::LP)) {
		return primary();
	}
	wxString func_name = curr_tok_str_val();
	mark_curr_token_func();
	inc_token(); // consume NAME token
	mark_curr_token_func();
	inc_token(); // consume '('
	if (curr_token() == Gda::RP) {
		mark_curr_token_func();
		inc_token(); // consume ')'
		// return 0-ary function NAME () value
		GdaFVSmtPtr p(new GdaFlexValue(0));
		return p;
	}
	GdaFVSmtPtr arg1(expression()); // evaluate first argument
	if (curr_token() == Gda::RP) {
		mark_curr_token_func();
		inc_token(); // consume ')'
		// evaluate unary function NAME ( arg1 )
		//GdaFVSmtPtr p(new GdaFlexValue(arg1));
		//return p;
		return arg1;
	}
	if (curr_token() != Gda::COMMA) {
		throw GdaParserException("',' or ')' expected");
	}
	inc_token(); // consume ','
	GdaFVSmtPtr arg2(expression()); // evaluate second argument
	if (curr_token() == Gda::RP) {
		mark_curr_token_func();
		inc_token(); // consume ')'
		// evaluate binary function NAME ( arg1 , arg2 )
		//GdaFVSmtPtr p(new GdaFlexValue(2));
		//return p;
		return arg2;
	}
	throw GdaParserException("')' expected");
}

GdaFVSmtPtr GdaParser::primary()
{	
	if (curr_token() == Gda::NUMBER) {
		GdaFVSmtPtr p(new GdaFlexValue(curr_tok_num_val()));
		//double v = curr_tok_num_val();
		inc_token(); // consume NUMBER token
		return p;
	} else if (curr_token() == Gda::NAME) {
		wxString key(curr_tok_str_val());
		//if (next_token() == Gda::ASSIGN) {
		//	(*table)[key] = expression(true);
		//}
		// check for existence in table first!
		if (table->find(key) == table->end()) {
			mark_curr_token_ident();
			mark_curr_token_problem();
			inc_token();
			throw GdaParserException(key + " not found in table");
		}
		GdaFVSmtPtr p(new GdaFlexValue((*(*table)[key])));
		mark_curr_token_ident();
		inc_token(); // consume NAME token
		return p;
	} else if (curr_token() == Gda::MINUS) { // unary minus
		inc_token(); // consume '-'
		GdaFVSmtPtr p(primary());
		p->NegateSelf();
		return p;
	} else if (curr_token() == Gda::LP) {
		inc_token(); // consume '('
		GdaFVSmtPtr e(expression());
		if (curr_token() != Gda::RP) {
			throw GdaParserException("')' expected");
		}
		inc_token(); // consume ')'
		return e;
	}
	throw GdaParserException("primary expected");
}

Gda::TokenEnum GdaParser::curr_token()
{
	if (tok_i >= tokens.size()) return Gda::END;
	return tokens[tok_i].token;
}

double GdaParser::curr_tok_num_val()
{
	if (tok_i >= tokens.size() ||
		curr_token() != Gda::NUMBER) return 0;
	return tokens[tok_i].number_value;
}

wxString GdaParser::curr_tok_str_val()
{
	if (tok_i >= tokens.size() ||
		curr_token() != Gda::NAME) return "";
	return tokens[tok_i].string_value;
}

Gda::TokenEnum GdaParser::next_token()
{
	int ii = tok_i + 1;
	if (ii >= tokens.size()) return Gda::END;
	return tokens[ii].token;
}

void GdaParser::inc_token()
{
	if (tok_i < tokens.size()) {
		eval_toks.push_back(tokens[tok_i]);
	}
	++tok_i;
}

void GdaParser::mark_curr_token_func()
{
	if (tok_i < tokens.size()) {
		tokens[tok_i].is_func = true;
		tokens[tok_i].is_ident = false;
	}
}

void GdaParser::mark_curr_token_ident()
{
	if (tok_i < tokens.size()) {
		tokens[tok_i].is_func = false;
		tokens[tok_i].is_ident = true;
	}
}

void GdaParser::mark_curr_token_problem()
{
	if (tok_i < tokens.size()) tokens[tok_i].problem_token = true;
}

