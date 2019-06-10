/******************************************************/
/*                                                    */
/* dxf.cpp - Drawing Exchange Format                  */
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

#include <cstring>
#include "dxf.h"
#include "binio.h"
#include "ldecimal.h"
using namespace std;

TagRange tagTable[]=
{
  {0,128}, // string
  {10,72}, // xyz. All xyz's are stored as three separate doubles.
  {40,72}, // double
  {60,2}, // 2-byte short
  {80,0}, // invalid
  {90,4}, // 4-byte int
  {100,128},
  {101,0},
  {102,128},
  {103,0},
  {105,132}, // hex string
  {106,0},
  {110,72}, // 110-139 are three doubles that go together. 140-149 is a scalar.
  {150,0},
  {160,8}, // 8-byte long
  {170,2},
  {180,0},
  {210,72},
  {240,0},
  {270,2},
  {290,1}, // bool
  {300,128},
  {310,129}, // hex string binary chunk
  {320,132}, // hex string handle
  {370,2},
  {390,132},
  {400,2},
  {410,128},
  {420,4},
  {430,128},
  {440,4},
  {450,8},
  {460,72},
  {470,128},
  {480,132},
  {482,0},
  {999,128},
  {1010,72},
  {1060,2},
  {1071,4},
  {1072,0}
};

int tagFormat(int tag)
{
  int lo=0,hi=sizeof(tagTable)/sizeof(tagTable[0]),mid;
  while (hi-lo>1)
  {
    mid=(lo+hi)/2;
    if (tagTable[mid].tag>tag)
      hi=mid;
    else
      lo=mid;
  }
  if (tagTable[lo].tag>tag)
    return 0;
  else
    return tagTable[lo].format;
}

GroupCode::GroupCode()
{
  tag=-1;
}

GroupCode::GroupCode(int tag0)
{
  tag=tag0;
  switch (tagFormat(tag))
  {
    case 1:
      flag=false;
      break;
    case 2:
    case 4:
    case 8:
    case 132:
      integer=0;
      break;
    case 72:
      real=0;
      break;
    case 128:
    case 129:
      new (&str) string();
      break;
  }
}

GroupCode::GroupCode(const GroupCode &b)
{
  tag=b.tag;
  switch (tagFormat(tag))
  {
    case 1:
      flag=b.flag;
      break;
    case 2:
    case 4:
    case 8:
    case 132:
      integer=b.integer;
      break;
    case 72:
      real=b.real;
      break;
    case 128:
    case 129:
      new (&str) string(b.str);
      break;
  }
}

GroupCode& GroupCode::operator=(const GroupCode &b)
{
  if ((tagFormat(tag)&-2)==128 && (tagFormat(b.tag)&-2)!=128)
    str.~string();
  if ((tagFormat(tag)&-2)!=128 && (tagFormat(b.tag)&-2)==128)
    new (&str) string();
  tag=b.tag;
  switch (tagFormat(tag))
  {
    case 1:
      flag=b.flag;
      break;
    case 2:
    case 4:
    case 8:
    case 132:
      integer=b.integer;
      break;
    case 72:
      real=b.real;
      break;
    case 128:
    case 129:
      str=b.str;
      break;
  }
}

GroupCode::~GroupCode()
{
  switch (tagFormat(tag))
  {
    case 128:
    case 129:
      str.~string();
      break;
  }
}

long long hexDecodeInt(string str)
{
  int i,ch;
  long long ret=0;
  for (i=0;i<str.length();i++)
  {
    ch=str[i];
    if (ch>'_')
      ch&=0xdf; // convert abcdef to ABCDEF
    if (ch>'9')
      ch-='A'-('9'+1);
    ret=(ret<<4)+(ch-'0');
  }
  return ret;
}

string hexEncodeInt(long long num)
{
  bool nonzero=false;
  char nybble;
  int i;
  string ret;
  for (i=28;i>=0;i-=4)
  {
    nybble=(num>>i)&15;
    if (nybble)
      nonzero=true;
    if (nonzero || !i)
      ret+=(nybble>9)?(nybble+'0'):(nybble+'A'-10);
  }
  return ret;
}

string hexDecodeString(string str)
{
  int i,ch;
  char byte;
  string ret;
  for (i=0;i<str.length();i++)
  {
    ch=str[i];
    if (ch>'_')
      ch&=0xdf;
    if (ch>'9')
      ch-='A'-('9'+1);
    byte=(byte<<4)+(ch-'0');
    if (i&1)
      ret+=byte;
  }
  return ret;
}

string hexEncodeString(string str)
{
  int i,ch;
  char nybble;
  string ret;
  for (i=0;i<str.length();i++)
  {
    ch=str[i];
    nybble=(ch>>4)&15;
    ret+=(nybble>9)?(nybble+'0'):(nybble+'A'-10);
    nybble=ch&15;
    ret+=(nybble>9)?(nybble+'0'):(nybble+'A'-10);
  }
  return ret;
}

GroupCode readDxfText(istream &file)
{
  string tagstr,datastr;
  GroupCode ret;
  getline(file,tagstr);
  getline(file,datastr);
  if (file.good())
  {
    try
    {
      ret=GroupCode(stoi(tagstr));
      switch (tagFormat(ret.tag))
      {
	case 1: // bools are stored in text as numbers
	  ret.flag=stoi(datastr)!=0;
	  break;
	case 2:
	case 4:
	case 8:
	  ret.integer=stoll(datastr);
	  break;
	case 72:
	  ret.real=stod(datastr);
	  break;
	case 128:
	  ret.str=datastr;
	  break;
	case 129:
	  ret.str=hexDecodeString(datastr);
	  break;
	case 132:
	  ret.integer=hexDecodeInt(datastr);
	  break;
      }
    }
    catch (...)
    {
      ret=GroupCode(-2);
    }
  }
  return ret;
}

GroupCode readDxfBinary(istream &file)
{
  GroupCode ret;
  int tag;
  tag=readleshort(file);
  if (file.good())
  {
    ret=GroupCode(tag);
    switch (tagFormat(ret.tag))
    {
      case 1:
	ret.flag=file.get();
	break;
      case 2:
	ret.integer=readleshort(file);
	break;
      case 4:
	ret.integer=readleint(file);
	break;
      case 8:
	ret.integer=readlelong(file);
	break;
      case 72:
	ret.real=readledouble(file);
	break;
      case 128:
	ret.str=readustring(file);
	break;
      case 129:
	ret.str=hexDecodeString(readustring(file));
	break;
      case 132:
	ret.integer=hexDecodeInt(readustring(file));
	break;
    }
  }
  return ret;
}

void writeDxfText(std::ostream &file,GroupCode code)
{
  string tagstr,datastr;
  tagstr=to_string(code.tag);
  switch(tagFormat(code.tag))
  {
    case 1: // bools are stored in text as numbers
      datastr=to_string((int)code.flag);
      break;
    case 2:
    case 4:
    case 8:
      datastr=to_string(code.integer);
      break;
    case 72:
      datastr=ldecimal(code.real);
      break;
    case 128:
      datastr=code.str;
      break;
    case 129:
      datastr=hexEncodeString(code.str);
      break;
    case 132:
      datastr=hexEncodeInt(code.integer);
      break;
  }
  file<<tagstr<<'\n'<<datastr<<'\n';
}

void writeDxfBinary(std::ostream &file,GroupCode code)
{
  writeleshort(file,code.tag);
  switch(tagFormat(code.tag))
  {
    case 1:
      file.put(code.flag);
      break;
    case 2:
      writeleshort(file,code.integer);
      break;
    case 4:
      writeleint(file,code.integer);
      break;
    case 8:
      writelelong(file,code.integer);
      break;
    case 72:
      writeledouble(file,code.real);
      break;
    case 128:
      writeustring(file,code.str);
      break;
    case 129:
      writeustring(file,hexEncodeString(code.str));
      break;
    case 132:
      writeustring(file,hexEncodeInt(code.integer));
      break;
  }
}

bool readDxfMagic(istream &file)
/* Looks for the seven bytes DXF^M^J^Z^@. If any control character appears before
 * DXF, returns false.
 */
{
  char buf[4]={"   "};
  while (!(buf[0]=='D' && buf[1]=='X' && buf[2]=='F') && buf[2]>=' ' && buf[2]<128 && file.good())
  {
    buf[0]=buf[1];
    buf[1]=buf[2];
    buf[2]=file.get();
  }
  if (!strcmp(buf,"DXF"))
    file.read(buf,4);
  return (file.good() && !strcmp(buf,"\r\n\032"));
}

void writeDxfMagic(ostream &file)
{
  file<<"DXF\r\n\032"<<'\0';
}

vector<GroupCode> readDxfGroups(istream &file,bool mode)
// mode is true for text.
{
  GroupCode oneCode;
  vector<GroupCode> ret;
  bool cont=true;
  if (!mode)
    cont=readDxfMagic(file);
  while (cont)
  {
    if (mode)
      oneCode=readDxfText(file);
    else
      oneCode=readDxfBinary(file);
    if (tagFormat(oneCode.tag))
      ret.push_back(oneCode);
    else
    {
      cont=false;
      if (file.good() || oneCode.tag+1) // An unknown tag was read.
	ret.clear();
    }
  }
  return ret;
}

void writeDxfGroups(ostream &file,vector<GroupCode> &codes,bool mode)
{
  int i;
  if (!mode)
    writeDxfMagic(file);
  for (i=0;i<codes.size();i++)
    if (mode)
      writeDxfText(file,codes[i]);
    else
      writeDxfBinary(file,codes[i]);
}

vector<GroupCode> readDxfGroups(string filename)
{
  int mode;
  ifstream file;
  vector<GroupCode> ret;
  for (mode=0;mode<2 && ret.size()==0;mode++)
  {
    file.open(filename,ios::binary);
    ret=readDxfGroups(file,mode);
    file.close();
  }
  return ret;
}

vector<array<xyz,3> > extractTriangles(vector<GroupCode> dxfData)
/* Scans dxfData looking for 3DFACE objects, extracting the first three
 * corners of each. The fourth corner is ignored. It should be the same as
 * one of the first three. If it isn't, the 3DFACE is a quadrilateral,
 * which might should be turned into two triangles.
 * 
 * This function ignores sections; if there's a 3DFACE in the BLOCKS section,
 * it will output it once, not every place the block is used.
 */
{
  int i,ncorner,coord;
  array<xyz,3> face;
  vector<array<xyz,3> > ret;
  int ncoords=16;
  for (i=0;i<dxfData.size();i++)
  {
    if (dxfData[i].tag==0 && dxfData[i].str=="3DFACE")
      ncoords=0;
    if (ncoords<12 && dxfData[i].tag>=10 && dxfData[i].tag<40)
    {
      coord=dxfData[i].tag/10;
      ncorner=dxfData[i].tag%10;
      if (ncorner<3)
	switch (coord)
	{
	  case 1:
	    face[ncorner]=xyz(dxfData[i].real,face[ncorner].gety(),face[ncorner].getz());
	    break;
	  case 2:
	    face[ncorner]=xyz(face[ncorner].getx(),dxfData[i].real,face[ncorner].getz());
	    break;
	  case 3:
	    face[ncorner]=xyz(face[ncorner].getx(),face[ncorner].gety(),dxfData[i].real);
	    break;
	}
      if (12==++ncoords)
	ret.push_back(face);
    }
  }
  return ret;
}

void insertXy(vector<GroupCode> &dxfData,int xtag,xy pnt)
// xtag is the tag of the x-coordinate
{
  GroupCode xCode(xtag),yCode(xtag+10);
  xCode.real=pnt.getx();
  yCode.real=pnt.gety();
  dxfData.push_back(xCode);
  dxfData.push_back(yCode);
}

void insertXyz(vector<GroupCode> &dxfData,int xtag,xyz pnt)
// xtag is the tag of the x-coordinate
{
  GroupCode xCode(xtag),yCode(xtag+10),zCode(xtag+20);
  xCode.real=pnt.getx();
  yCode.real=pnt.gety();
  zCode.real=pnt.getz();
  dxfData.push_back(xCode);
  dxfData.push_back(yCode);
  dxfData.push_back(zCode);
}

void dxfHeader(vector<GroupCode> &dxfData,BoundRect br)
{
  GroupCode comment(999),sectag(0),secname(2),paramtag(9),stringval(1);
  comment.str="DXF created by DeciSite";
  dxfData.push_back(comment);
  sectag.str="SECTION";
  secname.str="HEADER";
  dxfData.push_back(sectag);
  dxfData.push_back(secname);
  paramtag.str="$ACADVER";
  stringval.str="AC1006";
  dxfData.push_back(paramtag);
  dxfData.push_back(stringval);
  paramtag.str="$INSBASE";
  dxfData.push_back(paramtag);
  insertXyz(dxfData,10,xyz(0,0,0));
  paramtag.str="$EXTMIN";
  dxfData.push_back(paramtag);
  insertXy(dxfData,10,xy(br.left(),br.bottom()));
  paramtag.str="$EXTMAX";
  dxfData.push_back(paramtag);
  insertXy(dxfData,10,xy(br.right(),br.top()));
  sectag.str="ENDSEC";
  dxfData.push_back(sectag);
}

void insertTriangle(vector<GroupCode> &dxfData,triangle &tri)
{
  GroupCode entityType(0),layerName(8),colorNumber(62);
  entityType.str="3DFACE";
  layerName.str="0";
  colorNumber.integer=0;
  dxfData.push_back(entityType);
  dxfData.push_back(layerName);
  dxfData.push_back(colorNumber);
  insertXyz(dxfData,10,*tri.a);
  insertXyz(dxfData,11,*tri.b);
  insertXyz(dxfData,12,*tri.c); // A 3DFACE always has four corners. That it's a triangle
  insertXyz(dxfData,13,*tri.c); // is indicated by repeating a corner.
}
