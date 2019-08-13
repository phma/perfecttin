/******************************************************/
/*                                                    */
/* threads.h - multithreading                         */
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

#include <boost/thread.hpp>
#include <vector>
#include <array>

// These are used as both commands to the threads and status from the threads.
#define TH_RUN 1
#define TH_PAUSE 2
#define TH_WAIT 3
#define TH_STOP 4
#define TH_ASLEEP 256

// These are used to tell thread 0 to do things while threads are in pause or wait state.
#define ACT_LOAD 1
#define ACT_OCTAGON 2
#define ACT_WRITE_DXF 3
#define ACT_WRITE_TIN 4

struct ThreadAction
{
  int opcode;
  int param0;
  double param1;
  std::string filename;
};

extern boost::mutex wingEdge;
extern boost::mutex adjLog;
extern double stageTolerance;
extern int opcount,trianglesToPaint;
extern int currentAction;
extern boost::chrono::steady_clock clk;

void markBucketClean(int bucket);
void markBucketDirty(int bucket);
bool allBucketsClean();
void resizeBuckets(int n);
std::array<double,2> areaDone(double tolerance);
double busyFraction();
bool livelock(double areadone,double rmsadj);
void startThreads(int n);
void joinThreads();
void enqueueAction(ThreadAction a);
bool actionQueueEmpty();
void sleep(int thread);
void sleepDead(int thread);
void unsleep(int thread);
double maxSleepTime();
void randomizeSleep();
bool lockTriangles(int thread,std::vector<int> triangles);
void unlockTriangles(int thread);
void setThreadCommand(int newStatus);
int getThreadStatus();
void waitForThreads(int newStatus);

class TinThread
{
public:
  void operator()(int thread);
};
