/******************************************************/
/*                                                    */
/* csv.cpp - comma-separated value                    */
/*                                                    */
/******************************************************/
/* Copyright 2017 Pierre Abbat.
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

#include "csv.h"
using namespace std;

vector<string> parsecsvline(string line)
{
  bool inquote,endquote;
  size_t pos;
  int ch;
  vector<string> ret;
  inquote=endquote=false;
  while (line.length())
  {
    if (ret.size()==0)
      ret.push_back("");
    pos=line.find_first_of("\",");
    if (pos<line.length())
      ch=line[pos];
    else
    {
      ch=-512;
      pos=line.length();
    }
    if (pos>0)
    {
      ret.back()+=line.substr(0,pos);
      line.erase(0,pos);
      endquote=false;
    }
    if (ch==',')
    {
      if (inquote)
	ret.back()+=ch;
      else
	ret.push_back("");
      endquote=false;
    }
    if (ch=='"')
    {
      if (inquote)
      {
	inquote=false;
	endquote=true;
      }
      else
      {
	if (endquote)
	  ret.back()+=ch;
	endquote=false;
	inquote=true;
      }
    }
    if (line.length())
      line.erase(0,1);
  }
  return ret;
}

string makecsvword(string word)
{
  string ret;
  size_t pos;
  if (word.find_first_of("\",")==string::npos)
    ret=word;
  else
  {
    ret="\"";
    while (word.length())
    {
      pos=word.find_first_of('"');
      if (pos==string::npos)
      {
	ret+=word;
	word="";
      }
      else
      {
	ret+=word.substr(0,pos+1)+'"';
	word.erase(0,pos+1);
      }
    }
    ret+='"';
  }
  return ret;
}

string makecsvline(vector<string> words)
{
  string ret;
  int i;
  if (words.size()==1 && words[0].length()==0)
    ret="\"\""; // special case: a single empty word must be quoted
  for (i=0;i<words.size();i++)
  {
    if (i)
      ret+=',';
    ret+=makecsvword(words[i]);
  }
  return ret;
}
