/******************************************************/
/*                                                    */
/* octagon.cpp - bound the points with an octagon     */
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
#include <iostream>
#include "octagon.h"
#include "angle.h"
#include "ply.h"
#include "tin.h"
#include "random.h"
#include "ldecimal.h"
#include "boundrect.h"
#include "cogo.h"

using namespace std;

pointlist net;

void makeOctagon()
/* Creates an octagon which encloses cloud (defined in ply.cpp) and divides it
 * into six triangles.
 */
{
  int ori=rng.uirandom();
  BoundRect orthogonal(ori),diagonal(ori+DEG45);
  double bounds[8],width,margin;
  xy corners[8];
  int i;
  net.clear();
  for (i=0;i<cloud.size();i++)
  {
    orthogonal.include(cloud[i]);
    diagonal.include(cloud[i]);
  }
  bounds[0]=orthogonal.left();
  bounds[1]=diagonal.bottom();
  bounds[2]=orthogonal.bottom();
  bounds[3]=-diagonal.right();
  bounds[4]=-orthogonal.right();
  bounds[5]=-diagonal.top();
  bounds[6]=-orthogonal.top();
  bounds[7]=diagonal.left();
  for (i=0;i<4;i++)
  {
    width=-bounds[i]-bounds[i+4];
    margin=width/sqrt(cloud.size());
    bounds[i]-=margin;
    bounds[i+4]-=margin;
  }
  for (i=0;i<8;i++)
  {
    corners[i]=intersection(cossin(i*DEG45-ori)*bounds[i],(i+2)*DEG45-ori,cossin((i+1)*DEG45-ori)*bounds[(i+1)%8],(i+3)*DEG45-ori);
    net.addpoint(i+1,point(corners[i],0));
  }
  cout<<"Orientation "<<ldecimal(bintodeg(ori),0.01)<<endl;
  cout<<"Orthogonal ("<<orthogonal.left()<<','<<orthogonal.bottom()<<")-("<<orthogonal.right()<<','<<orthogonal.top()<<")\n";
  cout<<"Diagonal ("<<diagonal.left()<<','<<diagonal.bottom()<<")-("<<diagonal.right()<<','<<diagonal.top()<<")\n";
}

