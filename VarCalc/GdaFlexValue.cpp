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

#include <math.h>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "../logger.h"
#include "GdaFlexValue.h"

GdaFlexValue::GdaFlexValue()
	: V(1), obs(1), tms(1), is_string_lit(false),
	  weights_uuid(boost::uuids::nil_uuid())
{
	V = 0.0;
}

GdaFlexValue::GdaFlexValue(size_t obs_, size_t tms_)
	: V(obs_*tms_), obs(obs_), tms(tms_), is_string_lit(false),
	  weights_uuid(boost::uuids::nil_uuid())
{
	V = 0.0;
}

GdaFlexValue::GdaFlexValue(const std::valarray<double>& V_,
						   size_t obs_, size_t tms_)
	: V(V_), obs(obs_), tms(tms_), is_string_lit(false),
	  weights_uuid(boost::uuids::nil_uuid())
{
}

GdaFlexValue::GdaFlexValue(const std::vector<long>& counts)
	: V(counts.size()), obs(counts.size()), tms(1), is_string_lit(false),
	  weights_uuid(boost::uuids::nil_uuid())
{
	for (size_t i=0, sz=counts.size(); i<sz; ++i) {
		V[i] = (double) counts[i];
	}
}

GdaFlexValue::GdaFlexValue(const double& value)
	: V(value, 1), obs(1), tms(1), is_string_lit(false),
	  weights_uuid(boost::uuids::nil_uuid())
{
}

GdaFlexValue::GdaFlexValue(const GdaFlexValue& v)
	: V(v.V), obs(v.obs), tms(v.tms),
	  is_string_lit(v.is_string_lit), string_lit(v.string_lit),
	  weights_uuid(v.weights_uuid)
{
}

GdaFlexValue::GdaFlexValue(boost::uuids::uuid u)
	: V(0), obs(0), tms(1), is_string_lit(false), weights_uuid(u)
{
}

GdaFlexValue::GdaFlexValue(const wxString& str_lit)
	: V(0), obs(0), tms(1), is_string_lit(true), string_lit(str_lit)
{
	LOG(string_lit);
}

GdaFlexValue& GdaFlexValue::operator=(const GdaFlexValue& v)
{
	if (v.V.size() != V.size()) V.resize(v.V.size());
	V = v.V;
	obs = v.obs;
	tms = v.tms;
	is_string_lit = v.is_string_lit;
	string_lit = v.string_lit;
	weights_uuid = v.weights_uuid;
	return *this;
}

bool GdaFlexValue::IsStringLit() const
{
	return is_string_lit;
}

bool GdaFlexValue::IsData() const
{
	return weights_uuid.is_nil()
		&& !is_string_lit;
}

bool GdaFlexValue::IsWeights() const
{
	return !weights_uuid.is_nil() && !is_string_lit;
}

wxString GdaFlexValue::ToStr() const
{
	wxString s;
	if (IsStringLit()) {
		s << "string literal type\n";
		s << "string_lit: " << string_lit;
		return s;
	}
	if (IsWeights()) {
		s << "weights type\n";
		s << "weights_uuid: " << wxString(boost::uuids::to_string(weights_uuid));
		return s;
	}
	s << "data type\n";
	if (V.size() == 0) return s;
	if (V.size() == 1) {
		s << V[0];
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
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	// obs >= v.obs && tms >= v.tms
	if (obs == v.obs && tms == v.tms) {
		V += v.V;
	} else if (v.V.size() == 1) {
		V += v.V[0];
	} else if (tms > 1 && v.tms == 1) {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] += v.V;
		}
	} else if (obs > 1 && v.obs == 1) {
		for (size_t i=0; i<obs; ++i) {
			V[std::slice(i*tms,tms,1)] += v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator-=(const GdaFlexValue& v)
{
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	if (obs == v.obs && tms == v.tms) {
		V -= v.V;
	} else if (v.V.size() == 1) {
		V -= v.V[0];
	} else if (tms > 1 && v.tms == 1) {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] -= v.V;
		}
	} else if (obs > 1 && v.obs == 1) {
		for (size_t i=0; i<obs; ++i) {
			V[std::slice(i*tms,tms,1)] -= v.V;
		}
	}
	return *this;
}


GdaFlexValue& GdaFlexValue::operator*=(const GdaFlexValue& v)
{
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	if (obs == v.obs && tms == v.tms) {
		V *= v.V;
	} else if (v.V.size() == 1) {
		V *= v.V[0];
	} else if (tms > 1 && v.tms == 1) {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] *= v.V;
		}
	} else if (obs > 1 && v.obs == 1) {
		for (size_t i=0; i<obs; ++i) {
			V[std::slice(i*tms,tms,1)] *= v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator/=(const GdaFlexValue& v)
{
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	if (obs == v.obs && tms == v.tms) {
		V /= v.V;
	} else if (v.V.size() == 1) {
		V /= v.V[0];
	} else if (tms > 1 && v.tms == 1) {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] /= v.V;
		}
	} else if (obs > 1 && v.obs == 1) {
		for (size_t i=0; i<obs; ++i) {
			V[std::slice(i*tms,tms,1)] /= v.V;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::operator^=(const GdaFlexValue& v)
{
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	if (obs == v.obs && tms == v.tms) {
		V = pow(V, v.V);
	} else if (v.V.size() == 1) {
		V = pow(V, v.V[0]);
	} else if (tms > 1 && v.tms == 1) {
		for (size_t t=0; t<tms; ++t) {
			V[std::slice(t,obs,tms)] =
				pow(std::valarray<double>(V[std::slice(t,obs,tms)]), v.V);
		}
	} else if (obs > 1 && v.obs == 1) {
		for (size_t i=0; i<obs; ++i) {
			V[std::slice(i*tms,tms,1)] =
				pow(std::valarray<double>(V[std::slice(i*tms,tms,1)]), v.V);
		}
	}

	return *this;
}

GdaFlexValue& GdaFlexValue::NegateSelf()
{
	exception_if_not_data(*this);
	V = -V;
	return *this;
}

GdaFlexValue& GdaFlexValue::ApplyUniSelf(double (*f)(double))
{
	LOG_MSG("In GdaFlexValue::ApplyUniSelf");
	exception_if_not_data(*this);
	for (int i=0, sz=V.size(); i<sz; ++i) V[i] = f(V[i]);
	return *this;
}

GdaFlexValue& GdaFlexValue::ApplyBinarySelf(double (*f)(double, double),
											const GdaFlexValue& v)
{
	LOG_MSG("In GdaFlexValue::ApplyBinarySelf");
	exception_if_not_data(*this);
	exception_if_not_data(v);
	grow_if_smaller(v);
	GdaFlexValue cpy_v(v);
	cpy_v.grow_if_smaller(*this);
	for (int i=0, sz=V.size(); i<sz; ++i) V[i] = f(V[i], cpy_v.V[i]);
	return *this;
}

GdaFlexValue& GdaFlexValue::Sum()
{
	exception_if_not_data(*this);
	if (obs == 1) return *this;
	std::valarray<double> x(0.0, tms);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			x[t] += V[i*tms+t];
		}
	}
	V = x;
	obs = 1;
	return *this;
}

GdaFlexValue& GdaFlexValue::Mean()
{
	exception_if_not_data(*this);
	std::valarray<double> x(0.0, tms);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			x[t] += V[i*tms+t];
		}
		x[t] /= obs;
	}
	V = x;
	obs = 1;
	return *this;
}

GdaFlexValue& GdaFlexValue::StdDev()
{
	exception_if_not_data(*this);
	std::valarray<double> mean(0.0, tms);
	std::valarray<double> sum_sq(0.0, tms);
	std::valarray<double> sd(0.0, tms);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			mean[t] += V[i*tms+t];
		}
		mean[t] /= obs;
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] -= mean[t];
			sum_sq[t] += V[i*tms+t] * V[i*tms+t];
		}
		sd[t] = sqrt(sum_sq[t] / (((double) obs)-1.0));
	}
	V = sd;
	obs = 1;
	return *this;
}

GdaFlexValue& GdaFlexValue::DevFromMean()
{
	exception_if_not_data(*this);
	std::valarray<double> mean(0.0, tms);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			mean[t] += V[i*tms+t];
		}
		mean[t] /= obs;
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] -= mean[t];
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::Standardize()
{
	exception_if_not_data(*this);
	std::valarray<double> mean(0.0, tms);
	std::valarray<double> sum_sq(0.0, tms);
	std::valarray<double> sd(0.0, tms);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			mean[t] += V[i*tms+t];
		}
		mean[t] /= obs;
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] -= mean[t];
			sum_sq[t] += V[i*tms+t] * V[i*tms+t];
		}
		sd[t] = sqrt(sum_sq[t] / (((double) obs)-1.0));
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] /= sd[t];
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::Shuffle()
{
	exception_if_not_data(*this);
    // Mersenne Twister random number generator, randomly seeded
	// with current time in seconds since Jan 1 1970.
	static boost::mt19937 rng(std::time(0));
	static boost::random::uniform_int_distribution<> X(0, obs-1);
	// X(rng) -> returns a uniform random number from 0 to obs-1;
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			// swap each item in data with a random position in data.
			// This will produce a random permutation
			size_t r = (size_t) X(rng);
			double tmp = V[r*tms+t];
			V[r*tms+t] = V[i*tms+t];
			V[i*tms+t] = tmp;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::UniformDist()
{
	exception_if_not_data(*this);
	static boost::mt19937 rng(std::time(0));
	static boost::uniform_01<boost::mt19937> X(rng);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] = X();
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::GaussianDist(double mean, double sd)
{
	exception_if_not_data(*this);
	static boost::mt19937 rng(std::time(0));
	boost::normal_distribution<> norm_dist(mean, sd);
	boost::variate_generator<boost::mt19937&,
							 boost::normal_distribution<> >
		X(rng, norm_dist);
	for (size_t t=0; t<tms; ++t) {
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] = X();
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::Enumerate(double from, double step)
{
	exception_if_not_data(*this);
	for (size_t t=0; t<tms; ++t) {
		double x=from;
		for (size_t i=0; i<obs; ++i) {
			V[i*tms+t] = x;
			x+=step;
		}
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::Round()
{
	exception_if_not_data(*this);
	for (size_t i=0, sz=V.size(); i<sz; ++i) {
		V[i] = boost::math::round(V[i]);
	}
	return *this;
}

GdaFlexValue& GdaFlexValue::Max()
{
	exception_if_not_data(*this);
	if (obs == 1) return *this;
	double pos_inf = std::numeric_limits<double>::infinity();
	double nan = std::numeric_limits<double>::quiet_NaN();
	std::valarray<double> x(nan, tms);
	for (size_t t=0; t<tms; ++t) {
		bool any_defined = false;
		double m = -pos_inf;
		for (size_t i=0; i<obs; ++i) {
			double v = V[i*tms+t];
			if (v != v) continue;
			any_defined = true;
			if (v > m) m = v;
		}
		if (any_defined) x[t] = m;
	}
	V = x;
	obs = 1;
	return *this;
}

GdaFlexValue& GdaFlexValue::Min()
{
	exception_if_not_data(*this);
	if (obs == 1) return *this;
	double pos_inf = std::numeric_limits<double>::infinity();
	double nan = std::numeric_limits<double>::quiet_NaN();
	std::valarray<double> x(nan, tms);
	for (size_t t=0; t<tms; ++t) {
		bool any_defined = false;
		double m = pos_inf;
		for (size_t i=0; i<obs; ++i) {
			double v = V[i*tms+t];
			if (v != v) continue;
			any_defined = true;
			if (v < m) m = v;
		}
		if (any_defined) x[t] = m;
	}
	V = x;
	obs = 1;
	return *this;
}

GdaFlexValue& GdaFlexValue::Rotate(int step)
{
	exception_if_not_data(*this);
	for (size_t t=0; t<tms; ++t) {
		std::vector<double> tmp(obs);
		for (size_t i=0; i<obs; ++i) {
			tmp[i] = V[i*tms+t];
		}
		for (size_t i=0; i<obs; ++i) {
			int r = (((int) i)+step+obs) % ((int) obs);
			V[i*tms+t] = tmp[r];
		}
	}
	return *this;
}

double GdaFlexValue::GetDouble() const
{
	//exception_if_not_data(*this);
	return V.size() > 0 ? V[0] : std::numeric_limits<double>::quiet_NaN();
}

void GdaFlexValue::SetSize(size_t obs_, size_t tms_)
{
	if (obs == obs_ && tms == tms_) return;
	obs = obs_;
	tms = tms_;
	V.resize(obs_*tms_);
}

/**
  Increase the obs and tms dimensions to be compatible
  with the target value.

  Examples:

  A = [1]  B = [1]
               [2]
               [3]

  C = [1 2 3 4]  D = [1 2 3 4]
                     [5 6 7 8]
                     [9 10 11 12]

  A can grow to B, C or D
  B can grow to D
  C can grow to D
  B and C can not grow to each other's size,
  but rather they must grow to D's dimentions.

  eg B + C
  B -> [1 1 1 1]  C -> [1 2 3 4]
       [2 2 2 2]       [1 2 3 4]
       [3 3 3 3]       [1 2 3 4]

  B + C -> [2 3 4 5]
           [3 4 5 6]
           [4 5 6 7]

  B + mean(D)  in this case mean(D) collapses to the same
  dimensions as C

  So, increase dimensions does the following:
  increase this->obs if this->obs < v.obs
  increase this->tms if this->tms < v.tms

  Should be able to do this in a two step process. For
  example, to make A compatible with D
  [1] -> [1] -> [1 1 1 1]
         [1]    [1 1 1 1]
         [1]    [1 1 1 1]

  Enumerate will always become a single columm
  enumerate(from, step)

  

 */
void GdaFlexValue::grow_if_smaller(const GdaFlexValue& v)
{
	if (obs == v.obs && tms == v.tms) return;
	if (obs > 1 && v.obs > 1 && obs != v.obs) {
		throw GdaFVException("number of obs mismatch");
	}
	if (tms > 1 && v.tms > 1 && tms != v.tms) {
		throw GdaFVException("number of tms mismatch");
	}

	// in special case where V.size() == 1, can resize
    // much faster.
	if (V.size() == 1) {
		double t = V[0];
		V.resize(v.V.size());
		obs = v.obs;
		tms = v.tms;
		V = t;
		return;
	}

	// if *this has fewer obs, then increase obs dim
	if (obs < v.obs) {
		std::valarray<double> orig(V);
		V.resize(v.obs*tms);
		for (size_t i=0; i<v.obs; ++i) {
			V[std::slice(i*tms,tms,1)] = orig;
		}
		obs = v.obs;
	}

	// if *this has fewer tms, then increase tms dim
	if (tms < v.tms) {
		std::valarray<double> orig(V);
		V.resize(obs*v.tms);
		for (size_t t=0; t<v.tms; ++t) {
			V[std::slice(t,obs,v.tms)] = orig;
		}
		tms = v.tms;
	}
}

void GdaFlexValue::exception_if_not_data(const GdaFlexValue& x)
{
	if (!x.IsData()) {
		throw GdaFVException("value expected data expression");
	}
}

void GdaFlexValue::exception_if_not_weights(const GdaFlexValue& x)
{
	if (!x.IsWeights()) {
		throw GdaFVException("value expected weights expression");
	}
}

