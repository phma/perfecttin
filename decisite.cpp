/******************************************************/
/*                                                    */
/* decisite.cpp - main program                        */
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
#include "ply.h"
#include "ps.h"
#include "octagon.h"

using namespace std;

int main(int argc, char *argv[])
{
  PostScript ps;
  BoundRect br;
  int i,j;
  readPly("pc.ply");
  cout<<"Read "<<cloud.size()<<" points\n";
  makeOctagon();
  ps.open("decisite.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  ps.startpage();
  br.include(&net);
  ps.setscale(br);
  ps.setcolor(0,0,1);
  for (i=0;i<net.edges.size();i++)
    ps.line(net.edges[i],i,false,false);
  ps.setcolor(0,0,0);
  for (i=0;i<net.triangles.size();i++)
    for (j=0;j*j<net.triangles[i].dots.size();j++)
      ps.dot(net.triangles[i].dots[j*j]);
  ps.endpage();
  ps.close();
  return 0;
}
