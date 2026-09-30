/* LATTICE OF LIGHT: an irregular cloud of angular fragments phase-locks into
 * three geometric orbits surrounding a nested octahedral light chamber. */
float order,resolve;
float frame(vec3 p,vec3 b,float e){
    vec3 q=abs(p);
    return min(box(vec3(q.x,q.y-b.y,q.z-b.z),vec3(b.x,e,e)),
           min(box(vec3(q.x-b.x,q.y,q.z-b.z),vec3(e,b.y,e)),
               box(vec3(q.x-b.x,q.y-b.y,q.z),vec3(e,e,b.z))));
}
float edge(vec3 p,vec3 a,vec3 b){vec3 v=b-a;return length(p-a-v*clamp(dot(p-a,v)/dot(v,v),0.,1.));}
float crystal(vec3 p,float r){
    p=abs(p);
    return min(edge(p,vec3(r,0,0),vec3(0,r,0)),
           min(edge(p,vec3(0,r,0),vec3(0,0,r)),edge(p,vec3(0,0,r),vec3(r,0,0))));
}
vec2 field(vec3 p){
    p.xz=rot(T*.075)*p.xz;
    float d=20.,mat=0.;
    for(int layer=0;layer<3;layer++){
        float y=float(layer)-1.;
        vec3 s=p;s.y-=y*1.2;
        s.xz=rot(y*.18*sin(T*.12))*s.xz;
        float a=atan(s.z,s.x),r=length(s.xz),sector=PI/6.;
        float cell=floor((a+sector*.5)/sector);
        a=mod(a+sector*.5,sector)-sector*.5;
        /* Adjacent sectors are evaluated too, so scattered fragments can
         * cross cell boundaries without being clipped. */
        for(int neighbor=-1;neighbor<=1;neighbor++){
            float id=mod(cell+float(neighbor),12.),b=a-float(neighbor)*sector;
            vec3 q=vec3(r*sin(b),s.y,r*cos(b)-mix(3.1,2.6,resolve));
            q+=vec3(sin(id*2.3+T*.8),sin(id*3.7+T*.6),cos(id*1.7+T*.5))*(1.-order)*.8;
            q.xy=rot((1.-order)*sin(id+T*.2)*1.3)*q.xy;
            float shape=frame(q,vec3(mix(.29,.697,resolve),.33+.2*resolve,mix(.26,.035,resolve)),.011)-.003;
            if(shape<d){d=shape;mat=y+1.;}
        }
    }
    vec3 q=p;q.xy=rot(.24*T)*q.xy;q.yz=rot(.17*T)*q.yz;
    float core=crystal(q,1.72)-.022;
    core=max(core,.20-resolve*.8);
    if(core<d){d=core;mat=3.;}
    q.xz=rot(-T*.36)*q.xz;
    float inner=min(frame(q,vec3(.56),.014),crystal(q,1.02)-.018);
    inner=max(inner,.18-resolve*.5);
    if(inner<d){d=inner;mat=4.;}
    return vec2(d,mat);
}
void main(){
    vec2 uv=(gl_FragCoord.xy-.5*u.yz)/u.z;
    order=smoothstep(8.,28.,T);resolve=smoothstep(27.,46.,T);
    float coda=smoothstep(58.,63.,T);
    float a=.5+T*.063;
    vec3 eye=vec3(sin(a),.24+.06*sin(T*.17),cos(a))*(8.3-.6*order+.8*coda);
    vec3 forward=normalize(-eye),right=normalize(cross(forward,vec3(0,1,0))),up=cross(right,forward);
    vec3 ray=normalize(forward*1.25+uv.x*right+uv.y*up);
    vec3 col=vec3(.006,.009,.023);
    float d=0.,glow=0.,material=0.;bool hit=false;
    for(int i=0;i<144;i++){
        vec2 h=field(eye+ray*d);material=h.y;
        glow+=exp(-abs(h.x)*32.)*.014;
        if(h.x<.0012){hit=true;break;}
        if(d>15.)break;
        d+=max(h.x*.65,.002);
    }
    if(hit){
        vec3 p=eye+ray*d;vec2 e=vec2(.001,0);
        vec3 n=normalize(vec3(field(p+e.xyy).x-field(p-e.xyy).x,
             field(p+e.yxy).x-field(p-e.yxy).x,field(p+e.yyx).x-field(p-e.yyx).x));
        float diff=.25+.75*max(0.,dot(n,normalize(vec3(1,2,3))));
        vec3 tint=mix(vec3(.005,.55,2.),vec3(.42,.008,.8),sat(p.y*.3+.5));
        if(material>2.5)tint=mix(vec3(.4,.8,1.4),vec3(1.,.2,.55),sat(sin(T*.2)*.5+.5));
        float sweep=pow(.5+.5*sin(p.y*6.-T*6.283185),12.);
        col=tint*diff*(.55+.6*order)+tint*sweep*(.2+.45*u.w)*resolve;
        col+=vec3(.04,.3,.8)*pow(1.-max(0.,dot(n,-ray)),3.)*.65;
    }
    col+=glow*vec3(.06,.4,.9)*(.6+.5*order+.25*u.w);
    /* A dim reference grid gradually straightens behind the main structure. */
    vec2 grid=uv*vec2(14,14);
    grid+=sin(grid.yx*.7+T*.3)*(1.-order)*.8;
    float lines=min(abs(fract(grid.x)-.5),abs(fract(grid.y)-.5));
    col+=vec3(.012,.03,.055)*exp(-lines*80.)*(.3+.7*order)*exp(-length(uv)*1.5);
    for(int i=0;i<28;i++){
        float id=float(i);
        vec3 chaotic=vec3(hash(id+5.)-.5,hash(id+77.)-.5,hash(id+13.)-.5)*7.;
        vec3 p=mix(chaotic,vec3(mod(id,7.)-3.,floor(id/7.)-1.5,-1.)*.7,order);
        p.xz=rot(T*.075)*p.xz;p-=eye;
        float z=dot(p,forward);
        if(z>0.){vec2 q=1.25*vec2(dot(p,right),dot(p,up))/z;
            float r=length(uv-q);col+=vec3(.1,.4,.8)*exp(-r*r*120000.)*(.6+.3*u.w);}
    }
    col*=1.-.85*coda;
    gl_FragColor=vec4(finish(col,uv),1);
}
