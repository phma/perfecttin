/******************************************************/
/*                                                    */
/* relprime.cpp - relatively prime numbers            */
/*                                                    */
/******************************************************/
/* Copyright 2019 Pierre Abbat.
 * This file is part of PerfectTIN.
 *
 * PerfectTIN is free software: you can redistribute it and/or modify
 * modify it under the terms of the GNU Lesser General Public License as
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
#include <map>
#include <cmath>
#include "relprime.h"
#include "threads.h"

using namespace std;

const double quadirr[]=
{
#include "quadirr.txt"
};

map<uint64_t,unsigned> relprimes;
shared_mutex primeMutex;

unsigned gcd(unsigned a,unsigned b)
{
  while (a&&b)
  {
    if (a>b)
    {
      b^=a;
      a^=b;
      b^=a;
    }
    b%=a;
  }
  return a+b;
}

unsigned relprime(unsigned n,int thread)
/* Returns an integer relatively prime to n and close to n*q, where
 * q is the threadth quadratic irrational in the Quadlods order.
 */
{
  unsigned ret=0,twice;
  double phin;
  uint64_t inx=((uint64_t)thread<<32)+n;
  thread%=6542;
  if (thread<0)
    thread+=6542;
  primeMutex.lock_shared();
  if (relprimes.count(inx))
    ret=relprimes[inx];
  primeMutex.unlock_shared();
  if (!ret)
  {
    phin=n*quadirr[thread];
    ret=lrint(phin);
    twice=2*ret-(ret>phin);
    while (gcd(ret,n)!=1 || (ret>n && n>0))
      ret=twice-ret+(ret<=phin);
    primeMutex.lock();
    relprimes[inx]=ret;
    primeMutex.unlock();
  }
  return ret;
}
