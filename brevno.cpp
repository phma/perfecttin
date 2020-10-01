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
using namespace mitobrevno;

#ifdef Mitobrevno_FOUND
#define MB_BEGIN_SPLIT 0x2000
#define MB_END_SPLIT 0x3000

void openThreadLog()
{
  openLogFile("thread.log");
  describeEvent(MB_BEGIN_SPLIT,"split triangle");
  describeParam(0,"triangle");
}

void logBeginSplit(int thread,int tri)
{
  logEvent(MB_BEGIN_SPLIT,tri);
}

void logEndSplit(int thread,int tri)
{
  logEvent(MB_END_SPLIT,tri);
}

void writeBufLog()
{
  writeBufferedLog();
}

#endif
