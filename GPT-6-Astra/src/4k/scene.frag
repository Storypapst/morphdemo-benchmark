#version 130
uniform vec3 u;
mat2 R(float a){return mat2(cos(a),sin(a),-sin(a),cos(a));}
float tor(vec3 p){return length(vec2(length(p.xz)-1.,p.y))-.017;}
void main(){
 float t=u.x,order=smoothstep(6.,43.,t),beat=exp(-7.*fract(t*2.)),fade=smoothstep(0.,3.,t)*(1.-smoothstep(56.,60.,t));
 vec2 uv=(2.*gl_FragCoord.xy-u.yz)/u.z;
 vec3 col=vec3(.002,.006,.018),ro=vec3(0.,0.,4.5),rd=normalize(vec3(uv,-2.3));
 float ray=0.;
 for(int i=0;i<90;i++){
  vec3 p=ro+ray*rd;
  p.xz*=R(t*.12);p.yz*=R(.5+t*.09);
  vec3 q=p+(1.-order)*1.2*sin(p.zxy*3.+vec3(t*.9,t*.7,-t));
  q*=1.+.035*beat*order;
  float a=tor(q),b=tor(q.yxz)+1.-smoothstep(16.,28.,t),c=tor(q.xzy)+1.-smoothstep(28.,40.,t);
  float d=min(a,min(b,c));
  vec3 hue=a<b&&a<c?vec3(.03,.75,1.):vec3(1.,.31,.065);
  float e=exp(-70.*abs(d));
  col+=hue*(.005/(.012+abs(d))+.35*e)*.085;
  ray+=max(.016,abs(d)*.65);
  if(ray>9.)break;
 }
 float r=length(uv);
 float rings=abs(sin(r*42.-t*.7));
 col+=vec3(.015,.07,.09)*exp(-30.*rings)*smoothstep(.8,1.4,r)*order;
 col+=vec3(.04,.16,.20)*pow(max(0.,1.-r/2.),5.)*(.4+.6*beat)*order;
 float star=fract(sin(dot(floor(gl_FragCoord.xy/3.),vec2(12.9898,78.233)))*43758.5453);
 col+=step(.9994,star)*(.08+.1*sin(t+star*90.));
 vec2 z=(uv-vec2(-.59,-.73))*vec2(28.,-28.);
 int letter=int(floor(z.x/7.)),x=int(mod(floor(z.x),7.)),y=int(floor(z.y));
 int bits=letter==0?18732593:letter==1?15255086:letter==2?32045714:letter==3?32045584:18415153;
 if(letter>=0&&letter<5&&x<5&&y>=0&&y<5 && ((bits>>(24-x-y*5))&1)==1)
  col+=vec3(.6,.8,.85)*smoothstep(46.,50.,t);
 col=1.-exp(-col*2.2);
 col*=fade*(1.-.2*dot(uv,uv));
 gl_FragColor=vec4(pow(max(col,0.),vec3(.8)),1.);
}
