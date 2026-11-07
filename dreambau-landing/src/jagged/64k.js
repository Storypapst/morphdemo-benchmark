// Schlucht: a suspension bridge drawn against a valley in the blue hour.
production('64k', 820000, /*glsl*/`
const vec3 dusk=vec3(.15,.19,.37), copper=vec3(1.,.56,.29), lamp=vec3(1.,.86,.62);
float ridge(float x){
  return .51*abs(sin(x*4.4))+.23*abs(sin(x*9.6+1.))+ .14*abs(sin(x*21.+3.));
}
float cliff(float x){
  float canyon=exp(-10.24*x*x);
  return -.34-.36*canyon+.50*ridge(x+3.4)*(1.-canyon);
}
vec3 ranges(vec3 c,vec2 q,float drift,float dawn){
  for(int i=0;i<5;i+=1){
    float n=float(i),f=n/4.;
    float h=.36-f*.35+(.34+f*.1)*ridge(q.x*(1.1+f*.55)+n*2.3+drift*f);
    float fade=smoothstep(-.003,.003,h-q.y);
    vec3 near=mix(vec3(.18,.19,.34),vec3(.034,.046,.106),f);
    near+=vec3(.15,.07,.03)*dawn*exp(-abs(q.x-.39)*1.4)*(1.-f)*.8;
    c=mix(c,near,fade);
    c+=dusk*.33*ink(q.y-h,.0011)*(1.-f*.6);
  }
  return c;
}
float cable(float x,float deck){
  return deck+.07+(abs(x)<.49 ? .34*pow(abs(x)/.49,2.) : .34*(.90-abs(x))/.41);
}
void main(){
  vec2 p=pixel(),q=world(p);
  float into=phase(0.,2.5),lights=phase(CROSS-1.,9.),end=phase(43.,7.);
  float drift=.022*sin(T*.13),deck=.08+.25*end;
  vec3 c=mix(vec3(.025,.033,.079),vec3(.09,.093,.18),exp(-abs(q.y-.55)*2.2));
  vec2 sun=(q-vec2(.58,.55))*vec2(.92,1.);
  c+=copper*exp(-dot(sun,sun)*5.)*(.10+.14*lights);
  c+=lamp*ink(length(sun)-.097,.001)*.22*phase(4.,8.)*(1.-.64*end);
  c+=copper*step(length(sun),.096)*.105*phase(4.,8.);
  vec2 a=floor(q*63.),uv=fract(q*63.)-.5;
  float dotStar=h2(a);
  c+=vec3(.62,.68,1.)*exp(-dot(uv,uv)*31.)*step(.986,dotStar)*.53
     *step(.37,q.y)*(1.-.58*lights)*(.7+.3*sin(T*.8+dotStar*89.));
  c=ranges(c,q,drift,lights);

  float bank=cliff(q.x);
  float face=smoothstep(-.005,.004,bank-q.y);
  float stratum=fract((q.y-.18*ridge(q.x+2.3))*24.+vn(q*vec2(11.,4.))*.65);
  vec3 rock=vec3(.025,.036,.079)+dusk*(.14*stratum+.13*exp((q.y-bank)*9.));
  c=mix(c,rock,face);
  c+=dusk*.48*ink(q.y-bank,.002);
  float river=.086+.15*smoothstep(.55,1.,-q.y);
  float center=.058*sin(q.y*9.);
  float wet=(1.-smoothstep(river*.40,river,abs(q.x-center)))*step(q.y,-.54);
  c+=wet*vec3(.05,.16,.26)*(.22+.25*sin(q.y*137.+q.x*16.+min(T,60.)*2.));
  c+=wet*copper*lights*.20*pow(.5+.5*cos(q.y*95.-T*.5),6.);

  // Tall piers, then cables, then a moving light across the span.
  float tower=phase(BUILD,6.);
  float taper=.012-.008*clamp((q.y-deck+.3)*1.4,0.,1.);
  float mast=ink(abs(q.x)-.49,taper)*step(max(bank,-.49),q.y)*step(q.y,deck+.42*tower);
  float braces=ink(abs(q.x)-.49,.021)*ink(q.y-deck-.17,.005);
  c+=copper*tower*(mast*.65+braces*.43);
  float w=phase(BUILD+3.6,13.2)*1.09;
  float join=1.-smoothstep(w-.075,w,1.-abs(q.x)/.90);
  float bounds=step(abs(q.x),.90);
  float line=cable(q.x,deck);
  float sag=ink(q.y-line,.003)*step(deck,q.y);
  float thin=ink((fract(q.x*26.+.5)-.5)/26.,.0011)*step(deck,q.y)*step(q.y,line);
  float truss=ink(q.y-deck+.046*abs(fract(q.x*16.)*2.-1.)+.011,.0017);
  c+=bounds*join*phase(BUILD+3.6,2.)*copper*(.52*sag+.28*thin+.68*truss);
  c+=bounds*join*phase(BUILD+3.6,2.)*(lamp*.79*ink(q.y-deck,.006)+copper*.17*exp(-abs(q.y-deck)*32.));
  float abut=ink(abs(q.x)-.89,.013)*step(bank,q.y)*step(q.y,deck);
  float approach=ink(q.y-deck+.23*(abs(q.x)-.90),.005)*step(.90,abs(q.x))*(1.-smoothstep(.90,1.20,abs(q.x)));
  c+=copper*phase(BUILD+3.6,5.)*(abut*.43+approach*.58);
  float windows=pow(1.-abs(fract(q.x*39.)*2.-1.),6.)*ink(q.y-deck+.019,.004);
  c+=lights*bounds*lamp*windows*(.36+.19*E.x);
  float car=fract((T-CROSS)*.137)*1.67-.83;
  float shine=exp(-length((q-vec2(car,deck+.017))*vec2(.54,1.))*53.);
  c+=lamp*shine*lights*(1.-.66*end);
  c*=1.-.56*smoothstep(.61,1.52,length(q*vec2(1.,.83)));
  O=vec4(closeUp(c*into,p),1.);
}
`, K => {
  const hall = room(K, 3.1, 2.2, .20, .17);
  const winds = K.bus(.67, -.27, hall);
  const keys = K.bus(.80, .30, hall);
  const chords = [[38, 45, 50, 53], [34, 41, 46, 50], [41, 48, 53, 57], [36, 43, 48, 52]];
  for (let i = 8; i < 75; i++) {
    const t = i * beat, notes = chords[Math.floor(i / 8) % 4];
    if (i % 8 === 0) {
      for (let j = 0; j < notes.length; j++) K.p(j === 0 ? 'sine' : 'triangle', t + .035 * j, 2.55, K.hz(notes[j]), .091, .64, 1.6, winds, 700, 3);
      K.n(t, 1.56, 800, 260, .46, .062, hall, 'lowpass');
    }
    if (i % 2 && i > 16) K.p('sine', t, .22, K.hz(notes[i % 4] + 24), .103, .014, 1.04, keys);
    if (i >= 28 && i % 4 === 0) K.o('sine', t, .33, 125, 40, .205, K.out, .1);
    if (i >= 56 && i % 2 === 0) K.p('triangle', t + .15, .14, K.hz(notes[(i / 2) % 4] + 12), .105, .013, .47, hall, 1200);
  }
  signOff(K, hall, 50);
});
