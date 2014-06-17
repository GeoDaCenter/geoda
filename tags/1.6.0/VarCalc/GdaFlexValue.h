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

#ifndef __GEODA_CENTER_GDA_FLEX_VALUE_H__
#define __GEODA_CENTER_GDA_FLEX_VALUE_H__

#include <exception>
#include <ostream>
#include <valarray>
#include <boost/shared_ptr.hpp>
#include <wx/string.h>

class GdaFVException: public std::exception
{
public:
	GdaFVException()  {}
	GdaFVException(const wxString& msg_) :msg(msg_) {} 
	virtual ~GdaFVException() throw() {}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
private:
	wxString msg;
};

class GdaFlexValue
{
public:
	GdaFlexValue();
	GdaFlexValue(size_t obs, size_t tms);
	GdaFlexValue(const double& value);
	GdaFlexValue(const std::valarray<double>& V,
				 size_t obs, size_t tms);
	GdaFlexValue(const GdaFlexValue& v);
	GdaFlexValue& operator=(const GdaFlexValue& v);
	
	wxString ToStr();
	
	GdaFlexValue& operator+=(const GdaFlexValue& v);
	GdaFlexValue& operator-=(const GdaFlexValue& v);
	GdaFlexValue& operator*=(const GdaFlexValue& v);
	GdaFlexValue& operator/=(const GdaFlexValue& v);
	GdaFlexValue& operator^=(const GdaFlexValue& v);
	
	GdaFlexValue& NegateSelf();
	void SetDouble(double d) { V = d; }
	double GetDouble() const { return V[0]; }
	size_t GetObs() const { return obs; }
	size_t GetTms() const { return tms; }
	void SetSize(size_t obs, size_t tms);
	
	std::valarray<double>& GetValArrayRef() { return V; }
	
private:
	void grow_if_smaller(const GdaFlexValue& v);
	std::valarray<double> V;
	size_t obs; // or number of rows
	size_t tms; // or number of columns
};

typedef boost::shared_ptr<GdaFlexValue> GdaFVSmtPtr;

/*
 standardize() -> returns vec
 mean() -> constant
 std_dev() -> constant
 dev_from_mean() -> returns vec
 unif_random(0,1)
 unif_random()
 normal_random()
 normal_random(mean, sd)
 enumerate() -> returns vec
 ln(A)
 log(A)
 log(A, base)
 sqrt(A)
 W*A or lag(A) where W is a spacial weights or something that evaluates as such
 raw_rate(A,B) // A is event, B is base
 excess_risk(A,B)
 spatial_rate(A,B,W)
 lag(A,W)
 
 rook() // order = 1
 rook(order)
 rook(order, inc_lower = false)
 queen() // order = 1
 
 rook() && queen()  -> logical AND of sets.  in this case -> rook()
 
 min_euc
 W1 = threshold(0.64);
 W1 = threshold
 
 */



#endif



