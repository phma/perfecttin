/******************************************************/
/*                                                    */
/* triop.cpp - triangle operation                     */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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

point *split(triangle *tri,int thread)
/* Inserts a new point in the triangle, then moves the dots that are in the
 * two new triangles to them.
 */
{
  edge *sidea,*sideb,*sidec;
  triangle *newt0,*newt1;
  edge *newe0,*newe1,*newe2;
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
  int newTriNum=net.addtriangle(2,thread);
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
  tri->flatten();
  newt0->flatten();
  newt1->flatten();
  recordTriop();
  dealDots(tri,newt0,newt1);
  return pnt;
}

array<point *,3> quarter(triangle *tri,int thread)
/* Adds the midpoints of the three sides as new points and breaks tri into quarters
 * and the three adjacent triangles into halves.
 */
{
  int i;
  array<point *,3> pnts;
  array<edge *,9> eds;
  array<triangle *,6> tris;
  point *oppA,*oppB,*oppC;
  edge *sidea,*sideb,*sidec;
  triangle *neigha,*neighb,*neighc;
  net.wingEdge.lock();
  point *A=tri->a,*B=tri->b,*C=tri->c;
  point midA(((xyz)*B+(xyz)*C)/2);
  point midB(((xyz)*C+(xyz)*A)/2);
  point midC(((xyz)*A+(xyz)*B)/2);
  int newPointNum=net.points.size()+1;
  int newTriNum=net.addtriangle(6,thread);
  /* The new triangles must be created locked, because they are adjacent
   * in pairs, else another thread might try to flip the edge between them.
   */
  int newEdgeNum=net.edges.size();
  net.addpoint(newPointNum,midA);
  net.addpoint(newPointNum,midB);
  net.addpoint(newPointNum,midC);
  pnts[0]=&net.points[newPointNum];
  pnts[1]=&net.points[newPointNum+1];
  pnts[2]=&net.points[newPointNum+2];
  for (i=0;i<9;i++)
    eds[i]=&net.edges[newEdgeNum+i];
  for (i=0;i<6;i++)
    tris[i]=&net.triangles[newTriNum+i];
  sidea=C->edg(tri);
  sideb=A->edg(tri);
  sidec=B->edg(tri);
  neigha=tri->aneigh;
  neighb=tri->bneigh;
  neighc=tri->cneigh;
  oppA=neigha->otherCorner(B,C);
  oppB=neighb->otherCorner(C,A);
  oppC=neighc->otherCorner(A,B);
  A->removeEdge(sideb);
  A->removeEdge(sidec);
  B->removeEdge(sidec);
  B->removeEdge(sidea);
  C->removeEdge(sidea);
  C->removeEdge(sideb);
  sidea->a=pnts[1];
  sidea->b=pnts[2];
  sideb->a=pnts[2];
  sideb->b=pnts[0];
  sidec->a=pnts[0];
  sidec->b=pnts[1];
  pnts[0]->insertEdge(sideb);
  pnts[0]->insertEdge(sidec);
  pnts[1]->insertEdge(sidec);
  pnts[1]->insertEdge(sidea);
  pnts[2]->insertEdge(sidea);
  pnts[2]->insertEdge(sideb);
  eds[0]->a=A;
  eds[0]->b=pnts[2];
  eds[1]->a=pnts[2];
  eds[1]->b=B;
  eds[2]->a=B;
  eds[2]->b=pnts[0];
  eds[3]->a=pnts[0];
  eds[3]->b=C;
  eds[4]->a=C;
  eds[4]->b=pnts[1];
  eds[5]->a=pnts[1];
  eds[5]->b=A;
  eds[6]->a=pnts[0];
  eds[6]->b=oppA;
  eds[7]->a=pnts[1];
  eds[7]->b=oppB;
  eds[8]->a=pnts[2];
  eds[8]->b=oppC;
  tri->a=pnts[0];
  tri->b=pnts[1];
  tri->c=pnts[2];
  tris[0]->a=A;
  tris[0]->b=pnts[2];
  tris[0]->c=pnts[1];
  tris[1]->a=pnts[2];
  tris[1]->b=B;
  tris[1]->c=pnts[0];
  tris[2]->a=pnts[1];
  tris[2]->b=pnts[0];
  tris[2]->c=C;
  tris[3]->a=oppA;
  tris[3]->b=pnts[0];
  tris[3]->c=B;
  tris[4]->a=C;
  tris[4]->b=oppB;
  tris[4]->c=pnts[1];
  tris[5]->a=pnts[2];
  tris[5]->b=A;
  tris[5]->c=oppC;
  if (neigha->a==B)
    neigha->a=pnts[0];
  if (neigha->b==B)
    neigha->b=pnts[0];
  if (neigha->c==B)
    neigha->c=pnts[0];
  if (neighb->a==C)
    neighb->a=pnts[1];
  if (neighb->b==C)
    neighb->b=pnts[1];
  if (neighb->c==C)
    neighb->c=pnts[1];
  if (neighc->a==A)
    neighc->a=pnts[2];
  if (neighc->b==A)
    neighc->b=pnts[2];
  if (neighc->c==A)
    neighc->c=pnts[2];
  for (i=0;i<9;i++)
  {
    eds[i]->a->insertEdge(eds[i]);
    eds[i]->b->insertEdge(eds[i]);
  }
  tri->setEdgeTriPointers();
  neigha->setEdgeTriPointers();
  neighb->setEdgeTriPointers();
  neighc->setEdgeTriPointers();
  for (i=0;i<6;i++)
    tris[i]->setEdgeTriPointers();
  tri->setEdgeTriPointers();
  for (i=0;i<9;i++)
    eds[i]->setNeighbors();
  for (i=6;i<9;i++) // This sets the neighbor pointers of the triangles adjacent
    eds[i]->nextb->setNeighbors(); // to the part of the TIN affected.
  sidea->setNeighbors();
  sideb->setNeighbors();
  sidec->setNeighbors();
  net.wingEdge.unlock();
  recordTriop();
  for (i=0;i<6;i++)
    tris[i]->flatten();
  tri->flatten();
  neigha->flatten();
  neighb->flatten();
  neighc->flatten();
  dealDots(tri,tris[0],tris[1],tris[2]);
  dealDots(neigha,tris[3]);
  dealDots(neighb,tris[4]);
  dealDots(neighc,tris[5]);
  return pnts;
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

bool shouldSplit(triangle *tri,double tolerance,double minArea)
{
  if (!tri->inTolerance(tolerance,minArea))
    tri->setError(tolerance);
  return !tri->inTolerance(tolerance,minArea);
}

bool shouldQuarter(triangle *tri,double tolerance,double minArea)
{
  int i,qbits=7;
  if (tri->sarea<4*minArea)
    qbits=0;
  if (tri->aneigh==nullptr || tri->bneigh==nullptr || tri->cneigh==nullptr)
    qbits=0;
  //if (qbits && tri->dots.size()+tri->aneigh->dots.size()+tri->bneigh->dots.size()+tri->cneigh->dots.size()<10)
    //qbits=0;
  if (qbits && (tri->aneigh->sarea<2*minArea || tri->bneigh->sarea<2*minArea || tri->cneigh->sarea<2*minArea))
    qbits=0;
  for (i=0;qbits && i<tri->dots.size();i++)
    qbits&=tri->quadrant(tri->dots[i]);
  return qbits>0 && qbits<7;
}

int triop(triangle *tri,double tolerance,double minArea,int thread)
/* Returns true if it got a lock. If all three edgeop calls didn't get a lock,
 * it still returns true.
 */
{
  vector<point *> corners;
  array<point *,3> midpoints;
  edge *sidea,*sideb,*sidec;
  bool spl,qtr;
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
  if (gotLock1 && (qtr=shouldQuarter(tri,tolerance,minArea)))
  {
    triNeigh=triangleNeighbors(corners);
    gotLock2=lockTriangles(thread,triNeigh);
    if (gotLock2)
    {
      midpoints=quarter(tri,thread);
      corners.push_back(midpoints[0]);
      corners.push_back(midpoints[1]);
      corners.push_back(midpoints[2]);
      triNeigh=triangleNeighbors(corners);
      tri->unsetError();
      logAdjustment(adjustElev(triNeigh,corners));
    }
  }
  if (gotLock1 && !qtr && (spl=shouldSplit(tri,tolerance,minArea)))
  {
    triNeigh=triangleNeighbors(corners);
    gotLock2=lockTriangles(thread,triNeigh);
    if (gotLock2)
    {
      corners.push_back(split(tri,thread));
      triNeigh=triangleNeighbors(corners);
      tri->unsetError();
      logAdjustment(adjustElev(triNeigh,corners));
      edgeop(sidea,tolerance,minArea,thread);
      edgeop(sideb,tolerance,minArea,thread);
      edgeop(sidec,tolerance,minArea,thread);
    }
  }
  unlockTriangles(thread);
  return gotLock1*2+gotLock2; // 2 means deadlock
}
