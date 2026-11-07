// Shared clock and drawing primitives. This source is included in every production's byte budget.
const beat = 60 / 100;
const build = 28 * beat;
const cross = 56 * beat;
const copy = [76, 80, 84].map(n => n * beat);
const glslTimes = 'const float BUILD=' + build.toFixed(2) + ',CROSS=' + cross.toFixed(2) +
  ';const vec3 CUES=vec3(' + copy.map(t => t.toFixed(2)).join(',') + ');\n';

const common = /*glsl*/`
float phase(float t,float w){return smoothstep(t,t+w,T);}
vec2 pixel(){return (2.*gl_FragCoord.xy-R)/R.y;}
vec2 world(vec2 p){return vec2(p.x/min(R.x/R.y,1.42),p.y);}
float ink(float d,float w){return 1.-smoothstep(w,w+max(1.9/R.y,.0015),abs(d));}
float seg(vec2 p,vec2 a,vec2 b){
  vec2 u=p-a,v=b-a;
  return length(u-v*clamp(dot(u,v)/dot(v,v),0.,1.));
}
vec3 closeUp(vec3 c,vec2 p){
  vec3 a=smoothstep(CUES,CUES+1.05,vec3(T));
  float mask=1.-smoothstep(.78,1.33,length(p/(TT.zw*vec2(1.03,1.26))));
  c*=1.-a.x*.74*mask;
  vec3 d=1.-smoothstep(vec3(-.0018),vec3(.0025),tdist(p));
  c=mix(c,vec3(.85,.86,1.),a.x*d.x);
  c=mix(c,vec3(1.,.78,.56),a.y*d.y);
  return mix(c,vec3(.99,.96,.91),a.z*d.z);
}
`;

function production(id, px, picture, music) {
  Dream.add({ id, dur: 60, fin: 53, cta: 54, build, cross, px, frag: glslTimes + common + picture, music });
}

function room(K, seconds, decay, wet, damping) {
  const bus = K.c.createGain();
  bus.connect(K.bus(.74));
  bus.connect(K.rev(seconds, decay, wet, K.out, damping));
  K.p('sine', 2.85, .85, 98, .014, .21, .74, bus);
  return bus;
}

// Each line in the closing composition enters with an audible note or chord.
function signOff(K, send, root) {
  copy.forEach((t, i) => {
    const notes = i === 2 ? [0, 4, 7, 12] : [i ? 7 : 2, 12 + (i ? 4 : 2)];
    for (const n of notes) K.p('sine', t, i === 2 ? 4.15 : .32, K.hz(root + n), .12, .018, 2.8, send);
  });
}
