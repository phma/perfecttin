/******************************************************/
/*                                                    */
/* segment.cpp - 3d line segment                      */
/* base class of arc and spiral                       */
/*                                                    */
/******************************************************/
/* Copyright 2020,2021 Pierre Abbat.
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

#include <cmath>
#include <typeinfo>
#include <iostream>
#include <set>
#include <map>
#include <cfloat>
#include "segment.h"
#include "arc.h"
#include "spiral.h"
#include "ldecimal.h"
#include "rootfind.h"
#include "minquad.h"

#define TOLERMULT 33
/* TOLERMULT must have a power between DEG90 and DEG180.
 * Possible values: 3, 7, 14, 20, 33, ...
 * Smaller values make closest slower.
 */

using namespace std;
#ifndef NDEBUG
int closetime; // Holds the time spent in segment::closest.
#endif

segment::segment()
{
  start=end=xyz(0,0,0);
}

segment::segment(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
}

bool segment::operator==(const segment b) const
{
  return start==b.start && end==b.end;
}

segment segment::operator-() const
{
  return segment(end,start);
}

double segment::length() const
{
  return dist(xy(start),xy(end));
}

double segment::elev(double along) const
{
  return start.elev()+(end.elev()-start.elev())*along/length();
}

double segment::slope(double along)
{
  return (end.elev()-start.elev())/length();
}

xyz segment::station(double along) const
{
  double gnola,len;
  len=length();
  gnola=len-along;
  return xyz((start.east()*gnola+end.east()*along)/len,(start.north()*gnola+end.north()*along)/len,
	     elev(along));
}

double segment::contourcept(double e)
/* Finds ret such that elev(ret)=e. Used for tracing a contour from one subedge
 * to the next within a triangle.
 * 
 * This uses Newton's method.
 */
{
  double ret;
  Newton ne;
  ret=ne.init(0,elev(0)-e,slope(0),length(),elev(length())-e,slope(length()));
  while (!ne.finished())
  {
    ret=ne.step(elev(ret)-e,slope(ret));
  }
  if (ret<0)
    ret=NAN;
  if (ret>length())
    ret=NAN;
  return ret;
}

double segment::contourcept_br(double e)
/* Finds ret such that elev(ret)=e. Used for tracing a contour from one subedge
 * to the next within a triangle.
 * 
 * This uses Brent's method.
 * 
 * This needs to be tested when e=0. 3*DBL_EPSILON is apparently too small.
 */
{
  double ret;
  brent br;
  ret=br.init(0,elev(0)-e,length(),elev(length())-e,false);
  while (!br.finished())
  {
    ret=br.step(elev(ret)-e);
  }
  return ret;
}

xyz segment::midpoint() const
{
  return station(length()/2);
}

xy segment::center()
{
  return xy(nan(""),nan(""));
}

xy segment::pointOfIntersection()
/* The PI of a segment is indeterminate. Arbitrarily pick the midpoint,
 * which is the limit of the PI of an arc as the delta approaches zero.
 */
{
  return midpoint();
}

double segment::tangentLength(int which)
{
  return length()/2;
}

void segment::split(double along,segment &a,segment &b)
{
  double dummy;
  xyz splitpoint=station(along);
  a=segment(start,splitpoint);
  b=segment(splitpoint,end);
}

void segment::lengthen(int which,double along)
/* Lengthens or shortens the segment, moving the specified end.
 * Used for extend, trim, trimTwo, and fillet (trimTwo is fillet with radius=0).
 */
{
  double oldSlope,newSlope=slope(along);
  xyz newEnd=station(along);
  if (which==START)
  {
    start=newEnd;
  }
  if (which==END)
  {
    end=newEnd;
  }
}

bezier3d segment::approx3d(double precision)
/* Returns a chain of bezier3d splines which approximate the segment within precision.
 * Of course, for a segment, only one spline is needed and it is exact,
 * but for arcs and spiralarcs, more may be needed. Since startbearing, endbearing,
 * and length are virtual, this doesn't need to be overridden in the derived classes,
 * but it needs to construct two arcs or spiralarcs if it needs to split them.
 */
{
  segment *a,*b;
  int sb,eb,cb;
  double est;
  bezier3d ret;
  sb=startbearing();
  eb=endbearing();
  cb=chordbearing();
  if ((abs(foldangle(sb-cb))<DEG30 && abs(foldangle(eb-cb))<DEG30 && abs(foldangle(sb+eb-2*cb))<DEG30) || length()==0)
    est=bez3destimate(start,sb,length(),eb,end);
  else
    est=fabs(precision*2)+1;
  //cout<<"sb "<<bintodeg(sb)<<" eb "<<bintodeg(eb)<<" est "<<est<<endl;
  if (est<=precision || std::isnan(length()))
    ret=bezier3d(start,sb,slope(),slope(),eb,end);
  else
  {
    if (dist(xy(start),xy(end))>length())
      cerr<<"approx3d: bogus spiralarc"<<endl;
    else
    {
      if (typeid(*this)==typeid(spiralarc))
      {
	a=new spiralarc;
	b=new spiralarc;
	((spiralarc *)this)->split(length()/2,*(spiralarc *)a,*(spiralarc *)b);
      }
      else
      {
	a=new arc;
	b=new arc;
	((arc *)this)->split(length()/2,*(arc *)a,*(arc *)b);
      }
      //cout<<"{"<<endl;
      ret=a->approx3d(precision)+b->approx3d(precision);
      //cout<<"}"<<endl;
      delete a;
      delete b;
    }
  }
  return ret;
}

xy intersection (segment seg1,segment seg2)
/* This might should return a vector of xyz,
 * and will need versions for arc and maybe spiralarc.
 */
{
  return intersection(seg1.start,seg1.end,seg2.start,seg2.end);
}

inttype intersection_type(segment seg1,segment seg2)
{
  return intersection_type(seg1.start,seg1.end,seg2.start,seg2.end);
}

bool sameXyz(segment seg1,segment seg2)
{
  return (seg1.start==seg2.start && seg1.end==seg2.end)
      || (seg1.start==seg2.end && seg1.end==seg2.start);
}

double missDistance(segment seg1,segment seg2)
{
  return missDistance(seg1.start,seg1.end,seg2.start,seg2.end);
}

double segment::closest(xy topoint,double closesofar,bool offends)
/* Finds the closest point on the segment/arc/spiralarc to the point topoint.
 * Does successive parabolic interpolation on the square of the distance.
 * This method finds the exact closest point on a segment in one step;
 * the function takes one more step to verify the solution and maybe another
 * because of roundoff error. On arcs and spiralarcs, takes more steps.
 *
 * If the distance to the closest point is obviously greater than closesofar,
 * it stops early and returns an inaccurate result. This is used when finding
 * the closest point on an alignment consisting of many segments and arcs.
 *
 * This usually takes few iterations (most often 18 in testmanyarc), but
 * occasionally hundreds of thousands of iterations. This usually happens when
 * the distance is small (<100 µm out of 500 m), but at least once it took
 * the longest time on the point that was farthest from the spiralarc.
 */
{
  int nstartpoints,i,angerr,angtoler,endangle;
  double closest,closedist,lastclosedist,fardist,len,len2,vertex;
  map<double,double> stdist;
  set<double> inserenda,delenda;
  set<double>::iterator j;
  map<double,double>::iterator k0,k1,k2;
#ifndef NDEBUG
  closetime=0;
#endif
  closesofar*=closesofar;
  len=length();
  closest=fabs(curvature(0));
  closedist=fabs(curvature(len));
  if (closedist>closest)
    closest=closedist;
  nstartpoints=nearbyint(closest*len)+2;
  closedist=INFINITY;
  fardist=0;
  for (i=0;i<=nstartpoints;i++)
    inserenda.insert(((double)i/nstartpoints)*len);
  do
  {
    lastclosedist=closedist;
    for (j=delenda.begin();j!=delenda.end();++j)
      stdist.erase(*j);
    for (j=inserenda.begin();j!=inserenda.end();++j)
    {
      len2=sqr(dist((xy)station(*j),topoint));
      if (len2<closedist)
      {
	closest=*j;
	closedist=len2;
	angerr=((bearing(*j)-atan2i((xy)station(*j)-topoint))&(DEG180-1))-DEG90;
      }
      if (len2>fardist)
	fardist=len2;
      stdist[*j]=len2;
    }
    inserenda.clear();
    delenda.clear();
    for (k0=k1=stdist.begin(),k2=++k1,++k2;stdist.size()>2 && k2!=stdist.end();++k0,++k1,++k2)
    {
      vertex=minquad(k0->first,k0->second,k1->first,k1->second,k2->first,k2->second);
      if (vertex<0 && vertex>-len/2)
	vertex=-vertex;
      if (vertex>len && vertex<3*len/2)
	vertex=2*len-vertex;
      if (stdist.count(vertex) && vertex!=k1->first || (k1->second-k0->second)*(k2->second-k1->second)>0)
	delenda.insert(k1->first);
      if (!stdist.count(vertex) && vertex>=0 && vertex<=len)
	inserenda.insert(vertex);
#ifndef DEBUG
      closetime++;
#endif
    }
    if (lastclosedist>closedist)
      angtoler=1;
    else
      angtoler*=TOLERMULT;
  } while (abs(angerr)>=angtoler && closedist-(fardist-closedist)/7<closesofar && !((closest==0 && isinsector(dir(topoint,start)-startbearing(),0xf00ff00f)) || (closest==len && isinsector(dir(topoint,end)-endbearing(),0x0ff00ff0))));
  endangle=DEG90;
  if (closest==0)
    endangle=foldangle(dir(topoint,start)-startbearing());
  if (closest==len)
    endangle=foldangle(dir(end,topoint)-endbearing());
  if (!offends && endangle>-DEG90 && endangle<DEG90)
    closest=(closest==0)?-INFINITY:INFINITY;
  return closest;
}

double segment::dirbound(int angle,double boundsofar)
/* angle=0x00000000: returns least easting.
 * angle=0x20000000: returns least northing.
 * angle=0x40000000: returns negative of greatest easting.
 * The algorithm is the same as segment::closest, except that the distance
 * is not squared.
 */
{
  int nstartpoints,i,angerr,angtoler,endangle;
  double bound=HUGE_VAL,turncoord;
  double s=sin(angle),c=cos(angle);
  double closest,closedist,lastclosedist,fardist,len,len2,vertex;
  xy sta;
  map<double,double> stdist;
  set<double> inserenda,delenda;
  set<double>::iterator j;
  map<double,double>::iterator k0,k1,k2;
  len=length();
  closest=curvature(0);
  closedist=curvature(len);
  if (closedist>closest)
    closest=closedist;
  nstartpoints=nearbyint(fabs(closest*len))+2;
  closedist=INFINITY;
  fardist=-INFINITY;
  if (std::isfinite(len))
  {
    for (i=0;i<=nstartpoints;i++)
      inserenda.insert(((double)i/nstartpoints)*len);
    do
    {
      lastclosedist=closedist;
      for (j=delenda.begin();j!=delenda.end();++j)
	stdist.erase(*j);
      for (j=inserenda.begin();j!=inserenda.end();++j)
      {
	sta=station(*j);
	len2=sta.east()*c+sta.north()*s;
	if (len2<closedist)
	{
	  closest=*j;
	  closedist=len2;
	  angerr=((bearing(*j)-angle)&(DEG180-1))-DEG90;
	}
	if (len2>fardist)
	  fardist=len2;
	stdist[*j]=len2;
      }
      inserenda.clear();
      delenda.clear();
      for (k0=k1=stdist.begin(),k2=++k1,++k2;stdist.size()>2 && k2!=stdist.end();++k0,++k1,++k2)
      {
	vertex=minquad(k0->first,k0->second,k1->first,k1->second,k2->first,k2->second);
	if (vertex<0 && vertex>-len/2)
	  vertex=-vertex;
	if (vertex>len && vertex<3*len/2)
	  vertex=2*len-vertex;
	if (stdist.count(vertex) && vertex!=k1->first)
	  delenda.insert(k1->first);
	if (!stdist.count(vertex) && vertex>=0 && vertex<=len)
	  inserenda.insert(vertex);
      }
      if (lastclosedist>closedist)
	angtoler=1;
      else
	angtoler*=TOLERMULT;
    } while (abs(angerr)>=angtoler && closedist-(fardist-closedist)/7<boundsofar && !((closest==0 && isinsector(angle-startbearing(),0xf00ff00f)) || (closest==len && isinsector(angle-endbearing(),0x0ff00ff0))));
  }
  return closedist;
}
