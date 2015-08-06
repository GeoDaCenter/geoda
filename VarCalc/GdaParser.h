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

#ifndef __GEODA_CENTER_GDA_PARSER_H__
#define __GEODA_CENTER_GDA_PARSER_H__

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <wx/string.h>
#include "WeightsManInterface.h"
#include "GdaFlexValue.h"
#include "GdaLexer.h"
#include "NumericTests.h"

class GdaParserException: public std::exception
{
public:
	GdaParserException() {}
	GdaParserException(const wxString& msg_) :msg(msg_) {} 
	virtual ~GdaParserException() throw() {}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
private:
	wxString msg;
};

class GdaParser {
public:
	GdaParser();
	/** If no errors during evaluation, then true is returned and GetEvalVal
	 retuns the final output value.  If errors occurred, then GetErrorMsg
	 returns a helpful error message.  Regardless of success, GetEvalTokens
	 returns the list of tokens that were evaluated. */ 
	bool eval(const std::vector<GdaTokenDetails>& tokens,
			  std::map<wxString, GdaFVSmtPtr>* data_table,
			  WeightsManInterface* w_man_int);
	
	GdaFVSmtPtr GetEvalVal() { return eval_val; }
	wxString GetErrorMsg() { return error_msg; }
	/** Return a list of tokens that were successfully processed. Parser
	 will also set is_func and is_dent for Gda::NAME tokens */
	std::vector<GdaTokenDetails> GetEvalTokens() { return eval_toks; }
	
private:
	GdaFVSmtPtr expression();
	GdaFVSmtPtr logical_xor_expr();
	GdaFVSmtPtr logical_or_expr();
	GdaFVSmtPtr logical_and_expr();
	GdaFVSmtPtr logical_not_expr();
	GdaFVSmtPtr comp_expr();
	GdaFVSmtPtr add_expr();
	GdaFVSmtPtr mult_expr();
	GdaFVSmtPtr pow_expr();
	GdaFVSmtPtr func_expr();
	GdaFVSmtPtr primary();

	Gda::TokenEnum curr_token();
	double curr_tok_num_val();
	wxString curr_tok_str_val();
	Gda::TokenEnum next_token();
	void inc_token();
	void mark_curr_token_func();
	void mark_curr_token_ident();
	void mark_curr_token_problem();

	static void exception_if_not_data(const GdaFVSmtPtr x);
	static void exception_if_not_weights(const GdaFVSmtPtr x);
	
	size_t tok_i;
	std::vector<GdaTokenDetails> tokens;
	std::map<wxString, GdaFVSmtPtr>* data_table;
	WeightsManInterface* w_man_int;
	
	std::vector<GdaTokenDetails> eval_toks;
	wxString error_msg;
	GdaFVSmtPtr eval_val;
};


/** Grammar

program:
  END
  expression END

expression:
  logical_or_expr

logical_or_expr:
  logical_or_expr OR logical_and_expr
  logical_and_expr

logical_and_expr:
  logical_and_expr AND comp_expr
  comp_expr

logical_not_expr:
  NOT expression
  ! expression
  comp_expr

comp_expr:
  comp_expr < add_expr
  comp_expr <= add_expr
  comp_expr > add_expr
  comp_expr >= add_expr
  comp_expr = add_expr
  comp_expr <> add_expr
  comp_expr != add_expr
  add_expr

add_expr:
  add_expr + mult_expr
  add_expr - mult_expr
  mult_expr

mult_expr:
  mult_expr * pow_expr
  mult_expr / pow_expr
  pow_expr

pow_expr:
  func_expr ^ logical_or_expr
  func_expr

  2^3^4 -> 2^(3^4)  top down order by convention or right-to-left associativity

func_expr:
  NAME ()
  NAME ( logical_or_expr )
  NAME ( logical_or_expr, logical_or_expr )
  primary

primary:
  TRUE
  FALSE
  NUMBER
  NAME
  - primary
  ( logical_or_expr )
 */

#endif

