/******************************************************/
/*                                                    */
/* color.h - colors for points and triangles          */
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
#ifndef COLOR_H
#define COLOR_H
#include <cmath>
#include <algorithm>
#include "triangle.h"
#include "point.h"

#define CS_GRADIENT 0
#define CS_ELEVATION 1

class Color
{
public:
  Color(double red,double green,double blue);
  Color();
  double fr()
  {
    return r;
  }
  double fg()
  {
    return g;
  }
  double fb()
  {
    return b;
  }
  int br()
  {
    return std::min(255,(int)floor(255*r));
  }
  int bg()
  {
    return std::min(255,(int)floor(255*g));
  }
  int bb()
  {
    return std::min(255,(int)floor(255*b));
  }
private:
  double r,g,b;
};

class Colorize
{
public:
  Colorize();
  void setLimits(double l,double h);
  void setOrientation(int o);
  void setScheme(int s);
  Color operator()(point *pnt);
  Color operator()(triangle *tri);
private:
  double low;
  double high;
  int ori;
  int scheme;
};

#endif
