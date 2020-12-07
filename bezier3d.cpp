/******************************************************/
/*                                                    */
/* bezier3d.cpp - 3d Bézier splines                   */
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
/* bezier3d.cpp
 * 3d Bézier splines, used for approximations to spirals and arcs for display.
 * Of course the 3d approximation of a vertical curve is exact.
 */
#include <cmath>
#include "bezier3d.h"
#include "angle.h"
using namespace std;

bezier3d::bezier3d(xyz kra,xyz con1,xyz con2,xyz fam)
{
  controlpoints.push_back(kra);
  controlpoints.push_back(con1);
  controlpoints.push_back(con2);
  controlpoints.push_back(fam);
}

bezier3d::bezier3d(xyz kra,int bear0,double slp0,double slp1,int bear1,xyz fam)
{
  double len0,len1,len,corr0,corr1,corr2;
  int direc;
  len=dist(xy(kra),xy(fam));
  direc=dir(xy(kra),xy(fam));
  corr0=1-cos(bear0-direc);
  corr1=1-cos(bear1-direc);
  corr2=1-cos(bear0+bear1-2*direc); //this is 0 for circular curves
  len0=len/3.*(1+corr0/2-corr2/24);
  len1=len/3.*(1+corr1/2-corr2/24);
  controlpoints.push_back(kra);
  controlpoints.push_back(kra+xyz(cossin(bear0),slp0)*len0);
  controlpoints.push_back(fam-xyz(cossin(bear1),slp1)*len1);
  controlpoints.push_back(fam);
}

bezier3d::bezier3d()
{
  controlpoints.push_back(xyz(0,0,0));
}

int bezier3d::size() const
{
  return controlpoints.size()/3;
}

void bezier3d::close()
{
  if (controlpoints.size()>3 && controlpoints.size()%3 && controlpoints.back()==controlpoints[0])
    controlpoints.resize(controlpoints.size()-1);
}

bool bezier3d::isopen()
{
  return controlpoints.size()%3>0;
}

vector<xyz> bezier3d::operator[](int n)
{
  vector<xyz> ret;
  int i;
  for (i=0;i<4;i++)
    ret.push_back(controlpoints[(3*n+i)%controlpoints.size()]);
  return ret;
}

bezier3d operator+(const bezier3d &l,const bezier3d &r)
/* The arguments should both be open, and the last point of l should be the first point of r.
 * Adding the empty bezier3d to something has no effect.
 */
{
  int i;
  bezier3d ret(l);
  if (ret.controlpoints.size()==1)
    ret.controlpoints[0]=r.controlpoints[0];
  if (ret.controlpoints.size()%3==1 && r.size())
    ret.controlpoints.back()=(ret.controlpoints.back()+r.controlpoints[0])/2;
  for (i=ret.controlpoints.size()%3;i<r.controlpoints.size();i++)
    ret.controlpoints.push_back(r.controlpoints[i]);
  return ret;
}

bezier3d& bezier3d::operator+=(const bezier3d &r)
{
  int i;
  if (controlpoints.size()==1)
    controlpoints[0]=r.controlpoints[0];
  if (controlpoints.size()%3==1 && r.size())
    controlpoints.back()=(controlpoints.back()+r.controlpoints[0])/2;
  for (i=controlpoints.size()%3;i<r.controlpoints.size();i++)
    controlpoints.push_back(r.controlpoints[i]);
  return *this;
}

double bez3destimate(xy kra,int bear0,double len,int bear1,xy fam)
/* This should be used only when bear0-direc, bear1-direc, and bear0+bear1-2*direc
 * are all less than 30°. If any of them is greater, split the curve.
 * len is the horizontal distance along the curve, not the displacement
 * as in the constructor. This module has no way to know the length.
 */
{
  double corr0,corr1,corr2;
  int direc;
  direc=dir(xy(kra),xy(fam));
  corr0=1-cos(bear0-direc);
  corr1=1-cos(bear1-direc);
  corr2=1-cos(bear0+bear1-2*direc); //this is 0 for circular curves
  return ((sqr(corr0)+sqr(corr1))/20+pow(corr2,1.5)/30+sqrt(corr2)/3000)*len;
}

void bezier3d::rotate(Quaternion q)
{
  int i;
  for (i=0;i<controlpoints.size();i++)
    controlpoints[i]=q.rotate(controlpoints[i]);
}
