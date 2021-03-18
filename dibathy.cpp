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
#include "point.h"
#include "ldecimal.h"
using namespace std;

/* This program reads a CSV file which contains depth measurements of a body
 * of water, at two frequencies and thus at two layers of the bottom.
 * The points are in pairs with the same xy coordinates and different elevations.
 */

int main(int argc, char *argv[])
{
  string line;
  vector<string> parsedLine;
  xyz thePoint;
  int i;
  vector<xyz> pointColumn;
  while (cin.good())
  {
    getline(cin,line);
    parsedLine=parsecsvline(line);
    if (parsedLine.size()>3)
    {
      try
      { // Input is PNEZ..., output is XYZ, switch X and Z
	thePoint=xyz(stod(parsedLine[2]),stod(parsedLine[1]),stod(parsedLine[3]));
      }
      catch (...)
      {
	thePoint=nanxyz;
      }
    }
    else
      thePoint=nanxyz;
    if (pointColumn.size() && xy(thePoint)==xy(pointColumn[0]))
      pointColumn.push_back(thePoint);
    else
    {
      if (pointColumn.size())
      {
	cout<<"Elev";
	for (i=0;i<pointColumn.size();i++)
	  cout<<' '<<ldecimal(pointColumn[i].elev());
	if (pointColumn.size()>1)
	  cout<<((pointColumn[0].elev()>pointColumn[1].elev())?" >":" <");
	cout<<endl;
      }
      pointColumn.clear();
      if (thePoint.isfinite())
	pointColumn.push_back(thePoint);
    }
  }
  return 0;
}
