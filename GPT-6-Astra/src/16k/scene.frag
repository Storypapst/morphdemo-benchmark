#version 130
uniform float T;
uniform vec2 R;
#define PI 3.14159265
float sat(float x){return clamp(x,0.,1.);}
mat2 rot(float a){return mat2(cos(a),-sin(a),sin(a),cos(a));}
float hash(float n){return fract(sin(n*127.1)*43758.5453);}
float form,lockin;
vec2 scene(vec3 p){
 p.xz*=rot(.12*T+.1*sin(T*.3));
 float a=atan(p.z,p.x), sector=floor(a/(PI/4.)+.5);
 vec3 q=p;q.xz*=rot(-sector*PI/4.);
 q.x-=mix(2.7+hash(sector+9.)*1.7,1.18,form);
 q.y-=mix(sin(sector*17.+T*.28)*1.9,.15+sin(sector*PI*.5)*.3,form);
 q.xy*=rot((1.-form)*sin(sector*9.+T*.21));
 float wide=mix(.075,.30,form)*(1.+.08*sin(T*4.+sector));
 float shard=(abs(q.x)+abs(q.z)+abs(q.y)*.30-wide)*.61;
 float coregrow=smoothstep(19.,34.,T);
 vec3 c=p;c.xz*=rot(.18);
 float h=max(abs(c.x)*.866025+abs(c.z)*.5,abs(c.z));
 float core=max(h-.65*coregrow,(abs(c.y)+h*1.8-2.65*coregrow)*.5);
 float d=min(core,shard);
 float id=core<shard?1.:2.+hash(sector+32.);
 // A second crown unfolds into a precise, eightfold structure.
 q=p;q.xz*=rot(-(floor(a/(PI/4.))+.5)*PI/4.);
 q.x-=mix(4.,2.15,form);q.y-=mix(cos(sector*12.+T*.2)*2.,-.45,form);
 q.xy*=rot(mix(T*.15+sector,.52,form));
 float small=(abs(q.x)+abs(q.z)+abs(q.y)*.46-mix(.02,.23,form))*.63;
 if(small<d){d=small;id=3.5;}
 // Metallic armillary orbits become complete only in the final act.
 q=p;q.xy*=rot(.5);float ring=length(vec2(length(q.xz)-2.8,q.y))-.012*lockin;
 if(lockin>.01&&ring<d){d=ring;id=5.;}
 q=p;q.yz*=rot(-.8);ring=length(vec2(length(q.xz)-3.12,q.y))-.008*lockin;
 if(lockin>.01&&ring<d){d=ring;id=5.;}
 return vec2(d,id);
}
vec3 environment(vec3 d){
 float sky=pow(sat(d.y*.5+.5),2.);
 vec3 c=mix(vec3(.016,.021,.038),vec3(.07,.105,.15),sky);
 float strip=pow(sat(dot(d,normalize(vec3(-.8,.6,-.5)))),30.);
 float warm=pow(sat(dot(d,normalize(vec3(.7,.2,.2)))),60.);
 float ribbon=exp(-pow((d.x+.3)*40.,2.))*smoothstep(-.2,.15,d.y);
 float rim=exp(-pow((d.z-.55)*55.,2.))*smoothstep(-.1,.2,d.y);
 return c+strip*vec3(.85,.94,1.2)+warm*vec3(1.1,.48,.16)+ribbon*vec3(1.,1.25,1.7)+rim*vec3(1.4,.75,.28);
}
float glyph(vec2 uv,int code){
 vec2 g=floor(uv);if(g.x<0.||g.x>=5.||g.y<0.||g.y>=7.)return 0.;
 int row=int(g.y);int bits=0;
 // Embedded five by seven letterforms, generated as illuminated geometry.
 if(code==0){bits=row==0?17:row==1?27:row==2?21:17;}
 if(code==1){bits=(row==0||row==6)?14:17;}
 if(code==2){bits=row==0||row==3?30:row==1||row==2?17:row==4?20:row==5?18:17;}
 if(code==3){bits=row==0||row==3?30:row<3?17:16;}
 if(code==4){bits=row==3?31:17;}
 return float((bits>>(4-int(g.x)))&1);
}
float title(vec2 uv){
 uv.x+=14.5;float id=floor(uv.x/6.);return id<0.||id>4.?0.:glyph(vec2(mod(uv.x,6.),uv.y),int(id));
}
void main(){
 vec2 uv=(2.*gl_FragCoord.xy-R)/R.y;
 float fade=smoothstep(0.,2.5,T)*(1.-smoothstep(56.5,60.,T));
 form=smoothstep(9.,32.,T);lockin=smoothstep(32.,44.,T);
 float pulse=exp(-fract(T*2.)*7.)*smoothstep(14.,22.,T)*(1.-smoothstep(51.,56.,T));
 float angle=.22*sin(T*.11)+T*.025;
 float distance=mix(10.,7.7,smoothstep(6.,37.,T))+1.2*smoothstep(49.,58.,T);
 vec3 ro=vec3(sin(angle)*distance,1.1+sin(T*.075)*1.2,cos(angle)*distance);
 vec3 target=vec3(0.,.05,0.);vec3 ww=normalize(target-ro),uu=normalize(cross(ww,vec3(0,1,0))),vv=cross(uu,ww);
 vec3 rd=normalize(uu*uv.x+vv*uv.y+ww*1.8);
 vec3 col=vec3(.006,.011,.025);
 // An astronomical mist and two slow anamorphic rays hold the composition.
 float halo=exp(-length(uv*vec2(.9,1.1))*2.8);
 col+=halo*mix(vec3(.025,.03,.08),vec3(.06,.09,.14),form);
 col+=pow(sat(1.-abs(uv.y+.17)/.018),3.)*exp(-abs(uv.x)*.8)*vec3(.009,.017,.023);
 // Independent deterministic star field. It converges as the sculpture coheres.
 vec2 st=uv*55.;vec2 cell=floor(st);float rnd=hash(dot(cell,vec2(17.,113.)));
 float spark=pow(sat(1.-length(fract(st)-.5)*2.),12.)*step(.992,rnd);
 col+=spark*(.2+.15*sin(T+rnd*200.))*vec3(.7,.85,1.);
 float travel=0.;vec2 hit=vec2(0);float glow=0.;
 for(int i=0;i<112;i++){
  vec3 p=ro+rd*travel;hit=scene(p);
  glow+=.0018*exp(-abs(hit.x)*18.)*(1.+pulse*.8);
  if(hit.x<.0015||travel>20.)break;
  travel+=max(hit.x*.75,.001);
 }
 col+=glow*mix(vec3(.16,.32,.63),vec3(.45,.54,.72),lockin);
 if(travel<20.&&hit.x<.003){
  vec3 p=ro+rd*travel;vec2 e=vec2(.002,0.);
  vec3 n=normalize(vec3(scene(p+e.xyy).x-scene(p-e.xyy).x,scene(p+e.yxy).x-scene(p-e.yxy).x,scene(p+e.yyx).x-scene(p-e.yyx).x));
  vec3 l=normalize(vec3(-.6,1.,1.));float ndl=sat(dot(n,l));float fr=pow(1.-sat(dot(n,-rd)),3.);
  vec3 refl=environment(reflect(rd,n));
  vec3 pearl=.5+.5*cos(vec3(0.,2.,4.)+dot(n,-rd)*7.+p.y*.4+T*.05);
  vec3 base=mix(vec3(.09,.14,.18),vec3(.25,.34,.40),form);
  base=mix(base,pearl*.6,.20+.16*lockin);
  col=base*(.12+ndl*.75)+refl*(.8+fr*1.8);
  col+=environment(refract(rd,n,.66))*vec3(.025,.07,.1);
  col+=sat(length(fwidth(n))*2.)*vec3(.12,.17,.2)*form;
  col+=pow(sat(dot(reflect(-l,n),-rd)),100.)*vec3(2.,2.2,2.4);
  col+=pow(sat(dot(n,normalize(vec3(.8,-.1,-.5)))),8.)*vec3(.31,.17,.07);
  // Light travels up the facets on the same half-second pulse as the score.
  float scan=exp(-pow((p.y+3.)-mod(T*2.,8.),2.)*100.)*form;
  col+=scan*vec3(.2,.6,1.)*(.5+pulse);
  col+=fr*vec3(.17,.28,.43)*(.5+pulse);
  if(hit.y>4.5)col=vec3(.55,.31,.09)*(.6+pow(sat(dot(n,l)),2.)*2.)+pulse*.17;
  col=mix(col,vec3(.012,.025,.046),1.-exp(-travel*.018));
 }
 // Dust becomes a countable circular orbit: noise acquires a score.
 for(int j=0;j<32;j++){
  float fj=float(j),a=fj*2.39996+T*.15;
  vec3 p=vec3(cos(a),sin(fj*11.+T*.1)*.7,sin(a))*mix(3.8,2.45,form);
  p.y=mix(p.y,sin(a)*.6,lockin);
  vec3 rp=p-ro;float z=dot(rp,ww);vec2 screen=vec2(dot(rp,uu),dot(rp,vv))*1.8/z;
  float ds=length(uv-screen);float size=.000045/(ds*ds+.00007);
  col+=size*vec3(.1,.23,.35)*(.4+.6*hash(fj))*fade;
 }
 float vignette=sat(1.-dot(uv,uv)*.11);col*=vignette;
 col=vec3(1.)-exp(-col*1.6);col=pow(col,vec3(.4545));
 // Letterboxed cinematography and geometric typography, with no font asset.
 float bars=1.-smoothstep(.845,.85,abs(uv.y));col*=bars;
 float tit=smoothstep(1.,4.,T)*(1.-smoothstep(10.,13.,T))+smoothstep(51.,54.,T);
 vec2 textUV=vec2(uv.x, -uv.y-.57)*76.;
 float ink=title(textUV);
 col=mix(col,vec3(.76,.83,.86),ink*tit*fade);
 float line=step(abs(uv.y+.54),.0007)*step(abs(uv.x),.24);
 col+=line*vec3(.14,.2,.26)*tit;
 col+=(hash(dot(gl_FragCoord.xy,vec2(13.,53.))+floor(T*24.))-.5)*.006;
 gl_FragColor=vec4(col*fade,1.);
}
