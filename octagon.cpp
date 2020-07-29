/******************************************************/
/*                                                    */
/* octagon.cpp - bound the points with an octagon     */
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
#include <cstring>
#include <cassert>
#include "octagon.h"
#include "relprime.h"
#include "angle.h"
#include "cloud.h"
#include "tin.h"
#include "random.h"
#include "ldecimal.h"
#include "boundrect.h"
#include "cogo.h"
#include "threads.h"
#include "adjelev.h"

using namespace std;

pointlist net;
double clipLow,clipHigh;
array<double,2> areadone={0,0};
double mtxSquareSide;

BoundBlockTask::BoundBlockTask()
{
  dots=nullptr;
  result=nullptr;
  numDots=0;
}

void setMutexArea(double area)
{
  mtxSquareSide=sqrt(area/2)/mtxSquareSize;
}

double estimatedDensity()
{
  int i,totalDots;
  vector<double> areas;
  for (totalDots=i=0;i<net.triangles.size();i++)
  {
    totalDots+=net.triangles[i].dots.size();
    areas.push_back(net.triangles[i].area());
  }
  return totalDots/pairwisesum(areas);
}

void computeBoundBlock(BoundBlockTask &task)
{
  int i;
  for (i=0;i<task.numDots;i++)
  {
    task.result->orthogonal.include(task.dots[i]);
    task.result->diagonal.include(task.dots[i]);
  }
  if (task.result)
    task.result->ready=true;
}

double makeOctagon()
/* Creates an octagon which encloses cloud (defined in ply.cpp) and divides it
 * into six triangles. Returns the maximum error of any point in the cloud.
 */
{
  int ori=rng.uirandom();
  int totalDots[6];
  vector<DealBlockTask> tasks;
  vector<DealBlockResult> results;
  vector<BoundBlockTask> btasks;
  vector<BoundBlockResult> bresults;
  vector<int> blkSizes;
  bool allReady=false;
  double bounds[8],width,margin=0,err,maxerr=0,high=-INFINITY,low=INFINITY;
  xyz dot;
  bool valid=true;
  xy corners[8];
  vector<triangle *> trianglePointers;
  vector<point *> cornerPointers;
  int i,j,n,h,sz,triDots;
  triangle *tri;
  net.clear();
  net.triangles[0]; // Create a dummy triangle so that the GUI says "Making octagon"
  net.conversionTime=time(nullptr);
  resizeBuckets(1);
  clearTriangleLocks();
  sz=cloud.size();
  blkSizes=blockSizes(sz);
  for (triDots=i=0;i<blkSizes.size();i++)
  {
    btasks.resize(btasks.size()+1);
    bresults.resize(bresults.size()+1);
    btasks.back().dots=&cloud[triDots];
    btasks.back().numDots=blkSizes[i];
    triDots+=blkSizes[i];
  }
  for (i=n=0;i<btasks.size();i++)
  {
    btasks[i].result=&bresults[i];
    bresults[i].ready=false;
    bresults[i].orthogonal.setOrientation(ori);
    bresults[i].diagonal.setOrientation(ori+DEG45);
  }
  for (i=0;i<btasks.size();i++)
    enqueueBound(btasks[i]);
  while (!allReady)
  {
    if (!boundQueueEmpty())
    {
      BoundBlockTask task=dequeueBound();
      computeBoundBlock(task);
    }
    allReady=true;
    for (i=0;i<bresults.size();i++)
      allReady&=bresults[i].ready;
  }
  for (i=3;i<7;i++)
  {
    bounds[i]=INFINITY;
    bounds[i^4]=INFINITY;
  }
  for (i=0;i<bresults.size();i++)
  {
    if (bounds[0]>bresults[i].orthogonal.left())
      bounds[0]=bresults[i].orthogonal.left();
    if (bounds[1]>bresults[i].diagonal.bottom())
      bounds[1]=bresults[i].diagonal.bottom();
    if (bounds[2]>bresults[i].orthogonal.bottom())
      bounds[2]=bresults[i].orthogonal.bottom();
    if (bounds[3]>-bresults[i].diagonal.right())
      bounds[3]=-bresults[i].diagonal.right();
    if (bounds[4]>-bresults[i].orthogonal.right())
      bounds[4]=-bresults[i].orthogonal.right();
    if (bounds[5]>-bresults[i].diagonal.top())
      bounds[5]=-bresults[i].diagonal.top();
    if (bounds[6]>-bresults[i].orthogonal.top())
      bounds[6]=-bresults[i].orthogonal.top();
    if (bounds[7]>bresults[i].diagonal.left())
      bounds[7]=bresults[i].diagonal.left();
    if (low>bresults[i].diagonal.low())
      low=bresults[i].diagonal.low();
    if (high<bresults[i].diagonal.high())
      high=bresults[i].diagonal.high();
  }
  clipHigh=2*high-low;
  clipLow=2*low-high;
  for (i=0;i<4;i++)
  {
    width=-bounds[i]-bounds[i+4];
    margin+=width;
  }
  if (margin<=0) // Width is 0 in all directions;
    valid=false; // only one point or all points coincide.
  margin/=4*sqrt(cloud.size());
  for (i=0;i<4;i++)
  {
    bounds[i]-=margin;
    bounds[i+4]-=margin;
  }
  for (i=0;i<8;i++)
  {
    corners[i]=intersection(cossin(i*DEG45-ori)*bounds[i],(i+2)*DEG45-ori,cossin((i+1)*DEG45-ori)*bounds[(i+1)%8],(i+3)*DEG45-ori);
    net.addpoint(i+1,point(corners[i],0));
  }
  for (i=0;i<7;i++)
  {
    net.edges[i].a=&net.points[1];
    net.edges[i].b=&net.points[2+i];
    net.points[1].line=net.points[2+i].line=&net.edges[i];
  }
  for (i=0;i<6;i++)
  {
    net.edges[7+i].a=&net.points[2+i];
    net.edges[7+i].b=&net.points[3+i];
  }
  for (i=0;i<7;i++)
    net.edges[i].nexta=&net.edges[(i+1)%7];
  for (i=1;i<7;i++)
    net.edges[i].nextb=&net.edges[i+6];
  net.edges[0].nextb=&net.edges[7];
  for (i=7;i<13;i++)
    net.edges[i].nexta=&net.edges[i-7];
  for (i=7;i<12;i++)
    net.edges[i].nextb=&net.edges[i+1];
  net.edges[12].nextb=&net.edges[6];
  for (i=0;i<6;i++)
  {
    net.triangles[i].a=&net.points[1];
    net.triangles[i].b=&net.points[i+2];
    net.triangles[i].c=&net.points[i+3];
    net.edges[i+7].trib=&net.triangles[i];
    net.edges[i].trib=&net.triangles[i];
    net.edges[i+1].tria=&net.triangles[i];
    net.triangles[i].flatten();
  }
  for (i=0;i<5;i++)
  {
    net.triangles[i].setneighbor(&net.triangles[i+1]);
    net.triangles[i+1].setneighbor(&net.triangles[i]);
  }
  net.makeqindex();
  allReady=false;
  h=relprime(blkSizes.size());
  for (triDots=i=0;i<blkSizes.size();i++)
  {
    tasks.resize(tasks.size()+1);
    results.resize(results.size()+1);
    for (j=0;j<6;j++)
      tasks.back().tri[j]=&net.triangles[j];
    tasks.back().dots=&cloud[triDots];
    tasks.back().numDots=blkSizes[i];
    triDots+=blkSizes[i];
  }
  for (i=n=0;i<tasks.size();i++,n=(n+h)%tasks.size())
  { // For why the blocks are shuffled, see edgeop.cpp.
    tasks[i].result=&results[n];
    tasks[i].thread=0;
    results[n].ready=false;
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
  for (i=0;i<6;i++)
    totalDots[i]=0;
  for (i=0;i<results.size();i++)
    for (j=0;j<6;j++)
      totalDots[j]+=results[i].dots[j].size();
  for (i=0;i<6;i++)
  {
    net.triangles[i].dots.resize(totalDots[i]);
    for (triDots=j=0;j<results.size();j++)
    {
      if (results[j].dots[i].size())
	memmove((void *)&net.triangles[i].dots[triDots],(void *)&results[j].dots[i][0],results[j].dots[i].size()*sizeof(xyz));
      triDots+=results[j].dots[i].size();
    }
  }
  cloud.clear();
  cloud.shrink_to_fit();
  mtxSquareSide=0;
  for (i=0;i<6;i++)
  {
    //cout<<"triangle "<<i<<" has "<<net.triangles[i].dots.size()<<" dots\n";
    trianglePointers.push_back(&net.triangles[i]);
    net.revtriangles[&net.triangles[i]]=i;
    mtxSquareSide+=net.triangles[i].area();
  }
  setMutexArea(mtxSquareSide);
  for (i=1;i<=8;i++)
  {
    cornerPointers.push_back(&net.points[i]);
    net.convexHull.push_back(&net.points[i]);
  }
  logAdjustment(adjustElev(trianglePointers,cornerPointers));
  for (i=0;i<6;i++)
  {
    net.triangles[i].setError(INFINITY);
    err=net.triangles[i].vError;
    if (fabs(err)>maxerr)
      maxerr=fabs(err);
  }
  //for (i=1;i<=8;i++)
    //cout<<"corner "<<i<<" has elevation "<<net.points[i].elev()<<endl;
  if (!valid)
    maxerr=NAN;
  return maxerr;
}

int mtxSquare(xy pnt)
/* The plane is divided into squares, where each square corresponds to one mutex
 * of triMutex. The number of these mutexes is at least thrice the number of threads.
 * This function returns the number of the mutex corresponding to pnt.
 */
{
  int x,y;
  x=lrint(pnt.getx()/mtxSquareSide)%mtxSquareSize;
  y=lrint(pnt.gety()/mtxSquareSide)%mtxSquareSize;
  if (x<0)
    x+=mtxSquareSize;
  if (y<0)
    y+=mtxSquareSize;
  return y*mtxSquareSize+x;
}

int elevColor(double elev,bool loose)
/* Returns an integer 00rrggbb depending on the elevation.
 * Blue is the lowest dot elevation; red is the highest.
 */
{
  double x,y,rRed,rGreen,rBlue;
  int iRed,iGreen,iBlue;
  x=((elev-clipLow)*3/(clipHigh-clipLow)-1)*M_PI;
  y=sin(x);
  rRed=(x-y)/M_PI;
  rGreen=y;
  rBlue=(M_PI-x-y)/M_PI;
  if (loose)
  {
    rRed=(rRed+1)/2;
    rGreen=(rGreen+1)/2;
    rBlue=(rBlue+1)/2;
  }
  iRed=floor(rRed*256);
  iGreen=floor(rGreen*256);
  iBlue=floor(rBlue*256);
  iRed=max(0,min(255,iRed));
  iGreen=max(0,min(255,iGreen));
  iBlue=max(0,min(255,iBlue));
  return iRed*65536+iGreen*256+iBlue;
}
