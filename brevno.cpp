/******************************************************/
/*                                                    */
/* brevno.cpp - Mitobrevno interface                  */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
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
#include "brevno.h"
using namespace std;

#if defined(Mitobrevno_FOUND) && defined(ENABLE_MITOBREVNO)
#define MB_START_THREAD 0
#define MB_BEGIN_SPLIT 0x2000
#define MB_END_SPLIT 0x3000
using namespace mitobrevno;

void openThreadLog()
{
  openLogFile("thread.log");
  describeEvent(MB_BEGIN_SPLIT,"split triangle");
  describeParam(MB_BEGIN_SPLIT,0,"triangle");
  formatParam(MB_BEGIN_SPLIT,1,0);
  describeEvent(MB_START_THREAD,"start thread");
}

void logStartThread()
{
  vector<int> intParams;
  vector<float> floatParams;
  logEvent(MB_START_THREAD,intParams,floatParams);
}

void logBeginSplit(int tri)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(tri);
  logEvent(MB_BEGIN_SPLIT,intParams,floatParams);
}

void logEndSplit(int tri)
{
  vector<int> intParams;
  vector<float> floatParams;
  intParams.push_back(tri);
  logEvent(MB_END_SPLIT,intParams,floatParams);
}

void writeBufLog()
{
  writeBufferedLog();
}

#endif
