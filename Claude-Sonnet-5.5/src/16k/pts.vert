#version 430
layout(location=0) uniform ivec3 u;
layout(location=0) out vec4 C;
float T=float(u.x)*.001;
vec2 RS=vec2(u.yz);
const float PI=3.14159265;
mat2 R(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
float RN(uint i,uint k){return F(i*0x9E3779B1u+k*0x85EBCA6Bu+0x1234567u);}
const vec4 W[46]=vec4[](vec4(-14,-3,-14,3),vec4(-14,3,-12,-0.4),vec4(-12,-0.4,-10,3),vec4(-10,3,-10,-3),vec4(-4,0,-4.1,0.93),vec4(-4.1,0.93,-4.38,1.76),vec4(-4.38,1.76,-4.82,2.43),vec4(-4.82,2.43,-5.38,2.85),vec4(-5.38,2.85,-6,3),vec4(-6,3,-6.62,2.85),vec4(-6.62,2.85,-7.18,2.43),vec4(-7.18,2.43,-7.62,1.76),vec4(-7.62,1.76,-7.9,0.93),vec4(-7.9,0.93,-8,0),vec4(-8,0,-7.9,-0.93),vec4(-7.9,-0.93,-7.62,-1.76),vec4(-7.62,-1.76,-7.18,-2.43),vec4(-7.18,-2.43,-6.62,-2.85),vec4(-6.62,-2.85,-6,-3),vec4(-6,-3,-5.38,-2.85),vec4(-5.38,-2.85,-4.82,-2.43),vec4(-4.82,-2.43,-4.38,-1.76),vec4(-4.38,-1.76,-4.1,-0.93),vec4(-4.1,-0.93,-4,0),vec4(-2,-3,-2,3),vec4(-2,3,0,3),vec4(0,0,-2,0),vec4(-0.4,0,2,-3),vec4(0,3,0.75,2.8),vec4(0.75,2.8,1.3,2.25),vec4(1.3,2.25,1.5,1.5),vec4(1.5,1.5,1.3,0.75),vec4(1.3,0.75,0.75,0.2),vec4(0.75,0.2,0,0),vec4(4,-3,4,3),vec4(4,3,6,3),vec4(6,0,4,0),vec4(6,3,6.75,2.8),vec4(6.75,2.8,7.3,2.25),vec4(7.3,2.25,7.5,1.5),vec4(7.5,1.5,7.3,0.75),vec4(7.3,0.75,6.75,0.2),vec4(6.75,0.2,6,0),vec4(10,-3,10,3),vec4(14,-3,14,3),vec4(10,0,14,0));
float MI;                               // brightness of the particle inside its shape: travelling waves make the shapes move
vec3 sh(int s,uint i){
  float a=RN(i,1u),b=RN(i,2u),c=RN(i,3u);
  MI=1.;
  vec3 p,n=vec3(RN(i,5u),RN(i,6u),RN(i,7u))-.5;
  if(s==0){
    float z=2.*a-1.,ph=2.*PI*b,r=40.*pow(c,.333);
    p=r*vec3(sqrt(1.-z*z)*vec2(cos(ph),sin(ph)),z);
  }else if(s==1){
    float th=2.*PI*a,m=floor(b*40.),ph=2.*PI*(m+.5)/40.+T*.5,r=1.6+.1*(c-.5);
    MI=.55+.9*pow(.5+.5*sin(th*3.-T*2.),2.);
    p=vec3((6.5+r*cos(ph))*cos(th),r*sin(ph)*1.5,(6.5+r*cos(ph))*sin(th));
  }else if(s==2){
    float lo=2.*PI*floor(a*56.)/56.,la=asin(2.*b-1.),r=8.+.1*(c-.5);
    MI=.5+1.1*pow(.5+.5*sin(la*5.-T*3.),3.);
    p=r*vec3(cos(la)*cos(lo),sin(la),cos(la)*sin(lo));
  }else if(s==3){
    float r=.5+12.*sqrt(a),th=floor(b*3.)*2.094+r*.5+(c-.5)*.3+(T-30.)*.5/(1.+r*.35);
    p=vec3(r*cos(th),n.y*1.6*exp(-r*.15),r*sin(th));
  }else if(s==4){
    float t=2.*PI*a;
    MI=.35+1.3*pow(.5+.5*sin(t*4.-T*6.),3.);
    p=2.*vec3((2.+cos(3.*t))*cos(2.*t),-sin(3.*t)*1.4,(2.+cos(3.*t))*sin(2.*t))+n*.6;
  }else{
    float tot=0.;
    for(int k=0;k<46;k++)tot+=length(W[k].zw-W[k].xy);
    float x=a*tot,acc=0.,l=0.;int k=0;
    for(;k<45;k++){l=length(W[k].zw-W[k].xy);if(acc+l>x)break;acc+=l;}
    l=length(W[k].zw-W[k].xy);
    p=vec3(mix(W[k].xy,W[k].zw,(x-acc)/l),0.)*.95+n*vec3(.1,.1,.4);
    MI=.8+.4*sin(x*.5-T*3.);
  }
  return p;
}
vec3 pc(int s){
  return s==0?vec3(.55,.65,1.):s==1?vec3(.15,.75,1.):s==2?vec3(1.,.25,.6):s==3?vec3(1.,.65,.25):s==4?vec3(.3,1.,.55):vec3(1.,.82,.55);
}
const uint NM=1048576u;                 // main particles; then 100k background stars, then 5 kick rings of 40k
void main(){
  uint i=uint(gl_VertexID);
  float O=ORD(T),K=KE(T),ck=CK(T);
  float w=smoothstep(43.,47.,T);
  vec3 p,col;float I,sz,kk=1.,FX=-1.;
  float asp=min(RS.y,RS.x/1.7778);
  if(i<NM){
    float a=RN(i,10u),d=RN(i,11u);
    float f=.06+.5*pow(smoothstep(0.,15.,T),2.)+.5*smoothstep(15.,22.,T);
    float vis=step(a,f);
    float x=(T-2.*d-15.)/7.5;
    int j=int(clamp(floor(x)+1.,0.,5.));
    if(T>=45.)j=5;                                              // at the drop everybody heads for the word
    float k=j==0?1.:smoothstep(0.,.4,x-float(j-1));
    vec3 pA=sh(max(j-1,0),i);float mA=MI;vec3 pB=sh(j,i);float mB=MI;
    float tau=T-45.-clamp(floor((pB.x+15.)/6.),0.,4.)*.46875;   // the letters arrive one after the other, one per beat
    if(j==5)k=smoothstep(0.,.55,tau);
    kk=j==0?0.:j==1?k:1.;
    p=mix(pA,pB,k)+(vec3(RN(i,60u),RN(i,61u),RN(i,62u))-.5)*10.*sin(k*3.14159)*(.3+.7*RN(i,63u));   // in flight the particles spray out sideways
    col=mix(pc(max(j-1,0)),pc(j),k);
    col=mix(col,vec3(1),.25*RN(i,31u));
    p.xz*=R((1.-k)*k*(a-.5)*10.);
    float dis=1.-O;
    vec3 nz=sin(T*(.5+vec3(RN(i,20u),RN(i,21u),RN(i,22u))*2.)+vec3(RN(i,23u),RN(i,24u),RN(i,25u))*6.283);
    p+=nz*(dis*dis*dis*5.+.02);
    p*=1.+.05*K;
    vec3 cc=(vec3(F(floor(T*24.)+778.),F(floor(T*24.)+779.),F(floor(T*24.)+780.))-.5)*24.;
    float sp=mix(1.4*pow(RN(i,30u),2.)+.12,.018+.01*RN(i,30u),kk);
    I=sp*(1.+1.2*K)*vis*smoothstep(0.,1.5,T)*mix(1.,mix(mA,mB,k),kk);
    if(j==5)I*=1.+2.5*exp(-max(tau,0.)*4.)*step(0.,tau);
    I*=1.+ck*4.*exp(-dot(p-cc,p-cc)*.02)*(1.-kk);
    I*=mix(1.,.5+F(i+uint(T*8.)),dis*.7);
    sz=.03*mix(1.,.7,kk)*(1.+K*.4);
  }else if(i<NM+100000u){
    // background: a random shell of stars that, one by one, snaps onto a regular lattice while the word is standing
    uint q=i-NM;
    vec3 v=vec3(RN(q,40u),RN(q,41u),RN(q,42u))*2.-1.;
    vec3 ps=normalize(v+1e-3)*(120.+80.*RN(q,43u));
    vec3 pl=(vec3(float(q%46u),float((q/46u)%46u),float(q/2116u))-vec3(22.5,22.5,23.))*9.;
    pl.z+=sign(pl.z+.001)*7.;
    float lt=clamp((T-33.-RN(q,44u)*10.)/6.,0.,1.);
    p=pl+(ps-pl)*pow(1.-lt,3.);                                       // fast at first, then it settles into its lattice site
    float rq=length(pl)-KP(T)*70.;
    col=mix(vec3(.6,.7,1.)*(.6+.4*RN(q,44u)),vec3(1.,.8,.55),lt*.7);
    FX=mix(.2*(.4+.6*F(q+uint(T*3.)))*step(RN(q,45u),.3+.7*lt),.12+1.5*exp(-rq*rq*.004)*exp(-KP(T)*1.5),lt*w)*smoothstep(1.,6.,T);
    sz=.1;
  }else{
    uint q=i-NM-100000u;
    uint rg=q/40000u;
    float kb=floor(T/.46875)+1.-float(rg);
    float age=T-KT(kb);
    float on=step(0.,age)*KV(kb);
    float th=6.283*RN(q,50u);
    float r=2.+age*(9.+8.*(1.-w));
    vec3 hp=vec3(r*cos(th),(RN(q,51u)-.5)*.15,r*sin(th));
    vec3 vp=vec3(r*cos(th),r*sin(th),(RN(q,51u)-.5)*.15);
    p=mix(hp,vp,w);
    col=mix(vec3(.3,.7,1.),vec3(1.,.8,.5),w);
    I=on*exp(-age*1.6)*.3*mix(1.,.45,w);
    sz=.03;
  }
  float bu=exp(-max(T-56.25,0.)*5.)*step(56.25,T);
  p*=1.+.18*bu*step(i,NM);
  // camera: a different viewpoint for every shape, frontal on the word
  float e1=smoothstep(15.,22.5,T),e2=smoothstep(22.5,30.,T),e3=smoothstep(30.,37.5,T),e4=smoothstep(37.5,45.,T);
  float el=.3+.5*e1-.6*e2+.95*e3-.8*e4;
  float dz=mix(mix(3.,24.,smoothstep(6.,24.,T)),22.,e1*(1.-e2))+5.*e2*(1.-e3)+9.*e3*(1.-e4)-7.*e4;
  float ya=T*.07+.5*sin(T*.13)+.5*e1;
  p.xz*=R(mix(ya,.2*sin((min(T,56.25)-47.)*.34),w));   // frontal at 47 s, a slow parallax sweep, frontal again for the final chord
  p.yz*=R(mix(el+.05*sin(T*.4),.05*sin((T-47.)*.4),w));
  p.xy*=R(mix(.04*sin(T*.5),0.,w));
  p.xy+=(vec2(F(floor(T*30.)),F(floor(T*30.)+9.))-.5)*.35*K*(1.-w);
  p.z+=mix(dz,24.+2.*smoothstep(47.,58.,T),w);
  gl_Position=vec4(p.xy*1.2*asp/(.5*RS),0,p.z);
  float coc=abs(p.z-mix(dz,24.+2.*smoothstep(47.,58.,T),w))*.008;
  float sz2=sz+coc;
  gl_PointSize=clamp(sz2*1.2*asp/p.z,1.5,60.);
  float fl=max(sz2*sz2/.0009,.3);
  C=vec4(col*(FX>=0.?FX:I/fl),1);
}
