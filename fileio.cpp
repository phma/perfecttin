/******************************************************/
/*                                                    */
/* fileio.cpp - file I/O                              */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#include <vector>
#include <cmath>
#include <cstring>
#include "dxf.h"
#include "neighbor.h"
#include "boundrect.h"
#include "octagon.h"
#include "ply.h"
#include "xyzfile.h"
#include "ldecimal.h"
#include "las.h"
#include "threads.h"
#include "binio.h"
#include "angle.h"
#include "cloud.h"
#include "fileio.h"
using namespace std;

CoordCheck zCheck;
Printer3dSize printer3d;

PtinHeader::PtinHeader()
{
  conversionTime=tolRatio=density=numPoints=numConvexHull=numTriangles=0;
  tolerance=NAN;
}

CoordCheck::CoordCheck()
{
  clear();
}

void CoordCheck::clear()
{
  count=0;
  memset(stage0,0,sizeof(stage0));
  memset(stage1,0,sizeof(stage1));
  memset(stage2,0,sizeof(stage2));
  memset(stage3,0,sizeof(stage3));
  memset(stage4,0,sizeof(stage4));
}

void CoordCheck::dump()
// For debugging the test, which shifts mostly 0s into it.
{
  int i,j;
  for (i=0;i<15;i++)
  {
    cout<<"Stage 0["<<i<<"]\n";
    for (j=0;j<8192;j++)
      if (stage0[i][j])
	cout<<"["<<j<<" ("<<hex<<j<<")]="<<dec<<stage0[i][j]<<endl;
  }
  for (i=0;i<28;i++)
  {
    cout<<"Stage 1["<<i<<"]\n";
    for (j=0;j<8192;j++)
      if (stage1[i][j])
	cout<<"["<<j<<" ("<<hex<<j<<")]="<<dec<<stage1[i][j]<<endl;
  }
  for (i=0;i<41;i++)
  {
    cout<<"Stage 2["<<i<<"]\n";
    for (j=0;j<8192;j++)
      if (stage2[i][j])
	cout<<"["<<j<<" ("<<hex<<j<<")]="<<dec<<stage2[i][j]<<endl;
  }
  for (i=0;i<54;i++)
  {
    cout<<"Stage 3["<<i<<"]\n";
    for (j=0;j<8192;j++)
      if (stage3[i][j])
	cout<<"["<<j<<" ("<<hex<<j<<")]="<<dec<<stage3[i][j]<<endl;
  }
  for (i=0;i<65;i++)
  {
    cout<<"Stage 4["<<i<<"]\n";
    for (j=0;j<4096;j++)
      if (stage4[i][j])
	cout<<"["<<j<<" ("<<hex<<j<<")]="<<dec<<stage4[i][j]<<endl;
  }
}

CoordCheck& CoordCheck::operator<<(double val)
{
  int i;
  double lastStageSum;
  for (i=0;i<13;i++)
    if ((count>>i)&1)
      stage0[i][count&8191]=-val;
    else
      stage0[i][count&8191]=val;
  stage0[13][count&8191]=val;
  if ((count&8191)==8191)
  {
    for (i=0;i<13;i++)
      stage1[i][(count>>13)&8191]=pairwisesum(stage0[i],8192);
    lastStageSum=pairwisesum(stage0[13],8192);
    for (i=13;i<26;i++)
      if ((count>>i)&1)
	stage1[i][(count>>13)&8191]=-lastStageSum;
      else
	stage1[i][(count>>13)&8191]=lastStageSum;
    stage1[26][(count>>13)&8191]=lastStageSum;
    memset(stage0,0,sizeof(stage0));
  }
  if ((count&0x3ffffff)==0x3ffffff)
  {
    for (i=0;i<26;i++)
      stage2[i][(count>>26)&8191]=pairwisesum(stage1[i],8192);
    lastStageSum=pairwisesum(stage1[26],8192);
    for (i=26;i<39;i++)
      if ((count>>i)&1)
	stage2[i][(count>>26)&8191]=-lastStageSum;
      else
	stage2[i][(count>>26)&8191]=lastStageSum;
    stage2[39][(count>>26)&8191]=lastStageSum;
    memset(stage1,0,sizeof(stage1));
  }
  if ((count&0x7fffffffff)==0x7fffffffff)
  {
    for (i=0;i<39;i++)
      stage3[i][(count>>39)&8191]=pairwisesum(stage2[i],8192);
    lastStageSum=pairwisesum(stage2[39],8192);
    for (i=39;i<52;i++)
      if ((count>>i)&1)
	stage3[i][(count>>39)&8191]=-lastStageSum;
      else
	stage3[i][(count>>39)&8191]=lastStageSum;
    stage3[52][(count>>39)&8191]=lastStageSum;
    memset(stage2,0,sizeof(stage2));
  }
  if ((count&0xfffffffffffff)==0xfffffffffffff)
  {
    for (i=0;i<52;i++)
      stage4[i][(count>>52)&8191]=pairwisesum(stage3[i],8192);
    lastStageSum=pairwisesum(stage3[52],8192);
    for (i=52;i<64;i++)
      if ((count>>i)&1)
	stage4[i][(count>>52)&8191]=-lastStageSum;
      else
	stage4[i][(count>>52)&8191]=lastStageSum;
    memset(stage3,0,sizeof(stage3));
  }
  count++;
  return *this;
}

double CoordCheck::operator[](int n)
{
  int n0=n,n1=n,n2=n,n3=n;
  int s0=1,s1=1,s2=1,s3=1;
  if (n0>13)
  {
    n0=13;
    s0=1-2*((count>>n)&1);
  }
  if (n1>26)
  {
    n1=26;
    s1=1-2*((count>>n)&1);
  }
  if (n2>39)
  {
    n2=39;
    s2=1-2*((count>>n)&1);
  }
  if (n3>52)
  {
    n3=52;
    s3=1-2*((count>>n)&1);
  }
  return pairwisesum(stage0[n0],8192)*s0+pairwisesum(stage1[n1],8192)*s1+
	 pairwisesum(stage2[n2],8192)*s2+pairwisesum(stage3[n3],8192)*s3+
         pairwisesum(stage4[n],4096);
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

string extension(string fileName)
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
    fileName.erase(0,dotPos);
  else
    fileName="";
  return fileName;
}

string baseName(string fileName)
{
  long long slashPos;
  slashPos=fileName.rfind('/');
  return fileName.substr(slashPos+1);
}

void deleteFile(string fileName)
{
  remove(fileName.c_str());
}

void writeDxf(string outputFile,bool asc,double outUnit,int flags)
{
  vector<GroupCode> dxfCodes;
  int i;
  BoundRect br;
  br.include(&net);
  ofstream dxfFile(outputFile,ofstream::binary|ofstream::trunc);
  //dxfHeader(dxfCodes,br);
  tableSection(dxfCodes);
  openEntitySection(dxfCodes);
  for (i=0;i<net.triangles.size();i++)
    if (net.triangles[i].ptValid())
      if (net.triangles[i].dots.size() || (flags&1))
	insertTriangle(dxfCodes,net.triangles[i],outUnit);
      else;
    else
      cerr<<"Invalid triangle "<<i<<endl;
  closeEntitySection(dxfCodes);
  dxfEnd(dxfCodes);
  writeDxfGroups(dxfFile,dxfCodes,asc);
}

void writeStl(string outputFile,bool asc,double outUnit,int flags)
/* Unlike the other export functions, setting outUnit to feet does not result
 * in an STL file in feet; the file is always in millimeters. Rather, it means
 * to prefer scales 1:x where x is divisible by 12. Also the flag does not mean
 * to write empty triangles, which must always be written in STL; rather
 * it means to decrease the scale to a round number.
 */
{
  ofstream stlFile(outputFile,asc?ios::trunc:(ios::binary|ios::trunc));
  double bear;
  vector<StlTriangle> stltri;
  bear=turnFitInPrinter(net,printer3d);
  stltri=stlMesh(printer3d,false,false);
  if (asc)
    writeStlText(stlFile,stltri);
  else
    writeStlBinary(stlFile,stltri);
}

int readCloud(string &inputFile,double inUnit)
{
  int i,already=cloud.size(),ret=0;
  readPly(inputFile);
  if (cloud.size()>already)
    ret=RES_LOAD_PLY;
  else
  {
    try
    {
      readLas(inputFile);
    }
    catch (...)
    {
      cloud.resize(already);
    }
    if (cloud.size()>already)
      ret=RES_LOAD_LAS;
  }
  if (cloud.size()==already)
  {
    readXyzText(inputFile);
    if (cloud.size()>already)
      ret=RES_LOAD_XYZ;
  }
  if (cloud.size()>already)
    cout<<"Read "<<cloud.size()-already<<" dots\n";
  for (i=already;i<cloud.size();i++)
    cloud[i]*=inUnit;
  return ret;
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
  if (file.eof())
  {
    x=INFINITY;
    y=z=NAN;
  }
  else
  {
    z=y=x=readlefloat(file);
    if (std::isfinite(y))
      z=y=readlefloat(file);
    if (std::isfinite(z))
      z=readlefloat(file);
  }
  return xyz(x,y,z);
}

void writeTriangle(ostream &file,triangle *tri)
{
  int aInx,bInx,cInx;
  int i;
  xyz ctr;
  net.wingEdge.lock_shared();
  aInx=net.revpoints[tri->a];
  bInx=net.revpoints[tri->b];
  cInx=net.revpoints[tri->c];
  net.wingEdge.unlock_shared();
  ctr=((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3;
  writeleint(file,aInx);
  writeleint(file,bInx);
  writeleint(file,cInx);
  file.put(tri->dots.size()<255?tri->dots.size():255);
  /* To save space, dots are written in 4-byte floats as the difference from
   * the centroid. If there are at least 255 dots, the end is marked with NAN.
   */
  for (i=0;i<tri->dots.size();i++)
  {
    writePoint4(file,tri->dots[i]-ctr);
    zCheck<<tri->dots[i].getz();
  }
  if (tri->dots.size()>=255)
    writelefloat(file,NAN);
}

const int ptinHeaderFormat=0x00000028;
/* ptinHeaderFormat counts all bytes after the header format itself and before
 * the start of points. The low two bytes are the count of bytes; the high
 * two bytes are used to disambiguate between header formats that have
 * the same number of bytes.
 *
 * Header formats:
 * 0006 001c 01f0 1fc0	Magic numbers
 * uint32		Header format (0x20, 0x28, or 0x2c)
 * uint64		Timestamp
 * uint32		Tolerance ratio
 * double		Tolerance
 * double		Density, not present in format 0x20
 * uint32		Number of points
 * uint32		Number of points in convex hull
 * uint32		Number of triangles
 * uint32		Number of groups of polylines, not present in format 0x20 or 0x28
 */

/* Format of contour lines:
 * 0000		This is a group of contour lines
 * 0008		Length of the label
 * double	Label of this group of contour lines (the contour interval)
 * 0000		Each contour is a 2D polyline with elevation
 * uint32	Number of contours
 * Format of one contour:
 * double	Elevation
 * uint32	Number of endpoints
 * double*2n	Endpoints
 * uint32	Number of lengths
 * double*n	Lengths
 * uint32	Number of deltas
 * int32*n	Deltas (final zeros omitted)
 * uint32	Number of delta2s
 * int32*n	Delta2s (final zeros omitted)
 * TBD		Checksums
 */

void writePtin(string outputFile,int tolRatio,double tolerance,double density)
/* inputFile contains the tolerance ratio, unless it's 1.
 * tolerance is the final, not stage, tolerance. This can cause weirdness
 * if one changes the tolerance during a conversion.
 */
{
  int i,n;
  xyz pnt;
  triangle *tri;
  ofstream checkFile(outputFile,ios::binary);
  vector<double> zcheck;
  zCheck.clear();
  writeleshort(checkFile,6);
  writeleshort(checkFile,28);
  writeleshort(checkFile,496);
  writeleshort(checkFile,8128);
  writeleint(checkFile,ptinHeaderFormat);
  writelelong(checkFile,net.conversionTime);
  writeleint(checkFile,tolRatio);
  writeledouble(checkFile,NAN); // will be filled in later with tolerance
  writeledouble(checkFile,NAN); // will be filled in later with density
  writeleint(checkFile,net.points.size());
  writeleint(checkFile,net.convexHull.size());
  writeleint(checkFile,net.triangles.size());
  for (i=1;i<=net.points.size();i++)
  {
    net.wingEdge.lock_shared();
    pnt=net.points[i];
    net.wingEdge.unlock_shared();
    writePoint(checkFile,pnt);
  }
  for (i=0;i<net.convexHull.size();i++)
  {
    net.wingEdge.lock_shared();
    n=net.revpoints[net.convexHull[i]];
    net.wingEdge.unlock_shared();
    writeleint(checkFile,n);
  }
  for (i=0;i<net.triangles.size();i++)
  {
    net.wingEdge.lock_shared();
    tri=&net.triangles[i];
    net.wingEdge.unlock_shared();
    writeTriangle(checkFile,tri);
  }
  for (i=0;i<64;i++)
    zcheck.push_back(zCheck[i]);
  while (zcheck.size()>1 && zcheck[zcheck.size()-1]==zcheck[zcheck.size()-2])
    zcheck.resize(zcheck.size()-1);
  checkFile.put(zcheck.size());
  for (i=0;i<zcheck.size();i++)
    writeledouble(checkFile,zcheck[i]);
  checkFile.flush();
  checkFile.seekp(24,ios::beg);
  writeledouble(checkFile,tolerance);
  writeledouble(checkFile,density);
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
	ret.density=1/sqr(ret.tolerance);
	ret.numPoints=readleint(inputFile);
	ret.numConvexHull=readleint(inputFile);
	ret.numTriangles=readleint(inputFile);
	break;
      case 0x00000028:
	ret.conversionTime=readlelong(inputFile);
	ret.tolRatio=readleint(inputFile);
	ret.tolerance=readledouble(inputFile);
	ret.density=readledouble(inputFile);
	ret.numPoints=readleint(inputFile);
	ret.numConvexHull=readleint(inputFile);
	ret.numTriangles=readleint(inputFile);
	break;
      default:
	ret.tolRatio=PT_UNKNOWN_HEADER_FORMAT;
    }
    if (ret.numTriangles!=2*ret.numPoints-ret.numConvexHull-2)
      ret.tolRatio=PT_COUNT_MISMATCH;
  }
  else
    ret.tolRatio=PT_NOT_PTIN_FILE;
  return ret;
}

int skewsym(int a,int b)
/* skewsym(a,b)=-skewsym(b,a). This function is used to check that every edge
 * (a,b) in a triangle is matched by another edge (b,a) in another triangle
 * or in the convex hull. It is nonlinear so that omitting a triangle will not
 * result in 0.
 */
{
  int a1,a2,b1,b2;
  a1=a*0x69969669;
  a2=a*PHITURN;
  b1=b*0x69969669;
  b2=b*PHITURN;
  a1=((a1&0xffe00000)>>21)|(a1&0x1ff800)|((a1&0x7ff)<<21);
  a2=((a2&0xffff0000)>>16)|((a2&0xffff)<<16);
  b1=((b1&0xffe00000)>>21)|(b1&0x1ff800)|((b1&0x7ff)<<21);
  b2=((b2&0xffff0000)>>16)|((b2&0xffff)<<16);
  return a*b1*a2-b*a1*b2;
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
  int i,j,m,n,a,b,c;
  int edgeCheck=0;
  vector<int> convexHull;
  vector<double> areas,sqrOffsets;
  triangle *tri;
  xyz pnt,ctr;
  bool readingStarted=false;
  double high=-INFINITY,low=INFINITY;
  double absToler;
  vector<double> zcheck;
  zCheck.clear();
  header=readPtinHeader(ptinFile);
  if (nBuckets()==0)
    resizeBuckets(1);
  if (header.tolRatio>0 && header.tolerance>0)
  {
    net.clear();
    cloud.clear();
    readingStarted=true;
    for (i=1;i<=header.numPoints;i++)
    {
      net.wingEdge.lock();
      net.points[i]=point(readPoint(ptinFile));
      net.revpoints[&net.points[i]]=i;
      net.wingEdge.unlock();
      if (ptinFile.eof())
      {
	header.tolRatio=PT_EOF;
	break;
      }
      if (net.points[i].getz()>high)
	high=net.points[i].getz();
      if (net.points[i].getz()<low)
	low=net.points[i].getz();
    }
  }
  colorize.setLimits(low,high);
  swap(low,high);
  if (header.tolRatio>0 && header.tolerance>0)
    for (i=0;i<header.numConvexHull;i++)
    {
      n=readleint(ptinFile);
      if (n<1 || n>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      if (i)
	edgeCheck+=skewsym(n,convexHull.back());
      if (ptinFile.eof())
      {
	header.tolRatio=PT_EOF;
	break;
      }
      convexHull.push_back(n);
      net.convexHull.push_back(&net.points[n]);
    }
  if (convexHull.size())
    edgeCheck+=skewsym(convexHull[0],convexHull.back());
  if (header.tolRatio>0 && header.tolerance>0)
    if (!net.validConvexHull())
      header.tolRatio=PT_INVALID_CONVEX_HULL;
  if (header.tolRatio>0 && header.tolerance>0)
    for (i=0;i<header.numTriangles && header.tolRatio>0;i++)
    {
      net.wingEdge.lock();
      n=net.addtriangle();
      //cout<<n<<' ';
      tri=&net.triangles[n];
      a=readleint(ptinFile);
      //cout<<a<<' ';
      if (a<1 || a>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->a=&net.points[a];
      b=readleint(ptinFile);
      //cout<<b<<' ';
      if (b<1 || b>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->b=&net.points[b];
      c=readleint(ptinFile);
      //cout<<c<<'\n';
      if (c<1 || c>header.numPoints)
	header.tolRatio=PT_INVALID_POINT_NUMBER;
      tri->c=&net.points[c];
      ctr=((xyz)*tri->a+(xyz)*tri->b+(xyz)*tri->c)/3;
      tri->flatten();
      //cout<<i<<' '<<tri->sarea<<endl;
      if (!(tri->sarea>0)) // so written to catch the NaN case
	header.tolRatio=PT_BACKWARD_TRIANGLE;
      areas.push_back(tri->sarea);
      edgeCheck+=skewsym(a,b)+skewsym(b,c)+skewsym(c,a);
      m=ptinFile.get()&255;
      if (m<255)
	for (j=0;j<m;j++)
	{
	  pnt=readPoint4(ptinFile);
	  if (xy(pnt).length()>tri->peri/3)
	    header.tolRatio=PT_DOT_OUTSIDE;
	  sqrOffsets.push_back(sqr(pnt.getz()));
	  pnt+=ctr;
	  if (tri->in(pnt))
	    tri->dots.push_back(pnt);
	  else
	    /* Because dots are stored in float, roundoff error can push a dot
	     * near an edge into an adjacent triangle. This will not affect the
	     * PT_DOT_OUTSIDE check, but could result in the dot being shuttled
	     * into a faraway triangle as refinement of the TIN continues.
	     */
	    cloud.push_back(pnt);
	  zCheck<<pnt.getz();
	  if (pnt.getz()>high)
	    high=pnt.getz();
	  if (pnt.getz()<low)
	    low=pnt.getz();
	}
      else
	while (true)
	{
	  pnt=readPoint4(ptinFile);
	  if (xy(pnt).length()>tri->peri/3)
	    header.tolRatio=PT_DOT_OUTSIDE;
	  if (!pnt.isnan())
	    sqrOffsets.push_back(sqr(pnt.getz()));
	  pnt+=ctr;
	  if (pnt.isnan())
	  {
	    if (std::isinf(pnt.getx()))
	      header.tolRatio=PT_EOF;
	    break;
	  }
	  if (tri->in(pnt))
	    tri->dots.push_back(pnt);
	  else // See above.
	    cloud.push_back(pnt);
	  zCheck<<pnt.getz();
	  if (pnt.getz()>high)
	    high=pnt.getz();
	  if (pnt.getz()<low)
	    low=pnt.getz();
	}
      tri->dots.shrink_to_fit();
      net.wingEdge.unlock();
      markBucketDirty(i); // keep painting triangles
      if (tri->dots.size()>65536)
        sleepRead(); // Let GUI update between big triangles
    }
  //cout<<"edgeCheck="<<edgeCheck<<endl;
  if (header.tolRatio>0 && header.tolerance>0 && edgeCheck)
    header.tolRatio=PT_EDGE_MISMATCH;
  if (header.tolRatio>0 && header.tolerance>0)
  {
    colorize.setLimits(low,high);
    clipHigh=2*high-low;
    clipLow=2*low-high;
    n=ptinFile.get()&255;
    for (i=0;i<n;i++)
      zcheck.push_back(readledouble(ptinFile));
    if (n==0)
      zcheck.push_back(0);
    while (zcheck.size()<64)
      zcheck.push_back(zcheck.back());
    absToler=header.tolRatio*header.tolerance*sqrt(zCheck.getCount())/65536;
    for (i=0;i<n;i++)
    {
      //cout<<i<<' '<<zcheck[i]<<' '<<zCheck[i]<<' '<<(zcheck[i]-zCheck[i]);
      //cout<<' '<<absToler+fabs(zCheck[i])/536870912<<endl;
    }
    for (i=0;i<64;i++)
    {
      if (fabs(zcheck[i]-zCheck[i])>absToler+fabs(zCheck[i])/536870912)
	// zCheck[i]/1e12 suffices for completed jobs, but is too tight for early checkpoint files.
	header.tolRatio=PT_ZCHECK_FAIL;
    }
  }
  if (header.tolRatio>0 && header.tolerance>0)
  {
    setMutexArea(pairwisesum(areas));
    net.makeEdges();
    for (i=0;i<cloud.size();i++)
    {
      tri=tri->findt(cloud[i]);
      if (!tri)
      {
	cerr<<"Can't happen: No triangle found for dot\n";
	tri=&net.triangles[0];
      }
      else
	tri->dots.push_back(cloud[i]);
    }
  }
  else if (readingStarted)
    net.clear();
  cloud.clear();
  return header;
}

void dumpTriangles(string outputFile,vector<triangle *> tris)
{
  triangle *tri;
  xyz dot;
  xy grad;
  ofstream dumpFile(outputFile);
  PostScript ps;
  BoundRect br;
  ps.open(outputFile+".ps");
  int i,j,k;
  vector<edge *> edges;
  vector<point *> corners;
  edges=edgeNeighbors(tris);
  corners=pointNeighbors(tris);
  for (j=0;j<corners.size();j++)
  {
    dumpFile<<net.revpoints[corners[j]]<<' ';
    dumpFile<<ldecimal(corners[j]->getx())<<' ';
    dumpFile<<ldecimal(corners[j]->gety())<<' ';
    dumpFile<<ldecimal(corners[j]->getz())<<'\n';
    br.include(*corners[j]);
  }
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  ps.startpage();
  ps.setscale(br);
  ps.setcolor(0,0,1);
  for (i=0;i<edges.size();i++)
    ps.line2p(*edges[i]->a,*edges[i]->b);
  ps.setcolor(1,0,0);
  for (i=0;i<tris.size();i++)
  {
    tri=tris[i];
    grad=tri->gradient(tri->centroid());
    dumpFile<<net.revpoints[tri->a]<<' '<<net.revpoints[tri->b]<<' '<<net.revpoints[tri->c];
    dumpFile<<" Area "<<tri->sarea<<" Slope "<<grad.length()<<" Acic "<<tri->acicularity()<<endl;
    for (k=0;k<tri->dots.size();k++)
    {
      dot=tri->dots[k];
      dumpFile<<"  "<<ldecimal(dot.getx())<<' '<<ldecimal(dot.gety())<<' '<<ldecimal(dot.getz())<<'\n';
      ps.dot(dot);
    }
  }
}
