/******************************************************/
/*                                                    */
/* point.cpp - classes for points                     */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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

#include "point.h"
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstring>
#include "tin.h"
#include "ldecimal.h"
#include "angle.h"
using namespace std;

xy::xy(double e,double n)
{
  x=e;
  y=n;
}

xy::xy()
{
  x=0;
  y=0;
}

xy::xy(xyz point)
{
  x=point.x;
  y=point.y;
}

double xy::east() const
{
  return x;
}

double xy::north() const
{
  return y;
}

double xy::getx() const
{
  return x;
}

double xy::gety() const
{
  return y;
}

double xy::length() const
{
  return hypot(x,y);
}

bool xy::isfinite() const
{
  return std::isfinite(x) && std::isfinite(y);
}

bool xy::isnan() const
{
  return std::isnan(x) || std::isnan(y);
}

double xy::dirbound(int angle)
/* angle=0x00000000: returns easting.
 * angle=0x20000000: returns northing.
 * angle=0x40000000: returns negative of easting.
 */
{
  double s=sin(angle),c=cos(angle);
  return x*c+y*s;
}

void xy::_roscat(xy tfrom,int ro,double sca,xy cis,xy tto)
{
  double tx,ty;
  x-=tfrom.x;
  y-=tfrom.y;
  tx=x*cis.x-y*cis.y;
  ty=y*cis.x+x*cis.y;
  x=tx+tto.x;
  y=ty+tto.y;
}

void xy::roscat(xy tfrom,int ro,double sca,xy tto)
{
  _roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
}

xy operator+(const xy &l,const xy &r)
{xy sum(l.x+r.x,l.y+r.y);
 return sum;
 }

xy operator+=(xy &l,const xy &r)
{
  l.x+=r.x;
  l.y+=r.y;
  return l;
}

xy operator-=(xy &l,const xy &r)
{
  l.x-=r.x;
  l.y-=r.y;
  return l;
}

xy operator*(double l,const xy &r)
{
  xy prod(l*r.x,l*r.y);
  return prod;
}

xy operator*(const xy &l,double r)
{
  xy prod(l.x*r,l.y*r);
  return prod;
}

xy operator-(const xy &l,const xy &r)
{
  xy sum(l.x-r.x,l.y-r.y);
  return sum;
}

xy operator-(const xy &r)
{
  xy sum(-r.x,-r.y);
  return sum;
}

xy operator/(const xy &l,double r)
{xy prod(l.x/r,l.y/r);
 return prod;
 }

xy operator/=(xy &l,double r)
{
  l.x/=r;
  l.y/=r;
  return l;
}

bool operator!=(const xy &l,const xy &r)
{
  return l.x!=r.x || l.y!=r.y;
}

bool operator==(const xy &l,const xy &r)
{
  return l.x==r.x && l.y==r.y;
}

xy turn90(xy a)
{
  return xy(-a.y,a.x);
}

xy turn(xy a,int angle)
{
  double s,c;
  s=sin(angle);
  c=cos(angle);
  return xy(c*a.x-s*a.y,s*a.x+c*a.y);
}

double dist(xy a,xy b)
{
  return hypot(a.x-b.x,a.y-b.y);
}

int dir(xy a,xy b)
{
  return atan2i(b-a);
}

double dot(xy a,xy b)
{
  return (a.y*b.y+a.x*b.x);
}

const xy beforestart(-INFINITY,-INFINITY);
const xy afterend(INFINITY,INFINITY);
const xy nanxy(NAN,NAN);

xyz::xyz(double e,double n,double h)
{
  x=e;
  y=n;
  z=h;
}

xyz::xyz(xy en,double h)
{
  x=en.x;
  y=en.y;
  z=h;
}

double xyz::east() const
{
  return x;
}

double xyz::north() const
{
  return y;
}

double xyz::elev() const
{
  return z;
}

double xyz::getx() const
{
  return x;
}

double xyz::gety() const
{
  return y;
}

bool xyz::isfinite() const
{
  return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
}

bool xyz::isnan() const
{
  return std::isnan(x) || std::isnan(y) || std::isnan(z);
}

double xyz::getz() const
{
  return z;
}

double xyz::length()
{
  return sqrt(x*x+y*y+z*z);
}

xyz::xyz()
{
  x=y=z=0;
}

void xyz::raise(double height)
{
  z+=height;
}
void xyz::_roscat(xy tfrom,int ro,double sca,xy cis,xy tto)
{
  double tx,ty;
  x-=tfrom.x;
  y-=tfrom.y;
  tx=x*cis.x-y*cis.y;
  ty=y*cis.x+x*cis.y;
  x=tx+tto.x;
  y=ty+tto.y;
}

double dot(xyz a,xyz b)
{
  return a.x*b.x+a.y*b.y+a.z*b.z;
}

xyz cross(xyz a,xyz b)
{
  xyz ret;
  ret.x=a.y*b.z-b.y*a.z;
  ret.y=a.z*b.x-b.z*a.x;
  ret.z=a.x*b.y-b.x*a.y;
  return ret;
}

void xyz::roscat(xy tfrom,int ro,double sca,xy tto)
{
  _roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
}

bool operator==(const xyz &l,const xyz &r)
{
  return l.x==r.x && l.y==r.y && l.z==r.z;
}

bool operator!=(const xyz &l,const xyz &r)
{
  return l.x!=r.x || l.y!=r.y || l.z!=r.z;
}

xyz operator/(const xyz &l,const double r)
{
  return xyz(l.x/r,l.y/r,l.z/r);
}

xyz operator*=(xyz &l,double r)
{
  l.x*=r;
  l.y*=r;
  l.z*=r;
  return l;
}

xyz operator/=(xyz &l,double r)
{
  l.x/=r;
  l.y/=r;
  l.z/=r;
  return l;
}

xyz operator+=(xyz &l,const xyz &r)
{
  l.x+=r.x;
  l.y+=r.y;
  l.z+=r.z;
  return l;
}

xyz operator-=(xyz &l,const xyz &r)
{
  l.x-=r.x;
  l.y-=r.y;
  l.z-=r.z;
  return l;
}

xyz operator*(const xyz &l,const double r)
{
  return xyz(l.x*r,l.y*r,l.z*r);
}

xyz operator*(const double l,const xyz &r)
{
  return xyz(l*r.x,l*r.y,l*r.z);
}

xyz operator*(const xyz &l,const xyz &r) // cross product
{
  return xyz(l.y*r.z-l.z*r.y,l.z*r.x-l.x*r.z,l.x*r.y-l.y*r.x);
}

xyz operator+(const xyz &l,const xyz &r)
{
  return xyz(l.x+r.x,l.y+r.y,l.z+r.z);
}

xyz operator-(const xyz &l,const xyz &r)
{
  return xyz(l.x-r.x,l.y-r.y,l.z-r.z);
}

xyz operator-(const xyz &r)
{
  return xyz(-r.x,-r.y,-r.z);
}

double dist(xyz a,xyz b)
{
  return hypot(hypot(a.x-b.x,a.y-b.y),a.z-b.z);
}

const xyz nanxyz(NAN,NAN,NAN);

point::point()
{
  x=y=z=0;
  line=NULL;
}

point::point(double e,double n,double h)
{
  x=e;
  y=n;
  z=h;
  line=0;
}

point::point(xy pnt,double h)
{
  x=pnt.x;
  y=pnt.y;
  z=h;
  line=0;
}

point::point(xyz pnt)
{
  x=pnt.x;
  y=pnt.y;
  z=pnt.z;
  line=0;
}

point::point(const point &rhs)
{
  x=rhs.x;
  y=rhs.y;
  z=rhs.z;
  line=rhs.line;
}

point& point::operator=(const point &rhs)
{
  if (this!=&rhs)
  {
    x=rhs.x;
    y=rhs.y;
    z=rhs.z;
    line=rhs.line;
  }
  return *this;
}

int point::valence()
{
  int i;
  edge *oldline;
  for (i=0,oldline=line;line && (!i || oldline!=line);i++)
    line=line->next(this);
  return i;
}

vector<edge *> point::incidentEdges()
{
  int i;
  vector<edge *> ret;
  edge *oldline;
  for (i=0,oldline=line;line && (!i || oldline!=line);i++)
  {
    line=line->next(this);
    ret.push_back(line);
  }
  return ret;
}

edge *point::isNeighbor(point *pnt)
/* Returns the edge if this and pnt are neighbors. If this is pnt, returns some
 * edge if this has any neighbors, else nullptr.
 */
{
  int i;
  edge *ret=nullptr;
  edge *oldline;
  for (i=0,oldline=line;line && (!i || oldline!=line);i++)
  {
    line=line->next(this);
    if (line->a==pnt || line->b==pnt)
    {
      ret=line;
      break;
    }
  }
  return ret;
}

void point::insertEdge(edge *edg)
/* Inserts edg into the circular linked list of edges around this point.
 * One end of edg must be this point. If its bearing is exactly equal to
 * an already linked edge's, which means there's a flat triangle, you can get
 * a mistake.
 */
{
  int newBearing=edg->bearing(this);
  int i,minAnglePos,maxAnglePos,minAngle=DEG360-1,maxAngle=0;
  edge *oldline;
  vector<edge *> edges;
  vector<int> angles;
  for (i=0,oldline=line;line && (!i || oldline!=line);i++)
  {
    line=line->next(this);
    edges.push_back(line);
    angles.push_back((line->bearing(this)-newBearing)&(DEG360-1));
    if (angles[i]>=maxAngle)
    {
      maxAngle=angles[i];
      maxAnglePos=i;
    }
    if (angles[i]<=minAngle)
    {
      minAngle=angles[i];
      minAnglePos=i;
    }
  }
  if (angles.size())
  {
    if (!minAngle)
      cerr<<"Point at ("<<ldecimal(x)<<','<<ldecimal(y)<<','<<ldecimal(z)
          <<") is trying to insert edge with bearing "<<newBearing<<" ("
	  <<ldecimal(bintodeg(newBearing),bintodeg(1))<<"), but one already exists.\n";
    assert(minAngle);
    assert(minAnglePos==(maxAnglePos+1)%angles.size());
    edges[maxAnglePos]->setnext(this,edg);
    edg->setnext(this,edges[minAnglePos]);
  }
  else
  {
    edg->setnext(this,edg);
    line=edg;
  }
}

void point::removeEdge(edge *edg)
/* Removes edg from the circular linked list of edges around this point.
 */
{
  int i;
  edge *oldline;
  for (i=0,oldline=line;line && (!i || oldline!=line);i++,line=line->next(this))
  {
    if (line->next(this)==edg)
    {
      if (line==edg)
	line=nullptr;
      else
	line->setnext(this,edg->next(this));
      break;
    }
  }
}

edge *point::edg(triangle *tri)
{
  int i;
  edge *oldline,*l,*ret;
  for (i=0,oldline=l=line,ret=nullptr;!ret && l && (!i || oldline!=l);i++)
  {
    if (l->tri(this)==tri)
      ret=l;
    l=l->next(this);
    assert(l);
  }
  return ret;
}

/*void point::setedge(point *oend)
{int i;
 edge *oldline;
 for (i=0,oldline=line;(!i || oldline==line) && line->next(this)->otherend(this)!=oend;i++)
     line=line->next(this);
 }*/

