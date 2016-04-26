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

#include "../logger.h"
#include "GdaLexer.h"

wxString GdaTokenDetails::ToStr() const
{
	wxString s = GdaLexer::TokToStr(token);
	s << " orig_str: \"" << orig_str << "\"";
	s << " start_ind: " << start_ind;
	s << " end_ind: " << end_ind;
	if (token == Gda::NAME) s << " string_value: \"" << string_value << "\"";
	if (token == Gda::STRING) s << " string_value: \"" << string_value << "\"";
	if (token == Gda::NUMBER) s << " number_value: """ << number_value << "\"";
	if (problem_token) s << " PROBLEM TOKEN";
	return s;
}

wxRegEx GdaLexer::regex_ws("^[ \\t]+");
wxRegEx GdaLexer::regex_num("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");
wxRegEx GdaLexer::regex_name("^[A-Za-z_][A-Za-z_0-9]*");
wxRegEx GdaLexer::regex_str_lit("^\"(\\.|[^\"])*\"");

GdaLexer::GdaLexer()
{
}

wxString GdaLexer::TokToStr(Gda::TokenEnum tok)
{
	if (tok == Gda::NAME) return "NAME";
	if (tok == Gda::NUMBER) return "NUMBER";
	if (tok == Gda::STRING) return "STRING";
	if (tok == Gda::PLUS) return "PLUS";
	if (tok == Gda::MINUS) return "MINUS";
	if (tok == Gda::MUL) return "MUL";
	if (tok == Gda::DIV) return "DIV";
	if (tok == Gda::LP) return "LP";
	if (tok == Gda::RP) return "RP";
	if (tok == Gda::POW) return "POW";
	if (tok == Gda::COMMA) return "COMMA";
	if (tok == Gda::EQ) return "EQ";
	if (tok == Gda::NE) return "NE";
	if (tok == Gda::LT) return "LT";
	if (tok == Gda::GT) return "GT";
	if (tok == Gda::LE) return "LE";
	if (tok == Gda::GE) return "GE";
	if (tok == Gda::AND) return "AND";
	if (tok == Gda::OR) return "OR";
	if (tok == Gda::NOT) return "NOT";
	if (tok == Gda::XOR) return "XOR";
	return "END";
}

GdaTokenDetails GdaLexer::get_token()
{
	//LOG_MSG("In GdaLexer::get_token");
	GdaTokenDetails tok;

	// skip whitespace including '\n'
	if (regex_ws.Matches(input)) {
		size_t start;
		size_t len;
		regex_ws.GetMatch(&start, &len);
		input = input.SubString(start+len, input.length()-1);
		chars_consumed += len;
	}

	if (input.length() == 0) {
		tok.token = Gda::END;
		tok.orig_str = "";
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed;
		return tok;
	}

	wxString ch = input.SubString(0,0);
	if ( ch == "*" || ch == "/" || ch == "+" ||
		 ch == "-" || ch == "(" || ch == ")" ||
		 ch == "^" || ch == ",") {
		//LOG_MSG("GdaLexer::get_token(): " + ch);
		if (ch == "+") { tok.token = Gda::PLUS; }
		else if (ch == "-") { tok.token = Gda::MINUS; }
		else if (ch == "*") { tok.token = Gda::MUL; }
		else if (ch == "/") { tok.token = Gda::DIV; }
		else if (ch == "(") { tok.token = Gda::LP; }
		else if (ch == ")") { tok.token = Gda::RP; }
		else if (ch == "^") { tok.token = Gda::POW; }
		else { tok.token = Gda::COMMA; }
		tok.orig_str = ch;
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed+1;
		input = input.SubString(1,input.length()-1);
		chars_consumed += 1;
		return tok;
	}

	if (input.length() >= 2) {
		wxString ch = input.SubString(0,1);
		if ( ch == "<>" || ch == "!=" ||
			 ch == "<=" || ch == ">=" ) {
			if (ch == "<>" || ch == "!=") { tok.token = Gda::NE; }
			else if (ch == "<=") { tok.token = Gda::LE; }
			else { tok.token = Gda::GE; }
			tok.orig_str = ch;
			tok.start_ind = chars_consumed;
			tok.end_ind = chars_consumed+2;
			input = input.SubString(2,input.length()-1);
			chars_consumed += 2;
			return tok;
		}
	}

	if ( ch == "<" || ch == ">" || ch == "="
		 || ch == "!" || ch == "&" || ch == "|") {
		//LOG_MSG("GdaLexer::get_token(): " + ch);
		if (ch == "<") { tok.token = Gda::LT; }
		else if (ch == ">") { tok.token = Gda::GT; }
		else if (ch == "=") { tok.token = Gda::EQ; }
		else if (ch == "&") { tok.token = Gda::AND; }
		else if (ch == "|") { tok.token = Gda::OR; }
		else { tok.token = Gda::NOT; }
		tok.orig_str = ch;
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed+1;
		input = input.SubString(1,input.length()-1);
		chars_consumed += 1;
		return tok;
	}

	if (regex_num.Matches(input)) {
		size_t start;
		size_t len;
		regex_num.GetMatch(&start, &len);
		tok.token = Gda::NUMBER;
		tok.orig_str = input.SubString(start, len-1);
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed + len;
		tok.orig_str.ToDouble(&tok.number_value);
		input = input.SubString(start+len,input.length()-1);
		chars_consumed += len;
		return tok;
	}
	
	if (regex_name.Matches(input)) {
		size_t start;
		size_t len;
		regex_name.GetMatch(&start, &len);
		tok.orig_str = input.SubString(start, len-1);
		if (tok.orig_str.CmpNoCase("AND") == 0) {
			tok.token = Gda::AND;
		} else if (tok.orig_str.CmpNoCase("OR") == 0) {
			tok.token = Gda::OR;
		} else if (tok.orig_str.CmpNoCase("NOT") == 0) {
			tok.token = Gda::NOT;
		} else if (tok.orig_str.CmpNoCase("XOR") == 0) {
			tok.token = Gda::XOR;
		} else {
			tok.token = Gda::NAME;
		}
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed + len;
		tok.string_value = tok.orig_str;
		input = input.SubString(start+len,input.length()-1);
		chars_consumed += len;
		return tok;
	}

	if (regex_str_lit.Matches(input)) {
		size_t start;
		size_t len;
		regex_str_lit.GetMatch(&start, &len);
		tok.token = Gda::STRING;
		tok.orig_str = input.SubString(start, len-1);
		tok.start_ind = chars_consumed;
		tok.end_ind = chars_consumed + len;
		 // need to remove outer quotes.
		tok.string_value = input.SubString(start+1, len-2);
		input = input.SubString(start+len,input.length()-1);
		chars_consumed += len;
		return tok;
	}

	throw GdaLexerException("bad token: " + input.SubString(0,0));
}

bool GdaLexer::Tokenize(const wxString &s, std::vector<GdaTokenDetails>& tokens)
{
	LOG_MSG("Entering GdaLexer::Tokenize");
	tokens.clear();
	chars_consumed = 0;
	input = s;

	bool success = false;
	while (true) {
		try {
			tokens.push_back(get_token());
			if (tokens[tokens.size()-1].token == Gda::END) {
				success = true;
				break;
			}
		}
		catch ( GdaLexerException e ) {
			error_msg = e.what();
			GdaTokenDetails tok;
			tok.token = Gda::END;
			tokens.push_back(tok);
			success = false;
			break;
		}
	}
		LOG_MSG("Exiting GdaLexer::Tokenize");
	return success;
}

