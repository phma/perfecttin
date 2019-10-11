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
#include "manysum.h"
using namespace std;
#ifdef __MINGW64__
using namespace mingw_stdthread;
namespace th=mingw_stdthread;
#else
namespace th=std;
#endif
namespace cr=std::chrono;

th::shared_mutex wingEdge; // Lock this while changing pointers in the winged edge structure.
map<int,th::mutex> triMutex; // Lock this while locking or unlocking triangles.
th::shared_mutex holderMutex; // for triangleHolders
th::mutex adjLog;
th::mutex actMutex;
th::mutex bucketMutex;
th::mutex startMutex;

int threadCommand;
vector<th::thread> threads;
vector<int> threadStatus; // Bit 8 indicates whether the thread is sleeping.
vector<double> sleepTime;
vector<int> triangleHolders; // one per triangle
vector<vector<int> > heldTriangles; // one list of triangles per thread
double stageTolerance;
queue<ThreadAction> actQueue;
int currentAction;
int mtxSquareSize;

cr::steady_clock clk;
vector<int> cleanBuckets;
/* Indicates whether the buckets used by areaDone are clean or dirty.
 * A bucket is clean if the last thing done was add it up; it is dirty
 * if a triangle whose area is added in the bucket has changed
 * since the bucket was added up.
 */
vector<double> allBuckets,doneBuckets,doneq2Buckets;
int opcount,trianglesToPaint;
const char statusNames[][8]=
{
  "None","Run","Pause","Wait","Stop"
};

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
  opcount++;
  trianglesToPaint=net.triangles.size()*3;
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
  allBuckets.resize(n);
  doneBuckets.resize(n);
  doneq2Buckets.resize(n);
  bucketMutex.unlock();
}

array<double,2> areaDone(double tolerance)
{
  vector<double> allTri,doneTri,doneq2Tri;
  static int overtimeCount=0,bucket=0;
  int i;
  triangle *tri;
  double allSum;
  array<double,2> ret;
  cr::nanoseconds elapsed;
  cr::time_point<cr::steady_clock> timeStart=clk.now();
  if (allBuckets.size()==0)
  {
    allBuckets.push_back(0);
    doneBuckets.push_back(0);
    resizeBuckets(1);
  }
  if (overtimeCount==8)
  {
    resizeBuckets(allBuckets.size()*2);
    overtimeCount=0;
    //cout<<allBuckets.size()<<" buckets \n";
  }
  markBucketClean(bucket);
  wingEdge.lock_shared();
  for (i=bucket;i<net.triangles.size();i+=allBuckets.size())
  {
    allTri.push_back(net.triangles[i].sarea);
    if (net.triangles[i].inTolerance(tolerance))
      doneTri.push_back(net.triangles[i].sarea);
    else
      tri=&net.triangles[i];
    if (net.triangles[i].inTolerance(M_SQRT2*tolerance))
      doneq2Tri.push_back(net.triangles[i].sarea);
    else
      tri=&net.triangles[i];
  }
  wingEdge.unlock_shared();
  allBuckets[bucket]=pairwisesum(allTri);
  doneBuckets[bucket]=pairwisesum(doneTri);
  doneq2Buckets[bucket]=pairwisesum(doneq2Tri);
  allSum=pairwisesum(allBuckets);
  ret[0]=pairwisesum(doneBuckets)/allSum;
  ret[1]=pairwisesum(doneq2Buckets)/allSum;
  markBucketClean(bucket);
  elapsed=clk.now()-timeStart;
  if (elapsed>cr::milliseconds(20))
    overtimeCount++;
  else
    overtimeCount=0;
  bucket=(bucket+1)%allBuckets.size();
  return ret;
}

double busyFraction()
{
  int i,numBusy=0;
  for (i=0;i<threadStatus.size();i++)
    if ((threadStatus[i]&256)==0)
      numBusy++;
  return (double)numBusy/i;
}

bool livelock(double areadone,double rmsadj)
{
  static double lastAreaDone,lastRmsAdj;
  static int unchangedCount;
  if (lastAreaDone==areadone && lastRmsAdj==rmsadj && maxSleepTime()<100 && allBucketsClean())
    unchangedCount++;
  else
    unchangedCount=0;
  lastAreaDone=areadone;
  lastRmsAdj=rmsadj;
  return unchangedCount>=5;
}

void startThreads(int n)
{
  int i,m;
  threadCommand=TH_WAIT;
  heldTriangles.resize(n);
  sleepTime.resize(n);
  initTempPointlist(n);
  mtxSquareSize=ceil(sqrt(33*n));
  m=mtxSquareSize*mtxSquareSize;
  for (i=0;i<m;i++)
    triMutex[i];
  for (i=0;i<n;i++)
  {
    threads.push_back(th::thread(TinThread(),i));
    th::this_thread::sleep_for(chrono::milliseconds(10));
  }
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
    currentAction=ret.opcode;
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
  th::this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
  threadStatus[thread]&=255;
}

void sleepDead(int thread)
// Sleep to try to get out of deadlock.
{
  sleepTime[thread]=sleepTime[thread]*(1+1./net.triangles.size())+0.1;
  threadStatus[thread]|=256;
  th::this_thread::sleep_for(chrono::milliseconds(lrint(sleepTime[thread])));
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

set<int> whichLocks(vector<int> triangles)
{
  set<int> ret;
  triangle *tri;
  point *a,*b,*c;
  int i;
  wingEdge.lock_shared();
  for (i=0;i<triangles.size();i++)
  {
    tri=&net.triangles[triangles[i]];
    a=tri->a;
    b=tri->b;
    c=tri->c;
    if (a)
      ret.insert(mtxSquare(*a));
    if (b)
      ret.insert(mtxSquare(*b));
    if (c)
      ret.insert(mtxSquare(*c));
  }
  wingEdge.unlock_shared();
  if (ret.size()==0)
    ret.insert(0);
  return ret;
}

bool lockTriangles(int thread,vector<int> triangles)
/* Either it locks all the triangles, and returns true,
 * or it locks nothing, and returns false.
 *
 * holderMutex is there to prevent one thread from accessing triangleHolders
 * while another thread is moving it to resize it. It is *not* there to prevent
 * two threads from accessing the same element of triangleHolders at the same
 * time; that's what triMutex is for. triMutex divides the plane into squares,
 * so when the triangles are small, it's likely that all the triangles being
 * locked at once are covered by one triMutex.
 */
{
  bool ret=true;
  int i;
  int origSz;
  set<int> lockSet=whichLocks(triangles);
  set<int>::iterator j;
  for (j=lockSet.begin();j!=lockSet.end();j++)
    triMutex[*j].lock();
  origSz=heldTriangles[thread].size();
  for (i=0;ret && i<triangles.size();i++)
  {
    if (triangles[i]>=triangleHolders.size())
    {
      holderMutex.lock();
      if (triangles[i]>=triangleHolders.size()) // in case another thread resizes it
        triangleHolders.resize(triangles[i]+1,-1);
      holderMutex.unlock();
    }
    holderMutex.lock_shared();
    heldTriangles[thread].push_back(triangles[i]);
    if (triangleHolders[triangles[i]]>=(signed)heldTriangles.size())
    {
      cerr<<"triangleHolders["<<triangles[i]<<"] contains garbage\n";
      triangleHolders[triangles[i]]=-1;
    }
    if (triangleHolders[triangles[i]]>=0 && triangleHolders[triangles[i]]!=thread)
      ret=false;
    holderMutex.unlock_shared();
  }
  if (!ret)
    heldTriangles[thread].resize(origSz);
  holderMutex.lock_shared();
  for (i=0;ret && i<triangles.size();i++)
    triangleHolders[triangles[i]]=thread;
  holderMutex.unlock_shared();
  for (j=lockSet.begin();j!=lockSet.end();j++)
    triMutex[*j].unlock();
  return ret;
}

void unlockTriangles(int thread)
{
  int i;
  set<int> lockSet=whichLocks(heldTriangles[thread]);
  set<int>::iterator j;
  for (j=lockSet.begin();j!=lockSet.end();j++)
    triMutex[*j].lock();
  holderMutex.lock_shared();
  /* It is possible somehow for heldTriangles to hold a number of a triangle
   * that exists, but hasn't been added to triangleHolders yet. In this case,
   * assume that the nonexistent entry contains -1 and don't bother setting it.
   */
  for (i=0;i<heldTriangles[thread].size();i++)
    if (heldTriangles[thread][i]<triangleHolders.size() &&
        triangleHolders[heldTriangles[thread][i]]==thread)
      triangleHolders[heldTriangles[thread][i]]=-1;
  holderMutex.unlock_shared();
  heldTriangles[thread].clear();
  for (j=lockSet.begin();j!=lockSet.end();j++)
    triMutex[*j].unlock();
}

void clearTriangleLocks()
{
  int i;
  holderMutex.lock();
  triangleHolders.clear();
  triangleHolders.shrink_to_fit();
  for (i=0;i<heldTriangles.size();i++)
    heldTriangles[i].clear();
  holderMutex.unlock();
}

void setThreadCommand(int newStatus)
{
  threadCommand=newStatus;
  //cout<<statusNames[newStatus]<<endl;
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
    th::this_thread::sleep_for(chrono::milliseconds(n));
  } while (n);
}

void waitForQueueEmpty()
// Waits until the action queue is empty and all threads have completed their actions.
{
  int i,n;
  do
  {
    n=actQueue.size();
    for (i=0;i<threadStatus.size();i++)
      if (threadStatus[i]<256)
	n++;
    th::this_thread::sleep_for(chrono::milliseconds(n));
  } while (n);
}

void TinThread::operator()(int thread)
{
  int e=0,t=0,d=0;
  int triResult,edgeResult;
  edge *edg;
  triangle *tri;
  ThreadAction act;
  startMutex.lock();
  if (threadStatus.size()!=thread)
  {
    cout<<"Starting thread "<<threadStatus.size()<<", was passed "<<thread<<endl;
    thread=threadStatus.size();
  }
  threadStatus.push_back(0);
  startMutex.unlock();
  while (threadCommand!=TH_STOP)
  {
    if (threadCommand==TH_RUN)
    {
      threadStatus[thread]=TH_RUN;
      if (net.edges.size() && net.triangles.size())
      {
	wingEdge.lock_shared();
	e=(e+relprime(net.edges.size(),thread))%net.edges.size();
	edg=&net.edges[e];
	t=(t+relprime(net.triangles.size(),thread))%net.triangles.size();
	tri=&net.triangles[t];
	wingEdge.unlock_shared();
	edgeResult=edgeop(edg,stageTolerance,thread);
	triResult=triop(tri,stageTolerance,thread);
      }
      else
	triResult=edgeResult=2;
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
	case ACT_WRITE_PTIN:
	  writePtin(act.filename,act.param0,act.param1);
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
	  readCloud(act.filename,act.param1);
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
	case ACT_WRITE_PTIN:
	  cerr<<"Can't write PTIN in wait state\n";
	  unsleep(thread);
	  break;
	default:
	  sleep(thread);
      }
    }
  }
  threadStatus[thread]=TH_STOP;
}
