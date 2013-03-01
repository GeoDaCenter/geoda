/**
 * GeoDa TM, Copyright (C) 2011-2013 by Luc Anselin - all rights reserved
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

#ifndef __GEODA_CENTER_LINK_H__
#define __GEODA_CENTER_LINK_H__

class Link  
{
public :
    Link() {};
    Link(const int ix, const double &v) : nb(ix), weight(v) {};
    int			getIx()  const  {  return nb;  }
    double	getWeight()  const   {  return weight;  }
    void		setIx(const int x)  {  nb = x;  }
    void		setWeight(const double &v)  {  weight = v;  }
    Link & operator = (const Link & from)  
		{
        this-> nb = from.nb;
        this->weight = from.weight;
        return *this;
    }
    bool operator < (const Link & from)  
		{
        return (this->nb < from.nb);
    }
private :
	int				nb;
  double		weight;

};

#endif

