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

#ifndef __GEODA_CENTER_GDA_LEXER_H__
#define __GEODA_CENTER_GDA_LEXER_H__

#include <vector>
#include <wx/string.h>
#include <wx/regex.h>

class GdaLexerException: public std::exception
{
public:
	GdaLexerException() {}
	GdaLexerException(const wxString& msg_) :msg(msg_) {} 
	virtual ~GdaLexerException() throw() {}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
private:
	wxString msg;
};

namespace Gda {
enum TokenEnum {
	NAME, NUMBER, STRING, END,
	EQ, // "=" equality test
	NE, // "<>" not equal
	LT, // "<" less than
	GT, // ">" greater than
	LE, // "<=" less than or equal
	GE, // ">=" greater than or equal
	AND, OR, NOT, XOR,
	// printable ascii char codes start at 33,
	PLUS='+', MINUS='-', MUL='*', DIV='/',
	LP='(', RP=')', POW='^', COMMA=','
	};
}

struct GdaTokenDetails
{
	GdaTokenDetails() : token(Gda::END), number_value(0),
		start_ind(0), end_ind(0), is_func(false), is_ident(false),
		problem_token(false) {}
	Gda::TokenEnum token;
	double number_value;
	wxString string_value;
	wxString orig_str; // original string
	size_t start_ind; // index of first character
	size_t end_ind; // index of last character plus one
	bool is_func; // true if a function token or '(' or ')' as part of func
	bool is_ident; // true if an identifier token
	bool problem_token; // true if bad token or parser error
	wxString ToStr() const; // print out some details for debugging
};

class GdaLexer {
public:
	GdaLexer();
	/** If no errors during tokenizing, then true is returned and
	 tokens contains the complete list of extracted tokens.
	 If an error occurred, then GetErrorMsg returns a helpful error
	 message.  Regardless of success, tokens contains the list of
	 tokens up until the problematic token/character. */ 
	bool Tokenize(const wxString& s, std::vector<GdaTokenDetails>& tokens);

	wxString GetErrorMsg() { return error_msg; }
	static wxString TokToStr(Gda::TokenEnum tok);

	static wxRegEx regex_ws;
	static wxRegEx regex_num;
	static wxRegEx regex_name;
	static wxRegEx regex_str_lit;

private:
	GdaTokenDetails get_token();

	size_t chars_consumed;

	wxString input;
	wxString error_msg;
};

#endif

