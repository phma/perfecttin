/******************************************************/
/*                                                    */
/* qindex.h - quad index to tin                       */
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

#ifndef QINDEX_H
#define QINDEX_H
#include <vector>
#include <set>
#include "pointlist.h"
#include "triangle.h"
#include "ps.h"
#include "point.h"

class PostScript;

#if defined(_WIN32) || defined(__CYGWIN__)
double significand(double x);
#endif

class qindex
{
public:
  double x,y,side;
  union
  {
    qindex *sub[4]; // Either all four subs are set,
    triangle *tri;  // or tri alone is set,
    point *pnt[3];  // or up to three pnts are set, or they're all NULL.
  };
  triangle *findt(xy pnt,bool clip=false);
  point *findp(xy pont,bool clip=false);
  void insertPoint(point *pont,bool clip=false);
  int quarter(xy pnt,bool clip=false);
  xy middle();
  void sizefit(std::vector<xy> pnts);
  void split(std::vector<xy> pnts);
  void clear();
  void clearLeaves();
  void draw(PostScript &ps,bool root=true);
  std::vector<qindex*> traverse(int dir=0);
  void settri(triangle *starttri);
  std::set<triangle *> localTriangles(xy center,double radius,int max);
  qindex();
  ~qindex();
  int size(); // This returns the total number of nodes, which is 4n+1. The number of leaves is 3n+1.
};
#endif
