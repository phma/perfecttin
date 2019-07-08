/*                                                    */
/* cogo.h - coordinate geometry                       */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 * 
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PerfectTIN. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COGO_H
#define COGO_H
#include "point.h"

#define sign(x) ((x>0)-(x<0))
#define IN_AT_CORNER 0x69969669
/* IN_AT_CORNER is set so that if you add a few of them, as results of in()
 * are computed by adding results of in() for smaller components, you will
 * get an obviously wrong answer.
 */

enum inttype {NOINT, ACXBD, BDTAC, ACTBD, ACVBD, COINC, COLIN, IMPOS};
extern int debugdel;
extern FILE *randfil;

double area3(xy a,xy b,xy c);
xy intersection (xy a,xy c,xy b,xy d);
//Intersection of lines ac and bd.
xy intersection (xy a,int aBear,xy b,int bBear);
//Bearing-bearing intersection.
double missDistance (xy a,xy c,xy b,xy d);
//Distance by which lines miss intersecting.
inttype intersection_type(xy a,xy c,xy b,xy d);
/* NOINT  don't intersect
   ACXBD  intersection is in the midst of both AC and BD
   BDTAC  one end of BD is in the midst of AC
   ACTBD  one end of AC is in the midst of BD
   ACVBD  one end of AC is one end of BD
   COINC  A=C or B=D
   COLIN  all four points are collinear
   IMPOS  impossible, probably caused by roundoff error
   */
double in3(xy p,xy a,xy b,xy c);
/* Returns the winding number of abc arount p.
 * If abc is counterclockwise and p is inside it, returns 1.
 * If p is on the boundary of abc, returns 0.5 or -0.5, depending on the orientation
 * of abc.
 * If abc is flat, returns 0.
 * If p is one of a, b, and c, and abc is not flat, returns the angle
 * as a fraction of a rotation.
 */
bool crossTriangle(xy p,xy q,xy a,xy b,xy c);
/* Returns true if the segment pq crosses the triangle abc.
 * However, if pq passes through a and has one end on bc, returns false.
 */
double pldist(xy a,xy b,xy c);
// Signed distance from a to the line bc.
bool delaunay(xy a,xy c,xy b,xy d);
//Returns true if ac satisfies the criterion in the quadrilateral abcd.
//If false, the edge should be flipped to bd.
xy rand2p(xy a,xy b);
/* A random point in the circle with diameter ab. */
char *inttype_str(inttype i);
double distanceInDirection(xy a,xy b,int dir);
// Distance from a to b in the direction dir. Used for tangents of spirals.
#endif
