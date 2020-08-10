/******************************************************/
/*                                                    */
/* stl.cpp - stereolithography (3D printing) export   */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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

#include "stl.h"
#include "angle.h"
#include "octagon.h"
#include "boundrect.h"
using namespace std;

/* The STL polyhedron consists of three kinds of face: bottom, side, and top.
 * The bottom is a convex polygon which is triangulated. The sides are
 * trapezoids, each of which is drawn as two triangles. The top is the
 * TIN surface.
 */

StlTriangle::StlTriangle()
{
  normal=a=b=c=xyz(0,0,0);
}

StlTriangle::StlTriangle(xyz A,xyz B,xyz C)
{
  a=A;
  b=B;
  c=C;
  normal=cross(a-b,b-c);
  normal.normalize();
}

double hScale(BoundRect &br,Printer3dSize &pri)
{
  double xscale,yscale;
  xscale=pri.x/(br.right()-br.left());
  yscale=pri.y/(br.top()-br.bottom());
  if (xscale<yscale)
    return xscale;
  else
    return -yscale;
}

double hScale(pointlist &ptl,Printer3dSize &pri,int ori)
{
  int i;
  BoundRect br(ori);
  for (i=0;i<ptl.convexHull.size();i++)
    br.include(*ptl.convexHull[i]);
  return hScale(br,pri);
}

int turnFitInPrinter(pointlist &ptl,Printer3dSize &pri)
/* Finds the orientation where the pointlist best fits in the printer.
 * The worst fit orientation is where an internal diagonal is perpendicular
 * to the printer; the best fit orientation is either where a side of the
 * convex hull is parallel to the printer, or where the scale on both sides
 * of the printer is equal (shown by changing the sign of hScale).
 */
{
  set<int> turnings;
  map<int,double> scales;
  vector<int> inserenda;
  double maxScale=0;
  int i,j,bear;
  map<int,double>::iterator k;
  for (i=0;i<ptl.convexHull.size();i++)
    for (j=0;j<i;j++)
    {
      bear=dir((xy)*ptl.convexHull[i],(xy)*ptl.convexHull[j]);
      turnings.insert(bear&(DEG180-1));
      turnings.insert((bear+DEG90)&(DEG180-1));
    }
  for (int a:turnings)
  {
    scales[a]=hScale(ptl,pri,a);
  }
  do
  {
    inserenda.clear();
    for (k=scales.begin();k!=scales.end();++k)
      bear=k->first-DEG180;
    for (k=scales.begin();k!=scales.end();++k)
    {
      if (scales[bear&(DEG180-1)]*k->second<0 && k->first-bear>1)
	inserenda.push_back((k->first+bear)/2);
      bear=k->first;
    }
    for (i=0;i<inserenda.size();i++)
      scales[inserenda[i]]=hScale(ptl,pri,inserenda[i]);
  } while (inserenda.size());
  for (k=scales.begin();k!=scales.end();++k)
    if (fabs(k->second)>maxScale)
    {
      bear=k->first;
      maxScale=k->second;
    }
  return bear;
}

vector<StlTriangle> StlMesh(Printer3dSize &pri)
{
  int i,ori;
  double scale;
  xyz worldCenter,printerCenter;
  BoundRect br;
  vector<StlTriangle> ret;
  ori=turnFitInPrinter(net,pri);
  br.setOrientation(ori);
  br.include(&net);
  worldCenter=xyz((br.left()+br.right())/2,(br.bottom()+br.top())/2,br.low());
  printerCenter=xyz(pri.x/2,pri.y/2,pri.minBase);
  return ret;
}
