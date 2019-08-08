/******************************************************/
/*                                                    */
/* threads.cpp - multithreading                       */
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
#include <queue>
#include "threads.h"
#include "edgeop.h"
#include "triop.h"
#include "octagon.h"
#include "random.h"
#include "fileio.h"
#include "tintext.h"
#include "las.h"
#include "relprime.h"
using namespace std;
using namespace boost;

mutex wingEdge; // Lock this while changing pointers in the winged edge structure.
mutex triMutex; // Lock this while locking or unlocking triangles.
mutex adjLog;
mutex actMutex;
mutex bucketMutex;

int threadCommand;
vector<thread> threads;
vector<int> threadStatus; // Bit 8 indicates whether the thread is sleeping.
vector<double> sleepTime;
vector<int> triangleHolders; // one per triangle
vector<vector<int> > heldTriangles; // one list of triangles per thread
double stageTolerance;
queue<ThreadAction> actQueue;

vector<int> cleanBuckets;
/* Indicates whether the buckets used by areaDone are clean or dirty.
 * A bucket is clean if the last thing done was add it up; it is dirty
 * if a triangle whose area is added in the bucket has changed
 * since the bucket was added up.
 */

void markBucketClean(int bucket)
{
  bucketMutex.lock();
  bucket&=(cleanBuckets.size()-1); // size is always a power of 2
  if (cleanBuckets[bucket]<2)
    cleanBuckets[bucket]++;
  bucketMutex.unlock();
}

void markBucketDirty(int bucket)
{
  bucketMutex.lock();
  bucket&=(cleanBuckets.size()-1); // size is always a power of 2
  cleanBuckets[bucket]=0;
  bucketMutex.unlock();
}

bool allBucketsClean()
{
  int i;
  bool ret=true;
  bucketMutex.lock();
  for (i=0;ret && i<cleanBuckets.size();i++)
    if (cleanBuckets[i]<2)
      ret=false;
  bucketMutex.unlock();
  return ret;
}

void resizeBuckets(int n)
{
  bucketMutex.lock();
  cleanBuckets.resize(n);
  bucketMutex.unlock();
}

void startThreads(int n)
{
  int i;
  threadCommand=TH_WAIT;
  threadStatus.resize(n);
  heldTriangles.resize(n);
  sleepTime.resize(n);
  initTempPointlist(n);
  for (i=0;i<n;i++)
    threads.push_back(thread(TinThread(),i));
}

void joinThreads()
{
  int i;
  for (i=0;i<threads.size();i++)
    threads[i].join();
}

ThreadAction dequeueAction()
{
  ThreadAction ret;
  ret.opcode=0;
  actMutex.lock();
  if (actQueue.size())
  {
    ret=actQueue.front();
    actQueue.pop();
  }
  actMutex.unlock();
  return ret;
}

void enqueueAction(ThreadAction a)
{
  actMutex.lock();
  actQueue.push(a);
  actMutex.unlock();
}

bool actionQueueEmpty()
{
  return actQueue.size()==0;
}

void sleep(int thread)
{
  sleepTime[thread]+=1;
  if (sleepTime[thread]>1000)
    sleepTime[thread]=1000;
  threadStatus[thread]|=256;
  this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
  threadStatus[thread]&=255;
}

void sleepDead(int thread)
// Sleep to try to get out of deadlock.
{
  sleepTime[thread]=sleepTime[thread]*(1+1./net.triangles.size())+0.1;
  threadStatus[thread]|=256;
  this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
  threadStatus[thread]&=255;
}

void unsleep(int thread)
{
  sleepTime[thread]-=1;
  if (sleepTime[thread]<0 || std::isnan(sleepTime[thread]))
    sleepTime[thread]=0;
}

double maxSleepTime()
{
  int i;
  double max=0;
  for (i=0;i<sleepTime.size();i++)
    if (sleepTime[i]>max)
      max=sleepTime[i];
  return max;
}

void randomizeSleep()
{
  int i;
  for (i=0;i<sleepTime.size();i++)
    sleepTime[i]=rng.usrandom()/32.768;
}

bool lockTriangles(int thread,vector<int> triangles)
/* Either it locks all the triangles, and returns true,
 * or it locks nothing, and returns false.
 */
{
  bool ret=true;
  int i;
  triMutex.lock();
  for (i=0;i<triangles.size();i++)
  {
    while (triangles[i]>=triangleHolders.size())
      triangleHolders.push_back(-1);
    heldTriangles[thread].push_back(triangles[i]);
    if (triangleHolders[triangles[i]]>=0 && triangleHolders[triangles[i]]!=thread)
      ret=false;
  }
  for (i=0;ret && i<triangles.size();i++)
    triangleHolders[triangles[i]]=thread;
  triMutex.unlock();
  return ret;
}

void unlockTriangles(int thread)
{
  int i;
  triMutex.lock();
  for (i=0;i<heldTriangles[thread].size();i++)
    if (triangleHolders[heldTriangles[thread][i]]==thread)
      triangleHolders[heldTriangles[thread][i]]=-1;
  heldTriangles[thread].clear();
  triMutex.unlock();
}

void setThreadCommand(int newStatus)
{
  threadCommand=newStatus;
}

int getThreadStatus()
/* Returns aaaaaaaaaabbbbbbbbbbcccccccccc where
 * aaaaaaaaaa is the status all threads should be in,
 * bbbbbbbbbb is 0 if all threads are in the same state, and
 * cccccccccc is the state the threads are in.
 * If all threads are in the commanded state, but some may be asleep and others awake,
 * getThreadStatus()&0x3ffbfeff is a multiple of 1048577.
 */
{
  int i,oneStatus,minStatus=-1,maxStatus=0;
  for (i=0;i<threadStatus.size();i++)
  {
    oneStatus=threadStatus[i];
    maxStatus|=oneStatus;
    minStatus&=oneStatus;
  }
  return (threadCommand<<20)|((minStatus^maxStatus)<<10)|(minStatus&0x3ff);
}

void waitForThreads(int newStatus)
// Waits until all threads are in the commanded status.
{
  int i,n;
  threadCommand=newStatus;
  do
  {
    for (i=n=0;i<threadStatus.size();i++)
      if ((threadStatus[i]&255)!=threadCommand)
	n++;
    this_thread::sleep_for(chrono::milliseconds(n));
  } while (n);
}

void TinThread::operator()(int thread)
{
  int i,e=0,t=0,d=0;
  int triResult,edgeResult;
  ThreadAction act;
  while (threadCommand!=TH_STOP)
  {
    if (threadCommand==TH_RUN)
    {
      threadStatus[thread]=TH_RUN;
      edgeResult=edgeop(&net.edges[e],stageTolerance,thread);
      e=(e+relprime(net.edges.size(),thread))%net.edges.size();
      triResult=triop(&net.triangles[t],stageTolerance,thread);
      t=(t+relprime(net.triangles.size(),thread))%net.triangles.size();
      if (triResult==2 || edgeResult==2) // deadlock
	sleepDead(thread);
      else if (triResult==1 || edgeResult==1)
	sleep(thread);
      else
	unsleep(thread);
    }
    if (threadCommand==TH_PAUSE)
    { // The job is ongoing, but has to pause to write out the files.
      threadStatus[thread]=TH_PAUSE;
      if (thread)
	act.opcode=0;
      else
	act=dequeueAction();
      switch (act.opcode)
      {
	case ACT_LOAD:
	  cerr<<"Can't load a file in pause state\n";
	  unsleep(thread);
	  break;
	case ACT_OCTAGON:
	  cerr<<"Can't make octagon in pause state\n";
	  unsleep(thread);
	  break;
	case ACT_WRITE_DXF:
	  writeDxf(act.filename,act.param0,act.param1);
	  unsleep(thread);
	  break;
	case ACT_WRITE_TIN:
	  writeTinText(act.filename,act.param1);
	  unsleep(thread);
	  break;
	default:
	  sleep(thread);
      }
    }
    if (threadCommand==TH_WAIT)
    { // There is no job. The threads are waiting for a job.
      threadStatus[thread]=TH_WAIT;
      if (thread)
	act.opcode=0;
      else
	act=dequeueAction();
      switch (act.opcode)
      {
	case ACT_LOAD:
	  readLas(act.filename);
	  unsleep(thread);
	  break;
	case ACT_OCTAGON:
	  stageTolerance=-makeOctagon();
	  if (!std::isfinite(stageTolerance))
	  {
	    cerr<<"Point cloud covers no area or has infinite or NaN points\n";
	    //done=true;
	  }
	  unsleep(thread);
	  break;
	case ACT_WRITE_DXF:
	  cerr<<"Can't write DXF in wait state\n";
	  unsleep(thread);
	  break;
	case ACT_WRITE_TIN:
	  cerr<<"Can't write TIN in wait state\n";
	  unsleep(thread);
	  break;
	default:
	  sleep(thread);
      }
    }
  }
  threadStatus[thread]=TH_STOP;
}
