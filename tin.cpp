/******************************************************/
/*                                                    */
/* tin.cpp - triangulated irregular network           */
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

#include <map>
#include <cmath>
#include <iostream>
#include <cassert>
#include "tin.h"
#include "ps.h"
#include "point.h"
#include "pointlist.h"
#include "ldecimal.h"
#include "manysum.h"
#include "random.h"
#include "relprime.h"
#include "lohi.h"
#include "neighbor.h"

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
  else
  {
    if (tria)
      tria->setnoneighbor(this);
    if (trib)
      trib->setnoneighbor(this);
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
  if (nexta==nullptr || nextb==nullptr)
    return false;
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
  return ret;
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

void pointlist::maketriangles()
/* The TIN consisting of points and edges, but no triangles, has been made.
 * Add the triangles.
 */
{
  int i,j;
  point *a,*b,*c,*d;
  edge *e;
  triangle cib;
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
  array<double,2> tlohi;
  ret[0]=INFINITY;
  ret[1]=-INFINITY;
  for (i=0;i<triangles.size();i++)
  {
    tlohi=triangles[i].lohi();
    if (ret[0]>tlohi[0])
      ret[0]=tlohi[0];
    if (ret[1]<tlohi[1])
      ret[1]=tlohi[1];
  }
  return ret;
}

array<double,2> pointlist::lohi(polyline p,double tolerance)
/* Returns the lowest and highest elevations on or within the polyline.
 * Used when pruning or smoothing a contour to avoid crossing a point that is
 * outside the contour's tolerance. The polyline must be closed.
 */
{
  int i,sz=p.size(),nPoints=0;
  array<double,2> ret;
  array<double,2> tlohi;
  triangle *tri=findt(p.getEndpoint(0));
  segment seg;
  vector<triangle *> borderTriangles,overlapTriangles;
  vector<point *> neighborPoints,insidePoints;
  ret[0]=INFINITY;
  ret[1]=-INFINITY;
  for (i=0;i<sz && ret[1]-ret[0]<2*tolerance;i++)
  {
    seg=p.getsegment(i);
    if (!tri)
      tri=findt(p.getEndpoint(i),true);
    do
    {
      tlohi=tri->lohi(seg);
      updlohi(ret,tlohi);
      borderTriangles.push_back(tri);
      tri=tri->nextalong(seg);
    } while (tri && !tri->in(p.getEndpoint(i+1)));
  }
  if (ret[1]-ret[0]<2*tolerance)
    do
    {
      nPoints=insidePoints.size();
      overlapTriangles=triangleNeighbors(insidePoints);
      for (i=0;i<borderTriangles.size();i++)
	overlapTriangles.push_back(borderTriangles[i]);
      neighborPoints=pointNeighbors(overlapTriangles);
      insidePoints.clear();
      for (i=0;i<neighborPoints.size();i++)
	if (p.in(*neighborPoints[i]))
	  insidePoints.push_back(neighborPoints[i]);
    } while (nPoints!=insidePoints.size());
  for (i=0;i<nPoints;i++)
    updlohi(ret,insidePoints[i]->elev());
  return ret;
}

double pointlist::contourError(segment seg)
/* Computes the integral along the segment of the square of the difference
 * between the TIN's elevation and the segment's elevation. The return value
 * has dimensions of volume. Used when smoothing contours.
 */
{
  triangle *tri=findt(seg.getstart(),true);
  segment side[3];
  xy intpt;
  double along,lastAlong=0,lastError=0;
  map<double,double> alongError;
  map<double,double>::iterator it;
  vector<double> pieces;
  int i;
  alongError[0]=tri->elevation(seg.getstart())-seg.getstart().elev();
  do
  {
    side[0]=tri->a->edg(tri)->getsegment();
    side[1]=tri->b->edg(tri)->getsegment();
    side[2]=tri->c->edg(tri)->getsegment();
    for (i=0;i<3;i++)
      if (intersection_type(seg,side[i])!=NOINT)
      {
	intpt=intersection(seg,side[i]);
	along=dist(seg.getstart(),intpt);
	alongError[along]=tri->elevation(intpt)-seg.elev(along);
      }
    tri=tri->nexttoward(seg.getend());
  } while (tri && !tri->in(seg.getend()));
  /* If tri is null, seg is the end of an open contour whose endpoint is just
   * outside the TIN because of roundoff error, and it already put the error
   * in alongError because of the intersection.
   */
  if (tri)
    alongError[seg.length()]=tri->elevation(seg.getend())-seg.getend().elev();
  for (it=alongError.begin();it!=alongError.end();++it)
  {
    /* e.g. You're at 5, error is 3. You were last at 4, error is 7.
     * The square error goes from 9 to 49 and is 25 halfway through.
     * Compute (5-4)*(9+100+49)/6=26+1/3.
     */
    pieces.push_back((it->first-lastAlong)*
		     (sqr(lastError)+sqr(lastError+it->second)+sqr(it->second))/6);
    lastAlong=it->first;
    lastError=it->second;
  }
  return pairwisesum(pieces);
}
