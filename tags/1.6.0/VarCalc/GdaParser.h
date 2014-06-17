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

#ifndef __GEODA_CENTER_GDA_PARSER_H__
#define __GEODA_CENTER_GDA_PARSER_H__

#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <wx/string.h>
#include "GdaFlexValue.h"
#include "GdaLexer.h"

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
					 std::map<wxString, GdaFVSmtPtr> * table);
	
	GdaFVSmtPtr GetEvalVal() { return eval_val; }
	wxString GetErrorMsg() { return error_msg; }
	/** Return a list of tokens that were successfully processed. Parser
	 will also set is_func and is_dent for Gda::NAME tokens */
	std::vector<GdaTokenDetails> GetEvalTokens() { return eval_toks; }
	
private:
	GdaFVSmtPtr expression();
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

	size_t tok_i;
	std::vector<GdaTokenDetails> tokens;
	std::map<wxString, GdaFVSmtPtr>* table;
	
	std::vector<GdaTokenDetails> eval_toks;
	wxString error_msg;
	GdaFVSmtPtr eval_val;
};

/** Grammar

program:
  END
  expression END

expression:
  expression + mult_expr
  expression - mult_expr
  mult_expr

mult_expr:
  mult_expr * pow_expr
  mult_expr / pow_expr
  pow_expr

pow_expr:
  func_expr ^ expression
  func_expr

  2^3^4 -> 2^(3^4)  top down order by convention or right-to-left associativity

func_expr:
  NAME ()
  NAME ( expression )
  NAME ( expression, expression )
  primary

primary:
  NUMBER
  NAME
  - primary
  ( expression )
 */

#endif

