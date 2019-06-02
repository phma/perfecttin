/******************************************************/
/*                                                    */
/* decisite.cpp - main program                        */
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
#include <boost/program_options.hpp>
#include "ply.h"
#include "ps.h"
#include "octagon.h"
#include "triop.h"
#include "test.h"
#include "angle.h"
#include "edgeop.h"
#include "relprime.h"
#include "adjelev.h"
#include "manysum.h"
#include "ldecimal.h"

using namespace std;
namespace po=boost::program_options;

void drawNet(PostScript &ps)
{
  BoundRect br;
  int i,j;
  ps.startpage();
  br.include(&net);
  ps.setscale(br);
  ps.setcolor(0,0,1);
  for (i=0;i<net.edges.size();i++)
    ps.line(net.edges[i],i,false,false);
  ps.setcolor(0,0,0);
  for (i=0;i<net.triangles.size();i++)
    for (j=0;j*j<net.triangles[i].dots.size();j++)
      ps.dot(net.triangles[i].dots[j*j]);
  ps.endpage();
}

double areaDone()
{
  vector<double> allTri,doneTri;
  int i;
  for (i=0;i<net.triangles.size();i++)
  {
    allTri.push_back(net.triangles[i].sarea);
    if (net.triangles[i].flags&1)
      doneTri.push_back(net.triangles[i].sarea);
  }
  return pairwisesum(doneTri)/pairwisesum(allTri);
}

int main(int argc, char *argv[])
{
  PostScript ps;
  int i,e,t,d;
  time_t now,then;
  double tolerance;
  triangle *tri;
  string inputFile,outputFile;
  bool validArgs,validCmd=true;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  generic.add_options()
    ("tolerance,t",po::value<double>(&tolerance)->default_value(0.1),"Vertical tolerance")
    ("output,o",po::value<string>(&outputFile),"Output file");
  hidden.add_options()
    ("input",po::value<string>(&inputFile),"Input file");
  p.add("input",1);
  cmdline_options.add(generic).add(hidden);
  try
  {
    po::store(po::command_line_parser(argc,argv).options(cmdline_options).positional(p).run(),vm);
    po::notify(vm);
  }
  catch (exception &e)
  {
    cerr<<e.what()<<endl;
    validCmd=false;
  }
  if (inputFile.length())
  {
    readPly(inputFile);
    cout<<"Read "<<cloud.size()<<" points\n";
  }
  else
  {
    setsurface(CIRPAR);
    aster(100000);
  }
  makeOctagon();
  ps.open("decisite.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  drawNet(ps);
  for (i=1;i>6;i+=2)
    flip(&net.edges[i]);
  //drawNet(ps);
  for (i=0;i>6;i++)
    split(&net.triangles[i]);
  //drawNet(ps);
  for (i=0;i>13;i+=(i?1:6)) // edges 1-5 are interior
    bend(&net.edges[i]);
  initTempPointlist(1);
  tri=&net.triangles[0];
  for (i=e=t=d=0;i<10000;i++)
  {
    if ((i&(i-1))==0)
      net.makeqindex();
    edgeop(&net.edges[e],tolerance,0);
    e=(e+relprime(net.edges.size()))%net.edges.size();
    triop(&net.triangles[t],tolerance,0);
    t=(t+relprime(net.triangles.size()))%net.triangles.size();
    //tri=net.findt(cloud[d]);
    //triop(tri,tolerance,0);
    //d=(d+relprime(cloud.size()))%cloud.size();
    if (i==sqr(lrint(sqrt(i))))
      drawNet(ps);
    now=time(nullptr);
    if (now!=then)
    {
      cout<<i<<"  "<<ldecimal(areaDone()*100,0.1)<<"%  "<<ldecimal(rmsAdjustment(),tolerance/100)<<"     \r";
      cout.flush();
      then=now;
    }
  }
  drawNet(ps);
  cout<<'\n';
  ps.close();
  return 0;
}
