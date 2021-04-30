/******************************************************/
/*                                                    */
/* polyline.cpp - polylines                           */
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

/* A polyline can be open or closed. Smoothing a polyspiral does this:
 * Each point, except the ends of an open polyspiral, is assigned the bearing
 * at that point of the arc that passes through its predecessor, it, and
 * its successor. Each segment of the polyspiral is redrawn to be tangent
 * to the bearings at its ends, unless it would exceed bendlimit, in which case
 * it is redrawn as a straight line.
 */

#include <cassert>
#include <iostream>
#include "polyline.h"
#include "manysum.h"
#include "binio.h"
#include "relprime.h"
#include "ldecimal.h"
using namespace std;
int bendlimit=DEG180;

int midarcdir(xy a,xy b,xy c)
/* Returns the bearing of the arc abc at point b. May be off by 360°;
 * make sure consecutive bearings do not differ by more than 180°.
 */
{
  return dir(a,b)+dir(b,c)-dir(a,c);
}

int count1(unsigned int num)
{
  num=(num&0x55555555)+((num&0xaaaaaaaa)>>1);
  num=(num&0x33333333)+((num&0xcccccccc)>>2);
  num=(num&0x0f0f0f0f)+((num&0xf0f0f0f0)>>4);
  num=(num&0x00ff00ff)+((num&0xff00ff00)>>8);
  num=(num&0x0000ffff)+((num&0xffff0000)>>16);
  return num;
}

int signParity(unsigned int num)
{
  return 1-2*(count1(num)&1);
}

polyline::polyline()
{
  elevation=0;
}

polyarc::polyarc(): polyline::polyline()
{
}

polyspiral::polyspiral(): polyarc::polyarc()
{
  curvy=true;
}

polyline::polyline(double e)
{
  elevation=e;
}

polyarc::polyarc(double e): polyline::polyline(e)
{
}

polyspiral::polyspiral(double e): polyarc::polyarc(e)
{
  curvy=true;
}

polyarc::polyarc(polyline &p)
{
  elevation=p.elevation;
  endpoints=p.endpoints;
  lengths=p.lengths;
  cumLengths=p.cumLengths;
  deltas.resize(lengths.size());
}

polyspiral::polyspiral(polyline &p)
{
  int i,j;
  elevation=p.elevation;
  endpoints=p.endpoints;
  lengths=p.lengths;
  cumLengths=p.cumLengths;
  deltas.resize(lengths.size());
  delta2s.resize(lengths.size());
  bearings.resize(endpoints.size());
  midbearings.resize(lengths.size());
  midpoints.resize(lengths.size());
  clothances.resize(lengths.size());
  curvatures.resize(lengths.size());
  for (i=0;i<lengths.size();i++)
  {
    j=(i+1==endpoints.size())?0:i+1;
    midpoints[i]=(endpoints[i]+endpoints[j])/2;
    midbearings[i]=dir(endpoints[i],endpoints[j]);
    if (i)
      midbearings[i]=midbearings[i-1]+foldangle(midbearings[i]-midbearings[i-1]);
    bearings[i]=midbearings[i];
  }
  curvy=false;
}

void polyline::clear()
{
  endpoints.clear();
  lengths.clear();
  cumLengths.clear();
}

void polyarc::clear()
{
  endpoints.clear();
  lengths.clear();
  cumLengths.clear();
  deltas.clear();
}

void polyspiral::clear()
{
  endpoints.clear();
  lengths.clear();
  cumLengths.clear();
  deltas.clear();
  delta2s.clear();
  bearings.clear();
  midbearings.clear();
  midpoints.clear();
  clothances.clear();
  curvatures.clear();
}

bool polyline::isopen()
{
  return endpoints.size()>lengths.size();
}

int polyline::size()
{
  return lengths.size();
}

segment polyline::getsegment(int i)
{
  i%=(signed)lengths.size();
  if (i<0)
    i+=lengths.size();
  return segment(xyz(endpoints[i],elevation),xyz(endpoints[(i+1)%endpoints.size()],elevation));
}

arc polyarc::getarc(int i)
{
  i%=(signed)deltas.size();
  if (i<0)
    i+=deltas.size();
  return arc(xyz(endpoints[i],elevation),xyz(endpoints[(i+1)%endpoints.size()],elevation),deltas[i]);
}

spiralarc polyspiral::getspiralarc(int i)
{
  i%=(signed)deltas.size();
  if (i<0)
    i+=deltas.size();
  return spiralarc(xyz(endpoints[i],elevation),xyz(midpoints[i],elevation),
		   xyz(endpoints[(i+1)%endpoints.size()],elevation),midbearings[i],
		   curvatures[i],clothances[i],lengths[i]);
}

xyz polyline::getEndpoint(int i)
{
  i%=(signed)endpoints.size();
  if (i<0)
    i+=endpoints.size();
  return xyz(endpoints[i],elevation);
}

xyz polyline::getstart()
{
  return getEndpoint(0);
}

xyz polyline::getend()
// If it's closed, this is equivalent to getstart.
{
  return getEndpoint(lengths.size());
}

bezier3d polyline::approx3d(double precision)
{
  bezier3d ret;
  int i;
  for (i=0;i<size();i++)
    ret+=getsegment(i).approx3d(precision);
  if (!isopen())
    ret.close();
  return ret;
}

bezier3d polyarc::approx3d(double precision)
{
  bezier3d ret;
  int i;
  for (i=0;i<size();i++)
    ret+=getarc(i).approx3d(precision);
  if (!isopen())
    ret.close();
  return ret;
}

bezier3d polyspiral::approx3d(double precision)
{
  bezier3d ret;
  int i;
  for (i=0;i<size();i++)
    ret+=getspiralarc(i).approx3d(precision);
  if (!isopen())
    ret.close();
  return ret;
}

void polyline::dedup()
/* Collapses into one adjacent points that are too close together.
 * They result from running contourcept on two segments that are the same
 * in opposite directions.
 */
{
  int h,i,j,k;
  vector<xy>::iterator ptit;
  vector<double>::iterator lenit;
  xy avg;
  //if (dist(endpoints[0],xy(999992.534,1499993.823))<0.001)
  //  cout<<"Debug contour\r";
  for (i=0;i<endpoints.size() && endpoints.size()>2;i++)
  {
    h=i-1;
    if (h<0)
      if (isopen())
	h=0;
      else
	h+=endpoints.size();
    j=i+1;
    if (j>=endpoints.size())
      if (isopen())
	j--;
      else
	j-=endpoints.size();
    k=j+1;
    if (k>=endpoints.size())
      if (isopen())
	k--;
      else
	k-=endpoints.size();
    if (i!=j && (dist(endpoints[i],endpoints[j])*16777216<=dist(endpoints[h],endpoints[i]) || dist(endpoints[i],endpoints[j])*16777216<=dist(endpoints[j],endpoints[k]) || dist(endpoints[i],endpoints[j])*281474976710656.<=dist(endpoints[i],-endpoints[j]) || endpoints[j].isnan()))
    {
      avg=(endpoints[i]+endpoints[j])/2;
      ptit=endpoints.begin()+i;
      lenit=lengths.begin()+i;
      endpoints.erase(ptit);
      lengths.erase(lenit);
      lenit=cumLengths.begin()+i;
      cumLengths.erase(lenit);
      if (h>i)
	h--;
      if (k>i)
	k--;
      if (j>i)
        j=i;
      else
        i=j; // deleted the last element, what [i] was is no longer valid
      if (avg.isfinite())
	endpoints[i]=avg;
      if (i!=h)
	lengths[h]=dist(endpoints[h],endpoints[i]);
      if (j!=k)
	lengths[j]=dist(endpoints[k],endpoints[j]);
      i--; // in case three in a row are the same
    }
  }
}

void polyline::insert(xy newpoint,int pos)
/* Inserts newpoint in position pos. E.g. insert(xy(8,5),2) does
 * {(0,0),(1,1),(2,2),(3,3)} -> {(0,0),(1,1),(8,5),(2,2),(3,3)}.
 * If the polyline is open, inserting a point in position 0, -1, or after the last
 * (-1 means after the last) results in adding a line segment.
 * If the polyline is empty (and therefore closed), inserting a point results in
 * adding a line segment from that point to itself.
 * In all other cases, newpoint is inserted between two points and connected to
 * them with line segments.
 */
{
  bool wasopen;
  int i;
  vector<xy>::iterator ptit;
  vector<double>::iterator lenit;
  if (newpoint.isnan())
    cerr<<"Inserting NaN"<<endl;
  wasopen=isopen();
  if (pos<0 || pos>endpoints.size())
    pos=endpoints.size();
  ptit=endpoints.begin()+pos;
  lenit=lengths.begin()+pos;
  endpoints.insert(ptit,newpoint);
  lengths.insert(lenit,0);
  lenit=cumLengths.begin()+pos;
  if (pos<cumLengths.size())
    cumLengths.insert(lenit,cumLengths[pos]);
  else
    cumLengths.insert(lenit,0);
  pos--;
  if (pos<0)
    pos=0;
  for (i=0;i<2;i++)
  {
    if (pos+1<endpoints.size())
      lengths[pos]=dist(endpoints[pos],endpoints[pos+1]);
    if (pos+1==endpoints.size() && !wasopen)
      lengths[pos]=dist(endpoints[pos],endpoints[0]);
    pos++;
    if (pos>=lengths.size())
      pos=0;
  }
}

void polyarc::insert(xy newpoint,int pos)
/* Same as polyline::insert for beginning, end, and empty cases.
 * In all other cases, newpoint is inserted into an arc, whose delta is split
 * proportionally to the distances to the adjacent points.
 */
{
  bool wasopen;
  double totdist=0,totdelta=0;
  int i,savepos,newdelta[2];
  vector<xy>::iterator ptit;
  vector<int>::iterator arcit;
  vector<double>::iterator lenit;
  wasopen=isopen();
  if (pos<0 || pos>endpoints.size())
    pos=endpoints.size();
  ptit=endpoints.begin()+pos;
  lenit=lengths.begin()+pos;
  arcit=deltas.begin()+pos;
  endpoints.insert(ptit,newpoint);
  deltas.insert(arcit,0);
  lengths.insert(lenit,0);
  lenit=cumLengths.begin()+pos;
  if (pos<cumLengths.size())
    cumLengths.insert(lenit,cumLengths[pos]);
  else
    cumLengths.insert(lenit,0);
  pos--;
  if (pos<0)
    pos=0;
  savepos=pos;
  for (i=0;i<2;i++)
  {
    if (pos+1<endpoints.size())
    {
      lengths[pos]=dist(endpoints[pos],endpoints[pos+1]);
      totdist+=lengths[pos];
      totdelta+=deltas[pos];
    }
    if (pos+1==endpoints.size() && !wasopen)
    {
      lengths[pos]=dist(endpoints[pos],endpoints[0]);
      totdist+=lengths[pos];
      totdelta+=deltas[pos];
    }
    pos++;
    if (pos>=lengths.size())
      pos=0;
  }
  pos=savepos;
  if (totdist)
  {
    newdelta[0]=rint(lengths[pos]*totdelta/totdist);
    newdelta[1]=totdelta-newdelta[0];
  }
  else
  {
    newdelta[0]=totdelta;
    newdelta[1]=0;
  }
  for (i=0;i<2;i++)
  {
    if (pos+1<endpoints.size() || !wasopen)
    {
      deltas[pos]=newdelta[i];
      lengths[pos]=getarc(i).length();
    }
    pos++;
    if (pos>=lengths.size())
      pos=0;
  }
}

void polyspiral::insert(xy newpoint,int pos)
/* If there is one point after insertion and the polyspiral is closed:
 * Adds a line from the point to itself.
 * If there are two points after insertion and the polyspiral is open:
 * Adds a line from one point to the other.
 * If it's closed:
 * Adds two 180° arcs, making a circle.
 * If there are at least three points after insertion:
 * Updates the bearings of the new point and two adjacent points (unless
 * it is the first or last of an open polyspiral, in which case one) to
 * match an arc passing through three points, then updates the clothances
 * and curvatures of four consecutive spirals (unless the polyspiral is open
 * and the point is one of the first two or last two) to match the bearings.
 */
{
  bool wasopen;
  int i,savepos,newBearing=0;
  vector<xy>::iterator ptit,midit;
  vector<int>::iterator arcit,brgit,d2it,mbrit;
  vector<double>::iterator lenit,cloit,crvit;
  wasopen=isopen();
  if (pos<0 || pos>endpoints.size())
    pos=endpoints.size();
  if (bearings.size())
    if (pos<=bearings.size()/2)
      newBearing=bearings[pos];
    else
      newBearing=bearings[pos-1];
  ptit=endpoints.begin()+pos;
  brgit=bearings.begin()+pos;
  savepos=pos;
  pos--;
  if (pos<0)
    pos=0;
  midit=midpoints.begin()+pos;
  lenit=lengths.begin()+pos;
  arcit=deltas.begin()+pos;
  d2it=delta2s.begin()+pos;
  mbrit=midbearings.begin()+pos;
  cloit=clothances.begin()+pos;
  crvit=curvatures.begin()+pos;
  endpoints.insert(ptit,newpoint);
  deltas.insert(arcit,0);
  lengths.insert(lenit,1);
  midpoints.insert(midit,newpoint);
  bearings.insert(brgit,newBearing);
  delta2s.insert(d2it,0);
  midbearings.insert(mbrit,0);
  curvatures.insert(crvit,0);
  clothances.insert(cloit,0);
  lenit=cumLengths.begin()+pos;
  cumLengths.insert(lenit,0);
  pos=savepos;
  for (i=-1;i<2;i++)
    setbear((pos+i+endpoints.size())%endpoints.size());
  for (i=-1;i<3;i++)
    setspiral((pos+i+lengths.size())%lengths.size());
}

void polyline::replace(xy newpoint,int pos)
{
  pos%=(signed)endpoints.size();
  if (pos<0)
    pos+=endpoints.size();
  endpoints[pos]=newpoint;
  if (pos>0)
    lengths[pos-1]=dist(endpoints[pos-1],endpoints[pos]);
  else if (!isopen())
    lengths.back()=dist(endpoints.back(),endpoints[0]);
  if (pos<endpoints.size()-1)
    lengths[pos]=dist(endpoints[pos],endpoints[pos+1]);
  else if (!isopen())
    lengths[pos]=dist(endpoints[pos],endpoints[0]);
}

void polyarc::replace(xy newpoint,int pos)
{
  pos%=(signed)endpoints.size();
  if (pos<0)
    pos+=endpoints.size();
  endpoints[pos]=newpoint;
  if (pos>0)
    lengths[pos-1]=getarc(pos-1).length();
  else if (!isopen())
    lengths.back()=getarc(-1).length();
  lengths[pos]=getarc(pos).length();
}

void polyspiral::replace(xy newpoint,int pos)
{
  int i;
  pos%=(signed)endpoints.size();
  if (pos<0)
    pos+=endpoints.size();
  endpoints[pos]=newpoint;
  for (i=-1;i<2;i++)
    setbear((pos+i+endpoints.size())%endpoints.size());
  for (i=-1;i<3;i++)
    setspiral((pos+i+lengths.size())%lengths.size());
}

void polyline::erase(int pos)
{
  int i;
  vector<xy>::iterator ptit;
  vector<double>::iterator lenit,cumit;
  if (pos<0)
    pos=0;
  if (pos>=endpoints.size())
    pos=endpoints.size()-1;
  ptit=endpoints.begin()+pos;
  if (pos<lengths.size())
  {
    lenit=lengths.begin()+pos;
    cumit=cumLengths.begin()+pos;
  }
  else
  {
    lenit=lengths.begin()+pos-1;
    cumit=cumLengths.begin()+pos-1;
  }
  if (endpoints.size())
    endpoints.erase(ptit);
  if (lengths.size())
  {
    lengths.erase(lenit);
    cumLengths.erase(cumit);
  }
  i=pos-1; // If deleted point 1, length 0 is wrong.
  if (i<0)
    i=lengths.size()-1;
  if (i>=0)
    lengths[i]=getsegment(i).length();
}

void polyarc::erase(int pos)
{
  int i;
  vector<xy>::iterator ptit;
  vector<int>::iterator arcit;
  vector<double>::iterator lenit,cumit;
  if (pos<0)
    pos=0;
  if (pos>=endpoints.size())
    pos=endpoints.size()-1;
  ptit=endpoints.begin()+pos;
  if (pos<lengths.size())
  {
    lenit=lengths.begin()+pos;
    cumit=cumLengths.begin()+pos;
    arcit=deltas.begin()+pos;
  }
  else
  {
    lenit=lengths.begin()+pos-1;
    cumit=cumLengths.begin()+pos-1;
    arcit=deltas.begin()+pos-1;
  }
  if (endpoints.size())
    endpoints.erase(ptit);
  if (lengths.size())
  {
    lengths.erase(lenit);
    cumLengths.erase(cumit);
    deltas.erase(arcit);
  }
  i=pos-1; // If deleted point 1, length 0 is wrong.
  if (i<0)
    i=lengths.size()-1;
  if (i>=0)
    lengths[i]=getarc(i).length();
}

void polyspiral::erase(int pos)
{
  int i;
  vector<xy>::iterator ptit,midit;
  vector<int>::iterator arcit,brgit,d2it,mbrit;
  vector<double>::iterator lenit,cumit,cloit,crvit;
  spiralarc spi;
  if (pos<0)
    pos=0;
  if (pos>=endpoints.size())
    pos=endpoints.size()-1;
  ptit=endpoints.begin()+pos;
  brgit=bearings.begin()+pos;
  if (pos<lengths.size())
  {
    lenit=lengths.begin()+pos;
    cumit=cumLengths.begin()+pos;
    arcit=deltas.begin()+pos;
    d2it=delta2s.begin()+pos;
    midit=midpoints.begin()+pos;
    mbrit=midbearings.begin()+pos;
    cloit=clothances.begin()+pos;
    crvit=curvatures.begin()+pos;
  }
  else
  {
    lenit=lengths.begin()+pos-1;
    cumit=cumLengths.begin()+pos-1;
    arcit=deltas.begin()+pos-1;
    d2it=delta2s.begin()+pos-1;
    midit=midpoints.begin()+pos-1;
    mbrit=midbearings.begin()+pos-1;
    cloit=clothances.begin()+pos-1;
    crvit=curvatures.begin()+pos-1;
  }
  if (endpoints.size())
  {
    endpoints.erase(ptit);
    bearings.erase(brgit);
  }
  if (lengths.size())
  {
    lengths.erase(lenit);
    cumLengths.erase(cumit);
    deltas.erase(arcit);
    delta2s.erase(d2it);
    midpoints.erase(midit);
    midbearings.erase(mbrit);
    clothances.erase(cloit);
    curvatures.erase(crvit);
  }
  i=pos-1; // If deleted point 1, length 0 is wrong.
  if (i<0)
    i=lengths.size()-1;
  if (i>=0)
  {
    for (i=-1;i<2;i++)
      setbear((pos+i+endpoints.size())%endpoints.size());
    for (i=-1;i<3;i++)
      setspiral((pos+i+lengths.size())%lengths.size());
  }
}

/* After inserting, erasing, opening, or closing, call setlengths before calling
 * length or station. If insert called setlengths, manipulation would take
 * too long. So do a lot of inserts, then call setlengths.
 */
void polyline::setlengths()
{
  int i;
  manysum m;
  segment seg;
  assert(lengths.size()==cumLengths.size());
  for (i=0;i<lengths.size();i++)
  {
    seg=getsegment(i);
    lengths[i]=seg.length();
    m+=lengths[i];
    cumLengths[i]=m.total();
  }
}

void polyarc::setlengths()
{
  int i;
  manysum m;
  arc seg;
  assert(lengths.size()==cumLengths.size());
  assert(lengths.size()==deltas.size());
  for (i=0;i<deltas.size();i++)
  {
    seg=getarc(i);
    lengths[i]=seg.length();
    m+=lengths[i];
    cumLengths[i]=m.total();
  }
}

void polyspiral::setlengths()
{
  int i;
  manysum m;
  spiralarc seg;
  assert(lengths.size()==cumLengths.size());
  assert(lengths.size()==deltas.size());
  for (i=0;i<deltas.size();i++)
  {
    seg=getspiralarc(i);
    lengths[i]=seg.length();
    m+=lengths[i];
    cumLengths[i]=m.total();
  }
}

void polyarc::setdelta(int i,int delta)
{
  i%=deltas.size();
  if (i<0)
    i+=deltas.size();
  deltas[i]=delta;
  lengths[i]=getarc(i).length();
}

double polyline::in(xy point)
/* Returns 1 if the polyline winds once counterclockwise around point.
 * Returns 1/2 or -1/2 if point is on polyline's boundary, unless it
 * is a corner, in which case it returns another fraction.
 */
{
  double ret=0,subtarea;
  int i,subtended,sz=endpoints.size();
  for (i=0;i<lengths.size();i++)
  {
    if (point!=endpoints[i] && point!=endpoints[(i+1)%sz])
    {
      subtended=foldangle(dir(point,endpoints[(i+1)%sz])-dir(point,endpoints[i]));
      if (subtended==-DEG180)
      {
        subtarea=area3(endpoints[(i+1)%sz],point,endpoints[i]);
        if (subtarea>0)
          subtended=DEG180;
        if (subtarea==0)
          subtended=0;
      }
      ret+=bintorot(subtended);
    }
  }
  return ret;
}

double polyarc::in(xy point)
{
  double ret=polyline::in(point);
  int i;
  for (i=0;i<lengths.size();i++)
    ret+=getarc(i).in(point);
  return ret;
}

double polyspiral::in(xy point)
{
  double ret=polyline::in(point);
  int i;
  for (i=0;i<lengths.size();i++)
    ret+=getspiralarc(i).in(point);
  return ret;
}

double polyline::length()
{
  if (cumLengths.size())
    return cumLengths.back();
  else
    return 0;
}

double polyline::getCumLength(int i)
// Returns the total of the first i segments/arcs/spiralarcs.
{
  i--;
  if (i>=(signed)cumLengths.size())
    i=cumLengths.size()-1;
  if (i<0)
    return 0;
  else
    return cumLengths[i];
}

int polyline::stationSegment(double along)
{
  int before=-1,after=cumLengths.size();
  int middle,i=0;
  double midalong;
  while (before<after-1)
  {
    middle=(before+after+(i&1))/2;
    if (middle>=cumLengths.size())
      midalong=length();
    else if (middle<0)
      midalong=0;
    else
      midalong=cumLengths[middle];
    if (midalong>along)
      after=middle;
    else
      before=middle;
    ++i;
  }
  if (after==cumLengths.size() && after && along==cumLengths.back())
    after--; // station(length()) should return the endpoint, not NaN
  return after;
}

xyz polyline::station(double along)
{
  int seg=stationSegment(along);
  if (seg<0 || seg>=lengths.size())
    return xyz(NAN,NAN,NAN);
  else
    return getsegment(seg).station(along-(cumLengths[seg]-lengths[seg]));
}

xyz polyarc::station(double along)
{
  int seg=stationSegment(along);
  if (seg<0 || seg>=lengths.size())
    return xyz(NAN,NAN,NAN);
  else
    return getarc(seg).station(along-(cumLengths[seg]-lengths[seg]));
}

xyz polyspiral::station(double along)
{
  int seg=stationSegment(along);
  if (seg<0 || seg>=lengths.size())
    return xyz(NAN,NAN,NAN);
  else
    return getspiralarc(seg).station(along-(cumLengths[seg]-lengths[seg]));
}

double polyline::area()
{
  int i;
  xy startpnt;
  manysum a;
  if (endpoints.size())
    startpnt=endpoints[0];
  if (isopen())
    a+=NAN;
  else
    for (i=0;i<lengths.size();i++)
    {
      a+=area3(startpnt,endpoints[i],endpoints[(i+1)%endpoints.size()]);
    }
  return a.total();
}

double polyarc::area()
{
  int i;
  xy startpnt;
  manysum a;
  if (endpoints.size())
    startpnt=endpoints[0];
  if (isopen())
    a+=NAN;
  else
    for (i=0;i<lengths.size();i++)
    {
      a+=area3(startpnt,endpoints[i],endpoints[(i+1)%endpoints.size()]);
      a+=getarc(i).diffarea();
    }
  return a.total();
}

double polyspiral::area()
{
  int i;
  xy startpnt;
  manysum a;
  if (endpoints.size())
    startpnt=endpoints[0];
  if (isopen())
    a+=NAN;
  else
    for (i=0;i<lengths.size();i++)
    {
      a+=area3(startpnt,endpoints[i],endpoints[(i+1)%endpoints.size()]);
      a+=getspiralarc(i).diffarea();
    }
  return a.total();
}

void polyline::open()
{
  lengths.resize(endpoints.size()-1);
  cumLengths.resize(endpoints.size()-1);
}

void polyarc::open()
{
  deltas.resize(endpoints.size()-1);
  lengths.resize(endpoints.size()-1);
  cumLengths.resize(endpoints.size()-1);
}

void polyspiral::open()
{
  curvatures.resize(endpoints.size()-1);
  clothances.resize(endpoints.size()-1);
  midpoints.resize(endpoints.size()-1);
  midbearings.resize(endpoints.size()-1);
  delta2s.resize(endpoints.size()-1);
  deltas.resize(endpoints.size()-1);
  lengths.resize(endpoints.size()-1);
  cumLengths.resize(endpoints.size()-1);
}

void polyline::close()
{
  lengths.resize(endpoints.size());
  cumLengths.resize(endpoints.size());
  if (lengths.size())
    if (lengths.size()>1)
      cumLengths[lengths.size()-1]=cumLengths[lengths.size()-2]+lengths[lengths.size()-1];
    else
      cumLengths[0]=lengths[0];
}

void polyarc::close()
{
  deltas.resize(endpoints.size());
  lengths.resize(endpoints.size());
  cumLengths.resize(endpoints.size());
  if (lengths.size())
    if (lengths.size()>1)
      cumLengths[lengths.size()-1]=cumLengths[lengths.size()-2]+lengths[lengths.size()-1];
    else
      cumLengths[0]=lengths[0];
}

void polyspiral::close()
{
  curvatures.resize(endpoints.size());
  clothances.resize(endpoints.size());
  midpoints.resize(endpoints.size());
  midbearings.resize(endpoints.size());
  delta2s.resize(endpoints.size());
  deltas.resize(endpoints.size());
  lengths.resize(endpoints.size());
  cumLengths.resize(endpoints.size());
  if (lengths.size())
    if (lengths.size()>1)
      cumLengths[lengths.size()-1]=cumLengths[lengths.size()-2]+lengths[lengths.size()-1];
    else
      cumLengths[0]=lengths[0];
}

void polyline::_roscat(xy tfrom,int ro,double sca,xy cis,xy tto)
{
  int i;
  for (i=0;i<endpoints.size();i++)
    endpoints[i]._roscat(tfrom,ro,sca,cis,tto);
  for (i=0;i<lengths.size();i++)
    lengths[i]*=sca;
}

void polyspiral::_roscat(xy tfrom,int ro,double sca,xy cis,xy tto)
{
  int i;
  for (i=0;i<endpoints.size();i++)
  {
    endpoints[i]._roscat(tfrom,ro,sca,cis,tto);
    bearings[i]+=ro;
  }
  for (i=0;i<lengths.size();i++)
  {
    lengths[i]*=sca;
    cumLengths[i]*=sca;
    midbearings[i]+=ro;
    curvatures[i]/=sca;
    clothances[i]/=sqr(sca);
  }
}

void polyspiral::setbear(int i)
{
  int h,j,prevbear,nextbear,avgbear;
  i%=endpoints.size();
  if (i<0)
    i+=endpoints.size();
  h=i-1;
  j=i+1;
  if (h<0)
    if (isopen())
      h+=3;
    else
      h+=endpoints.size();
  if (j>=endpoints.size())
    if (isopen())
      j-=3;
    else
      j-=endpoints.size();
  if (endpoints.size()==2)
    if (isopen())
      bearings[i]=dir(endpoints[0],endpoints[1]);
    else
      bearings[i]=dir(endpoints[i],endpoints[j])-DEG90;
  if (endpoints.size()>2)
  {
    bearings[i]=midarcdir(endpoints[h],endpoints[i],endpoints[j]);
    prevbear=bearings[h];
    nextbear=bearings[j];
    if (i==0)
      if (isopen())
	prevbear=nextbear;
      else
	prevbear+=DEG360;
    if (i==endpoints.size()-1)
      if (isopen())
	nextbear=prevbear;
      else
	nextbear+=DEG360;
    avgbear=prevbear+(nextbear-prevbear)/2;
    bearings[i]=avgbear+foldangle(bearings[i]-avgbear);
  }
}

void polyspiral::setbear(int i,int bear)
{
  i%=endpoints.size();
  if (i<0)
    i+=endpoints.size();
  bearings[i]=bear;
}

void polyspiral::setspiral(int i)
{
  int j,d1,d2;
  spiralarc s;
  j=i+1;
  if (j>=endpoints.size())
    j=0;
  s=spiralarc(xyz(endpoints[i],elevation),xyz(endpoints[j],elevation));
  d1=bearings[j]-bearings[i]+DEG360*(j<i);
  d2=bearings[j]+bearings[i]+DEG360*(j<i)-2*dir(endpoints[i],endpoints[j]);
  if (!curvy || abs(d1)>=bendlimit || abs(d2)>=bendlimit || abs(d1)+abs(d2)>=bendlimit)
    d1=d2=0;
  s.setdelta(d1,d2);
  if (std::isnan(s.length()))
    s.setdelta(0,0);
  if (lengths[i]==0)
    cerr<<"length["<<i<<"]=0"<<endl;
  deltas[i]=s.getdelta();
  delta2s[i]=s.getdelta2();
  lengths[i]=s.length();
  //if (std::isnan(lengths[i]))
    //cerr<<"length["<<i<<"]=nan"<<endl;
  midbearings[i]=s.bearing(lengths[i]/2);
  midpoints[i]=s.station(lengths[i]/2);
  curvatures[i]=s.curvature(lengths[i]/2);
  clothances[i]=s.clothance();
}

void polyspiral::smooth()
{
  int i;
  curvy=true;
  for (i=0;i<endpoints.size();i++)
    setbear(i);
  for (i=0;i<lengths.size();i++)
    setspiral(i);
}

void crspace(ofstream &ofile,int i)
{
  if (i%10)
    ofile<<' ';
  else if (i)
    ofile<<endl;
}

unsigned int polyline::checksum()
/* This computes bearings, so the number may differ slightly between platforms,
 * such as Linux and BSD, because of roundoff error or M_PIl. See angle.h.
 */
{
  unsigned int ret=0;
  int i,esz=endpoints.size(),lsz=lengths.size();
  for (i=0;i<esz;i++)
    ret+=signParity(i)*dir(xy(0,0),endpoints[i]);
  for (i=0;i<lsz;i++)
    ret+=signParity(i)*dir(endpoints[i],endpoints[(i+1)%esz]);
  return ret;
}

unsigned int polyarc::checksum()
{
  unsigned int ret=0;
  int i,esz=endpoints.size(),lsz=lengths.size();
  for (i=0;i<esz;i++)
    ret+=signParity(i)*dir(xy(0,0),endpoints[i]);
  for (i=0;i<lsz;i++)
  {
    ret+=signParity(i)*dir(endpoints[i],endpoints[(i+1)%esz]);
    ret+=signParity(i)*deltas[i];
  }
  return ret;
}

unsigned int polyspiral::checksum()
{
  unsigned int ret=0;
  int i,esz=endpoints.size(),lsz=lengths.size();
  for (i=0;i<esz;i++)
  {
    ret+=signParity(i)*dir(xy(0,0),endpoints[i]);
  }
  for (i=0;i<lsz;i++)
  {
    ret-=signParity(i)*dir(endpoints[i],endpoints[(i+1)%esz]);
    ret+=signParity(i)*(deltas[i]+delta2s[i]+2*midbearings[i]);
    /* The midbearings of a polyspiral copied from a polyline are the same as
     * the dir from one endpoint to the next, so 2*midbearings[i]-dir(
     * endpoints[i],endpoints[i+1] is equal to dir(endpoints[i],endpoints[i+1].
     */
  }
  return ret;
}

void polyline::write(ostream &file)
{
  int i;
  writeledouble(file,elevation);
  writeleint(file,endpoints.size());
  for (i=0;i<endpoints.size();i++)
  {
    writeledouble(file,endpoints[i].getx());
    writeledouble(file,endpoints[i].gety());
  }
  writeleint(file,lengths.size());
  for (i=0;i<lengths.size();i++)
    writeledouble(file,lengths[i]);
  writeleint(file,0); // deltas
  writeleint(file,0); // delta2s
}
