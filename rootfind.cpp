/******************************************************/
/*                                                    */
/* rootfind.cpp - root-finding methods                */
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
#include <utility>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "cogo.h"
#include "ldecimal.h"
#include "rootfind.h"

using namespace std;

/* Input:
 * 9s trit is the sign of the contrapoint,
 * 3s trit is the sign of the new point,
 * 1s trit is the sign of the old point.
 * Output:
 * Bits 0 and 1:
 * 0: done
 * 1: new point becomes old point
 * 2: new point becomes contrapoint
 * 3: should never happen.
 * Bit 2: true if new point should be moved one ulp toward old point, if s==x.
 */
char sidetable[27]=
{
  3,7,6, 3,0,0, 3,3,1,
  3,3,7, 3,0,3, 7,3,3,
  1,3,3, 0,0,3, 6,7,3
};

double invquad(double x0,double y0,double x1,double y1,double x2,double y2)
{
  double z0,z1,z2,offx;
  z0=x0;
  z1=x1;
  z2=x2;
  if (z0>z1)
    swap(z0,z1);
  if (z1>z2)
    swap(z1,z2);
  if (z0>z1)
    swap(z0,z1);
  if (z0<0 && z2>0)
    offx=0;
  if (z0>=0)
    offx=z0;
  if (z2<=0)
    offx=z2;
  x0-=offx;
  x1-=offx;
  x2-=offx;
  z0=x0*y1*y2/(y0-y1)/(y0-y2);
  z1=x1*y2*y0/(y1-y2)/(y1-y0);
  z2=x2*y0*y1/(y2-y0)/(y2-y1);
  return (z0+z1+z2)+offx;
}

bool brent::between(double s)
{
  double g=(3*a+b)/4;
  return (g<s && s<b) || (b<s && s<g);
}

double brent::init(double x0,double y0,double x1,double y1,bool intmode)
{
  imode=intmode;
  debug=false;
  if (fabs(y0)>fabs(y1))
  {
    c=a=x0;
    fc=fa=y0;
    b=x1;
    fb=y1;
  }
  else
  {
    c=a=x1;
    fc=fa=y1;
    b=x0;
    fb=y0;
  }
  mflag=false;
  side=1;
  x=b-fb*(a-b)/(fa-fb);
  if (imode)
    x=rint(x);
  if (!between(x))
  {
    x=(a+b)/2;
    mflag=true;
  }
  if (imode)
    x=rint(x);
  if ((y0>0 && y1>0) || (y0<0 && y1<0))
  {
    x=NAN;
    side=3;
  }
  return x;
}

/* This routine has a bug which shows up in testcontour, in the triangle
 * centered at 0.6438,3.85625, on the subdivider of that triangle, which goes
 * from (1.064,4.729) to (1.343,2.950), when offset by (-1000000,-1500000,0).
 * When not offset, it proceeds from
 * $4 = {a = 0, fa = 0.2589999999999999, b = 1.7924233852431699, 
 *   fb = -2.9371782783726985e-12, c = 1.792423838572657, 
 *   fc = -5.5101025270287707e-08, d = 1.793766453650733, 
 *   fd = -0.00016323015394562046, x = 1.7924233852190035, side = 1, 
 *   mflag = false, imode = false, debug = false}
 * to
 * $5 = {a = 1.7924233852431699, fa = -2.9371782783726985e-12, 
 *   b = 1.7924233852190035, fb = 5.5511151231257827e-17, c = 1.7924233852431699, 
 *   fc = -2.9371782783726985e-12, d = 1.792423838572657, 
 *   fd = -5.5101025270287707e-08, x = 1.7924233852190039, side = 6, 
 *   mflag = false, imode = false, debug = false}
 * When offset, it proceeds from
 * $4 = {a = 0, fa = 0.2589999999999999, b = 1.7924233851384037, 
 *   fb = -2.9371782783726985e-12, c = 1.792423838467889, 
 *   fc = -5.5101025103754253e-08, d = 1.7937664535460136, 
 *   fd = -0.00016323015395466878, x = 1.7924233851142375, side = 1, 
 *   mflag = false, imode = false, debug = false}
 * to
 * $5 = {a = 0, fa = 0.2589999999999999, b = 1.7924233851142375, 
 *   fb = -2.7755575615628914e-17, c = 1.7924233851384037, 
 *   fc = -2.9371782783726985e-12, d = 1.792423838467889, 
 *   fd = -5.5101025103754253e-08, x = 1.7924233851142373, side = 1, 
 *   mflag = false, imode = false, debug = false}
 * and never changes a, resulting in garbage for the contourcept.
 */
double brent::step(double y)
{
  double s,bsave=b,asave=a;
  bool iq;
  side=sidetable[9*sign(fa)+3*sign(y)+sign(fb)+13];
  if ((side&3)%3)
  {
    d=c;
    c=b;
    fd=fc;
    fc=fb;
  }
  switch (side&3)
  {
    case 0:
      s=x;
      break;
    case 1:
      b=x;
      fb=y;
      break;
    case 2:
      a=x;
      fa=y;
      break;
    case 3:
      s=NAN;
  }
  if (debug)
    cout<<"side="<<side<<endl;
  if ((side&3)%3)
  {
    if (fabs(fb)>fabs(fa))
    {
      swap(fa,fb);
      swap(a,b);
    }
  }
  if (fa==fb || fb==fc || fc==fa)
  {
    s=b-fb*(b-a)/(fb-fa);
    iq=false;
  }
  else
  {
    s=invquad(a,fa,b,fb,c,fc);
    iq=true;
  }
  if (imode)
    s=rint(s);
  if ((side&3)%3 && x==s)
  {
    if (debug)
      cout<<"Same as last time"<<endl;
    if (imode)
    {
      if (bsave<x)
        side^=4;
      if (side&4)
        s++;
      else
        s--;
    }
    else
      s=nextafter(s,(side&4)?bsave:asave);
  }
  if (debug)
  {
    cout<<setw(23)<<ldecimal(a)<<setw(23)<<ldecimal(b)<<setw(23)<<ldecimal(c)<<' '<<iq<<endl;
    cout<<setw(23)<<ldecimal(fa)<<setw(23)<<ldecimal(fb)<<setw(23)<<ldecimal(fc)<<endl;
    cout<<"s="<<ldecimal(s);
  }
  if (between(s) && fabs(s-b)<fabs(mflag?b-c:c-d)/2)
    mflag=false;
  else
  {
    mflag=true;
    s=(a+b)/2;
  }
  if (imode)
    s=rint(s);
  if (debug)
    cout<<' '<<ldecimal(s)<<endl;
  if (mflag && (s==a || s==b)) // interval [a,b] is too small to bisect, we're done
  {
    s=b;
    side=0;
  }
  if ((side&3)%3)
    x=s;
  return s;
}

double Newton::init(double x0,double y0,double z0,double x1,double y1,double z1)
{
  done=false;
  nodec=0;
  a=x0;
  fa=y0;
  da=z0;
  b=x1;
  fb=y1;
  db=z1;
  if (fabs(fa/da)>fabs(fb/db))
  {
    swap(a,b);
    swap(fa,fb);
    swap(da,db);
  }
  x=a-fa/da;
  if (sign(fa)*sign(fb)<0 && sign(x-a)*sign(x-b)>=0)
    x=(a+b)/2;
  return x;
}

double Newton::step(double y,double z)
{
  if (!done)
  {
    b=a;
    fb=fa;
    db=da;
    a=x;
    fa=y;
    da=z;
    if (fa==0 || da==0)
      done=true;
    else
    {
      if (sign(fa)*sign(fb)<0)
      {
        x=(a+b)/2;
        if (x==a || x==b)
          done=true;
      }
      else
        x=a-fa/da;
    }
    if (fabs(fa)>=fabs(fb))
      nodec++;
    if (nodec>64)
      done=true;
  }
  if (std::isnan(x))
    done=true;
  return x;
}
