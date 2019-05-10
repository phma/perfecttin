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

int main(int argc, char *argv[])
{
  PostScript ps;
  BoundRect br;
  int i;
  readPly("pc.ply");
  ps.open("decisite.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  ps.startpage();
  for (i=0;i<cloud.size();i++)
    br.include(cloud[i]);
  ps.setscale(br);
  for (i=0;i<cloud.size();i++)
    ps.dot(cloud[i]);
  ps.endpage();
  ps.close();
  return 0;
}
