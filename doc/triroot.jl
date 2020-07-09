#!/usr/bin/julia -i
function triroot(x::Real)
  √(2x+1/4)-1/2
end

function dec(x::Real)
  y=triroot(x)
  y+(2y-1)sin(2π*y)/4π
end

function rmdr(x::Real)
  x-dec(x)
end
