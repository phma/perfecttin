/******************************************************/
/*                                                    */
/* threads.h - multithreading                         */
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

#include <boost/thread.hpp>
#include <vector>

// These are used as both commands to the threads and status from the threads.
#define TH_RUN 1
#define TH_PAUSE 2
#define TH_STOP 3

extern boost::mutex wingEdge;
extern double stageTolerance;

void markBucketClean(int bucket);
void markBucketDirty(int bucket);
bool allBucketsClean();
void resizeBuckets(int n);
void startThreads(int n);
void sleep(int thread);
void unsleep(int thread);
bool lockTriangles(int thread,std::vector<int> triangles);
void unlockTriangles(int thread);
void waitForThreads();

class TinThread
{
  void operator()(int thread);
};
