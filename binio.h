/******************************************************/
/*                                                    */
/* binio.h - binary input/output                      */
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
#include <fstream>
#include <string>

void writebeshort(std::ostream &file,short i);
void writeleshort(std::ostream &file,short i);
short readbeshort(std::istream &file);
short readleshort(std::istream &file);
void writebeint(std::ostream &file,int i);
void writeleint(std::ostream &file,int i);
int readbeint(std::istream &file);
int readleint(std::istream &file);
void writebelong(std::ostream &file,long long i);
void writelelong(std::ostream &file,long long i);
long long readbelong(std::istream &file);
long long readlelong(std::istream &file);
void writebefloat(std::ostream &file,float f);
void writelefloat(std::ostream &file,float f);
float readbefloat(std::istream &file);
float readlefloat(std::istream &file);
void writebedouble(std::ostream &file,double f);
void writeledouble(std::ostream &file,double f);
double readbedouble(std::istream &file);
double readledouble(std::istream &file);
void writegeint(std::ostream &file,int i); // for Decisite's geoid files
int readgeint(std::istream &file);
void writeustring(std::ostream &file,std::string s);
std::string readustring(std::istream &file);

