/******************************************************/
/*                                                    */
/* dibathy.cpp - two-layer bathymetry separation      */
/*                                                    */
/******************************************************/
/* Copyright 2021 Pierre Abbat.
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
#include <iostream>
#include <fstream>
#include "csv.h"
using namespace std;

/* This program reads a CSV file which contains depth measurements of a body
 * of water, supposedly at two frequencies and thus at two layers of the bottom.
 * Neither of us, having looked at the file, has figured out how to tell
 * which points belong to which layer.
 */

int main(int argc, char *argv[])
{
  string line;
  vector<string> parsedLine;
  while (cin.good())
  {
    getline(cin,line);
    parsedLine=parsecsvline(line);
    if (parsedLine.size()>3) // Input is PNEZ..., output is XYZ, switch X and Y
      cout<<parsedLine[2]<<' '<<parsedLine[1]<<' '<<parsedLine[3]<<' '<<endl;
  }
  return 0;
}
