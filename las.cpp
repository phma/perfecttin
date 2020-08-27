/******************************************************/
/*                                                    */
/* las.cpp - laser point cloud files                  */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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

#include <cstring>
#include "las.h"
#include "binio.h"
#include "octagon.h"
#include "angle.h"
#include "cloud.h"

const int MASK_GPSTIME=0x7fa;
const int MASK_RGB=0x5ac;
const int MASK_WAVE=0x630;

using namespace std;

string read32(ifstream &file)
{
  char buf[40];
  memset(buf,0,40);
  file.read(buf,32);
  return string(buf);
}

LasHeader::LasHeader()
{
  lasfile=nullptr;
  versionMajor=versionMinor=0;
}

LasHeader::~LasHeader()
{
  close();
}

void LasHeader::open(std::string fileName)
{
  int magicBytes;
  int headerSize;
  int i;
  size_t total;
  unsigned int legacyNPoints[6];
  int whichNPoints=15;
  if (lasfile)
    close();
  lasfile=new ifstream(fileName,ios::binary);
  magicBytes=readbeint(*lasfile);
  if (magicBytes==0x4c415346)
  {
    sourceId=readleshort(*lasfile);
    globalEncoding=readleshort(*lasfile);
    guid1=readleint(*lasfile);
    guid2=readleshort(*lasfile);
    guid3=readleshort(*lasfile);
    lasfile->read(guid4,8);
    versionMajor=lasfile->get();
    versionMinor=lasfile->get();
    systemId=read32(*lasfile);
    softwareName=read32(*lasfile);
    creationDay=readleshort(*lasfile);
    creationYear=readleshort(*lasfile);
    headerSize=readleshort(*lasfile);
    pointOffset=readleint(*lasfile);
    nVariableLength=readleint(*lasfile);
    pointFormat=lasfile->get();
    pointLength=readleshort(*lasfile);
    for (i=0;i<6;i++)
      legacyNPoints[i]=readleint(*lasfile);
    xScale=readledouble(*lasfile);
    yScale=readledouble(*lasfile);
    zScale=readledouble(*lasfile);
    xOffset=readledouble(*lasfile);
    yOffset=readledouble(*lasfile);
    zOffset=readledouble(*lasfile);
    maxX=readledouble(*lasfile);
    minX=readledouble(*lasfile);
    maxY=readledouble(*lasfile);
    minY=readledouble(*lasfile);
    maxZ=readledouble(*lasfile);
    minZ=readledouble(*lasfile);
    // Here ends the LAS 1.2 header (length 0xe3). The LAS 1.4 header (length 0x177) continues.
    if (headerSize>0xe3)
    {
      startWaveform=readlelong(*lasfile);
      startExtendedVariableLength=readlelong(*lasfile);
      nExtendedVariableLength=readleint(*lasfile);
      for (i=0;i<16;i++)
	nPoints[i]=readlelong(*lasfile);
    }
    else
    {
      startWaveform=startExtendedVariableLength=nExtendedVariableLength=0;
      for (i=0;i<16;i++)
	nPoints[i]=0;
    }
    /* Check the numbers of points by return. They should add up to the total:
     * 986 377 233 144 89 55 34 21 13 8 5 3 2 1 1 0
     * In some files, the total (nPoints[0], legacyNPoints[0]) is nonzero,
     * but the numbers of points by return are all 0. This is invalid, but
     * seen in the wild in files produced by photogrammetry.
     */
    for (i=1,total=0;i<6;i++)
    {
      total+=legacyNPoints[i];
      if (legacyNPoints[i]>legacyNPoints[0])
	whichNPoints&=~5;
    }
    if (total!=legacyNPoints[0])
      whichNPoints&=~1;
    if (total!=0 || legacyNPoints[0]==0)
      whichNPoints&=~4;
    for (i=1,total=0;i<16;i++)
    {
      total+=nPoints[i];
      if (nPoints[i]>nPoints[0])
	whichNPoints&=~10;
    }
    if (total!=nPoints[0])
      whichNPoints&=~2;
    if (total!=0 || nPoints[0]==0)
      whichNPoints&=~8;
    for (i=0;i<6;i++)
    {
      if (legacyNPoints[i]!=nPoints[i] && legacyNPoints[i]!=0)
	whichNPoints&=~10;
      if (nPoints[i]!=legacyNPoints[i] && nPoints[i]!=0)
	whichNPoints&=~5;
    }
    if (whichNPoints>0 && (whichNPoints&3)==0)
    {
      cerr<<"Number of points by return are all 0. Setting first return to all points.\n";
      nPoints[1]=nPoints[0];
      legacyNPoints[1]=legacyNPoints[0];
    }
    if (whichNPoints==1 || whichNPoints==4)
      for (i=0;i<6;i++)
	nPoints[i]=legacyNPoints[i];
    if (whichNPoints==0)
      for (i=0;i<6;i++)
	nPoints[i]=0;
    if (pointLength==0)
      versionMajor=versionMinor=nPoints[0]=0;
  }
  else // file does not begin with "LASF"
    versionMajor=versionMinor=nPoints[0]=0;
}

bool LasHeader::isValid()
{
  return versionMajor>0 && versionMinor>0;
}

void LasHeader::close()
{
  delete(lasfile);
  lasfile=nullptr;
}

size_t LasHeader::numberPoints(int r)
{
  return nPoints[r];
}

LasPoint LasHeader::readPoint(size_t num)
{
  LasPoint ret;
  int xInt,yInt,zInt;
  int temp;
  lasfile->seekg(num*pointLength+pointOffset,ios_base::beg);
  xInt=readleint(*lasfile);
  yInt=readleint(*lasfile);
  zInt=readleint(*lasfile);
  ret.intensity=readleshort(*lasfile);
  if (pointFormat<6)
  {
    temp=lasfile->get();
    ret.returnNum=temp&7;
    ret.nReturns=(temp>>3)&7;
    ret.scanDirection=(temp>>6)&1;
    ret.edgeLine=(temp>>7)&1;
    ret.classification=(unsigned char)lasfile->get();
    ret.scanAngle=degtobin((signed char)lasfile->get());
    ret.userData=(unsigned char)lasfile->get();
    ret.pointSource=readleshort(*lasfile);
    if ((1<<pointFormat)&MASK_GPSTIME) // 5, 4, 3, or 1
      ret.gpsTime=readledouble(*lasfile);
    if ((1<<pointFormat)&MASK_RGB) // 5, 3, or 2
    {
      ret.red=readleshort(*lasfile);
      ret.green=readleshort(*lasfile);
      ret.blue=readleshort(*lasfile);
    }
    if ((1<<pointFormat)&MASK_WAVE) // 5 or 4
    {
      ret.waveIndex=(unsigned char)lasfile->get();
      ret.waveformOffset=readlelong(*lasfile);
      ret.waveformSize=readleint(*lasfile);
      ret.waveformTime=readlefloat(*lasfile);
      ret.xDir=readlefloat(*lasfile);
      ret.yDir=readlefloat(*lasfile);
      ret.zDir=readlefloat(*lasfile);
    }
  }
  else // formats 6 through 10
  {
    temp=lasfile->get();
    ret.returnNum=temp&15;
    ret.nReturns=(temp>>4)&15;
    temp=lasfile->get();
    ret.classificationFlags=temp&15;
    ret.scannerChannel=(temp>>4)&3;
    ret.scanDirection=(temp>>6)&1;
    ret.edgeLine=(temp>>7)&1;
    ret.classification=(unsigned char)lasfile->get();
    ret.userData=(unsigned char)lasfile->get();
    ret.scanAngle=degtobin(readleshort(*lasfile)*0.006);
    ret.pointSource=readleshort(*lasfile);
    ret.gpsTime=readledouble(*lasfile);
    // switch(pointFormat)...
  }
  ret.location=xyz(xOffset+xScale*xInt,yOffset+yScale*yInt,zOffset+zScale*zInt);
  if (!lasfile->good())
    throw -1;
  return ret;
}

void readLas(string fileName)
{
  size_t i;
  LasHeader header;
  LasPoint pnt;
  header.open(fileName);
  //cout<<"File contains "<<header.numberPoints()<<" dots\n";
  for (i=0;i<header.numberPoints();i++)
  {
    pnt=header.readPoint(i);
    //if (i%1000000==0)
      //cout<<"return "<<pnt.returnNum<<" of "<<pnt.nReturns<<endl;
    cloud.push_back(pnt.location);
  }
}
