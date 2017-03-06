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

#include <limits>
#include <math.h>
#include "../logger.h"
#include "GdaParser.h"

GdaParser::GdaParser()
	: data_table(0), w_man_int(0)
{
}

bool GdaParser::eval(const std::vector<GdaTokenDetails>& tokens_,
					 std::map<wxString, GdaFVSmtPtr>* data_table_,
					 WeightsManInterface* w_man_int_)
{
	tokens = tokens_;
	data_table = data_table_;
	w_man_int = w_man_int_;
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
	catch (GdaFVException e) {
		error_msg = e.what();
	}
	catch (std::exception e) {
		error_msg = e.what();
	}
	return success;
}

GdaFVSmtPtr GdaParser::expression()
{
	return logical_xor_expr();
}

GdaFVSmtPtr GdaParser::logical_xor_expr()
{
	using namespace std;
	GdaFVSmtPtr left = logical_or_expr();
	
	for (;;) {
		if (curr_token() == Gda::XOR) {
			inc_token(); // consume XOR
			GdaFVSmtPtr right = logical_or_expr();
			left->ApplyBinarySelf(&Gda::logical_xor, *right);
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::logical_or_expr()
{
	using namespace std;
	GdaFVSmtPtr left = logical_and_expr();
	
	for (;;) {
		if (curr_token() == Gda::OR) {
			inc_token(); // consume OR
			GdaFVSmtPtr right = logical_and_expr();
			left->ApplyBinarySelf(&Gda::logical_or, *right);
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::logical_and_expr()
{
	using namespace std;
	GdaFVSmtPtr left = logical_not_expr();
	
	for (;;) {
		if (curr_token() == Gda::AND) {
			inc_token(); // consume AND
			GdaFVSmtPtr right = logical_not_expr();
			left->ApplyBinarySelf(&Gda::logical_and, *right);
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::logical_not_expr()
{
	if (curr_token() == Gda::NOT) {
		inc_token(); // consume NOT
		GdaFVSmtPtr p(expression());
		p->ApplyUniSelf(&Gda::logical_not);
		return p;
	}
	return comp_expr();
}

GdaFVSmtPtr GdaParser::comp_expr()
{
	GdaFVSmtPtr left = add_expr();
	
	for (;;) {
		if (curr_token() == Gda::LT) {
			inc_token(); // consume <
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::lt, *right);
		} else if (curr_token() == Gda::LE) {
			inc_token(); // consume <=
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::le, *right);
		} else if (curr_token() == Gda::GT) {
			inc_token(); // consume >
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::gt, *right);
		} else if (curr_token() == Gda::GE) {
			inc_token(); // consume >=
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::ge, *right);
		} else if (curr_token() == Gda::EQ) {
			inc_token(); // consume =
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::eq, *right);
		} else if (curr_token() == Gda::NE) {
			inc_token(); // consume <>
			GdaFVSmtPtr right = add_expr();
			left->ApplyBinarySelf(&Gda::ne, *right);
		} else {
			return left;
		}
	}
}

GdaFVSmtPtr GdaParser::add_expr()
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
	inc_token(); // consume '('
	if (curr_token() == Gda::RP) {
		inc_token(); // consume ')'
		// return 0-ary function NAME () value
		LOG(func_name);
		if (func_name.CmpNoCase("rook") == 0 ||
			func_name.CmpNoCase("queen") == 0) {
			throw GdaParserException("automatic weights creation "
									 "not yet supported");
			if (!w_man_int) {
				throw GdaParserException("no weights available.");
			}
			WeightsMetaInfo wmi;
			if (func_name.CmpNoCase("rook") == 0) {
				wmi.SetToRook("");
			} else {
				wmi.SetToQueen("");
			}
			boost::uuids::uuid u = w_man_int->RequestWeights(wmi);
			GdaFVSmtPtr p(new GdaFlexValue(u));
			return p;
		}
		throw GdaParserException("unknown function \"" + func_name + "\"");
	}
	GdaFVSmtPtr arg1(expression()); // evaluate first argument
	if (curr_token() == Gda::RP) {
		inc_token(); // consume ')'
		// evaluate unary function NAME ( arg1 )
		LOG(func_name);
		if (func_name.CmpNoCase("sqrt") == 0) {
			arg1->ApplyUniSelf(&sqrt);
		} else if (func_name.CmpNoCase("cos") == 0) {
			arg1->ApplyUniSelf(&cos);
		} else if (func_name.CmpNoCase("sin") == 0) {
			arg1->ApplyUniSelf(&sin);
		} else if (func_name.CmpNoCase("tan") == 0) {
			arg1->ApplyUniSelf(&tan);
		} else if (func_name.CmpNoCase("acos") == 0) {
			arg1->ApplyUniSelf(&acos);
		} else if (func_name.CmpNoCase("asin") == 0) {
			arg1->ApplyUniSelf(&asin);
		} else if (func_name.CmpNoCase("atan") == 0) {
			arg1->ApplyUniSelf(&atan);
		} else if (func_name.CmpNoCase("abs") == 0 ||
			func_name.CmpNoCase("fabs") == 0) {
			arg1->ApplyUniSelf(&fabs);
		} else if (func_name.CmpNoCase("ceil") == 0) {
			arg1->ApplyUniSelf(&ceil);
		} else if (func_name.CmpNoCase("floor") == 0) {
			arg1->ApplyUniSelf(&floor);
		} else if (func_name.CmpNoCase("round") == 0) {
			arg1->Round();
		} else if (func_name.CmpNoCase("log") == 0 ||
			func_name.CmpNoCase("ln") == 0) {
			arg1->ApplyUniSelf(&log);
		} else if (func_name.CmpNoCase("log10") == 0) {
			arg1->ApplyUniSelf(&log10);
		} else if (func_name.CmpNoCase("sum") == 0) {
			arg1->Sum();
		} else if (func_name.CmpNoCase("mean") == 0 ||
			func_name.CmpNoCase("avg") == 0) {
			arg1->Mean();
		} else if (func_name.CmpNoCase("sum") == 0) {
			arg1->Sum();
		} else if (func_name.CmpNoCase("stddev") == 0) {
			arg1->StdDev();
		} else if (func_name.CmpNoCase("dev_fr_mean") == 0) {
			arg1->DevFromMean();
		} else if (func_name.CmpNoCase("standardize") == 0) {
			arg1->Standardize();
		} else if (func_name.CmpNoCase("shuffle") == 0) {
			arg1->Shuffle();
		} else if (func_name.CmpNoCase("rot_down") == 0) {
			arg1->Rotate(-1);
		} else if (func_name.CmpNoCase("rot_up") == 0) {
			arg1->Rotate(1);
		} else if (func_name.CmpNoCase("unif_dist") == 0) {
			arg1->UniformDist();
		} else if (func_name.CmpNoCase("norm_dist") == 0) {
			arg1->GaussianDist(0,1);
		} else if (func_name.CmpNoCase("enumerate") == 0) {
			arg1->Enumerate(1, 1);
		} else if (func_name.CmpNoCase("max") == 0) {
			arg1->Max();
		} else if (func_name.CmpNoCase("min") == 0) {
			arg1->Min();
		} else if (func_name.CmpNoCase("is_defined") == 0) {
			arg1->ApplyUniSelf(&Gda::is_defined);
		} else if (func_name.CmpNoCase("is_finite") == 0) {
			arg1->ApplyUniSelf(&Gda::is_finite);
		} else if (func_name.CmpNoCase("is_nan") == 0) {
			arg1->ApplyUniSelf(&Gda::is_nan);
		} else if (func_name.CmpNoCase("is_pos_inf") == 0) {
			arg1->ApplyUniSelf(&Gda::is_pos_inf);
		} else if (func_name.CmpNoCase("is_neg_inf") == 0) {
			arg1->ApplyUniSelf(&Gda::is_neg_inf);
		} else if (func_name.CmpNoCase("is_inf") == 0) {
			arg1->ApplyUniSelf(&Gda::is_inf);
		} else if (func_name.CmpNoCase("counts") == 0) {
			if (!w_man_int) {
				throw GdaParserException("no weights available.");
			}
			if (!arg1->IsWeights()) {
				throw GdaParserException("first argument of counts must be weights");
			}
			std::vector<long> counts;
			if (!w_man_int->GetCounts(arg1->GetWUuid(), counts)) {
				throw GdaParserException("could not find neighbor counts");
			}
			GdaFVSmtPtr p(new GdaFlexValue(counts));
			return p;
		} else {
			throw GdaParserException("unknown function \"" + func_name + "\"");
		}
		return arg1;
	}
	if (curr_token() != Gda::COMMA) {
		throw GdaParserException("',' or ')' expected");
	}
	inc_token(); // consume ','
	GdaFVSmtPtr arg2(expression()); // evaluate second argument
	if (curr_token() == Gda::RP) {
		inc_token(); // consume ')'
		// evaluate binary function NAME ( arg1 , arg2 )
		LOG(func_name);
		if (func_name.CmpNoCase("pow") == 0) {
			arg1->ApplyBinarySelf(&pow, *arg2);
		} else if (func_name.CmpNoCase("lag") == 0) {
			if (!w_man_int) {
				throw GdaParserException("no weights available.");
			}
			if (!arg1->IsWeights()) {
				throw GdaParserException("first argument of lag must be weights.");
			}
			if (!arg2->IsData()) {
				throw GdaParserException("second argument of lag must be data.");
			}
			if (!w_man_int->WeightsExists(arg1->GetWUuid())) {
				throw GdaParserException("invalid weights.");
			}
			LOG(arg1->ToStr());
			GdaFVSmtPtr p(new GdaFlexValue());
			if (!w_man_int->Lag(arg1->GetWUuid(), *arg2, *p)) {
				throw GdaParserException("error computing spatial lag");
			}
			return p;
		} else {
			throw GdaParserException("unknown function \"" + func_name + "\"");
		}
		return arg1;
	}
	if (curr_token() != Gda::COMMA) {
		throw GdaParserException("',' or ')' expected");
	}
	inc_token(); // consume ','
	GdaFVSmtPtr arg3(expression()); // evaluate third argument
	if (curr_token() == Gda::RP) {
		// mark as function token.
		inc_token(); // consume ')'
		// evaluate binary function NAME ( arg1 , arg2, arg3 )
		LOG(func_name);
		if (func_name.CmpNoCase("enumerate") == 0 ||
			func_name.CmpNoCase("norm_dist") == 0) {
			if (arg2->GetObs() != 1 || arg2->GetTms() != 1) {
				throw GdaParserException("second argument of " + func_name
										 + " must be a constant.");
			}
			if (arg3->GetObs() != 1 || arg3->GetTms() != 1) {
				throw GdaParserException("third argument of " + func_name
										 + " must be a constant.");
			}
		}
		if (func_name.CmpNoCase("enumerate") == 0) {
			arg1->Enumerate(arg2->GetDouble(), arg3->GetDouble());
		} else if (func_name.CmpNoCase("norm_dist") == 0) {
			double sd = arg3->GetDouble();
			if (sd-sd != 0 || sd < 0) {
				// x-x == 0 is a reliable test for double being finite
				throw GdaParserException("third argument of " + func_name
										 + " must be a non-negative finite real.");
			}
			arg1->GaussianDist(arg2->GetDouble(), sd);
		} else {
			throw GdaParserException("unknown function \"" + func_name + "\"");
		}
		return arg1;
	}
	throw GdaParserException("')' expected");
}

GdaFVSmtPtr GdaParser::primary()
{	
	if (curr_token() == Gda::STRING) {
		GdaFVSmtPtr p(new GdaFlexValue(curr_tok_str_val()));
		inc_token(); // consume STRING token
		return p;
	}
	if (curr_token() == Gda::NUMBER) {
		GdaFVSmtPtr p(new GdaFlexValue(curr_tok_num_val()));
		//double v = curr_tok_num_val();
		inc_token(); // consume NUMBER token
		return p;
	} else if (curr_token() == Gda::NAME) {
		wxString key(curr_tok_str_val());
		//if (next_token() == Gda::ASSIGN) {
		//	(*data_table)[key] = expression(true);
		//}
		// check for existence in data_table and in weights if w_man_int exists
		if (data_table->find(key) == data_table->end() &&
			(!w_man_int ||
			 (w_man_int && w_man_int->FindIdByTitle(key).is_nil()))) {
			mark_curr_token_ident();
			mark_curr_token_problem();
			inc_token();
			throw GdaParserException(key + " not a known identifier");
		}
		mark_curr_token_ident();
		inc_token(); // consume NAME token
		if (data_table->find(key) != data_table->end()) {
			GdaFVSmtPtr p(new GdaFlexValue((*(*data_table)[key])));
			return p;
		} else {
			GdaFVSmtPtr p(new GdaFlexValue(w_man_int->FindIdByTitle(key)));
			return p;
		}
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
		(curr_token() != Gda::NAME &&
		 curr_token() != Gda::STRING)) return "";
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

void GdaParser::exception_if_not_data(const GdaFVSmtPtr x)
{
	if (!x->IsData()) {
		throw GdaParserException("parser expected data expression");
	}
}

void GdaParser::exception_if_not_weights(const GdaFVSmtPtr x)
{
	if (!x->IsWeights()) {
		throw GdaParserException("parser expected weights expression");
	}
}

