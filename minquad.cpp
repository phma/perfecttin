/******************************************************/
/*                                                    */
/* minquad.cpp - minimum of quadratic                 */
/*                                                    */
/******************************************************/
/* Copyright 2021 Pierre Abbat.
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
#include "minquad.h"

double minquad(double x0,double y0,double x1,double y1,double x2,double y2)
/* Finds the minimum (or maximum) of a quadratic, given three points on it.
 * x1 should be between x0 and x2.
 * Used to find the closest point on a segment or arc to a given point.
 */
{
  double xm,s1,s2;
  xm=(x0+2*x1+x2)/4;
  s1=(y1-y0)/(x1-x0);
  s2=(y2-y1)/(x2-x1);
  //cout<<"s1="<<s1<<" s2="<<s2<<" xm="<<xm<<endl;
  return xm+(x2-x0)*(s1+s2)/(s1-s2)/4;
}
