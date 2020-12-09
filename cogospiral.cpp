/******************************************************/
/*                                                    */
/* cogospiral.cpp - intersections of spirals          */
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
#include <cfloat>
#include <iostream>
#include "ldecimal.h"
#include "cogospiral.h"
#include "manysum.h"
#include "relprime.h"
#include "matrix.h"
#include "pointlist.h"
#include "rootfind.h"

using namespace std;

alosta::alosta()
{
  along=0;
  station=xy(0,0);
  bearing=0;
  curvature=0;
}

alosta::alosta(double a,xy s)
{
  along=a;
  station=s;
  bearing=0;
  curvature=0;
}

alosta::alosta(double a,xy s,int b,double c)
{
  along=a;
  station=s;
  bearing=b;
  curvature=c;
}

void alosta::setStation(segment *seg,double alo)
{
  along=alo;
  station=seg->station(alo);
  bearing=seg->bearing(alo);
  curvature=seg->curvature(alo);
}

bool sortpts(alosta a[],alosta b[])
// Returns true if any swaps took place.
{
  bool ret=false;
  int i,j,apos,bpos;
  double dst,closest;
  closest=INFINITY;
  for (i=0;i<3;i++)
    for (j=0;j<3;j++)
    {
      dst=dist(a[i].station,b[j].station);
      if (dst<closest)
      {
	closest=dst;
	apos=i;
	bpos=j;
      }
    }
  if (apos>0)
  {
    ret=true;
    swap(a[0],a[apos]);
  }
  if (bpos>0)
  {
    ret=true;
    swap(b[0],b[bpos]);
  }
  closest=INFINITY;
  for (i=1;i<3;i++)
    for (j=1;j<3;j++)
    {
      dst=dist(a[i].station,b[j].station);
      if (dst<closest)
      {
	closest=dst;
	apos=i;
	bpos=j;
      }
    }
  if (apos>1)
  {
    ret=true;
    swap(a[1],a[apos]);
  }
  if (bpos>1)
  {
    ret=true;
    swap(b[1],b[bpos]);
  }
  if (a[0].station==a[1].station)
  {
    ret=true;
    swap(a[1],a[2]); // if a[0]==a[1], a divide by zero results in the next iteration
  }
  if (b[0].station==b[1].station)
  {
    ret=true;
    swap(b[1],b[2]);
  }
  return ret;
}

bool sortpts2(alosta a[],alosta b[])
// Returns true if any swaps took place.
{
  bool ret=false;
  int i,j,apos,bpos;
  double dst,closest;
  closest=INFINITY;
  for (i=0;i<2;i++)
    for (j=0;j<2;j++)
    {
      dst=dist(a[i].station,b[j].station);
      if (dst<closest)
      {
	closest=dst;
	apos=i;
	bpos=j;
      }
    }
  if (apos>0)
  {
    ret=true;
    swap(a[0],a[apos]);
  }
  if (bpos>0)
  {
    ret=true;
    swap(b[0],b[bpos]);
  }
  return ret;
}

vector<alosta> intersection1(segment *a,double a1,double a2,segment *b,double b1,double b2,bool extend)
/* Returns two alostas, one for a and one for b, or nothing.
 * If extend is true, the segments/arcs/spiralarcs are extended to twice their length;
 * e.g. one of length 5 extends from station -2.5 to station 7.5.
 * 
 * It can exit in three ways:
 * • The point in aalosta and the point in balosta which are closest are close
 *   enough to be the same point. They are returned.
 * • The new points are farther from each other and the previous points than
 *   previous points are from each other. Returns an empty vector.
 * • The new point is out of range of either or both of the curves.
 *   Returns an empty vector.
 */
{
  bool isnewcloser;
  xy insect;
  double di0,di1,d01;
  int closecount=0,mirrorcount=0;
  alosta aalosta[3],balosta[3];
  vector<alosta> ret;
  aalosta[0].setStation(a,a1);
  aalosta[1].setStation(a,a2);
  balosta[0].setStation(b,b1);
  balosta[1].setStation(b,b2);
  do
  {
    insect=intersection(aalosta[0].station,aalosta[1].station,balosta[0].station,balosta[1].station);
    di0=dist(insect,aalosta[0].station);
    di1=dist(insect,aalosta[1].station);
    d01=dist(aalosta[0].station,aalosta[1].station);
    if (di1>d01 && di1>di0)
      di0=-di0;
    if (di0>d01 && di0>di1)
      di1=-di1;
    aalosta[2].along=(aalosta[0].along*di1+aalosta[1].along*di0)/d01;
    if (aalosta[2].along<-a->length()/2 || aalosta[2].along>3*a->length()/2)
    {
      aalosta[2].along=NAN;
      mirrorcount++;
    }
    if (!extend && aalosta[2].along<0)
    {
      aalosta[2].along=-aalosta[2].along;
      mirrorcount++;
    }
    if (!extend && aalosta[2].along>a->length())
    {
      aalosta[2].along=2*a->length()-aalosta[2].along;
      mirrorcount++;
    }
    aalosta[2].setStation(a,aalosta[2].along);
    di0=dist(insect,balosta[0].station);
    di1=dist(insect,balosta[1].station);
    d01=dist(balosta[0].station,balosta[1].station);
    if (di1>d01 && di1>di0)
      di0=-di0;
    if (di0>d01 && di0>di1)
      di1=-di1;
    balosta[2].along=(balosta[0].along*di1+balosta[1].along*di0)/d01;
    if (balosta[2].along<-b->length()/2 || balosta[2].along>3*b->length()/2)
    {
      balosta[2].along=NAN;
      mirrorcount++;
    }
    if (!extend && balosta[2].along<0)
    {
      balosta[2].along=-balosta[2].along;
      mirrorcount++;
    }
    if (!extend && balosta[2].along>b->length())
    {
      balosta[2].along=2*b->length()-balosta[2].along;
      mirrorcount++;
    }
    balosta[2].setStation(b,balosta[2].along);
    isnewcloser=sortpts(aalosta,balosta);
    //cout<<"isnewcloser "<<isnewcloser<<' '<<ldecimal(dist(aalosta[0].station,balosta[0].station))<<' '<<(a.length()+b.length()+dist(aalosta[0].station,-balosta[0].station))*DBL_EPSILON*4096<<endl;
    if (dist(aalosta[0].station,balosta[0].station)<(a->length()+b->length()+dist(aalosta[0].station,-balosta[0].station))*DBL_EPSILON*4096)
    {
      closecount++;
      if (aalosta[0].station==balosta[0].station)
	closecount++;
    }
    else
      closecount=0;
  }
  while (isnewcloser && closecount<2 && mirrorcount<256);
  if (closecount>1)
  {
    ret.push_back(aalosta[0]);
    ret.push_back(balosta[0]);
  }
  return ret;
}

vector<alosta> intersection1(segment *a,double a1,segment *b,double b1,bool extend)
/* Returns two alostas, one for a and one for b, or nothing.
 * If extend is true, the segments/arcs/spiralarcs are extended to twice their length;
 * e.g. one of length 5 extends from station -2.5 to station 7.5.
 * 
 * It can exit in three ways:
 * • The point in aalosta and the point in balosta which are closest are close
 *   enough to be the same point. They are returned.
 * • The new points are farther from each other and the previous points than
 *   previous points are from each other. Returns an empty vector.
 * • The new point is out of range of either or both of the curves.
 *   Returns an empty vector.
 */
{
  bool isnewcloser;
  xy insect;
  double di0,di1,d01;
  int closecount=0,mirrorcount=0;
  alosta aalosta[2],balosta[2];
  vector<alosta> ret;
  aalosta[0].setStation(a,a1);
  balosta[0].setStation(b,b1);
  do
  {
    insect=intersection(aalosta[0].station,aalosta[0].bearing,balosta[0].station,balosta[0].bearing);
    di0=distanceInDirection(aalosta[0].station,insect,aalosta[0].bearing);
    aalosta[1].along=aalosta[0].along+di0;
    if (aalosta[1].along<-a->length()/2 || aalosta[1].along>3*a->length()/2)
      aalosta[1].along=NAN;
    if (!extend && aalosta[1].along<0)
    {
      aalosta[1].along=-aalosta[1].along;
      mirrorcount++;
    }
    if (!extend && aalosta[1].along>a->length())
    {
      aalosta[1].along=2*a->length()-aalosta[1].along;
      mirrorcount++;
    }
    aalosta[1].setStation(a,aalosta[1].along);
    di0=distanceInDirection(balosta[0].station,insect,balosta[0].bearing);
    balosta[1].along=balosta[0].along+di0;
    if (balosta[1].along<-b->length()/2 || balosta[1].along>3*b->length()/2)
      balosta[1].along=NAN;
    if (!extend && balosta[1].along<0)
    {
      balosta[1].along=-balosta[1].along;
      mirrorcount++;
    }
    if (!extend && balosta[1].along>b->length())
    {
      balosta[1].along=2*b->length()-balosta[1].along;
      mirrorcount++;
    }
    balosta[1].setStation(b,balosta[1].along);
    isnewcloser=sortpts2(aalosta,balosta);
    //cout<<"isnewcloser "<<isnewcloser<<' '<<ldecimal(dist(aalosta[0].station,balosta[0].station))<<' '<<(a.length()+b.length()+dist(aalosta[0].station,-balosta[0].station))*DBL_EPSILON*4096<<endl;
    if (dist(aalosta[0].station,balosta[0].station)<(a->length()+b->length()+dist(aalosta[0].station,-balosta[0].station))*DBL_EPSILON*4096)
    {
      closecount++;
      if (aalosta[0].station==balosta[0].station)
	closecount++;
    }
    else
      closecount=0;
  }
  while (isnewcloser && closecount<2 && mirrorcount<256);
  if (closecount>1)
  {
    ret.push_back(aalosta[0]);
    ret.push_back(balosta[0]);
  }
  return ret;
}

/* If two spiralarcs intersect twice near the end of both, the secant method
 * may miss one, but the tangent method will find both. Conversely, if they
 * osculate, the tangent method will fail to converge because of roundoff,
 * but the secant method will find the intersection. So both methods are needed.
 */
vector<array<alosta,2> > intersections(segment *a,segment *b,bool extend)
/* Returns the intersections of a and b in order along a. In cases of multiple
 * intersection (tangency or osculation), it may return the wrong number of
 * intersections, occasionally even the wrong parity of number of intersections
 * (1 or 3 for tangent circles, where it should return 2). You must check whether
 * the resulting pieces of a are on opposite sides of b.
 */
{
  vector<array<alosta,2> > inters,ret;
  array<alosta,2> int1;
  vector<alosta> int0;
  vector<int> bounds;
  int h,i,j,adiv,bdiv,range,rangeSize;
  double maxcur,endcur,alen,blen;
  alen=a->length();
  blen=b->length();
  maxcur=fabs(a->curvature(0));
  endcur=fabs(a->curvature(alen));
  if (endcur>maxcur)
    maxcur=endcur;
  adiv=nearbyint(maxcur*alen+alen/blen)+3;
  if (adiv<3 || adiv>4096)
    adiv=4096;
  maxcur=fabs(b->curvature(0));
  endcur=fabs(b->curvature(blen));
  if (endcur>maxcur)
    maxcur=endcur;
  bdiv=nearbyint(maxcur*blen+blen/alen)+3;
  if (bdiv<3 || bdiv>4096)
    bdiv=4096;
  if (std::isnan(alen))
    adiv=-1;
  if (std::isnan(blen))
    bdiv=-1;
  for (i=0;i<adiv;i++)
    for (j=0;j<bdiv;j++)
    {
      int0=intersection1(a,i*alen/adiv,(i+1)*alen/adiv,b,j*blen/bdiv,(j+1)*blen/bdiv,extend);
      if (int0.size())
      {
	int1[0]=int0[0];
	int1[1]=int0[1];
	inters.push_back(int1);
      }
    }
  for (i=0;i<=adiv;i++)
    for (j=0;j<=bdiv;j++)
    {
      int0=intersection1(a,i*alen/adiv,b,j*blen/bdiv,extend);
      if (int0.size())
      {
	int1[0]=int0[0];
	int1[1]=int0[1];
	inters.push_back(int1);
      }
    }
  for (h=relprime(inters.size());h;h=(h>1)?relprime(h):0) // Shell sort
    for (i=h;i<inters.size();i++)
      for (j=i-h;j>=0 && (inters[j][0].along>inters[j+h][0].along || (inters[j][0].along==inters[j+h][0].along && inters[j][1].along>inters[j+h][1].along));j-=h)
	swap(inters[j],inters[j+h]);
  for (i=0;i<=inters.size();i++)
    if (i==0 || i==inters.size() || -abs(inters[i][0].bearing-inters[i][1].bearing-inters[i-1][0].bearing+inters[i-1][1].bearing)<-255)
    {
      bounds.push_back(i);
      //cout<<i<<' ';
    }
  //cout<<endl;
  for (range=0;range<bounds.size()-1;range++)
  {
    rangeSize=bounds[range+1]-bounds[range];
    for (h=relprime(rangeSize);h;h=(h>1)?relprime(h):0)
      for (i=h;i<rangeSize;i++) // Sort just the alostas of b within the range.
	for (j=i-h;j>=0 && (inters[j+bounds[range]][1].along>inters[j+bounds[range]+h][1].along);j-=h)
	  swap(inters[j+bounds[range]][1],inters[j+bounds[range]+h][1]);
    if (rangeSize&1)
      int1=inters[bounds[range]+rangeSize/2];
    else // Take the median, not the mean. There are usually a few
    {    // outliers which would make the mean inaccurate.
      int1[0].along=(inters[bounds[range]+rangeSize/2  ][0].along+
                     inters[bounds[range]+rangeSize/2-1][0].along)/2;
      if (int1[0].along==inters[bounds[range]+rangeSize/2][0].along)
	int1[0]=inters[bounds[range]+rangeSize/2][0];
      else if (int1[0].along==inters[bounds[range]+rangeSize/2-1][0].along)
	int1[0]=inters[bounds[range]+rangeSize/2-1][0];
      else
	int1[0].setStation(a,int1[0].along);
      int1[1].along=(inters[bounds[range]+rangeSize/2  ][1].along+
                     inters[bounds[range]+rangeSize/2-1][1].along)/2;
      if (int1[1].along==inters[bounds[range]+rangeSize/2][1].along)
	int1[1]=inters[bounds[range]+rangeSize/2][1];
      else if (int1[1].along==inters[bounds[range]+rangeSize/2-1][1].along)
	int1[1]=inters[bounds[range]+rangeSize/2-1][1];
      else
	int1[1].setStation(b,int1[1].along);
    }
    ret.push_back(int1);
  }
  //ret=inters;
  return ret;
}
