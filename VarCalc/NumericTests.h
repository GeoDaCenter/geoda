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

#ifndef __GEODA_CENTER_NUMERIC_TESTS_H__
#define __GEODA_CENTER_NUMERIC_TESTS_H__

namespace Gda {
	// logical operations
	double logical_eq(double a, double b);
	double logical_ineq(double a, double b);
	double logical_or(double a, double b);
	double logical_and(double a, double b);
	double logical_xor(double a, double b);
	double logical_not(double a);
	// comparison operations
	double eq(double a, double b);
	double ne(double a, double b);
	double lt(double a, double b);
	double gt(double a, double b);
	double le(double a, double b);
	double ge(double a, double b);
	// real number tests
	double is_defined(double a); // same as !is_nan
	double is_finite(double a);
	double is_nan(double a);  // same as !is_defined
	double is_pos_inf(double a);
	double is_neg_inf(double a);
	double is_inf(double a);
}

#endif

