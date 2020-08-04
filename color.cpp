/******************************************************/
/*                                                    */
/* color.cpp - colors for points and triangles        */
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

#include "color.h"
#include "neighbor.h"
using namespace std;

Color::Color(double red,double green,double blue)
{
  r=min(1.,max(0.,red));
  g=min(1.,max(0.,green));
  b=min(1.,max(0.,blue));
}

Color::Color()
{
  r=g=b=0;
}

Colorize::Colorize()
{
  high=low=NAN;
  ori=0;
  scheme=CS_GRADIENT;
}

Color Colorize::gradientColor(xy gradient)
{
  double r,g,b;
  r=0.5+gradient.north()*0.1294+gradient.east()*0.483;
  g=0.5+gradient.north()*0.3535-gradient.east()*0.3535;
  b=0.5-gradient.north()*0.483 -gradient.east()*0.1294;
  return Color(r,g,b);
}

Color Colorize::elevationColor(double elevation)
{
  double x,y,r,g,b;
  x=(elevation-low)/(high-low)*M_PI;
  y=sin(x);
  r=(x-y)/M_PI;
  g=y;
  b=(M_PI-x-y)/M_PI;
  return Color(r,g,b);
}

void Colorize::setLimits(double l,double h)
{
  low=l;
  high=h;
}

void Colorize::setOrientation(int o)
{
  ori=o;
}

void Colorize::setScheme(int s)
{
  scheme=s;
}

Color Colorize::operator()(point *pnt)
{
  Color ret;
  vector<point *> pnt1(1,pnt);
  vector<triangle *> tris;
  xy sumGradient;
  int i;
  switch (scheme)
  {
    case CS_GRADIENT:
      tris=triangleNeighbors(pnt1);
      for (i=0;i<tris.size();i++)
	if (tris[i]->a)
	  sumGradient+=tris[i]->gradient(tris[i]->centroid());
      sumGradient/=tris.size();
      ret=gradientColor(sumGradient);
      break;
    case CS_ELEVATION:
      if (pnt)
	ret=elevationColor(pnt->elev());
      break;
  }
  return ret;
}

Color Colorize::operator()(triangle *tri)
{
  Color ret;
  return ret;
}
