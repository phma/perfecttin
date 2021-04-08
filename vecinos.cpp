/******************************************************/
/*                                                    */
/* vecinos.cpp - list neighbors of a point in CSV     */
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
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <cmath>
#include "csv.h"
#include "fileio.h"
#include "point.h"
#include "xyzfile.h"
#include "ldecimal.h"
using namespace std;
namespace po=boost::program_options;

/* This program reads a CSV file which contains depth measurements of a body
 * of water, at two frequencies and thus at two layers of the bottom.
 * Some points need to be discarded. Given one of these points, it lists
 * all points within a specified horizontal distance of it.
 */

int main(int argc, char *argv[])
{
  string line;
  vector<string> parsedLine;
  xyz thePoint,lastHigh=nanxyz,lastLow=nanxyz,lastReject=nanxyz;
  xyz firstHigh=nanxyz,firstLow=nanxyz,firstReject=nanxyz;
  string inputFileName;
  ifstream inputFile;
  double distance;
  int i;
  int pointNum,thePointNum;
  bool validCmd=true;
  bool oneLayer=false;
  map<int,xyz> pointList;
  map<int,xyz>::iterator j;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  generic.add_options()
    ("distance,d",po::value<double>(&distance)->default_value(0),"Distance")
    ("point,p",po::value<int>(&pointNum)->default_value(0),"Point number")
    ("one-layer,o","One layer");
  hidden.add_options()
    ("input",po::value<string>(&inputFileName),"Input file");
  p.add("input",-1);
  cmdline_options.add(generic).add(hidden);
  try
  {
    po::store(po::command_line_parser(argc,argv).options(cmdline_options).positional(p).run(),vm);
    po::notify(vm);
  }
  catch (exception &ex)
  {
    cerr<<ex.what()<<endl;
    validCmd=false;
  }
  if (inputFileName.length())
    inputFile.open(inputFileName);
  else
  {
    validCmd=false;
    cerr<<"Usage: vecinos [options] file\n";
    cerr<<generic;
  }
  while (validCmd && inputFile.good())
  {
    getline(inputFile,line);
    parsedLine=parsecsvline(line);
    if (parsedLine.size()>3)
    {
      try
      { // Input is PNEZ..., output is XYZ, switch X and Z
	thePoint=xyz(stod(parsedLine[2]),stod(parsedLine[1]),stod(parsedLine[3]));
	thePointNum=stoi(parsedLine[0]);
      }
      catch (...)
      {
	thePoint=nanxyz;
      }
    }
    else
      thePoint=nanxyz;
    if (thePoint.isfinite())
      pointList[thePointNum]=thePoint;
  }
  if (validCmd)
  {
    if (pointList.count(pointNum))
    {
      for (j=pointList.begin();j!=pointList.end();++j)
	if (dist(xy(j->second),xy(pointList[pointNum]))<=distance)
	  cout<<j->first<<endl;
    }
    else
      cerr<<"No point numbered "<<pointNum<<" in "<<inputFileName<<endl;
  }
  return 0;
}
