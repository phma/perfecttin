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

double triangle::elevation(xy pnt)
/* Computes the elevation of triangle tri at the point x,y.
 * This is the oldest function in Decisite, attested in C from 2007-12-13,
 * though since all files in the directory have the same date, I probably
 * wrote it earlier and copied it.
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

void triangle::setcentercp()
{
#ifndef FLATTRIANGLE
  ctrl[3]=((ctrl[0]+ctrl[1]+ctrl[2]+ctrl[4]+ctrl[5]+ctrl[6])*3-(a->z+b->z+c->z)*2)/12;
#endif
}

bool triangle::iscorner(point *v)
{
  return (a==v)||(b==v)||(c==v);
}

void triangle::flatten()
{
#ifndef FLATTRIANGLE
  ctrl[0]=(2*a->z+b->z)/3;
  ctrl[1]=(2*a->z+c->z)/3;
  ctrl[2]=(2*b->z+a->z)/3;
  ctrl[3]=(a->z+b->z+c->z)/3;
  ctrl[4]=(2*c->z+a->z)/3;
  ctrl[5]=(2*b->z+c->z)/3;
  ctrl[6]=(2*c->z+b->z)/3;
  nocubedir=INT_MAX;
  totcritpointcount=0;
  critpoints.clear();
#endif
  sarea=area();
  setgradmat();
}

void triangle::setgradient(xy pnt,xy grad)
// Sets the gradient at corner pnt to grad. If pnt is not a corner,
// either sets the nearest corner or does nothing.
{
  int which;
  double crit;
  crit=1/(2.0*(1/dist(xy(*a),xy(*b))+1/dist(xy(*c),xy(*a))+1/dist(xy(*b),xy(*c))));
  grad/=3; // control points are 1/3 of the way along sides
#ifndef FLATTRIANGLE
  if (dist(pnt,*a)<crit)
  {
    ctrl[0]=a->z+dot(grad,xy(*b)-xy(*a));
    ctrl[1]=a->z+dot(grad,xy(*c)-xy(*a));
  }
  if (dist(pnt,*b)<crit)
  {
    ctrl[5]=b->z+dot(grad,xy(*c)-xy(*b));
    ctrl[2]=b->z+dot(grad,xy(*a)-xy(*b));
  }
  if (dist(pnt,*c)<crit)
  {
    ctrl[4]=c->z+dot(grad,xy(*a)-xy(*c));
    ctrl[6]=c->z+dot(grad,xy(*b)-xy(*c));
  }
  nocubedir=INT_MAX;
#endif
  sarea=area();
  setgradmat();
}

double triangle::ctrlpt(xy pnt1,xy pnt2)
/* Returns the elevation of the control point 1/3 of the way from pnt1 to pnt2.
 * If they aren't different corners of the triangle, returns the one in the middle.
 */
{
  int which;
  double crit,ret;
  crit=1/(2.0*(1/dist(xy(*a),xy(*b))+1/dist(xy(*c),xy(*a))+1/dist(xy(*b),xy(*c))));
  which=(dist(pnt1,*a)<crit)+2*(dist(pnt1,*b)<crit)+3*(dist(pnt1,*c)<crit)
       +4*(dist(pnt2,*a)<crit)+8*(dist(pnt2,*b)<crit)+12*(dist(pnt2,*c)<crit);
#ifdef FLATTRIANGLE
  switch (which&3)
  {
    case 1:
      ret=2*a->z;
      break;
    case 2:
      ret=2*b->z;
      break;
    case 3:
      ret=2*c->z;
      break;
  }
  switch (which&12)
  {
    case 4:
      ret+=a->z;
      break;
    case 8:
      ret+=b->z;
      break;
    case 12:
      ret+=c->z;
      break;
  }
  ret/=3;
#else
  ret=ctrl[ctrlpttab[which]];
#endif
  assert(which<16); // If this fails, there's a bug in ctrlpt. This bug has been fixed.
  assert(ctrlpttab[which]!=3); // If this fails, there's a bug in the caller.
  return ret;
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

#ifndef FLATTRIANGLE
xy triangle::spcoord(double x,double y)
/* Given semiperimeter coordinates rotated to nocubedir,
 * returns coordinates in the global coordinate system.
 */
{
  double s;
  xy along,across,cen;
  s=peri/2;
  cen=centroid();
  along=cossin(nocubedir)*s;
  across=cossin(nocubedir+DEG90)*s;
  return cen+across*y+along*x;
}
#endif

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

void triangle::writeXml(ofstream &ofile,pointlist &pl)
{
  int i;
  ofile<<"<triangle corners=\"";
  ofile<<pl.revpoints[a]<<' '<<pl.revpoints[b]<<' '<<pl.revpoints[c];
  ofile<<"\" acicularity=\""<<acicularity();
#ifndef FLATTRIANGLE
  ofile<<"\" control=\"";
  for (i=0;i<7;i++)
  {
    if (i)
      ofile<<' ';
    ofile<<ldecimal(ctrl[i]);
  }
#endif
  ofile<<"\" />";
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

#ifndef FLATTRIANGLE
int triangle::findnocubedir()
/* The range of atan2i is [-0x40000000,0x40000000] ([-180°,180°]).
 * nocubedir is found by adding 0x15555555 (60°) and 0x2aaaaaab (120°)
 * to atan2i (direction of 1st harmonic of 3rd derivative) and searching
 * between them. It is therefore in [-0x2aaaaaab,0x6aaaaaab] and cannot
 * be 0x7fffffff, hence nocubedir is set to 0x7fffffff (MAXINT) to
 * indicate that it has not been computed.
 * 
 * The 3rd derivative as a function of angle has the form a×sin(θ-b)+c×sin(3θ-d).
 * 1. Find b.
 * 2. Compute 3rd deriv at b+60° and b+120°. These add up to 0. Normally they
 * will be opposite in sign, in which case you can look for the zero between them.
 * But if they have the same sign, try b+30 and b+90° If those still have
 * the same sign, try b+90° and b+150. If even those have the same sign,
 * the 3d deriv is identically 0 and you're seeing roundoff error; return b+90°.
 * 
 * This uses Brent's root-finding algorithm.
 */
{
  int i,b,d,beg,mid,end;
  double d3a45[4],begderiv,midderiv,endderiv,crit;
  brent br;
  //cout<<"findnocubedir ";
  for (i=0;i<4;i++)
  {
    d3a45[i]=deriv3(xsect(i<<28,0)); // every 45°
    //cout<<d3a45[i]<<' ';
  }
  //cout<<hex<<degtobin(330)<<endl;
  b=atan2i(d3a45[2]+M_SQRT1_2*(d3a45[1]+d3a45[3]),d3a45[0]-M_SQRT1_2*(d3a45[3]-d3a45[1]));
  d=atan2i(d3a45[2]-M_SQRT1_2*(d3a45[1]+d3a45[3]),d3a45[0]-M_SQRT1_2*(d3a45[1]-d3a45[3]));
  beg=b+0x15555555;
  end=b+0x2aaaaaab;
  begderiv=deriv3(xsect(beg,0));
  endderiv=deriv3(xsect(end,0));
  if (begderiv*endderiv>0)
  {
    beg=b+0xaaaaaab;
    end=b+0x20000000;
    begderiv=deriv3(xsect(beg,0));
    endderiv=deriv3(xsect(end,0));
  }
  if (begderiv*endderiv>0)
  {
    beg=b+0x20000000;
    end=b+0x35555555;
    begderiv=deriv3(xsect(beg,0));
    endderiv=deriv3(xsect(end,0));
  }
  if (begderiv*endderiv>0)
  {
    beg=end=b+0x10000000;
    begderiv=endderiv=deriv3(xsect(end,0));
  }
  nocubedir=br.init(0,begderiv,end-beg,endderiv,true);
  //br.setdebug(true);
  if (end==beg)
    nocubedir=0;
  while (end!=beg && !br.finished())
    nocubedir=br.step(deriv3(xsect(beg+nocubedir,0)));
  nocubedir+=beg;
  return nocubedir;
}
#endif

double triangle::flatoffset()
/* The offset at which the cross-section in the nocubedir direction has zero quadratic component.
 * If it's in the range [-1.5,1.5], it is valid. If outside this range, it may be made up.
 */
{
  double minusquad,plusquad,offset;
  //int cdir0,cdir1;
#ifdef FLATTRIANGLE
  offset=0;
#else
  if (nocubedir==INT_MAX)
  {
    //cdir0=findnocubedir0();
    nocubedir=findnocubedir();
    //if (cdir0!=cdir1)
    //  cout<<"Cdirs don't match "<<cdir0<<"!="<<cdir1<<endl;
  }
  minusquad=deriv2(xsect(nocubedir,-1.5));
  plusquad=deriv2(xsect(nocubedir,1.5));
  offset=1.5*(minusquad+plusquad)/(minusquad-plusquad);
  if (!isfinite(offset))
    offset=-1048576;
#endif
  return offset;
}

#ifndef FLATTRIANGLE
double triangle::vtxeloff(double off)
{
  double vtx;
  vtx=paravertex(xsect(nocubedir,off));
  return spelevation(nocubedir,vtx,off);
}

vector<xyz> triangle::slices(bool side)
/* The xyz's are in semiperimeter coordinates rotated to nocubedir.
 * side=true for the positive offset side.
 */
{
  int i,signch=0;
  vector<xyz> tranches;
  double off,vtx,z,flat;
  bool stop=false;
  flat=flatoffset();
  for (i=0,off=side?1.7:-1.7;!stop;i++,off=(off*16+flat)/17)
  {
    vtx=paravertex(xsect(nocubedir,off));
    z=spelevation(nocubedir,vtx,off);
    tranches.push_back(xyz(vtx,off,z));
    if (i)
    {
      if (abs(off-flat)<3e-6)
	stop=true;
      if (abs(off)>2)
	stop=true; // flat is not in (-1.5,1.5)
      signch+=(tranches[i].x>0)^(tranches[i-1].x>0);
      if (signch>2)
	stop=true;
      if (abs(tranches[i].x)>abs(tranches[i-1].x) && abs(tranches[i-1].x)>1.5 && (tranches[i].x>0)==(tranches[i-1].x>0))
	stop=true;
    }
  }
  return tranches;
}

/* Four-point successive parabolic interpolation is done using the paravertex function as follows:
 * x a     b     c     d   -> e     f     a     b
 *   a  x  b     c     d   ->             a e f b
 *   a     b  x  c     d   ->                   b e f c
 *   a     b     c  x  d   ->                         c e f d
 *   a     b     c     d x ->                         c     d     e     f
 */

xy triangle::critical_point(double start,double startz,double end,double endz)
// start and end are offsets perpendicular to nocubedir
{
  int lw=0,lastlw=0,i=0;
  double flw,vtx,p;
  vector<double> y(4),z(4); // x and y are semiperimeter coordinates
  y[3]=1;
  y[1]=start;
  y[2]=end;
  z[1]=startz;
  z[2]=endz;
  while (fabs(y[3]-y[0])*peri>1e-6 && abs(lw*lastlw)<4 && lw<100 && (i<10 || fabs(deriv2(z))>1e-9))
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
	z[1]=vtxeloff(y[1]);
	z[0]=vtxeloff(y[0]);
	break;
      case -1:
      case 0:
      case 1:
	y[1]=(2*y[0]+y[3])/3;
	y[2]=(2*y[3]+y[0])/3;
	z[1]=vtxeloff(y[1]);
	z[2]=vtxeloff(y[2]);
	break;
      case 2:
	y[2]=2*y[1]-y[0];
	y[3]=2*y[2]-y[1];
	z[3]=vtxeloff(y[3]);
	z[2]=vtxeloff(y[2]);
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
    i++;
  }
  if (abs(lw*lastlw)<4)
  {
    vtx=paravertex(xsect(nocubedir,(y[0]*(1.5-p)+y[3]*(1.5+p))/3));
    return spcoord(vtx,(y[0]*(1.5-p)+y[3]*(1.5+p))/3);
  }
  else
  {
    vtx=nan("");
    return xy(vtx,vtx);
  }
}
#endif

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
#ifndef FLATTRIANGLE

vector<xy> triangle::criticalpts_side(bool side)
{
  vector<xyz> tranches;
  vector<xy> critpts;
  int i;
  tranches=slices(side);
  for (i=1;i<tranches.size()-1;i++)
    if ((tranches[i-1].elev()<tranches[i].elev())^(tranches[i].elev()<tranches[i+1].elev()))
      critpts.push_back(critical_point(tranches[i-1].north(),tranches[i-1].elev(),tranches[i+1].north(),tranches[i+1].elev()));
  return critpts;
}

vector<xy> triangle::criticalpts_axis()
/* Finds critical points on the flat axis.
 * There are at most two, which are necessarily saddle points.
 * If there is one, it's a monkey saddle point or chair point, and may be hard to recognize.
 */
{
  vector<xy> critpts;
  vector<double> pside,mside,diff,along;
  double flat,pvertex,mvertex,dvertex,a,b,c,disc,velev,around[8];
  int i,signflips;
  flat=flatoffset();
  pside=xsect(nocubedir,flat+3e-06);
  mside=xsect(nocubedir,flat-3e-06);
  for (i=0;i<4;i++)
    diff.push_back(pside[i]-mside[i]);
  pvertex=paravertex(pside);
  mvertex=paravertex(mside);
  if (fabs(pvertex)<1.5 && fabs(mvertex)<1.5)
  {
    c=deriv0(diff);
    b=deriv1(diff);
    a=deriv2(diff)/2;
    disc=b*b-4*a*c;
    if (disc>0 && fabs(sqrt(disc)/a)<1e-5) // sometimes in monkey, the two points are just over 3e-6 apart
      disc=0; // but it's really only one saddle point of multiplicity 2
    if (disc==0)
      along.push_back(-b/(2*a));
    if (disc>0)
    {
      along.push_back((-b+sqrt(disc))/(2*a));
      along.push_back((-b-sqrt(disc))/(2*a));
    }
    /* If no points found, check for a saddle point (which must be a monkey
    * saddle point) at the vertex.
    */
    if (!along.size())
    {
      dvertex=paravertex(diff);
      velev=spelevation(nocubedir,dvertex,flat);
      around[0]=spelevation(nocubedir,dvertex+0.000070,flat+0.000169)-velev;
      around[1]=spelevation(nocubedir,dvertex+0.000169,flat+0.000070)-velev;
      around[2]=spelevation(nocubedir,dvertex+0.000169,flat-0.000070)-velev;
      around[3]=spelevation(nocubedir,dvertex+0.000070,flat-0.000169)-velev;
      around[4]=spelevation(nocubedir,dvertex-0.000070,flat-0.000169)-velev;
      around[5]=spelevation(nocubedir,dvertex-0.000169,flat-0.000070)-velev;
      around[6]=spelevation(nocubedir,dvertex-0.000169,flat+0.000070)-velev;
      around[7]=spelevation(nocubedir,dvertex-0.000070,flat+0.000169)-velev;
      for (i=signflips=0;i<8;i++)
	signflips+=around[i]*around[(i+1)&7]<0;
      if (signflips>=3)
	along.push_back(dvertex);
    }
  }
  for (i=0;i<along.size();i++)
    critpts.push_back(spcoord(along[i],flat));
  return critpts;
}
#endif

void triangle::findcriticalpts()
/* On the Raspberry Pi, this has in the past found critical points
 * on a flat table. An unititalized variable may be involved.
 * See https://github.com/phma/bezitopo/issues/1 .
 */
{
#ifndef FLATTRIANGLE
  vector<xy> critpts,ret;
  int i;
  critpts=criticalpts_side(false);
  for (i=0;i<critpts.size();i++)
    if (in(critpts[i]))
      ret.push_back(critpts[i]);
  critpts=criticalpts_side(true);
  for (i=0;i<critpts.size();i++)
    if (in(critpts[i]))
      ret.push_back(critpts[i]);
  critpts=criticalpts_axis();
  for (i=0;i<critpts.size();i++)
    if (in(critpts[i]))
      ret.push_back(critpts[i]);
  critpoints=ret;
#endif
}

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

void triangle::setsubslopes(segment &s)
/* Sets the slopes of a segment that subdivides a triangle.
 * If one end is a primary critical point, its slope is set to 0.
 * If one end is a secondary critical point (a max or min found on a subdividing
 * segment), the slope is 0 iff the segment is part of the segment where
 * the point was found. Those segments are produced by splitting the original
 * segment; the end slope may not be exactly 0, but there is no need to call
 * this method. So this method ignores secondary critical points.
 */
{
#ifndef FLATTRIANGLE
  int i;
  xy dir;
  dir=cossin(s.chordbearing());
  for (i=0;i<critpoints.size();i++)
    if (xy(s.getstart())==critpoints[i])
      break;
  if (i<critpoints.size())
    s.setslope(START,0);
  else
    s.setslope(START,dot(gradient(s.getstart()),dir));
  for (i=0;i<critpoints.size();i++)
    if (xy(s.getend())==critpoints[i])
      break;
  if (i<critpoints.size())
    s.setslope(END,0);
  else
    s.setslope(END,dot(gradient(s.getend()),dir));
#endif
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

void triangle::subdivide()
{
#ifndef FLATTRIANGLE
  int h,i,j,n,newcrit,round,nExtraSegments;
  inttype itype;
  bool del;
  xyz cr;
  xy dir;
  edge *sid;
  segment newseg0,newseg1;
  vector<xyz> sidea,sideb,sidec;
  vector<int> next;
  vector<double> lens,vex;
  vector<xy> morecritpoints;
  vector<int> critdir;
  vector<segment> subdivcopy;
  multimap<double,int> failIntersection;
  multimap<double,int>::iterator fi;
  morecritpoints=critpoints;
  for (i=0;i<critpoints.size();i++)
    critdir.push_back(INT_MAX);
  round=0;
  do
  {
    subdiv.clear();
    sid=a->edg(this);
    for (i=0;i<2;i++)
      if (isfinite(sid->extrema[i]))
	sideb.push_back(sid->critpoint(i));
    sid=b->edg(this);
    for (i=0;i<2;i++)
      if (isfinite(sid->extrema[i]))
	sidec.push_back(sid->critpoint(i));
    sid=c->edg(this);
    for (i=0;i<2;i++)
      if (isfinite(sid->extrema[i]))
	sidea.push_back(sid->critpoint(i));
    for (i=0;i<morecritpoints.size();i++)
      for (j=0;j<i;j++)
	subdiv.push_back(segment(xyz(morecritpoints[i],elevation(morecritpoints[i])),xyz(morecritpoints[j],elevation(morecritpoints[j]))));
    for (i=0;i<morecritpoints.size();i++)
    {
      cr=xyz(morecritpoints[i],elevation(morecritpoints[i]));
      for (j=0;j<sidea.size();j++)
	subdiv.push_back(segment(cr,sidea[j]));
      for (j=0;j<sideb.size();j++)
	subdiv.push_back(segment(cr,sideb[j]));
      for (j=0;j<sidec.size();j++)
	subdiv.push_back(segment(cr,sidec[j]));
      subdiv.push_back(segment(cr,*a));
      subdiv.push_back(segment(cr,*b));
      subdiv.push_back(segment(cr,*c));
    }
    for (i=0;i<sidea.size();i++)
    {
      subdiv.push_back(segment(*a,sidea[i]));
      for (j=0;j<sideb.size();j++)
	subdiv.push_back(segment(sidea[i],sideb[j]));
    }
    for (i=0;i<sideb.size();i++)
    {
      subdiv.push_back(segment(*b,sideb[i]));
      for (j=0;j<sidec.size();j++)
	subdiv.push_back(segment(sideb[i],sidec[j]));
    }
    for (i=0;i<sidec.size();i++)
    {
      subdiv.push_back(segment(*c,sidec[i]));
      for (j=0;j<sidea.size();j++)
	subdiv.push_back(segment(sidec[i],sidea[j]));
    }
    for (i=0;i<subdiv.size();i++)
    {
      dir=cossin(subdiv[i].chordbearing());
      setsubslopes(subdiv[i]);
      next.push_back(subdiv[i].vextrema(false).size());
      lens.push_back(subdiv[i].length());
    }
    for (h=31;h;h/=2) // Shell sort. The maximum possible number of lines is 60.
      for (i=h;i<subdiv.size();i++)
	for (j=i-h;j>=0 && (next[j]>next[j+h] || (next[j]==next[j+h] && lens[j]>lens[j+h]));j-=h)
	{
	  swap(lens[j],lens[j+h]);
	  swap(next[j],next[j+h]);
	  swap(subdiv[j],subdiv[j+h]);
	}
    //for (i=0;i<subdiv.size();i++)
      //cout<<i<<' '<<setprecision(3)<<bintodeg(subdiv[i].chordbearing())<<' '<<subdiv[i].startslope()<<' '<<subdiv[i].endslope()<<' '<<next[i]<<' '<<lens[i]<<endl;
    subdivcopy=subdiv;
    for (i=subdiv.size()-1;i>0;i--)
      for (del=j=0;j<i && !del;j++)
      {
	itype=intersection_type(subdiv[i],subdiv[j]);
	//cout<<i<<' '<<j<<' '<<inttype_str(itype)<<endl;
	switch (itype)
	{
	  case NOINT:
	  case COINC: // can't happen
            break;
	  case COLIN: // may need special treatment
            cout<<"subdiv COLIN\n";
            break;
	  case IMPOS:
            cout<<"subdiv IMPOS\n";
            break;
	  case ACVBD:
	    break;
	  case ACTBD: // This case is unusual and would require deleting subdiv[j].
            cout<<"subdiv ACTBD\n";
	    break;    // I'm ignoring it for now.
	  case BDTAC:
	  case ACXBD:
	    del=true;
	    break;
	}
	if (del)
	{
	  subdiv.erase(subdiv.begin()+i);
	  next.erase(next.begin()+i);
	  lens.erase(lens.begin()+i);
	}
      }
    for (i=0;i<subdivcopy.size();i++)
    {
      for (j=0;j<subdiv.size();j++)
      {
	itype=intersection_type(subdivcopy[i],subdiv[j]);
	if (itype==ACXBD || (subdivcopy[i]==subdiv[j]))
	  j=2*subdivcopy.size();
      }
      if (j==subdiv.size())
      {
	subdiv.push_back(subdivcopy[i]);
	next.push_back(subdivcopy[i].vextrema(false).size());
	lens.push_back(subdivcopy[i].length());
      }
    }
    n=subdiv.size();
    for (i=newcrit=0;i<n;i++)
    {
      vex=subdiv[i].vextrema(false);
      if (vex.size())
      {
	++newcrit;
	morecritpoints.push_back(subdiv[i].station(vex[0]));
        //if (fabs(frac(morecritpoints.back().getx())-0.152)<0.001
        //    && fabs(frac(morecritpoints.back().gety())-0.380)<0.001)
        //  cout<<"point I "<<ldecimal(morecritpoints.back().getx())<<','<<
        //        ldecimal(morecritpoints.back().gety())<<endl; // See testcontour.
	critdir.push_back(subdiv[i].chordbearing());
	subdiv[i].split(vex[0],newseg0,newseg1);
	subdiv[i]=newseg0;
	subdiv.push_back(newseg1);
        //if (subdiv[i].chordbearing()!=subdiv.back().chordbearing())
          //cout<<"splitting segment "<<i<<' '<<foldangle(subdiv[i].chordbearing()-critdir.back())
            //<<' '<<foldangle(subdiv.back().chordbearing()-critdir.back())<<endl;
      }
    }
    subdivcopy.clear();
    //cout<<morecritpoints.size()-critpoints.size()<<" secondary critical points"<<endl;
    for (i=critpoints.size();i<morecritpoints.size();i++)
    {
      cr=xyz(morecritpoints[i],elevation(morecritpoints[i]));
      for (j=0;j<i;j++)
      {
        newseg0=segment(cr,xyz(morecritpoints[j],elevation(morecritpoints[j])));
	if (abs(foldangle(critdir[i]-newseg0.chordbearing()))>100)
	  subdivcopy.push_back(newseg0);
      }
      for (j=0;j<sidea.size();j++)
      {
        newseg0=segment(cr,sidea[j]);
	if (abs(foldangle(critdir[i]-newseg0.chordbearing()))>100)
	  subdivcopy.push_back(newseg0);
      }
      for (j=0;j<sideb.size();j++)
      {
        newseg0=segment(cr,sideb[j]);
	if (abs(foldangle(critdir[i]-newseg0.chordbearing()))>100)
	  subdivcopy.push_back(newseg0);
      }
      for (j=0;j<sidec.size();j++)
      {
        newseg0=segment(cr,sidec[j]);
	if (abs(foldangle(critdir[i]-newseg0.chordbearing()))>100)
	  subdivcopy.push_back(newseg0);
      }
      newseg0=segment(cr,*a);
      if (((critdir[i]-newseg0.chordbearing()+1)&(DEG180-1))>2)
	subdivcopy.push_back(newseg0);
      newseg0=segment(cr,*b);
      if (((critdir[i]-newseg0.chordbearing()+1)&(DEG180-1))>2)
	subdivcopy.push_back(newseg0);
      newseg0=segment(cr,*c);
      if (((critdir[i]-newseg0.chordbearing()+1)&(DEG180-1))>2)
	subdivcopy.push_back(newseg0);
    }
    for (i=0;i<subdivcopy.size();i++)
    {
      setsubslopes(subdivcopy[i]);
      for (j=0;j<subdiv.size();j++)
      {
	itype=intersection_type(subdivcopy[i],subdiv[j]);
        /* For some strange reason, sometimes subdiv[j] and subdivcopy[i]
         * have the same xy coordinates of their start and end points,
         * in reverse order, but different z coordinates. Therefore the
         * collinearity test is necessary.
         */
        if (itype==ACXBD || itype==COLIN || subdivcopy[i]==subdiv[j] || subdivcopy[i]==-subdiv[j])
	  j=2*subdiv.size()+1;
      }
      if (j==subdiv.size())
      {
	subdiv.push_back(subdivcopy[i]);
	next.push_back(subdivcopy[i].vextrema(false).size());
	lens.push_back(subdivcopy[i].length());
      }
    }
    round++;
  }
  while (newcrit>0 && round<1);
  /*for (i=0;i<subdiv.size();i++)
    cout<<i<<' '<<setprecision(3)<<bintodeg(subdiv[i].chordbearing())<<' '<<subdiv[i].startslope()<<' '<<subdiv[i].endslope()<<' '<<next[i]<<' '<<lens[i]<<endl;*/
  /* Tracing errors are caused by extra subdivision segments. The correct
   * number is 1 segment for every side critical point and 3 for every
   * interior critical point. With the boundary added, it's 2 for every
   * side critical point and 3 for every interior critical point plus 3.
   */
  totcritpointcount=morecritpoints.size();
  if (subdiv.size()!=3*totcritpointcount+sidea.size()+sideb.size()+sidec.size())
  {
    nExtraSegments=subdiv.size()-3*totcritpointcount-sidea.size()-sideb.size()-sidec.size();
    //cout<<"centroid "<<ldecimal(centroid().getx())<<','<<ldecimal(centroid().gety())<<'\n';
    //cout<<morecritpoints.size()<<" interior critpoints ("<<morecritpoints.size()-critpoints.size()<<" secondary) ";
    //cout<<sidea.size()+sideb.size()+sidec.size()<<" side critpoints "<<subdiv.size()<<" subdivs\n";
    for (i=subdivcopy.size();i<subdiv.size();i++)
      for (j=0;j<subdivcopy.size();j++)
      {
        //cout<<i<<' '<<j<<' '<<inttype_str(intersection_type(subdiv[i],subdiv[j]))
          //<<' '<<missDistance(subdiv[i],subdiv[j])<<endl;
        if (intersection_type(subdiv[i],subdiv[j])==NOINT)
          failIntersection.insert(pair<double,int>(missDistance(subdiv[i],subdiv[j]),i));
      }
    for (fi=failIntersection.begin();nExtraSegments && fi!=failIntersection.end();++fi)
    {
      if (subdiv[fi->second].length())
        nExtraSegments--;
      subdiv[fi->second]=segment();
    }
    for (i=0,j=subdiv.size()-1;i<j;)
    {
      while (i<subdiv.size() && subdiv[i].length()>0)
        i++;
      while (j>=0 && subdiv[j].length()==0)
        j--;
      if (i<j)
        swap(subdiv[i],subdiv[j]);
    }
    subdiv.resize(i);
  }
#endif
}

/* 2015-07-12: There was a bug in addperimeter.
 * A triangle of the TIN of asteraceous pattern:
 * 9:  -2.8490243449190173,-1.1760358336627177
 * 14: -2.1900276194166555,-3.1150889274934381
 * 6:  -0.66186370825269425,-2.4621000044063992
 * Area: 1.6967542609633028
 *
 * With test surface HYPAR this triangle was subdivided as follows:
 * 0: -2.8490243449190173,-1.1760358336627177 -> -1.3206809836707525,-2.7436145592545782
 * 1: -2.8490243449190173,-1.1760358336627177 -> -0.67221800596207093,-2.4665244261985735
 * 2: -2.8490243449190173,-1.1760358336627177 -> -2.1900276194166555,-3.1150889274934381
 * 3: -2.1900276194166555,-3.1150889274934381 -> -1.3206809836707525,-2.7436145592545782
 * 4: -1.3206809836707525,-2.7436145592545782 -> -0.67221800596207093,-2.4665244261985735
 * 5: -1.3206809836707525,-2.7436145592545782 -> -0.66186370825269425,-2.4621000044063992
 * 6: -0.66186370825269425,-2.4621000044063992 -> -2.8490243449190173,-1.1760358336627177
 *
 * It should be subdivided like this:
 * 0: -2.8490243449190173,-1.1760358336627177 -> -1.3206809836707525,-2.7436145592545782
 * 1: -2.8490243449190173,-1.1760358336627177 -> -0.67221800596207093,-2.4665244261985735
 * 2: -2.8490243449190173,-1.1760358336627177 -> -2.1900276194166555,-3.1150889274934381
 * 3: -2.1900276194166555,-3.1150889274934381 -> -1.3206809836707525,-2.7436145592545782
 * 4: -1.3206809836707525,-2.7436145592545782 -> -0.67221800596207093,-2.4665244261985735
 * 5: -0.67221800596207093,-2.4665244261985735 -> -0.66186370825269425,-2.4621000044063992
 * 6: -0.66186370825269425,-2.4621000044063992 -> -2.8490243449190173,-1.1760358336627177
 */

void triangle::addperimeter()
{
  int i,oldnumber;
  int sizeWithPerimeter,sizeWithoutPerimeter;
  edge *sid;
  vector<xyz> sidea,sideb,sidec;
  sid=a->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sideb.push_back(sid->critpoint(i));
  sid=b->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sidec.push_back(sid->critpoint(i));
  sid=c->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sidea.push_back(sid->critpoint(i));
  if (sidec.size()>1 && dist(xy(*a),xy(sidec[0]))>dist(xy(*a),xy(sidec[1])))
    swap(sidec[0],sidec[1]);
  if (sidea.size()>1 && dist(xy(*b),xy(sidea[0]))>dist(xy(*b),xy(sidea[1])))
    swap(sidea[0],sidea[1]);
  if (sideb.size()>1 && dist(xy(*c),xy(sideb[0]))>dist(xy(*c),xy(sideb[1])))
    swap(sideb[0],sideb[1]);
#ifdef FLATTRIANGLE
  sizeWithPerimeter=2*(sidea.size()+sideb.size()+sidec.size())+3;
  sizeWithoutPerimeter=(sidea.size()+sideb.size()+sidec.size());
#else
  sizeWithPerimeter=2*(sidea.size()+sideb.size()+sidec.size())+3*totcritpointcount+3;
  sizeWithoutPerimeter=(sidea.size()+sideb.size()+sidec.size())+3*totcritpointcount;
#endif
  oldnumber=subdiv.size();
  if (oldnumber<sizeWithPerimeter)
  {
    if (sidec.size())
    {
      subdiv.push_back(segment(*a,sidec[0]));
      for (i=0;i<sidec.size()-1;i++)
        subdiv.push_back(segment(sidec[i],sidec[i+1]));
      subdiv.push_back(segment(sidec[i],*b));
    }
    else
      subdiv.push_back(segment(*a,*b));
    if (sidea.size())
    {
      subdiv.push_back(segment(*b,sidea[0]));
      for (i=0;i<sidea.size()-1;i++)
        subdiv.push_back(segment(sidea[i],sidea[i+1]));
      subdiv.push_back(segment(sidea[i],*c));
    }
    else
      subdiv.push_back(segment(*b,*c));
    if (sideb.size())
    {
      subdiv.push_back(segment(*c,sideb[0]));
      for (i=0;i<sideb.size()-1;i++)
        subdiv.push_back(segment(sideb[i],sideb[i+1]));
      subdiv.push_back(segment(sideb[i],*a));
    }
    else
      subdiv.push_back(segment(*c,*a));
    for (i=oldnumber;i<subdiv.size();i++)
      setsubslopes(subdiv[i]);
  }
  assert(subdiv.size()>=3);
}

void triangle::removeperimeter()
{
  int acnt,bcnt,ccnt,i;
  int sizeWithPerimeter,sizeWithoutPerimeter;
  edge *sid;
  vector<xyz> sidea,sideb,sidec;
  sid=a->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sideb.push_back(sid->critpoint(i));
  sid=b->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sidec.push_back(sid->critpoint(i));
  sid=c->edg(this);
  for (i=0;i<2;i++)
    if (isfinite(sid->extrema[i]))
      sidea.push_back(sid->critpoint(i));
#ifdef FLATTRIANGLE
  sizeWithPerimeter=2*(sidea.size()+sideb.size()+sidec.size())+3;
  sizeWithoutPerimeter=(sidea.size()+sideb.size()+sidec.size());
#else
  sizeWithPerimeter=2*(sidea.size()+sideb.size()+sidec.size())+3*totcritpointcount+3;
  sizeWithoutPerimeter=(sidea.size()+sideb.size()+sidec.size())+3*totcritpointcount;
#endif
  if (subdiv.size()>sizeWithoutPerimeter)
  {
    for (i=subdiv.size()-1,acnt=bcnt=ccnt=0;i>=0 && acnt<3 && bcnt<3 && ccnt<3 && acnt+bcnt+ccnt<6;i--)
    {
      if (subdiv[i].getstart()==*a)
        acnt++;
      if (subdiv[i].getend()==*a)
        acnt++;
      if (subdiv[i].getstart()==*b)
        bcnt++;
      if (subdiv[i].getend()==*b)
        bcnt++;
      if (subdiv[i].getstart()==*c)
        ccnt++;
      if (subdiv[i].getend()==*c)
        ccnt++;
    }
    i++;
    assert(subdiv.size()-i<=9);
    if (acnt==2 && bcnt==2 && ccnt==2)
    {
      subdiv.resize(i);
      subdiv.shrink_to_fit();
    }
    else
      assert(acnt==2 && bcnt==2 && ccnt==2);
  }
  assert(subdiv.size()==sizeWithoutPerimeter);
}

array<double,4> triangle::lohi()
/* Returns an array of four numbers: the lowest elevation anywhere in the triangle,
 * the lowest elevation on the perimeter, the highest elevation on the perimeter,
 * and the highest elevation anywhere.
 */
{
  int i;
  double e;
  edge *sid=NULL;
  array<double,4> ret;
  ret[1]=ret[2]=a->z;
  if (b->z<ret[1])
    ret[1]=b->z;
  if (b->z>ret[2])
    ret[2]=b->z;
  if (c->z<ret[1])
    ret[1]=c->z;
  if (c->z>ret[2])
    ret[2]=c->z;
  sid=a->edg(this);
  for (i=0;i<2;i++)
  {
    e=sid->critpoint(i).z;
    if (std::isfinite(e))
    {
      if (e<ret[1])
	ret[1]=e;
      if (e>ret[2])
	ret[2]=e;
    }
  }
  sid=b->edg(this);
  for (i=0;i<2;i++)
  {
    e=sid->critpoint(i).z;
    if (std::isfinite(e))
    {
      if (e<ret[1])
	ret[1]=e;
      if (e>ret[2])
	ret[2]=e;
    }
  }
  sid=c->edg(this);
  for (i=0;i<2;i++)
  {
    e=sid->critpoint(i).z;
    if (std::isfinite(e))
    {
      if (e<ret[1])
	ret[1]=e;
      if (e>ret[2])
	ret[2]=e;
    }
  }
  ret[0]=ret[1];
  ret[3]=ret[2];
#ifndef FLATTRIANGLE
  for (i=0;i<critpoints.size();i++)
  {
    e=elevation(critpoints[i]);
    if (e<ret[0])
      ret[0]=e;
    if (e>ret[3])
      ret[3]=e;
  }
#endif
  return ret;
}

/* Convert an index number to a triangle's subdiv to and from a pointer to edge
 * with a part number added. This is used to transfer the subdiv from one triangle
 * to the next when drawing contours. The index number has bit 16 set or clear
 * to indicate which side of the segment the next point of the contour is on.
 * If it's set, you're crossing the segment rightward, which is to the outside
 * if the segment is on the perimeter.
 */
uintptr_t triangle::edgepart(int subdir) // subdir should have bit 16 set, but it's ignored
{
  int acnt,bcnt,ccnt,apos,bpos,cpos,i,base,pieces;
  edge *sid=NULL;
  bool backward;
  uintptr_t ret;
  for (i=subdiv.size()-1,acnt=bcnt=ccnt=0;i>=0 && acnt<3 && bcnt<3 && ccnt<3 && acnt+bcnt+ccnt<6;i--)
  {
    if (subdiv[i].getstart()==*a)
    {
      apos=i;
      acnt++;
    }
    if (subdiv[i].getend()==*a)
      acnt++;
    if (subdiv[i].getstart()==*b)
    {
      bpos=i;
      bcnt++;
    }
    if (subdiv[i].getend()==*b)
      bcnt++;
    if (subdiv[i].getstart()==*c)
    {
      cpos=i;
      ccnt++;
    }
    if (subdiv[i].getend()==*c)
      ccnt++;
  }
  assert (apos<bpos && bpos<cpos);
  subdir&=65535;
  if (subdir>=apos) // the side starting at A is side c
  {
    sid=b->edg(this); // which is found by asking point B
    base=apos;
    pieces=bpos-apos;
  }
  if (subdir>=bpos) // the side starting at B is side a
  {
    sid=c->edg(this); // which is found by asking point C
    base=bpos;
    pieces=cpos-bpos;
  }
  if (subdir>=cpos) // the side starting at C is side b
  {
    sid=a->edg(this); // which is found by asking point A
    base=cpos;
    pieces=subdiv.size()-cpos;
  }
  if (subdir>subdiv.size())
    sid=NULL;
  if (sid)
  {
    backward=area3(*sid->a,*sid->b,centroid())<0;
    ret=(uintptr_t)sid;
    assert((ret&3)==0);
    ret+=backward?(base-subdir+pieces-1):(subdir-base);
  }
  else
    ret=0;
  return ret;
}

int triangle::subdir(uintptr_t edgepart)
{
  int acnt,bcnt,ccnt,apos,bpos,cpos,i,base=-1,pieces;
  edge *sid;
  bool backward;
  int ret;
  for (i=subdiv.size()-1,acnt=bcnt=ccnt=0;i>=0 && acnt<3 && bcnt<3 && ccnt<3 && acnt+bcnt+ccnt<6;i--)
  {
    if (subdiv[i].getstart()==*a)
    {
      apos=i;
      acnt++;
    }
    if (subdiv[i].getend()==*a)
      acnt++;
    if (subdiv[i].getstart()==*b)
    {
      bpos=i;
      bcnt++;
    }
    if (subdiv[i].getend()==*b)
      bcnt++;
    if (subdiv[i].getstart()==*c)
    {
      cpos=i;
      ccnt++;
    }
    if (subdiv[i].getend()==*c)
      ccnt++;
  }
  assert (apos<bpos && bpos<cpos); // if this fails, you may have forgotten to add the perimeter
  sid=(edge *)(edgepart&~3);
  if (sid==b->edg(this))
  {
    base=apos;
    pieces=bpos-apos;
  }
  if (sid==c->edg(this))
  {
    base=bpos;
    pieces=cpos-bpos;
  }
  if (sid==a->edg(this))
  {
    base=cpos;
    pieces=subdiv.size()-cpos;
  }
  if (base>=0)
  {
    backward=area3(*sid->a,*sid->b,centroid())<0;
    ret=base+(backward?(pieces-1-(edgepart&3)):(edgepart&3));
    if ((ret-base)>=pieces || ret<base)
      ret=-1;
  }
  else
    ret=-1;
  return ret;
}

struct prorec
{
  int n;
  double a;
  xy farend;
};

int triangle::proceed(int subdir,double elevation)
{
  int i,j,sign,ret;
  bool done;
  vector<prorec> list;
  prorec p;
  xy s,e;
  sign=1;
  if (subdir&65536)
    sign=-1;
  subdir&=65535;
  if (subdir<subdiv.size())
  {
    s=subdiv[subdir].getstart();
    e=subdiv[subdir].getend();
    for (i=0;i<subdiv.size();i++)
    {
      p.n=-1;
      if (subdiv[i].getstart()==s || subdiv[i].getstart()==e)
      {
	p.n=i;
	p.farend=subdiv[i].getend();
      }
      if (subdiv[i].getend()==s || subdiv[i].getend()==e)
      {
	p.n=i;
	p.farend=subdiv[i].getstart();
      }
      p.a=area3(s,e,p.farend)*sign;
      if (p.a<=0)
	p.n=-1;
      if (p.n>=0)
	list.push_back(p);
    }
    //cout<<subdir<<((sign>0)?'L':'R')<<": "<<list.size()<<" adjacent segments on that side"<<endl;
    // The size of the list varies from 0 to 7. Sort it by area.
    do
    {
      done=true;
      for (i=0;i+1<list.size();i++)
	if (list[i].a>list[i+1].a)
	{
	  done=false;
	  swap(list[i],list[i+1]);
	}
    } while (!done);
    /*for (i=0;i<list.size();i++)
      cout<<i<<' '<<list[i].a<<' '<<setprecision(7)<<
	" ("<<list[i].farend.east()<<','<<list[i].farend.north()<<')'<<endl;*/
    for (i=j=0;i<list.size();i++)
      for (j=i+1;j<list.size() && list[i].a==list[j].a;j++)
	if (list[i].farend==list[j].farend)
	  goto found;
    found:
    //cout<<i<<' '<<j<<endl;
    if (j<list.size())
    {
      if ((subdiv[list[j].n].getstart().elev()<elevation)^(subdiv[list[j].n].getend().elev()<elevation))
	ret=list[j].n;
      else
	ret=list[i].n;
      if (area3(subdiv[ret].getstart(),subdiv[ret].getend(),(s+e)/2)>0)
	ret+=65536;
    }
    else
      ret=-1;
  }
  else
    ret=-2;
  return ret;
}

bool triangle::crosses(int subdir,double elevation)
{
  subdir&=65535;
  if (subdir<subdiv.size())
    return subdiv[subdir].crosses(elevation);
  else
    return false;
}

bool triangle::upleft(int subdir)
/* Returns true if up is on the left.
 * Used to orient contour lines counterclockwise around peaks.
 */
{
  bool sign;
  sign=(subdir&65536)>0;
  subdir&=65535;
  if (subdir<subdiv.size())
    return (subdiv[subdir].getstart().elev()>subdiv[subdir].getend().elev())^sign;
  else
    return false;
}

xy triangle::contourcept(int subdir,double elevation)
{
  subdir&=65535;
  //if (subdir<subdiv.size() && fabs(elevation-0.21)<0.01 && fabs(subdiv[subdir].startslope()+0.183)<0.001 && fabs(subdiv[subdir].endslope()+0.121)<0.001)
  //  cout<<"Contour test spike segment"<<endl;
  if (subdir<subdiv.size())
    return subdiv[subdir].station(subdiv[subdir].contourcept(elevation));
  else
    return xy(NAN,NAN);
}

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

segment triangle::dirclip(const xy pnt,const int dir)
/* This is called when refining contours. By this time, the perimeter
 * has been removed. It returns a segment passing through pnt in the
 * direction dir clipped by all the subdivision lines.
 */
{
  segment ret;
  xy astart=pnt-0.75*peri*cossin(dir),bstart=pnt+0.75*peri*cossin(dir);
  xy aend=astart,bend=bstart;
  xy intpt;
  int i,itype;
  intpt=intersection(aend,bend,*a,*b);
  if (intpt.isfinite())
    clip1(astart,aend,intpt,bend,bstart);
  intpt=intersection(aend,bend,*b,*c);
  if (intpt.isfinite())
    clip1(astart,aend,intpt,bend,bstart);
  intpt=intersection(aend,bend,*c,*a);
  if (intpt.isfinite())
    clip1(astart,aend,intpt,bend,bstart);
  for (i=0;i<subdiv.size();i++)
  {
    itype=intersection_type(aend,bend,subdiv[i].getstart(),subdiv[i].getend());
    if (itype==ACXBD || itype==BDTAC)
    {
      intpt=intersection(aend,bend,subdiv[i].getstart(),subdiv[i].getend());
      clip1(astart,aend,intpt,bend,bstart);
    }
  }
  ret=segment(xyz(aend,elevation(aend)),xyz(bend,elevation(bend)));
  setsubslopes(ret);
  return ret;
}
