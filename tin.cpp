/******************************************************/
/*                                                    */
/* tin.cpp - triangulated irregular network           */
/*                                                    */
/******************************************************/
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

#include <map>
#include <cmath>
#include <iostream>
#include "tin.h"
#include "ps.h"
#include "point.h"
#include "pointlist.h"
#include "ldecimal.h"
#include "manysum.h"
#include "random.h"
#include "smooth5.h"
#include "relprime.h"

#define THR 16777216
//threshold for goodcenter to determine if a point is sufficiently
//on the good side of a side of a triangle

using std::map;
using std::multimap;
using std::vector;
using namespace std;

edge::edge()
{
  a=b=nullptr;
  nexta=nextb=nullptr;
  tria=trib=nullptr;
  extrema[0]=extrema[1]=NAN;
  broken=contour=stlsplit=0;
  flipcnt=0;
}

edge* edge::next(point* end)
{if (end==a)
    return nexta;
 else
    return nextb;
 }

triangle* edge::tri(point* end)
{if (end==a)
    return tria;
 else
    return trib;
 }

void edge::setnext(point* end,edge* enext)
{
  if (end==a)
    nexta=enext;
  else
    nextb=enext;
}

int edge::bearing(point *end)
// Returns the bearing of the other end from this one.
{
  if (end==a)
    return dir(xy(*a),xy(*b));
  else
    return dir(xy(*b),xy(*a));
}

void edge::setNeighbors()
// Sets the triangles on the sides of the edge to point to each other.
{
  if (tria && trib)
  {
    tria->setneighbor(trib);
    trib->setneighbor(tria);
  }
}

point* edge::otherend(point* end)
{
  assert(end==a || end==b);
  if (end==a)
    return b;
  else
    return a;
}

triangle* edge::othertri(triangle* t)
{
  assert(t==tria || t==trib);
  if (t==tria)
    return trib;
  else
    return tria;
}

xy edge::midpoint()
{
  return (xy(*a)+xy(*b))/2;
}

void edge::dump(pointlist *topopoints)
{printf("addr=%p a=%d b=%d nexta=%p nextb=%p\n",this,topopoints->revpoints[a],topopoints->revpoints[b],nexta,nextb);
 }

void edge::flip(pointlist *topopoints)
/* Given an edge which is a diagonal of a quadrilateral,
 * sets it to the other diagonal. It is rotated clockwise.
 */
{
  edge *temp1,*temp2;
  int i,size;
  size=topopoints->points.size();
  for (i=0;i<size && a->line->next(a)!=this;i++)
    a->line=a->line->next(a);
  assert(i<size); //If this assertion fails, the nexta and nextb pointers are messed up.
  for (i=0;i<size && b->line->next(b)!=this;i++)
    b->line=b->line->next(b);
  assert(i<size);
  // The four edges of the quadrilateral are now nexta, a->line, nextb, and b->line.
  if (nexta->a==a)
    nexta->nextb=this;
  else
    nexta->nexta=this;
  if (nextb->b==b)
    nextb->nexta=this;
  else
    nextb->nextb=this;
  if (a->line->a==a)
    a->line->nexta=nexta;
  else
    a->line->nextb=nexta;
  if (b->line->b==b)
    b->line->nextb=nextb;
  else
    b->line->nexta=nextb;
  temp1=b->line;
  temp2=a->line;
  a=nexta->otherend(a);
  b=nextb->otherend(b);
  nexta=temp1;
  nextb=temp2;
  /* Now adjust the triangles, if there are any. When showing the TIN on screen
   * and letting the user flip edges, the triangles are needed for hit-testing.
   * The control points are unaffected, so elevations are garbage, unless you
   * flip the same edge four times and the edge was side a of both triangles
   * before flipping.
   */
  for (i=0;i<size && a->line->next(a)!=this;i++)
    a->line=a->line->next(a);
  assert(i<size);
  for (i=0;i<size && b->line->next(b)!=this;i++)
    b->line=b->line->next(b);
  assert(i<size);
  if (tria)
  {
    tria->a=nextb->otherend(b);
    tria->b=b;
    tria->c=a;
    if (a->line->a==a)
      a->line->trib=tria;
    else
      a->line->tria=tria;
    a->line->setNeighbors();
    nextb->setNeighbors();
    tria->peri=tria->perimeter();
    tria->sarea=tria->area();
  }
  if (trib)
  {
    trib->a=nexta->otherend(a);
    trib->b=a;
    trib->c=b;
    if (b->line->a==b)
      b->line->trib=trib;
    else
      b->line->tria=trib;
    b->line->setNeighbors();
    nexta->setNeighbors();
    trib->peri=tria->perimeter();
    trib->sarea=tria->area();
  }
  setNeighbors();
  broken&=~4; // checkBreak0 has to recompute bits 0 and 1
  flipcnt++;
}

void edge::reverse()
/* Used when constructing a TIN from triangles read in, to find the boundary.
 * Equivalent to flipping twice. extrema is unaffected, as when the TIN has
 * just been read in, the extrema aren't calculated yet.
 */
{
  swap(a,b);
  swap(nexta,nextb);
  swap(tria,trib);
}

bool edge::isinterior()
// Returns true if this edge is in the interior; false if it's in the convex hull -
// or if the pointers are messed up.
{
  point *tempa,*tempb;
  tempa=nexta->otherend(a);
  tempb=nextb->otherend(b);
  return nexta->next(tempa)->otherend(tempa)==b &&
         nextb->next(tempb)->otherend(tempb)==a &&
         tempa!=tempb;
}

bool edge::isFlippable()
/* Returns true if the edge is interior and is the diagonal
 * of a convex quadrilateral. Flipping an edge that is the diagonal
 * of a concave quadrilateral results in an overhanging cliff or cave.
 */
{
  point *tempa,*tempb;
  tempa=nexta->otherend(a);
  tempb=nextb->otherend(b);
  return nexta->next(tempa)->otherend(tempa)==b &&
         nextb->next(tempb)->otherend(tempb)==a &&
         tempa!=tempb &&
         intersection_type(*a,*b,*tempa,*tempb)==ACXBD;
}

bool edge::delaunay()
{
  point *tempa,*tempb;
  if (nexta==NULL || nextb==NULL)
  {
    std::cerr<<"null next edge in delaunay\n";
    return true;
  }
  tempa=nexta->otherend(a);
  tempb=nextb->otherend(b);
  return (!isinterior()) || ::delaunay(*a,*b,*tempa,*tempb);
}

multimap<double,point*> convexhull;
// The points are ordered by their azimuth from the starting point.
xy startpnt;
multimap<double,point*> outward;
// The points are ordered by their distance from the starting point.

void dumphull()
{multimap<double,point*>::iterator i;
 printf("dump convex hull:\n");
 for (i=convexhull.begin();i!=convexhull.end();i++)
     printf("az=%f pt=%p\n",i->first,i->second);
 //printf("begin=%p end=%p rbegin=%p rend=%p\n",convexhull.begin(),convexhull.end(),convexhull.rbegin(),convexhull.rend());
 printf("end dump\n");
 }

void pointlist::dumpedges()
{
  map<int,edge>::iterator i;
  printf("dump edges:\n");
  for (i=edges.begin();i!=edges.end();i++)
     i->second.dump(this);
  printf("end dump\n");
}

void pointlist::dumpedges_ps(PostScript &ps,bool colorfibaster)
{
  map<int,edge>::iterator i;
  int n;
  for (i=edges.begin(),n=0;i!=edges.end();i++,n++)
     ps.line(i->second,n,colorfibaster);
}

void pointlist::dumpnext_ps(PostScript &ps)
{
  map<int,edge>::iterator i;
  ps.setcolor(0,0.7,0);
  for (i=edges.begin();i!=edges.end();i++)
  {
    if (i->second.nexta)
      ps.line2p(i->second.midpoint(),i->second.nexta->midpoint());
    if (i->second.nextb)
      ps.line2p(i->second.midpoint(),i->second.nextb->midpoint());
  }
}

void dumphull_ps(PostScript &ps)
{
  multimap<double,point*>::iterator i;
  xy pnt,pnt1;
  int j;
  ps.widen(5);
  for (i=convexhull.begin(),j=0;i!=convexhull.end();i++,j++)
  {
    ps.setcolor(0,0,0);
    ps.write(xy(10,3*j+10),ldecimal(i->first));
    ps.setcolor(1,1,0);
    if (j)
      ps.line2p(pnt,*i->second);
    else
      pnt1=*i->second;
    pnt=*i->second;
  }
  ps.line2p(pnt,pnt1);
  ps.widen(0.2);
}

void pointlist::splitBreaklines()
// Split the breaklines, which are lists of points, into individual line segments.
{
  int i,j;
  array<int,2> bl;
  break0.clear();
  for (i=0;i<type0Breaklines.size();i++)
    for (j=0;j<type0Breaklines[i].size();j++)
    {
      bl=type0Breaklines[i][j];
      if (points.count(bl[0])==0 || points.count(bl[1])==0)
        throw badBreaklineEnd;
      break0.push_back(segment(points[bl[0]],points[bl[1]]));
    }
}

double edge::length()
{
  xy c,d;
  c=*a;
  d=*b;
  return dist(c,d);
}

segment edge::getsegment()
{
  segment ret(*a,*b);
  triangle *tri;
  if (tria)
    tri=tria;
  else
    tri=trib;
  if (tri)
  {
    ret.setctrl(START,tri->ctrlpt(*a,*b));
    ret.setctrl(END,tri->ctrlpt(*b,*a));
  }
  return ret;
}

array<double,4> edge::ctrlpts()
{
  array<double,4> ret;
  if (tria)
  {
    ret[0]=tria->ctrlpt(*a,*b);
    ret[2]=tria->ctrlpt(*b,*a);
  }
  else
    ret[0]=ret[2]=NAN;
  if (trib)
  {
    ret[1]=trib->ctrlpt(*a,*b);
    ret[3]=trib->ctrlpt(*b,*a);
  }
  else
    ret[1]=ret[3]=NAN;
  return ret;
}

void edge::findextrema()
{
  int i;
  vector<double> ext;
  ext=getsegment().vextrema(false);
  for (i=0;i<2;i++)
    if (i>=ext.size())
      extrema[i]=nan("");
    else
      extrema[i]=ext[i];
}

xyz edge::critpoint(int i)
{
  return getsegment().station(extrema[i]);
}

void edge::clearmarks()
{
  contour=0;
}

void edge::mark(int n)
{
  contour|=(1<<n);
}

bool edge::ismarked(int n)
{
  return (contour>>n)&1;
}

void edge::stlSplit(double maxError)
{
  segment thisSeg=getsegment();
  double maxAccel,error;
  maxAccel=thisSeg.accel(0);
  if (thisSeg.accel(length())>maxAccel)
    maxAccel=thisSeg.accel(length());
  error=sqr(length())*maxAccel/4;
  for (stlmin=0;stlmin<216 && error>maxError*sqr(stltable[stlmin]);stlmin++);
  if (stlmin==216)
    stlmin--;
}

bool goodcenter(xy a,xy b,xy c,xy d)
/* a is the proposed starting point; b, c, and d are the three closest
   points to a. a has to be on the same side of at least two sides as the
   interior of the triangle.

   If a is inside the triangle, the convex hull procedure will succeed (as
   long as no two points coincide). If a is out a corner, the procedure
   is guaranteed to fail. If a is out a side, it may or may not fail. In
   a lozenge, the procedure succeeds when a is out a side and the triangle
   is not straight (the usual cause for rejection in the lozenge is a
   straight triangle). The locus of a which is inside the triangle is
   quite tiny (about 1/4n). In Independence Park, however, the locus of
   a which is inside the triangle is most of the park. Points in the street
   are mostly out the side of the triangle, but result in failure. The best
   way to pick the point, then, is to allow points out the side, but check
   for failure, and if it happens, try another point.
   */
{
  double A,B,C,D,perim;
  int n;
  A=area3(b,c,d);
  B=area3(a,c,d);
  C=area3(b,a,d);
  D=area3(b,c,a);
  if (A<0)
    {
      A=-A;
      B=-B;
      C=-C;
      D=-D;
    }
  n=(B>A/THR)+(C>A/THR)+(D>A/THR);
  if (fabs(B/THR)>A || fabs(C/THR)>A || fabs(D/THR)>A)
    n=0;
  perim=dist(b,c)+dist(c,d)+dist(d,b);
  if (A<perim*perim/THR)
    n=0;
  return n>1;
}

int pointlist::checkBreak0(edge &e)
{
  int i;
  segment s;
  if ((e.broken&4)==0)
  {
    e.broken&=-4;
    s=e.getsegment();
    for (i=0;i<break0.size();i++)
    {
      if (intersection_type(s,break0[i])==ACXBD)
        e.broken|=2;
      if (sameXyz(s,break0[i]))
        e.broken|=1;
    }
    e.broken|=4;
  }
  return e.broken&3;
}

bool pointlist::shouldFlip(edge &e)
{
  bool ret;
  if (e.flipcnt>30)
    cout<<"overflip"<<endl;
  switch (checkBreak0(e))
  {
    case 0: // no breaklines here, check Delaunay
      ret=!e.delaunay();
      break;
    case 1: // edge is in a breakline, don't flip
      ret=false;
      break;
    case 2: // edge crosses a breakline, flip if possible
      ret=e.isFlippable();
      break;
    case 3: // edge is in one breakline and crosses another, error
      throw breaklinesCross;
      break;
  }
  return ret;
}

bool pointlist::tryStartPoint(PostScript &ps,xy &startpnt)
/* This is the sweep-hull algorithm (http://s-hull.org), except that the
 * startpoint is random instead of the circumcenter of three points.
 * I did not know about the algorithm when I wrote it.
 */
{
  int m,n,val,maxedges,edgeoff;
  double maxdist,mindist,idist,minx,miny,maxx,maxy;
  xy A,B,C,farthest;
  multimap<double,point*>::iterator j,k,inspos,left,right;
  vector<point*> visible; // points of convex hull visible from new point
  ptlist::iterator i;
  bool fail;
  maxedges=3*points.size()-6;
  edges.clear();
  convexhull.clear();
  for (m=0;m<100;m++)
  {
    outward.clear();
    for (i=points.begin();i!=points.end();i++)
      outward.insert(ipoint(dist(startpnt,i->second),&i->second));
    for (j=outward.begin(),n=0;j!=outward.end();j++,n++)
    {
      if (n==0)
        A=*(j->second);
      if (n==1)
        B=*(j->second);
      if (n==2)
        C=*(j->second);
      farthest=*(j->second);
    }
    //printf("m=%d startpnt=(%f,%f)\n",m,startpnt.east(),startpnt.north());
    if (m>0 && goodcenter(startpnt,A,B,C))
    {
      //printf("m=%d found good center\n",m);
      break;
    }
    if (m&4)
      startpnt=rand2p(startpnt,farthest);
    else
      startpnt=rand2p(startpnt,C);
  }
  miny=maxy=startpnt.north();
  minx=maxx=startpnt.east();
  for (i=points.begin();i!=points.end();i++)
  {
    if (i->second.east()>maxx)
      maxx=i->second.east();
    if (i->second.east()<minx)
      minx=i->second.east();
    if (i->second.north()>maxy)
      maxy=i->second.north();
    if (i->second.north()<miny)
      miny=i->second.north();
  }
  if (ps.isOpen())
  {
    ps.setscale(minx,miny,maxx,maxy);
    ps.startpage();
    ps.setcolor(0,0,1);
    ps.dot(startpnt);
    ps.setcolor(1,.5,0);
    for (i=points.begin();i!=points.end();i++)
      ps.dot(i->second,to_string(revpoints[&i->second]));
    ps.endpage();
  }
  j=outward.begin();
  //printf("edges %d\n",edges.size());
  edges[0].a=j->second;
  j->second->line=&(edges[0]);
  convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
  j++;
  edges[0].b=j->second;
  j->second->line=&(edges[0]);
  edges[0].nexta=edges[0].nextb=&(edges[0]);
  convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
  //printf("edges %d\n",edges.size());
  /* Before:
   * A-----B
   * |    /|
   * |   / |
   * |  /  |
   * | /   |
   * |/    |
   * C-----D
   *
   *          E
   * outward=(D,C,B,A,E)
   * edges=(DC,BD,BC,AB,AC)
   * After:
   * edges=(DC,BD,BC,AB,AC,EC,ED,EB)
   */
  for (j++;j!=outward.end();j++)
  {
    convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
    inspos=convexhull.find(dir(startpnt,*(j->second)));
    /* First find how much the convex hull subtends as seen from the new point,
     * expressed as distance from the startpnt to the line between an old point
     * and the new point.
     */
    mindist=dist(startpnt,*(j->second));
    maxdist=-mindist;
    for (left=inspos,idist=n=0;/*idist>=maxdist && */n<convexhull.size();left--,n++)
    {
      if (left->second==j->second)
        idist=0;
      else
      { // i points to a list of points, j to a map of pointers to points
        idist=pldist(startpnt,*(left->second),*(j->second));
        if (idist>maxdist)
          maxdist=idist;
      }
      if (left==convexhull.begin())
        left=convexhull.end();
    }
    for (right=inspos,idist=n=0;/*idist<=mindist && */n<convexhull.size();right++,n++)
    {
      if (right==convexhull.end())
        right=convexhull.begin();
      if (right->second==j->second)
        idist=0;
      else
      {
        idist=pldist(startpnt,*(right->second),*(j->second));
        if (idist<mindist)
          mindist=idist;
      }
    }
    //printf("maxdist=%f mindist=%f\n",maxdist,mindist);
    /* Then find which convex hull points are the rightmost and leftmost
     * that can be seen from the new point.
     */
    for (left=inspos,idist=n=0;n<2||idist<maxdist;left--,n++)
    {
      if (left->second==j->second)
        idist=0;
      else
        idist=pldist(startpnt,*(left->second),*(j->second));
      if (left==convexhull.begin())
        left=convexhull.end();
    }
    left++;
    for (right=inspos,idist=n=0;n<2||idist>mindist;right++,n++)
    {
      if (right==convexhull.end())
        right=convexhull.begin();
      if (right->second==j->second)
        idist=0;
      else
        idist=pldist(startpnt,*(right->second),*(j->second));
    }
    right--;
    //putchar('\n');
    // Now make a list of the visible points. There are 3 on average.
    visible.clear();
    edgeoff=edges.size();
    for (k=left,n=0,m=1;m;k++,n++,m++)
    {
      if (k==convexhull.end())
        k=convexhull.begin();
      if (k!=inspos) // skip the point just added - don't join it to itself
      {
        visible.push_back(k->second); // this adds one element, hence -1 in next line
        edges[edges.size()].a=j->second;
        edges[edges.size()-1].b=k->second;
      }
      if (k==right || n==maxedges)
        m=-1;
    }
    val=--n; // subtract one for the point itself
    //printf("%d points visible\n",n);
    // Now delete old convex hull points that are now in the interior.
    for (m=1;m<visible.size()-1;m++)
      if (convexhull.erase(dir(startpnt,*visible[m]))>1)
        throw(samePoints);
    //dumppoints();
    //dumpedges();
    for (n=0;n<val;n++)
    {
      edges[(n+1)%val+edgeoff].nexta=&edges[n+edgeoff];
      if (visible[n]->line)
      {
        assert(n+edgeoff<edges.size() && n+edgeoff>=0);
        edges[n+edgeoff].nextb=visible[n]->line->next(visible[n]);
        visible[n]->line->setnext(visible[n],&edges[n+edgeoff]);
      }
    }
    for (fail=false,n=edgeoff;n<edges.size();n++)
      if (edges[n].nexta==NULL || edges[n].nextb==NULL)
        fail=true;
    if (fail)
      break;
    //dumpedges();
    j->second->line=&edges[edgeoff];
    visible[val-1]->line=&edges[edgeoff+val-1];
  }
  if (!goodcenter(startpnt,A,B,C))
    fail=true;
  return fail;
}

int1loop pointlist::convexHull()
/* This is used when reading a bare TIN from a DXF file. The difference between
 * the convex hull and the boundary is a region that must be filled in
 * with triangles.
 */
{
  int m,n,val,maxedges,edgeoff;
  double maxdist,mindist,idist,minx,miny,maxx,maxy;
  xy startpnt,A,B,C,farthest;
  multimap<double,point*>::iterator j,k,inspos,left,right;
  vector<point*> visible; // points of convex hull visible from new point
  int1loop ret;
  ptlist::iterator i;
  vector<double> xsum,ysum;
  maxedges=3*points.size()-6;
  convexhull.clear();
  for (i=points.begin();i!=points.end();i++)
  {
    xsum.push_back(i->second.east());
    ysum.push_back(i->second.north());
  }
  startpnt=xy(pairwisesum(xsum)/xsum.size(),pairwisesum(ysum)/ysum.size());
  for (m=0;m<100;m++)
  {
    outward.clear();
    for (i=points.begin();i!=points.end();i++)
      outward.insert(ipoint(dist(startpnt,i->second),&i->second));
    for (j=outward.begin(),n=0;j!=outward.end();j++,n++)
    {
      if (n==0)
        A=*(j->second);
      if (n==1)
        B=*(j->second);
      if (n==2)
        C=*(j->second);
      farthest=*(j->second);
    }
    //printf("m=%d startpnt=(%f,%f)\n",m,startpnt.east(),startpnt.north());
    if (m>0 && goodcenter(startpnt,A,B,C))
    {
      //printf("m=%d found good center\n",m);
      break;
    }
    if (m&4)
      startpnt=rand2p(startpnt,farthest);
    else
      startpnt=rand2p(startpnt,C);
  }
  j=outward.begin();
  convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
  j++;
  convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
  //printf("edges %d\n",edges.size());
  /* Before:
   * A-----B
   * |    /|
   * |   / |
   * |  /  |
   * | /   |
   * |/    |
   * C-----D
   *
   *          E
   * outward=(D,C,B,A,E)
   * edges=(DC,BD,BC,AB,AC)
   * After:
   * edges=(DC,BD,BC,AB,AC,EC,ED,EB)
   */
  for (j++;j!=outward.end();j++)
  {
    convexhull.insert(ipoint(dir(startpnt,*(j->second)),j->second));
    inspos=convexhull.find(dir(startpnt,*(j->second)));
    /* First find how much the convex hull subtends as seen from the new point,
     * expressed as distance from the startpnt to the line between an old point
     * and the new point.
     */
    mindist=dist(startpnt,*(j->second));
    maxdist=-mindist;
    for (left=inspos,idist=n=0;/*idist>=maxdist && */n<convexhull.size();left--,n++)
    {
      if (left->second==j->second)
        idist=0;
      else
      { // i points to a list of points, j to a map of pointers to points
        idist=pldist(startpnt,*(left->second),*(j->second));
        if (idist>maxdist)
          maxdist=idist;
      }
      if (left==convexhull.begin())
        left=convexhull.end();
    }
    for (right=inspos,idist=n=0;/*idist<=mindist && */n<convexhull.size();right++,n++)
    {
      if (right==convexhull.end())
        right=convexhull.begin();
      if (right->second==j->second)
        idist=0;
      else
      {
        idist=pldist(startpnt,*(right->second),*(j->second));
        if (idist<mindist)
          mindist=idist;
      }
    }
    //printf("maxdist=%f mindist=%f\n",maxdist,mindist);
    /* Then find which convex hull points are the rightmost and leftmost
     * that can be seen from the new point.
     */
    for (left=inspos,idist=n=0;n<2||idist<maxdist;left--,n++)
    {
      if (left->second==j->second)
        idist=0;
      else
        idist=pldist(startpnt,*(left->second),*(j->second));
      if (left==convexhull.begin())
        left=convexhull.end();
    }
    left++;
    for (right=inspos,idist=n=0;n<2||idist>mindist;right++,n++)
    {
      if (right==convexhull.end())
        right=convexhull.begin();
      if (right->second==j->second)
        idist=0;
      else
        idist=pldist(startpnt,*(right->second),*(j->second));
    }
    right--;
    //putchar('\n');
    // Now make a list of the visible points. There are 3 on average.
    visible.clear();
    edgeoff=edges.size();
    for (k=left,n=0,m=1;m;k++,n++,m++)
    {
      if (k==convexhull.end())
        k=convexhull.begin();
      if (k!=inspos) // skip the point just added
        visible.push_back(k->second);
      if (k==right || n==maxedges)
        m=-1;
    }
    val=--n; // subtract one for the point itself
    // Now delete old convex hull points that are now in the interior.
    for (m=1;m<visible.size()-1;m++)
      convexhull.erase(dir(startpnt,*visible[m]));
  }
  for (j=convexhull.begin();j!=convexhull.end();j++)
    ret.push_back(revpoints[j->second]);
  return ret;
}

int pointlist::flipPass(PostScript &ps,bool colorfibaster)
{
  int m,n,e,step;
  e=rng.usrandom()%edges.size();
  step=rng.usrandom();
  do
  {
    step=(step+relprime(edges.size()))%edges.size();
  } while (gcd(step,edges.size())>1);
  for (m=n=0;n<edges.size();n++)
  {
    if (shouldFlip(edges[e]))
    {
      edges[e].flip(this);
      m++;
      //debugdel=0;
      if (e>680 && e<680)
      {
        ps.startpage();
        dumpedges_ps(ps,colorfibaster);
        ps.endpage();
      }
      //debugdel=1;
    }
    e=(e+step)%edges.size();
  }
  debugdel=0;
  if (ps.isOpen())
  {
    ps.startpage();
    dumpedges_ps(ps,colorfibaster);
    //dumpnext_ps();
    ps.endpage();
  }
  //debugdel=1;
  return m;
}

void pointlist::maketin(string filename,bool colorfibaster)
/* Makes a triangulated irregular network. If <3 points, throws noTriangle without altering
 * the existing TIN. If two points are equal, or close enough to likely cause problems,
 * throws samePoints; the TIN is partially constructed and will have to be destroyed.
 */
{
  ptlist::iterator i;
  int m,m2,n,flipcount,passcount,cycles;
  bool fail;
  PostScript ps;
  if (points.size()<3)
    throw noTriangle;
  startpnt=xy(0,0);
  for (i=points.begin();i!=points.end();i++)
    startpnt+=i->second;
  startpnt/=points.size();
  edges.clear();
  splitBreaklines();
  /* startpnt has to be within or out the side of the triangle formed
   * by the three nearest points. In a 100-point asteraceous pattern,
   * the centroid is out one corner, and the first triangle is drawn
   * negative, with point 0 connected wrong.
   */
  startpnt=points.begin()->second;
  if (filename.length())
  {
    ps.open(filename);
    ps.setpaper(papersizes["A4 portrait"],0);
    ps.prolog();
    ps.setPointlist(*this);
  }
  for (m2=0,fail=true;m2<100 && fail;m2++)
    fail=tryStartPoint(ps,startpnt);
  if (fail)
  {
    throw flatTriangle;
    /* Failing to make a proper TIN, after trying a hundred start points,
     * normally means that all triangles are flat.
     */
  }
  if (ps.isOpen())
  {
    ps.startpage();
    dumpedges_ps(ps,colorfibaster);
    //dumpnext_ps();
    ps.dot(startpnt);
    ps.endpage();
  }
  flipcount=passcount=0;
  //debugdel=1;
  /* The flipping algorithm can take quadratic time, but usually does not
   * on real-world data. It can also get stuck in a loop because of roundoff error.
   * This requires at least five points in a perfect circle with nothing else
   * inside. The test datum {ring(1000);rotate(30);} results in flipping edges
   * around 13 points forever. To stop this, I put a cap on the number of passes.
   * The worst cases are ring with lots of rotation and ellipse. They take about
   * 280 passes for 1000 points. I think that a cap of 1 pass per 3 points is
   * reasonable.
   */
  flipcount=passcount=0;
  do
  {
    flipcount+=m=flipPass(ps,colorfibaster);
    passcount++;
  } while (m && passcount*3<=points.size());
  //printf("Total %d edges flipped in %d passes\n",flipcount,passcount);
  if (ps.isOpen())
  {
    ps.startpage();
    dumpedges_ps(ps,colorfibaster);
    ps.dot(startpnt);
    ps.endpage();
    ps.trailer();
    ps.close();
  }
}

void pointlist::makegrad(double corr)
// Compute the gradient at each point.
// corr is a correlation factor which is how much the slope
// at one end of an edge affects the slope at the other.
{
  ptlist::iterator i;
  int n,m;
  edge *e;
  double zdiff,zxtrap,zthere;
  xy gradthere,diff;
  double sum1,sumx,sumy,sumz,sumxx,sumxy,sumxz,sumzz,sumyy,sumyz;
  for (i=points.begin();i!=points.end();i++)
    i->second.gradient=xy(0,0);
  for (n=0;n<10;n++)
  {
    for (i=points.begin();i!=points.end();i++)
    {
      //i->second.gradient=xy(0,0);
      sum1=sumx=sumy=sumz=sumxx=sumxy=sumxz=sumzz=sumyy=sumyz=0;
      for (m=0,e=i->second.line;m==0 || e!=i->second.line;m++,e=e->next(&i->second))
      if (!(e->broken&8))
      {
	gradthere=e->otherend(&i->second)->gradient;
	diff=(xy)(*e->otherend(&i->second))-(xy)i->second;
	zdiff=e->otherend(&i->second)->elev()-i->second.elev();
	zxtrap=zdiff-dot(gradthere,diff);
	zthere=zdiff+corr*zxtrap;
	sum1+=1;
	sumx+=diff.east();
	sumy+=diff.north();
	sumz+=zthere;
	sumxx+=diff.east()*diff.east();
	sumyy+=diff.north()*diff.north();
	sumzz+=zthere*zthere;
	sumxy+=diff.east()*diff.north();
	sumxz+=diff.east()*zthere;
	sumyz+=diff.north()*zthere;
      }
      //printf("point %d sum1=%f zdiff=%f zthere=%f\n",i->first,sum1,zdiff,zthere);
      if (sum1)
      {
	sum1++; //add the point i to the set
	sumx/=sum1;
	sumy/=sum1;
	sumz/=sum1;
	sumxx/=sum1;
	sumyy/=sum1;
	sumzz/=sum1;
	sumxy/=sum1;
	sumxz/=sum1;
	sumyz/=sum1;
	sumxx-=sumx*sumx;
	sumyy-=sumy*sumy;
	sumzz-=sumz*sumz;
	sumxy-=sumx*sumy;
	sumxz-=sumx*sumz;
	sumyz-=sumy*sumz;
	/* Gradient is computed by this matrix equation:
	(xx xy)   (gradx)
	(     ) × (     ) = (xz yz)
	(xy yy)   (grady) */
	i->second.newgradient=xy(sumxz/sumxx,sumyz/sumyy);
	/*if (i->first==63)
	printf("sumxz %f sumxx %f sumyz %f sumyy %f\n",sumxz,sumxx,sumyz,sumyy);*/
      }
      else
	fprintf(stderr,"Warning: point at address %p has no edges that don't cross breaklines\n",&i->second);
    }
    for (i=points.begin();i!=points.end();i++)
    {
      i->second.oldgradient=i->second.gradient;
      i->second.gradient=i->second.newgradient;
    }
  }
}

void pointlist::maketriangles()
/* The TIN consisting of points and edges, but no triangles, has been made.
 * Add the triangles.
 */
{
  int i,j;
  point *a,*b,*c,*d;
  edge *e;
  triangle cib,*t;
  triangles.clear();
  for (i=0;i<edges.size();i++)
  {
    a=edges[i].a;
    b=edges[i].b;
    e=edges[i].next(b);
    edges[i].tria=edges[i].trib=NULL; // otherwise they may point to a triangle that no longer exists, causing a crash
    c=e->otherend(b);
    d=e->next(c)->otherend(c);
    if (a<b && a<c && a==d)
    {
      cib.a=c;
      cib.b=b;
      cib.c=a;
      if (cib.area()>=0)
      {
        cib.peri=cib.perimeter();
        triangles[triangles.size()]=cib;
        edges[i].tria=&triangles[triangles.size()-1];
      }
    }
    a=edges[i].b;
    b=edges[i].a;
    e=edges[i].next(b);
    c=e->otherend(b);
    d=e->next(c)->otherend(c);
    if (a<b && a<c && a==d)
    {
      cib.a=c;
      cib.b=b;
      cib.c=a;
      if (cib.area()>=0)
      {
	cib.peri=cib.perimeter();
        triangles[triangles.size()]=cib;
        edges[i].trib=&triangles[triangles.size()-1];
      }
    }
  }
  for (i=0;i<edges.size()*2;i++)
  {
    j=i%edges.size();
    if (!edges[j].tria)
      edges[j].tria=edges[j].nextb->tri(edges[j].b);
    if (!edges[j].trib)
      edges[j].trib=edges[j].nexta->tri(edges[j].a);
  }
  for (i=0;i<edges.size();i++)
    edges[i].setNeighbors();
}

void pointlist::makeBareTriangles(vector<array<xyz,3> > bareTriangles)
/* Assigns point numbers to the corners of the triangles. Makes a qindex and
 * a map of triangles, but no edges. Can throw samePoints.
 */
{
  int i,j;
  vector<xy> corners;
  point *pont[3];
  triangle newtri;
  clear();
  for (i=0;i<bareTriangles.size();i++)
    for (j=0;j<3;j++)
      corners.push_back(bareTriangles[i][j]);
  qinx.sizefit(corners);
  qinx.split(corners);
  for (i=0;i<bareTriangles.size();i++)
  {
    if (area3(bareTriangles[i][0],bareTriangles[i][1],bareTriangles[i][2])<0)
      swap(bareTriangles[i][0],bareTriangles[i][2]);
    for (j=0;j<3;j++)
    {
      pont[j]=qinx.findp(bareTriangles[i][j]);
      if (!pont[j])
      {
	addpoint(1,point(bareTriangles[i][j],""));
	pont[j]=&points[points.size()];
	qinx.insertPoint(pont[j]);
      }
    }
    newtri.a=pont[0];
    newtri.b=pont[1];
    newtri.c=pont[2];
    newtri.flatten();
    triangles[triangles.size()]=newtri;
  }
  qinx.clearLeaves();
}

void pointlist::triangulatePolygon(vector<point *> poly)
/* Given a polygon, triangulates it, adding the triangles to pointlist::triangles.
 * The polygon results from reading in a TIN as bare triangles. It is the space
 * between the triangles and their convex hull, or enclosed by the triangles
 * if not simply connected.
 */
{
  int h,i,j,a,b,c,ai,bi,ci,sz=poly.size(),ba,bb,bc,isInside;
  vector<point *> subpoly;
  vector<double> coords;
  multimap<double,int> outwardMap;
  vector<int> outwardVec;
  multimap<double,int>::iterator k;
  double xmean,ymean;
  xy startpnt;
  bool found=false;
  triangle newtri;
  for (i=0;i<sz;i++)
    coords.push_back(poly[i]->getx());
  xmean=pairwisesum(coords);
  coords.clear();
  for (i=0;i<sz;i++)
    coords.push_back(poly[i]->gety());
  ymean=pairwisesum(coords);
  coords.clear();
  startpnt=xy(xmean,ymean);
  while (true)
  {
    for (isInside=i=0;i<sz;i++)
      isInside+=foldangle(dir(startpnt,*poly[(i+1)%sz])-dir(startpnt,*poly[i]));
    if (isInside)
      break;
    startpnt=(startpnt+*poly[rng.usrandom()%sz])/2;
  }
  for (i=0;i<sz;i++)
    outwardMap.insert(pair<double,int>(dist(startpnt,*poly[i]),i));
  for (k=outwardMap.begin();k!=outwardMap.end();++k)
    outwardVec.push_back(k->second);
  outwardMap.clear();
  assert(outwardVec.size()==sz);
  for (ai=0;ai<sz && !found;ai++)
    for (bi=0;bi<ai && !found;bi++)
      for (ci=0;ci<bi && !found;ci++)
      {
	a=outwardVec[ai];
	b=outwardVec[bi];
	c=outwardVec[ci];
	if ((b+sz-a)%sz+(c+sz-b)%sz>sz)
	  swap(b,c);
	ba=dir(xy(*poly[b]),xy(*poly[c]));
	bb=dir(xy(*poly[c]),xy(*poly[a]));
	bc=dir(xy(*poly[a]),xy(*poly[b]));
	if (area3(*poly[a],*poly[b],*poly[c])<=0 ||
	    abs(foldangle(ba-bb+DEG180))<2 ||
	    abs(foldangle(bb-bc+DEG180))<2 ||
	    abs(foldangle(bc-ba+DEG180))<2)
	  continue;
	found=true;
	for (i=0;found && i<sz;i++)
	{
	  if (i!=a && i!=b && i!=c)
	  {
	    if (in3(*poly[i],*poly[a],*poly[b],*poly[c]))
	      found=false;
	    ba=dir(xy(*poly[i]),xy(*poly[a]));
	    bb=dir(xy(*poly[i]),xy(*poly[b]));
	    bc=dir(xy(*poly[i]),xy(*poly[c]));
	    if (abs(foldangle(ba-bb+DEG180))<2 ||
		abs(foldangle(bb-bc+DEG180))<2 ||
		abs(foldangle(bc-ba+DEG180))<2)
	      found=false;
	  }
	  if (crossTriangle(*poly[i],*poly[(i+1)%sz],*poly[a],*poly[b],*poly[c]))
	    found=false;
	}
      }
  if (found)
  {
    newtri.a=poly[a];
    newtri.b=poly[b];
    newtri.c=poly[c];
    newtri.flatten();
    triangles[triangles.size()]=newtri;
    for (i=a;i!=b;i=(i+1)%sz)
      subpoly.push_back(poly[i]);
    subpoly.push_back(poly[b]);
    triangulatePolygon(subpoly);
    subpoly.clear();
    for (i=b;i!=c;i=(i+1)%sz)
      subpoly.push_back(poly[i]);
    subpoly.push_back(poly[c]);
    triangulatePolygon(subpoly);
    subpoly.clear();
    for (i=c;i!=a;i=(i+1)%sz)
      subpoly.push_back(poly[i]);
    subpoly.push_back(poly[a]);
    triangulatePolygon(subpoly);
  }
}

void pointlist::makeEdges()
/* The points and triangles are present, but the edges are not, or some
 * triangles have been added to make the TIN convex, but their edges haven't.
 * Add the edges.
 */
{
  int i;
  edge newedge;
  edge *edg;
  for (i=0;i<triangles.size();i++)
  {
    if (triangles[i].sarea<1e-6)
      cerr<<"tiny triangle "<<triangles[i].a<<' '<<triangles[i].b<<' '<<triangles[i].c<<'\n';
    if (!triangles[i].a->isNeighbor(triangles[i].b))
    {
      newedge.a=triangles[i].a;
      newedge.b=triangles[i].b;
      edges[edges.size()]=newedge;
      triangles[i].a->insertEdge(&edges[edges.size()-1]);
      triangles[i].b->insertEdge(&edges[edges.size()-1]);
    }
    if (!triangles[i].b->isNeighbor(triangles[i].c))
    {
      newedge.a=triangles[i].b;
      newedge.b=triangles[i].c;
      edges[edges.size()]=newedge;
      triangles[i].b->insertEdge(&edges[edges.size()-1]);
      triangles[i].c->insertEdge(&edges[edges.size()-1]);
    }
    if (!triangles[i].c->isNeighbor(triangles[i].a))
    {
      newedge.a=triangles[i].c;
      newedge.b=triangles[i].a;
      edges[edges.size()]=newedge;
      triangles[i].c->insertEdge(&edges[edges.size()-1]);
      triangles[i].a->insertEdge(&edges[edges.size()-1]);
    }
    edg=triangles[i].a->isNeighbor(triangles[i].b);
    if (edg->a==triangles[i].a)
      edg->trib=&triangles[i];
    else
      edg->tria=&triangles[i];
    edg->setNeighbors();
    edg=triangles[i].b->isNeighbor(triangles[i].c);
    if (edg->a==triangles[i].b)
      edg->trib=&triangles[i];
    else
      edg->tria=&triangles[i];
    edg->setNeighbors();
    edg=triangles[i].c->isNeighbor(triangles[i].a);
    if (edg->a==triangles[i].c)
      edg->trib=&triangles[i];
    else
      edg->tria=&triangles[i];
    edg->setNeighbors();
  }
}

void pointlist::fillInBareTin()
/* Call this after makeBareTriangles or reading in a TIN from a .bez file.
 * It makes edges, fills in any gaps with arbitrary triangles, makes edges
 * again, and makes the quadtree index. The result is a convex TIN with
 * a quad index, just as if it were made with maketin (but the filled-in areas
 * are unlikely to be Delaunay).
 */
{
  intloop holes;
  int i;
  makeEdges();
  holes=boundary();
  holes.push_back(convexHull());
  holes.consolidate();
  for (i=0;i<holes.size();i++)
    triangulatePolygon(fromInt1loop(holes[i]));
  makeEdges();
  updateqindex();
}

double pointlist::totalEdgeLength()
{
  vector<double> edgeLengths;
  map<int,edge>::iterator i;
  for (i=edges.begin();i!=edges.end();i++)
    edgeLengths.push_back(i->second.length());
  return pairwisesum(edgeLengths);
}

array<double,2> pointlist::lohi()
{
  int i;
  array<double,2> ret;
  array<double,4> tlohi;
  ret[0]=INFINITY;
  ret[1]=-INFINITY;
  for (i=0;i<triangles.size();i++)
  {
    tlohi=triangles[i].lohi();
    if (ret[0]>tlohi[0])
      ret[0]=tlohi[0];
    if (ret[1]<tlohi[3])
      ret[1]=tlohi[3];
  }
  return ret;
}
