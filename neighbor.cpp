/******************************************************/
/*                                                    */
/* neighbor.cpp - triangle neighbors of points        */
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
  edge *ed;
  int i,j;
  set<triangle *>::iterator k;
  wingEdge.lock();
  for (i=0;i<corners.size();i++)
  {
    for (ed=corners[i]->line,j=0;j<net.edges.size() && (ed!=corners[i]->line || j==0);ed=ed->next(corners[i]),j++)
      tmpRet.insert(ed->tri(corners[i]));
    if (ed!=corners[i]->line)
      cerr<<"Winged edge corruption\n";
  }
  for (k=tmpRet.begin();k!=tmpRet.end();k++)
    if (*k!=nullptr)
      ret.push_back(*k);
  wingEdge.unlock();
  return ret;
}
