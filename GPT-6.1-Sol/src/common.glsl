#version 330 compatibility
uniform vec4 u;
#define T u.x
const float PI=3.14159265;
float sat(float x){return clamp(x,0.,1.);}
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}
float hash(float n){return fract(sin(n*127.1)*43758.5453);}
float hash3(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float noise(vec3 p){
    vec3 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);
    return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),
                   mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),
                   mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1,1,1)),f.x),f.y),f.z);
}
float box(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}
float segment(vec2 p,vec2 a,vec2 b){vec2 q=p-a,v=b-a;return length(q-v*clamp(dot(q,v)/dot(v,v),0.,1.));}
float word(vec2 p){
    p.x+=3.;int g=int(floor(p.x/1.2));p.x=mod(p.x,1.2)-.6;
    float d=10.;
    if(g==0){d=min(segment(p,vec2(-.4,-.5),vec2(-.4,.5)),segment(p,vec2(.4,-.5),vec2(.4,.5)));
        d=min(d,min(segment(p,vec2(-.4,.5),vec2(0,-.1)),segment(p,vec2(0,-.1),vec2(.4,.5))));}
    if(g==1)d=abs(length(p/vec2(.4,.5))-1.)*.4;
    if(g==2||g==3){
        d=segment(p,vec2(-.4,-.5),vec2(-.4,.5));
        d=min(d,segment(p,vec2(-.4,.5),vec2(.08,.5)));
        d=min(d,segment(p,vec2(-.4,0),vec2(.08,0)));
        if(p.x>=.08)d=min(d,abs(length(p-vec2(.08,.25))-.25));
        if(g==2)d=min(d,segment(p,vec2(-.03,0),vec2(.4,-.5)));
    }
    if(g==4){d=min(segment(p,vec2(-.4,-.5),vec2(-.4,.5)),segment(p,vec2(.4,-.5),vec2(.4,.5)));
        d=min(d,segment(p,vec2(-.4,0),vec2(.4,0)));}
    return d;
}
vec3 finish(vec3 c,vec2 p){
    float vignette=1.-.35*dot(p,p);
    c*=max(.25,vignette);
    c=c/(1.+c);c=pow(max(c,0.),vec3(.4545));
    c+=(hash3(vec3(gl_FragCoord.xy,T))-.5)/255.;
    float logo=smoothstep(52.,56.,T)*(1.-smoothstep(62.,64.,T));
    vec2 text=(p-vec2(0,-.31))/.067;
    float d=word(text)*.067;
    c=mix(c,vec3(.89,.9,.88),logo*(1.-smoothstep(.001,.0025,d)));
    /* A fine, quiet measure at the bottom also marks the three movements. */
    float ruler=(1.-smoothstep(.0007,.0018,abs(p.y+.438)))*step(abs(p.x),.64);
    c+=ruler*vec3(.09)*smoothstep(1.,4.,T)*(1.-smoothstep(60.,64.,T));
    c+=ruler*step(p.x,(T/64.-.5)*1.28)*vec3(.11,.13,.14);
    c*=smoothstep(0.,2.,T)*(1.-smoothstep(62.,64.,T));
    return clamp(c,0.,1.);
}
