/******************************************************/
/*                                                    */
/* xyzfile.cpp - XYZ point cloud files                */
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
#include <cmath>
#include "xyzfile.h"
#include "cloud.h"
#include "ldecimal.h"
using namespace std;

/* There is no specification for XYZ point cloud files. They are plain text,
 * with the first three fields being x, y, and z, but the fields after that
 * could be anything, and the separator could be spaces, comma, or tab.
 * There is also a chemical XYZ file format, in which the first field is
 * a chemical element symbol (alphabetic, e.g. F or Ne).
 */

xyz parseXyz(string line)
{
  double x=NAN,y=NAN,z=NAN;
  size_t pos;
  int i,ncomma;
  vector<string> words;
  while (line.length())
  {
    i=ncomma=0;
    while (i<line.length())
    {
      if (line[i]==',' && ncomma)
	break;
      if (line[i]==',')
	ncomma++;
      if (line[i]!=',' && !isblank(line[i]))
	break;
      i++;
    }
    line.erase(0,i);
    pos=line.find_first_of(" \t,");
    words.push_back(line.substr(0,pos));
    line.erase(0,pos);
  }
  if (words.size()>=3)
  {
    try
    {
      x=stod(words[0]);
      y=stod(words[1]);
      z=stod(words[2]);
    }
    catch (...)
    {
      x=y=z=NAN;
    }
  }
  return xyz(x,y,z);
}

void readXyzText(string fname)
{
  ifstream xyzfile(fname);
  string line;
  xyz pnt;
  while (xyzfile)
  {
    getline(xyzfile,line);
    pnt=parseXyz(line);
    if (pnt.isnan())
      break;
    cloud.push_back(pnt);
  }
}

void writeXyzTextDot(ofstream &file,xyz dot)
{
  file<<ldecimal(dot.getx())<<' '<<ldecimal(dot.gety())<<' '<<ldecimal(dot.getz())<<'\n';
}
