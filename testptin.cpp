/******************************************************/
/*                                                    */
/* testptin.cpp - test program                        */
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

#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <cfloat>
#include <cstring>
#include "config.h"
#include "point.h"
#include "cogo.h"
#include "test.h"
#include "tin.h"
#include "dxf.h"
#include "angle.h"
#include "pointlist.h"
#include "qindex.h"
#include "random.h"
#include "ps.h"
#include "manysum.h"
#include "ldecimal.h"
#include "relprime.h"
#include "binio.h"
#include "matrix.h"
#include "leastsquares.h"

#define tassert(x) testfail|=(!(x))

using namespace std;

char hexdig[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
bool slowmanysum=false;
bool testfail=false;
vector<string> args;

void outsizeof(string typeName,int size)
{
  cout<<"size of "<<typeName<<" is "<<size<<endl;
}

void testsizeof()
// This is not run as part of "make test".
{
  outsizeof("xy",sizeof(xy));
  outsizeof("xyz",sizeof(xyz));
  outsizeof("point",sizeof(point));
  outsizeof("edge",sizeof(edge));
  outsizeof("triangle",sizeof(triangle));
  outsizeof("qindex",sizeof(qindex));
  /* A large TIN has 3 edges per point, 2 triangles per point,
   * and 4/9 to 4/3 qindex per point. On x86_64, this amounts to
   * point	40	40
   * edge	48	144
   * triangle	176	352
   * qindex	56	25-75
   * Total		561-611 bytes.
   */
}

bool shoulddo(string testname)
{
  int i;
  bool ret,listTests=false;
  if (testfail)
  {
    cout<<"failed before "<<testname<<endl;
    //sleep(2);
  }
  ret=args.size()==0;
  for (i=0;i<args.size();i++)
  {
    if (testname==args[i])
      ret=true;
    if (args[i]=="-l")
      listTests=true;
  }
  if (listTests)
    cout<<testname<<endl;
  return ret;
}

int main(int argc, char *argv[])
{
  int i;
  for (i=1;i<argc;i++)
    args.push_back(argv[i]);
  if (shoulddo("sizeof"))
    testsizeof();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
