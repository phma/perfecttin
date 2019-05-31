/******************************************************/
/*                                                    */
/* adjelev.cpp - adjust elevations of points          */
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

#include <iostream>
#include <cmath>
#include <cassert>
#include "leastsquares.h"
#include "adjelev.h"
#include "angle.h"
#include "manysum.h"
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
  for (ndots=i=0;i<tri.size();i++)
  {
    ndots+=tri[i]->dots.size();
    tri[i]->flatten(); // sets sarea, needed for areaCoord
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
      pnt[k]->raise(x[k]);
    else
      ret.validMatrix=false;
    xsq.push_back(sqr(x[k]));
  }
  ret.msAdjustment=pairwisesum(xsq)/xsq.size();
  //if (singular)
    //cout<<"Matrix in least squares is singular"<<endl;
  return ret;
}

void logAdjustment(adjustRecord rec)
{
  adjustmentLog.push_back(rec);
}

double rmsAdjustment()
// Returns the square root of the average of recent adjustments.
{
  int nRecent=lrint(sqrt(adjustmentLog.size()));
  vector<double> xsq;
  int i;
  for (i=adjustmentLog.size()-1;i>=0 && xsq.size()<nRecent;i--)
    if (std::isfinite(adjustmentLog[i].msAdjustment))
      xsq.push_back(adjustmentLog[i].msAdjustment);
  if (std::isnan(sqrt(pairwisesum(xsq)/xsq.size())))
    i++;
  return sqrt(pairwisesum(xsq)/xsq.size());
}
