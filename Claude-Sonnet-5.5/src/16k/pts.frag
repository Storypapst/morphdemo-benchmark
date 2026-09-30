#version 430
layout(location=0) in vec4 C;
out vec4 o;
void main(){vec2 q=gl_PointCoord*2.-1.;float d=dot(q,q);o=C*exp(-d*3.5)*step(d,1.);}
