// Kante: a single irregular trace becomes the section drawing for a crossing.
production('4k', 510000, /*glsl*/`
float height(float x){
  return -.02+.23*sin(x*9.)+.075*sin(x*32.)+.065*sin(x*72.)-.63*exp(-16.*x*x);
}
void main(){
  vec2 p=pixel(),q=world(p);
  float lit=phase(0.,2.),draw=phase(1.,9.),b=phase(BUILD,16.8),run=phase(CROSS,5.),end=phase(44.,6.);
  vec3 cold=vec3(.42,.49,.91),gold=vec3(1.,.62,.33);
  vec3 c=vec3(.022,.027,.066)+vec3(.03,.035,.075)*exp(-dot(q,q)*1.4);
  vec2 g=abs(fract(q*10.+.5)-.5);
  c+=.10*cold*ink(min(g.x,g.y)/10.,0.)*lit;
  float jitter=.028*sin(T*2.+q.x*45.)*(1.-b);
  float y=height(q.x)+jitter;
  float edge=max(1.,(R.x/R.y)/1.42);
  float sweep=mix(-edge-.08,edge+.08,draw);
  float range=1.-smoothstep(sweep-.08,sweep,q.x);
  c+=cold*range*(.17*exp(-abs(q.y-y)*32.)+.60*ink(q.y-y,.004)+.10*step(q.y,y));
  vec2 node=vec2(sweep,height(sweep));
  c+=cold*exp(-length(q-node)*39.)*(.35+.4*E.y)*(1.-draw);
  float d=.14+end*.32;
  float deck=step(abs(q.x),.87)*smoothstep(q.x-.07,q.x+.015,-.89+1.82*b);
  float rail=ink(q.y-d,.009)+.25*ink(q.y-d+.035,.002);
  float x=fract(q.x*9.+.5)-.5;
  float post=ink(x/9.,.002)*step(d-.062,q.y)*step(q.y,d);
  float truss=ink(q.y-d+.066*abs(x*2.)+.009,.003)*step(d-.081,q.y);
  float footing=ink(abs(q.x)-.48,.007)*step(y-.045,q.y)*step(q.y,d);
  c+=gold*deck*(rail+post*.54+truss*.73+footing*.56+.19*exp(-abs(q.y-d)*29.))*(.76+.2*E.x);
  float car=fract((T-CROSS)*.18)*1.62-.81;
  c+=vec3(1.,.9,.70)*run*exp(-length((q-vec2(car,d+.012))*vec2(.7,1.))*57.)*(1.-end*.78);
  float gap=ink(length(q-vec2(0.,-.5+.023*sin(T*2.)))-.073,.002);
  c+=gap*vec3(.96,.35,.21)*phase(7.,3.)*(1.-b)*.78;
  c*=1.-.38*smoothstep(.65,1.42,length(q));
  O=vec4(closeUp(c*lit,p),1.);
}
`, K => {
  const send = room(K, 1.45, 2.7, .18, .21);
  for (let i = 7; i < 75; i++) {
    const t = i * beat;
    const n = [45, 45, 48, 43][Math.floor(i / 8) % 4];
    if (i % 2 === 0) K.o('sine', t, .24, 110, 43, i < 28 ? .12 : .29);
    if (i % 4 === 1 || i > 56) K.p('triangle', t + .018, .16, K.hz(n + [12, 19, 24][i % 3]), .094, .014, .45, send, 1400);
    if (i >= 28 && i % 2 === 1) K.n(t, .074, 3600, 1550, 1.5, .082, send);
  }
  signOff(K, send, 48);
});
