/******************************************************/
/*                                                    */
/* fileio.cpp - file I/O                              */
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
#include <vector>
#include <cmath>
#include "dxf.h"
#include "boundrect.h"
#include "octagon.h"
#include "ply.h"
#include "las.h"
#include "threads.h"
#include "binio.h"
#include "cloud.h"
#include "fileio.h"
using namespace std;

PtinHeader::PtinHeader()
{
  conversionTime=tolRatio=numPoints=numConvexHull=numTriangles=0;
  tolerance=NAN;
}

/* These functions are common to the command-line and GUI programs.
 * In the GUI program, file I/O is done by a thread, one at a time (except
 * that multiple formats of the same TIN may be output by different threads
 * at once), while the program is not working on the TIN.
 */
string noExt(string fileName)
{
  long long slashPos,dotPos; // npos turns into -1, which is convenient
  string between;
  slashPos=fileName.rfind('/');
  dotPos=fileName.rfind('.');
  if (dotPos>slashPos)
  {
    between=fileName.substr(slashPos+1,dotPos-slashPos-1);
    if (between.find_first_not_of('.')>between.length())
      dotPos=-1;
  }
  if (dotPos>slashPos)
    fileName.erase(dotPos);
  return fileName;
}

string baseName(string fileName)
{
  long long slashPos;
  slashPos=fileName.rfind('/');
  return fileName.substr(slashPos+1);
}

void writeDxf(string outputFile,bool asc,double outUnit)
{
  vector<GroupCode> dxfCodes;
  int i;
  BoundRect br;
  br.include(&net);
  ofstream dxfFile(outputFile,ofstream::binary|ofstream::trunc);
  dxfHeader(dxfCodes,br);
  tableSection(dxfCodes);
  openEntitySection(dxfCodes);
  for (i=0;i<net.triangles.size();i++)
    if (net.triangles[i].ptValid())
      insertTriangle(dxfCodes,net.triangles[i],outUnit);
    else
      cerr<<"Invalid triangle "<<i<<endl;
  closeEntitySection(dxfCodes);
  dxfEnd(dxfCodes);
  writeDxfGroups(dxfFile,dxfCodes,asc);
}

void readCloud(string inputFile,double inUnit)
{
  int i,already=cloud.size();
  readPly(inputFile);
  if (cloud.size()==already)
    readLas(inputFile);
  if (cloud.size()>already)
    cout<<"Read "<<cloud.size()-already<<" dots\n";
  for (i=already;i<cloud.size();i++)
    cloud[i]*=inUnit;
}

void writePoint(ostream &file,xyz pnt)
{
  writeledouble(file,pnt.getx());
  writeledouble(file,pnt.gety());
  writeledouble(file,pnt.getz());
}

xyz readPoint(istream &file)
{
  double x,y,z;
  x=readledouble(file);
  y=readledouble(file);
  z=readledouble(file);
  return xyz(x,y,z);
}

void writePoint4(ostream &file,xyz pnt)
{
  writelefloat(file,pnt.getx());
  writelefloat(file,pnt.gety());
  writelefloat(file,pnt.getz());
}

xyz readPoint4(istream &file)
{
  double x,y,z;
  z=y=x=readlefloat(file);
  if (std::isfinite(y))
    z=y=readlefloat(file);
  if (std::isfinite(z))
    z=readlefloat(file);
  return xyz(x,y,z);
}

void writeTriangle(ostream &file,triangle *tri)
{
  int aInx,bInx,cInx;
  int i;
  xyz ctr;
  wingEdge.lock_shared();
  aInx=net.revpoints[tri->a];
  bInx=net.revpoints[tri->b];
  cInx=net.revpoints[tri->c];
  wingEdge.unlock_shared();
  ctr=((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3;
  writeleint(file,aInx);
  writeleint(file,bInx);
  writeleint(file,cInx);
  file.put(tri->dots.size()<255?tri->dots.size():255);
  /* To save space, dots are written in 4-byte floats as the difference from
   * the centroid. If there are at least 255 dots, the end is marked with NAN.
   */
  for (i=0;i<tri->dots.size();i++)
    writePoint4(file,tri->dots[i]-ctr);
  if (tri->dots.size()>=255)
    writelefloat(file,NAN);
}

const int ptinHeaderFormat=0x00000020;
/* ptinHeaderFormat counts all bytes after the header format itself and before
 * the start of points. The low two bytes are the count of bytes; the high
 * two bytes are used to disambiguate between header formats that have
 * the same number of bytes.
 */

void writePtin(string outputFile,int tolRatio,double tolerance)
/* inputFile contains the tolerance ratio, unless it's 1.
 * tolerance is the final, not stage, tolerance. This can cause weirdness
 * if one changes the tolerance during a conversion.
 */
{
  int i,n;
  unsigned checkSum=0;
  xyz pnt;
  triangle *tri;
  ofstream checkFile(outputFile,ios::binary);
  writeleshort(checkFile,6);
  writeleshort(checkFile,28);
  writeleshort(checkFile,496);
  writeleshort(checkFile,8128);
  writeleint(checkFile,ptinHeaderFormat);
  writelelong(checkFile,net.conversionTime);
  writeleint(checkFile,tolRatio);
  writeledouble(checkFile,NAN); // will be filled in later with tolerance
  writeleint(checkFile,net.points.size());
  writeleint(checkFile,net.convexHull.size());
  writeleint(checkFile,net.triangles.size());
  for (i=1;i<=net.points.size();i++)
  {
    wingEdge.lock_shared();
    pnt=net.points[i];
    wingEdge.unlock_shared();
    writePoint(checkFile,pnt);
  }
  for (i=0;i<net.convexHull.size();i++)
  {
    wingEdge.lock_shared();
    n=net.revpoints[net.convexHull[i]];
    wingEdge.unlock_shared();
    writeleint(checkFile,n);
  }
  for (i=0;i<net.triangles.size();i++)
  {
    wingEdge.lock_shared();
    tri=&net.triangles[i];
    wingEdge.unlock_shared();
    writeTriangle(checkFile,tri);
  }
  checkFile.flush();
  checkFile.seekp(24,ios::beg);
  writeledouble(checkFile,tolerance);
}

PtinHeader readPtinHeader(istream &inputFile)
{
  PtinHeader ret;
  int headerFormat;
  if (readleshort(inputFile)==6 && readleshort(inputFile)==28 &&
      readleshort(inputFile)==496 && readleshort(inputFile)==8128)
  {
    headerFormat=readleint(inputFile);
    switch (headerFormat)
    {
      case 0x00000020:
	ret.conversionTime=readlelong(inputFile);
	ret.tolRatio=readleint(inputFile);
	ret.tolerance=readledouble(inputFile);
	ret.numPoints=readleint(inputFile);
	ret.numConvexHull=readleint(inputFile);
	ret.numTriangles=readleint(inputFile);
	break;
      default:
	ret.tolRatio=PT_UNKNOWN_HEADER_FORMAT;
    }
  }
  else
    ret.tolRatio=PT_NOT_PTIN_FILE;
  return ret;
}

PtinHeader readPtinHeader(std::string inputFile)
{
  ifstream ptinFile(inputFile,ios::binary);
  return readPtinHeader(ptinFile);
}

PtinHeader readPtin(std::string inputFile)
{
  ifstream ptinFile(inputFile,ios::binary);
  PtinHeader header;
  int i,j,m,n;
  triangle *tri;
  xyz pnt,ctr;
  bool readingStarted=false;
  header=readPtinHeader(ptinFile);
  if (header.tolRatio>0 && header.tolerance>0)
  {
    net.clear();
    cloud.clear();
    readingStarted=true;
    for (i=1;i<=header.numPoints;i++)
      net.points[i]=point(readPoint(ptinFile));
  }
  if (header.tolRatio>0 && header.tolerance>0)
    for (i=0;i<header.numConvexHull;i++)
    {
      n=readleint(ptinFile);
      if (n<1 || n>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      net.convexHull.push_back(&net.points[n]);
    }
  if (header.tolRatio>0 && header.tolerance>0)
    for (i=0;i<header.numTriangles;i++)
    {
      n=net.addtriangle();
      tri=&net.triangles[n];
      m=readleint(ptinFile);
      if (m<1 || m>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->a=&net.points[m];
      m=readleint(ptinFile);
      if (m<1 || m>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->b=&net.points[m];
      m=readleint(ptinFile);
      if (m<1 || m>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->c=&net.points[m];
      ctr=((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3;
      tri->flatten();
      if (!tri->sarea>0)
	header.tolRatio=PT_BACKWARD_TRIANGLE;
      m=ptinFile.get()&255;
      if (m<255)
	for (j=0;j<m;j++)
	{
	  pnt=readPoint4(ptinFile)+ctr;
	  tri->dots.push_back(pnt);
	}
      else
	while (true)
	{
	  pnt=readPoint4(ptinFile)+ctr;
	  if (pnt.isnan())
	    break;
	  tri->dots.push_back(pnt);
	}
    }
  if (header.tolRatio>0 && header.tolerance>0)
    net.makeEdges();
  else if (readingStarted)
    net.clear();
  return header;
}
