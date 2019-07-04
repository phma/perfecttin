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
#include <boost/chrono.hpp>
#include "ply.h"
#include "config.h"
#include "las.h"
#include "cloud.h"
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
#include "dxf.h"
#include "threads.h"

using namespace std;
namespace po=boost::program_options;
namespace cr=boost::chrono;

xy magnifyCenter(2301525.560,1432062.436);
double magnifySize=5;
cr::steady_clock clk;
const bool drawDots=false;
const bool colorGradient=true;
const bool colorAbsGradient=false;
const bool doTestPattern=false;

void drawNet(PostScript &ps)
{
  BoundRect br;
  int i,j;
  double slope,r,g,b;
  xy gradient;
  ps.startpage();
  br.include(&net);
  ps.setscale(br);
  //ps.setcolor(1,0,0);
  //ps.circle(magnifyCenter,magnifySize);
  ps.setcolor(0,0,1);
  for (i=0;i<net.edges.size();i++)
    ps.line(net.edges[i],i,false,false);
  ps.setcolor(0,0,0);
  for (i=0;i<net.triangles.size();i++)
  {
    if (drawDots)
      for (j=0;j*j<net.triangles[i].dots.size();j++)
	ps.dot(net.triangles[i].dots[j*j]);
    if (colorGradient)
    {
      gradient=net.triangles[i].gradient(net.triangles[i].centroid());
      slope=gradient.length();
      if (colorAbsGradient)
	ps.setcolor((slope>1)?0:1-slope,(slope>1)?0:1-slope,1);
      else
      {
	r=0.5+gradient.north()*0.3   -gradient.east()*0.4;
	g=0.5+gradient.north()*0.3535-gradient.east()*0.3535;
	b=0.5+gradient.north()*0.4   -gradient.east()*0.3;
	if (r>1)
	  r=1;
	if (r<0)
	  r=0;
	if (g>1)
	  g=1;
	if (g<0)
	  g=0;
	if (b>1)
	  b=1;
	if (b<0)
	  b=0;
	ps.setcolor(r,g,b);
      }
      ps.startline();
      ps.lineto(*net.triangles[i].a);
      ps.lineto(*net.triangles[i].b);
      ps.lineto(*net.triangles[i].c);
      ps.endline(true,true);
    }
  }
  ps.endpage();
}

void drawMag(PostScript &ps)
{
  int i,j;
  ps.startpage();
  ps.setscale(magnifyCenter.getx()-magnifySize,magnifyCenter.gety()-magnifySize,
	      magnifyCenter.getx()+magnifySize,magnifyCenter.gety()+magnifySize);
  ps.setcolor(0,0,1);
  for (i=0;i<net.edges.size();i++)
    if (fabs(pldist(magnifyCenter,*net.edges[i].a,*net.edges[i].b))<2*magnifySize)
      ps.line(net.edges[i],i,false,false);
  ps.setcolor(0,0,0);
  for (i=0;i<cloud.size();i++)
    if (dist(magnifyCenter,xy(cloud[i]))<2*magnifySize)
      ps.dot(cloud[i]);
  ps.endpage();
}

string baseName(string fileName)
{
  long long slashPos,dotPos; // npos turns into -1, which is convenient
  string between;
  slashPos=fileName.rfind('/');
  dotPos=fileName.rfind('.');
  if (dotPos>slashPos)
  {
    between=fileName.substr(slashPos+1,dotPos-slashPos-1);
    if (between.find_first_not_of('.')>between.length())
      dotPos=-1;
  }
  if (dotPos>slashPos)
    fileName.erase(dotPos);
  return fileName;
}

double areaDone(double tolerance)
{
  vector<double> allTri,doneTri;
  static vector<double> allBuckets,doneBuckets;
  static int overtimeCount=0,bucket=0;
  int i;
  double ret;
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
    allBuckets.resize(allBuckets.size()*2);
    doneBuckets.resize(doneBuckets.size()*2);
    overtimeCount=0;
    //cout<<allBuckets.size()<<" buckets \n";
  }
  markBucketClean(bucket);
  for (i=bucket;i<net.triangles.size();i+=allBuckets.size())
  {
    allTri.push_back(net.triangles[i].sarea);
    if (net.triangles[i].inTolerance(tolerance))
      doneTri.push_back(net.triangles[i].sarea);
  }
  allBuckets[bucket]=pairwisesum(allTri);
  doneBuckets[bucket]=pairwisesum(doneTri);
  ret=pairwisesum(doneBuckets)/pairwisesum(allBuckets);
  markBucketClean(bucket);
  elapsed=clk.now()-timeStart;
  if (elapsed>cr::milliseconds(20))
    overtimeCount++;
  else
    overtimeCount=0;
  bucket=(bucket+1)%allBuckets.size();
  return ret;
}

void writeDxf(string outputFile)
{
  vector<GroupCode> dxfCodes;
  int i;
  BoundRect br;
  br.include(&net);
  ofstream dxfFile(outputFile,ofstream::binary|ofstream::trunc);
  dxfHeader(dxfCodes,br);
  tableSection(dxfCodes);
  openEntitySection(dxfCodes);
  for (i=0;i<net.triangles.size();i++)
    insertTriangle(dxfCodes,net.triangles[i]);
  closeEntitySection(dxfCodes);
  dxfEnd(dxfCodes);
  writeDxfGroups(dxfFile,dxfCodes,false);
}

int main(int argc, char *argv[])
{
  PostScript ps;
  int i,e,t,d;
  int nthreads=boost::thread::hardware_concurrency();
  time_t now,then;
  double tolerance,areadone=0,rmsadj;
  bool done=false;
  triangle *tri;
  string inputFile,outputFile;
  bool validArgs,validCmd=true;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  if (nthreads<2)
    nthreads=2;
  generic.add_options()
    ("tolerance,t",po::value<double>(&tolerance)->default_value(0.1),"Vertical tolerance")
    ("threads,j",po::value<int>(&nthreads)->default_value(nthreads),"Number of worker threads")
    ("output,o",po::value<string>(&outputFile),"Output file");
  hidden.add_options()
    ("input",po::value<string>(&inputFile),"Input file");
  p.add("input",1);
  cmdline_options.add(generic).add(hidden);
  cout<<"DeciSite version "<<VERSION<<" © "<<COPY_YEAR<<" Pierre Abbat, GPLv3+\n";
#ifdef LibPLYXX_FOUND
  cout<<"PLY file code © Simon Rajotte, MIT license\n";
#endif
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
  if (!outputFile.length() && baseName(inputFile).length())
    outputFile=baseName(inputFile)+".dxf";
  if (!outputFile.length())
  {
    if (inputFile.length())
      cerr<<"Please specify output file with -o\n";
    else
    {
      cout<<"Usage: decisite [options] input-file\n";
      cout<<generic;
    }
    validCmd=false;
  }
  if (outputFile==inputFile && validCmd)
  {
    cerr<<"Not overwriting input file\n";
    validCmd=false;
  }
  if (validCmd)
  {
    if (inputFile.length())
    {
      readPly(inputFile);
      if (cloud.size()==0)
	readLas(inputFile);
      if (cloud.size())
	cout<<"Read "<<cloud.size()<<" dots\n";
    }
    else if (doTestPattern)
    {
      setsurface(CIRPAR);
      aster(100000);
    }
    if (cloud.size()==0)
    {
      if (inputFile.length())
	cerr<<"No point cloud found in "<<inputFile<<endl;
      done=true;
    }
    areadone=makeOctagon();
    if (!std::isfinite(areadone))
    {
      cerr<<"Point cloud covers no area or has infinite or NaN points\n";
      done=true;
    }
    stageTolerance=tolerance;
    while (stageTolerance<areadone)
      stageTolerance*=2;
    if (!done)
    {
      ps.open("decisite.ps");
      ps.setpaper(papersizes["A4 portrait"],0);
      ps.prolog();
      drawNet(ps);
    }
    for (i=1;i>6;i+=2)
      flip(&net.edges[i]);
    //drawNet(ps);
    for (i=0;i>6;i++)
      split(&net.triangles[i]);
    //drawNet(ps);
    for (i=0;i>13;i+=(i?1:6)) // edges 1-5 are interior
      bend(&net.edges[i]);
    net.makeqindex();
    if (nthreads<1)
      nthreads=1;
    startThreads(nthreads);
    tri=&net.triangles[0];
    for (i=e=t=d=0;!done;i++)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
      now=time(nullptr);
      if (now!=then)
      {
	areadone=areaDone(stageTolerance);
	rmsadj=rmsAdjustment();
	cout<<"Toler "<<stageTolerance;
	cout<<"  "<<ldecimal(areadone*100,areadone*(1-areadone)*10)<<"% done  ";
	cout<<net.triangles.size()<<" tri  adj ";
	cout<<ldecimal(rmsadj,tolerance/100)<<"     \r";
	cout.flush();
	then=now;
	if (areadone==1 && allBucketsClean())
	{
	  waitForThreads(TH_PAUSE);
	  net.makeqindex();
	  adjustLooseCorners(stageTolerance);
	  stageTolerance/=2;
	  if (stageTolerance<tolerance)
	    done=true;
	  else
	  {
	    drawNet(ps);
	    writeDxf(outputFile);
	    waitForThreads(TH_RUN);
	  }
	}
      }
    }
    waitForThreads(TH_STOP);
    if (ps.isOpen())
    {
      drawNet(ps);
      cout<<'\n';
      ps.close();
    }
    if (outputFile.length() && areadone==1)
      writeDxf(outputFile);
    joinThreads();
  }
  return 0;
}
