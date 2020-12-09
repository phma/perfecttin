/******************************************************/
/*                                                    */
/* cogospiral.h - intersections of spirals            */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
 * This file is part of PerfectTIN.
 *
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License and Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and Lesser General Public License along with PerfectTIN. If not, see
 * <http://www.gnu.org/licenses/>.
 */
/* To find the intersection of two spirals, given two points near it on each:
 * 1. Find the intersection of the lines defined by the points.
 * 2. Interpolate or extrapolate a new point on each curve.
 * 3. Discard the farthest pair of points where one is on one curve and the other
 *    is on the other. If the farthest points are both the new points, or there
 *    are no new points because the lines are parallel, give up.
 * 4. If the closest points are identical, or on both curves the points are so
 *    close together that no point between them can be computed, you're done.
 *    Otherwise return to step 1.
 * To find all the intersections, pick points on both curves as in segment::closest,
 * then pick pairs of points on each curve and run the above algorithm.
 * 
 * As segments and arcs are special cases of spiralarcs, the algorithm works
 * for them too. Intersections of segments and arcs can be computed by formula;
 * intersections involving a spiral must be computed iteratively, since the
 * spiral cannot be written in closed form. Intersections involving an arc
 * of very small delta should be done iteratively, to avoid loss of precision.
 */
#include <vector>
#include <array>
#include "point.h"
#include "spiral.h"

/* Constants for Gaussian quadrature of sixth-degree polynomial resulting from
 * squaring a cubic, for calculating the RMS distance between a spiral and
 * the approximation thereto.
 * GAUSSQ4P0P=(1-sqrt(3/7+2/7*sqrt(6/5)))/2
 * GAUSSQ4P1P=(1-sqrt(3/7-2/7*sqrt(6/5)))/2
 * GAUSSQ4P0W=(18-sqrt(30))/72
 * GAUSSQ4P1W=(18+sqrt(30))/72
 * GAUSSQ4P2P=1-GAUSSQ4P1P
 * GAUSSQ4P3P=1-GAUSSQ4P0P
 * GAUSSQ4P2W=GAUSSQ4P1W
 * GAUSSQ4P3W=GAUSSQ4P0W
 */
#define GAUSSQ4P0P .06943184420297371239
#define GAUSSQ4P1P .33000947820757186760
#define GAUSSQ4P0W .17392742256872692868
#define GAUSSQ4P1W .32607257743127307131

struct alosta
{
  double along;
  xy station;
  int bearing;
  double curvature;
  alosta();
  alosta(double a,xy s);
  alosta(double a,xy s,int b,double c);
  void setStation(segment *seg,double alo);
};

std::vector<alosta> intersection1(segment *a,double a1,double a2,segment *b,double b1,double b2,bool extend=false);
std::vector<alosta> intersection1(segment *a,double a1,segment *b,double b1,bool extend=false);
std::vector<std::array<alosta,2> > intersections(segment *a,segment *b,bool extend=false);
