/******************************************************/
/*                                                    */
/* adjelev.cpp - adjust elevations of points          */
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

#include <iostream>
#include <cmath>
#include <cassert>
#include <cstring>
#include "leastsquares.h"
#include "adjelev.h"
#include "angle.h"
#include "manysum.h"
#include "octagon.h"
#include "neighbor.h"
#include "threads.h"
using namespace std;

vector<adjustRecord> adjustmentLog;
size_t adjLogSz=0;

void checkIntElev(vector<point *> corners)
/* Check whether any of the elevations is an integer.
 * Some elevations have been integers like 0 or 256.
 */
{
  int i;
  for (i=0;i<corners.size();i++)
    if (frac(corners[i]->elev())==0)
      cout<<"Point "<<net.revpoints[corners[i]]<<" elevation "<<corners[i]->elev()<<endl;
}

vector<int> blockSizes(int total)
/* Compute the block sizes for adjusting a triangle with a large number of dots.
 * The sequence is decreasing so that all the threads working on the
 * adjustment will finish at about the same time. The first few terms may
 * increase and not be multiples of the step size, but after that they will
 * be decreasing by the step size.
 */
{
  vector<int> ret;
  int block;
  double triroot,dec;
  while (total)
  {
    if (total>TASK_STEP_SIZE)
    {
      triroot=sqrt((double)total/TASK_STEP_SIZE*2+0.25)-0.5;
      dec=triroot+(2*triroot-1)*sin(2*M_PI*triroot)/4/M_PI;
      block=lrint(dec*TASK_STEP_SIZE);
      if (block>total)
	block=total;
    }
    else
      block=total;
    assert(block>0);
    ret.push_back(block);
    total-=block;
  }
  return ret;
}

adjustRecord adjustElev(vector<triangle *> tri,vector<point *> pnt)
/* Adjusts the points by least squares to fit all the dots in the triangles.
 * The triangles should be all those that have at least one corner in
 * the list of points. Corners of triangles which are not in pnt will not
 * be adjusted.
 *
 * This function spends approximately equal amounts of time in areaCoord,
 * elevation, and linearLeastSquares (most of which is transmult). To spread
 * the work over threads when there are few triangles, therefore, I should
 * create tasks consisting of a block of about 65536 dots all from one triangle,
 * which are to be run through areaCoord, elevation, and transmult, and combine
 * the resulting square matrices and column matrices once all the tasks are done.
 * Any idle thread can do a task; this function will do whatever tasks are not
 * picked up by other threads.
 */
{
  int i,j,k,ndots,mostDots,triDots;
  matrix a;
  double localLow=INFINITY,localHigh=-INFINITY,localClipLow,localClipHigh;
  double pointLow=INFINITY,point2Low=INFINITY,pointHigh=-INFINITY,point2High=-INFINITY;
  double pointClipLow,pointClipHigh;
  edge *ed;
  vector<AdjustBlockTask> tasks;
  vector<AdjustBlockResult> results;
  vector<int> blkSizes;
  bool allReady=false;
  adjustRecord ret{true,0};
  vector<double> b,x,xsq,nextCornerElev;
  vector<point *> nearPoints; // includes points held still
  double oldelev;
  nearPoints=pointNeighbors(tri);
  for (ndots=mostDots=i=0;i<tri.size();i++)
  {
    ndots+=tri[i]->dots.size();
    if (tri[i]->dots.size()>mostDots)
      mostDots=tri[i]->dots.size();
    tri[i]->flatten(); // sets sarea, needed for areaCoord
    if (net.revtriangles.count(tri[i]))
      markBucketDirty(net.revtriangles[tri[i]]);
  }
  if (mostDots>TASK_STEP_SIZE*3)
  {
    for (i=0;i<tri.size();i++)
    {
      blkSizes=blockSizes(tri[i]->dots.size());
      for (triDots=j=0;j<blkSizes.size();j++)
      {
	tasks.resize(tasks.size()+1);
	results.resize(results.size()+1);
	tasks.back().tri=tri[i];
	tasks.back().pnt=pnt;
	tasks.back().dots=&tri[i]->dots[triDots];
	tasks.back().numDots=blkSizes[j];
	triDots+=blkSizes[j];
      }
    }
    for (i=0;i<tasks.size();i++)
    {
      tasks[i].result=&results[i];
      results[i].ready=false;
    }
    for (i=0;i<tasks.size();i++)
      enqueueAdjust(tasks[i]);
  }
  while (!allReady)
  {
    if (!adjustQueueEmpty())
    {
      AdjustBlockTask task=dequeueAdjust();
      computeAdjustBlock(task);
    }
    allReady=true;
    for (i=0;i<results.size();i++)
      allReady&=results[i].ready;
  }
  if (results.size())
    for (i=0;i<results.size();i++)
    {
      if (results[i].high>localHigh)
	localHigh=results[i].high;
      if (results[i].low<localLow)
	localLow=results[i].low;
    }
  else
  {
    a.resize(ndots,pnt.size());
    /* Clip the adjusted elevations to the interval from the lowest dot to the
    * highest dot, and from the second lowest point to the second highest point,
    * including neighboring points not being adjusted, expanded by 3. The reason
    * it's the second highest/lowest point is that, if there's already a spike,
    * we want to clip it.
    */
    for (ndots=i=0;i<tri.size();i++)
      for (j=0;j<tri[i]->dots.size();j++,ndots++)
      {
	for (k=0;k<pnt.size();k++)
	  a[ndots][k]=tri[i]->areaCoord(tri[i]->dots[j],pnt[k]);
	b.push_back(tri[i]->dots[j].elev()-tri[i]->elevation(tri[i]->dots[j]));
	if (tri[i]->dots[j].elev()>localHigh)
	  localHigh=tri[i]->dots[j].elev();
	if (tri[i]->dots[j].elev()<localLow)
	  localLow=tri[i]->dots[j].elev();
      }
  }
  for (i=0;i<nearPoints.size();i++)
  {
    if (nearPoints[i]->elev()>pointHigh)
    {
      point2High=pointHigh;
      pointHigh=nearPoints[i]->elev();
    }
    else if (nearPoints[i]->elev()>point2High)
      point2High=nearPoints[i]->elev();
    if (nearPoints[i]->elev()<pointLow)
    {
      point2Low=pointLow;
      pointLow=nearPoints[i]->elev();
    }
    else if (nearPoints[i]->elev()<point2Low)
      point2Low=nearPoints[i]->elev();
  }
  pointClipHigh=1.125*point2High-0.125*point2Low;
  pointClipLow=1.125*point2Low-0.125*point2High;
  if (results.size())
  {
    for (i=1;i<results.size();i*=2) // In-place pairwise sum
      for (j=0;j+i<results.size();j+=2*i)
      {
	results[j].mtmPart+=results[j+i].mtmPart;
	results[j].mtvPart+=results[j+i].mtvPart;
      }
    results[0].mtmPart.gausselim(results[0].mtvPart);
    for (i=0;i<results[0].mtmPart.getcolumns();i++)
      if (results[0].mtmPart[i][i]==0)
	results[0].mtvPart[i][0]=NAN;
    x=results[0].mtvPart;
  }
  else
    if (ndots<pnt.size())
      x=minimumNorm(a,b);
    else
      x=linearLeastSquares(a,b);
  assert(x.size()==pnt.size());
  localClipHigh=2*localHigh-localLow;
  localClipLow=2*localLow-localHigh;
  if (localClipHigh>clipHigh)
    localClipHigh=clipHigh;
  if (localClipLow<clipLow)
    localClipLow=clipLow;
  if (localClipHigh>pointClipHigh && nearPoints.size()>pnt.size())
    localClipHigh=pointClipHigh;
  if (localClipLow<pointClipLow && nearPoints.size()>pnt.size())
    localClipLow=pointClipLow;
  for (k=0;k<pnt.size();k++)
  {
    if (std::isfinite(x[k]) && fabs(x[k])<16384 && ndots)
    /* Ground elevation is in the range [-11000,8850]. Roundoff error in a
     * singular matrix produces numbers around 1e18. Any number too big
     * means that the matrix is singular, even if the number is finite.
     */
    {
      //if (fabs(x[k])>2000)
	//cout<<"Big adjustment!\n";
      oldelev=pnt[k]->elev();
      pnt[k]->raise(x[k]);
      if (pnt[k]->elev()>localClipHigh)
	pnt[k]->raise(localClipHigh-pnt[k]->elev());
      if (pnt[k]->elev()<localClipLow)
	pnt[k]->raise(localClipLow-pnt[k]->elev());
    }
    else
    {
      oldelev=pnt[k]->elev();
      nextCornerElev.clear();
      for (ed=pnt[k]->line,j=0;ed!=pnt[k]->line || j==0;ed=ed->next(pnt[k]),j++)
      nextCornerElev.push_back(ed->otherend(pnt[k])->elev());
      pnt[k]->raise(pairwisesum(nextCornerElev)/nextCornerElev.size()-pnt[k]->elev());
      ret.validMatrix=false;
    }
    xsq.push_back(sqr(pnt[k]->elev()-oldelev));
  }
  ret.msAdjustment=pairwisesum(xsq)/xsq.size();
  for (i=0;i<tri.size();i++)
    if (net.revtriangles.count(tri[i]))
      markBucketDirty(net.revtriangles[tri[i]]);
  //if (singular)
    //cout<<"Matrix in least squares is singular"<<endl;
  return ret;
}

AdjustBlockTask::AdjustBlockTask()
{
  tri=nullptr;
  dots=nullptr;
  numDots=0;
  result=nullptr;
}

void computeAdjustBlock(AdjustBlockTask &task)
{
  int i,j,k,ndots=0;
  matrix m(task.numDots,task.pnt.size());
  vector<double> v;
  vector<int> cx;
  vector<point *> cp;
  if (!task.result)
    return;
  AdjustBlockResult &result=*task.result;
  result.high=-INFINITY;
  result.low=INFINITY;
  for (j=0;j<task.pnt.size();j++)
    if (task.pnt[j]==task.tri->a || task.pnt[j]==task.tri->b || task.pnt[j]==task.tri->c)
    {
      cx.push_back(j);
      cp.push_back(task.pnt[j]);
    }
  for (j=0;j<task.numDots;j++,ndots++)
  {
    for (k=0;k<cx.size();k++)
      m[ndots][cx[k]]=task.tri->areaCoord(task.tri->dots[j],cp[k]);
    v.push_back(task.tri->dots[j].elev()-task.tri->elevation(task.tri->dots[j]));
    if (task.tri->dots[j].elev()>result.high)
      result.high=task.tri->dots[j].elev();
    if (task.tri->dots[j].elev()<result.low)
      result.low=task.tri->dots[j].elev();
  }
  matrix mt,vmat=columnvector(v);
  mt=m.transpose();
  result.mtmPart=mt.transmult();
  result.mtvPart=mt*vmat;
  result.ready=true;
}

void logAdjustment(adjustRecord rec)
{
  adjLog.lock();
  adjustmentLog.push_back(rec);
  adjLogSz++;
  if (adjustmentLog.size()%2==0 && adjustmentLog.size()>2.5*sqrt(adjLogSz)+1024)
  {
    int n=adjustmentLog.size()/2;
    memmove(&adjustmentLog[0],&adjustmentLog[n],n*sizeof(adjustRecord));
    adjustmentLog.resize(n);
  }
  adjLog.unlock();
}

double rmsAdjustment()
// Returns the square root of the average of recent adjustments.
{
  int nRecent;
  vector<double> xsq;
  int i,sz;
  static size_t lastsz;
  double ret;
  static double lastret;
  adjLog.lock_shared();
   sz=adjustmentLog.size();
  if (adjLogSz==lastsz)
    ret=lastret;
  else
  {
    nRecent=lrint(sqrt(adjLogSz));
    for (i=sz-1;i>=0 && xsq.size()<nRecent;i--)
      if (std::isfinite(adjustmentLog[i].msAdjustment))
	xsq.push_back(adjustmentLog[i].msAdjustment);
    lastret=ret=sqrt(pairwisesum(xsq)/xsq.size());
    lastsz=adjLogSz;
  }
  adjLog.unlock_shared();
  return ret;
}

void adjustLooseCorners(double tolerance)
/* A loose corner is a point which is not a corner of any triangle with dots in it.
 * Loose corners are not adjusted by adjustElev; their elevation can be 0
 * or wild values left over from when they were the far corners of triangles
 * with only a few dots.
 */
{
  vector<point *> looseCorners,oneCorner;
  vector<triangle *> nextTriangles;
  edge *ed;
  vector<double> nextCornerElev;
  int i,j,ndots;
  oneCorner.resize(1);
  for (i=1;i<=net.points.size();i++)
  {
    oneCorner[0]=&net.points[i];
    nextTriangles=triangleNeighbors(oneCorner);
    for (ndots=j=0;j<nextTriangles.size();j++)
      ndots+=nextTriangles[j]->dots.size();
    if (ndots==0)
      looseCorners.push_back(oneCorner[0]);
  }
  for (i=0;i<looseCorners.size();i++)
  {
    nextCornerElev.clear();
    for (ed=looseCorners[i]->line,j=0;ed!=looseCorners[i]->line || j==0;ed=ed->next(looseCorners[i]),j++)
      nextCornerElev.push_back(ed->otherend(looseCorners[i])->elev());
    looseCorners[i]->raise(pairwisesum(nextCornerElev)/nextCornerElev.size()-looseCorners[i]->elev());
  }
}

void clearLog()
{
  adjLog.lock();
  adjustmentLog.clear();
  adjLogSz=0;
  adjLog.unlock();
}
