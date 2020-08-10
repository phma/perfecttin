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

double hScale(pointlist &ptl,Printer3dSize &pri,int ori)
{
  double xscale,yscale;
  BoundRect br(ori);
  br.include(&ptl);
  xscale=pri.x/(br.right()-br.left());
  yscale=pri.y/(br.top()-br.bottom());
  if (xscale<yscale)
    return xscale;
  else
    return -yscale;
}

int turnFitInPrinter(pointlist &ptl,Printer3dSize &pri)
{
  set<int> turnings;
  vector<int> turningv;
  int i,j,bear;
  for (i=0;i<ptl.convexHull.size();i++)
    for (j=0;j<i;j++)
    {
      bear=dir((xy)*ptl.convexHull[i],(xy)*ptl.convexHull[j]);
      turnings.insert(bear&(DEG180-1));
      turnings.insert((bear+DEG90)&(DEG180-1));
    }
  for (int a:turnings)
  {
    cout<<a<<endl;
    turningv.push_back(a);
  }
  return 0;
}
