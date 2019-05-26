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
}
