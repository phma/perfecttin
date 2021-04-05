/******************************************************/
/*                                                    */
/* perfecttin.cpp - main program                      */
/*                                                    */
/******************************************************/
/* Copyright 2019,2020 Pierre Abbat.
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
#include <boost/program_options.hpp>
#include <chrono>
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
#include "tintext.h"
#include "carlsontin.h"
#include "landxml.h"
#include "fileio.h"
#include "brevno.h"

#define FMT_DXF_TXT 1
#define FMT_DXF_BIN 2
#define FMT_PLY_TXT 3
#define FMT_PLY_BIN 4
#define FMT_TIN 5
#define FMT_CARLSON_TIN 6
#define FMT_LANDXML 7

using namespace std;
namespace po=boost::program_options;
namespace cr=chrono;

xy magnifyCenter(2301525.560,1432062.436);
double magnifySize=5;
double inUnit=1,outUnit=1;
bool exportEmpty=false;
const bool drawDots=false;
const bool colorGradient=true;
const bool colorAbsGradient=false;
const bool doTestPattern=true;
const char formats[][12]=
{
  "",
  "dxftxt",
  "dxfbin",
  "plytxt",
  "plybin",
  "tin",
  "carlsontin",
  "landxml"
};
const char colorSchemes[][12]=
{
  "gradient",
  "elevation"
};

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
	r=0.5+gradient.north()*0.1294+gradient.east()*0.483;
	g=0.5+gradient.north()*0.3535-gradient.east()*0.3535;
	b=0.5-gradient.north()*0.483 -gradient.east()*0.1294;
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
  int i;
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

double parseUnit(const string &unit)
{
  if (unit=="m")
    return 1;
  if (unit=="ft")
    return 0.3048;
  if (unit=="usft")
    return 12e2/3937;
  if (unit=="inft")
    return 0.3047996;
  return 0;
}

void parseUnits(string units)
{
  string inUnitStr,outUnitStr;
  size_t commapos;
  commapos=units.find(',');
  if (commapos<units.length())
  {
    inUnitStr=units.substr(0,commapos);
    outUnitStr=units.substr(commapos+1);
  }
  else
    inUnitStr=outUnitStr=units;
  inUnit=parseUnit(inUnitStr);
  outUnit=parseUnit(outUnitStr);
}

int parseFormat(const string &format)
{
  int i,ret=-1;
  for (i=0;i<sizeof(formats)/sizeof(formats[0]);i++)
    if (format==formats[i])
      ret=i;
#ifndef Plytapus_FOUND
  if (ret==FMT_PLY_BIN || ret==FMT_PLY_TXT)
    ret=-1;
#endif
  return ret;
}

int parseColorScheme(const string &scheme)
{
  int i,ret=-1;
  for (i=0;i<sizeof(colorSchemes)/sizeof(colorSchemes[0]);i++)
    if (scheme==colorSchemes[i])
      ret=i;
  return ret;
}

int main(int argc, char *argv[])
{
  PostScript ps;
  int i,e,t,d;
  int nthreads=thread::hardware_concurrency();
  int asterPoints=0;
  int ptinFilesOpened=0,pointCloudsLoaded=0;
  time_t now,then;
  double tolerance,rmsadj,density;
  bool done=false;
  bool asciiFormat=false;
  int format,colorScheme;
  size_t already;
  string formatStr,colorStr;
  triangle *tri;
  string outputFile;
  vector<string> inputFiles;
  string unitStr;
  ThreadAction ta;
  PtinHeader ptinHeader;
  bool validCmd=true;
  po::options_description generic("Options");
  po::options_description hidden("Hidden options");
  po::options_description cmdline_options;
  po::positional_options_description p;
  po::variables_map vm;
  if (nthreads<2)
    nthreads=2;
  generic.add_options()
    ("units,u",po::value<string>(&unitStr)->default_value("m"),"Units")
    ("tolerance,t",po::value<double>(&tolerance)->default_value(0.1,"0.1"),"Vertical tolerance")
    ("threads,j",po::value<int>(&nthreads)->default_value(nthreads),"Number of worker threads")
    ("asteraceous",po::value<int>(&asterPoints),"Process an asteraceous test pattern")
    ("output,o",po::value<string>(&outputFile),"Output file")
    ("format,f",po::value<string>(&formatStr),"Output format")
    ("color",po::value<string>(&colorStr)->default_value("gradient"),"Color scheme")
    ("export-empty,e","Export empty triangles");
  hidden.add_options()
    ("input",po::value<vector<string> >(&inputFiles),"Input file");
  p.add("input",-1);
  cmdline_options.add(generic).add(hidden);
  cout<<"PerfectTIN version "<<VERSION<<" © "<<COPY_YEAR<<" Pierre Abbat, GPLv3+\n";
#ifdef Plytapus_FOUND
  cout<<"Plytapus library version "<<plytapusVersion()<<" © "<<plytapusYear()<<" Simon Rajotte and Pierre Abbat, MIT license\n";
#endif
  try
  {
    po::store(po::command_line_parser(argc,argv).options(cmdline_options).positional(p).run(),vm);
    po::notify(vm);
    if (vm.count("export-empty"))
      exportEmpty=true;
  }
  catch (exception &ex)
  {
    cerr<<ex.what()<<endl;
    validCmd=false;
  }
  if (outputFile.length() && extension(outputFile)==".ptin")
    outputFile=noExt(outputFile);
  if (!outputFile.length() && inputFiles.size()==1 && noExt(inputFiles[0]).length())
    outputFile=noExt(inputFiles[0]);
  if (!outputFile.length())
  {
    if (inputFiles.size())
      cerr<<"Please specify output file with -o\n";
    else
    {
      cout<<"Usage: perfecttin [options] input-file\n";
      cout<<generic;
    }
    validCmd=false;
  }
  parseUnits(unitStr);
  if (inUnit==0 || outUnit==0)
  {
    cerr<<"Units are m (meter), ft (international foot),\nusft (US survey foot) or inft (Indian survey foot).\n";
    cerr<<"Specify input and output units with a comma, e.g. ft,inft .\n";
    validCmd=false;
  }
  format=parseFormat(formatStr);
  if (format<0)
  {
    cerr<<"Formats are ";
    for (i=1;i<sizeof(formats)/sizeof(formats[0]);i++)
    {
#ifndef Plytapus_FOUND
      if (i==FMT_PLY_TXT)
	i+=2;
#endif
      if (i>1)
	cerr<<", ";
      if (i==sizeof(formats)/sizeof(formats[0])-1)
	cerr<<"and ";
      cerr<<formats[i];
    }
    cerr<<".\n";
    validCmd=false;
  }
  colorScheme=parseColorScheme(colorStr);
  if (colorScheme<0)
  {
    cerr<<"Color schemes are ";
    for (i=0;i<sizeof(colorSchemes)/sizeof(colorSchemes[0]);i++)
    {
      //if (i>0)
	//cerr<<", ";
      if (i==sizeof(colorSchemes)/sizeof(colorSchemes[0])-1)
	cerr<<" and ";
      cerr<<colorSchemes[i];
    }
    cerr<<".\n";
    validCmd=false;
  }
  colorize.setScheme(colorScheme);
  if (validCmd)
  {
    if (inputFiles.size())
      for (i=0;i<inputFiles.size();i++)
      {
	ptinHeader=readPtin(inputFiles[i]);
	if (ptinHeader.tolRatio>0 && ptinHeader.tolerance>0)
	{
	  ptinFilesOpened++;
	  net.conversionTime=ptinHeader.conversionTime;
	  tolerance=ptinHeader.tolerance;
	  density=ta.ptinResult.density;
	  stageTolerance=tolerance*ptinHeader.tolRatio;
	  minArea=sqr(stageTolerance/tolerance)/densify/density;
	  resizeBuckets(1);
	  if (ptinHeader.tolRatio>1 &&
	      extension(noExt(inputFiles[i]))=="."+to_string(ptinHeader.tolRatio))
	    outputFile=noExt(noExt(inputFiles[i]));
	  if (ptinHeader.tolRatio==1)
	  {
	    done=true;
	    areadone[0]=1;
	  }
	}
	else
	{
	  already=cloud.size();
	  readCloud(inputFiles[i],inUnit);
	  if (already!=cloud.size())
	    pointCloudsLoaded++;
	}
      }
    else if (doTestPattern && asterPoints>0)
    {
      setsurface(CIRPAR);
      aster(asterPoints);
    }
    if (ptinFilesOpened>1)
    {
      cerr<<"Can't open more than one PerfectTIN file at once\n";
      done=true;
    }
    if (ptinFilesOpened && pointCloudsLoaded)
    {
      cerr<<"Can't open a PerfectTIN file and load a point cloud\n";
      done=true;
    }
    if (ptinFilesOpened==0 && cloud.size()==0)
    {
      if (inputFiles.size())
	cerr<<"No point cloud found in "<<inputFiles[0]<<endl;
      done=true;
    }
    if (nthreads<1)
      nthreads=1;
    startThreads(nthreads);
    if (!ptinFilesOpened && !done)
    {
      areadone[0]=makeOctagon();
      if (!std::isfinite(areadone[0]))
      {
	cerr<<"Point cloud covers no area or has infinite or NaN points\n";
	done=true;
      }
      density=estimatedDensity();
      stageTolerance=tolerance;
      while (stageTolerance<areadone[0])
	stageTolerance*=2;
      minArea=sqr(stageTolerance/tolerance)/densify/density;
    }
    if (!done)
    {
      //ps.open("perfecttin.ps");
      //ps.setpaper(papersizes["A4 portrait"],0);
      //ps.prolog();
      //drawNet(ps);
    }
    for (i=1;i>6;i+=2)
      flip(&net.edges[i],-1);
    //drawNet(ps);
    for (i=0;i>6;i++)
      split(&net.triangles[i],-1);
    //drawNet(ps);
    for (i=0;i>13;i+=(i?1:6)) // edges 1-5 are interior
      bend(&net.edges[i],-1);
    net.makeqindex();
    tri=&net.triangles[0];
    waitForThreads(TH_RUN);
    for (i=e=t=d=0;!done;i++)
    {
      this_thread::sleep_for(chrono::milliseconds(1));
      writeBufLog();
      now=time(nullptr);
      if (now!=then)
      {
	areadone=areaDone(stageTolerance,sqr(stageTolerance/tolerance)/density);
	rmsadj=rmsAdjustment();
	cout<<"Toler "<<stageTolerance;
	cout<<"  "<<ldecimal(areadone[0]*100,areadone[0]*(1-areadone[0])*10)<<"% done  ";
	cout<<net.triangles.size()<<" tri  adj ";
	cout<<ldecimal(rmsadj,tolerance/100)<<"     \r";
	cout.flush();
	then=now;
	if (livelock(areadone[0],rmsadj))
	{
	  //cerr<<"Livelock detected\n";
	  randomizeSleep();
	}
	if ((areadone[0]==1 && allBucketsClean()) || (areadone[1]==1 && stageTolerance>tolerance))
	{
	  waitForThreads(TH_PAUSE);
	  net.makeqindex();
	  stageTolerance/=2;
	  minArea/=4;
	  if (stageTolerance<tolerance)
	    done=true;
	  else
	  {
	    ta.param1=tolerance;
	    ta.param2=density;
	    ta.param0=lrint(4*stageTolerance/tolerance);
	    ta.opcode=ACT_DELETE_FILE;
	    ta.filename=outputFile+"."+to_string(ta.param0)+".ptin";
	    enqueueAction(ta);
	    ta.param0=lrint(stageTolerance/tolerance);
	    ta.opcode=ACT_WRITE_PTIN;
	    if (ta.param0==1)
	      ta.filename=outputFile+".ptin";
	    else
	      ta.filename=outputFile+"."+to_string(ta.param0)+".ptin";
	    enqueueAction(ta);
	    if (ps.isOpen())
	      drawNet(ps);
	    waitForQueueEmpty();
	    waitForThreads(TH_RUN);
	  }
	}
      }
      writeBufLog();
    }
    waitForThreads(TH_STOP);
    if (ps.isOpen())
    {
      drawNet(ps);
      cout<<'\n';
      ps.close();
    }
    if (outputFile.length() && areadone[0]==1)
    {
      switch (format)
      {
	case FMT_LANDXML:
	  writeLandXml(outputFile+".xml",outUnit,exportEmpty);
	  break;
	case FMT_CARLSON_TIN:
	  writeCarlsonTin(outputFile+".tin",outUnit,exportEmpty);
	  break;
	case FMT_DXF_BIN:
	  writeDxf(outputFile+".dxf",false,outUnit,exportEmpty);
	  break;
	case FMT_DXF_TXT:
	  writeDxf(outputFile+".dxf",true,outUnit,exportEmpty);
	  break;
	case FMT_PLY_BIN:
	  writePly(outputFile+".ply",false,outUnit,exportEmpty);
	  break;
	case FMT_PLY_TXT:
	  writePly(outputFile+".ply",true,outUnit,exportEmpty);
	  break;
	case FMT_TIN:
	  writeTinText(outputFile+".tin",outUnit,exportEmpty);
	  break;
      }
      deleteFile(outputFile+".2.ptin");
    }
    writeBufLog();
    joinThreads();
  }
  return 0;
}
