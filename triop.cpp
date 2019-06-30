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

#include <cassert>
#include <cmath>
#include "neighbor.h"
#include "adjelev.h"
#include "octagon.h"
#include "triop.h"
#include "edgeop.h"
#include "angle.h"
#include "threads.h"

using namespace std;

point *split(triangle *tri)
/* Inserts a new point in the triangle, then moves the dots that are in the
 * two new triangles to them.
 */
{
  edge *sidea,*sideb,*sidec;
  vector<xyz> remainder; // the dots that remain in tri
  int i;
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
  pnt->line=&net.edges[newEdgeNum];
  sidea=tri->c->edg(tri);
  sideb=tri->a->edg(tri);
  sidec=tri->b->edg(tri);
  net.edges[newEdgeNum  ].setnext(pnt,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(pnt,&net.edges[newEdgeNum+2]);
  net.edges[newEdgeNum+2].setnext(pnt,&net.edges[newEdgeNum  ]);
  sidea->setnext(tri->b,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(tri->b,sidec);
  sidec->setnext(tri->a,&net.edges[newEdgeNum  ]);
  net.edges[newEdgeNum  ].setnext(tri->b,sideb);
  sideb->setnext(tri->c,&net.edges[newEdgeNum+2]);
  net.edges[newEdgeNum+2].setnext(tri->b,sidea);
  int newTriNum=net.addtriangle(2);
  net.triangles[newTriNum].a=tri->a;
  net.triangles[newTriNum].b=tri->b;
  net.triangles[newTriNum].c=pnt;
  net.triangles[newTriNum+1].a=tri->a;
  net.triangles[newTriNum+1].b=pnt;
  net.triangles[newTriNum+1].c=tri->c;
  tri->a=pnt;
  net.edges[newEdgeNum  ].tria=net.edges[newEdgeNum+2].trib=&net.triangles[newTriNum+1];
  net.edges[newEdgeNum+1].tria=net.edges[newEdgeNum  ].trib=&net.triangles[newTriNum];
  net.edges[newEdgeNum+2].tria=net.edges[newEdgeNum+1].trib=tri;
  if (sideb->tria==tri)
    sideb->tria=&net.triangles[newTriNum+1];
  if (sideb->trib==tri)
    sideb->trib=&net.triangles[newTriNum+1];
  if (sidec->tria==tri)
    sidec->tria=&net.triangles[newTriNum];
  if (sidec->trib==tri)
    sidec->trib=&net.triangles[newTriNum];
  sidea->setNeighbors();
  sideb->setNeighbors();
  sidec->setNeighbors();
  net.edges[newEdgeNum  ].setNeighbors();
  net.edges[newEdgeNum+1].setNeighbors();
  net.edges[newEdgeNum+2].setNeighbors();
  //assert(net.checkTinConsistency());
  for (i=0;i<tri->dots.size();i++)
    if (net.triangles[newTriNum].in(tri->dots[i]))
      net.triangles[newTriNum].dots.push_back(tri->dots[i]);
    else if (net.triangles[newTriNum+1].in(tri->dots[i]))
      net.triangles[newTriNum+1].dots.push_back(tri->dots[i]);
    else
      remainder.push_back(tri->dots[i]);
    swap(tri->dots,remainder);
  tri->dots.shrink_to_fit();
  net.triangles[newTriNum].dots.shrink_to_fit();
  net.triangles[newTriNum+1].dots.shrink_to_fit();
  tri->flatten();
  net.triangles[newTriNum].flatten();
  net.triangles[newTriNum+1].flatten();
  // unlock
  return pnt;
}

bool lockTriangles(int thread,vector<triangle *> triPtr)
{
  vector<int> triangles;
  int i;
  for (i=0;i<triPtr.size();i++)
    triangles.push_back(net.revtriangles[triPtr[i]]);
  return lockTriangles(thread,triangles);
}

bool shouldSplit(triangle *tri,double tolerance)
{
  if (!tri->inTolerance(tolerance))
    tri->setError(tolerance);
  return !tri->inTolerance(tolerance);
}

void triop(triangle *tri,double tolerance,int thread)
{
  vector<point *> corners;
  edge *sidea,*sideb,*sidec;
  bool spl;
  corners.push_back(tri->a);
  corners.push_back(tri->b);
  corners.push_back(tri->c);
  sidea=tri->c->edg(tri);
  sideb=tri->a->edg(tri);
  sidec=tri->b->edg(tri);
  if (spl=shouldSplit(tri,tolerance))
  {
    corners.push_back(split(tri));
    tri->unsetError();
    logAdjustment(adjustElev(triangleNeighbors(corners),corners));
    edgeop(sidea,tolerance,thread);
    edgeop(sideb,tolerance,thread);
    edgeop(sidec,tolerance,thread);
  }
}
