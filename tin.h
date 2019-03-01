/******************************************************/
/*                                                    */
/* tin.h - triangulated irregular network             */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of Decisite.
 * 
 * Decisite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Decisite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decisite. If not, see <http://www.gnu.org/licenses/>.
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
  double extrema[2];
  char broken;
  /* Bit 0 is used in viewtin to mean that the edge is in a type-0 breakline.
   * Bit 0 is used when making the TIN to mean that the edge coincides with a type-0 breakline.
   * Bit 1 means that the edge crosses a type-0 breakline.
   * If bits 0 and 1 are both set, two type-0 breaklines cross, which is an error.
   * Bit 2 means that bits 0 and 1 are up to date.
   * Bit 3 means that a type-1 breakline crosses the edge.
   */
  char contour;
  /* When drawing a contour, set one of the low 3 bits of edge::contour to true
   * for each edge that crosses the contour. Keep the flags set when you go to
   * the next contour of the same elevation. When you go to the next elevation,
   * clear the flags.
   */
  unsigned char stlmin;
  // Code for the minimum number of pieces this edge must be split into.
  unsigned char stlsplit;
  /* Smooth number code of the number of pieces this edge is split into
   * when writing an STL file.
   */
  short flipcnt;
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
  segment getsegment();
  std::array<double,4> ctrlpts();
  xyz critpoint(int i);
  void findextrema();
  void clearmarks();
  void mark(int n);
  bool ismarked(int n);
  void stlSplit(double maxError);
};

typedef std::pair<double,point*> ipoint;

#endif
