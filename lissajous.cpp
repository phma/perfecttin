/******************************************************/
/*                                                    */
/* lissajous.cpp - Lissajous curves                   */
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
#include <iostream>
#include "lissajous.h"
#include "relprime.h"
using namespace std;

// num[i]/denom[i] is a rational approximation to quadirr[i].
int Lissajous::num[]=
{
  987,2131,1393,1519,1189,1801,1027,
  2111,135,379,1693,1249,357,365,
  1871,2069,659,1905,1171,1287,3431
};

int Lissajous::denom[]=
{
  1597,2911,3363,2705,3927,2789,1897,
  3009,701,1197,2193,3083,2549,1017,
  2351,2873,3037,3629,2759,1579,6043
};

void Lissajous::test()
{
  int i;
  for (i=0;i<21;i++)
    cout<<i<<' '<<num[i]<<'/'<<denom[i]<<' '<<quadirr[i]<<' '<<(double)num[i]/denom[i]-quadirr[i]<<endl;
}
