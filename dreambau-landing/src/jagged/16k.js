// Tragwerk: an axonometric model on a survey table. Each bay arrives on the beat and locks into a shared deck.
production('16k', 650000, /*glsl*/`
const vec3 blue=vec3(.37,.73,1.), brass=vec3(1.,.63,.32), chalk=vec3(.88,.94,1.);
float box(vec2 p,vec2 h){vec2 d=abs(p)-h;return length(max(d,0.))+min(max(d.x,d.y),0.);}
float shape(float d){return 1.-smoothstep(-.003,.003,d);}
float land(vec2 q){
  vec2 a=abs(q);
  return a.x-.68+abs(q.y+.14)*.25;
}
vec3 bay(vec3 c,vec2 p,float x,float deck,float appear,float ping,float noise){
  vec2 u=p-vec2(x,deck+x*.115+(1.-appear)*(.40+.14*noise));
  vec2 v=vec2(u.x+u.y*.55,u.y-u.x*.115);
  float deckTop=shape(box(v,vec2(.084,.039)));
  float deckFace=shape(box(v+vec2(.008,.042),vec2(.084,.022)));
  float leg=ink(seg(u,vec2(-.06,-.043),vec2(-.075,-.37)),.005);
  leg+=ink(seg(u,vec2(.063,-.043),vec2(.049,-.37)),.005);
  float brace=ink(seg(u,vec2(-.064,-.057),vec2(.052,-.30)),.003);
  brace+=ink(seg(u,vec2(.056,-.06),vec2(-.071,-.31)),.003);
  float base=shape(box(v+vec2(.15,.391),vec2(.093,.018)));
  c+=blue*(leg*.42+brace*.28+base*.22)*appear;
  c=mix(c,brass*(.35+.10*noise),deckFace*appear);
  c=mix(c,mix(blue,brass,.64)*(.54+.22*noise)+chalk*ping*.13,deckTop*appear);
  c+=chalk*ink(v.y-.038,.0015)*step(abs(v.x),.084)*appear*(.37+.15*ping);
  return c;
}
void main(){
  vec2 p=pixel(),q=world(p);
  float lift=phase(BUILD-4.,8.),deck=mix(.005,.39,phase(44.,6.));
  vec3 c=vec3(.026,.041,.081);
  vec2 grid=vec2(q.x-q.y*.63,q.y+q.x*.43);
  vec2 g=abs(fract(grid*11.+.5)-.5);
  c+=blue*ink(min(g.x,g.y)/11.,0.)*.11*exp(-dot(q,q)*.72);
  // Near and far banks are read as terraced slabs, below the connecting deck.
  vec2 t=vec2(q.x-q.y*.36,q.y+q.x*.15);
  float terraces=land(t);
  float footing=ink(terraces,.004)+ink(terraces-.061,.002)+ink(terraces-.122,.002)*.6;
  float fill=step(0.,terraces)*step(t.y,.44);
  c+=blue*(fill*.075+footing*.19)*(1.-smoothstep(.3,.52,t.y));
  float contour=.19+.16*sin(q.x*15.)+.13*cos(q.x*27.)+.07*sin(q.x*54.)-.51*exp(-q.x*q.x*9.);
  c+=blue*(ink(q.y-contour,.003)*.7+.14*exp(-abs(q.y-contour)*19.))*phase(2.,9.)*(1.-lift*.8);

  for(int i=0;i<9;i+=1){
    float n=float(i),x=(n-4.)*.174;
    float time=BUILD+1.80*n;
    float a=phase(time,3.4);
    float k=exp(-abs(T-time-2.9)*4.)*.5+E.y*.3;
    c=bay(c,q,x,deck,a,k,h1(n+12.));
    vec2 pos=vec2(x,deck+x*.115+(1.-a)*.5);
    float guide=ink(seg(q,pos+vec2(0,.06),pos+vec2(.05,.40)),.0018);
    c+=blue*guide*(1.-a)*phase(time-.7,.7)*.33;
  }
  float highway=phase(CROSS,6.)*step(abs(q.x),.745);
  float u=fract(q.x*4.6-T*.37);
  float trace=ink(q.y-deck-q.x*.115-.017,.003);
  c+=highway*brass*trace*(.37+.63*pow(u,11.));
  c+=brass*exp(-abs(q.y-deck-q.x*.115)*27.)*.14*highway;
  c*=1.-.44*smoothstep(.48,1.42,length(q));
  O=vec4(closeUp(c*phase(0.,2.6),p),1.);
}
`, K => {
  const send = room(K, 2.4, 2.2, .2, .23);
  const left = K.bus(.74, -.38, send), right = K.bus(.74, .38, send);
  const roots = [42, 45, 49, 40];
  for (let i = 7; i < 74; i++) {
    const root = roots[Math.floor(i / 8) % roots.length], t = i * beat;
    const f = K.hz(root + [12, 19, 24, 28][(i * 3) % 4]);
    K.p('sine', t + (i % 2) * .065, .14, f, .107, .007, .52, i % 2 ? left : right);
    if (i > 28 && i % 4 === 0) {
      K.p('triangle', t, 1.15, K.hz(root), .14, .055, .57, send, 630);
      K.o('sine', t, .26, 104, 38, .22, K.out, .086);
    }
    if (i >= 28 && i <= 52 && (i - 28) % 3 === 0) K.n(t, .11, 3100, 420, 2.7, .21, i % 2 ? right : left);
    if (i > 55 && i % 4 === 2) K.p('sine', t, .21, f * 2, .12, .012, .73, send);
  }
  signOff(K, send, 49);
});
