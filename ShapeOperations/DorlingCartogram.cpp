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

#include <wx/msgdlg.h>
#include <wx/stopwatch.h>
#include "../logger.h"
#include "../GenUtils.h"
#include "GalWeight.h"
#include "DorlingCartogram.h"

CartNbrInfo::CartNbrInfo(GalElement* gal, int num_obs)
: bodies(num_obs+1)
{
	typedef int* int_ptr;
	typedef double* dbl_ptr;
	nbours = new int[bodies];
	nbour = new int_ptr[bodies];
	border = new dbl_ptr[bodies];
	perimeter = new double[bodies];

	for (int i=0; i<num_obs; i++) {
		int n_cnt = gal[i].Size();
		nbours[i+1] = n_cnt;
		nbour[i+1] = new int[n_cnt+1];
		border[i+1] = new double[n_cnt+1];
		perimeter[i+1] = n_cnt;
		for (int j=0; j<n_cnt; j++) {
			nbour[i+1][j+1] = gal[i][j]+1;
			border[i+1][j+1] = 1;
		}
	}
	
	//for (int i=1; i<=num_obs; i++) {
	//	wxString msg;
	//	msg << "obs: " << i << " perimeter: " << perimeter[i];
	//	msg << " nbours: " << nbours[i];
	//	for (int j=1; j<=nbours[i]; j++) {
	//		msg << "\n         ";
	//		msg << "nbour[" << i << "][" << j << "]: " << nbour[i][j];
	//		msg << ", border: " << border[i][j];
	//	}
	//	LOG_MSG(msg);
	//}
	LOG_MSG("Done CartNbrInfo creation");
}

CartNbrInfo::~CartNbrInfo()
{
	if (nbour) {
		for (int i=1; i<bodies; i++) if (nbour[i]) delete [] nbour[i];
		delete [] nbour;
	}
	if (border) {
		for (int i=1; i<bodies; i++) if (border[i]) delete [] border[i];
		delete [] border;
	}
	if (perimeter) delete [] perimeter;
}


const double DorlingCartogram::friction = 0.25;
const double DorlingCartogram::ratio = 0.1;
const double DorlingCartogram::pi = 3.141592653589793238463;

DorlingCartogram::DorlingCartogram(CartNbrInfo* nbs,
								   const std::vector<double>& orig_x,
								   const std::vector<double>& orig_y,
								   const std::vector<double>& orig_data,
								   const double& orig_data_min,
								   const double& orig_data_max)
: output_radius(orig_x.size()),
output_x(orig_x.size()), output_y(orig_x.size()),
bodies(orig_x.size()+1),
nbours(nbs->nbours), nbour(nbs->nbour), border(nbs->border),
perimeter(nbs->perimeter),
secs_per_iter(0.01)
{
    LOG_MSG("Entering DorlingCartogram()");
	x = new double[bodies];
	y = new double[bodies];
	people = new double[bodies];
	radius = new double[bodies];
	xvector = new double[bodies];
	yvector = new double[bodies];
	list = new int[bodies];
	tree = new leaf[bodies];
	
	init_cartogram(orig_x, orig_y, orig_data, orig_data_min, orig_data_max);
    
    LOG_MSG("Exiting DorlingCartogram()");
}

DorlingCartogram::~DorlingCartogram()
{
	if (x) delete [] x;
	if (y) delete [] y;
	if (people) delete [] people;
	if (radius) delete [] radius;
	if (xvector) delete [] xvector;
	if (yvector) delete [] yvector;
	if (list) delete [] list;
	if (tree) delete [] tree;
}

// Global variables: tree, x, y, body
void DorlingCartogram::add_point(int pointer, int axis, int body)
{
    if (tree[pointer].id == 0) {
		tree[pointer].id = body;
		tree[pointer].left = 0;
		tree[pointer].right = 0;
		tree[pointer].xpos = x[body];
		tree[pointer].ypos = y[body];
    } else {
		if (axis == 1) {
			if (x[body] >= tree[pointer].xpos) {
				if (tree[pointer].left == 0){ 
					end_pointer += 1;
					tree[pointer].left = end_pointer;
				}
				add_point(tree[pointer].left, 3-axis, body);
			} else {
				if (tree[pointer].right == 0) { 
					end_pointer +=1;
					tree[pointer].right = end_pointer;
				}
				add_point(tree[pointer].right, 3-axis, body);
			}
		} else {
			if (y[body] >= tree[pointer].ypos) {
				if (tree[pointer].left == 0) {
					end_pointer += 1;
					tree[pointer].left = end_pointer;
				}
				add_point(tree[pointer].left, 3-axis, body);
			} else {
				if (tree[pointer].right == 0) { 
					end_pointer += 1;
					tree[pointer].right = end_pointer;
				}
				add_point(tree[pointer].right, 3-axis, body);
			}
		}
	}
}

// Global variables: tree, x, y, body, list, number, distance
// modified variables: list and number
void DorlingCartogram::get_point(int pointer, int axis, int body)
{
	if (pointer > 0) {
		if (tree[pointer].id > 0) {
			if (axis == 1) { 
				if (x[body]-distance < tree[pointer].xpos) {
					get_point(tree[pointer].right, 3-axis, body);
				}
				if (x[body]+distance >= tree[pointer].xpos) {
					get_point(tree[pointer].left, 3-axis, body);
				}
			}
			if (axis == 2) {
				if (y[body]-distance < tree[pointer].ypos) {
					get_point(tree[pointer].right, 3-axis, body);
				}
				if (y[body] + distance >= tree[pointer].ypos) {
					get_point(tree[pointer].left, 3-axis, body);
				}
			}
			if ((x[body]-distance < tree[pointer].xpos) && 
				(x[body]+distance >= tree[pointer].xpos)) {
				if ((y[body]-distance < tree[pointer].ypos) &&
					(y[body]+distance >= tree[pointer].ypos)) { 
					number += 1;
					list[number] = tree[pointer].id;
				}
			}
		}
	}
}

// We pass in orig_data_min(max) as parameters rather than calculating
// them so that radius scales can be identical for mutiple cartograms
// across time.  orig_x / orig_y will normally be the same, so they
// are not passed in.
void DorlingCartogram::init_cartogram(const std::vector<double>& orig_x,
									  const std::vector<double>& orig_y,
									  const std::vector<double>& orig_data,
									  const double& orig_data_min,
									  const double& orig_data_max)
{	
	const double map_to_people = 9000000;
	const double map_to_coorindate = 300000;
	
	// Dorling's code assumes a strictly positive people array since
	// he was dealing with population counts in each region.  We will
	// therefore scale and translate our array as appropriate.
	double min = orig_data_min;
	double max = orig_data_max;
	double range = max-min;
	for (int i=0, its=orig_data.size(); i<its; i++) people[i+1] = orig_data[i];
	if (min < 0) {
		double d = -min;
		for (int b=1; b<bodies; b++) people[b] += d;
		min += d;
		max += d;
	}
	if (min == max) {
		for (int b=1; b<bodies; b++) people[b] = 1;
		min = 1;
		max = 1;
	}
	if (min*10 < range) {
		double d = range/10;
		for (int b=1; b<bodies; b++) people[b] += d;
		min += d;
		max += d;
	}

	double xmin, xmax, ymin, ymax;
	SampleStatistics::CalcMinMax(orig_x, xmin, xmax);
	SampleStatistics::CalcMinMax(orig_y, ymin, ymax);
	double xrange = xmax-xmin;
	if (xrange == 0) xrange = 1.0;
	double yrange = ymax-ymin;
	if (yrange == 0) yrange = 1.0;
	double map_range = GenUtils::max<double>(xrange, yrange);
	
	double t_dist = 0.0;
	double t_radius = 0.0;
	for (int body=1; body<bodies; body++) {
		x[body] = (orig_x[body-1] / map_range) * map_to_coorindate;
		y[body] = (orig_y[body-1] / map_range) * map_to_coorindate;
	}
	
	if (range == 0) range = 1.0;
	for (int b=1; b<bodies; b++) {
		people[b] = (people[b] / range) * map_to_people;
	}
	double xd;
	double yd;
	for (int body=1; body<bodies; body++) {
		for (int nb = 1; nb <= nbours[body]; nb++) {
			if (nbour[body][nb] > 0) {
				if (nbour[body][nb] < body) {
					xd = x[body] - x[nbour[body][nb]];
					yd = y[body] - y[nbour[body][nb]];
					t_dist += sqrt(xd*xd+yd*yd);
					t_radius += sqrt(people[body]/pi) +
						sqrt(people[nbour[body][nb]]/pi);
				}
			}
		}    
	}
	
	
	double scale = t_dist / t_radius;
	if (scale == 0) scale = 1.0;
	widest = 0.0;
	for (int body=1; body<bodies; body++) {
		radius[body] = scale*sqrt(people[body]/pi);
		//LOG_MSG(wxString::Format("obs %d, radius: %f people: %f",
		//						 body, radius[body], people[body]));
		if (radius[body] > widest) widest = radius[body];
		xvector[body] = 0.0;
		yvector[body] = 0.0;
	}
	LOG_MSG("initialization complete");
	
	for (int i=0, its=bodies-1; i<its; i++) output_radius[i] = radius[i+1];
}


int DorlingCartogram::improve(int num_iters)
{
	wxStopWatch sw;
	
	// start the big loop creating the tree each iter
	
	int other;
	double closest;
	double dist;
	double overlap;
	double atrdst;
	double repdst;
	double xattract;
	double yattract;
	double xrepel;
	double yrepel;
	double xtotal;
	double ytotal;
	double xd;
	double yd;
	
    for (int itter=0; itter<num_iters; itter++) {
        for (int body=1; body<bodies; body++) tree[body].id = 0;
        end_pointer = 1;
		
        for (int body=1; body<bodies; body++) add_point(1,1,body);
		
		// loop of independent body movements
		
        for (int body=1; body<bodies; body++) {
            // get <number> of neighbors within <distance> into <list[]>
            number = 0;
            distance = widest + radius[body];
            get_point(1,1,body);
			
            xrepel = yrepel = 0.0;
            xattract = yattract = 0.0;
            closest = widest;
            //LOG_MSG(wxString::Format("  number %d:", number));
			
            // work out repelling force of overlapping neighbors
            if (number > 0) {
                for (int nb=1; nb<=number; nb++) {
                    other = list[nb];
                    if (other != body) {
                        xd = x[other]-x[body];
                        yd = y[other]-y[body];
                        dist = sqrt(xd*xd+yd*yd);
                        if (dist < closest) closest = dist;
                        overlap = radius[body] + radius[other]-dist;
                        if (overlap > 0 && dist > 1) {
							xrepel = xrepel-overlap*(x[other]-x[body])/dist;
							yrepel = yrepel-overlap*(y[other]-y[body])/dist;
                        }
                    }
                }
            }
			
			// work out forces of attraction between neighbours
			
            for (int nb=1; nb<=nbours[body]; nb++) {
                other = nbour[body][nb];
                if (other != 0) {
                    xd = (x[body]-x[other]);
                    yd = (y[body]-y[other]);
                    dist = sqrt(xd*xd+yd*yd);
                    overlap = dist - radius[body] - radius[other];
                    if (overlap > 0.0) {
						overlap = overlap *
							border[body][nb]/perimeter[body];
                        xattract = xattract + overlap*(x[other]-x[body])/dist;
                        yattract = yattract + overlap*(y[other]-y[body])/dist;
                    }
                }
            }
			
			// now work out the combined effect of attraction and repulsion
			
            atrdst = sqrt(xattract * xattract + yattract * yattract);
            repdst = sqrt(xrepel * xrepel+ yrepel * yrepel);
            if (repdst > closest) {
                xrepel = closest * xrepel / (repdst +1.0);
                yrepel = closest * yrepel / (repdst +1.0);
                repdst = closest;
            }
            if (repdst > 0.0) {
                xtotal = (1.0-ratio) * xrepel +
					ratio*(repdst*xattract/(atrdst+1.0));
                ytotal = (1.0-ratio) * yrepel +
					ratio*(repdst*yattract/(atrdst+1.0));
            } else {
                if (atrdst > closest) {
                    xattract = closest *xattract/(atrdst+1);
                    yattract = closest *yattract/(atrdst+1);
                }
                xtotal = xattract;
                ytotal = yattract;
            }
            xvector[body] = friction * (xvector[body]+xtotal);
            yvector[body] = friction * (yvector[body]+ytotal);
        }
		
		// update the positions
        
		for (int body=1; body<bodies; body++) {
            x[body] += (xvector[body]); //+ 0.5);
            y[body] += (yvector[body]); // + 0.5);
        }
    }
	
	//LOG_MSG(wxString::Format("CartogramNewView after %d iterations:",
	//						 num_iters));
	//for (int b=1; b<bodies; b++) {
	//	LOG_MSG(wxString::Format("  obs %d, r: %f x: %f y %f", b,
	//							 radius[b], x[b], y[b]));
	//}
	
	for (int i=0, its=bodies-1; i<its; i++) {
		output_x[i] = x[i+1];
		output_y[i] = y[i+1];
	}
	
	int ms = sw.Time();
	secs_per_iter = (((double) ms)/1000.0) / ((double) num_iters);
	LOG_MSG(wxString::Format("CartogramNewView after %d iterations took %d ms",
							 num_iters, (int) ms));
	//wxMessageBox(wxString::Format("CartogramNewView after %d iterations took %ld ms",
	//							  num_iters, sw.Time()));
	return ms;
}

