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
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include "csv.h"
#include "fileio.h"
#include "point.h"
#include "xyzfile.h"
#include "ldecimal.h"
using namespace std;
namespace po=boost::program_options;

/* This program reads a CSV file which contains depth measurements of a body
 * of water, at two frequencies and thus at two layers of the bottom.
 * The points are in pairs with the same xy coordinates and different elevations.
 */

int main(int argc, char *argv[])
{
  string line;
  vector<string> parsedLine;
  xyz thePoint;
  string inputFileName,baseFileName;
  ifstream inputFile;
  ofstream highFile,lowFile,rejectFile;
  double maxSpread;
  int i;
  bool validCmd=true;
  vector<xyz> pointColumn;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  generic.add_options()
    ("maxspread,s",po::value<double>(&maxSpread)->default_value(INFINITY,"inf"),"Maximum vertical spread");
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
  {
    inputFile.open(inputFileName);
    baseFileName=noExt(inputFileName);
    highFile.open(baseFileName+"-high.xyz");
    lowFile.open(baseFileName+"-low.xyz");
    rejectFile.open(baseFileName+"-reject.xyz");
  }
  else
  {
    validCmd=false;
    cerr<<"Usage: dibathy [options] file\n";
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
      if (pointColumn.size()) // The points can occur in either order.
      {
	/*cout<<"Elev";
	for (i=0;i<pointColumn.size();i++)
	  cout<<' '<<ldecimal(pointColumn[i].elev());
	if (pointColumn.size()>1)
	  cout<<((pointColumn[0].elev()>pointColumn[1].elev())?" >":" <");
	cout<<endl;*/
	if (pointColumn.size()==2)
	{
	  if (pointColumn[0].elev()>pointColumn[1].elev())
	    swap(pointColumn[0],pointColumn[1]);
	  if (pointColumn[1].elev()-pointColumn[0].elev()>maxSpread)
	  {
	    writeXyzTextDot(rejectFile,pointColumn[0]);
	    writeXyzTextDot(rejectFile,pointColumn[1]);
	  }
	  else
	  {
	    writeXyzTextDot(lowFile,pointColumn[0]);
	    writeXyzTextDot(highFile,pointColumn[1]);
	  }
	}
	else
	  for (i=0;i<pointColumn.size();i++)
	    writeXyzTextDot(rejectFile,pointColumn[i]);
      }
      pointColumn.clear();
      if (thePoint.isfinite())
	pointColumn.push_back(thePoint);
    }
  }
  return 0;
}
