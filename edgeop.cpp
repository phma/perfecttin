/******************************************************/
/*                                                    */
/* edgeop.cpp - edge operation                        */
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

#include <cstring>
#include <cassert>
#include "tin.h"
#include "angle.h"
#include "edgeop.h"
#include "octagon.h"
#include "triop.h"
#include "neighbor.h"
#include "threads.h"
#include "adjelev.h"

#define CRITLOGSIZE 24
using namespace std;

map<int,pointlist> tempPointlist;
/* Small pointlists, 5 points and 4 triangles, used for deciding whether to
 * flip an edge. One per worker thread.
 */
double critLog[CRITLOGSIZE];
set<edge *> edgesFlippedSet; // Records all edges flipped since the last
vector<edge *> edgesFlippedVector; // triangle operation.
shared_mutex edgesFlippedMutex;

DealBlockTask::DealBlockTask()
{
  dots=nullptr;
  result=nullptr;
  for (numDots=0;numDots<4;numDots++)
    tri[numDots]=nullptr;
  numDots=0;
}

void logCrit(double crit)
{
  memmove(critLog,critLog+1,(CRITLOGSIZE-1)*sizeof(double));
  critLog[CRITLOGSIZE-1]=crit;
}

void initTempPointlist(int nthreads)
{
  int i;
  for (i=0;i<nthreads;i++)
    tempPointlist[i];
}

void recordFlip(edge *e)
{
  edgesFlippedMutex.lock();
  edgesFlippedSet.insert(e);
  edgesFlippedVector.push_back(e);
  edgesFlippedMutex.unlock();
}

void recordTriop()
{
  edgesFlippedMutex.lock();
  edgesFlippedSet.clear();
  edgesFlippedVector.clear();
  edgesFlippedMutex.unlock();
}

bool runFlips(edge *e)
{
  edgesFlippedMutex.lock_shared();
  bool ret=edgesFlippedVector.size()>3*edgesFlippedSet.size() && edgesFlippedSet.count(e);
  edgesFlippedMutex.unlock_shared();
  return ret;
}

void computeDealBlock(DealBlockTask &task)
{
  int i,j,x,p2;
  size_t sz;
  for (p2=1;p2<=task.numDots;p2*=2);
  if (p2>task.numDots)
    p2/=2;
  x=0x55555555&(p2-1);
  for (i=0;i<task.numDots;i++)
  {
    if (i==p2)
      x=0;
    j=i^x;
    assert(!task.result->ready);
    if (task.tri[1]->in(task.dots[j]))
      task.result->dots[1].push_back(task.dots[j]);
    else if (task.tri[2] && task.tri[2]->in(task.dots[j]))
      task.result->dots[2].push_back(task.dots[j]);
    else if (task.tri[3] && task.tri[3]->in(task.dots[j]))
      task.result->dots[3].push_back(task.dots[j]);
    else
      task.result->dots[0].push_back(task.dots[j]);
  }
  if (task.result)
    task.result->ready=true;
}

void dealDots(int thread,triangle *tri0,triangle *tri1,triangle *tri2,triangle *tri3)
/* Places dots in their proper triangle. tri0 and tri1 may not be null,
 * but tri2 and tri3 may.
 */
{
  int i,j,x,p2,triDots;
  size_t sz;
  int totalDots[4];
  vector<xyz> remainder; // the dots that remain in tri0
  vector<DealBlockTask> tasks;
  map<int,DealBlockResult> results;
  vector<int> blkSizes;
  bool allReady=false;
  if (tri1->dots.size())
  {
    sz=tri0->dots.size();
    tri0->dots.resize(tri0->dots.size()+tri1->dots.size());
    memmove((void *)&tri0->dots[sz],(void *)&tri1->dots[0],tri1->dots.size()*sizeof(xyz));
    tri1->dots.clear();
  }
  if (tri2 && tri2->dots.size())
  {
    sz=tri0->dots.size();
    tri0->dots.resize(tri0->dots.size()+tri2->dots.size());
    memmove((void *)&tri0->dots[sz],(void *)&tri2->dots[0],tri2->dots.size()*sizeof(xyz));
    tri2->dots.clear();
  }
  if (tri3 && tri3->dots.size())
  {
    sz=tri0->dots.size();
    tri0->dots.resize(tri0->dots.size()+tri3->dots.size());
    memmove((void *)&tri0->dots[sz],(void *)&tri3->dots[0],tri3->dots.size()*sizeof(xyz));
    tri3->dots.clear();
  }
  if (tri0->dots.size()>98304)
  {
    blkSizes=blockSizes(tri0->dots.size());
    for (triDots=i=0;i<blkSizes.size();i++)
    {
      results[tasks.size()];
      tasks.resize(tasks.size()+1);
      tasks.back().tri[0]=tri0;
      tasks.back().tri[1]=tri1;
      tasks.back().tri[2]=tri2;
      tasks.back().tri[3]=tri3;
      tasks.back().dots=&tri0->dots[triDots];
      tasks.back().numDots=blkSizes[i];
      triDots+=blkSizes[i];
    }
    for (i=0;i<tasks.size();i++)
    {
      tasks[i].result=&results[i];
      tasks[i].thread=thread;
      results[i].ready=false;
    }
    for (i=0;i<tasks.size();i++)
      enqueueDeal(tasks[i]);
    while (!allReady)
    {
      if (!dealQueueEmpty())
      {
	DealBlockTask task=dequeueDeal();
	computeDealBlock(task);
      }
      allReady=true;
      for (i=0;i<results.size();i++)
	allReady&=results[i].ready;
    }
    totalDots[0]=totalDots[1]=totalDots[2]=totalDots[3]=0;
    for (i=0;i<results.size();i++)
      for (j=0;j<4;j++)
	totalDots[j]+=results[i].dots[j].size();
    remainder.resize(totalDots[0]);
    for (triDots=i=0;i<results.size();i++)
    {
      if (results[i].dots[0].size())
	memmove((void *)&remainder[triDots],(void *)&results[i].dots[0][0],results[i].dots[0].size()*sizeof(xyz));
      triDots+=results[i].dots[0].size();
    }
    tri1->dots.resize(totalDots[1]);
    for (triDots=i=0;i<results.size();i++)
    {
      if (results[i].dots[1].size())
	memmove((void *)&tri1->dots[triDots],(void *)&results[i].dots[1][0],results[i].dots[1].size()*sizeof(xyz));
      triDots+=results[i].dots[1].size();
    }
    if (tri2)
      tri2->dots.resize(totalDots[2]);
    for (triDots=i=0;i<results.size();i++)
    {
      if (results[i].dots[2].size())
	memmove((void *)&tri2->dots[triDots],(void *)&results[i].dots[2][0],results[i].dots[2].size()*sizeof(xyz));
      triDots+=results[i].dots[2].size();
    }
    if (tri3)
      tri3->dots.resize(totalDots[3]);
    for (triDots=i=0;i<results.size();i++)
    {
      if (results[i].dots[3].size())
	memmove((void *)&tri3->dots[triDots],(void *)&results[i].dots[3][0],results[i].dots[3].size()*sizeof(xyz));
      triDots+=results[i].dots[3].size();
    }
  }
  else
  {
    for (p2=1;p2<=tri0->dots.size();p2*=2);
    if (p2>tri0->dots.size())
      p2/=2;
    x=0x55555555&(p2-1);
    for (i=0;i<tri0->dots.size();i++)
    {
      if (i==p2)
	x=0;
      j=i^x;
      if (tri1->in(tri0->dots[j]))
	tri1->dots.push_back(tri0->dots[j]);
      else if (tri2 && tri2->in(tri0->dots[j]))
	tri2->dots.push_back(tri0->dots[j]);
      else if (tri3 && tri3->in(tri0->dots[j]))
	tri3->dots.push_back(tri0->dots[j]);
      else
	remainder.push_back(tri0->dots[j]);
    }
  }
  remainder.shrink_to_fit();
  tri1->dots.shrink_to_fit();
  if (tri2)
    tri2->dots.shrink_to_fit();
  if (tri3)
    tri3->dots.shrink_to_fit();
  swap(tri0->dots,remainder);
}

void flip(edge *e,int thread)
{
  net.wingEdge.lock();
  e->flip(&net);
  //assert(net.checkTinConsistency());
  net.wingEdge.unlock();
  recordFlip(e);
  e->tria->flatten();
  e->trib->flatten();
  e->tria->unsetError();
  e->trib->unsetError();
  dealDots(thread,e->trib,e->tria);
}

point *bend(edge *e,int thread)
/* Inserts a new point, bending and breaking the edge e of the perimeter,
 * then flips the edge e.
 */
{
  edge *anext=e,*bnext=e;
  int abear,ebear,bbear;
  net.wingEdge.lock();
  do
    anext=anext->next(e->a);
  while (anext->isinterior());
  do
    bnext=bnext->next(e->b);
  while (bnext->isinterior());
  abear=anext->bearing(e->a);
  ebear=e->bearing(e->a);
  bbear=bnext->bearing(e->b);
  while (abs(abear-ebear)>DEG90)
    abear+=DEG180;
  while (abs(bbear-ebear)>DEG90)
    bbear+=DEG180;
  abear=ebear+(abear-ebear)/2;
  bbear=ebear+(bbear-ebear)/2;
  point newPoint(intersection(*e->a,abear,*e->b,bbear),(e->a->elev()+e->b->elev())/2);
  int newPointNum=net.points.size()+1;
  net.addpoint(newPointNum,newPoint);
  point *pnt=&net.points[newPointNum];
  int newEdgeNum=net.edges.size();
  net.edges[newEdgeNum  ].a=e->a;
  net.edges[newEdgeNum  ].b=pnt;
  net.edges[newEdgeNum+1].a=e->b;
  net.edges[newEdgeNum+1].b=pnt;
  pnt->line=&net.edges[newEdgeNum];
  net.edges[newEdgeNum  ].setnext(pnt,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(pnt,&net.edges[newEdgeNum  ]);
  int newTriNum=net.addtriangle(1,thread);
  net.triangles[newTriNum].a=pnt;
  // If abear-bbear<0, then e is counterclockwise around the TIN.
  if (abear-bbear<0)
  {
    net.triangles[newTriNum].b=e->b;
    net.triangles[newTriNum].c=e->a;
    anext->setnext(e->a,&net.edges[newEdgeNum]);
    net.edges[newEdgeNum  ].setnext(e->a,e);
    e->setnext(e->b,&net.edges[newEdgeNum+1]);
    net.edges[newEdgeNum+1].setnext(e->b,bnext);
    e->tria=&net.triangles[newTriNum];
    net.edges[newEdgeNum  ].trib=&net.triangles[newTriNum];
    net.edges[newEdgeNum+1].tria=&net.triangles[newTriNum];
    net.insertHullPoint(pnt,e->a);
  }
  else
  {
    net.triangles[newTriNum].b=e->a;
    net.triangles[newTriNum].c=e->b;
    bnext->setnext(e->b,&net.edges[newEdgeNum+1]);
    net.edges[newEdgeNum+1].setnext(e->b,e);
    e->setnext(e->a,&net.edges[newEdgeNum]);
    net.edges[newEdgeNum  ].setnext(e->a,anext);
    e->trib=&net.triangles[newTriNum];
    net.edges[newEdgeNum  ].tria=&net.triangles[newTriNum];
    net.edges[newEdgeNum+1].trib=&net.triangles[newTriNum];
    net.insertHullPoint(pnt,e->b);
  }
  e->setNeighbors();
  //assert(net.checkTinConsistency());
  net.wingEdge.unlock();
  flip(e,thread);
  return pnt;
}

bool spikyTriangle(xy a,xy b,xy c)
/* Returns true if the altitude of b is such that the angle A or C must be
 * less than a second of arc. Such a triangle should never appear in the TIN.
 */
{
  return dist(a,c)>412529.6*fabs(pldist(b,c,a));
}

bool shouldFlip(edge *e,double tolerance,double minArea,int thread)
/* Decides whether to flip an interior edge. If it is flippable (that is,
 * the two triangles form a convex quadrilateral), it decides based on
 * a combination of two criteria:
 * 1. They would fit the dots better after flipping than before.
 * 2. The triangles' circumcircles would not contain each other's other point. (Delaunay)
 */
{
  int i,j,k,triDots;
  array<triangle *,2> triab;
  triangle *tri;
  int totalDots[4];
  vector<DealBlockTask> tasks;
  map<int,DealBlockResult> results;
  vector<int> blkSizes;
  bool allReady=false;
  bool validTemp,ret=false,inTol,isSpiky,wouldbeSpiky;
  double elev13,elev24,elev5;
  double crit1=0,crit2=0;
  double areas[4];
  int ndots[4];
  vector<triangle *> alltris;
  vector<point *> allpoints;
  tempPointlist[thread].clear();
  tempPointlist[thread].addpoint(1,*e->a);
  tempPointlist[thread].addpoint(2,*e->nexta->otherend(e->a));
  tempPointlist[thread].addpoint(3,*e->b);
  tempPointlist[thread].addpoint(4,*e->nextb->otherend(e->b));
  elev5=(tempPointlist[thread].points[1].elev()+
	 tempPointlist[thread].points[2].elev()+
	 tempPointlist[thread].points[3].elev()+
	 tempPointlist[thread].points[4].elev())/4;
  tempPointlist[thread].addpoint(5,point(intersection(*e->a,*e->b,
			  *e->nextb->otherend(e->b),*e->nexta->otherend(e->a)),elev5));
  isSpiky=spikyTriangle(tempPointlist[thread].points[1],
			tempPointlist[thread].points[2],
			tempPointlist[thread].points[3]) ||
	  spikyTriangle(tempPointlist[thread].points[3],
			tempPointlist[thread].points[4],
			tempPointlist[thread].points[1]);
  wouldbeSpiky=spikyTriangle(tempPointlist[thread].points[4],
			     tempPointlist[thread].points[1],
			     tempPointlist[thread].points[2]) ||
	       spikyTriangle(tempPointlist[thread].points[2],
			     tempPointlist[thread].points[3],
			     tempPointlist[thread].points[4]);
  if (isSpiky && wouldbeSpiky)
    cout<<"spiky triangle\n";
  inTol=e->tria->inTolerance(tolerance,minArea)&&e->trib->inTolerance(tolerance,minArea);
  if (e->tria->dots.size()+e->trib->dots.size()<1)
    inTol=false; // Try not to have acicular triangles in holes
  if (!inTol)
  {
    for (i=0;i<4;i++)
    {
      tempPointlist[thread].edges[i].a=&tempPointlist[thread].points[i+1];
      tempPointlist[thread].edges[i].b=&tempPointlist[thread].points[(i+1)%4+1];
      tempPointlist[thread].edges[i+4].a=&tempPointlist[thread].points[i+1];
      tempPointlist[thread].edges[i+4].b=&tempPointlist[thread].points[5];
    }
    for (i=0;i<4;i++)
    {
      tempPointlist[thread].edges[i].nexta=&tempPointlist[thread].edges[(i+3)%4];
      tempPointlist[thread].edges[i].nextb=&tempPointlist[thread].edges[(i+1)%4+4];
      tempPointlist[thread].edges[i+4].nexta=&tempPointlist[thread].edges[i];
      tempPointlist[thread].edges[i+4].nextb=&tempPointlist[thread].edges[(i+3)%4+4];
    }
    for (i=1;i<6;i++)
      tempPointlist[thread].points[i].line=&tempPointlist[thread].edges[(i-1)%4+4];
    tempPointlist[thread].maketriangles();
    validTemp=tempPointlist[thread].checkTinConsistency();
    /*
    *           2
    *         / | \
    *      0/   5   \1
    *     /  0  |  1  \
    *   /       |       \
    * 1----4----5----6----3
    *   \       |       /
    *     \  3  |  2  /
    *      3\   7   /2
    *         \ | /
    *           4
    * Line 1-3 is the edge before flipping; line 2-4 is what it would be after.
    * It is possible for valid splittings to produce an invalid temporary pointlist.
    * Split △ABC at D, then split △ABD at E. C, D, and E are collinear. Then try
    * to flip BD. The temporary pointlist looks like this:
    * 2
    * | \
    * |   \
    * |  1  \
    * |       \
    * 1=5-------3
    * |       /
    * |  2  /
    * |   /
    * | /
    * 4
    * Because of roundoff error, it may appear to be flippable, but in fact is not.
    */
    for (i=0;i<4;i++)
      areas[i]=area3(tempPointlist[thread].points[(i+1)%4+1],
		    tempPointlist[thread].points[(i)%4+1],
		    tempPointlist[thread].points[5]);
    triab[0]=e->tria;
    triab[1]=e->trib;
    if (validTemp)
    {
      tri=&tempPointlist[thread].triangles[0];
      if (e->tria->dots.size()>98304 || e->trib->dots.size()>98304)
      {
	for (i=0;i<2;i++)
	{
	  blkSizes=blockSizes(triab[i]->dots.size());
	  for (triDots=j=0;j<blkSizes.size();j++)
	  {
	    results[tasks.size()];
	    tasks.resize(tasks.size()+1);
	    for (k=0;k<4;k++)
	      tasks.back().tri[k]=&tempPointlist[thread].triangles[k];
	    tasks.back().dots=&triab[i]->dots[triDots];
	    tasks.back().numDots=blkSizes[j];
	    triDots+=blkSizes[j];
	  }
	}
	for (i=0;i<tasks.size();i++)
	{
	  tasks[i].result=&results[i];
	  tasks[i].thread=thread;
	  results[i].ready=false;
	}
	for (i=0;i<tasks.size();i++)
	  enqueueDeal(tasks[i]);
	while (!allReady)
	{
	  if (!dealQueueEmpty())
	  {
	    DealBlockTask task=dequeueDeal();
	    computeDealBlock(task);
	  }
	  allReady=true;
	  for (i=0;i<results.size();i++)
	    allReady&=results[i].ready;
	}
	totalDots[0]=totalDots[1]=totalDots[2]=totalDots[3]=0;
	for (i=0;i<results.size();i++)
	  for (j=0;j<4;j++)
	    totalDots[j]+=results[i].dots[j].size();
	for (i=0;i<4;i++)
	{
	  tempPointlist[thread].triangles[i].dots.resize(totalDots[i]);
	  for (triDots=j=0;j<results.size();j++)
	  {
	    if (results[j].dots[i].size())
	      memmove((void *)&tempPointlist[thread].triangles[i].dots[triDots],(void *)&results[j].dots[i][0],results[j].dots[i].size()*sizeof(xyz));
	    triDots+=results[j].dots[i].size();
	  }
	}
      }
      else
	for (i=0;i<2;i++)
	  for (j=0;j<triab[i]->dots.size();j++)
	  {
	    tri=tri->findt(triab[i]->dots[j],true);
	    tri->dots.push_back(triab[i]->dots[j]);
	  }
      for (i=1;i<6;i++)
	allpoints.push_back(&tempPointlist[thread].points[i]);
      for (i=0;i<4;i++)
	alltris.push_back(&tempPointlist[thread].triangles[i]);
      if (adjustElev(alltris,allpoints).validMatrix)
      {
	elev13=(tempPointlist[thread].points[1].elev()*tempPointlist[thread].edges[6].length()+
		tempPointlist[thread].points[3].elev()*tempPointlist[thread].edges[4].length())/
	      (tempPointlist[thread].edges[4].length()+tempPointlist[thread].edges[6].length());
	elev24=(tempPointlist[thread].points[2].elev()*tempPointlist[thread].edges[7].length()+
		tempPointlist[thread].points[4].elev()*tempPointlist[thread].edges[5].length())/
	      (tempPointlist[thread].edges[5].length()+tempPointlist[thread].edges[7].length());
	elev5=tempPointlist[thread].points[5].elev();
      }
      else
	elev13=elev24=elev5=0;
      if (elev13==elev24)
	crit1=0;
      else
	crit1=(fabs(elev13-elev5)-fabs(elev24-elev5))/(fabs(elev13-elev5)+fabs(elev24-elev5));
      for (i=0;i<4;i++)
	ndots[i]=tempPointlist[thread].edges[i].tria->dots.size();
      crit2=(tempPointlist[thread].edges[4].length()*
	    tempPointlist[thread].edges[6].length()-
	    tempPointlist[thread].edges[5].length()*
	    tempPointlist[thread].edges[7].length())/
	    (tempPointlist[thread].edges[4].length()*
	    tempPointlist[thread].edges[6].length()+
	    tempPointlist[thread].edges[5].length()*
	    tempPointlist[thread].edges[7].length());
      ret=crit1+crit2>0;
    }
    logCrit(crit1);
    logCrit(crit2);
  }
  logCrit(isSpiky);
  logCrit(wouldbeSpiky);
  if (isSpiky && !wouldbeSpiky)
    ret=true;
  if (wouldbeSpiky && !isSpiky)
    ret=false;
  if (runFlips(e))
    ret=false;
  return ret;
}

bool shouldBend(edge *e,double tolerance,double minArea)
{
  if (e->tria)
    return shouldSplit(e->tria,tolerance,minArea);
  else
    return shouldSplit(e->trib,tolerance,minArea);
}

int edgeop(edge *e,double tolerance,double minArea,int thread)
{
  bool did=false;
  bool gotLock1,gotLock2=true;
  vector<point *> corners;
  vector<triangle *> triNeigh,triAdj;
  corners.push_back(e->a);
  corners.push_back(e->b);
  if (e->tria)
  {
    triAdj.push_back(e->tria);
    corners.push_back(e->nextb->otherend(e->b));
  }
  if (e->trib)
  {
    triAdj.push_back(e->trib);
    corners.push_back(e->nexta->otherend(e->a));
  }
  gotLock1=lockTriangles(thread,triAdj);
  if (gotLock1 && e->isinterior())
    if (e->isFlippable() && shouldFlip(e,tolerance,minArea,thread))
    {
      triNeigh=triangleNeighbors(corners);
      gotLock2=lockTriangles(thread,triNeigh);
      if (gotLock2)
      {
	flip(e,thread);
	did=true;
      }
    }
    else;
  else
    if (gotLock1 && shouldBend(e,tolerance,minArea))
    {
      triNeigh=triangleNeighbors(corners);
      gotLock2=lockTriangles(thread,triNeigh);
      if (gotLock2)
      {
	corners.push_back(bend(e,thread));
	triNeigh=triangleNeighbors(corners);
	did=true;
      }
    }
  if (triNeigh.size()==0)
  {
    triNeigh=triangleNeighbors(corners);
    gotLock2=lockTriangles(thread,triNeigh);
  }
  if (gotLock2 && (did || std::isnan(rmsAdjustment())))
    logAdjustment(adjustElev(triNeigh,corners));
  unlockTriangles(thread);
  return gotLock1*2+gotLock2; // 2 means deadlock
}
