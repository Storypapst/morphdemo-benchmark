#version 330 core
out vec4 color;
uniform float T;
uniform vec2 R;
const float PI=3.14159265359;
mat2 rot(float a){return mat2(cos(a),-sin(a),sin(a),cos(a));}
float hash(float n){return fract(sin(n*127.1)*43758.5453);}
float hash3(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1)),f.x),f.y),f.z);}
float fbm(vec3 p){float n=0.,a=.5;for(int i=0;i<4;i++){n+=a*noise(p);p=p*2.03+vec3(1.7,9.2,3.4);a*=.5;}return n;}
float order;
float pulse;
float cameraDist;
// Distances to six continuous helical cables, woven around one common ring.
vec3 cable(vec3 p){
    float a=atan(p.z,p.x);
    float R0=1.68+.11*sin(a*3.+T*.25)*(1.-order);
    vec2 q=vec2(length(p.xz)-R0,p.y);
    q*=rot(a*3.+T*.105);
    float twist=atan(q.y,q.x);
    float sector=2.*PI/6.;
    float id=floor((twist+sector*.5)/sector);
    vec2 w=q*rot(-id*sector)-vec2(.34,0.);
    float drift=(1.-order)*(.1+.19*sin(a*7.+id*4.+T*.8));
    float radius=.086+.011*sin(a*54.+id*1.1);
    float d=length(w)-radius+drift;
    // The first act contains incomplete strands; they close into a single fabric.
    float fissure=(sin(a*7.+id*2.+T*.12)-mix(-.35,1.3,order))*.13;
    d=max(d,fissure);
    return vec3(d,id,a);
}
vec3 scene(vec3 p){
    p.xz*=rot(T*.045);
    vec3 c=cable(p);
    // Three impossibly thin engraving rings complete the celestial instrument.
    vec3 r=p; r.xy*=rot(.32);r.yz*=rot(.3);
    float orbital=abs(length(vec2(length(r.xz)-2.45,r.y)))-.012;
    r=p;r.xy*=rot(-.32);r.yz*=rot(-.3);
    orbital=min(orbital,length(vec2(length(r.xz)-2.67,r.y))-.009);
    orbital+=.16*(1.-smoothstep(22.,38.,T));
    if(orbital<c.x)c=vec3(orbital,8.,atan(r.z,r.x));
    // The inner cage is assembled from meridians, parallels, and radial needles.
    r=p;r.xz*=rot(-T*.09);r.xy*=rot(.28);
    float radius=length(r),longitude=atan(r.z,r.x),latitude=acos(clamp(r.y/max(radius,.001),-1.,1.));
    float weave=min(abs(sin(longitude*9.))*length(r.xz)/9.,abs(sin(latitude*8.))*radius/8.);
    float cage=length(vec2(radius-.91,weave))-.007;
    float radial=length(p.xz);
    float spoke=length(vec2(p.y,sin(atan(p.z,p.x)*9.)*radial/9.))-.008;
    spoke=max(spoke,max(.91-radial,radial-1.35));
    cage=min(cage,spoke)+.2*(1.-smoothstep(23.,36.,T));
    if(cage<c.x)c=vec3(cage,9.,longitude);
    return c;
}
vec3 normalAt(vec3 p){vec2 e=vec2(.002,0);return normalize(vec3(scene(p+e.xyy).x-scene(p-e.xyy).x,scene(p+e.yxy).x-scene(p-e.yxy).x,scene(p+e.yyx).x-scene(p-e.yyx).x));}
vec3 sky(vec3 d){
    float milky=fbm(d*4.+vec3(0,T*.009,0));
    float cloud=pow(max(0.,milky-.29),3.);
    vec3 col=vec3(.002,.005,.014)+cloud*vec3(.08,.18,.23);
    col+=pow(max(0.,1.-abs(d.y+.12)),18.)*.015*vec3(.8,.5,.18);
    for(int j=0;j<3;j++){
        vec3 p=d*(110.+float(j)*90.);
        vec3 cell=floor(p),f=fract(p)-.5;
        float h=hash3(cell);
        float star=pow(max(0.,1.-length(f)*2.7),9.);
        col+=star*step(.973,h)*(.3+hash3(cell+7.)*2.)*mix(vec3(.27,.64,.85),vec3(1.,.62,.24),hash3(cell+9.));
    }
    return col;
}
void main(){
    vec2 uv=(2.*gl_FragCoord.xy-R)/R.y;
    order=smoothstep(5.,32.,T);
    pulse=exp(-fract(T*(T<28.?1.:2.))*7.)*smoothstep(10.,25.,T)*(1.-smoothstep(52.,55.,T));
    float establish=smoothstep(0.,13.,T);
    float open=smoothstep(25.,37.,T);
    float finale=smoothstep(46.,56.,T);
    float az=.18+T*.07+sin(T*.075)*.22;
    float elev=mix(.15,.68,open)-finale*.17+.07*sin(T*.18);
    cameraDist=mix(4.6,5.35,establish)+.5*open+1.25*finale;
    vec3 ro=vec3(cos(az)*cos(elev),sin(elev),sin(az)*cos(elev))*cameraDist;
    vec3 target=vec3(0,-.08,0);
    vec3 ww=normalize(target-ro),uu=normalize(cross(ww,vec3(0,1,0))),vv=cross(uu,ww);
    vec3 rd=normalize(uu*uv.x+vv*uv.y+ww*1.72);
    vec3 col=sky(rd);
    float tt=.1,d=1.;vec3 hit=vec3(0);float glow=0.;
    for(int i=0;i<124;i++){
        vec3 p=ro+rd*tt;
        hit=scene(p);d=hit.x;
        glow+=exp(-max(d,.001)*28.)*max(.003,d*.72)*1.5;
        if(d<.0014||tt>12.)break;
        tt+=max(d*.72,.003);
    }
    if(tt<12.&&d<.005){
        vec3 p=ro+rd*tt,n=normalAt(p);
        vec3 l=normalize(vec3(-3,5,3)-p),h=normalize(l-rd);
        float diffuse=max(0.,dot(n,l));
        float fres=pow(1.-max(0.,dot(n,-rd)),3.);
        float gleam=pow(max(0.,dot(n,h)),100.);
        vec3 copper=vec3(.72,.245,.065),teal=vec3(.018,.24,.23);
        float idx=mod(abs(hit.y),3.);
        vec3 albedo=mix(copper,teal,step(1.5,idx));
        vec3 pp=p;pp.xz*=rot(T*.045);
        float angle=atan(pp.z,pp.x);
        vec2 q=vec2(length(pp.xz)-1.68,pp.y);q*=rot(angle*3.+T*.105);
        vec2 w=q*rot(-hit.y*PI/3.)-vec2(.34,0.);
        float engraved=.5+.5*sin(atan(w.y,w.x)*26.+angle*120.);
        albedo*=.7+.3*engraved;
        float wave=pow(max(0.,sin(angle*3.-T*2.4+hit.y*1.1)),22.);
        float seam=pow(engraved,18.);
        vec3 emission=mix(vec3(1.6,.6,.12),vec3(.02,.95,1.25),step(1.5,idx));
        col=albedo*(.1+diffuse*.82)+gleam*vec3(2.2,1.8,1.1)+fres*vec3(.2,.47,.55);
        col+=emission*(seam*.13+wave*(.28+pulse*.5))*order;
        if(hit.y>7.)col=vec3(.55,.31,.12)*(.9+pulse)+vec3(.8,.58,.26)*pow(max(0.,sin(hit.z*45.+T)),20.);
        if(hit.y>8.5)col=vec3(.72,.48,.22)*(.5+diffuse)+vec3(1.5,1.,.5)*gleam;
        // A secondary amber softbox outlines the shadowed half of the weave.
        col+=vec3(.7,.18,.035)*pow(max(0.,dot(n,normalize(vec3(3,-2,-3)))),2.)*.3;
        col*=exp(-tt*.026);
    }
    col+=glow*vec3(.1,.23,.23)*(1.+pulse*.4);
    // The core is a volumetric radial engine: rays condense as order appears.
    float closest=max(0.,dot(-ro,rd));
    vec3 nearP=ro+rd*closest;
    float coreD=length(nearP);
    float vis=tt>closest?.95:.12;
    float core=exp(-coreD*coreD*20.)*smoothstep(19.,48.,T);
    col+=vis*core*vec3(2.5,1.05,.28)*(1.+pulse*.65);
    col+=vis*exp(-coreD*2.4)*vec3(.055,.12,.14)*open;
    vec2 centerUV=vec2(dot(-ro,uu),dot(-ro,vv));
    float rays=pow(abs(sin(atan(uv.y-centerUV.y,uv.x-centerUV.x)*21.+T*.03)),22.);
    col+=vis*rays*exp(-coreD*1.7)*vec3(.06,.034,.016)*open;
    // Floating golden microfilaments: perspective, parallax, and out-of-focus bokeh.
    for(int j=0;j<5;j++){
        float fj=float(j);
        vec2 p=uv*(7.+fj*6.);
        p+=vec2(T*(.09+fj*.027),sin(T*.14+fj)*.35);
        vec2 cell=floor(p),q=fract(p)-.5;
        float h=hash(dot(cell,vec2(31.1,73.9))+fj*53.);
        q-=.3*vec2(sin(h*36.+T*.25),cos(h*27.+T*.31));
        float dd=length(q*vec2(1.,1.8));
        float dust=exp(-dd*dd*(180.+fj*150.))*step(.75,h);
        col+=dust*mix(vec3(.085,.25,.29),vec3(.6,.3,.065),h)*(.3+fj*.12)*(.2+.8*order);
    }
    // The initial field is sparse and unstable, then the sculpture gains substance.
    float fade=smoothstep(0.,3.,T)*(1.-smoothstep(57.,60.,T));
    color=vec4(col*fade,1.);
}
