/******************************************************/
/*                                                    */
/* neighbor.cpp - triangle neighbors of points etc.   */
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

#include <set>
#include "neighbor.h"
#include "tin.h"
#include "threads.h"
#include "octagon.h"

using namespace std;

vector<triangle *> triangleNeighbors(vector<point *> corners)
{
  vector<triangle *> ret;
  set<triangle *> tmpRet;
  edge *ed,*ed0;
  int i,j;
  set<triangle *>::iterator k;
  net.wingEdge.lock_shared();
  for (i=0;i<corners.size();i++)
  {
    for (ed=ed0=corners[i]->line,j=0;j<net.edges.size() && (ed!=ed0 || j==0);ed=ed->next(corners[i]),j++)
      tmpRet.insert(ed->tri(corners[i]));
    if (ed!=ed0)
      cerr<<"Winged edge corruption\n";
  }
  for (k=tmpRet.begin();k!=tmpRet.end();k++)
    if (*k!=nullptr)
      ret.push_back(*k);
  net.wingEdge.unlock_shared();
  return ret;
}

vector<edge *> edgeNeighbors(vector<triangle *> triangles)
{
  vector<edge *> ret;
  set<edge *> tmpRet;
  int i;
  set<edge *>::iterator k;
  net.wingEdge.lock_shared();
  for (i=0;i<triangles.size();i++)
  {
    tmpRet.insert(triangles[i]->a->edg(triangles[i]));
    tmpRet.insert(triangles[i]->b->edg(triangles[i]));
    tmpRet.insert(triangles[i]->c->edg(triangles[i]));
  }
  for (k=tmpRet.begin();k!=tmpRet.end();k++)
    if (*k!=nullptr)
      ret.push_back(*k);
  net.wingEdge.unlock_shared();
  return ret;
}

vector<point *> pointNeighbors(vector<triangle *> triangles)
{
  vector<point *> ret;
  set<point *> tmpRet;
  int i;
  set<point *>::iterator k;
  net.wingEdge.lock_shared();
  for (i=0;i<triangles.size();i++)
  {
    tmpRet.insert(triangles[i]->a);
    tmpRet.insert(triangles[i]->b);
    tmpRet.insert(triangles[i]->c);
  }
  for (k=tmpRet.begin();k!=tmpRet.end();k++)
    if (*k!=nullptr)
      ret.push_back(*k);
  net.wingEdge.unlock_shared();
  return ret;
}
