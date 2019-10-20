/******************************************************/
/*                                                    */
/* fileio.h - file I/O                                */
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
#ifndef FILEIO_H
#define FILEIO_H
#include <string>
#include "manysum.h"

#define PT_UNKNOWN_HEADER_FORMAT -1
#define PT_NOT_PTIN_FILE -2
#define PT_COUNT_MISMATCH -3
#define PT_INVALID_POINT_NUMBER -4
#define PT_BACKWARD_TRIANGLE -5
#define PT_INVALID_CONVEX_HULL -6
#define PT_EOF -7
#define PT_EDGE_MISMATCH -8
#define PT_DOT_OUTSIDE -9
/* Unknown header format: file was written by a newer version of PerfectTIN.
 * Not ptin file: file is not a PerfectTIN file.
 * Count mismatch: file is not a PerfectTIN file.
 * Any other negative tolRatio value: file is corrupt.
 * tolRatio>0 but tolerance is NaN: file was incompletely written.
 */

struct PtinHeader
{
  PtinHeader();
  time_t conversionTime;
  double tolerance; // NaN means file wasn't finished being written
  int tolRatio; // negative means an error
  int numPoints;
  int numConvexHull;
  int numTriangles;
};

class CoordCheck
{
private:
  size_t count;
  manysum sums[64];
  double stage0[14][8192],stage1[27][8192],stage2[40][8192],
         stage3[53][8192],stage4[64][4096];
public:
  void clear();
  CoordCheck& operator<<(double val);
  double operator[](int n);
  size_t getCount()
  {
    return count;
  }
};

std::string noExt(std::string fileName);
std::string baseName(std::string fileName);
void writeDxf(std::string outputFile,bool asc,double outUnit);
void readCloud(std::string inputFile,double inUnit);
void writePtin(std::string outputFile,int tolRatio,double tolerance);
PtinHeader readPtinHeader(std::istream &inputFile);
PtinHeader readPtinHeader(std::string inputFile);
PtinHeader readPtin(std::string inputFile);
#endif
