/******************************************************/
/*                                                    */
/* unifiro.h - unique first-in random-out             */
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
#ifndef UNIFIRO_H
#define UNIFIRO_H
#include <set>
#include <vector>
#include "relprime.h"
#include "mthreads.h"

template <typename T> class Unifiro
{ // T should be a pointer type
private:
  std::set<T> uni;
  std::vector<T> firo;
  std::mutex m;
  size_t pos;
public:
  T dequeue()
  {
    T ret=nullptr;
    if (firo.size())
    {
      m.lock();
      ret=firo.back();
      uni.erase(ret);
      firo.pop_back();
      m.unlock();
    }
    return ret;
  }
  void enqueue(T t,int thread)
  {
    m.lock();
    if (uni.count(t)==0)
    {
      uni.insert(t);
      firo.push_back(t);
      pos=(pos+relprime(firo.size(),thread))%firo.size();
      swap(firo.back(),firo[pos]);
    }
    m.unlock();
  }
};
#endif
