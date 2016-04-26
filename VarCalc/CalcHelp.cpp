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

#include "CalcHelp.h"

bool operator<(const CalcHelpEntry& lh, const CalcHelpEntry& rh)
{
	return lh.func < rh.func;
}

std::map<wxString, CalcHelpEntry> CalcHelp::dict;

void CalcHelp::init()
{
	using namespace std;
	typedef CalcHelpEntry::StrPair CSP;
	typedef CalcHelpEntry::ArgPair ARG;
	ARG bool_arg("boolean");
	CSP bool_arg_desc("boolean", "0 represents 'false',"
					  " and any non-zero represents 'true'");
	ARG vec_arg("real");
	CSP vec_arg_desc("real", "Real number or vector of reals.");
	ARG weights_arg("weights");
	CSP weights_arg_desc("weights", "A name representing a weights matrix."
						 " See Tools &gt; Weights &gt; Weights Manager.");
	CSP ex_vec_desc("Assume A is a table variable with values [1, 2, 3, "
					"4, 5]",
					"");
	CSP ex_rates_desc("Assume E and B represent "
					  "event and base variables respectively.", "");
	CSP ex_weights_desc("Assume R the name of a "
					   "rook contiguity weights matrix.","");
	// Operators
	{
		CalcHelpEntry e;
		e.func = "+";
		e.desc = "Addition operator.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("2 + 3","5"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "-";
		e.desc = "Subtraction operator.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("2-3","-1"));
		e.exs.push_back(CSP("-1","-1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "*";
		e.desc = "Multiplication operator.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("2*3","6"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "/";
		e.desc = "Division operator.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("1/2","0.5"));
		e.exs.push_back(CSP("1/0","infinity"));
		e.exs.push_back(CSP("-1/0","-infinity"));
		e.exs.push_back(CSP("0/0","not a number"));
		dict["/"] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "^";
		e.desc = "Exponentiation operator "
			"(infix version of &ldquo;pow&rdquo; below).";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("3 ^ 2","9"));
		e.exs.push_back(CSP("2 ^ 3 ^ 2","512"));
		e.exs.push_back(CSP("(2 ^ 3) ^ 2","64"));
		e.exs.push_back(CSP("16 ^ 1/2","4"));
		e.exs.push_back(CSP("16 ^ 0.5","4"));
		e.exs.push_back(CSP("-2 ^ -3","-0.125"));
		e.exs.push_back(CSP("-1 ^ 0.5","not a number"));
		e.exs.push_back(CSP("-2 ^ -3.5","not a number"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "pow";
		e.desc = "Exponentiation operator "
			"(prefix version of &ldquo;^&rdquo; above).";
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("pow(3, 2)","9"));
		e.exs.push_back(CSP("pow(16, 1/2)","4"));
		e.exs.push_back(CSP("pow(16, 0.5)","4"));
		e.exs.push_back(CSP("pow(-2, -3)","-0.125"));
		e.exs.push_back(CSP("pow(-1, 0.5)","not a number"));
		e.exs.push_back(CSP("pow(-2, -3.5)","not a number"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "=";
		e.desc = "Equality test. Evaluates to 1 for true, 0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("1 = 1","1"));
		e.exs.push_back(CSP("1 = 0","0"));
		e.exs.push_back(CSP("3.5 = 0.5+3","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "&lt;&gt;";
		e.desc = "Inequality test. Evaluates to 1 for true, 0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("1 &lt;&gt; 1","0"));
		e.exs.push_back(CSP("1 &lt;&gt; 0","1"));
		e.exs.push_back(CSP("3.5 &lt;&gt; 0.5+3","0"));
		dict["<>"] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "&lt;";
		e.desc = "Less than test. Evaluates to 1 for true, 0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("4 &lt; 4","0"));
		e.exs.push_back(CSP("4 &lt; 3.9","1"));
		dict["<"] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "&lt;=";
		e.desc = "Less than or equal to test. Evaluates to 1 for true, "
			"0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("4 &lt;= 4","1"));
		e.exs.push_back(CSP("4 &lt;= 3.9","1"));
		dict["<="] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "&gt;";
		e.desc = "Greater than test. Evaluates to 1 for true, 0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("4 &gt; 4","0"));
		e.exs.push_back(CSP("3.9 &gt; 4","1"));
		dict[">"] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "&gt;=";
		e.desc = "Greater than or equal to test. "
			"Evaluates to 1 for true, 0 for false.";
		e.infix = true;
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("4 &gt;= 4","1"));
		e.exs.push_back(CSP("3.9 &gt;= 4","1"));
		dict[">="] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "AND";
		e.desc = "Logical conjunction.";
		e.infix = true;
		e.syn_args.push_back(bool_arg);
		e.args_desc.push_back(bool_arg_desc);
		e.exs.push_back(CSP("1 AND 1","1"));
		e.exs.push_back(CSP("0 AND 1","0"));
		e.exs.push_back(CSP("(3&lt;4) AND (4&gt;3)","1"));
		e.exs.push_back(CSP("(3&lt;4) AND (4&lt;3)","0"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "OR";
		e.desc = "Logical disjunction.";
		e.infix = true;
		e.syn_args.push_back(bool_arg);
		e.args_desc.push_back(bool_arg_desc);
		e.exs.push_back(CSP("1 OR 1","1"));
		e.exs.push_back(CSP("0 OR 1","1"));
		e.exs.push_back(CSP("0 OR 0","0"));
		e.exs.push_back(CSP("3&lt;4 OR 4&gt;3","1"));
		e.exs.push_back(CSP("3&lt;4 OR 4&lt;3","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "XOR";
		e.desc = "Logical exclusive disjunction.";
		e.infix = true;
		e.syn_args.push_back(bool_arg);
		e.args_desc.push_back(bool_arg_desc);
		e.exs.push_back(CSP("1 XOR 1","0"));
		e.exs.push_back(CSP("0 XOR 1","1"));
		e.exs.push_back(CSP("0 XOR 0","0"));
		e.exs.push_back(CSP("3&lt;4 XOR 3&gt;4","0"));
		e.exs.push_back(CSP("3&lt;4 XOR 4&lt;3","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "NOT";
		e.desc = "Logical negation.";
		e.infix = true;
		e.syn_args.push_back(bool_arg);
		e.args_desc.push_back(bool_arg_desc);
		e.exs.push_back(CSP("NOT 0","1"));
		e.exs.push_back(CSP("NOT 1","0"));
		e.exs.push_back(CSP("NOT 1.4","0"));
		e.exs.push_back(CSP("NOT 3&lt;4","0"));
		e.exs.push_back(CSP("NOT NOT 1","1"));
		e.exs.push_back(CSP("NOT NOT 1.4","1"));
		dict[e.func] = e;
	}
	
	
	// Math
	{
		CalcHelpEntry e;
		e.func = "sqrt";
		e.desc = "Principal square root function.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("sqrt(2)","1.41421356237"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "sin";
		e.desc = "Returns sine of an angle.";
		e.syn_args.push_back(ARG("angle"));
		e.args_desc.push_back(CSP("angle","angle in radians"));
		e.exs.push_back(CSP("sin(2)","0.90929742682"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "cos";
		e.desc = "Returns cosine of an angle.";
		e.syn_args.push_back(ARG("angle"));
		e.args_desc.push_back(CSP("angle","angle in radians"));
		e.exs.push_back(CSP("cos(2)","-0.41614683654"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "tan";
		e.desc = "Returns tangent of an angle.";
		e.syn_args.push_back(ARG("angle"));
		e.args_desc.push_back(CSP("angle","angle in radians"));
		e.exs.push_back(CSP("tan(0.5)","0.54630248984"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "asin";
		e.desc = "Returns arc sine of real, expressed in radians.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("asin(0.9)","1.11976951"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "acos";
		e.desc = "Returns arc cosine of real, expressed in radians.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("acos(-0.4)","1.98231317"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "atan";
		e.desc = "Returns arc tangent of real, expressed in radians.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("atan(0.55)","0.502843211"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "abs";
		e.desc = "Absolute value function";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("abs(-1)","1"));
		e.exs.push_back(CSP("abs(1)","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "ceil";
		e.desc = "Ceiling function. Real rounded up to nearest integer.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("ceil(2.2)","3"));
		e.exs.push_back(CSP("ceil(-2.2)","-2"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "floor";
		e.desc = "Floor function. Real rounded down to nearest integer.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("floor(2.2)","2"));
		e.exs.push_back(CSP("floor(-2.2)","-3"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "round";
		e.desc = "Round to nearest integer function.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("round(2.2)","2"));
		e.exs.push_back(CSP("round(-2.2)","-2"));
		e.exs.push_back(CSP("round(-2.51)","-3"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "log";
		e.alt_func = "ln";
		e.desc = "Natural logarithm function.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("log(2.7)","0.99325177301"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "log10";
		e.desc = "Common (base-10) logarithm function.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("log10(100)","2"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "log2";
		e.alt_func = "lg";
		e.desc = "Binary (base-2) logarithm function.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("log2(16)","4"));
		dict[e.func] = e;
	}
	
	// Statistics
	{
		CalcHelpEntry e;
		e.func = "mean";
		e.alt_func = "avg";
		e.desc = "Calculate the mean.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("mean(A)","3"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "max";
		e.desc = "Find the maximum value.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("max(A)","5"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "min";
		e.desc = "Find the minimum value.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("min(A)","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "sum";
		e.desc = "Calculate the sum.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("sum(A)","15"));
		e.exs.push_back(CSP("sum(5)","5"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "stddev";
		e.desc = "Corrected sample standard deviation.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("stddev(A)","1.56114"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "dev_fr_mean";
		e.desc = "Deviation from mean.  If A represents a column of data, "
			"dev_fr_mean(A) is equivalent to A-mean(A)";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("dev_from_mean(A)","[-2, -1, 0, 1, 2]"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "standardize";
		e.desc = "Subtract mean and divide by the corrected sample "
		"standard deviation.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("standardize(A)","[-1.28112, -0.640558, 0, "
							"0.640558, 1.28112]"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "unif_dist";
		e.desc = "Randomly draw from uniform distribution over real unit "
		"interval. Note, argument is only used for dimentions.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("unif_dist(A)","[0.0873, 0.3385, 0.2465"
							", 0.4559, 0.1066]"));
		e.exs.push_back(CSP("floor(unif_dist(A)*6)+1)", "[6, 3, 1, 3, 5]"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "norm_dist";
		e.desc = "Randomly draw from normal distribution. "
		" Note, first argument is only used for dimentions.";
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(ARG("[mean, sd]", true));
		e.args_desc.push_back(vec_arg_desc);
		e.args_desc.push_back(CSP("mean","Mean of distribution. "
								  "Default is 0."));
		e.args_desc.push_back(CSP("sd","Standard deviation of distribution. "
								  "Default is 1."));
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("norm_dist(A)","[-0.3014, 0.7552, -0.1984, "
							"-0.0121, 1.2231]"));
		e.exs.push_back(CSP("norm_dist(A, 100, 10)","[84.1200, 90.3629, "
							"91.1010, 102.9929, 82.2158]"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "enumerate";
		e.desc = "Enumerate . Note, first argument is only used"
		" for dimentions.";
		e.syn_args.push_back(vec_arg);
		e.syn_args.push_back(ARG("[from, step]", true));
		e.args_desc.push_back(vec_arg_desc);
		e.args_desc.push_back(CSP("from","Starting real number. "
								  "Default is 1."));
		e.args_desc.push_back(CSP("step","Size of each step. "
								  "Default is 1."));
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("enumerate(A)","[1, 2, 3, 4, 5]"));
		e.exs.push_back(CSP("enumerate(A, 0, 0.1)","[0, 0.1, 0.2, 0.3, 0.4]"));
		e.exs.push_back(CSP("enumerate(A, 1, -2)","[1, -1, -3, -5, -7]"));
		dict[e.func] = e;
	}
	
	// Ordering
	{
		CalcHelpEntry e;
		e.func = "shuffle";
		e.desc = "Randomly permute observation positions.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("shuffle(A)","[3, 1, 4, 2, 5]"));
		e.exs.push_back(CSP("shuffle(A)","[4, 5, 3, 2, 1]"));
		e.exs.push_back(CSP("shuffle(100)","100"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "rot_down";
		e.desc = "Rotate position of each observation down (or to the right)"
			" one position.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("rot_down(A)","[5, 1, 2, 3, 4]"));
		e.exs.push_back(CSP("rot_down(rot_down(A))","[4, 5, 1, 2, 3]"));
		e.exs.push_back(CSP("rot_down(100)","100"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "rot_up";
		e.desc = "Rotate position of each observation up (or to the left) one"
			" position.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("rot_up(A)","[2, 3, 4, 5, 1]"));
		e.exs.push_back(CSP("rot_up(rot_up(A))","[3, 4, 5, 1, 2]"));
		e.exs.push_back(CSP("rot_up(100)","100"));
		dict[e.func] = e;
	}

	// Numerical Tests
	{
		CalcHelpEntry e;
		e.func = "is_defined";
		e.desc = "Test for a well-defined real.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_defined(sqrt(-1))","0"));
		e.exs.push_back(CSP("is_defined(1/0)","1"));
		e.exs.push_back(CSP("is_defined(0/0)","0"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "is_finite";
		e.desc = "Test for real and finite.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_finite(100)","1"));
		e.exs.push_back(CSP("is_finite(1/0)","0"));
		e.exs.push_back(CSP("is_finite(sqrt(-1))","0"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "is_nan";
		e.desc = "Returns true iff argument is not defined or is imaginary.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_nan(sqrt(-1))","1"));
		e.exs.push_back(CSP("is_nan(100)","0"));
		e.exs.push_back(CSP("is_nan(1/0)","0"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "is_pos_inf";
		e.desc = "Test for positive infinity.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_pos_inf(100)","0"));
		e.exs.push_back(CSP("is_pos_inf(1/0)","1"));
		e.exs.push_back(CSP("is_pos_inf(-1/0)","0"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "is_neg_inf";
		e.desc = "Test for negative infinity.";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_neg_inf(-100)","0"));
		e.exs.push_back(CSP("is_neg_inf(1/0)","0"));
		e.exs.push_back(CSP("is_neg_inf(-1/0)","1"));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "is_inf";
		e.desc = "Test for infinity (positive or negative).";
		e.syn_args.push_back(vec_arg);
		e.args_desc.push_back(vec_arg_desc);
		e.exs.push_back(CSP("is_inf(100)","0"));
		e.exs.push_back(CSP("is_inf(1/0)","1"));
		e.exs.push_back(CSP("is_inf(100 + 1/0)","1"));
		e.exs.push_back(CSP("is_inf(-1/0)","1"));
		dict[e.func] = e;
	}
	
	// Weights
	{
		CalcHelpEntry e;
		e.func = "counts";
		e.desc = "Return the number of neighbors for each observation as "
		"defined by a weights matrix.";
		e.syn_args.push_back(weights_arg);
		e.args_desc.push_back(weights_arg_desc);
		e.exs.push_back(ex_weights_desc);
		e.exs.push_back(CSP("counts(R)","Vector containing number of neighbors "
							" for each observation defined by "
							"weights matrix R."));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "lag";
		e.desc = "Compute the spatial lag of a vector.";
		e.syn_args.push_back(weights_arg);
		e.args_desc.push_back(weights_arg_desc);
		e.syn_args.push_back(ARG("var"));
		e.args_desc.push_back(CSP("var","Table variable of reals."));
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(ex_weights_desc);
		e.exs.push_back(CSP("lag(R,A)","Vector containing spatially "
							"lagged values of A according to R weights."));
		dict[e.func] = e;
	}
	
	// Rates
	{
		CalcHelpEntry e;
		e.func = "raw_rate";
		e.desc = "Raw Rate.";
		e.syn_args.push_back(ARG("event"));
		e.args_desc.push_back(CSP("event","Table variable of reals."));
		e.syn_args.push_back(ARG("base"));
		e.args_desc.push_back(CSP("base","Table variable of reals."));
		e.exs.push_back(ex_rates_desc);
		e.exs.push_back(CSP("raw_rate(E,B)",""));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "excess_risk";
		e.desc = "Excess Risk.";
		e.syn_args.push_back(ARG("event"));
		e.args_desc.push_back(CSP("event","Table variable of reals."));
		e.syn_args.push_back(ARG("base"));
		e.args_desc.push_back(CSP("base","Table variable of reals."));
		e.exs.push_back(ex_vec_desc);
		e.exs.push_back(CSP("excess_risk(E,B)",""));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "emp_bayes";
		e.desc = "Empirical Bayes.";
		e.syn_args.push_back(ARG("event"));
		e.args_desc.push_back(CSP("event","Table variable of reals."));
		e.syn_args.push_back(ARG("base"));
		e.args_desc.push_back(CSP("base","Table variable of reals."));
		e.exs.push_back(ex_rates_desc);
		e.exs.push_back(CSP("emp_bayes(E,B)",""));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "spatial_rate";
		e.desc = "Spatial Rate.";
		e.syn_args.push_back(weights_arg);
		e.args_desc.push_back(weights_arg_desc);
		e.syn_args.push_back(ARG("event"));
		e.args_desc.push_back(CSP("event","Table variable of reals."));
		e.syn_args.push_back(ARG("base"));
		e.args_desc.push_back(CSP("base","Table variable of reals."));
		e.exs.push_back(ex_weights_desc);
		e.exs.push_back(ex_rates_desc);
		e.exs.push_back(CSP("spatial_rate(R,E,B)",""));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "sp_emp_bayes";
		e.desc = "Spatial Emperical Bayes.";
		e.syn_args.push_back(weights_arg);
		e.args_desc.push_back(weights_arg_desc);
		e.syn_args.push_back(ARG("event"));
		e.args_desc.push_back(CSP("event","Table variable of reals."));
		e.syn_args.push_back(ARG("base"));
		e.args_desc.push_back(CSP("base","Table variable of reals."));
		e.exs.push_back(ex_weights_desc);
		e.exs.push_back(ex_rates_desc);
		e.exs.push_back(CSP("sp_emp_bayes(R,E,B)",""));
		dict[e.func] = e;
	}
	{
		CalcHelpEntry e;
		e.func = "emp_bayes_rt_std";
		e.desc = "Emperical Bayes Rate Standardization.";
		e.syn_args.push_back(ARG("var"));
		e.args_desc.push_back(CSP("var","Table variable of reals."));
		e.syn_args.push_back(ARG("var"));
		e.args_desc.push_back(CSP("var","Table variable of reals."));
		e.exs.push_back(ex_rates_desc);
		e.exs.push_back(CSP("emp_bayes_rt_std(E,B)",""));
		dict[e.func] = e;
	}
}

bool CalcHelp::HasEntry(const wxString& f)
{
	return dict.find(f) != dict.end();
}

CalcHelpEntry CalcHelp::GetEntry(const wxString& f)
{
	if (dict.find(f) == dict.end()) return CalcHelpEntry();
	return dict[f];
}


