/******************************************************/
/*                                                    */
/* segment.cpp - 3d line segment                      */
/* base class of arc and spiral                       */
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
  control1=control2=0;
}

segment::segment(xyz kra,xyz fam)
{
  start=kra;
  end=fam;
  control1=(2*start.elev()+end.elev())/3;
  control2=(start.elev()+2*end.elev())/3;
  /* This can result in slight parabolicity.
   * start=end=        2.916666666666667
   * control1=control2=2.9166666666666665
   * setslope(which,0) will fix it.
   */
}

segment::segment(xyz kra,double c1,double c2,xyz fam)
{
  start=kra;
  end=fam;
  control1=c1;
  control2=c2;
}

int segment::type()
{
  return OBJ_SEGMENT;
}

bool segment::operator==(const segment b) const
{
  return start==b.start && end==b.end && control1==b.control1 && control2==b.control2;
}

segment segment::operator-() const
{
  return segment(end,control2,control1,start);
}

double segment::length() const
{
  return dist(xy(start),xy(end));
}

void segment::setslope(int which,double s)
{
  switch(which)
  {
    case START:
      control1=start.elev()+s*length()/3;
      break;
    case END:
      control2=end.elev()-s*length()/3;
      break;
  }
}

void segment::setctrl(int which,double el)
{
  switch(which)
  {
    case START:
      control1=el;
      break;
    case END:
      control2=el;
      break;
  }
}

double segment::elev(double along) const
{
  return vcurve(start.elev(),control1,control2,end.elev(),along/length());
}

double segment::slope(double along)
{
  return vslope(start.elev(),control1,control2,end.elev(),along/length())/length();
}

double segment::accel(double along)
{
  return vaccel(start.elev(),control1,control2,end.elev(),along/length())/sqr(length());
}

double segment::startslope()
{
  return (control1-start.elev())*3/length();
}

double segment::endslope()
{
  return (end.elev()-control2)*3/length();
}

double segment::jerk()
{
  return (endslope()-startslope())/length();
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

vector<double> segment::vextrema(bool withends)
{
  double len;
  int i;
  vector<double> ret;
  ret=::vextrema(start.elev(),control1,control2,end.elev());
  for (i=ret.size()-1;i>=0 && !withends;i--)
    if (ret[i]==0 || ret[i]==1 || std::isnan(ret[i]))
      ret.erase(ret.begin()+i);
  if (ret.size())
    len=length();
  for (i=0;i<ret.size();i++)
    ret[i]*=len;
  return ret;
}

xyz segment::midpoint() const
{
  return station(length()/2);
}

bcir segment::boundCircle() const
/* TODO: compute the smallest circle containing the segment. This is correct
 * for a segment, but for a spiralarc it needs to start with the two ends,
 * construct the circle, and keep adding the farthest point from the center
 * until the circle stops changing.
 */
{
  bcir ret;
  ret.center=midpoint();
  ret.radius=length()/2;
  return ret;
}

Circle segment::osculatingCircle(double along) const
{
  return Circle(station(along),bearing(along),curvature(along));
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
  vsplit(start.elev(),control1,control2,end.elev(),along/length(),a.control1,a.control2,dummy,b.control1,b.control2);
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
    oldSlope=endslope();
    start=newEnd;
    setslope(START,newSlope);
    setslope(END,oldSlope);
  }
  if (which==END)
  {
    oldSlope=startslope();
    end=newEnd;
    setslope(END,newSlope);
    setslope(START,oldSlope);
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
    ret=bezier3d(start,sb,startslope(),endslope(),eb,end);
  else
  {
    if (dist(xy(start),xy(end))>length() || std::isnan(control1) || std::isnan(control2))
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

vector<drawingElement> segment::render3d(double precision,int layer,int color,int width,int linetype)
{
  vector<drawingElement> ret;
  ret.push_back(drawingElement(approx3d(precision),color,width,linetype));
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

int segment::tooCurlyRegion(xy pnt)
/* Returns a bit set if xy is in one of six regions relative to start and end
 * defined by which angle is obtuse and its sign. Used in spiralarc::isTooCurly.
 */
{
  int a=chordbearing(),b=dir(end,pnt),c=dir(pnt,start);
  if (dist(end,pnt)<chordlength()/16777216)
    b=endbearing()+DEG180;
  if (dist(pnt,start)<chordlength()/16777216)
    c=startbearing()+DEG180;
  int A=foldangle(c-b+DEG180),B=foldangle(a-c+DEG180),C=foldangle(b-a+DEG180);
  return 1*( A>DEG90 &&  A<DEG180)+
	 2*( B>DEG90 &&  B<DEG180)+
	 4*( C>DEG90 &&  C<DEG180)+
	 8*(-A>DEG90 && -A<DEG180)+
	16*(-B>DEG90 && -B<DEG180)+
	32*(-C>DEG90 && -C<DEG180);
}

bool segment::isCurly()
{
  return false;
}

bool segment::isTooCurly()
{
  return false;
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

bool segment::hasProperty(int prop)
{
  return (prop>=PROP_DELTA && prop<=PROP_END_CURVATURE) ||
         (prop>=PROP_CENTER && prop<=PROP_POINT_INTERSECTION) ||
         prop==PROP_MIDPOINT_XYZ ||
         prop==PROP_MIDPOINT_XY ||
         (prop>=PROP_STARTPOINT && prop<=PROP_ENDPOINT);
}

int segment::getIntProperty(int prop)
{
  int ret=0;
  switch (prop)
  {
    case PROP_DELTA:
      ret=getdelta();
      break;
    case PROP_DELTA2:
      ret=getdelta2();
      break;
  }
  return ret;
}

double segment::getDoubleProperty(int prop)
{
  double ret=NAN;
  switch (prop)
  {
    case PROP_LENGTH:
      ret=length();
      break;
    case PROP_RADIUS:
      ret=radius(length()/2);
      break;
    case PROP_CURVATURE:
      ret=curvature(length()/2);
      break;
    case PROP_DELTA:
      ret=bintorad(getdelta()); // TODO for spiralarcs, there should be a real delta method
      break;
    case PROP_DELTA2:
      ret=bintorad(getdelta2());
      break;
    case PROP_START_TANGENT:
      ret=tangentLength(START);
      break;
    case PROP_END_TANGENT:
      ret=tangentLength(END);
      break;
    case PROP_THROW:
      ret=sthrow();
      break;
  }
  return ret;
}

xy segment::getXyProperty(int prop)
{
  xy ret=nanxy;
  switch (prop)
  {
    case PROP_CENTER:
      ret=center();
      break;
    case PROP_MIDPOINT_XY:
      ret=midpoint();
      break;
  }
  return ret;
}

xyz segment::getXyzProperty(int prop)
{
  xyz ret=nanxyz;
  switch (prop)
  {
    case PROP_STARTPOINT:
      ret=start;
      break;
    case PROP_ENDPOINT:
      ret=end;
      break;
    case PROP_MIDPOINT_XYZ:
      ret=midpoint();
      break;
  }
  return ret;
}

void segment::writeXml(ofstream &ofile)
{
  ofile<<"<segment c1=\""<<ldecimal(control1)<<"\" c2=\""<<ldecimal(control2)<<"\">";
  start.writeXml(ofile);
  end.writeXml(ofile);
  ofile<<"</segment>";
}
