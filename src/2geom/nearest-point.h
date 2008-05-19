/*
 * nearest point routines for D2<SBasis> and Piecewise<D2<SBasis>>
 *
 *
 * Authors:
 * 		
 * 		Marco Cecchetti <mrcekets at gmail.com>
 * 
 * Copyright 2007-2008  authors
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 */


#ifndef _NEAREST_POINT_H_
#define _NEAREST_POINT_H_


#include <vector>

#include "d2.h"
#include "piecewise.h"
#include "exception.h"



namespace Geom
{

/*
 * Given a line L specified by a point A and direction vector v,
 * return the point on L nearest to p. Note that the returned value
 * is with respect to the _normalized_ direction of v!
 */
inline double nearest_point(Point const &p, Point const &A, Point const &v)
{
    Point d(p - A);
    return d[0] * v[0] + d[1] * v[1];
}

////////////////////////////////////////////////////////////////////////////////
// D2<SBasis> versions

/*
 * Return the parameter t of a nearest point on the portion of the curve "c", 
 * related to the interval [from, to], to the point "p".
 * The needed curve derivative "dc" is passed as parameter.
 * The function return the first nearest point to "p" that is found.
 */
double nearest_point( Point const& p,
		              D2<SBasis> const& c, D2<SBasis> const& dc, 
		              double from = 0, double to = 1 );

inline
double nearest_point( Point const& p, 
		              D2<SBasis> const& c, 
		              double from = 0, double to = 1 )
{
	return nearest_point(p, c, Geom::derivative(c), from, to);
}

/*
 * Return the parameters t of all the nearest points on the portion of 
 * the curve "c", related to the interval [from, to], to the point "p".
 * The needed curve derivative "dc" is passed as parameter.
 */
std::vector<double> 
all_nearest_points( Point const& p, 
		            D2<SBasis> const& c, D2<SBasis> const& dc, 
		            double from = 0, double to = 1 );

inline
std::vector<double> 
all_nearest_points( Point const& p, 
		            D2<SBasis> const& c, 
		            double from = 0, double to = 1 )
{
	return all_nearest_points(p, c,  Geom::derivative(c), from, to);
}


////////////////////////////////////////////////////////////////////////////////
// Piecewise< D2<SBasis> > versions

double nearest_point( Point const& p, 
		              Piecewise< D2<SBasis> > const& c, 
		              double from, double to );

inline
double nearest_point( Point const& p, Piecewise< D2<SBasis> > const& c ) 
{
	return nearest_point(p, c, c.cuts[0], c.cuts[c.size()]);
}


std::vector<double> 
all_nearest_points( Point const& p, 
					Piecewise< D2<SBasis> > const& c, 
		            double from, double to );

inline
std::vector<double> 
all_nearest_points( Point const& p, Piecewise< D2<SBasis> > const& c ) 
{
	return all_nearest_points(p, c, c.cuts[0], c.cuts[c.size()]);
}

} // end namespace Geom



#endif /*_NEAREST_POINT_H_*/
