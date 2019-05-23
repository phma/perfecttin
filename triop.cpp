/******************************************************/
/*                                                    */
/* triop.cpp - triangle operation                     */
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

#include "octagon.h"
#include "triop.h"

void split(triangle *tri)
/* Inserts a new point in the triangle, then moves the dots that are in the
 * two new triangles to them.
 */
{
  // lock
  point newPoint(((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3);
  int newPointNum=net.points.size()+1;
  net.addpoint(newPointNum,newPoint);
  point *pnt=&net.points[newPointNum];
  int newEdgeNum=net.edges.size();
  net.edges[newEdgeNum].a=pnt;
  net.edges[newEdgeNum].b=tri->a;
  net.edges[newEdgeNum+1].a=pnt;
  net.edges[newEdgeNum+1].b=tri->b;
  net.edges[newEdgeNum+2].a=pnt;
  net.edges[newEdgeNum+2].b=tri->c;
  // ...
  // unlock
}
