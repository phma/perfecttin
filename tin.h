/******************************************************/
/*                                                    */
/* tin.h - triangulated irregular network             */
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

#ifndef TIN_H
#define TIN_H
#include <exception>
#include <map>
#include <vector>
#include <array>
#include <string>
#include "cogo.h"
#include "triangle.h"
#include "pointlist.h"

class pointlist;

class edge
{
public:
  point *a,*b;
  edge *nexta,*nextb;
  // nexta points to the next edge counterclockwise about a.
  triangle *tria,*trib;
  // When going from a to b, tria is on the right and trib on the left.
  edge();
  void flip(pointlist *topopoints);
  void reverse();
  point* otherend(point* end);
  triangle* othertri(triangle* t);
  edge* next(point* end);
  triangle* tri(point* end);
  xy midpoint();
  void setnext(point* end,edge* enext);
  int bearing(point *end);
  void setNeighbors();
  bool isinterior();
  bool isFlippable();
  bool delaunay();
  void dump(pointlist *topopoints);
  double length();
};

typedef std::pair<double,point*> ipoint;

#endif
