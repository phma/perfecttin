/******************************************************/
/*                                                    */
/* tintext.cpp - output TIN in text file              */
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
// https://www.xmswiki.com/wiki/TIN_Files

#include <fstream>
#include "octagon.h"
#include "ldecimal.h"
using namespace std;

void writeTinText(string outputFile)
{
  int i;
  ofstream tinFile(outputFile,ofstream::trunc);
  tinFile<<"TIN\nBEGT\nVERT "<<net.points.size()<<endl;
  for (i=1;i<=net.points.size();i++)
  {
    tinFile<<ldecimal(net.points[i].getx())<<' ';
    tinFile<<ldecimal(net.points[i].gety())<<' ';
    tinFile<<ldecimal(net.points[i].getz())<<" 0\n"; // The last number is the lock flag, whatever that means.
  }
  tinFile<<"TRI "<<net.triangles.size()<<endl;
  for (i=0;i<net.triangles.size();i++)
  {
    tinFile<<net.revpoints[net.triangles[i].a]<<' ';
    tinFile<<net.revpoints[net.triangles[i].b]<<' ';
    tinFile<<net.revpoints[net.triangles[i].c]<<'\n';
  }
  tinFile<<"ENDT\n";
}
