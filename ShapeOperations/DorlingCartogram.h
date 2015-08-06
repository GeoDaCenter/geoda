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

/**
 * The following code is mostly a modified version of Daniel Dorling's
 * Area Cartogram code (his own invention), as found in the Appendix of the
 * book:
 *   Area cartograms: their use and creation, Concepts and Techniques in 
 *     Modern Geography series no. 59, University of East Anglia: Environmental
 *     Publications.
 *
 * Dorling's code is Public Domain.  The original code is in C, and we have
 * made only small modifications to support data structure needs.  For easy
 * comparison to the original code, we have mostly left his variable names,
 * comments, and looping logic intact.
 */

#ifndef __GEODA_CENTER_DORLING_CARTOGRAM_H__
#define __GEODA_CENTER_DORLING_CARTOGRAM_H__

#include <vector>

class GalElement;

// nbour, border and perimeter not used in get_point or add_point
//   also, these variables only depend on input x,y which is constant
//   over time
//int** nbour; // completely read only after initilized: list of neighbors
//int* nbours; // read only after initialized.  Number of neighbors
//double** border; // completely read only after initilized
//double* perimeter; // read-only, based on border
struct CartNbrInfo {
	CartNbrInfo(GalElement* gal, int num_obs);
	virtual ~CartNbrInfo();
	
	int bodies; // num_obs+1.  Will follow Dorling convention of arrays
	            // starting from 1
	int* nbours;  // neighbor counts. Critical for border and nbour
	//array allocations
	int** nbour;  // neighbor ids.  ids start from 1
	double** border; // borders will be 1.0
	double* perimeter; // 1.0 * nbours[i]
};


class DorlingCartogram {
public:
	
	DorlingCartogram(CartNbrInfo* nbs, const std::vector<double>& orig_x,
					 const std::vector<double>& orig_y,
					 const std::vector<double>& orig_data,
					 const double& orig_data_min,
					 const double& orig_data_max);
	virtual ~DorlingCartogram();
	
	int improve(int num_iters);
	
	std::vector<double> output_x;
	std::vector<double> output_y;
	std::vector<double> output_radius;
	// estimate of seconds per iteration based on last execution of improve()
	double secs_per_iter;
	
	// variables that never change after initialization
	
protected:
	// number of observations + 1
	// There appears to be no reason not to start at 0.  This was likely
	// done since the original code was written in Pascal where arrays
	// are indexed from 1.
	int bodies;

	void init_cartogram(const std::vector<double>& orig_x,
						const std::vector<double>& orig_y,
						const std::vector<double>& orig_data,
						const double& orig_data_min,
						const double& orig_data_max);
	
	void add_point(int pointer, int axis, int body);
	void get_point(int pointer, int axis, int body);
	
	struct leaf {
		int id;
		double xpos;
		double ypos;
		int left;
		int right;
	};
	
	int* nbours;
	int** nbour;
	double** border;
	double* perimeter;
	
	// original population data.  This is read only, and is
	// completely forgotten after it is used to defined the radius array
	double* people;
	
	// Note: Dorling assumes that all populations are non-zero.  So, we
	// should scale up input data so that it is non-negative.  To ensure
	// that bounded well above zero, could add 10% of range + (- min value)
	// so that data is bounded well away from zero.
	

	// arrays: read-only by get_point and add_point.  These are initially
	// set to the orginal position, but over time they are modified as the
	// circles move after each iteration.
	double* x;
	double* y;
	
	// local arrays used to update x,y after each iteration.
	double* xvector;
	double* yvector;
	
	// similar use to xvector, yvector.  Does not need to
	// be global, except for efficient reuse.  Radius is set once at the
	// beginning, but then remains constant, so it is not quite like
	// xvector and yvector which change every iteration.
	//std::vector<double> radius;
	double* radius;
	
	// variables that do change after initialization	
	int number; // modified by get_point and used as a global variable
	   // for to index into list array.
	
	int end_pointer; // modified by add_point and used as a global variable

	double widest; // also max in output_radius
	double distance; // read-only in get_point.  Could be passed into
	// get_point as a parameter
	
	int* list; // array that is modified by get_point and used
	           // in main loop of iterations to keep track of neighbors
	
	
	leaf* tree; // array of leaf structs
	
	static const double friction;
	static const double ratio;
	static const double pi;
};

#endif
