/******************************************************/
/*                                                    */
/* carlsontin.cpp - output TIN in Carlson DTM format  */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
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

#include <fstream>
#include "octagon.h"
#include "binio.h"
#include "fileio.h"
#include "carlsontin.h"
using namespace std;

#define CA_POINT 0x1c01
#define CA_TRIANGLE 0xd04

const unsigned int carlsonHeaderWords[]=
/* I have no idea whether any of this means anything or is just binary junk.
 * Two TIN files produced on the same computer with different numbers of points
 * have identical headers; two TIN files with the 3face option have a few
 * words different; another TIN file produced on my data collector has
 * completely different numbers in the header.
 */
{
  0x17b0800, // different in 3dface
  0x1981bc,0x73d6795a,3,0x73d67928,0x863c32cb, // different in 3dface
  0x4000,0xd06782,0,25,0,1,0x198188,0x100052,
  0x198248,0x73d6cf5e,0xf5fd64a7, // different in 3dface
  0xfffffffe,0x73d67928,0x73d679b3,0x198f78,0x8301,64,0x180,0x198204,
  1,0x198214,0x73d2f97c,0x198204,0x198f78,0x8301,0x198f78,
  0xd06780,0,0xd06780,0,0x198210,0x73d22e2b,0x38bcce8, // different in 3dface
  0x19821c,0x73d2f33b,19,0x198258,0x73d3019b,0x73d97408,
  0x73d30192,0x863c312f, // different in 3dface
  0,0x172c3178, // different in 3dface
  0x198f78,0x73d97408,0x19822c,0x1983f4,0x198974,0x73d6cf5e,0x14fd780f, // different in 3dface
  0xb5ed8d09,0xb0c6f7a0
};

void writeCarlsonTin(string outputFile,double outUnit)
{
  int i;
  ofstream tinFile(outputFile,ofstream::trunc|ofstream::binary);
  writeleshort(tinFile,0xff00);
  tinFile<<"#Carlson DTM $Revision: 20717 $\n";
  for (i=0;i<sizeof(carlsonHeaderWords)/sizeof(int);i++)
    writeleint(tinFile,carlsonHeaderWords[i]);
  writeleshort(tinFile,0x3e);
  for (i=1;i<=net.points.size();i++)
  {
    writeleshort(tinFile,CA_POINT);
    writeleint(tinFile,i);
    writePoint(tinFile,net.points[i]/outUnit);
  }
  for (i=0;i<net.triangles.size();i++)
  {
    writeleshort(tinFile,CA_TRIANGLE);
    writeleint(tinFile,net.revpoints[net.triangles[i].a]);
    writeleint(tinFile,net.revpoints[net.triangles[i].b]);
    writeleint(tinFile,net.revpoints[net.triangles[i].c]);
    tinFile.put(0);
  }
}
