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
#include <string>

#define PT_UNKNOWN_HEADER_FORMAT -1
#define PT_NOT_PTIN_FILE -2

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

std::string noExt(std::string fileName);
std::string baseName(std::string fileName);
void writeDxf(std::string outputFile,bool asc,double outUnit);
void readCloud(std::string inputFile,double inUnit);
void writePtin(std::string outputFile,int tolRatio,double tolerance);
PtinHeader readPtinHeader(std::istream &inputFile);
PtinHeader readPtinHeader(std::string inputFile);
PtinHeader readPtin(std::string inputFile);
