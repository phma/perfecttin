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
  xy corners[8];
  int i;
  net.clear();
  for (i=0;i<cloud.size();i++)
  {
    orthogonal.include(cloud[i]);
    diagonal.include(cloud[i]);
  }
  cout<<"Orientation "<<ldecimal(bintodeg(ori),0.01)<<endl;
  cout<<"Orthogonal ("<<orthogonal.left()<<','<<orthogonal.bottom()<<")-("<<orthogonal.right()<<','<<orthogonal.top()<<")\n";
  cout<<"Diagonal ("<<diagonal.left()<<','<<diagonal.bottom()<<")-("<<diagonal.right()<<','<<diagonal.top()<<")\n";
}

