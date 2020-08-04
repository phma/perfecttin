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

Color Colorize::operator()(point *pnt)
{
  Color ret;
  return ret;
}

Color Colorize::operator()(triangle *tri)
{
  Color ret;
  return ret;
}
