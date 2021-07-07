/******************************************************/
/*                                                    */
/* testptin.cpp - test program                        */
/*                                                    */
/******************************************************/
/* Copyright 2019-2021 Pierre Abbat.
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
#include "xyzfile.h"
#include "cloud.h"
#include "stl.h"
#include "angle.h"
#include "pointlist.h"
#include "adjelev.h"
#include "octagon.h"
#include "edgeop.h"
#include "triop.h"
#include "qindex.h"
#include "random.h"
#include "ps.h"
#include "threads.h"
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
const bool drawDots=true;
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

void drawNet(PostScript &ps)
{
  BoundRect br;
  int i,j;
  double slope,r,g,b;
  xy gradient;
  ps.startpage();
  br.include(&net);
  ps.setscale(br);
  ps.setcolor(0,0,1);
  for (i=0;i<net.edges.size();i++)
    ps.line(net.edges[i],i,false,false);
  ps.setcolor(0,0,0);
  for (i=0;i<net.triangles.size();i++)
  {
    if (drawDots)
      for (j=0;j<net.triangles[i].dots.size();j++)
	ps.dot(net.triangles[i].dots[j]);
  }
  ps.endpage();
}

void testintegertrig()
{
  double sinerror,coserror,ciserror,totsinerror,totcoserror,totciserror;
  int i;
  char bs=8;
  for (totsinerror=totcoserror=totciserror=i=0;i<128;i++)
  {
    sinerror=sin(i<<24)+sin((i+64)<<24);
    coserror=cos(i<<24)+cos((i+64)<<24);
    ciserror=hypot(cos(i<<24),sin(i<<24))-1;
    if (sinerror>0.04 || coserror>0.04 || ciserror>0.04)
    {
      printf("sin(%8x)=%a sin(%8x)=%a\n",i<<24,sin(i<<24),(i+64)<<24,sin((i+64)<<24));
      printf("cos(%8x)=%a cos(%8x)=%a\n",i<<24,cos(i<<24),(i+64)<<24,cos((i+64)<<24));
      printf("abs(cis(%8x))=%a\n",i<<24,hypot(cos(i<<24),sin(i<<24)));
    }
    totsinerror+=sinerror*sinerror;
    totcoserror+=coserror*coserror;
    totciserror+=ciserror*ciserror;
  }
  printf("total sine error=%e\n",totsinerror);
  printf("total cosine error=%e\n",totcoserror);
  printf("total cis error=%e\n",totciserror);
  tassert(totsinerror+totcoserror+totciserror<2e-29);
  //On Linux, the total error is 2e-38 and the M_PIl makes a big difference.
  //On DragonFly BSD, the total error is 1.7e-29 and M_PIl is absent.
  tassert(bintodeg(0)==0);
  tassert(fabs(bintodeg(0x15555555)-60)<0.0000001);
  tassert(fabs(bintomin(0x08000000)==1350));
  tassert(fabs(bintosec(0x12345678)-184320)<0.001);
  tassert(fabs(bintogon(0x1999999a)-80)<0.0000001);
  tassert(fabs(bintorad(0x4f1bbcdd)-3.88322208)<0.00000001);
  for (i=-2147400000;i<2147400000;i+=rng.usrandom()+18000)
  {
    cout<<setw(11)<<i<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs;
    cout.flush();
    tassert(degtobin(bintodeg(i))==i);
    tassert(mintobin(bintomin(i))==i);
    tassert(sectobin(bintosec(i))==i);
    tassert(gontobin(bintogon(i))==i);
    tassert(radtobin(bintorad(i))==i);
  }
  tassert(sectobin(1295999.9999)==-2147483648);
  tassert(sectobin(1296000.0001)==-2147483648);
  tassert(sectobin(-1295999.9999)==-2147483648);
  tassert(sectobin(-1296000.0001)==-2147483648);
  cout<<"           "<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs<<bs;
}

void test1in(xy p,xy a,xy b,xy c,double windnum)
{
  double wind;
  wind=in3(p,a,b,c);
  if (wind!=windnum && (windnum!=IN_AT_CORNER || wind<=-1 || wind==0 || wind>=1))
    cout<<"Triangle ("<<a.east()<<','<<a.north()<<"),("<<
      b.east()<<','<<b.north()<<"),("<<
      c.east()<<','<<c.north()<<"): ("<<
      p.east()<<','<<p.north()<<")'s winding number is "<<wind<<
      ", should be "<<windnum<<endl;
  tassert(wind==windnum || windnum==IN_AT_CORNER);
  wind=in3(p,c,b,a);
  if (windnum<10 && windnum>-10)
    windnum=-windnum;
  if (wind!=windnum && (windnum!=IN_AT_CORNER || wind<=-1 || wind==0 || wind>=1))
    cout<<"Triangle ("<<c.east()<<','<<c.north()<<"),("<<
      b.east()<<','<<b.north()<<"),("<<
      a.east()<<','<<a.north()<<"): ("<<
      p.east()<<','<<p.north()<<")'s winding number is "<<wind<<
      ", should be "<<windnum<<endl;
  tassert(wind==windnum || windnum==IN_AT_CORNER);
}

void testin()
{
  xy a(0,0),b(4,0),c(0,3),d(4/3.,1),e(4,3),f(5,0),g(7,-1),h(8,-3),
     i(3,-5),j(0,-6),k(-2,-2),l(-4,0),m(-4,-3),n(-4,6),o(-3,7),p(0,8),q(2,1.5);
  test1in(d,a,b,c,1);
  test1in(e,a,b,c,0);
  test1in(f,a,b,c,0);
  test1in(g,a,b,c,0);
  test1in(h,a,b,c,0);
  test1in(i,a,b,c,0);
  test1in(j,a,b,c,0);
  test1in(k,a,b,c,0);
  test1in(l,a,b,c,0);
  test1in(m,a,b,c,0);
  test1in(n,a,b,c,0);
  test1in(o,a,b,c,0);
  test1in(p,a,b,c,0);
  test1in(q,a,b,c,0.5);
  test1in(a,a,b,c,0.25);
  test1in(b,a,b,c,IN_AT_CORNER);
  test1in(c,a,b,c,IN_AT_CORNER);
  test1in(b,c,h,n,0);
}

void testarea3()
{
  int i,j,itype;
  xy a(0,0),b(4,0),c(0,3),d(4,4),e;
  tassert(area3(c,a,b)==6);
}

void testrandom()
{
  int hist[256],i;
  int done=0,max,min,maxstep=0;
  manysum xsum,ysum,zsum;
  memset(hist,0,sizeof(hist));
  while (!done)
  {
    hist[rng.ucrandom()]++;
    for (max=i=0,min=16777777;i<256;i++)
    {
      if (hist[i]>max)
	max=hist[i];
      if (hist[i]<min)
	min=hist[i];
    }
    if (max>16777215)
      done=-1;
    if (max-min>1.1*pow(max,1/3.) && max-min<0.9*pow(max,2/3.))
      done=1;
    if (max-maxstep>=16384)
    {
      maxstep=max;
      //cout<<max<<' '<<min<<endl;
    }
  }
  tassert(done==1);
  cout<<"Random test: max "<<max<<" min "<<min<<endl;
  tassert(done==1);
}

void knowndet(matrix &mat)
/* Sets mat to a triangular matrix with ones on the diagonal, which is known
 * to have determinant 1, then permutes the rows and columns so that Gaussian
 * elimination will mess up the entries.
 *
 * The number of rows should be 40 at most. More than that, and it will not
 * be shuffled well.
 */
{
  int i,j,ran,rr,rc,cc,flipcount,size,giveup;
  mat.setidentity();
  size=mat.getrows();
  for (i=0;i<size;i++)
    for (j=0;j<i;j++)
      mat[i][j]=(rng.ucrandom()*2-255)/BYTERMS;
  for (flipcount=giveup=0;(flipcount<2*size || (flipcount&1)) && giveup<10000;giveup++)
  { // If flipcount were odd, the determinant would be -1 instead of 1.
    ran=rng.usrandom();
    rr=ran%size;
    ran/=size;
    rc=ran%size;
    ran/=size;
    cc=ran%size;
    if (rr!=rc)
    {
      flipcount++;
      mat.swaprows(rr,rc);
    }
    if (rc!=cc)
    {
      flipcount++;
      mat.swapcolumns(rc,cc);
    }
  }
}

void dumpknowndet(matrix &mat)
{
  int i,j,byte;
  for (i=0;i<mat.getrows();i++)
    for (j=0;j<mat.getcolumns();j++)
    {
      if (mat[i][j]==0)
	cout<<"z0";
      else if (mat[i][j]==1)
	cout<<"z1";
      else
      {
	byte=rint(mat[i][j]*BYTERMS/2+127.5);
	cout<<hexdig[byte>>4]<<hexdig[byte&15];
      }
    }
  cout<<endl;
}

void loadknowndet(matrix &mat,string dump)
{
  int i,j,byte;
  string item;
  for (i=0;i<mat.getrows();i++)
    for (j=0;j<mat.getcolumns();j++)
    {
      item=dump.substr(0,2);
      dump.erase(0,2);
      if (item[0]=='z')
	mat[i][j]=item[1]-'0';
      else
      {
	byte=stoi(item,0,16);
	mat[i][j]=(byte*2-255)/BYTERMS;
      }
    }
  cout<<endl;
}

void testmatrix()
{
  int i,j,chk2,chk3,chk4;
  matrix m1(3,4),m2(4,3),m3(4,3),m4(4,3);
  matrix t1(37,41),t2(41,43),t3(43,37),p1,p2,p3;
  matrix t1t,t2t,t3t,p1t,p2t,p3t;
  matrix hil(8,8),lih(8,8),hilprod;
  matrix kd(7,7);
  matrix r0,c0,p11;
  matrix rs1(3,4),rs2,rs3,rs4;
  vector<double> rv,cv;
  double tr1,tr2,tr3,de1,de2,de3,tr1t,tr2t,tr3t;
  double toler=1.2e-12;
  double kde;
  double lo,hi;
  manysum lihsum;
  m1[2][0]=5;
  m1[1][3]=7;
  tassert(m1[2][0]==5);
  m2[2][0]=9;
  m2[1][4]=6;
  tassert(m2[2][0]==9);
  for (i=0;i<4;i++)
    for (j=0;j<3;j++)
    {
      m2[i][j]=rng.ucrandom();
      m3[i][j]=rng.ucrandom();
    }
  for (chk2=chk3=i=0;i<4;i++)
    for (j=0;j<3;j++)
    {
      chk2=(50*chk2+(int)m2[i][j])%83;
      chk3=(50*chk3+(int)m3[i][j])%83;
    }
  m4=m2+m3;
  for (chk4=i=0;i<4;i++)
    for (j=0;j<3;j++)
      chk4=(50*chk4+(int)m4[i][j])%83;
  tassert(chk4==(chk2+chk3)%83);
  m4=m2-m3;
  for (chk4=i=0;i<4;i++)
    for (j=0;j<3;j++)
      chk4=(50*chk4+(int)m4[i][j]+332)%83;
  tassert(chk4==(chk2-chk3+83)%83);
  lo=INFINITY;
  hi=0;
  for (i=0;i<1;i++)
  {
    t1.randomize_c();
    t2.randomize_c();
    t3.randomize_c();
    t1t=t1.transpose();
    t2t=t2.transpose();
    t3t=t3.transpose();
    p1=t1*t2*t3;
    p2=t2*t3*t1;
    p3=t3*t1*t2;
    p1t=t3t*t2t*t1t;
    p2t=t1t*t3t*t2t;
    p3t=t2t*t1t*t3t;
    tr1=p1.trace();
    tr2=p2.trace();
    tr3=p3.trace();
    tr1t=p1t.trace();
    tr2t=p2t.trace();
    tr3t=p3t.trace();
    de1=p1.determinant();
    de2=p2.determinant();
    de3=p3.determinant();
    cout<<"trace1 "<<ldecimal(tr1)
	<<" trace2 "<<ldecimal(tr2)
	<<" trace3 "<<ldecimal(tr3)<<endl;
    tassert(fabs(tr1-tr2)<toler && fabs(tr2-tr3)<toler && fabs(tr3-tr1)<toler);
    tassert(fabs(tr1-tr1t)<toler && fabs(tr2-tr2t)<toler && fabs(tr3-tr3t)<toler);
    tassert(tr1!=0);
    cout<<"det1 "<<de1
	<<" det2 "<<de2
	<<" det3 "<<de3<<endl;
    tassert(fabs(de1)>1e80 && fabs(de2)<1e60 && fabs(de3)<1e52);
    // de2 and de3 would actually be 0 with exact arithmetic.
    if (fabs(de2)>hi)
      hi=fabs(de2);
    if (fabs(de2)<lo)
      lo=fabs(de2);
  }
  cout<<"Lowest det2 "<<lo<<" Highest det2 "<<hi<<endl;
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      hil[i][j]=1./(i+j+1);
  lih=invert(hil);
  for (i=0;i<8;i++)
    for (j=0;j<i;j++)
      lihsum+=fabs(lih[i][j]-lih[j][i]);
  cout<<"Total asymmetry of inverse of Hilbert matrix is "<<lihsum.total()<<endl;
  hilprod=hil*lih;
  lihsum.clear();
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      lihsum+=fabs(hilprod[i][j]-(i==j));
  cout<<"Total error of Hilbert matrix * inverse is "<<lihsum.total()<<endl;
  tassert(lihsum.total()<2e-5 && lihsum.total()>1e-15);
  for (i=0;i<1;i++)
  {
    knowndet(kd);
    //loadknowndet(kd,"z0z0z0z1c9dd28z0z1z03c46c35cz0z0z0z0z1z0z0aa74z169f635e3z0z0z0z003z0z1z0z0z0z0fcz146z160z000f50091");
    //loadknowndet(kd,"a6z1fc7ce056d6d1z0z0z0z0z1b49ez0z0z1z02f53z1z0z0z0z0z0z0e2z0z097z1230488z0z19b25484e1ez0z0z0z0z0z1");
    kd.dump();
    dumpknowndet(kd);
    kde=kd.determinant();
    cout<<"Determinant of shuffled matrix is "<<ldecimal(kde)<<" diff "<<kde-1<<endl;
    tassert(fabs(kde-1)<4e-12);
  }
  for (i=0;i<11;i++)
  {
    rv.push_back((i*i*i)%11);
    cv.push_back((i*3+7)%11);
  }
  r0=rowvector(rv);
  c0=columnvector(cv);
  p1=r0*c0;
  p11=c0*r0;
  tassert(p1.trace()==253);
  tassert(p11.trace()==253);
  tassert(p1.determinant()==253);
  tassert(p11.determinant()==0);
  for (i=0;i<3;i++)
    for (j=0;j<4;j++)
      rs1[i][j]=(j+1.)/(i+1)-(i^j);
  rs2=rs1;
  rs2.resize(4,3);
  rs3=rs1*rs2;
  rs4=rs2*rs1;
  for (i=0;i<3;i++)
    for (j=0;j<3;j++)
      tassert(rs1[i][j]==rs2[i][j]);
  cout<<"det rs3="<<ldecimal(rs3.determinant())<<endl;
  tassert(fabs(rs3.determinant()*9-100)<1e-12);
  tassert(rs4.determinant()==0);
  rs4[3][3]=1;
  tassert(fabs(rs4.determinant()*9-100)<1e-12);
}

void testrelprime()
{
  tassert(relprime(987)==610);
  tassert(relprime(610)==377);
  tassert(relprime(377)==233);
  tassert(relprime(233)==144);
  tassert(relprime(144)==89);
  tassert(relprime(89)==55);
  tassert(relprime(55)==34);
  tassert(relprime(100000)==61803);
  tassert(relprime(100)==61);
  tassert(relprime(0)==1);
  tassert(relprime(1)==1);
  tassert(relprime(2)==1);
  tassert(relprime(3)==2);
  tassert(relprime(4)==3);
  tassert(relprime(5)==3);
  tassert(relprime(6)==5);
}

void testmanysum()
{
  manysum ms,negms;
  int i,j,h;
  double x,naiveforwardsum,forwardsum,pairforwardsum,naivebackwardsum,backwardsum,pairbackwardsum;
  vector<double> summands;
  double odd[32];
  long double oddl[32];
  int pairtime=0;
  //QTime starttime;
  cout<<"manysum"<<endl;
  for (i=0;i<32;i++)
    oddl[i]=odd[i]=2*i+1;
  for (i=0;i<32;i++)
  {
    tassert(pairwisesum(odd,i)==i*i);
    tassert(pairwisesum(oddl,i)==i*i);
  }
  ms.clear();
  summands.clear();
  tassert(pairwisesum(summands)==0);
  for (naiveforwardsum=i=0;i>-7;i--)
  {
    x=pow(1000,i);
    for (j=0;j<(slowmanysum?1000000:100000);j++)
    {
      naiveforwardsum+=x;
      ms+=x;
      negms-=x;
      summands.push_back(x);
    }
  }
  forwardsum=ms.total();
  tassert(forwardsum==-negms.total());
  //starttime.start();
  pairforwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  ms.clear();
  summands.clear();
  for (naivebackwardsum=0,i=-6;i<1;i++)
  {
    x=pow(1000,i);
    for (j=0;j<(slowmanysum?1000000:100000);j++)
    {
      naivebackwardsum+=x;
      ms+=x;
      summands.push_back(x);
    }
  }
  backwardsum=ms.total();
  //starttime.start();
  pairbackwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  cout<<"Forward: "<<ldecimal(naiveforwardsum)<<' '<<ldecimal(forwardsum)<<' '<<ldecimal(pairforwardsum)<<endl;
  cout<<"Backward: "<<ldecimal(naivebackwardsum)<<' '<<ldecimal(backwardsum)<<' '<<ldecimal(pairbackwardsum)<<endl;
  tassert(fabs((forwardsum-backwardsum)/(forwardsum+backwardsum))<DBL_EPSILON);
  tassert(fabs((pairforwardsum-pairbackwardsum)/(pairforwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-pairforwardsum)/(forwardsum+pairforwardsum))<DBL_EPSILON);
  tassert(fabs((backwardsum-pairbackwardsum)/(backwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-naiveforwardsum)/(forwardsum+naiveforwardsum))<1000000*DBL_EPSILON);
  tassert(fabs((backwardsum-naivebackwardsum)/(backwardsum+naivebackwardsum))<1000*DBL_EPSILON);
  tassert(fabs((naiveforwardsum-naivebackwardsum)/(naiveforwardsum+naivebackwardsum))>30*DBL_EPSILON);
  ms.clear();
  summands.clear();
  h=slowmanysum?1:16;
  for (naiveforwardsum=i=0;i>-0x360000;i-=h)
  {
    x=exp(i/65536.);
    naiveforwardsum+=x;
    ms+=x;
    summands.push_back(x);
  }
  forwardsum=ms.total();
  //starttime.start();
  pairforwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  ms.clear();
  summands.clear();
  for (naivebackwardsum=0,i=-0x35ffff&-h;i<1;i+=h)
  {
    x=exp(i/65536.);
    naivebackwardsum+=x;
    ms+=x;
    summands.push_back(x);
  }
  backwardsum=ms.total();
  //starttime.start();
  pairbackwardsum=pairwisesum(summands);
  //pairtime+=starttime.elapsed();
  cout<<"Forward: "<<ldecimal(naiveforwardsum)<<' '<<ldecimal(forwardsum)<<' '<<ldecimal(pairforwardsum)<<endl;
  cout<<"Backward: "<<ldecimal(naivebackwardsum)<<' '<<ldecimal(backwardsum)<<' '<<ldecimal(pairbackwardsum)<<endl;
  tassert(fabs((forwardsum-backwardsum)/(forwardsum+backwardsum))<DBL_EPSILON);
  tassert(fabs((pairforwardsum-pairbackwardsum)/(pairforwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-pairforwardsum)/(forwardsum+pairforwardsum))<DBL_EPSILON);
  tassert(fabs((backwardsum-pairbackwardsum)/(backwardsum+pairbackwardsum))<DBL_EPSILON);
  tassert(fabs((forwardsum-naiveforwardsum)/(forwardsum+naiveforwardsum))<1000000*DBL_EPSILON);
  tassert(fabs((backwardsum-naivebackwardsum)/(backwardsum+naivebackwardsum))<1000*DBL_EPSILON);
  tassert(fabs((naiveforwardsum-naivebackwardsum)/(naiveforwardsum+naivebackwardsum))>30*DBL_EPSILON);
  //cout<<"Time in pairwisesum: "<<pairtime<<endl;
}

void testspiral()
{
  xy a,b,c,d,limitpoint;
  int i,j,bearing,bearing2,lastbearing,curvebearing,diff,badcount;
  double t,t2;
  float segalo[]={-5.96875,-5.65625,-5.3125,-4.875,-4.5,-4,-3.5,-2.828125,-2,0,
    2,2.828125,3.5,4,4.5,4.875,5.3125,5.65625,5.96875};
  vector<xy> spoints;
  vector<int> sbearings;
  bezier3d a3d;
  spiralarc sarc;
  PostScript ps;
  a=cornu(0);
  tassert(a==xy(0,0));
  ps.open("spiral.ps");
  ps.prolog();
  ps.startpage();
  ps.setscale(-1,-1,1,1,degtobin(0));
  //widen(10);
  for (i=-120;i<121;i++)
  {
    b=cornu(t=i/20.);
    if (i*i==14400)
    {
      limitpoint=b;
      ps.dot(b);
    }
    else
      if (i>-119)
	ps.line2p(c,b);
    //printf("spiral %f %f,%f %f\n",t,b.east(),b.north(),1/sqr(dist(b,limitpoint)));
    if (i>=0)
      spoints.push_back(b);
    c=b;
  }
  for (i=1,badcount=0;i<119;i++)
  {
    curvebearing=ispiralbearing(i/20.,0,2);
    bearing=dir(spoints[i-1],spoints[i+1]); // compute the difference between a chord of the spiral
    diff=(curvebearing-bearing)&0x7fffffff; // and a tangent in the middle of the arc
    diff|=(diff&0x40000000)<<1; // diff could be near 0° or 360°; this bit manipulation puts it near 0°
    //printf("%3d diff=%d (%f')\n",i,diff,bintomin(diff));
    badcount+=(diff<=-300000 || diff>=-250000); // diff is between -3'00" and -2'30" when the increment is 1/20
  }
  /* On i386 (Pentium), the last 12 bearings are off by up to 2°
   * On x86_64 (both Intel Core and Intel Atom), they are all accurate.
   * This is NOT explained by sizeof(long double), which is 12 bytes on i386
   * and 16 bytes on x86_64; only 10 bytes are stored, and the rest is wasted.
   * When computing cornu(-6), the fifth step is facpower*=36/5,
   * where facpower is -419904 before and -3023308.8 after.
   * On x86_64, facpower is 0xc014b887333333333333.
   * On i386,   facpower is 0xc014b887333333333800.
   * I checked this with 64-bit Linux, 64-bit DragonFly, and 32-bit DragonFly;
   * it depends on the processor, not the operating system.
   * The ARM on a Raspberry Pi does not have a distinct long double type
   * and gets 14 bad bearings.
   * Running under Valgrind, the program says there are 10 bad bearings.
   */
  printf("%d bad bearings out of 118\n",badcount);
  tassert(badcount<=15);
  for (bearing=i=0,lastbearing=1;i<100 && bearing!=lastbearing;i++)
  {
    t=bintorad(bearing);
    a=cornu(-sqrt(t));
    b=cornu(sqrt(t+M_PI/2));
    lastbearing=bearing;
    bearing=dir(a,b);
  }
  ps.setcolor(0,0,1);
  ps.line2p(a,b);
  ps.endpage();
  //b=cornu(1,1,1);
  ps.startpage();
  for (j=-3;j<=3;j++)
  {
    switch ((j+99)%3)
    {
      case 0:
	ps.setcolor(1,0,0);
	break;
      case 1:
	ps.setcolor(0,0.4,0);
	break;
      case 2:
	ps.setcolor(0,0,1);
	break;
    }
    for (i=-20;i<21;i++)
    {
      b=cornu(t=i/20.,2*j,2);
      if (i>-20)
      {
	ps.line2p(c,b);
	tassert(dist(c,b)>0.049 && dist(c,b)<=0.05);
	//cout<<dist(c,b)<<' ';
      }
      c=b;
    }
    //cout<<endl;
  }
  ps.endpage();
  //ps.startpage();
  /* Compare cornu(t,curvature,clothance) with cornu(t).
   * The code would draw a set of zero-length lines along a spiral.
   * As these would result in a blank page, it's commented out.
   */
  a=cornu(sqrt(M_PI*2));
  for (i=-20;i<21;i++)
  {
    b=cornu(i/20.,0,2);
    c=cornu(i/20.);
    //cout<<i<<' '<<dist(b,c)<<endl;
    tassert(dist(b,c)<1e-12); // it's less than 6e-17 on 64-bit
    b=cornu(i/20.,sqrt(M_PI*8),2);
    c=cornu(i/20.+sqrt(M_PI*2))-a;
    //cout<<i<<' '<<dist(b,c)<<endl;
    tassert(dist(b,c)<1e-12); // it's less than 1.1e-15 on 64-bit
    //ps.line2p(b,c);
    b=cornu(i/20.,1,0);
    c=xy(0,1);
    //cout<<i<<' '<<dist(b,c)-1<<endl;
    tassert(fabs(dist(b,c)-1)<1e-12); // it's 0 or -1.11e-16 on 64-bit
  }
  //ps.endpage();
  ps.startpage();
  ps.setscale(-1,-1,1,1,0);
  spoints.clear();
  for (i=0;i<sizeof(segalo)/sizeof(segalo[0]);i++)
  {
    spoints.push_back(cornu(segalo[i]));
    sbearings.push_back(ispiralbearing(segalo[i],0,2));
  }
  for (i=0;i<sizeof(segalo)/sizeof(segalo[0])-1;i++)
  {
    sarc=spiralarc(xyz(spoints[i],0),xyz(spoints[i+1],0));
    sarc.setdelta(sbearings[i+1]-sbearings[i],sbearings[i+1]+sbearings[i]-2*dir(spoints[i],spoints[i+1]));
    a3d+=sarc.approx3d(0.01);
  }
  ps.spline(a3d);
  for (bearing=i=0,lastbearing=1;i<100 && bearing!=lastbearing;i++)
  {
    t=bintorad(bearing);
    a=cornu(-sqrt(t));
    b=cornu(sqrt(t+M_PI/2));
    lastbearing=bearing;
    bearing=dir(a,b);
  }
  for (bearing2=i=0,lastbearing=1;i<100 && bearing2!=lastbearing;i++)
  {
    t2=bintorad(bearing2);
    c=cornu(-sqrt(t2));
    d=cornu(sqrt(t2+M_PI));
    lastbearing=bearing2;
    bearing2=dir(c,d);
  }
  ps.dot(limitpoint);
  ps.dot(-limitpoint);
  ps.setcolor(0,0,1);
  ps.line2p(a,b);
  //ps.line2p(c,d);
  ps.endpage();
  ps.trailer();
  ps.close();
  tassert(bearing==162105696);
  tassert(bearing2==229309921);
  cout<<"Barely curly spiralarc is from "<<ldecimal(-sqrt(t))<<" to "<<ldecimal(sqrt(t+M_PI/2))<<endl;
  cout<<"Barely too curly spiralarc is from "<<ldecimal(-sqrt(t2))<<" to "<<ldecimal(sqrt(t2+M_PI))<<endl;
}

void testsegment()
{
  int i;
  double cept;
  vector<double> extrema;
  xyz beg(0,0,3),end(300,400,7),sta;
  segment a(beg,end),b,c;
  tassert(a.length()==500);
  tassert(a.chordlength()==500);
  tassert(a.chordbearing()==316933406);
  cept=a.contourcept(5);
  cout<<"a crosses 5 at "<<cept<<"; a.elev()="<<a.elev(cept)<<endl;
  tassert(fabs(a.elev(cept)-5)<1e-6);
  cept=a.contourcept(2);
  tassert(std::isnan(cept));
  sta=a.station(200);
  tassert(sta==xyz(120,160,4.6));
  tassert(std::isinf(a.radius(0)));
  tassert(a.curvature(0)==0);
  tassert(!isfinite(a.center().east()));
  tassert(a.diffarea()==0);
  a.split(200,b,c);
  tassert(dist(b.station(123),a.station(123))<0.001);
  tassert(dist(c.station(200),a.station(400))<0.001);
}

void testarc()
{
  int i;
  double xx;
  vector<double> extrema;
  xyz beg(0,0,3),end(300,400,7),beg1(0,-15,0),end1(0,15,0),sta,sta1,sta2;
  xy ctr;
  arc a(beg,end),b,c;
  tassert(fabs(a.length()-500)<0.001);
  tassert(a.chordlength()==500);
  a.setdelta(degtobin(60));
  tassert(fabs(a.length()-523.599)<0.001);
  tassert(a.chordlength()==500);
  tassert(fabs(a.diffarea()-(M_PI*sqr(500)/6-250*500*M_SQRT_3_4))<1e-4);
  sta=a.station(200);
  //printf("sta.x=%.17f sta.y=%.17f sta.z=%.17f \n",sta.east(),sta.north(),sta.elev());
  tassert(dist(sta,xyz(163.553,112.7825,4.5279))<0.001);
  //printf("arc radius %f\n",a.radius(1));
  tassert(fabs(a.radius(0)-500)<0.001);
  tassert(fabs(a.curvature(0)-0.002)<0.000001);
  //printf("arc center %f,%f\n",a.center().east(),a.center().north());
  ctr=a.center();
  //printf("distance %f\n",dist(xy(sta),ctr));
  tassert(fabs(dist(xy(sta),ctr)-500)<0.001);
  tassert(fabs(ctr.east()+196.410)<0.001);
  tassert(fabs(ctr.north()-459.8075)<0.001);
  a.split(200,b,c);
  sta=a.station(200);
  //printf("a.station %f,%f,%f\n",sta.east(),sta.north(),sta.elev());
  sta=b.station(200);
  printf("b.station %f,%f,%f %f\n",sta.east(),sta.north(),sta.elev(),b.length());
  tassert(dist(b.station(123),a.station(123))<0.001);
  tassert(dist(c.station(200),a.station(400))<0.001);
  sta=xyz(150,200,5);
  b=arc(beg,sta,end);
  sta2=b.station(250);
  cout<<"arc3 "<<sta2.elev()<<endl;
  tassert(sta2.elev()==5);
  sta=xyz(150,200,10);
  b=arc(beg,sta,end);
  sta2=b.station(250);
  cout<<"arc3 "<<sta2.elev()<<endl;
  tassert(sta2.elev()==5);
  sta=xyz(200,150,10);
  b=arc(beg,sta,end);
  sta2=b.station(252.905);
  cout<<"arc3 "<<sta2.east()<<' '<<sta2.north()<<' '<<sta2.elev()<<endl;
  //cout<<dist(sta,sta2)<<endl;
  tassert(dist(xy(sta),xy(sta2))<0.001);
  a=arc(beg1,end1,3);
  sta=a.station(10);
  sta1=a.station(15);
  sta2=a.station(20);
  cout<<"arc4 "<<sta.east()<<' '<<sta.north()<<endl;
  cout<<"arc4 "<<sta1.east()<<' '<<sta1.north()<<endl;
  cout<<"arc4 "<<sta2.east()<<' '<<sta2.north()<<endl;
  xx=(sta.east()+sta1.east()+sta2.east())/25;
  tassert(xx>3.6e-9 && xx<3.7e-9);
  tassert(rint(sta.east()/xx)==8);
  tassert(rint(sta1.east()/xx)==9);
  tassert(rint(sta2.east()/xx)==8);
  a.setcurvature(0.01,0.01);
  cout<<"setcurvature: radius="<<a.radius(0)<<endl;
  tassert(abs(a.radius(0)-100)<0.0001);
}

void testspiralarc()
{
  int i,j,nfail;
  double bear[3],len,begcur,endcur,arcarea,spiralarea;
  vector<double> extrema;
  xyz beg(0,0,3),end(300,400,7),sta,pi;
  xyz begk(6024724080.6769285,135556291815.4817,0),endk(82597158811.140015,-1116177821.7897496,0);
  xy ctr;
  xy kmlpnt(-337.97179595901059,364.38542430496875);
  arc kmlarc(begk,endk);
  spiralarc a(beg,end),b(beg,0.001,0.001,end),c,arch[10];
  spiralarc az0(xyz(189794.012,496960.750,0),1531654601,0,1145.229168e-6,60.96,0); // From a road in Arizona
  spiralarc kmlspi;
  arc aarc(beg,end);
  bezier3d a3d;
  PostScript ps;
  BoundRect br;
  kmlarc.setdelta(-1052617934);
  // This arc arises in the KML test, and the program blew its stack when compiled with MSVC.
  ps.open("spiralarc.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  tassert(fabs(a.length()-500)<0.001);
  tassert(a.chordlength()==500);
  cout<<b.length()<<' '<<b.curvature(200)<<endl;
  tassert(fabs(b.length()-505.361)<0.001);
  tassert(fabs(b.curvature(200)-0.001)<0.000001);
  a._setdelta(degtobin(60),degtobin(60));
  cout<<"chord bearing "<<bintodeg(a.chordbearing())<<endl;
  cout<<"bearing at beg "<<(bear[0]=bintodeg(a.bearing(0)))<<endl;
  cout<<"bearing at mid "<<(bear[1]=bintodeg(a.bearing(250)))<<endl;
  cout<<"bearing at end "<<(bear[2]=bintodeg(a.bearing(500)))<<endl;
  cout<<"delta "<<bear[2]-bear[0]<<" skew "<<bear[0]+bear[2]-2*bear[1]<<endl;
  a._fixends(1);
  cout<<"new length "<<(len=a.length())<<endl;
  cout<<"bearing at beg "<<(bear[0]=bintodeg(a.bearing(0)))<<endl;
  cout<<"bearing at mid "<<(bear[1]=bintodeg(a.bearing(len/2)))<<endl;
  cout<<"bearing at end "<<(bear[2]=bintodeg(a.bearing(len)))<<endl;
  cout<<"delta "<<bear[2]-bear[0]<<" skew "<<bear[0]+bear[2]-2*bear[1]<<endl;
  a.setdelta(0,degtobin(254)); // this barely fails
  a.setdelta(degtobin(26),degtobin(8));
  cout<<"new length "<<(len=a.length())<<endl;
  cout<<"bearing at beg "<<(bear[0]=bintodeg(a.bearing(0)))<<endl;
  cout<<"bearing at mid "<<(bear[1]=bintodeg(a.bearing(len/2)))<<endl;
  cout<<"bearing at end "<<(bear[2]=bintodeg(a.bearing(len)))<<endl;
  cout<<"delta "<<bear[2]-bear[0]<<" skew "<<bear[0]+bear[2]-2*bear[1]<<endl;
  tassert(fabs(bear[2]-bear[0]-26)<1e-5);
  tassert(fabs(bear[0]+bear[2]-2*bintodeg(a.chordbearing())-8)<1e-5);
  cout<<"curvature at beg "<<a.curvature(0)<<endl;
  cout<<"curvature at end "<<a.curvature(len)<<endl;
  a.split(200,b,c);
  sta=a.station(123);
  printf("a.station %f,%f,%f\n",sta.east(),sta.north(),sta.elev());
  sta=b.station(123);
  printf("b.station %f,%f,%f %f\n",sta.east(),sta.north(),sta.elev(),b.length());
  tassert(dist(b.station(123),a.station(123))<0.001);
  tassert(dist(c.station(200),a.station(400))<0.001);
  for (i=-20,nfail=0;i<=20;i++)
  {
    for (j=-20;j<=20;j++)
    {
      a.setcurvature(i/1000.,j/1000.);
      if (a.valid())
      {
	tassert(fabs(a.curvature(0)-i/1000.)<1e-6);
	tassert(fabs(a.curvature(a.length())-j/1000.)<1e-6);
	cout<<'.';
      }
      else
      {
	nfail++;
	cout<<' ';
      }
    }
    cout<<endl;
  }
  cout<<"setcurvature: "<<nfail<<" failures"<<endl;
  tassert(nfail>656 && nfail<1066);
  a.setdelta(radtobin(0.01),0);
  aarc.setdelta(radtobin(0.01),0);
  cout<<"spiralarc "<<a.diffarea()<<" arc "<<aarc.diffarea()<<
    ' '<<ldecimal(a.diffarea()/aarc.diffarea())<<endl;
  a.setdelta(DEG60,0);
  aarc.setdelta(DEG60,0);
  cout<<"spiralarc "<<a.diffarea()<<" arc "<<aarc.diffarea()<<
    ' '<<ldecimal(a.diffarea()/aarc.diffarea())<<endl;
  tassert(fabs(a.diffarea()/aarc.diffarea()-1)<1e-10);
  for (i=1;i<4;i++)
  {
    for (j=0;j<5;j++)
    {
      beg=xyz(cornu(-0.5,i*0.01,j*0.01),0);
      end=xyz(cornu(0.5,i*0.01,j*0.01),0);
      begcur=spiralcurvature(-0.5,i*0.01,j*0.01);
      endcur=spiralcurvature(0.5,i*0.01,j*0.01);
      c=spiralarc(beg,begcur,endcur,end);
      tassert(fabs(c.length()-1)<1e-12);
      spiralarea=c.diffarea();
      if (j)
        cout<<ldecimal(sqr(j*0.01)*(i*0.01)/(spiralarea-arcarea))<<' ';
      else
        arcarea=spiralarea;
    }
    cout<<endl;
  }
  ps.startpage();
  ps.setscale(-10,-10,10,10,degtobin(0));
  // Make something that resembles an Archimedean spiral
  //arch[0]=spiralarc(xyz(-0.5,0,0),2.,2/3.,xyz(1.5,0,0));
  arch[0]=spiralarc(xyz(-0.5,0,0),-DEG90,2.,2/3.,M_PI,0);
  for (i=1;i<10;i++)
    arch[i]=spiralarc(arch[i-1].getend(),arch[i-1].endbearing(),1/(i+0.5),1/(i+1.5),M_PI*(i+1),0);
  for (i=0;i<10;i++)
    a3d+=arch[i].approx3d(0.01);
  ps.spline(a3d);
  for (i=0;i<-10;i++)
  {
    if (i&1)
      ps.setcolor(1,0,0);
    else
      ps.setcolor(0,0,1);
    ps.spline(arch[i].approx3d(0.01));
  }
  cout<<"Archimedes-like spiral ended on "<<arch[9].getend().getx()<<','<<arch[9].getend().gety()<<endl;
  tassert(dist(arch[9].getend(),xy(-0.752,-10.588))<0.001);
  ps.endpage();
  ps.startpage();
  pi=xyz(az0.pointOfIntersection(),0);
  br.clear();
  br.include(az0.getstart());
  br.include(az0.getend());
  br.include(pi);
  ps.setscale(br);
  cout<<"Arizona spiral\nEndpoint: "<<az0.getend().getx()<<','<<az0.getend().gety()<<endl;
  tassert(dist(az0.getend(),xy(189780.746,496901.254))<0.0007);
  cout<<"Chord: "<<az0.chordlength()<<'<'<<bintodeg(az0.chordbearing())<<endl;
  cout<<"Point of intersection: "<<ldecimal(pi.getx())<<','<<ldecimal(pi.gety())<<endl;
  tassert(dist(pi,xy(189784.706,496921.187))<0.0007);
  cout<<"Tangents: "<<az0.tangentLength(START)<<", "<<az0.tangentLength(END)<<endl;
  tassert(fabs(az0.tangentLength(START)-40.643)<0.0007);
  tassert(fabs(az0.tangentLength(END)-20.322)<0.0007);
  ps.spline(az0.approx3d(0.001/ps.getscale()));
  ps.line2p(az0.getstart(),pi);
  ps.line2p(pi,az0.getend());
  ps.endpage();
  ps.trailer();
  ps.close();
  /* beardiff-delta=DEG360, which results in different code flow by MSVC than
   * by GCC. The problem is that the spiralarc equivalent of this arc, when
   * split by the code compiled by MSVC, produces NaN.
   */
  kmlspi=spiralarc(kmlarc);
  cout<<"kmlpnt is "<<(kmlspi.in(kmlpnt)?"":"not ")<<"in kmlspi\n";
  cout<<"kmlpnt is "<<(kmlarc.in(kmlpnt)?"":"not ")<<"in kmlarc\n";
  tassert(!kmlarc.in(kmlpnt));
}

void testchecksum()
{
  CoordCheck *check;
  unsigned n1,n2,i,less,more;
  n1=rng.uirandom();
  n2=rng.usrandom();
  n2=(n2<<8)+(n1&255);
  n1>>=8;
  cout<<hex<<"n1="<<n1<<" n2="<<n2<<dec<<endl;
  less=min(n1,n2);
  more=max(n1,n2);
  check=new CoordCheck;
  for (i=0;i<less;i++)
    *check<<0;
  *check<<(i==n1)+2*(i==n2);
  i++;
  for (;i<more;i++)
    *check<<0;
  *check<<(i==n1)+2*(i==n2);
  //check->dump();
  for (i=0;i<32;i++)
  {
    cout<<(*check)[i]<<' ';
    tassert((*check)[i]==(signed)(3-2*((n1>>i)&1)-4*((n2>>i)&1)));
  }
  cout<<endl;
  delete check;
}

void testldecimal()
{
  double d;
  bool looptests=false;
  cout<<ldecimal(1/3.)<<endl;
  cout<<ldecimal(M_PI)<<endl;
  cout<<ldecimal(-64664./65536,1./131072)<<endl;
  /* This is a number from the Alaska geoid file, -0.9867, which was output
   * as -.98669 when converting to GSF.
   */
  if (looptests)
  {
    for (d=M_SQRT_3-20*DBL_EPSILON;d<=M_SQRT_3+20*DBL_EPSILON;d+=DBL_EPSILON)
      cout<<ldecimal(d)<<endl;
    for (d=1.25-20*DBL_EPSILON;d<=1.25+20*DBL_EPSILON;d+=DBL_EPSILON)
      cout<<ldecimal(d)<<endl;
    for (d=95367431640625;d>1e-14;d/=5)
      cout<<ldecimal(d)<<endl;
    for (d=123400000000000;d>3e-15;d/=10)
      cout<<ldecimal(d)<<endl;
    for (d=DBL_EPSILON;d<=1;d*=2)
      cout<<ldecimal(M_SQRT_3,d)<<endl;
  }
  cout<<ldecimal(0)<<' '<<ldecimal(INFINITY)<<' '<<ldecimal(NAN)<<' '<<ldecimal(-5.67)<<endl;
  cout<<ldecimal(3628800)<<' '<<ldecimal(1296000)<<' '<<ldecimal(0.000016387064)<<endl;
  tassert(ldecimal(0)=="0");
  tassert(ldecimal(1)=="1");
  tassert(ldecimal(-1)=="-1");
  tassert(isalpha(ldecimal(INFINITY)[0]));
  tassert(isalpha(ldecimal(NAN)[1])); // on MSVC is negative, skip the minus sign
  tassert(ldecimal(1.7320508)=="1.7320508");
  tassert(ldecimal(1.7320508,0.0005)=="1.732");
  tassert(ldecimal(-0.00064516)=="-.00064516");
  tassert(ldecimal(3628800)=="3628800");
  tassert(ldecimal(1296000)=="1296e3");
  tassert(ldecimal(0.000016387064)=="1.6387064e-5");
  tassert(ldecimal(-64664./65536,1./131072)=="-.9867");
}

void testleastsquares()
{
  matrix a(3,2);
  vector<double> b,x;
  b.push_back(4);
  b.push_back(1);
  b.push_back(3);
  a[0][0]=1;
  a[0][1]=3; // http://ltcconline.net/greenl/courses/203/MatrixOnVectors/leastSquares.htm
  a[1][0]=2;
  a[1][1]=4;
  a[2][0]=1;
  a[2][1]=6;
  x=linearLeastSquares(a,b);
  cout<<"Least squares ("<<ldecimal(x[0])<<','<<ldecimal(x[1])<<")\n";
  tassert(dist(xy(x[0],x[1]),xy(-29/77.,51/77.))<1e-9);
  a.resize(2,3);
  b.clear();
  b.push_back(1);
  b.push_back(0);
  a[0][0]=1;
  a[0][1]=1; // https://www.math.usm.edu/lambers/mat419/lecture15.pdf
  a[0][2]=1;
  a[1][0]=-1;
  a[1][1]=-1;
  a[1][2]=1;
  x=minimumNorm(a,b);
  cout<<"Minimum norm ("<<ldecimal(x[0])<<','<<ldecimal(x[1])<<','<<ldecimal(x[2])<<")\n";
  tassert(dist(xyz(x[0],x[1],x[2]),xyz(0.25,0.25,0.5))<1e-9);
}

void test1adjelev(const double data[],int nData,double elevInt)
{
  int i;
  xyz pnt;
  triangle *tri;
  vector<triangle *> tri4;
  vector<point *> point5;
  net.clear();
  for (i=0;i<15;i+=3)
  {
    cout<<ldecimal(data[i])<<','<<ldecimal(data[i+1])<<','<<ldecimal(data[i+2])<<'\n';
    net.addpoint(i/3+1,point(data[i],data[i+1],data[i+2]));
    point5.push_back(&net.points[i/3+1]);
  }
  net.addtriangle(4);
  for (i=0;i<4;i++)
  {
    net.triangles[i].b=&net.points[i+1];
    net.triangles[i].a=&net.points[(i+1)%4+1];
    net.triangles[i].c=&net.points[5];
    net.triangles[i].flatten();
    tri4.push_back(&net.triangles[i]);
  }
  net.makeEdges();
  net.makeqindex();
  for (i=15;i<nData;i+=3)
  {
    pnt=xyz(data[i],data[i+1],data[i+2]);
    tri=net.findt(pnt);
    if (tri)
      tri->dots.push_back(pnt);
    else
      cout<<"point not in any triangle\n";
  }
  resizeBuckets(1);
  adjustElev(tri4,point5,-1,0);
  for (i=1;i<=5;i++)
    cout<<ldecimal(net.points[i].getx())<<','<<ldecimal(net.points[i].gety())<<','<<ldecimal(net.points[i].getz())<<'\n';
  tassert(fabs(net.points[5].getz()-elevInt)<0.001);
}

void testadjelev()
{
  AdjustBlockResult result;
  AdjustBlockTask task;
  int i;
  vector<int> blkSizes;
  static const double data0[]=
  /* This is actually a test of least squares. The data are from a debugging
   * session. The first five triples are points, the next nine are dots in
   * triangle 1, and the last is a dot in triangle 3. The dots are from
   * a point cloud supplied by 3DSurvey. The resulting matrix is singular,
   * but because of roundoff error, it appears not to be.
   */
  {
    3533555.8840822875,5383585.659137168,413.6458686964101,
    3533555.659112278,5383585.94544682,414.2803922905308,
    3533555.581388603,5383586.310384517,413.7572658044724,
    3533556.1608249694,5383585.9448964745,413.6527867551932,
    3533555.7510551126,5383585.945345965,0, // intersection is assigned 0
    3533555.9703733,5383586.0626831,413.6781603,
    3533556.0810758,5383585.9756126,413.6885076,
    3533555.9381925,5383586.0676994,413.6860052,
    3533555.9534971,5383586.0211678,413.6619764,
    3533555.9398481,5383586.0491371,413.6778418,
    3533555.9803755,5383586.0334473,413.6606813,
    3533556.0323545,5383586.017004,413.6864973,
    3533556.0233061,5383586.0072403,413.6559778,
    3533555.9591962998,5383586.0435181,413.6697737,
    3533555.865019,5383585.6876717,413.7036215
  };
  static const double data1[]=
  {
    5,0,31,
    0,-5,41,
    -5,0,59,
    0,5,26,
    0,0,53,
    -1,-3,58,
    1,-3,97,
    -3,-1,93,
    -1,-1,23,
    1,-1,84,
    3,-1,62,
    -3,1,64,
    -1,1,33,
    1,1,83,
    3,1,27,
    -1,3,95,
    1,3,2
  };
  clipHigh=500;
  test1adjelev(data0,45,413.608);
  test1adjelev(data1,51,49.25);
  computeAdjustBlock(task);
  blkSizes=blockSizes(86400); // 1024 times intertriangular. d/s is irrelevant.
  for (i=0;i<blkSizes.size();i++)
    cout<<blkSizes[i]<<((i+1>=blkSizes.size())?'\n':'+');
  blkSizes=blockSizes(500000);
  for (i=0;i<blkSizes.size();i++)
    cout<<blkSizes[i]<<((i+1>=blkSizes.size())?'\n':'+');
}

void testadjblock()
/* Computes the adjustment using dots that are on the plane z=2x+y+7 with
 * lots of noise added.
 */
{
  int i,j,h,phase,angle;
  double r,z;
  triangle *tri;
  xy xycoord;
  AdjustBlockTask task;
  AdjustBlockResult result;
  net.clear();
  net.addpoint(1,point(1,0,exp(1)));
  net.addpoint(2,point(-0.5,M_SQRT_3_4,M_PI));
  net.addpoint(3,point(-0.5,-M_SQRT_3_4,M_SQRT2));
  i=net.addtriangle();
  tassert(i==0);
  tri=&net.triangles[0];
  tri->a=&net.points[1];
  tri->b=&net.points[2];
  tri->c=&net.points[3];
  tri->flatten();
  h=(rng.usrandom()&4095)|1;
  tri->dots.resize(4096);
  tri->dots[0]=xyz(0,0,7);
  for (i=5;i<4096;i+=5)
  {
    r=sqrt(i/4096.);
    angle=rng.uirandom();
    phase=rng.uirandom();
    for (j=0;j<5;j++)
    {
      /* This was "cossin(angle+j*DEG72)", but 5*DEG72 is 0x80000002, which is
       * undefined behavior to compute as a signed int. In the package build
       * for Ubuntu, the compiler put both j*DEG72 and j*DEG144 in registers,
       * made j*DEG72>0X7fffffff the termination condition, then deleted it
       * because it can't happen, resulting in an infinite loop. Computing it
       * as unsigned fixes this bug.
       */
      xycoord=cossin((int)(angle+(unsigned)j*DEG72))*r;
      z=2*xycoord.getx()+xycoord.gety()+7+sin((int)(phase+(unsigned)j*DEG144));
      tri->dots[((i-j)*h)&4095]=xyz(xycoord,z);
    }
  }
  task.tri=tri;
  task.pnt.resize(3);
  for (i=0;i<3;i++)
    task.pnt[i]=&net.points[i+1];
  task.dots=&tri->dots[0];
  task.numDots=4096;
  task.result=&result;
  result.ready=false;
  computeAdjustBlock(task);
  tassert(result.ready);
  for (i=0;i<3;i++)
  {
    for (j=0;j<3;j++)
      printf("%7.3f ",result.mtmPart[i][j]);
    printf("    %7.3f\n",result.mtvPart[i][0]);
  }
  result.mtmPart.gausselim(result.mtvPart);
  for (i=0;i<3;i++)
    net.points[i+1].raise(result.mtvPart[i][0]);
  cout<<ldecimal(tri->elevation(xy(0,0)))<<endl;
  cout<<ldecimal(tri->elevation(xy(1,0)))<<endl;
  cout<<ldecimal(tri->elevation(xy(0,1)))<<endl;
  tassert(fabs(tri->elevation(xy(0,0))-7)<1e-9);
  tassert(fabs(tri->elevation(xy(1,0))-9)<1e-9);
  tassert(fabs(tri->elevation(xy(0,1))-8)<1e-9);
}

void testflip()
{
  double lengthBefore,lengthAfter;
  double areaBefore,areaAfter;
  int i;
  PostScript ps;
  ps.open("flip.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  setsurface(CIRPAR);
  aster(1500);
  makeOctagon();
  drawNet(ps);
  for (areaBefore=i=0;i<net.triangles.size();i++)
    areaBefore+=net.triangles[i].sarea;
  lengthBefore=net.edges[3].length();
  cout<<"length "<<lengthBefore<<' '<<net.triangles[2].dots.size()<<" dots in 2 "<<net.triangles[3].dots.size()<<" dots in 3\n";
  tassert(abs((int)net.triangles[2].dots.size()-402)<18);
  tassert(abs((int)net.triangles[3].dots.size()-402)<18);
  flip(&net.edges[3],-1);
  drawNet(ps);
  lengthAfter=net.edges[3].length();
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  cout<<"length "<<lengthAfter<<' '<<net.triangles[2].dots.size()<<" dots in 2 "<<net.triangles[3].dots.size()<<" dots in 3\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after\n";
  tassert(abs((int)net.triangles[2].dots.size()-729)<18);
  tassert(abs((int)net.triangles[3].dots.size()-75)<18);
  tassert(fabs(lengthBefore/lengthAfter-M_SQRT2)<0.02);
  tassert(fabs(areaAfter-areaBefore)<1e-6);
  tassert(net.checkTinConsistency());
  ps.close();
}

void testbend()
{
  double areaBefore,areaAfter;
  int i;
  PostScript ps;
  ps.open("bend.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  setsurface(CIRPAR);
  aster(1500);
  makeOctagon();
  drawNet(ps);
  cout<<"length 3 "<<net.edges[3].length()<<" length 4 "<<net.edges[4].length()<<' '<<net.triangles[3].dots.size()<<" dots in 3\n";
  tassert(abs((int)net.triangles[3].dots.size()-402)<15);
  for (areaBefore=i=0;i<net.triangles.size();i++)
    areaBefore+=net.triangles[i].sarea;
  bend(&net.edges[10],-1);
  drawNet(ps);
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  cout<<"length 10 "<<net.edges[10].length()<<' '<<net.triangles[3].dots.size()<<" dots in 3 "<<net.triangles[6].dots.size()<<" dots in 6\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after, ";
  cout<<(areaAfter-areaBefore)/areaBefore*100<<"% increase\n";
  tassert(abs((int)net.triangles[3].dots.size()-184)<15);
  tassert(abs((int)net.triangles[6].dots.size()-218)<15);
  tassert(fabs(net.edges[10].length()/(net.edges[3].length()+net.edges[4].length())-0.5307)<0.002);
  tassert(fabs(areaAfter-areaBefore-117)<10);
  tassert(net.checkTinConsistency());
  ps.close();
}

void testsplit()
{
  double areaBefore,areaAfter;
  int i;
  int dots3before,dots3after,dots6,dots7;
  PostScript ps;
  ps.open("split.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  setsurface(CIRPAR);
  aster(1500);
  makeOctagon();
  drawNet(ps);
  for (areaBefore=i=0;i<net.triangles.size();i++)
    areaBefore+=net.triangles[i].sarea;
  dots3before=net.triangles[3].dots.size();
  cout<<"Before: "<<dots3before<<" dots in 3\n";
  tassert(abs(dots3before-402)<15);
  split(&net.triangles[3],-1);
  drawNet(ps);
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  dots3after=net.triangles[3].dots.size();
  dots6=net.triangles[6].dots.size();
  dots7=net.triangles[7].dots.size();
  cout<<"After: "<<dots3after<<" dots in 3 "<<dots6<<" dots in 6 "<<dots7<<" dots in 7\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after\n";
  tassert(abs(dots3after-116)<15);
  tassert(abs(dots6-143)<15);
  tassert(abs(dots7-143)<15);
  tassert(fabs(areaAfter-areaBefore)<1e-6);
  tassert(net.checkTinConsistency());
  ps.close();
}

void testquarter()
{
  double areaBefore,areaAfter;
  int i;
  int dots1before,dots1after,dots2before,dots2after;
  int dots3before,dots3after,dots4before,dots4after;
  int dots6,dots7,dots8,dots9,dots10,dots11;
  PostScript ps;
  ps.open("quarter.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  setsurface(CIRPAR);
  aster(1500);
  makeOctagon();
  flip(&net.edges[3],-1); // This makes triangle 2 interior, a prerequisite for quartering.
  drawNet(ps);
  for (areaBefore=i=0;i<net.triangles.size();i++)
    areaBefore+=net.triangles[i].sarea;
  dots1before=net.triangles[1].dots.size();
  dots2before=net.triangles[2].dots.size();
  dots3before=net.triangles[3].dots.size();
  dots4before=net.triangles[4].dots.size();
  //tassert(abs(dots3before-402)<15);
  quarter(&net.triangles[2],-1);
  drawNet(ps);
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  dots1after=net.triangles[1].dots.size();
  dots2after=net.triangles[2].dots.size();
  dots3after=net.triangles[3].dots.size();
  dots4after=net.triangles[4].dots.size();
  dots6=net.triangles[6].dots.size();
  dots7=net.triangles[7].dots.size();
  dots8=net.triangles[8].dots.size();
  dots9=net.triangles[9].dots.size();
  dots10=net.triangles[10].dots.size();
  dots11=net.triangles[11].dots.size();
  cout<<"Before: "<<dots2before<<" dots in 2\n";
  cout<<"After: "<<dots2after<<" dots in 2 "<<dots6<<" dots in 6 "<<dots7<<" dots in 7 "<<dots8<<" dots in 8\n";
  cout<<"Before: "<<dots3before<<" dots in 3\n";
  cout<<"After: "<<dots3after<<" dots in 3 "<<dots9<<" dots in 9\n";
  cout<<"Before: "<<dots4before<<" dots in 4\n";
  cout<<"After: "<<dots4after<<" dots in 4 "<<dots10<<" dots in 10\n";
  cout<<"Before: "<<dots1before<<" dots in 1\n";
  cout<<"After: "<<dots1after<<" dots in 1 "<<dots11<<" dots in 11\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after\n";
  tassert(abs(dots2before-729)<17);
  tassert(abs(dots2after-186)<15);
  tassert(abs(dots6-181)<15);
  tassert(abs(dots7-181)<15);
  tassert(abs(dots8-181)<15);
  tassert(abs(dots3before-75)<15);
  tassert(fabs(dots3after-37.5)<15);
  tassert(fabs(dots9-37.5)<15);
  tassert(fabs(areaAfter-areaBefore)<1e-6);
  tassert(net.checkTinConsistency());
  ps.close();
}

void testcontour()
{
  double areaBefore,areaAfter;
  int i;
  int dots3before,dots3after,dots6,dots7;
  PostScript ps;
  ps.open("contour.ps");
  ps.setpaper(papersizes["A4 landscape"],0);
  ps.prolog();
  setsurface(CIRPAR);
  aster(1500);
  makeOctagon();
  drawNet(ps);
  for (areaBefore=i=0;i<net.triangles.size();i++)
    areaBefore+=net.triangles[i].sarea;
  dots3before=net.triangles[3].dots.size();
  cout<<"Before: "<<dots3before<<" dots in 3\n";
  split(&net.triangles[3],-1);
  split(&net.triangles[2],-1);
  flip(&net.edges[3],-1);
  drawNet(ps);
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  dots3after=net.triangles[3].dots.size();
  dots6=net.triangles[6].dots.size();
  dots7=net.triangles[7].dots.size();
  cout<<"After: "<<dots3after<<" dots in 3 "<<dots6<<" dots in 6 "<<dots7<<" dots in 7\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after\n";
  ps.close();
}

void test1stl(PostScript &ps,string name)
{
  int i;
  double bear;
  Printer3dSize printer;
  vector<StlTriangle> stltri;
  ofstream stlBinFile(name+"bin.stl",ios::binary);
  ofstream stlTxtFile(name+"txt.stl");
  makeOctagon();
  printer.shape=P3S_RECTANGULAR;
  printer.x=210;
  printer.y=297;
  printer.z=192;
  printer.minBase=10;
  for (i=0;i<180;i+=5)
    cout<<i<<' '<<hScale(net,printer,degtobin(i))<<endl;
  bear=turnFitInPrinter(net,printer);
  cout<<"Turn by "<<bintodeg(bear)<<endl;
  stltri=stlMesh(printer,false,false);
  writeStlBinary(stlBinFile,stltri);
  writeStlText(stlTxtFile,stltri);
  ps.startpage();
  ps.setscale(11,15,199,282); // setscale uses a 5% margin on each side
  for (i=0;i<stltri.size();i++)
  {
    ps.startline();
    ps.lineto(stltri[i].a);
    ps.lineto(stltri[i].b);
    ps.lineto(stltri[i].c);
    ps.endline(true);
    cout<<stltri[i].b.getx()<<' '<<stltri[i].b.gety()<<' '<<stltri[i].b.getz()<<endl;
  }
  ps.endpage();
}

void teststl()
{
  PostScript ps;
  ps.open("stl.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  setsurface(CIRPAR);
  lozenge(150);
  test1stl(ps,"lozenge");
  aster(150);
  test1stl(ps,"aster");
  ps.close();
}

void testoutlier()
// Generates point clouds with outliers.
{
  int i;
  ofstream file("outlier.xyz");
  setsurface(RUGAE);
  aster(4096);
  cloud.push_back(xyz(0,0,-1024));
  for (i=0;i<cloud.size();i++)
    writeXyzTextDot(file,cloud[i]);
}

xy intersection(polyline &p,xy start,xy end)
/* Given start and end, of which one is in p and the other is out,
 * returns a point on p. It can't use Brent's method because p.in
 * gives no clue about distance. It uses bisection.
 */
{
  xy mid;
  double sin,min,ein;
  sin=p.in(start);
  ein=p.in(end);
  min=9;
  while (fabs(min)>=0.75 || fabs(min)<=0.25)
  {
    mid=(start+end)/2;
    min=p.in(mid);
    if (dist(start,mid)==0 || dist(mid,end)==0)
      break;
    if (fabs(sin-min)>fabs(min-ein))
    {
      end=mid;
      ein=min;
    }
    else if (fabs(sin-min)<fabs(min-ein))
    {
      start=mid;
      sin=min;
    }
    else
      break;
  }
  return mid;
}

void testpolyline()
{
  int i;
  polyline p,p1;
  polyarc q,q1;
  polyspiral r,r1;
  PostScript ps;
  ofstream polyfile("polyline.dat");
  ifstream readfile;
  xy a(2,1.333),b(1.5,2),c(1.5000000001,2),d(1.499999999,2);
  xy e(3,0),f(3.5,0.5),g(0,4),mid;
  /* a: near centroid; b: center of circle, midpoint of hypot;
   * c and d: on opposite sites of b; e: corner;
   * f and g: other points on circle
   */
  cout<<"testpolyline"<<endl;
  ps.open("polyline.ps");
  ps.setpaper(papersizes["A4 portrait"],0);
  ps.prolog();
  r.smooth(); // sets the curvy flag
  bendlimit=DEG180+7;
  p.insert(xy(0,0));
  q.insert(xy(0,0));
  r.insert(xy(0,0));
  p.insert(xy(3,0));
  q.insert(xy(3,0));
  r.insert(xy(3,0));
  p.setlengths();
  q.setlengths();
  r.setlengths();
  cout<<p.length()<<' '<<r.length()<<endl;
  tassert(p.length()==6);
  tassert(fabs(r.length()-3*M_PI)<1e-6);
  p.insert(xy(3,4));
  q.insert(xy(3,4));
  r.insert(xy(3,4));
  p.setlengths();
  cout<<p.length()<<endl;
  tassert(p.length()==12);
  q.setlengths();
  tassert(q.length()==12);
  tassert(q.area()==6);
  r.setlengths();
  tassert(fabs(r.length()-5*M_PI)<1e-6);
  q.open();
  tassert(q.length()==7);
  tassert(std::isnan(q.area()));
  q.close();
  q.setlengths();
  tassert(q.length()==12);
  q.setdelta(0,439875013);
  q.setdelta(1,633866811);
  q.setdelta(2,1073741824);
  q.setlengths();
  /* Total area of circle is 19.6350. Of this,
   * 6.0000 is in the triangle,
   * 9.8175 is on the 5 side,
   * 2.7956 is on the 4 side, and
   * 1.0219 is on the 3 side.
   */
  cout<<"testpolyline: area of circle is "<<q.area()<<" (arc), "<<r.area()<<" (spiral)"<<endl;
  tassert(fabs(q.length()-M_PI*5)<0.0005);
  tassert(fabs(q.area()-M_PI*6.25)<0.0005);
  tassert(fabs(r.length()-M_PI*5)<0.0005);
  tassert(fabs(r.area()-M_PI*6.25)<0.0005);
  cout<<"checksums: p "<<hex<<p.checksum()<<" q "<<q.checksum()<<" r "<<r.checksum()<<dec<<endl;
  tassert(abs(foldangle(p.checksum()-q.checksum()))>1000);
  tassert(abs(foldangle(q.checksum()-r.checksum()))<10);
  p.write(polyfile);
  q.write(polyfile);
  r.write(polyfile);
  ps.startpage();
  ps.setscale(-1,-0.5,4,4.5);
  ps.spline(p.approx3d(0.001));
  ps.spline(q.approx3d(0.001));
  ps.spline(r.approx3d(0.001));
  ps.endpage();
  cout<<q.getarc(0).center().north()<<endl;
  cout<<q.length()<<endl;
  cout<<"p: a "<<p.in(a)<<" b "<<p.in(b)<<" c "<<p.in(c)<<" d "<<p.in(d)
    <<" e "<<p.in(e)<<" f "<<p.in(f)<<" g "<<p.in(g)<<endl;
  tassert(p.in(a)==1);
  tassert(p.in(b)==0.5);
  tassert(p.in(c)==1);
  tassert(p.in(d)==0);
  tassert(p.in(e)==0.25);
  tassert(p.in(f)==0);
  tassert(p.in(g)==0);
  cout<<"q: a "<<q.in(a)<<" b "<<q.in(b)<<" c "<<q.in(c)<<" d "<<q.in(d)
    <<" e "<<q.in(e)<<" f "<<q.in(f)<<" g "<<q.in(g)<<endl;
  tassert(q.in(a)==1);
  tassert(q.in(b)==1);
  tassert(q.in(c)==1);
  tassert(q.in(d)==1);
  tassert(q.in(e)==0.5);
  mid=intersection(q,b,2*f-b);
  cout<<"q x b-f ("<<ldecimal(mid.getx())<<','<<ldecimal(mid.gety())<<')'<<endl;
  tassert(dist(f,mid)<1e-8);
  mid=intersection(q,b,2*g-b);
  cout<<"q x b-g ("<<ldecimal(mid.getx())<<','<<ldecimal(mid.gety())<<')'<<endl;
  tassert(dist(g,mid)<1e-8);
  cout<<"r: a "<<r.in(a)<<" b "<<r.in(b)<<" c "<<r.in(c)<<" d "<<r.in(d)
    <<" e "<<r.in(e)<<" f "<<r.in(f)<<" g "<<r.in(g)<<endl;
  tassert(r.in(a)==1);
  tassert(r.in(b)==1);
  tassert(r.in(c)==1);
  tassert(r.in(d)==1);
  tassert(r.in(e)==0.5);
  mid=intersection(r,b,2*f-b);
  cout<<"r x b-f ("<<ldecimal(mid.getx())<<','<<ldecimal(mid.gety())<<')'<<endl;
  tassert(dist(f,mid)<1e-8);
  mid=intersection(r,b,2*g-b);
  cout<<"r x b-g ("<<ldecimal(mid.getx())<<','<<ldecimal(mid.gety())<<')'<<endl;
  tassert(dist(g,mid)<1e-8);
  polyfile.close();
  readfile.open("polyline.dat");
  p1.read(readfile);
  tassert(p1.area()==p.area());
  q1.read(readfile);
  tassert(q1.area()==q.area());
  r1.read(readfile);
  tassert(r1.area()==r.area());
  bendlimit=DEG120;
  r=polyspiral();
  for (i=0;i<600;i++)
  {
    r.insert(cossin(i*0xfc1ecd6));
    if (i==0)
      r.open();
  }
  r.smooth();
  r.setlengths();
  ps.startpage();
  ps.setscale(-1,-1,1,1);
  ps.spline(r.approx3d(0.001));
  ps.endpage();
  for (i=0;i<r.length();i++)
    tassert(fabs(r.station(i).length()-1)<1e-15);
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
  if (shoulddo("random"))
    testrandom(); // may take >7 s; time is random
  if (shoulddo("in"))
    testin();
  if (shoulddo("area3"))
    testarea3();
  if (shoulddo("matrix"))
    testmatrix();
  if (shoulddo("relprime"))
    testrelprime();
  if (shoulddo("manysum"))
    testmanysum();
  if (shoulddo("segment"))
    testsegment();
  if (shoulddo("arc"))
    testarc();
  if (shoulddo("spiral"))
    testspiral();
  if (shoulddo("spiralarc"))
    testspiralarc(); // 10.5 s
  if (shoulddo("checksum"))
    testchecksum(); // >1 s 3/4 of time
  if (shoulddo("ldecimal"))
    testldecimal();
  if (shoulddo("integertrig"))
    testintegertrig();
  if (shoulddo("leastsquares"))
    testleastsquares();
  if (shoulddo("adjelev"))
    testadjelev();
  if (shoulddo("adjblock"))
    testadjblock();
  if (shoulddo("flip"))
    testflip();
  if (shoulddo("bend"))
    testbend();
  if (shoulddo("split"))
    testsplit();
  if (shoulddo("quarter"))
    testquarter();
  if (shoulddo("contour"))
    testcontour();
  if (shoulddo("stl"))
    teststl();
  if (shoulddo("polyline"))
    testpolyline();
  if (shoulddo("outlier"))
    testoutlier();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
