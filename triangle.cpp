/********************************************************/
/*                                                      */
/* triangle.cpp - triangles                             */
/*                                                      */
/********************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#include <cstring>
#include <climits>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cfloat>
#include "triangle.h"
#include "angle.h"
#include "threads.h"
#include "ldecimal.h"
#include "tin.h"
#include "lohi.h"
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

ErrorBlockTask::ErrorBlockTask()
{
  dots=nullptr;
  tri=nullptr;
  numDots=0;
  result=nullptr;
}

void computeErrorBlock(ErrorBlockTask &task)
{
  double tempVError=0,err1;
  int i;
  for (i=0;i<task.numDots && tempVError<task.tolerance;i++)
  {
    err1=fabs(task.dots[i].elev()-task.tri->elevation(task.dots[i]));
    if (err1>tempVError)
      tempVError=err1;
  }
  if (task.result)
  {
    task.result->vError=tempVError;
    task.result->ready=true;
  }
}

triangle::triangle()
{
  a=b=c=NULL;
  aneigh=bneigh=cneigh=NULL;
  vError=INFINITY;
  peri=sarea=0;
  memset(gradmat,0,sizeof(gradmat));
  flags=0;
  aElev=bElev=cElev=NAN;
}

bool triangle::ptValid()
{
  return a&&b&&c;
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

bool triangle::inTolerance(double tolerance,double minArea)
/* Returns true if the triangle
 * •would become smaller than minArea if split,
 * •has too few dots to be split, or
 * •has all dots within tolerance (without actually checking them all).
 */
{
  return dots.size()<9 ||
         sarea<3*minArea ||
         ((fabs(aElev-a->elev())+fabs(bElev-b->elev()))+
	  (fabs(cElev-c->elev())+vError))<tolerance;
}

void triangle::setError(double tolerance)
{
  double tempVError=0,err1;
  int i,triDots;
  vector<ErrorBlockTask> tasks;
  vector<ErrorBlockResult> results;
  vector<int> blkSizes;
  bool allReady=false;
  if (dots.size()>TASK_STEP_SIZE*3)
  {
    blkSizes=blockSizes(dots.size());
    for (triDots=i=0;i<blkSizes.size();i++)
    {
      tasks.resize(tasks.size()+1);
      results.resize(results.size()+1);
      tasks.back().tri=this;
      tasks.back().tolerance=tolerance;
      tasks.back().dots=&dots[triDots];
      tasks.back().numDots=blkSizes[i];
      triDots+=blkSizes[i];
    }
    for (i=0;i<tasks.size();i++)
    {
      tasks[i].result=&results[i];
      results[i].ready=false;
    }
    for (i=0;i<tasks.size();i++)
      enqueueError(tasks[i]);
    while (!allReady)
    {
      ErrorBlockTask task=dequeueError();
      computeErrorBlock(task);
      allReady=true;
      for (i=0;i<results.size();i++)
	allReady&=results[i].ready;
    }
    for (tempVError=i=0;i<results.size();i++)
      if (tempVError<results[i].vError)
	tempVError=results[i].vError;
  }
  else
    for (i=0;i<dots.size() && (tempVError<tolerance || i<9);i++)
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

int triangle::quadrant(xy pnt)
/* 0: pnt is in the middle
 * 1: pnt is near A
 * 2: pnt is near B
 * 4: pnt is near C
 * 3,5,6: pnt is outside
 * 7: impossible, unless triangle is backward
 */
{
  return (area3(pnt,*b,*c)>sarea/2)+(area3(*a,pnt,*c)>sarea/2)*2+(area3(*a,*b,pnt)>sarea/2)*4;
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

point *triangle::otherCorner(point *v0,point *v1)
{
  int bm=0;
  if (v0==a || v1==a)
    bm+=1;
  if (v0==b || v1==b)
    bm+=2;
  if (v0==c || v1==c)
    bm+=4;
  switch (bm)
  {
    case 3:
      return c;
    case 5:
      return b;
    case 6:
      return a;
    default:
      return nullptr;
  }
}

void triangle::flatten()
{
  sarea=area();
  peri=perimeter();
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

void triangle::setEdgeTriPointers()
// Sets the edge pointers of the three sides to point to this triangle.
{
  int i;
  edge *line,*oldline;
  line=oldline=a->line;
  for (i=0;line!=oldline || i==0;i++,line=line->next(a))
    if (line->otherend(a)==b)
      break;
  if (line->a==a && line->b==b)
    line->trib=this;
  if (line->a==b && line->b==a)
    line->tria=this;
  line=oldline=b->line;
  for (i=0;line!=oldline || i==0;i++,line=line->next(b))
    if (line->otherend(b)==c)
      break;
  if (line->a==b && line->b==c)
    line->trib=this;
  if (line->a==c && line->b==b)
    line->tria=this;
  line=oldline=c->line;
  for (i=0;line!=oldline || i==0;i++,line=line->next(c))
    if (line->otherend(c)==a)
      break;
  if (line->a==c && line->b==a)
    line->trib=this;
  if (line->a==a && line->b==c)
    line->tria=this;
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

const char nextalongTable[3][3][3]=
{
  7,4,4, 1,1,5, 1,1,1,
  2,4,4, 2,0,4, 3,1,1,
  2,6,4, 2,2,4, 2,2,7
};

triangle *triangle::nextalong(segment &seg)
/* Returns the next triangle along the segment.
 * If the segment does not cross the triangle, tries to get back to the segment.
 * Used when pruning and smoothing contours.
 * If seg is an arc or spiralarc and intersects a side twice, may miss
 * a triangle.
 *
 * During the pruning phase, all segment endpoints are on TIN edges, and it
 * often happens that a segment collinearly overlaps two or more edges.
 * This can result in nextalong returning triangle ploni for almoni, and
 * almoni for ploni, and if the segment's endpoint lies on their common edge
 * but appears, because of roundoff, to be in neither triangle, the result
 * is an infinite loop. This loop is broken in pointlist::lohi and
 * pointlist::insertContourPiece.
 */
{
  double p,q,r,pqr;
  xy fwdpnt,backpnt;
  double close=seg.closest(centroid(),INFINITY,true);
  //if (!isfinite(close))
    //close=seg.length()/2;
  double fwd=close+peri/2,back=close-peri/2;
  fwdpnt=seg.station(fwd);
  backpnt=seg.station(back);
  p=area3(*a,backpnt,fwdpnt);
  q=area3(*b,backpnt,fwdpnt);
  r=area3(*c,backpnt,fwdpnt);
  pqr=fabs(p)+fabs(q)+fabs(r);
  if (fabs(p)<pqr/1e9)
    p=0;
  if (fabs(q)<pqr/1e9)
    q=0;
  if (fabs(r)<pqr/1e9)
    r=0;
  if (close>=seg.length() && dist(centroid(),(xy)seg.getend())>peri/2)
    return nullptr; // we've gone too far
  switch (nextalongTable[sign(p)+1][sign(q)+1][sign(r)+1])
  {
    case 4: // seg exits side a
      return aneigh;
    case 2: // seg exits side b
      return bneigh;
    case 1: // seg exits side c
      return cneigh;
    case 3: // seg exits vertex A
    case 5: // seg exits vertex B
    case 6: // seg exits vertex C
    case 7: // triangle is entirely on one side of seg
      return nexttoward(fwdpnt);
    default: // all three vertices are on seg
      cerr<<"can't happen: straight triangle in nextalong\n";
      return nullptr;
  }
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

array<double,2> triangle::lohi()
/* Returns an array of two numbers: the lowest elevation anywhere in the triangle,
 * and the highest elevation anywhere.
 */
{
  array<double,2> ret;
  ret[0]=ret[1]=a->z;
  updlohi(ret,b->z);
  updlohi(ret,c->z);
  return ret;
}

array<double,2> triangle::lohi(segment seg)
/* Returns the lowest and highest elevations of that part of the segment
 * that lies in the triangle.
 */
{
  segment side;
  array<double,2> ret;
  ret[0]=INFINITY;
  ret[1]=-INFINITY;
  if (in(seg.getstart()))
    updlohi(ret,elevation(seg.getstart()));
  if (in(seg.getend()))
    updlohi(ret,elevation(seg.getend()));
  side=a->edg(this)->getsegment();
  if (intersection_type(seg,side)!=NOINT)
    updlohi(ret,elevation(intersection(seg,side)));
  side=b->edg(this)->getsegment();
  if (intersection_type(seg,side)!=NOINT)
    updlohi(ret,elevation(intersection(seg,side)));
  side=c->edg(this)->getsegment();
  if (intersection_type(seg,side)!=NOINT)
    updlohi(ret,elevation(intersection(seg,side)));
  return ret;
}

/* Unlike Bezitopo, PerfectTIN's triangles are flat and do not have subdivisions.
 * The sides are numbered as if they were subdivisions, with 0 being c,
 * 1 being a, and 2 being b.
 */
edge *triangle::edgepart(int subdir)
{
  edge *sid=nullptr;
  subdir&=65535;
  if (subdir==0) // the side starting at A is side c
  {
    sid=b->edg(this); // which is found by asking point B
  }
  if (subdir==1) // the side starting at B is side a
  {
    sid=c->edg(this); // which is found by asking point C
  }
  if (subdir==2) // the side starting at C is side b
  {
    sid=a->edg(this); // which is found by asking point A
  }
  return sid;
}

int triangle::subdir(edge *edgepart)
{
  int ret=-1;
  if (edgepart==b->edg(this))
    ret=0;
  if (edgepart==c->edg(this))
    ret=1;
  if (edgepart==a->edg(this))
    ret=2;
  return ret;
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

int triangle::proceed(int subdir,double elevation)
{
  int i,j,sign,ret;
  sign=1; // entering triangle: proceed to the other side
  if (subdir&65536)
    sign=-1; // exiting triangle: return -1
  subdir&=65535;
  for (ret=65536,i=0;i<3;i++)
    if (crosses(i,elevation))
      ret+=i;
  if (sign>0)
    ret-=subdir;
  else
    ret=-1;
  return ret;
}

bool triangle::crosses(int subdir,double elevation)
{
  subdir&=65535;
  switch (subdir)
  {
    case 0:
      return (b->elev()<elevation)^(a->elev()<elevation);
      break;
    case 1:
      return (c->elev()<elevation)^(b->elev()<elevation);
      break;
    case 2:
      return (a->elev()<elevation)^(c->elev()<elevation);
      break;
    default:
      return false;
  }
}

bool triangle::upleft(int subdir)
/* Returns true if up is on the left.
 * Used to orient contour lines counterclockwise around peaks.
 */
{
  bool sign;
  sign=(subdir&65536)>0;
  subdir&=65535;
  switch (subdir)
  {
    case 0:
      return (a->elev()>b->elev())^sign;
      break;
    case 1:
      return (b->elev()>c->elev())^sign;
      break;
    case 2:
      return (c->elev()>a->elev())^sign;
      break;
    default:
      return false;
  }
}

xy triangle::contourcept(int subdir,double elevation)
{
  segment seg;
  subdir&=65535;
  switch (subdir)
  {
    case 0:
      seg=b->edg(this)->getsegment();
      break;
    case 1:
      seg=c->edg(this)->getsegment();
      break;
    case 2:
      seg=a->edg(this)->getsegment();
      break;
  }
  if (subdir<3)
    return seg.station(seg.contourcept(elevation));
  else
    return xy(NAN,NAN);
}
