/******************************************************/
/*                                                    */
/* triop.cpp - triangle operation                     */
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
  triangle *new0,*new1;
  vector<xyz> remainder; // the dots that remain in tri
  int i;
  wingEdge.lock();
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
  new0=&net.triangles[newTriNum];
  new1=&net.triangles[newTriNum+1];
  new0->a=tri->a;
  new0->b=tri->b;
  new0->c=pnt;
  new1->a=tri->a;
  new1->b=pnt;
  new1->c=tri->c;
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
  wingEdge.unlock();
  for (i=0;i<tri->dots.size();i++)
    if (new0->in(tri->dots[i]))
      new0->dots.push_back(tri->dots[i]);
    else if (new1->in(tri->dots[i]))
      new1->dots.push_back(tri->dots[i]);
    else
      remainder.push_back(tri->dots[i]);
    swap(tri->dots,remainder);
  tri->dots.shrink_to_fit();
  new0->dots.shrink_to_fit();
  new1->dots.shrink_to_fit();
  tri->flatten();
  new0->flatten();
  new1->flatten();
  return pnt;
}

bool lockTriangles(int thread,vector<triangle *> triPtr)
{
  vector<int> triangles;
  int i;
  wingEdge.lock_shared();
  for (i=0;i<triPtr.size();i++)
    triangles.push_back(net.revtriangles[triPtr[i]]);
  wingEdge.unlock_shared();
  return lockTriangles(thread,triangles);
}

bool shouldSplit(triangle *tri,double tolerance)
{
  if (!tri->inTolerance(tolerance))
    tri->setError(tolerance);
  return !tri->inTolerance(tolerance);
}

int triop(triangle *tri,double tolerance,int thread)
/* Returns true if it got a lock. If all three edgeop calls didn't get a lock,
 * it still returns true.
 */
{
  vector<point *> corners;
  edge *sidea,*sideb,*sidec;
  bool spl;
  bool gotLock1,gotLock2=true;
  vector<triangle *> triNeigh;
  triNeigh.push_back(tri);
  gotLock1=lockTriangles(thread,triNeigh);
  if (gotLock1)
  {
    corners.push_back(tri->a);
    corners.push_back(tri->b);
    corners.push_back(tri->c);
    sidea=tri->c->edg(tri);
    sideb=tri->a->edg(tri);
    sidec=tri->b->edg(tri);
  }
  if (gotLock1 && (spl=shouldSplit(tri,tolerance)))
  {
    triNeigh=triangleNeighbors(corners);
    gotLock2=lockTriangles(thread,triNeigh);
    if (gotLock2)
    {
      corners.push_back(split(tri));
      triNeigh=triangleNeighbors(corners);
      tri->unsetError();
      logAdjustment(adjustElev(triNeigh,corners));
      edgeop(sidea,tolerance,thread);
      edgeop(sideb,tolerance,thread);
      edgeop(sidec,tolerance,thread);
    }
  }
  unlockTriangles(thread);
  return gotLock1*2+gotLock2; // 2 means deadlock
}
