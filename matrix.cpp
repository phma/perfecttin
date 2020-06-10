/******************************************************/
/*                                                    */
/* matrix.cpp - matrices                              */
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

#include <cassert>
#include <cstring>
#include <utility>
#include <iostream>
#include <iomanip>
#include "matrix.h"
#include "manysum.h"
#include "random.h"
#include "angle.h"

using namespace std;

matrix::matrix()
{
  rows=columns=0;
  entry=nullptr;
}

matrix::matrix(unsigned r,unsigned c)
{
  rows=r;
  columns=c;
  entry=new double[r*c];
  memset(entry,0,sizeof(double)*r*c);
}

matrix::matrix(const matrix &b)
{
  rows=b.rows;
  columns=b.columns;
  entry=new double[rows*columns];
  memcpy(entry,b.entry,sizeof(double)*rows*columns);
}

matrix::matrix(matrix &&b)
{
  rows=b.rows;
  columns=b.columns;
  entry=b.entry;
  b.rows=b.columns=0;
  b.entry=nullptr;
}

matrix::~matrix()
{
  delete[] entry;
}

void matrix::resize(unsigned newrows,unsigned newcolumns)
{
  unsigned fewrows,fewcolumns,i;
  double *newentry;
  newentry=new double[newrows*newcolumns];
  if (newrows<rows)
    fewrows=newrows;
  else
    fewrows=rows;
  if (newcolumns<columns)
    fewcolumns=newcolumns;
  else
    fewcolumns=columns;
  memset(newentry,0,sizeof(double)*newrows*newcolumns);
  for (i=0;i<fewrows;i++)
    memcpy(newentry+i*newcolumns,entry+i*columns,fewcolumns*sizeof(double));
  swap(newentry,entry);
  rows=newrows;
  columns=newcolumns;
  delete[] newentry;
}

void matrix::setzero()
{
  memset(entry,0,rows*columns*sizeof(double));
}

void matrix::setidentity()
{
  int i;
  if (rows!=columns)
    throw matrixmismatch;
  setzero();
  for (i=0;i<rows;i++)
    (*this)[i][i]=1;
}

void matrix::dump()
{
  int i,j,wid=10,prec=3;
  cout<<scientific;
  for (i=0;i<rows;i++)
  {
    for (j=0;j<columns;j++)
      cout<<setw(wid)<<setprecision(prec)<<(*this)[i][j];
    cout<<endl;
  }
}

matrix &matrix::operator=(const matrix &b)
{
  if (this!=&b)
  {
    matrix c(b);
    swap(rows,c.rows);
    swap(columns,c.columns);
    swap(entry,c.entry);
  }
  return *this;
}

matrix &matrix::operator=(matrix &&b)
{
  if (this!=&b)
  {
    swap(rows,b.rows);
    swap(columns,b.columns);
    swap(entry,b.entry);
  }
  return *this;
}

double *matrix::operator[](unsigned row)
{
  assert(row<rows);
  return entry+columns*row;
}

matrix matrix::operator+(matrix& b)
{
  if (rows!=b.rows || columns!=b.columns)
    throw matrixmismatch;
  matrix ret(*this);
  int i;
  for (i=0;i<rows*columns;i++)
    ret.entry[i]+=b.entry[i];
  return ret;
}

matrix matrix::operator-(matrix& b)
{
  if (rows!=b.rows || columns!=b.columns)
    throw matrixmismatch;
  matrix ret(*this);
  int i;
  for (i=0;i<rows*columns;i++)
    ret.entry[i]-=b.entry[i];
  return ret;
}

matrix matrix::operator*(matrix &b)
{
  if (columns!=b.rows)
    throw matrixmismatch;
  matrix ret(rows,b.columns);
  int i,j,k;
  double *sum;
  sum=new double[columns];
  for (i=0;i<rows;i++)
    for (j=0;j<b.columns;j++)
    {
      for (k=0;k<columns;k++)
	sum[k]=(*this)[i][k]*b[k][j];
      ret[i][j]=pairwisesum(sum,columns);
    }
  delete[] sum;
  return ret;
}

matrix matrix::transmult()
{
  matrix ret(rows,rows);
  int i,j,k;
  double *sum;
  sum=new double[columns];
  for (i=0;i<rows;i++)
    for (j=0;j<=i;j++)
    {
      for (k=0;k<columns;k++)
	sum[k]=(*this)[i][k]*(*this)[j][k];
      ret[i][j]=ret[j][i]=pairwisesum(sum,columns);
    }
  delete[] sum;
  return ret;
}

double matrix::trace()
{
  if (columns!=rows)
    throw matrixmismatch;
  manysum ret;
  int i;
  for (i=0;i<rows;i++)
    ret+=(*this)[i][i];
  return ret.total();
}

matrix matrix::transpose() const
{
  matrix ret(columns,rows);
  int i,j;
  for (i=0;i<rows;i++)
    for (j=0;j<columns;j++)
      ret.entry[j*rows+i]=entry[i*columns+j];
  return ret;
}

void matrix::swaprows(unsigned r0,unsigned r1)
{
  double *temp;
  temp=new double[columns];
  memcpy(temp,(*this)[r0],sizeof(double)*columns);
  memcpy((*this)[r0],(*this)[r1],sizeof(double)*columns);
  memcpy((*this)[r1],temp,sizeof(double)*columns);
  delete[] temp;
}

void matrix::swapcolumns(unsigned c0,unsigned c1)
{
  unsigned i;
  for (i=0;i<rows;i++)
    swap((*this)[i][c0],(*this)[i][c1]);
}

void matrix::randomize_c()
{
  int i;
  for (i=0;i<rows*columns;i++)
    entry[i]=(rng.ucrandom()*2-255)/BYTERMS;
}

rowsult matrix::rowop(matrix &b,int row0,int row1,int piv)
/* Does 0 or more of the elementary row operations:
 * 0: swap row0 and row1
 * 1: divide row0 by the number in the pivot column
 * 2: subtract row0 multiplied by the number in the
 *    pivot column of row1 from row1.
 * Bits 0, 1, or 2 of flags are set to tell what it did.
 * The pivot of row 0 is returned as detfactor, negated if it swapped rows.
 * If b is *this, it is ignored. If not, its rows are swapped, divided,
 * and subtracted along with this's rows.
 */
{
  rowsult ret;
  int i;
  double *temp,*rw0,*rw1,*rwb0,*rwb1;
  double slope,minslope=INFINITY;
  i=columns;
  if (b.columns>i)
    i=b.columns;
  temp=new double[i];
  rw0=(*this)[row0];
  rw1=(*this)[row1];
  if (this==&b)
    rwb0=rwb1=nullptr;
  else
  {
    rwb0=b[row0];
    rwb1=b[row1];
  }
  slope=ret.flags=0;
  if (piv>=0 && rw0[piv]==0 && rw1[piv]==0)
    piv=-1;
  ret.pivot=piv;
  if (piv>=0 && rw0[piv]==0)
    ret.flags=9;
  for (i=0;piv<0 && i<columns;i++) // Find a pivot if none was given.
    if (rw0[i]!=0 || rw1[i]!=0)
    {
      if (fabs(rw0[i])>fabs(rw1[i]) || row0>=row1)
      {
	slope=fabs(rw1[i]/rw0[i]);
	ret.flags&=~8;
      }
      else
      {
	slope=fabs(rw0[i]/rw1[i]);
	ret.flags|=8;
      }
      if (slope<minslope)
      {
	minslope=slope;
	ret.flags=(ret.flags>>3)*9;
	ret.pivot=i;
      }
    }
  ret.flags&=1;
  if (ret.flags)
  {
    memcpy(temp,rw0,sizeof(double)*columns);
    memcpy(rw0,rw1,sizeof(double)*columns);
    memcpy(rw1,temp,sizeof(double)*columns);
    if (rwb0)
    {
      memcpy(temp,rwb0,sizeof(double)*b.columns);
      memcpy(rwb0,rwb1,sizeof(double)*b.columns);
      memcpy(rwb1,temp,sizeof(double)*b.columns);
    }
  }
  if (ret.pivot<0)
    ret.detfactor=0;
  else
    ret.detfactor=rw0[ret.pivot];
  if (ret.detfactor!=0 && ret.detfactor!=1)
  {
    for (i=0;i<columns;i++)
      rw0[i]/=ret.detfactor;
    for (i=0;rwb0 && i<b.columns;i++)
      rwb0[i]/=ret.detfactor;
    ret.flags+=2;
  }
  if (ret.pivot>=0)
    slope=rw1[ret.pivot];
  if (slope!=0 && row0!=row1)
  {
    for (i=0;i<columns;i++)
      rw1[i]-=rw0[i]*slope;
    for (i=0;rwb0 && i<b.columns;i++)
      rwb1[i]-=rwb0[i]*slope;
    ret.flags+=4;
  }
  if (ret.flags&1)
    ret.detfactor=-ret.detfactor;
  delete[] temp;
  return ret;
}

void matrix::gausselim(matrix &b)
{
  int i,j;
  //dump();
  /*for (i=1;i<rows;i*=2)
    for (j=0;j+i<rows;j++)
      if ((j&i)==0)
	rowop(b,j,j+i,-1);*/
  for (i=0;i<rows;i++)
  {
    findpivot(b,i,i);
    for (j=0;j<rows;j++)
      rowop(b,i,j,i);
    //cout<<endl;
    //dump();
  }
  for (i=rows-1;i>=0;i--)
  {
    for (j=0;j<i;j++)
      rowop(b,i,j,i);
    //cout<<endl;
    //dump();
  }
  //cout<<endl;
  //b.dump();
}

bool matrix::findpivot(matrix &b,int row,int column)
/* Finds a pivot element in column, at or below row. If all elements are 0,
 * tries the next column to the right, until it runs out of matrix.
 * If the pivot element is not in row, it swaps two rows and returns true.
 * This tells _determinant to multiply by -1.
 */
{
  int i,j,pivotrow;
  double *squares,*ratios,*thisrow,maxratio;
  squares=new double[columns];
  ratios=new double[rows];
  for (pivotrow=-1,maxratio=0;pivotrow<row && column<columns;column++)
  {
    memset(ratios,0,rows*sizeof(double));
    for (i=row;i<rows;i++)
    {
      thisrow=(*this)[i];
      memset(squares,0,columns*sizeof(double));
      for (j=column+1;j<columns;j++)
	squares[j-column-1]=sqr(thisrow[j]);
      ratios[i]=sqr(thisrow[column])/pairwisesum(squares,columns-column);
      if (ratios[i]>maxratio)
      {
	pivotrow=i;
	maxratio=ratios[i];
      }
    }
  }
  if (pivotrow>row)
  {
    swaprows(pivotrow,row);
    if (&b!=this)
      b.swaprows(pivotrow,row);
  }
  delete[] ratios;
  delete[] squares;
  return pivotrow>row;
}

double matrix::_determinant()
{
  int i,j,lastpivot=-1,runlen;
  vector<double> factors;
  rowsult rsult;
  for (i=0;i<rows;i++)
  {
    if (findpivot(*this,i,i))
      factors.push_back(-1);
    for (j=i+1;j<rows;j++)
    {
      rsult=rowop(*this,i,j,i);
      if (rsult.detfactor!=1)
	factors.push_back(rsult.detfactor);
      if (i!=j)
      {
	if (rsult.pivot==lastpivot)
	  runlen++;
	else
	  runlen=0;
	lastpivot=rsult.pivot;
      }
    }
    //cout<<endl;
    //dump();
  }
  if (rows)
    factors.push_back((*this)[rows-1][columns-1]);
  else
    factors.push_back(1);
  for (i=1;i<factors.size();i*=2)
    for (j=0;j+i<factors.size();j+=2*i)
      factors[j]*=factors[j+i];
  return factors[0];
}

matrix invert(matrix m)
{
  matrix x(m),ret(m);
  ret.setidentity();
  x.gausselim(ret);
  if (x.getrows()>0 && x[x.getrows()-1][x.getrows()-1]==0)
    ret[0][0]=NAN;
  return ret;
}

double matrix::determinant()
{
  if (rows!=columns)
    throw matrixmismatch;
  matrix b(*this);
  return b._determinant();
}

matrix::operator vector<double>() const
{
  vector<double> ret;
  ret.resize(rows*columns);
  memcpy(&ret[0],entry,sizeof(double)*rows*columns);
  return ret;
}

matrix rowvector(const vector<double> &v)
{
  matrix ret(1,v.size());
  memcpy(ret[0],&v[0],sizeof(double)*v.size());
  return ret;
}

matrix columnvector(const vector<double> &v)
{
  matrix ret(v.size(),1);
  if (v.size())
    memcpy(ret[0],&v[0],sizeof(double)*v.size());
  return ret;
}
