#version 330 core
out vec4 color;
uniform sampler2D S;
uniform vec2 R;
uniform float T;
float hash(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}
// Original monoline vector lettering, analytically antialiased.
float line(vec2 p,vec2 a,vec2 b){vec2 q=p-a,v=b-a;return length(q-v*clamp(dot(q,v)/dot(v,v),0.,1.));}
float letter(vec2 p,int ch){
 if(p.x<-.5||p.x>5.5||p.y<-.5||p.y>7.5)return 0.;
 float d=100.;
 vec2 a=vec2(0,0),b=vec2(0,7),c=vec2(5,7),e=vec2(5,0),m=vec2(0,3.5),n=vec2(5,3.5);
 if(ch==77){d=min(min(line(p,a,b),line(p,b,vec2(2.5,3.2))),min(line(p,vec2(2.5,3.2),c),line(p,c,e)));}
 if(ch==79){d=min(min(line(p,vec2(1,0),vec2(4,0)),line(p,vec2(1,7),vec2(4,7))),min(line(p,vec2(0,1),vec2(0,6)),line(p,vec2(5,1),vec2(5,6))));d=min(d,min(min(line(p,vec2(1,0),vec2(0,1)),line(p,vec2(0,6),vec2(1,7))),min(line(p,vec2(4,7),vec2(5,6)),line(p,vec2(5,1),vec2(4,0)))));}
 if(ch==82||ch==80){d=min(line(p,a,b),min(line(p,b,vec2(4,7)),line(p,m,vec2(4,3.5))));d=min(d,min(line(p,vec2(4,7),vec2(5,6)),min(line(p,vec2(5,6),vec2(5,4.5)),line(p,vec2(5,4.5),vec2(4,3.5)))));if(ch==82)d=min(d,line(p,vec2(2.3,3.5),e));}
 if(ch==72){d=min(line(p,a,b),min(line(p,c,e),line(p,m,n)));}
 if(ch==84){d=min(line(p,b,c),line(p,vec2(2.5,0),vec2(2.5,7)));}
 if(ch==69||ch==70){d=min(line(p,a,b),min(line(p,b,c),line(p,m,vec2(4,3.5))));if(ch==69)d=min(d,line(p,a,e));}
 if(ch==76){d=min(line(p,a,b),line(p,a,e));}
 if(ch==78){d=min(line(p,a,b),min(line(p,b,e),line(p,e,c)));}
 if(ch==73){d=min(line(p,vec2(2.5,0),vec2(2.5,7)),min(line(p,vec2(1,0),vec2(4,0)),line(p,vec2(1,7),vec2(4,7))));}
 if(ch==83){d=min(min(line(p,vec2(0,6),vec2(1,7)),line(p,vec2(1,7),c)),min(line(p,vec2(0,6),vec2(0,4.5)),line(p,vec2(0,4.5),vec2(5,2.5))));d=min(d,min(line(p,vec2(5,2.5),vec2(5,1)),min(line(p,vec2(5,1),vec2(4,0)),line(p,vec2(4,0),a))));}
 if(ch==68){d=min(line(p,a,b),min(line(p,b,vec2(3.5,7)),line(p,a,vec2(3.5,0))));d=min(d,min(line(p,vec2(3.5,7),vec2(5,5.5)),min(line(p,vec2(5,5.5),vec2(5,1.5)),line(p,vec2(5,1.5),vec2(3.5,0)))));}
 float aa=max(.05,fwidth(p.y)*.65);
 return 1.-smoothstep(.19-aa,.19+aa,d);
}
float title(vec2 p){
 int chars[5]=int[5](77,79,82,80,72);
 float a=0.;for(int i=0;i<5;i++)a=max(a,letter(p-vec2(float(i)*9.,0),chars[i]));return a;
}
float loom(vec2 p){
 int chars[8]=int[8](84,72,69,32,76,79,79,77);
 float a=0.;for(int i=0;i<8;i++)a=max(a,letter(p-vec2(float(i)*7.,0),chars[i]));return a;
}
float words(vec2 p,int which){
 int chars[15]=int[15](70,82,79,77,32,78,79,73,83,69,32,84,79,32,0);
 float a=0.;
 if(which==1){int q[5]=int[5](79,82,68,69,82);for(int i=0;i<5;i++)a=max(a,letter(p-vec2(float(i)*7.,0),q[i]));}
 else for(int i=0;i<14;i++)a=max(a,letter(p-vec2(float(i)*7.,0),chars[i]));
 return a;
}
vec3 aces(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.);}
void main(){
 float fit=min(R.x/1920.,R.y/1080.);
 vec2 uv=(gl_FragCoord.xy-R*.5)/(vec2(1920,1080)*fit)+.5;
 if(uv.x<0.||uv.x>1.||uv.y<0.||uv.y>1.){color=vec4(0,0,0,1);return;}
 vec2 aspect=vec2(1080./1920.,1.);
 vec3 c=texture(S,uv).rgb;
 vec3 bloom=vec3(0);
 for(int j=0;j<16;j++){
  float a=float(j)*2.399963;
  vec2 d=vec2(cos(a),sin(a))*sqrt((float(j)+.5)/16.);
  bloom+=max(texture(S,uv+d*.006*aspect).rgb-.24,0.)*.025;
  bloom+=max(texture(S,uv+d*.025*aspect).rgb-.35,0.)*.035;
 }
 c+=bloom;
 c*=pow(16.*uv.x*uv.y*(1.-uv.x)*(1.-uv.y),.14);
 c=pow(aces(c*1.38),vec3(1./2.2));
 c+=(hash(gl_FragCoord.xy+floor(T*24.))-.5)*.009;
 float end=smoothstep(49.,53.,T)*(1.-smoothstep(57.,60.,T));
 vec2 p=(uv-vec2(.5,.275))*vec2(1920,1080)/vec2(8.5,5.8);p+=vec2(20.5,3.5);
 float txt=title(p)*end;
 float start=smoothstep(2.,4.,T)*(1.-smoothstep(8.,11.,T));
 p=(uv-vec2(.5,.26))*vec2(1920,1080)/vec2(4.,2.5)+vec2(27.,3.5);
 txt=max(txt,loom(p)*start*.9);
 p=(uv-vec2(.5,.20))*vec2(1920,1080)/vec2(2.4,1.8)+vec2(44.5,3.5);
 txt=max(txt,words(p,0)*end*.5);
 p=(uv-vec2(.5,.175))*vec2(1920,1080)/vec2(2.4,1.8)+vec2(16.5,3.5);
 txt=max(txt,words(p,1)*end*.7);
 c=mix(c,vec3(.88,.89,.84),txt);
 float bars=smoothstep(.08,.081,uv.y)*smoothstep(.08,.081,1.-uv.y);
 color=vec4(c*bars,1.);
}
