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
#include "GdaFlexValue.h"

GdaFlexValue::GdaFlexValue()
: V(1), obs(1), tms(1)
{
	V = 0.0;
}

GdaFlexValue::GdaFlexValue(size_t obs_, size_t tms_)
: V(obs_*tms_), obs(obs_), tms(tms_)
{
	V = 0.0;
}

GdaFlexValue::GdaFlexValue(const std::valarray<double>& V_,
						   size_t obs_, size_t tms_)
: V(V_), obs(obs_), tms(tms_)
{
}

GdaFlexValue::GdaFlexValue(const double& value)
: V(value, 1), obs(1), tms(1)
{
}

GdaFlexValue::GdaFlexValue(const GdaFlexValue& v)
: V(v.V), obs(v.obs), tms(v.tms)
{
}

GdaFlexValue& GdaFlexValue::operator=(const GdaFlexValue& v)
{
	if (v.V.size() != V.size()) V.resize(v.V.size());
	V = v.V;
	return *this;
}

wxString GdaFlexValue::ToStr()
{
	wxString s;
	if (V.size() == 0) return s;
	if (V.size() == 1) {
		s << GetDouble();
	} else {
		for (size_t r=0; r<obs; ++r) {
			s << "[";
			for (size_t t=0; t<tms-1; ++t) {
				s << V[r*tms+t] << ", ";
			}
			s << V[r*tms+tms-1] << "]";
			if (r<obs-1) s << "\n";
		} 
	}
	return s;
}

GdaFlexValue& GdaFlexValue::operator+=(const GdaFlexValue& v)
{
	grow_if_smaller(v);
	// we know V.size() >= v.V.size()
	if (V.size() == v.V.size()) {
		V += v.V;
	} else if (v.V.size() == 1) {
		V += v.GetDouble();
	} else {
		// must be case that tms > 1 and v.tms == 1
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] += v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator-=(const GdaFlexValue& v)
{
	grow_if_smaller(v);
	if (V.size() == v.V.size()) {
		V -= v.V;
	} else if (v.V.size() == 1) {
		V -= v.GetDouble();
	} else {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] -= v.V;
		}
	}
	return *this;
}


GdaFlexValue& GdaFlexValue::operator*=(const GdaFlexValue& v)
{
	grow_if_smaller(v);
	if (V.size() == v.V.size()) {
		V *= v.V;
	} else if (v.V.size() == 1) {
		V *= v.GetDouble();
	} else {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] *= v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator/=(const GdaFlexValue& v)
{
	grow_if_smaller(v);
	if (V.size() == v.V.size()) {
		V /= v.V;
	} else if (v.V.size() == 1) {
		V /= v.GetDouble();
	} else {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] /= v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator^=(const GdaFlexValue& v)
{
	grow_if_smaller(v);
	if (V.size() == v.V.size()) {
		V = pow(V, v.V);
	} else if (v.V.size() == 1) {
		V = pow(V, v.GetDouble());
	} else {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(obs*t,obs,1)] =
			pow(std::valarray<double>(V[std::slice(t,obs,tms)]), v.V);
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::NegateSelf()
{
	V = -V;
	return *this;
}

void GdaFlexValue::SetSize(size_t obs_, size_t tms_)
{
	if (obs == obs_ && tms == tms_) return;
	obs = obs_;
	tms = tms_;
	V.resize(obs_*tms_);
}

void GdaFlexValue::grow_if_smaller(const GdaFlexValue& v)
{
	if (V.size() == v.V.size()) return;
	if (obs > 1 && v.obs > 1 && obs != v.obs) {
		throw GdaFVException("number of obs mismatch");
	}
	if (tms > 1 && v.tms > 1 && tms != v.tms) {
		throw GdaFVException("number of tms mismatch");
	}
	if (V.size() > v.V.size()) return;
	
	// we are smaller, so need to grow
	// also know that obs and tms dims are compatible.
	if (V.size() == 1) {
		double t = GetDouble();
		V.resize(v.V.size());
		obs = v.obs;
		tms = v.tms;
		V = t;
		return;
	}
	
	// must be case that obs > 1 and tms == 1 and v.tms > 1
	if (!(obs > 1) && (tms == 1) && (v.tms > 1)) {
		throw GdaFVException("unexpected dimensions");
	}
	
	std::valarray<double> orig(V);
	V.resize(v.V.size());
	for (size_t t=0; t<v.tms; ++t) {
		V[std::slice(t,obs,v.tms)] = orig;
	}
	obs = v.obs;
	tms = v.tms;
}

