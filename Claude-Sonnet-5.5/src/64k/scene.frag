#version 430
layout(location=0) uniform ivec3 u;
out vec4 o;
float T=float(u.x)*.001;
vec2 RS=vec2(u.yz);
const float PI=3.14159265;

mat2 R(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
float h21(vec2 p){p=fract(p*vec2(123.34,456.21));p+=dot(p,p+45.32);return fract(p.x*p.y);}
float vn(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(h21(i),h21(i+vec2(1,0)),f.x),mix(h21(i+vec2(0,1)),h21(i+vec2(1,1)),f.x),f.y);}
float fbm(vec2 p,int n){float s=0.,a=.5;for(int i=0;i<n;i++){s+=a*vn(p);p=mat2(1.6,1.2,-1.2,1.6)*p;a*=.5;}return s;}

// ---------------------------------------------------------------- timeline
float zc(float t){                       // camera distance along the flight path
  float a=8.*t;
  float b=96.+6.*(t-12.);
  float c=180.+10.*(t-26.);
  float d=340.+44.*(1.-pow(1.-clamp((t-42.)/14.,0.,1.),2.));
  float z=mix(a,b,smoothstep(10.,14.,t));
  z=mix(z,c,smoothstep(24.,28.,t));
  z=mix(z,d,smoothstep(40.,44.,t));
  return z;
}
const float ZEND=430.;
// ---------------------------------------------------------------- hex tiling (Shane)
const vec2 HS=vec2(1.7320508,1.);
vec4 hexc(vec2 p){
  vec4 hC=floor(vec4(p,p-vec2(1,.5))/HS.xyxy)+.5;
  vec4 h=vec4(p-hC.xy*HS,p-(hC.zw+.5)*HS);
  return dot(h.xy,h.xy)<dot(h.zw,h.zw)?vec4(h.xy,hC.xy):vec4(h.zw,hC.zw+.5);
}
float hexd(vec2 p){p=abs(p);return max(dot(p,HS*.5),p.y);}
const float CS=.62;
float tA,tB,FR,KPT;
vec3 CAM;
const float LT=45.,BEAT=.625;      // the word is built letter by letter on the beats of the drop
float front(vec2 xz){float r=length(xz-vec2(0,ZEND));return smoothstep(FR+30.,FR-30.,r);}
float px(float z){return sin(z*.008)*40.*(1.-smoothstep(300.,420.,z));}   // the flight path winds through a valley
float valley(vec2 xz){return smoothstep(6.,70.,abs(xz.x-px(xz.y)));}
float h31(vec3 p){p=fract(p*vec3(.1031,.11369,.13787));p+=dot(p,p.yzx+19.19);return fract((p.x+p.y)*p.z);}
float vn3(vec3 p){vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
  return mix(mix(mix(h31(i),h31(i+vec3(1,0,0)),f.x),mix(h31(i+vec3(0,1,0)),h31(i+vec3(1,1,0)),f.x),f.y),
             mix(mix(h31(i+vec3(0,0,1)),h31(i+vec3(1,0,1)),f.x),mix(h31(i+vec3(0,1,1)),h31(i+vec3(1,1,1)),f.x),f.y),f.z);}
const float CL0=100.,CL1=150.;
float cloudD(vec3 p,float amt){
  float h=(p.y-CL0)/(CL1-CL0),prof=4.*h*(1.-h);
  vec3 q=p*.012+vec3(T*.03,0,T*.012);
  float n=.5*vn3(q)+.25*vn3(q*2.13)+.125*vn3(q*4.37)+.0625*vn3(q*8.9);
  return clamp((n*prof*2.1-.5)*4.,0.,1.)*amt;
}
float ridge(vec2 p,int n){float s=0.,a=.5;for(int i=0;i<n;i++){float r=1.-abs(2.*vn(p)-1.);s+=a*r*r;p=mat2(1.6,1.2,-1.2,1.6)*p;a*=.5;}return s;}
float rough(vec2 xz){float v=valley(xz);float wob=(fbm(xz*.06+vec2(T*.15,0.),3)-.5)*5.*STA(T);return ((ridge(xz*.010,6)*95.-8.)*v+fbm(xz*.11,3)*3.*(.3+.7*v)+1.+wob)*tA;}
float coltop(vec2 c){float v=valley(c);float h=(ridge(c*.010,3)*95.-8.)*v+1.;return floor(h*tB/4.+.5)*4.;}
const vec4 W[46]=vec4[](vec4(-14,-3,-14,3),vec4(-14,3,-12,-0.4),vec4(-12,-0.4,-10,3),vec4(-10,3,-10,-3),vec4(-4,0,-4.1,0.93),vec4(-4.1,0.93,-4.38,1.76),vec4(-4.38,1.76,-4.82,2.43),vec4(-4.82,2.43,-5.38,2.85),vec4(-5.38,2.85,-6,3),vec4(-6,3,-6.62,2.85),vec4(-6.62,2.85,-7.18,2.43),vec4(-7.18,2.43,-7.62,1.76),vec4(-7.62,1.76,-7.9,0.93),vec4(-7.9,0.93,-8,0),vec4(-8,0,-7.9,-0.93),vec4(-7.9,-0.93,-7.62,-1.76),vec4(-7.62,-1.76,-7.18,-2.43),vec4(-7.18,-2.43,-6.62,-2.85),vec4(-6.62,-2.85,-6,-3),vec4(-6,-3,-5.38,-2.85),vec4(-5.38,-2.85,-4.82,-2.43),vec4(-4.82,-2.43,-4.38,-1.76),vec4(-4.38,-1.76,-4.1,-0.93),vec4(-4.1,-0.93,-4,0),vec4(-2,-3,-2,3),vec4(-2,3,0,3),vec4(0,0,-2,0),vec4(-0.4,0,2,-3),vec4(0,3,0.75,2.8),vec4(0.75,2.8,1.3,2.25),vec4(1.3,2.25,1.5,1.5),vec4(1.5,1.5,1.3,0.75),vec4(1.3,0.75,0.75,0.2),vec4(0.75,0.2,0,0),vec4(4,-3,4,3),vec4(4,3,6,3),vec4(6,0,4,0),vec4(6,3,6.75,2.8),vec4(6.75,2.8,7.3,2.25),vec4(7.3,2.25,7.5,1.5),vec4(7.5,1.5,7.3,0.75),vec4(7.3,0.75,6.75,0.2),vec4(6.75,0.2,6,0),vec4(10,-3,10,3),vec4(14,-3,14,3),vec4(10,0,14,0));
float sdSeg(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;return length(pa-ba*clamp(dot(pa,ba)/dot(ba,ba),0.,1.));}
const vec2 WC=vec2(0.,ZEND-24.);
const float WSC=1.9;
float wordh(vec2 c){
  vec2 g=vec2(-(c.x-WC.x),(c.y-WC.y)/1.7)/WSC;
  if(abs(g.x)>15.5||abs(g.y)>4.)return 0.;
  float d=1e9;for(int k=0;k<46;k++)d=min(d,sdSeg(g,W[k].xy,W[k].zw));
  float ta=T-LT-clamp(floor((g.x+15.)/6.),0.,4.)*BEAT;
  ta=max(ta,0.);
  return step(d,.6)*step(.0001,ta)*(1.-exp(-ta*8.)*cos(ta*12.))*5.;
}
float rip(vec2 c){float r=length(c-CAM.xz)-KPT*38.;return .9*exp(-r*r*.02)*exp(-KPT*2.5);}   // a ripple leaves the camera with every kick
vec2 LC(float i){return vec2(WC.x+(12.-6.*i)*WSC,WC.y);}                                        // centre of letter i on the ground
float rings(vec2 p){                                                                             // shock rings of the five letters
  float s=0.;
  for(int i=0;i<5;i++){float ta=T-LT-float(i)*BEAT,q=length(p-LC(float(i)))-ta*30.;s+=step(0.,ta)*exp(-q*q*.03)*exp(-max(ta,0.)*2.2);}
  return s;
}
float scene(vec3 p,out float oc){
  oc=front(p.xz);
  float dr=(p.y-rough(p.xz))*.55;
  if(oc<.01)return dr;
  vec4 hc=hexc(p.xz/CS);
  vec2 ctr=hc.zw*HS*CS;
  float top=coltop(ctr)+wordh(ctr)+rip(ctr)*oc;
  float dc=max(hexd(hc.xy)*CS-.5*CS*.93,p.y-top);
  dc=min(dc,p.y+80.);
  return mix(dr,dc,oc);
}
// ---------------------------------------------------------------- sky
vec3 SUN,SUNC;float FLASH;
vec3 sky(vec3 rd){
  float s=max(dot(rd,SUN),0.);
  float hz=pow(1.-max(rd.y,0.),4.);
  float dawn=smoothstep(36.,58.,T);
  vec3 zen=mix(vec3(.004,.007,.028),vec3(.09,.16,.38),dawn);
  vec3 hor=mix(vec3(.02,.035,.09),vec3(.85,.4,.2),dawn);
  vec3 c=mix(zen,hor,hz);
  // clouds
  vec2 cp=rd.xz/(rd.y+.15)*.9+vec2(T*.01,0);
  float cl=smoothstep(.45,.8,fbm(cp*1.3,5))*smoothstep(.0,.25,rd.y);
  c=mix(c,mix(vec3(.10,.12,.2),vec3(1.,.7,.5),dawn)*(.25+.75*pow(s,2.))+FLASH*vec3(.6,.7,1.),cl*.7);
  c+=SUNC*(pow(s,600.)*8.+pow(s,14.)*.25);
  c+=FLASH*vec3(.4,.5,.9)*(.1+hz);
  vec3 q=rd*180.,id=floor(q);                         // stars: a soft point of a few pixels in a random spot inside the cell
  vec3 sc=id+.5+(vec3(h31(id+3.),h31(id+7.),h31(id+11.))-.5)*.5;
  float st=step(.996,h31(id))*smoothstep(.24,0.,length(cross(rd,normalize(sc)))*180.)*(.4+.6*h31(id+5.));
  c+=st*vec3(.8,.9,1.)*3.*max(rd.y,0.)*(1.-dawn)*(1.-cl);
  return c;
}
float shadow(vec3 p,vec3 l){
  float res=1.,t=.2,o1;
  for(int i=0;i<30;i++){
    float h=scene(p+l*t,o1);
    res=min(res,10.*h/t);
    t+=clamp(h,.06,5.);
    if(res<.02||t>34.)break;
  }
  return clamp(res,0.,1.);
}
float aoc(vec3 p,vec3 n){
  float a=0.,o1;
  for(int i=1;i<5;i++){float h=float(i)*.35;a+=(h-scene(p+n*h,o1))/float(i+1);}
  return clamp(1.-a*.8,0.,1.);
}
// what a mirror-like floor sees in the finale: the glowing letters, the sky and the sun (a second, short march)
vec3 refl(vec3 p,vec3 d){
  float t=.7,o1,e=0.;
  for(int i=0;i<64;i++){e=scene(p+d*t,o1);if(e<.004*t||t>150.)break;t+=e;}
  if(t>150.)return vec3(0);
  vec3 q=p+d*t;
  vec4 h=hexc(q.xz/CS);vec2 c=h.zw*HS*CS;
  float w=wordh(c),L=step(.01,w)*step(coltop(c)+w-.15,q.y)*step(hexd(h.xy)*CS,.5*CS*.92);
  return vec3(1.,.55,.2)*L*3.5;
}
// lightning bolt in screen space: jagged main channel from the flash position up to the top of the picture plus one fork
float bolt(vec2 q,float x0,float y1,float s){
  float dx=0.,a=.13;
  for(int i=0;i<4;i++){dx+=(vn(vec2(q.y*(2.+float(i*i)*3.),s*3.7+float(i)*17.))-.5)*2.*a;a*=.5;}
  float d=abs(q.x-x0-dx);
  float m=smoothstep(y1-.02,y1+.02,q.y)*smoothstep(.75,.5,q.y);
  float yb=mix(y1,.5,.45),d2=abs(q.x-x0-dx-(q.y-yb)*(F(s+33.)-.5)*1.4);
  float m2=step(q.y,yb)*smoothstep(y1+.08,yb,q.y);
  return m*(exp(-d*d*1.5e5)+.35*exp(-d*d*3000.)+.7*m2*exp(-d2*d2*1.5e5));
}
void main(){
  float asp=min(RS.y,RS.x/1.7778);
  vec2 uv=(gl_FragCoord.xy-.5*RS)/asp;
  vec2 uv0=uv;
  float shk=STA(T)*.010+TH(T)*.03+LG(T)*.008;                     // the storm shakes the camera, and the shake dies away with the order
  uv=R((vn(vec2(T*3.1,7.3))-.5)*6.*shk)*uv+(vec2(vn(vec2(T*2.7,3.1)),vn(vec2(T*3.3,9.1)))-.5)*2.*shk;
  float K=KE(T);KPT=KP(T);
  tA=mix(1.,.12,smoothstep(20.,56.,T));
  tB=mix(1.,.0,smoothstep(38.,56.,T));
  FR=mix(-40.,560.,smoothstep(9.,50.,T));
  FLASH=LG(T)*.6+CK(T)*.4*(1.-smoothstep(12.,20.,T));
  float fin=smoothstep(39.,45.,T);
  float zz=zc(T);
  float ay=mix(178.,158.,smoothstep(0.,9.,T));                 // drift above the storm, then plunge through the cloud deck at 9..12.5 s and settle into the valley
  ay=mix(ay,96.,smoothstep(9.,12.5,T));ay=mix(ay,14.,smoothstep(12.5,27.,T));ay=mix(ay,9.,smoothstep(30.,44.,T));
  vec3 pa=vec3(px(zz)+sin(T*.11)*8.,ay+3.*sin(T*.5)-1.5*K*smoothstep(20.,30.,T)+26.*smoothstep(33.,37.5,T)*(1.-smoothstep(38.5,42.,T)),zz);
  float wz=mix(66.,50.,smoothstep(46.,59.,T));
  vec3 pb=vec3(sin(T*.12)*3.,mix(14.,40.,smoothstep(44.,52.,T)),WC.y-wz);
  CAM=mix(pa,pb,fin);
  vec3 ta=mix(vec3(px(zz+100.)+sin(T*.13)*6.,CAM.y-mix(24.,9.,smoothstep(20.,40.,T)),CAM.z+100.),vec3(0.,0.,WC.y+2.),fin);
  vec3 f=normalize(ta-CAM),r=normalize(cross(f,vec3(0,1,0)));
  r.y+=(.12*sin(T*.17)+.35*(px(zz+30.)-px(zz))*.06*smoothstep(24.,34.,T))*(1.-fin);r=normalize(r);
  vec3 up=cross(r,f);
  vec3 rd=normalize(f*(1.3-.06*K)+r*uv.x+up*uv.y);
  SUN=normalize(vec3(.35,mix(.55,.07,smoothstep(28.,58.,T)),1.));
  SUNC=mix(vec3(.35,.45,1.),vec3(1.,.58,.28),smoothstep(30.,52.,T));
  float t=0.,oc=0.;
  float d;
  for(int i=0;i<300;i++){
    d=scene(CAM+rd*t,oc);
    if(d<.001*t||t>900.)break;
    t+=d*.9;
  }
  vec3 col;
  vec3 skc=sky(rd);
  float dawn=smoothstep(36.,58.,T);
  if(t<900.){
    vec3 p=CAM+rd*t;
    float o1;
    vec2 e=vec2(.008+.0008*t,0);
    vec3 gr=vec3(scene(p+e.xyy,o1)-scene(p-e.xyy,o1),scene(p+e.yxy,o1)-scene(p-e.yxy,o1),scene(p+e.yyx,o1)-scene(p-e.yyx,o1));
    vec3 n=dot(gr,gr)>1e-14?normalize(gr):vec3(0,1,0);
    vec4 hc=hexc(p.xz/CS);
    vec2 ctr=hc.zw*HS*CS;
    float ct=coltop(ctr),wh=wordh(ctr);
    float df=max(dot(n,SUN),0.);
    float sh=t<300.?shadow(p+n*.02*(1.+t*.01),SUN):1.;
    float ao=t<250.?aoc(p,n):1.;
    float fre=pow(1.-max(dot(n,-rd),0.),4.);
    float tint=h21(hc.zw);
    vec3 base=mix(vec3(.16,.17,.2),vec3(.02,.025,.04)*(.7+.6*tint),oc);
    vec3 rf=reflect(rd,n);
    col=base*(.12*ao+df*sh*SUNC*2.2)+sky(rf)*(.015+.22*fre)*oc*ao;
    col+=SUNC*pow(max(dot(rf,SUN),0.),160.)*sh*(.3+.5*oc)*oc;
    float gap=smoothstep(.34,.5,hexd(hc.xy))*oc*step(p.y,ct+wh+.1);
    vec3 seam=mix(vec3(.12,.45,1.),vec3(1.,.5,.15),dawn);
    float rq=length(p.xz-CAM.xz)-KPT*38.;
    float ring=exp(-rq*rq*.02)*exp(-KPT*2.5);
    float wave=.5+.5*sin(length(p.xz-CAM.xz)*.35-T*4.);
    float drop=rings(p.xz);
    float lod=1.-smoothstep(45.,190.,t);
    float gs=mix(.27*step(p.y,ct+wh+.1),gap,lod);          // far away the seams average out to a steady glow
    col+=seam*gs*(.5+.5*wave+1.9*K+4.*ring+6.*drop)*(.5+.7*(1.-tA))*ao*(1.-.75*dawn*(1.-.6*min(drop,1.)))*oc;
    float lit=step(.01,wh)*step(ct+wh-.15,p.y)*step(hexd(hc.xy)*CS,.5*CS*.92);
    col+=vec3(1.,.55,.2)*lit*(1.1+.6*n.y+K)*3.;
    if(T>42.&&oc>.5&&lit<.5&&n.y>.6&&abs(p.x)<40.&&abs(p.z-WC.y)<30.)col+=refl(p+n*.03,rf)*(.15+.85*pow(1.-max(dot(n,-rd),0.),3.))*.55*ao*smoothstep(42.,46.,T);
    float rr=length(p.xz-vec2(0,ZEND));
    float fr_=exp(-abs(rr-FR)*.18)*smoothstep(8.,12.,T)*(1.-smoothstep(44.,50.,T));
    col+=vec3(.2,.75,1.)*fr_*(3.+2.*K)*(.6+.4*h21(floor(p.xz*2.)+floor(T*8.)));
    col+=vec3(.5,.6,1.)*FLASH*.8*(1.-oc)*(.2+df);
  }else col=skc;
  vec3 BL=vec3(0);
  if(LG(T)>.01){
    float sl=floor(T*2.);
    vec3 lp0=vec3(sin(sl*7.)*90.,110.,CAM.z+30.+cos(sl*3.)*80.),v=lp0-CAM;
    float zf=dot(v,f),bx=zf>1.?1.3*dot(v,r)/zf:(F(sl+31.)-.5)*1.3,by=zf>1.?1.3*dot(v,up)/zf:-.1;
    BL=vec3(.75,.85,1.)*bolt(uv0,clamp(bx,-.8,.8),clamp(by,-.45,.2),sl)*LG(T)*9.;
  }
  float camt=(1.-smoothstep(16.,36.,T))*.9+.12*smoothstep(36.,58.,T);
  if(camt>.01){
    float ta=0.,tb=min(t,900.);
    float ya=(CL0-CAM.y)/(abs(rd.y)<1e-4?1e-4:rd.y),yb=(CL1-CAM.y)/(abs(rd.y)<1e-4?1e-4:rd.y);
    float e0=min(ya,yb),e1=max(ya,yb);
    bool inside=CAM.y>CL0&&CAM.y<CL1;
    ta=inside?0.:max(e0,0.);
    tb=min(tb,inside?max(e1,0.):e1);
    if(tb>ta){
      float dt=(tb-ta)/40.;
      float jit=F(gl_FragCoord.x*17.+gl_FragCoord.y*4099.+floor(T*30.));
      vec3 acc=vec3(0);float tr=1.;
      vec3 lp=vec3(sin(floor(T*2.)*7.)*90.,110.,CAM.z+30.+cos(floor(T*2.)*3.)*80.);
      for(int i=0;i<40;i++){
        float tt=ta+(float(i)+jit)*dt;
        vec3 p=CAM+rd*tt;
        float d=cloudD(p,camt);
        if(d>.01){
          float l=cloudD(p+SUN*14.,camt);
          vec3 lit=SUNC*exp(-l*2.6)*.2+vec3(.008,.012,.03);
          float dl=length(p-lp);
          lit+=vec3(.7,.8,1.)*FLASH*exp(-dl*dl*.00012)*4.;
          lit+=vec3(.5,.6,1.)*FLASH*.2;
          float a=1.-exp(-d*dt*.07);
          acc+=tr*lit*a*(.5+.5*d);
          tr*=1.-a;
          if(tr<.02)break;
        }
      }
      col=(col+BL)*tr+acc;
    }else col+=BL;
  }else col+=BL;
  float hf=exp(-max(CAM.y+rd.y*t*.5,0.)*.012);
  float fog=1.-exp(-t*.0019*(.35+.65*hf));
  vec3 fc=skc*.5+vec3(.015,.025,.05)+SUNC*pow(max(dot(rd,SUN),0.),8.)*.2*(.3+dawn);
  col=mix(col,fc,fog);
  if(!(col.x==col.x&&col.y==col.y&&col.z==col.z))col=vec3(0);
  o=vec4(min(col,vec3(60.)),1);
}
