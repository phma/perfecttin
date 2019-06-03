/******************************************************/
/*                                                    */
/* edgeop.cpp - edge operation                        */
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

#include <cstring>
#include <cassert>
#include "tin.h"
#include "angle.h"
#include "edgeop.h"
#include "octagon.h"
#include "triop.h"
#include "neighbor.h"
#include "adjelev.h"

using namespace std;

vector<pointlist> tempPointlist;
/* Small pointlists, 5 points and 4 triangles, used for deciding whether to
 * flip an edge. One per worker thread.
 */

void initTempPointlist(int nthreads)
{
  tempPointlist.resize(nthreads);
}

void flip(edge *e)
{
  vector<xyz> allDots;
  int i;
  // lock
  e->flip(&net);
  //assert(net.checkTinConsistency());
  allDots.resize(e->tria->dots.size()+e->trib->dots.size());
  memmove(&allDots[0],&e->tria->dots[0],e->tria->dots.size()*sizeof(xyz));
  memmove(&allDots[e->tria->dots.size()],&e->trib->dots[0],e->trib->dots.size()*sizeof(xyz));
  e->tria->dots.clear();
  e->trib->dots.clear();
  for (i=0;i<allDots.size();i++)
    if (e->tria->in(allDots[i]))
      e->tria->dots.push_back(allDots[i]);
    else
      e->trib->dots.push_back(allDots[i]);
  e->tria->dots.shrink_to_fit();
  e->trib->dots.shrink_to_fit();
  e->tria->flatten();
  e->trib->flatten();
  // unlock
}

point *bend(edge *e)
/* Inserts a new point, bending and breaking the edge e of the perimeter,
 * then flips the edge e.
 */
{
  edge *anext=e,*bnext=e;
  int abear,ebear,bbear;
  // lock
  do
    anext=anext->next(e->a);
  while (anext->isinterior());
  do
    bnext=bnext->next(e->b);
  while (bnext->isinterior());
  abear=anext->bearing(e->a);
  ebear=e->bearing(e->a);
  bbear=bnext->bearing(e->b);
  while (abs(abear-ebear)>DEG90)
    abear+=DEG180;
  while (abs(bbear-ebear)>DEG90)
    bbear+=DEG180;
  abear=ebear+(abear-ebear)/2;
  bbear=ebear+(bbear-ebear)/2;
  point newPoint(intersection(*e->a,abear,*e->b,bbear),0);
  int newPointNum=net.points.size()+1;
  net.addpoint(newPointNum,newPoint);
  point *pnt=&net.points[newPointNum];
  int newEdgeNum=net.edges.size();
  net.edges[newEdgeNum  ].a=e->a;
  net.edges[newEdgeNum  ].b=pnt;
  net.edges[newEdgeNum+1].a=e->b;
  net.edges[newEdgeNum+1].b=pnt;
  pnt->line=&net.edges[newEdgeNum];
  net.edges[newEdgeNum  ].setnext(pnt,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(pnt,&net.edges[newEdgeNum  ]);
  int newTriNum=net.triangles.size();
  net.triangles[newTriNum].a=pnt;
  // If abear-bbear<0, then e is counterclockwise around the TIN.
  if (abear-bbear<0)
  {
    net.triangles[newTriNum].b=e->b;
    net.triangles[newTriNum].c=e->a;
    anext->setnext(e->a,&net.edges[newEdgeNum]);
    net.edges[newEdgeNum  ].setnext(e->a,e);
    e->setnext(e->b,&net.edges[newEdgeNum+1]);
    net.edges[newEdgeNum+1].setnext(e->b,bnext);
    e->tria=&net.triangles[newTriNum];
    net.edges[newEdgeNum  ].trib=&net.triangles[newTriNum];
    net.edges[newEdgeNum+1].tria=&net.triangles[newTriNum];
  }
  else
  {
    net.triangles[newTriNum].b=e->a;
    net.triangles[newTriNum].c=e->b;
    bnext->setnext(e->b,&net.edges[newEdgeNum+1]);
    net.edges[newEdgeNum+1].setnext(e->b,e);
    e->setnext(e->a,&net.edges[newEdgeNum]);
    net.edges[newEdgeNum  ].setnext(e->a,anext);
    e->trib=&net.triangles[newTriNum];
    net.edges[newEdgeNum  ].tria=&net.triangles[newTriNum];
    net.edges[newEdgeNum+1].trib=&net.triangles[newTriNum];
  }
  e->setNeighbors();
  //assert(net.checkTinConsistency());
  // unlock
  flip(e);
  return pnt;
}

bool shouldFlip(edge *e,int thread)
/* Decides whether to flip an interior edge. If it is flippable (that is,
 * the two triangles form a convex quadrilateral), it decides based on
 * a combination of three criteria:
 * 1. They would fit the dots better after flipping than before.
 * 2. The number of dots on each side would be better balanced after flipping.
 * 3. The edge would be shorter after flipping.
 */
{
  int i,j;
  array<triangle *,2> triab;
  triangle *tri;
  bool validTemp,ret=false;
  double elev13,elev24,elev5;
  double crit1,crit2,crit3;
  double areas[4];
  int ndots[4];
  vector<triangle *> alltris;
  vector<point *> allpoints;
  tempPointlist[thread].clear();
  tempPointlist[thread].addpoint(1,*e->a);
  tempPointlist[thread].addpoint(2,*e->nexta->otherend(e->a));
  tempPointlist[thread].addpoint(3,*e->b);
  tempPointlist[thread].addpoint(4,*e->nextb->otherend(e->b));
  tempPointlist[thread].addpoint(5,point(intersection(*e->a,*e->b,
			  *e->nextb->otherend(e->b),*e->nexta->otherend(e->a)),0));
  for (i=0;i<4;i++)
  {
    tempPointlist[thread].edges[i].a=&tempPointlist[thread].points[i+1];
    tempPointlist[thread].edges[i].b=&tempPointlist[thread].points[(i+1)%4+1];
    tempPointlist[thread].edges[i+4].a=&tempPointlist[thread].points[i+1];
    tempPointlist[thread].edges[i+4].b=&tempPointlist[thread].points[5];
  }
  for (i=0;i<4;i++)
  {
    tempPointlist[thread].edges[i].nexta=&tempPointlist[thread].edges[(i+3)%4];
    tempPointlist[thread].edges[i].nextb=&tempPointlist[thread].edges[(i+1)%4+4];
    tempPointlist[thread].edges[i+4].nexta=&tempPointlist[thread].edges[i];
    tempPointlist[thread].edges[i+4].nextb=&tempPointlist[thread].edges[(i+3)%4+4];
  }
  for (i=1;i<6;i++)
    tempPointlist[thread].points[i].line=&tempPointlist[thread].edges[(i-1)%4+4];
  tempPointlist[thread].maketriangles();
  validTemp=tempPointlist[thread].checkTinConsistency();
  /*
   *           2
   *         / | \
   *      0/   5   \1
   *     /  0  |  1  \
   *   /       |       \
   * 1----4----5----6----3
   *   \       |       /
   *     \  3  |  2  /
   *      3\   7   /2
   *         \ | /
   *           4
   * Line 1-3 is the edge before flipping; line 2-4 is what it would be after.
   * It is possible for valid splittings to produce an invalid temporary pointlist.
   * Split △ABC at D, then split △ABD at E. C, D, and E are collinear. Then try
   * to flip BD. The temporary pointlist looks like this:
   * 2
   * | \
   * |   \
   * |  1  \
   * |       \
   * 1=5-------3
   * |       /
   * |  2  /
   * |   /
   * | /
   * 4
   * Because of roundoff error, it may appear to be flippable, but in fact is not.
   */
  if (validTemp)
  {
    for (i=0;i<4;i++)
      areas[i]=tempPointlist[thread].triangles[i].area();
    if (areas[0]+areas[3]>3e8*(areas[1]+areas[2]) ||
        areas[2]+areas[1]>3e8*(areas[3]+areas[0]))
      validTemp=false;
  }
  triab[0]=e->tria;
  triab[1]=e->trib;
  if (validTemp)
  {
    tri=&tempPointlist[thread].triangles[0];
    for (i=0;i<2;i++)
      for (j=0;j<triab[i]->dots.size();j++)
      {
	tri=tri->findt(triab[i]->dots[j]);
	tri->dots.push_back(triab[i]->dots[j]);
      }
    for (i=1;i<6;i++)
      allpoints.push_back(&tempPointlist[thread].points[i]);
    for (i=0;i<4;i++)
      alltris.push_back(&tempPointlist[thread].triangles[i]);
    if (adjustElev(alltris,allpoints).validMatrix)
    {
      elev13=(tempPointlist[thread].points[1].elev()*tempPointlist[thread].edges[6].length()+
	      tempPointlist[thread].points[3].elev()*tempPointlist[thread].edges[4].length())/
	     (tempPointlist[thread].edges[4].length()+tempPointlist[thread].edges[6].length());
      elev24=(tempPointlist[thread].points[2].elev()*tempPointlist[thread].edges[7].length()+
	      tempPointlist[thread].points[4].elev()*tempPointlist[thread].edges[5].length())/
	     (tempPointlist[thread].edges[5].length()+tempPointlist[thread].edges[7].length());
      elev5=tempPointlist[thread].points[5].elev();
    }
    else
      elev13=elev24=elev5=0;
    if (elev13==elev24)
      crit1=0;
    else
      crit1=(fabs(elev13-elev5)-fabs(elev24-elev5))/(fabs(elev13-elev5)+fabs(elev24-elev5));
    for (i=0;i<4;i++)
      ndots[i]=tempPointlist[thread].triangles[i].dots.size();
    crit2=(abs(ndots[0]+ndots[1]-ndots[2]-ndots[3])-abs(ndots[0]-ndots[1]-ndots[2]+ndots[3]))/
          (ndots[0]+ndots[1]+ndots[2]+ndots[3]+1.);
    crit3=(tempPointlist[thread].edges[4].length()-
           tempPointlist[thread].edges[5].length()+
	   tempPointlist[thread].edges[6].length()-
	   tempPointlist[thread].edges[7].length())/
	  (tempPointlist[thread].edges[4].length()+
           tempPointlist[thread].edges[5].length()+
	   tempPointlist[thread].edges[6].length()+
	   tempPointlist[thread].edges[7].length());
    ret=crit1+crit2+crit3>0;
  }
  return ret;
}

bool shouldBend(edge *e,double tolerance)
{
  if (e->tria)
    return shouldSplit(e->tria,tolerance);
  else
    return shouldSplit(e->trib,tolerance);
}

void edgeop(edge *e,double tolerance,int thread)
{
  vector<point *> corners;
  corners.push_back(e->a);
  corners.push_back(e->b);
  if (e->tria)
    corners.push_back(e->nextb->otherend(e->b));
  if (e->trib)
    corners.push_back(e->nexta->otherend(e->a));
  if (e->isinterior())
    if (e->isFlippable() && shouldFlip(e,thread))
      flip(e);
    else;
  else
    if (shouldBend(e,tolerance))
      corners.push_back(bend(e));
  logAdjustment(adjustElev(triangleNeighbors(corners),corners));
  if (e->tria && e->tria->sarea<1e-6)
    cout<<"tiny triangle a\n";
  if (e->trib && e->trib->sarea<1e-6)
    cout<<"tiny triangle b\n";
}
