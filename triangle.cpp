/********************************************************/
/*                                                      */
/* triangle.cpp - triangles                             */
/*                                                      */
/********************************************************/
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
#include <cstring>
#include <climits>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cfloat>
#include "triangle.h"
#include "angle.h"
#include "ldecimal.h"
#include "tin.h"
using namespace std;

const char ctrlpttab[16]=
{
  3, 3, 3, 3,
  3, 3, 2, 4,
  3, 0, 3, 6,
  3, 1, 5, 3
};

#ifndef NDEBUG

testfunc::testfunc(double cub,double quad,double lin,double con)
{
  coeff[3]=cub;
  coeff[2]=quad;
  coeff[1]=lin;
  coeff[0]=con;
}

double testfunc::operator()(double x)
{
  return (((coeff[3])*x+coeff[2])*x+coeff[1])*x+coeff[0];
}

#endif

triangle::triangle()
{
  a=b=c=NULL;
  aneigh=bneigh=cneigh=NULL;
  flags=0;
}

double triangle::area()
{
  return area3(*a,*b,*c);
}

double triangle::perimeter()
{
  return dist(xy(*a),xy(*b))+dist(xy(*c),xy(*a))+dist(xy(*b),xy(*c));
}

double triangle::acicularity()
/* The acicularity of an equilateral triangle is 1. An acicular triangle is
 * one with at least one very small acute angle. Acicular triangles can mess up
 * tracing of contours.
 */
{
  return sqr(peri)/sarea/M_SQRT_432;
}

double triangle::areaCoord(xy pnt,point *v)
/* Returns the area coordinate (aka barycentric coordinate) of pnt toward v.
 * If v is not a corner of the triangle, returns zero.
 * The coordinate ranges from 0 at the other two corners to 1 at v,
 * if the point is inside the triangle.
 */
{
  double ret=0;
  if (v==a)
    ret=area3(pnt,*b,*c)/sarea;
  if (v==b)
    ret=area3(*a,pnt,*c)/sarea;
  if (v==c)
    ret=area3(*a,*b,pnt)/sarea;
  return ret;
}

double triangle::elevation(xy pnt)
/* Computes the elevation of triangle tri at the point x,y.
 */
{
  double p,q,r; // Fraction of distance from a side to opposite corner. p+q+r=1.
  double totarea;
  p=area3(pnt,*b,*c);
  q=area3(*a,pnt,*c);
  r=area3(*a,*b,pnt);
  totarea=p+q+r; // should equal sarea, but because of roundoff it may be many ulps off
  p/=totarea;
  q/=totarea;
  r/=totarea;
  return q*b->z+p*a->z+r*c->z;
}

bool triangle::inTolerance(double tolerance)
/* Returns true if the triangle
 * •is smaller than an equilateral triangle whose side is tolerance,
 * •has too few dots to be split, or
 * •has all dots within tolerance (without actually checking them all).
 */
{
  return dots.size()<9 ||
         sarea<sqr(tolerance)*M_SQRT_3/4 ||
         ((fabs(aElev-a->elev())+fabs(bElev-b->elev()))+
	  (fabs(cElev-c->elev())+vError))<tolerance;
}

void triangle::setError(double tolerance)
{
  double tempVError=0,err1;
  int i;
  for (i=0;i<dots.size() && tempVError<tolerance;i++)
  {
    err1=fabs(dots[i].elev()-elevation(dots[i]));
    if (err1>tempVError)
      tempVError=err1;
  }
  vError=tempVError;
  aElev=a->elev();
  bElev=b->elev();
  cElev=c->elev();
}

void triangle::unsetError()
{
  vError=INFINITY;
}

xyz triangle::gradient3(xy pnt)
{
  double p,q,r,s,gp,gq,gr;
  s=area();
  p=area3(pnt,*b,*c)/s;
  q=area3(*a,pnt,*c)/s;
  r=area3(*a,*b,pnt)/s;
  gp=a->z;
  gq=b->z;
  gr=c->z;
  return xyz(gp,gq,gr);
}

xy triangle::gradient(xy pnt)
{
  xyz g3;
  double g2[2];
  g3=gradient3(pnt);
  g2[0]=g3.x*gradmat[0][0]+g3.y*gradmat[0][1]+g3.z*gradmat[0][2];
  g2[1]=g3.x*gradmat[1][0]+g3.y*gradmat[1][1]+g3.z*gradmat[1][2];
  return xy(g2[0],g2[1]);
}

triangleHit triangle::hitTest(xy pnt)
{
  double p,q,r; // Fraction of distance from a side to opposite corner. p+q+r=1.
  double totarea;
  int flags;
  triangleHit ret;
  ret.tri=nullptr;
  ret.edg=nullptr;
  ret.cor=nullptr;
  p=area3(pnt,*b,*c);
  q=area3(*a,pnt,*c);
  r=area3(*a,*b,pnt);
  totarea=p+q+r; // should equal sarea, but because of roundoff it may be many ulps off
  p/=totarea;
  q/=totarea;
  r/=totarea;
  flags=(p<0.1)+2*(q<0.1)+4*(r<0.1)+8*(p>0.9)+16*(q>0.9)+32*(r>0.9);
  if (flags==3)
    flags=(p<q)?1:2;
  if (flags==6)
    flags=(q<r)?2:4;
  if (flags==5)
    flags=(r<p)?4:1;
  switch (flags)
  {
    case 0:
      ret.tri=this;
      break;
    case 1:
      ret.edg=c->edg(this); // A
      break;
    case 2:
      ret.edg=a->edg(this); // B
      break;
    case 4:
      ret.edg=b->edg(this); // C
      break;
    case 14:
      ret.cor=a;
      break;
    case 21:
      ret.cor=b;
      break;
    case 35:
      ret.cor=c;
      break;
  }
  return ret;
}

void triangle::setgradmat()
{
  xy scalt;
  scalt=turn90(xy(*c-*b))/sarea/2;
  gradmat[0][0]=scalt.x;
  gradmat[1][0]=scalt.y;
  scalt=turn90(xy(*a-*c))/sarea/2;
  gradmat[0][1]=scalt.x;
  gradmat[1][1]=scalt.y;
  scalt=turn90(xy(*b-*a))/sarea/2;
  gradmat[0][2]=scalt.x;
  gradmat[1][2]=scalt.y;
}

bool triangle::in(xy pnt)
{
  return area3(pnt,*b,*c)>=0 && area3(*a,pnt,*c)>=0 && area3(*a,*b,pnt)>=0;
}

xy triangle::centroid()
{
  return (xy(*a)+xy(*b)+xy(*c))/3; //FIXME: check if this affects numerical stability
}

bool triangle::inCircle(xy pnt,double radius)
/* Quick and dirty test used to fill in missing triangles in localTriangles.
 * Likely misses a triangle if pnt is out one corner and the circle passes
 * between that corner and the centroid. May return false positives.
 */
{
  int ba=dir(*a,pnt);
  int bb=dir(*b,pnt);
  int bc=dir(*c,pnt);
  int suba=abs(foldangle(bb-bc));
  int subb=abs(foldangle(bc-ba));
  int subc=abs(foldangle(ba-bb));
  int maxsub;
  if (foldangle(suba+subb+subc))
  {
    maxsub=suba;
    if (subb>maxsub)
      maxsub=subb;
    if (subc>maxsub)
      maxsub=subc;
    return coshalf(maxsub)*dist(pnt,centroid())<radius;
  }
  else
    return true; // total subtended angle == 360°, pnt is inside triangle
}

bool triangle::iscorner(point *v)
{
  return (a==v)||(b==v)||(c==v);
}

void triangle::flatten()
{
  sarea=area();
  setgradmat();
}

void triangle::setneighbor(triangle *neigh)
{
  bool sha,shb,shc;
  sha=neigh->iscorner(a);
  shb=neigh->iscorner(b);
  shc=neigh->iscorner(c);
  if (sha&&shb)
    cneigh=neigh;
  if (shb&&shc)
    aneigh=neigh;
  if (shc&&sha)
    bneigh=neigh;
}

void triangle::setnoneighbor(edge *neigh)
{
  bool sha,shb,shc;
  sha=a==neigh->a || a==neigh->b;
  shb=b==neigh->a || b==neigh->b;
  shc=c==neigh->a || c==neigh->b;
  if (sha&&shb)
    cneigh=nullptr;
  if (shb&&shc)
    aneigh=nullptr;
  if (shc&&sha)
    bneigh=nullptr;
}

triangle *triangle::nexttoward(xy pnt)
// If the point is in the triangle, return the same triangle.
// Else return which triangle to look in next.
// If returns NULL, the point is outside the convex hull.
{
  double p,q,r;
  p=area3(pnt,*b,*c);
  q=area3(*a,pnt,*c);
  r=area3(*a,*b,pnt);
  if (p>=0 && q>=0 && r>=0)
    return this;
  else if (p<q && p<r)
    return aneigh;
  else if (q<r)
    return bneigh;
  else if (!pnt.isnan())
    return cneigh;
  else
   return nullptr;
}

triangle *triangle::findt(xy pnt,bool clip)
{
  triangle *here,*there,*loopdet=nullptr;
  int i=0;
  here=there=this;
  while (here && !here->in(pnt) && here!=loopdet)
  {
    if ((i&(i-1))==0)
      loopdet=here;
    here=here->nexttoward(pnt);
    if (here)
      there=here;
    ++i;
  }
  return clip?there:here;
}

vector<double> triangle::xsect(int angle,double offset)
/* Where s is the semiperimeter, samples the surface at four points,
 * -3s/2, -s/2, s/2, 3s/2, relative to the centroid, offset by offset.
 * offset is multplied by the semiperimeter and ranges from -1.5 to 1.5.
 */
{
  double s;
  int i;
  vector<double> ret;
  xy along,across,cen;
  s=peri/2;
  cen=centroid();
  along=cossin(angle)*s;
  across=cossin(angle+DEG90)*s;
  for (i=-3;i<5;i+=2)
    ret.push_back(elevation(cen+across*offset+along*i*0.5));
  return ret;
}

double triangle::spelevation(int angle,double x,double y)
/* Where s is the semiperimeter, samples the surface at a point
 * x along the line in the direction angle offset by y.
 * x and y are multplied by the semiperimeter.
 */
{
  double s;
  xy along,across,cen;
  s=peri/2;
  cen=centroid();
  along=cossin(angle)*s;
  across=cossin(angle+DEG90)*s;
  return elevation(cen+across*y+along*x);
}

edge *triangle::checkBentContour()
/* Check for the following conditions:
 * • The triangle does not have any critical points.
 * • The edges are all monotonic.
 * • The gradient on two sides (at a place TBD) points in and on the third
 *   points out, or vice versa.
 * • The gradients on the two sides have an angle greater than 90° between them.
 * These conditions may result in contours being drawn backward or tangled.
 * The fix is to split the third side in half, which will add a subdivision
 * line cutting the bent contours, hopefully preventing them from being misdrawn.
 */
{
  return nullptr;
}

double deriv0(vector<double> xsect)
{
  return (-xsect[3]+9*xsect[2]+9*xsect[1]-xsect[0])/16;
}

double deriv1(vector<double> xsect)
{
  return (xsect[0]-27*xsect[1]+27*xsect[2]-xsect[3])/24;
}

double deriv2(vector<double> xsect)
{
  return (xsect[3]-xsect[2]-xsect[1]+xsect[0])/2;
}

double deriv3(vector<double> xsect)
{
  return xsect[3]-3*xsect[2]+3*xsect[1]-xsect[0];
}

double paravertex(vector<double> xsect)
// Finds the vertex of the parabola, assuming that it is a parabola (no cubic component).
{
  double d1,d2;
  d1=deriv1(xsect);
  d2=deriv2(xsect);
  return -d1/d2;
}

#ifndef NDEBUG

double parabinter(testfunc func,double start,double startz,double end,double endz)
{
  int lw=0,lastlw=0,i;
  double flw,p;
  vector<double> y(4),z(4);
  y[3]=1;
  y[1]=start;
  y[2]=end;
  z[1]=startz;
  z[2]=endz;
  while (fabs(y[3]-y[0])>1e-6 && abs(lw*lastlw)<4 && lw<100 && (fabs(y[3]-y[0])>1e-3 || fabs(deriv2(z))>1e-9))
  {
    switch (lw)
    {
      case -2:
	y[3]=y[1];
	y[2]=y[0];
	z[3]=z[1];
	z[2]=z[0];
	break;
      case -1:
	y[3]=y[1];
	z[3]=z[1];
	break;
      case 0:
	y[3]=y[2];
	y[0]=y[1];
	z[3]=z[2];
	z[0]=z[1];
	break;
      case 1:
	y[0]=y[2];
	z[0]=z[2];
	break;
      case 2:
	y[0]=y[2];
	y[1]=y[3];
	z[0]=z[2];
	z[1]=z[3];
	break;
    }
    switch (lw)
    {
      case -2:
	y[1]=2*y[2]-y[3];
	y[0]=2*y[1]-y[2];
	z[1]=func(y[1]);
	z[0]=func(y[0]);
	break;
      case -1:
      case 0:
      case 1:
	y[1]=(2*y[0]+y[3])/3;
	y[2]=(2*y[3]+y[0])/3;
	z[1]=func(y[1]);
	z[2]=func(y[2]);
	break;
      case 2:
	y[2]=2*y[1]-y[0];
	y[3]=2*y[2]-y[1];
	z[3]=func(y[3]);
	z[2]=func(y[2]);
	break;
    }
    lastlw=lw; // detect 2 followed by -2, which is an infinite loop that can happen with flat triangles
    flw=rint(p=paravertex(z));
    if (isfinite(flw))
    {
      lw=flw;
      if (lw>2)
	lw=2;
      if (lw<-2)
	lw=-2;
    }
    else
      lw=256;
    for (i=0;i<4;i++)
      cout<<setw(9)<<setprecision(5)<<y[i];
    cout<<endl;
    for (i=0;i<4;i++)
      cout<<setw(9)<<setprecision(5)<<z[i];
    cout<<endl<<lw<<endl;
  }
  return (y[0]*(1.5-p)+y[3]*(1.5+p))/3;
}

#endif

int triangle::pointtype(xy pnt)
/* Given a point, returns its type by counting the zero-crossings around it:
 * 0: maximum, minimum, or flat point
 * 2: slope point (it cannot tell a slope point from a chair point, which is rare)
 * 4: saddle point
 * 6: monkey saddle point
 * >6: grass point, actually a flat point of a nonzero table surface because of roundoff error.
 */
{
  int i,zerocrossings,positives,negatives;
  double velev,around[255],last;
  velev=elevation(pnt);
  for (i=0;i<255;i++)
    around[(i*2)%255]=elevation(pnt+cossin(i*0x01010101)*0.000183)-velev;
  for (i=254;i>=0 && around[i]==0;i++);
  if (i>=0)
    last=around[i];
  else
    last=0;
  for (i=zerocrossings=positives=negatives=0;i<255;i++)
  {
    if (around[i]>0)
    {
      positives++;
      if (last<0)
      {
	zerocrossings++;
	last=around[i];
      }
    }
    if (around[i]<0)
    {
      negatives++;
      if (last>0)
      {
	zerocrossings++;
	last=around[i];
      }
    }
  }
  switch (zerocrossings)
  {
    case 0:
      if (positives)
	i=PT_MIN;
      else if (negatives)
	i=PT_MAX;
      else
	i=PT_FLAT;
      break;
    case 2:
      i=PT_SLOPE;
      break;
    case 4:
      i=PT_SADDLE;
      break;
    case 6:
      i=PT_MONKEY;
      break;
    default:
      i=PT_GRASS;
  }
  return i;
}

/* To subdivide a triangle:
 * 1. Find all critical points in the interior and on the edges.
 * 2. Connect the interior critical points to each other.
 * 3. Connect each interior critical point to all corners and edge critical points.
 * 4. Connect the edge critical points to each other.
 * 5. Connect each corner to the critical points on the opposite edge.
 * 6. Sort all these segments by their numbers of extrema and by length.
 * 7. Of any two segments that intersect in ACXBD, ACTBD, or BDTAC manner,
 *    remove the one with more extrema.
 *    If they have the same number of extrema, remove the longer.
 */

void clip1(const xy &astart,xy &a,const xy &x,xy &b,const xy &bstart)
// If x is between a and b, moves a or b to x so that the midpoint is still between them.
{
  double d01,d02,d03,d14,d24,d34;
  d01=dist(astart,a);
  d02=dist(astart,x);
  d03=dist(astart,b);
  d14=dist(a,bstart);
  d24=dist(x,bstart);
  d34=dist(b,bstart);
  if (d01<d02 && d02<d03 && d14>d24 && d24>d34)
  {
    if (d02>d24)
      b=x;
    else if (d24>d02)
      a=x;
  }
}
