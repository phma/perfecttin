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
  triangle *newt0,*newt1;
  edge *newe0,*newe1,*newe2;
  vector<xyz> remainder; // the dots that remain in tri
  int i;
  net.wingEdge.lock();
  point newPoint(((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3);
  int newPointNum=net.points.size()+1;
  net.addpoint(newPointNum,newPoint);
  point *pnt=&net.points[newPointNum];
  int newEdgeNum=net.edges.size();
  newe0=&net.edges[newEdgeNum];
  newe1=&net.edges[newEdgeNum+1];
  newe2=&net.edges[newEdgeNum+2];
  newe0->a=pnt;
  newe0->b=tri->a;
  newe1->a=pnt;
  newe1->b=tri->b;
  newe2->a=pnt;
  newe2->b=tri->c;
  pnt->line=newe0;
  sidea=tri->c->edg(tri);
  sideb=tri->a->edg(tri);
  sidec=tri->b->edg(tri);
  newe0->setnext(pnt,newe1);
  newe1->setnext(pnt,newe2);
  newe2->setnext(pnt,newe0);
  sidea->setnext(tri->b,newe1);
  newe1->setnext(tri->b,sidec);
  sidec->setnext(tri->a,newe0);
  newe0->setnext(tri->b,sideb);
  sideb->setnext(tri->c,newe2);
  newe2->setnext(tri->b,sidea);
  int newTriNum=net.addtriangle(2);
  newt0=&net.triangles[newTriNum];
  newt1=&net.triangles[newTriNum+1];
  newt0->a=tri->a;
  newt0->b=tri->b;
  newt0->c=pnt;
  newt1->a=tri->a;
  newt1->b=pnt;
  newt1->c=tri->c;
  tri->a=pnt;
  newe0->tria=newe2->trib=newt1;
  newe1->tria=newe0->trib=newt0;
  newe2->tria=newe1->trib=tri;
  if (sideb->tria==tri)
    sideb->tria=newt1;
  if (sideb->trib==tri)
    sideb->trib=newt1;
  if (sidec->tria==tri)
    sidec->tria=newt0;
  if (sidec->trib==tri)
    sidec->trib=newt0;
  sidea->setNeighbors();
  sideb->setNeighbors();
  sidec->setNeighbors();
  newe0->setNeighbors();
  newe1->setNeighbors();
  newe2->setNeighbors();
  //assert(net.checkTinConsistency());
  net.wingEdge.unlock();
  for (i=0;i<tri->dots.size();i++)
    if (newt0->in(tri->dots[i]))
      newt0->dots.push_back(tri->dots[i]);
    else if (newt1->in(tri->dots[i]))
      newt1->dots.push_back(tri->dots[i]);
    else
      remainder.push_back(tri->dots[i]);
    swap(tri->dots,remainder);
  tri->dots.shrink_to_fit();
  newt0->dots.shrink_to_fit();
  newt1->dots.shrink_to_fit();
  tri->flatten();
  newt0->flatten();
  newt1->flatten();
  return pnt;
}

bool lockTriangles(int thread,vector<triangle *> triPtr)
{
  vector<int> triangles;
  int i;
  net.wingEdge.lock_shared();
  for (i=0;i<triPtr.size();i++)
    triangles.push_back(net.revtriangles[triPtr[i]]);
  net.wingEdge.unlock_shared();
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
