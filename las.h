/******************************************************/
/*                                                    */
/* las.h - laser point cloud files                    */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of Decisite.
 * 
 * Decisite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Decisite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Decisite. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <iostream>
#include "point.h"

struct LasPoint
{
  xyz location;
  unsigned short intensity;
  unsigned short returnNum,nReturns;
  bool scanDirection,edgeLine;
  unsigned short classification,classificationFlags;
  unsigned short scannerChannel;
  unsigned short userData;
  short scanAngle;
  unsigned short waveIndex;
  double gpsTime;
  unsigned short nir,red,green,blue;
  size_t waveformOffset;
  float waveformTime,xDir,yDir,zDir;
};

class LasHeader
{
private:
  std::ifstream *lasfile;
  unsigned short sourceId,globalEncoding;
  unsigned int guid1;
  unsigned short guid2,guid3;
  char guid4[8];
  char versionMajor,versionMinor;
  std::string systemId,softwareName;
  unsigned short creationDay,creationYear;
  unsigned int pointOffset;
  unsigned int nVariableLength;
  unsigned short pointFormat,pointLength;
  double xScale,yScale,zScale,xOffset,yOffset,zOffset;
  double maxX,minX,maxY,minY,maxZ,minZ;
  size_t startWaveform,startExtendedVariableLength;
  unsigned int nExtendedVariableLength;
  size_t nPoints[16]; // [0] is total; [1]..[15] are each return
public:
  LasHeader();
  void open(std::string fileName);
  bool isValid();
  void close();
  size_t numberPoints(int r=0);
  LasPoint readPoint(size_t num);
};
