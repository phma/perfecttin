/******************************************************/
/*                                                    */
/* threads.h - multithreading                         */
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

#ifndef THREADS_H
#define THREADS_H

#ifdef __MINGW64__
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <../mingw-std-threads/mingw.thread.h>
#include <../mingw-std-threads/mingw.mutex.h>
#include <../mingw-std-threads/mingw.shared_mutex.h>
#else
#include <thread>
#include <mutex>
#include <shared_mutex>
#endif
#include <chrono>
#include <vector>
#include <array>
#include "fileio.h"

// These are used as both commands to the threads and status from the threads.
#define TH_RUN 1
#define TH_PAUSE 2
#define TH_WAIT 3
#define TH_STOP 4
#define TH_ASLEEP 256

// These are used to tell thread 0 to do things while threads are in wait state,
// or any thread to do things while threads are in pause state.
#define ACT_LOAD 1
#define ACT_OCTAGON 2
#define ACT_WRITE_DXF 3
#define ACT_WRITE_TIN 4
#define ACT_WRITE_PTIN 5
#define ACT_READ_PTIN 6
#define ACT_WRITE_CARLSON_TIN 7
#define ACT_DELETE_FILE 8
#define ACT_WRITE_LANDXML 9
#define ACT_QINDEX 10

#define RES_LOAD_PLY 1
#define RES_LOAD_LAS 2

struct ThreadAction
{
  int opcode;
  int param0;
  double param1; // measuring unit or tolerance
  double param2; // density
  std::string filename;
  int flags;
  /* Bit 0: write empty triangles when exporting TIN
   */
  int result;
  PtinHeader ptinResult;
};

extern std::mutex adjLog;
extern double stageTolerance;
extern int opcount,trianglesToPaint;
extern int currentAction;
extern std::chrono::steady_clock clk;
extern int mtxSquareSize;

void markBucketClean(int bucket);
void markBucketDirty(int bucket);
bool allBucketsClean();
void resizeBuckets(int n);
int nBuckets();
std::array<double,2> areaDone(double tolerance,double minArea);
double busyFraction();
bool livelock(double areadone,double rmsadj);
void startThreads(int n);
void joinThreads();
void enqueueAction(ThreadAction a);
ThreadAction dequeueResult();
bool actionQueueEmpty();
bool resultQueueEmpty();
void sleep(int thread);
void sleepDead(int thread);
void unsleep(int thread);
double maxSleepTime();
void randomizeSleep();
bool lockTriangles(int thread,std::vector<int> triangles);
void lockNewTriangles(int thread,int n);
void unlockTriangles(int thread);
void clearTriangleLocks();
void setThreadCommand(int newStatus);
int getThreadStatus();
void waitForThreads(int newStatus);
void waitForQueueEmpty();

class TinThread
{
public:
  void operator()(int thread);
};

#endif
