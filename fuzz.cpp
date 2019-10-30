/******************************************************/
/*                                                    */
/* fuzz.cpp - fuzzing main program                    */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 * 
 * PerfectTIN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PerfectTIN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PerfectTIN. If not, see <http://www.gnu.org/licenses/>.
 */
#include "threads.h"
#include "point.h"
#include "octagon.h"
#include "config.h"
#include "fileio.h"

using namespace std;

int main(int argc, char *argv[])
{
  int exitStatus;
  int i,sz;
  int nthreads;
  string command,file;
  PtinHeader header;
  if (argc>=2)
  {
    command=argv[1];
    file=argv[2];
  }
  if (command=="readPtin")
    header=readPtin(file);
  if (command=="readCloud")
    readCloud(file,1);
  exitStatus=0;
  cout<<header.tolRatio<<"×"<<header.tolerance<<endl;
  sz=net.edges.size();
  cout<<sz<<" edges\n";
  return exitStatus;
}
