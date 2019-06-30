/******************************************************/
/*                                                    */
/* threads.cpp - multithreading                       */
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
#include "threads.h"
using namespace std;

boost::mutex wingEdge; // Lock this while changing pointers in the winged edge structure.
boost::mutex triMutex; // Lock this while locking or unlocking triangles.
int threadCommand;
vector<int> threadStatus; // Bit 8 indicates whether the thread is sleeping.

vector<int> cleanBuckets;
/* Indicates whether the buckets used by areaDone are clean or dirty.
 * A bucket is clean if the last thing done was add it up; it is dirty
 * if a triangle whose area is added in the bucket has changed
 * since the bucket was added up.
 */

void markBucketClean(int bucket)
{
  bucket&=(cleanBuckets.size()-1); // size is always a power of 2
  if (cleanBuckets[bucket]<2)
    cleanBuckets[bucket]++;
}

void markBucketDirty(int bucket)
{
  bucket&=(cleanBuckets.size()-1); // size is always a power of 2
  cleanBuckets[bucket]=0;
}

bool allBucketsClean()
{
  int i;
  bool ret=true;
  for (i=0;ret && i<cleanBuckets.size();i++)
    if (cleanBuckets[i]<2)
      ret=false;
  return ret;
}

void resizeBuckets(int n)
{
  cleanBuckets.resize(n);
}

