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
#include "NumericTests.h"

double Gda::logical_or(double a, double b) { return (bool) a || (bool) b; }
double Gda::logical_and(double a, double b) { return (bool) a && (bool) b; }
double Gda::logical_xor(double a, double b) {
	return ((bool) a && !((bool) b)) ||
		(!((bool) a) && (bool) b); }
double Gda::logical_not(double a) { return !((bool) a); }
double Gda::eq(double a, double b) { return (double) (a == b); }
double Gda::ne(double a, double b) { return (double) (a != b); }
double Gda::lt(double a, double b) { return a < b; }
double Gda::gt(double a, double b) { return a > b; }
double Gda::le(double a, double b) { return a <= b; }
double Gda::ge(double a, double b) { return a >= b; }

double Gda::is_defined(double a) { return ! (a != a); }
double Gda::is_finite(double a) { return a-a==0; }
double Gda::is_nan(double a) { return a != a; }
double Gda::is_pos_inf(double a) {
	return !(a != a) && (a-a!=0) && (a>0); }
double Gda::is_neg_inf(double a)  {
	return !(a != a) && (a-a!=0) && (a<0); }
double Gda::is_inf(double a) { return a-a!=0; }

