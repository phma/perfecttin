/******************************************************/
/*                                                    */
/* triop.cpp - triangle operation                     */
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

#include <cassert>
#include "octagon.h"
#include "triop.h"

using namespace std;

void split(triangle *tri)
/* Inserts a new point in the triangle, then moves the dots that are in the
 * two new triangles to them.
 */
{
  edge *sidea,*sideb,*sidec;
  vector<xyz> remainder; // the dots that remain in tri
  int i;
  // lock
  point newPoint(((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3);
  int newPointNum=net.points.size()+1;
  net.addpoint(newPointNum,newPoint);
  point *pnt=&net.points[newPointNum];
  int newEdgeNum=net.edges.size();
  net.edges[newEdgeNum].a=pnt;
  net.edges[newEdgeNum].b=tri->a;
  net.edges[newEdgeNum+1].a=pnt;
  net.edges[newEdgeNum+1].b=tri->b;
  net.edges[newEdgeNum+2].a=pnt;
  net.edges[newEdgeNum+2].b=tri->c;
  pnt->line=&net.edges[newEdgeNum];
  sidea=tri->c->edg(tri);
  sideb=tri->a->edg(tri);
  sidec=tri->b->edg(tri);
  net.edges[newEdgeNum  ].setnext(pnt,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(pnt,&net.edges[newEdgeNum+2]);
  net.edges[newEdgeNum+2].setnext(pnt,&net.edges[newEdgeNum  ]);
  sidea->setnext(tri->b,&net.edges[newEdgeNum+1]);
  net.edges[newEdgeNum+1].setnext(tri->b,sidec);
  sidec->setnext(tri->a,&net.edges[newEdgeNum  ]);
  net.edges[newEdgeNum  ].setnext(tri->b,sideb);
  sideb->setnext(tri->c,&net.edges[newEdgeNum+2]);
  net.edges[newEdgeNum+2].setnext(tri->b,sidea);
  int newTriNum=net.triangles.size();
  net.triangles[newTriNum].a=tri->a;
  net.triangles[newTriNum].b=tri->b;
  net.triangles[newTriNum].c=pnt;
  net.triangles[newTriNum+1].a=tri->a;
  net.triangles[newTriNum+1].b=pnt;
  net.triangles[newTriNum+1].c=tri->c;
  tri->a=pnt;
  net.edges[newEdgeNum  ].tria=net.edges[newEdgeNum+2].trib=&net.triangles[newTriNum+1];
  net.edges[newEdgeNum+1].tria=net.edges[newEdgeNum  ].trib=&net.triangles[newTriNum];
  net.edges[newEdgeNum+2].tria=net.edges[newEdgeNum+1].trib=tri;
  if (sideb->tria==tri)
    sideb->tria=&net.triangles[newTriNum+1];
  if (sideb->trib==tri)
    sideb->trib=&net.triangles[newTriNum+1];
  if (sidec->tria==tri)
    sidec->tria=&net.triangles[newTriNum];
  if (sidec->trib==tri)
    sidec->trib=&net.triangles[newTriNum];
  sidea->setNeighbors();
  sideb->setNeighbors();
  sidec->setNeighbors();
  net.edges[newEdgeNum  ].setNeighbors();
  net.edges[newEdgeNum+1].setNeighbors();
  net.edges[newEdgeNum+2].setNeighbors();
  assert(net.checkTinConsistency());
  // unlock
}
