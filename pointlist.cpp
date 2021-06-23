/******************************************************/
/*                                                    */
/* pointlist.cpp - list of points                     */
/*                                                    */
/******************************************************/
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

#include <cmath>
#include "angle.h"
#include "pointlist.h"
#include "ldecimal.h"
#include "threads.h"

using namespace std;

const bool loudTinConsistency=false;

void pointlist::clear()
{
  wingEdge.lock();
  qinx.clear();
  contours.clear();
  triangles.clear();
  revtriangles.clear();
  edges.clear();
  points.clear();
  revpoints.clear();
  convexHull.clear();
  edgePool.clear();
  trianglePool.clear();
  swishFactor=0;
  wingEdge.unlock();
}

void pointlist::clearTin()
{
  wingEdge.lock();
  triangles.clear();
  revtriangles.clear();
  edges.clear();
  wingEdge.unlock();
}

int pointlist::size()
{
  return points.size();
}

void pointlist::clearmarks()
{
  map<int,edge>::iterator e;
  for (e=edges.begin();e!=edges.end();e++)
    e->second.clearmarks();
}

void pointlist::eraseEmptyContours()
{
  vector<polyspiral> nonempty;
  int i;
  for (i=0;i<contours.size();i++)
    if (contours[i].size()>2 || contours[i].isopen())
      nonempty.push_back(contours[i]);
  nonempty.shrink_to_fit();
  swap(contours,nonempty);
}

bool pointlist::checkTinConsistency()
{
  bool ret=true;
  int i,n,nInteriorEdges=0,nNeighborTriangles=0,turn1;
  double a;
  long long totturn;
  ptlist::iterator p;
  vector<int> edgebearings;
  edge *ed;
  for (p=points.begin();p!=points.end();++p)
  {
    ed=p->second.line;
    if (ed==nullptr || (ed->a!=&p->second && ed->b!=&p->second))
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" line pointer is wrong.\n";
    }
    edgebearings.clear();
    do
    {
      if (ed)
	ed=ed->next(&p->second);
      if (ed)
	edgebearings.push_back(ed->bearing(&p->second));
    } while (ed && ed!=p->second.line && edgebearings.size()<=edges.size());
    if (edgebearings.size()>=edges.size())
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" next pointers do not return to line pointer.\n";
    }
    for (totturn=i=0;i<edgebearings.size();i++)
    {
      turn1=(edgebearings[(i+1)%edgebearings.size()]-edgebearings[i])&(DEG360-1);
      totturn+=turn1;
      if (turn1==0)
      {
	ret=false;
	if (loudTinConsistency)
	  cerr<<"Point "<<p->first<<" has two equal bearings.\n";
      }
    }
    if (totturn!=(long long)DEG360) // DEG360 is construed as positive when cast to long long
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Point "<<p->first<<" bearings do not wind once counterclockwise.\n";
    }
  }
  for (i=0;i<edges.size();i++)
  {
    if (edges[i].isinterior())
      nInteriorEdges++;
    if ((edges[i].tria!=nullptr)+(edges[i].trib!=nullptr)!=1+edges[i].isinterior())
    {
      ret=false;
      if (loudTinConsistency)
	cerr<<"Edge "<<i<<" has wrong number of adjacent triangles.\n";
    }
    if (edges[i].tria)
    {
      a=n=0;
      if (edges[i].tria->a==edges[i].a || edges[i].tria->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->a);
      if (edges[i].tria->b==edges[i].a || edges[i].tria->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->b);
      if (edges[i].tria->c==edges[i].a || edges[i].tria->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].tria->c);
      if (n!=2)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle a does not have edge as a side.\n";
      }
      if (a>=0)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle a is on the wrong side.\n";
      }
    }
    if (edges[i].trib)
    {
      a=n=0;
      if (edges[i].trib->a==edges[i].a || edges[i].trib->a==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->a);
      if (edges[i].trib->b==edges[i].a || edges[i].trib->b==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->b);
      if (edges[i].trib->c==edges[i].a || edges[i].trib->c==edges[i].b)
        n++;
      else
        a+=area3(*edges[i].a,*edges[i].b,*edges[i].trib->c);
      if (n!=2)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle b does not have edge as a side.\n";
      }
      if (a<=0)
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Edge "<<i<<" triangle b is on the wrong side.\n";
      }
    }
  }
  for (i=0;i<triangles.size();i++)
  {
    if (triangles[i].aneigh)
    {
      nNeighborTriangles++;
      if ( triangles[i].aneigh->iscorner(triangles[i].a) ||
          !triangles[i].aneigh->iscorner(triangles[i].b) ||
          !triangles[i].aneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor a is wrong.\n";
      }
    }
    if (triangles[i].bneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].bneigh->iscorner(triangles[i].a) ||
           triangles[i].bneigh->iscorner(triangles[i].b) ||
          !triangles[i].bneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor b is wrong.\n";
      }
    }
    if (triangles[i].cneigh)
    {
      nNeighborTriangles++;
      if (!triangles[i].cneigh->iscorner(triangles[i].a) ||
          !triangles[i].cneigh->iscorner(triangles[i].b) ||
           triangles[i].cneigh->iscorner(triangles[i].c))
      {
        ret=false;
        if (loudTinConsistency)
	  cerr<<"Triangle "<<i<<" neighbor c is wrong.\n";
      }
    }
  }
  if (nInteriorEdges*2!=nNeighborTriangles)
  {
    ret=false;
    if (loudTinConsistency)
      cerr<<"Interior edges and neighbor triangles don't match.\n";
  }
  return ret;
}

void pointlist::addpoint(int numb,point pnt,bool overwrite)
{int a;
 if (points.count(numb))
    if (overwrite)
       points[a=numb]=pnt;
    else
       {if (numb<0)
           {a=points.begin()->first-1;
            if (a>=0)
               a=-1;
            }
        else
           {a=points.rbegin()->first+1;
            if (a<=0)
               a=1;
            }
        points[a]=pnt;
        }
 else
    points[a=numb]=pnt;
 revpoints[&(points[a])]=a;
 }

int pointlist::addtriangle(int n,int thread)
{
  int i;
  int newTriNum=triangles.size();
  if (thread>=0)
    lockNewTriangles(thread,n);
  for (i=0;i<n;i++)
  {
    triangles[newTriNum+i].sarea=0;
    revtriangles[&triangles[newTriNum+i]]=newTriNum+i;
  }
  return newTriNum;
}

void pointlist::insertHullPoint(point *newpnt,point *prec)
/* Inserts a point into the convex hull, which usually has much fewer points
 * than a path across the middle of the TIN. It is used to paint the space
 * around the TIN in the GUI.
 */
{
  int i;
  convexHull.push_back(newpnt);
  for (i=convexHull.size()-2;i>-1 && convexHull[i]!=prec;i--)
    swap(convexHull[i],convexHull[i+1]);
}

int pointlist::closestHullPoint(xy pnt)
{
  int i,ret=-1;
  double d,closeDist=INFINITY;
  for (i=0;i<convexHull.size();i++)
    if ((d=dist((xy)*convexHull[i],pnt))<closeDist)
    {
      ret=i;
      closeDist=d;
    }
  return ret;
}

double pointlist::distanceToHull(xy pnt)
/* pnt should be outside the hull. This is used to compute how big a scale
 * can be drawn in the corner of the window.
 */
{
  int n=closestHullPoint(pnt);
  int sz=convexHull.size();
  xy a,b,c;
  double ret=NAN;
  if (sz && n>=0)
  {
    a=(xy)*convexHull[(n+sz-1)%sz];
    b=(xy)*convexHull[n];
    c=(xy)*convexHull[(n+1)%sz];
    ret=dist(b,pnt);
    if (isinsector(dir(b,pnt)-dir(b,a),0xf00ff00f) && pldist(pnt,b,a)<ret)
      ret=pldist(pnt,b,a);
    if (isinsector(dir(b,pnt)-dir(b,c),0xf00ff00f) && pldist(pnt,c,b)<ret)
      ret=pldist(pnt,c,b);
  }
  return ret;
}

bool pointlist::validConvexHull()
/* Deflection angles around the convex hull must be positive (else it isn't convex)
 * and no more than 45° (because it starts as an equiangular octagon).
 */
{
  int i;
  bool ret=true;
  int deflectionAngle;
  int sz=convexHull.size();
  for (i=0;i<sz;i++)
  {
    deflectionAngle=foldangle(dir((xy)*convexHull[(i+1)%sz],(xy)*convexHull[i])-
			      dir((xy)*convexHull[i],(xy)*convexHull[(i+sz-1)%sz]));
    //cout<<i<<' '<<ldecimal(bintodeg(deflectionAngle))<<endl;
    if (deflectionAngle<1 || deflectionAngle>DEG45+1)
      ret=false;
  }
  return ret;
}

vector<int> pointlist::valencyHistogram()
{
  int j;
  ptlist::iterator i;
  edge *e;
  point *p;
  vector<int> ret;
  wingEdge.lock_shared();
  for (i=points.begin();i!=points.end();++i)
  {
    p=&i->second;
    e=p->line;
    for (j=0;j==0 || e!=p->line;j++)
      e=e->next(p);
    if (ret.size()<=j)
      ret.resize(j+1);
    ret[j]++;
  }
  wingEdge.unlock_shared();
  return ret;
}

bool pointlist::shouldWrite(int n,int flags)
/* Called from a function that exports a TIN (except in STL format) to export
 * some of the triangles. The flags come from a ThreadAction (see threads.h).
 * If bit 0 is set, write empty triangles.
 * If bit 1 is set and there is a boundary, write only triangles whose centroid
 * is in the boundary.
 */
{
  bool ret=triangles[n].dots.size() || (flags&1);
  if (boundary.size() && (flags&2))
    ret=ret && boundary.in(triangles[n].centroid());
  return ret;
}

void pointlist::makeqindex()
{
  vector<xy> plist;
  ptlist::iterator i;
  qinx.clear();
  for (i=points.begin();i!=points.end();++i)
    plist.push_back(i->second);
  qinx.sizefit(plist);
  qinx.split(plist);
  if (triangles.size())
    qinx.settri(&triangles[0]);
}

void pointlist::updateqindex()
/* Use this when you already have a quad index, split to cover all the points,
 * but the leaves don't point to the right triangles because you've flipped
 * some edges.
 */
{
  if (triangles.size())
    qinx.settri(&triangles[0]);
}

double pointlist::elevation(xy location)
{
  triangle *t;
  t=qinx.findt(location);
  if (t)
    return t->elevation(location);
  else
    return nan("");
}

double pointlist::dirbound(int angle)
/* angle=0x00000000: returns least easting.
 * angle=0x20000000: returns least northing.
 * angle=0x40000000: returns negative of greatest easting.
 */
{
  ptlist::iterator i;
  double bound=HUGE_VAL,turncoord;
  double s=sin(angle),c=cos(angle);
  for (i=points.begin();i!=points.end();++i)
  {
    turncoord=i->second.east()*c+i->second.north()*s;
    if (turncoord<bound)
      bound=turncoord;
  }
  return bound;
}

triangle *pointlist::findt(xy pnt,bool clip)
{
  return qinx.findt(pnt,clip);
}

void pointlist::roscat(xy tfrom,int ro,double sca,xy tto)
{
  xy cs=cossin(ro);
  ptlist::iterator j;
  for (j=points.begin();j!=points.end();++j)
    j->second._roscat(tfrom,ro,sca,cossin(ro)*sca,tto);
}

