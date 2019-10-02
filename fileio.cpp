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

void writePoint4(ostream &file,xyz pnt)
{
  writelefloat(file,pnt.getx());
  writelefloat(file,pnt.gety());
  writelefloat(file,pnt.getz());
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

const int ptinHeaderFormat=0x00000018;
/* ptinHeaderFormat counts all bytes after the header format itself and before
 * the start of points. The low two bytes are the count of bytes; the high
 * two bytes are used to disambiguate between header formats that have
 * the same number of bytes.
 */

void writePtin(string inputFile,int tolRatio,double tolerance)
/* inputFile contains the tolerance ratio, unless it's 1.
 * tolerance is the final, not stage, tolerance. This can cause weirdness
 * if one changes the tolerance during a conversion.
 */
{
  int i,n;
  unsigned checkSum=0;
  xyz pnt;
  triangle *tri;
  ofstream checkFile(inputFile,ios::binary);
  writeleshort(checkFile,6);
  writeleshort(checkFile,28);
  writeleshort(checkFile,496);
  writeleshort(checkFile,8128);
  writeleint(checkFile,ptinHeaderFormat);
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
  for (i=1;i<=net.convexHull.size();i++)
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
  checkFile.seekp(16,ios::beg);
  writeledouble(checkFile,tolerance);
}
