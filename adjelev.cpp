/******************************************************/
/*                                                    */
/* adjelev.cpp - adjust elevations of points          */
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

#include <iostream>
#include <cmath>
#include <cassert>
#include "leastsquares.h"
#include "adjelev.h"
#include "angle.h"
#include "manysum.h"
#include "octagon.h"
#include "neighbor.h"
#include "threads.h"
using namespace std;

vector<adjustRecord> adjustmentLog;

adjustRecord adjustElev(vector<triangle *> tri,vector<point *> pnt)
/* Adjusts the points by least squares to fit all the dots in the triangles.
 * The triangles should be all those that have at least one corner in
 * the list of points. Corners of triangles which are not in pnt will not
 * be adjusted.
 */
{
  int i,j,k,ndots;
  matrix a;
  adjustRecord ret{true,0};
  vector<double> b,x,xsq;
  double oldelev;
  for (ndots=i=0;i<tri.size();i++)
  {
    ndots+=tri[i]->dots.size();
    tri[i]->flatten(); // sets sarea, needed for areaCoord
    if (net.revtriangles.count(tri[i]))
      markBucketDirty(net.revtriangles[tri[i]]);
  }
  a.resize(ndots,pnt.size());
  for (ndots=i=0;i<tri.size();i++)
    for (j=0;j<tri[i]->dots.size();j++,ndots++)
    {
      for (k=0;k<pnt.size();k++)
	a[ndots][k]=tri[i]->areaCoord(tri[i]->dots[j],pnt[k]);
      b.push_back(tri[i]->dots[j].elev()-tri[i]->elevation(tri[i]->dots[j]));
    }
  x=linearLeastSquares(a,b);
  assert(x.size()==pnt.size());
  for (k=0;k<pnt.size();k++)
  {
    if (std::isfinite(x[k]))
    {
      //if (fabs(x[k])>2000)
	//cout<<"Big adjustment!\n";
      oldelev=pnt[k]->elev();
      pnt[k]->raise(x[k]);
      if (pnt[k]->elev()>clipHigh)
	pnt[k]->raise(clipHigh-pnt[k]->elev());
      if (pnt[k]->elev()<clipLow)
	pnt[k]->raise(clipLow-pnt[k]->elev());
    }
    else
    {
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

void logAdjustment(adjustRecord rec)
{
  adjLog.lock();
  adjustmentLog.push_back(rec);
  adjLog.unlock();
}

double rmsAdjustment()
// Returns the square root of the average of recent adjustments.
{
  int nRecent;
  vector<double> xsq;
  int i,sz;
  adjLog.lock();
  sz=adjustmentLog.size();
  adjLog.unlock();
  nRecent=lrint(sqrt(sz));
  for (i=sz-1;i>=0 && xsq.size()<nRecent;i--)
    if (std::isfinite(adjustmentLog[i].msAdjustment))
      xsq.push_back(adjustmentLog[i].msAdjustment);
  if (std::isnan(sqrt(pairwisesum(xsq)/xsq.size())))
    i++;
  return sqrt(pairwisesum(xsq)/xsq.size());
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
  adjLog.unlock();
}
