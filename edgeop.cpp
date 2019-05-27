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
using namespace std;

vector<pointlist> tempPointlist;
/* Small pointlists, 5 points and 4 triangles, used for deciding whether to
 * flip an edge. One per worker thread.
 */

void flip(edge *e)
{
  vector<xyz> allDots;
  int i;
  // lock
  e->flip(&net);
  assert(net.checkTinConsistency());
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
  // unlock
}

void bend(edge *e)
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
  assert(net.checkTinConsistency());
  // unlock
  flip(e);
}
