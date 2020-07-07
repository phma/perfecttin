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

double makeOctagon()
/* Creates an octagon which encloses cloud (defined in ply.cpp) and divides it
 * into six triangles. Returns the maximum error of any point in the cloud.
 */
{
  int ori=rng.uirandom();
  int totalDots[6];
  vector<DealBlockTask> tasks;
  vector<DealBlockResult> results;
  vector<int> blkSizes;
  bool allReady=false;
  BoundRect orthogonal(ori),diagonal(ori+DEG45);
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
  for (i=0;i<cloud.size();i++)
  {
    orthogonal.include(cloud[i]);
    diagonal.include(cloud[i]);
    if (cloud[i].elev()>high)
      high=cloud[i].elev();
    if (cloud[i].elev()<low)
      low=cloud[i].elev();
  }
  bounds[0]=orthogonal.left();
  bounds[1]=diagonal.bottom();
  bounds[2]=orthogonal.bottom();
  bounds[3]=-diagonal.right();
  bounds[4]=-orthogonal.right();
  bounds[5]=-diagonal.top();
  bounds[6]=-orthogonal.top();
  bounds[7]=diagonal.left();
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
  }
  for (i=0;i<5;i++)
  {
    net.triangles[i].setneighbor(&net.triangles[i+1]);
    net.triangles[i+1].setneighbor(&net.triangles[i]);
  }
  //cout<<"Orientation "<<ldecimal(bintodeg(ori),0.01)<<endl;
  //cout<<"Orthogonal ("<<orthogonal.left()<<','<<orthogonal.bottom()<<")-("<<orthogonal.right()<<','<<orthogonal.top()<<")\n";
  //cout<<"Diagonal ("<<diagonal.left()<<','<<diagonal.bottom()<<")-("<<diagonal.right()<<','<<diagonal.top()<<")\n";
  tri=&net.triangles[0];
  net.makeqindex();
  sz=cloud.size();
  blkSizes=blockSizes(sz);
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
  {
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
    for (n=0;n<net.triangles[i].dots.size();n++)
    {
      dot=net.triangles[i].dots[n];
      err=dot.elev()-net.triangles[i].elevation(dot);
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
