/******************************************************/
/*                                                    */
/* relprime.cpp - relatively prime numbers            */
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
#include <map>
#include <cmath>
#include "relprime.h"

using namespace std;

map<unsigned,unsigned> relprimes;

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

unsigned relprime(unsigned n)
// Returns the integer closest to n/φ of those relatively prime to n.
{
  unsigned ret,twice;
  double phin;
  ret=relprimes[n];
  if (!ret)
  {
    phin=n*M_1PHI;
    ret=rint(phin);
    twice=2*ret-(ret>phin);
    while (gcd(ret,n)!=1)
      ret=twice-ret+(ret<=phin);
    relprimes[n]=ret;
  }
  return ret;
}
