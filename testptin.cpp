/******************************************************/
/*                                                    */
/* testptin.cpp - test program                        */
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
#include "octagon.h"
#include "edgeop.h"
#include "triop.h"
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
  ms.prune();
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
  ms.prune();
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
  ms.prune();
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
  ms.prune();
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
  tassert(abs((int)net.triangles[2].dots.size()-402)<15);
  tassert(abs((int)net.triangles[3].dots.size()-402)<15);
  flip(&net.edges[3]);
  drawNet(ps);
  lengthAfter=net.edges[3].length();
  for (areaAfter=i=0;i<net.triangles.size();i++)
  {
    tassert(net.triangles[i].sarea>1);
    areaAfter+=net.triangles[i].sarea;
  }
  cout<<"length "<<lengthAfter<<' '<<net.triangles[2].dots.size()<<" dots in 2 "<<net.triangles[3].dots.size()<<" dots in 3\n";
  cout<<"Area "<<areaBefore<<" before, "<<areaAfter<<" after\n";
  tassert(abs((int)net.triangles[2].dots.size()-729)<15);
  tassert(abs((int)net.triangles[3].dots.size()-75)<15);
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
  bend(&net.edges[10]);
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
  split(&net.triangles[3]);
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
  flip(&net.edges[3]); // This makes triangle 2 interior, a prerequisite for quartering.
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
  tassert(abs(dots2before-729)<15);
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
    testmanysum(); // >2 s
  if (shoulddo("ldecimal"))
    testldecimal();
  if (shoulddo("leastsquares"))
    testleastsquares();
  if (shoulddo("flip"))
    testflip();
  if (shoulddo("bend"))
    testbend();
  if (shoulddo("split"))
    testsplit();
  if (shoulddo("quarter"))
    testquarter();
  cout<<"\nTest "<<(testfail?"failed":"passed")<<endl;
  return testfail;
}
