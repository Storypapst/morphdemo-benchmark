#version 430
layout(location=0) uniform ivec3 u;
out vec4 o;

float T=float(u.x)*.001;
vec2 RS=vec2(u.yz);
float O,S,K,FIN;

mat2 R(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
float hs(vec3 p){return F(p.x*13.+p.y*571.+p.z*3119.+1e4);}

const uint G[5]=uint[](0x11DD631u,0xE8C62Eu,0x1E8FA51u,0x1E8FA10u,0x118FE31u);

vec3 cam,RD,LQ,LN;
const float WZ=64.;
float edg,fw,SZ,MO;
vec3 NW;
float map(vec3 p){
  fw=smoothstep(.5,1.,FIN);
  float dw=abs(p.z-2.*WZ)-12.;                                // the finale only keeps a slab of lattice around the word
  if(FIN>.5&&dw>0.){edg=0.;return dw+.15;}
  vec3 c=floor(p*.5+.5);
  vec3 l=p-2.*c;
  float r=hs(c);
  vec3 j=vec3(hs(c+1.),hs(c+2.),hs(c+3.))-.5;
  float gl=0.;
  if(c.z==WZ){
    float col=c.x+14.,row=2.-c.y;
    if(col>=0.&&col<29.&&row>=0.&&row<5.){
      int li=int(col/6.);float cx=col-float(li)*6.,lt=T-44.-float(li)*.5;     // the letters pop up one after the other on the beats of the drop
      if(cx<5.) gl=float((G[li]>>uint(24.-(row*5.+cx)))&1u)*step(0.,lt)*(1.-exp(-lt*9.)*cos(lt*14.));
    }
  }
  float dens=mix(.06+.2*smoothstep(0.,18.,T),.55,smoothstep(12.,24.,T));
  float sz=mix(.32*step(r,dens),.03,fw);
  float dc=length(2.*c-cam);
  float pw=exp(-fract(T*2.-dc*.04)*6.)*smoothstep(20.,30.,T);
  sz*=(1.+.3*pw+.25*K+fw*pw)*smoothstep(4.,9.,dc);
  float rr=length(c.xy)*2.+log(K+.001)*5.;                     // ring that leaves the centre with every kick
  if(c.z==WZ) sz=max(sz,(.55*gl*(1.+.15*K)+.15*K*exp(-rr*rr*.03))*fw);
  l-=j*(1.-O)*1.1;
  vec3 l0=l;
  float mo=smoothstep(26.,30.,T)*(1.-smoothstep(37.,41.,T))*smoothstep(.2,.8,.5+.5*sin(T*1.3+c.x*.5+c.z*.35))*(1.-fw);   // cubes melt into spheres, cell by cell
  l.xy*=R((r-.5)*(1.-O)*9.+T*.1*(1.-O));
  l.yz*=R((hs(c+7.)-.5)*(1.-O)*9.);
  vec3 q=abs(l)-sz;
  float e=.02+sz*mo,d=length(max(q+e,0.))+min(max(q.x,max(q.y,q.z))+e,0.)-e;
  vec3 tt=(sign(RD)-(p-2.*c))/RD;
  float bd=min(tt.x,min(tt.y,tt.z))+.002;
  SZ=sz;MO=mo;NW=normalize(l0);
  if(sz<.01) return bd;
  LQ=q;
  LN=vec3(step(q.yzx,q.xyz)*step(q.zxy,q.xyz))*sign(l);
  edg=step(1.5,dot(step(abs(q),vec3(.04)),vec3(1)));
  return min(d,bd);
}

void main(){
  float asp=min(RS.y,RS.x/1.7778);
  vec2 p=(gl_FragCoord.xy-.5*RS)/asp;
  K=KE(T);
  O=ORD(T);S=STA(T);
  FIN=smoothstep(38.,44.,T);
  float ck=CK(T);
  vec2 uv=p;
  float ty=floor(gl_FragCoord.y/8.);
  float gt=step(.985-.1*S,F(ty*7.+floor(T*12.)*131.));
  uv.x+=gt*(F(ty)-.5)*.4*S;
  uv*=1.-.04*K;
  cam=vec3(sin(T*.3)*2.*(1.-O*.7),cos(T*.21)*2.*(1.-O*.7),min(T,44.)*1.4+max(T-44.,0.)*.5);
  vec3 rd=normalize(vec3(uv,1.2));
  rd.xy*=R(sin(T*.13)*(1.-O)*.8+.12*sin(T*.37)*O*(1.-FIN));
  RD=rd;
  float t=0.,gw=0.,d=1.;
  for(int i=0;i<100;i++){
    d=map(cam+rd*t);
    gw+=exp(-d*10.)*d*.05*(1.+K);
    if(d<.002||t>100.)break;
    t+=d*.85;
  }
  vec3 col=vec3(0);
  if(d<.01){
    map(cam+rd*t);
    vec3 ec=mix(mix(vec3(1),vec3(.2,.9,1.),O),vec3(1.,.35,.8),smoothstep(28.,31.,T)*(1.-smoothstep(37.,40.,T)));
    float lit=max(dot(LN,normalize(vec3(.4,.8,-.5))),0.);
    float wl=fw*step(.3,SZ);                                    // a letter cube of the finale
    col=mix(vec3(.004,.006,.01)+.025*lit,mix(vec3(1.,.62,.25),vec3(1.,.9,.7),smoothstep(44.,58.,T)*.6)*(.5+lit),wl);
    col+=edg*(1.-MO)*mix(ec,vec3(1.,.8,.5),wl)*(1.2+2.*K)*(1.-.6*fw*(1.-wl));
    col+=MO*(pow(1.-max(dot(NW,-rd),0.),2.5)*1.6+max(dot(NW,vec3(.4,.8,-.5)),0.)*.25)*ec;
    col*=exp(-t*.03*(1.-.8*wl));
  }
  col+=(1.-exp(-gw*1.5))*vec3(.1,.35,.8)*.5*(1.-.85*fw);
  float n=F(gl_FragCoord.x+gl_FragCoord.y*4099.+floor(T*24.)*77777.);
  float sp=smoothstep(0.,7.,T);
  col+=step(1.-.35*S*sp,n)*S*vec3(n)*.9;
  col*=1.-.4*step(1.,fract(gl_FragCoord.y*.5))*S;
  col+=ck*.02*vec3(.7,.8,1.);
  col+=(n-.5)*.015;
  col+=smoothstep(56.,56.05,T)*(1.-smoothstep(56.05,57.,T))*.7;
  col*=(1.-smoothstep(58.,60.,T))*(1.-.6*dot(p,p)/(1.+.5*S));   // fade out, vignette
  col=mix(col,vec3(n*n*1.4),.9*(1.-smoothstep(.3,2.6,T)));   // television snow at the very start
  col=1.-exp(-col*1.7);
  o=vec4(pow(max(col,0.),vec3(.4545)),1);
}
