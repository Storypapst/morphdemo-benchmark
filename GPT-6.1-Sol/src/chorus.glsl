/* CHORUS OF MATTER: broken porcelain and hot dust become three woven,
 * continuous ribbons, then a luminous, suspended crown. */
float order,resolve;
vec2 field(vec3 p){
    vec3 q=p;
    q.xz=rot(.10*T)*q.xz;
    q.xy=rot(.17*sin(T*.12))*q.xy;
    float a=atan(q.z,q.x),r=length(q.xz);
    float d=100.,material=1.;
    for(int i=0;i<3;i++){
        float ph=float(i)*2.094395;
        float radius=1.43+.22*cos(3.*a+ph)*resolve;
        float height=.62*sin(3.*a+ph+T*.22)*(1.-.30*resolve);
        height+=(1.-order)*.27*sin(a*8.+T*.57+ph);
        float tube=length(vec2(r-radius,q.y-height))-(.065+.095*order*(1.+.035*u.w));
        tube=max(tube,(sin(a*12.+ph+T*.27)-order*1.8)*.085);
        tube+=(1.-order)*.022*sin(19.*q.x+T)*sin(17.*q.y)*sin(16.*q.z);
        if(tube<d){d=tube;material=float(i)+1.;}
    }
    if(resolve>.001){
        vec3 s=q;s.y+=.6;
        float ring=length(vec2(length(s.xz)-2.08,s.y))-.027*resolve;
        if(ring<d){d=ring;material=4.;}
        s.y-=1.2;
        ring=length(vec2(length(s.xz)-2.08,s.y))-.027*resolve;
        if(ring<d){d=ring;material=4.;}
    }
    float floorD=p.y+1.52;
    if(floorD<d){d=floorD;material=0.;}
    return vec2(d,material);
}
vec3 normal(vec3 p){vec2 e=vec2(.0015,0);return normalize(vec3(field(p+e.xyy).x-field(p-e.xyy).x,
    field(p+e.yxy).x-field(p-e.yxy).x,field(p+e.yyx).x-field(p-e.yyx).x));}
float shadow(vec3 p,vec3 light){
    float visibility=1.,d=.04;
    for(int i=0;i<22;i++){float h=field(p+light*d).x;visibility=min(visibility,12.*h/d);d+=clamp(h,.04,.4);}
    return clamp(visibility,.12,1.);
}
void main(){
    vec2 uv=(gl_FragCoord.xy-.5*u.yz)/u.z;
    order=smoothstep(7.,27.,T);resolve=smoothstep(31.,49.,T);
    float coda=smoothstep(58.,63.,T);
    float angle=.34+.11*T;
    float radius=mix(7.4,5.8,order)+.6*coda;
    vec3 eye=vec3(radius*sin(angle),1.5+.6*sin(T*.075),radius*cos(angle));
    vec3 target=vec3(0,-.06,0);
    vec3 f=normalize(target-eye),right=normalize(cross(f,vec3(0,1,0))),up=cross(right,f);
    vec3 ray=normalize(f*1.75+uv.x*right+uv.y*up);
    vec3 color=vec3(.009,.011,.018);
    color+=vec3(.06,.025,.009)*pow(max(0.,dot(ray,normalize(vec3(-1,.4,-1)))),7.);
    vec3 background=color;
    float distance=0.,m=0.,h=0.;bool hit=false;
    for(int i=0;i<192;i++){
        vec2 v=field(eye+ray*distance);h=v.x;m=v.y;
        if(h<.0008*max(1.,distance*.2)){hit=true;break;}
        if(distance>35.)break;
        distance+=h*.46;
    }
    if(hit){
        vec3 p=eye+ray*distance,n=normal(p),light=normalize(vec3(-3,6,4));
        float sh=shadow(p+n*.012,light),diff=max(dot(n,light),0.)*sh;
        float ao=1.;
        for(int j=1;j<5;j++){float s=float(j)*.065;ao-=(s-field(p+n*s).x)*(.85/float(j));}
        ao=clamp(ao,.3,1.);
        float grain=noise(p*3.);
        float vein=exp(-65.*abs(sin(p.y*6.+grain*8.)));
        vec3 base=m<.5?vec3(.013,.016,.024):mix(vec3(.72,.69,.59),vec3(.36,.18,.045),vein*.6);
        if(m>3.5)base=vec3(.35,.17,.025);
        float fresnel=pow(1.-max(0.,dot(n,-ray)),4.);
        vec3 halfv=normalize(light-ray);
        float spec=pow(max(0.,dot(n,halfv)),90.)*sh;
        color=base*(.09+.85*diff+.16*max(n.y,0.))*ao;
        color+=vec3(1.,.73,.36)*spec*(.8+.7*resolve);
        if(m>.5)color+=vec3(.13,.2,.29)*fresnel*ao;
        float thread=pow(.5+.5*sin(atan(p.z,p.x)*18.-T*4.),16.);
        if(m>0.)color+=vec3(1.3,.37,.06)*thread*resolve*(.1+.22*u.w);
        if(m>3.5)color+=vec3(1.7,.6,.1)*(.35+.15*u.w);
        if(m<.5){
            /* A subdued reflection grounds the floating form in the room. */
            vec3 rr=reflect(ray,n);float rd=.06;bool reflection=false;
            for(int k=0;k<56;k++){
                float h=field(p+rr*rd).x;
                if(h<.004){reflection=true;break;}
                if(rd>9.)break;rd+=h*.46;
            }
            if(reflection){
                vec3 rn=normal(p+rr*rd);
                float lighting=.12+.8*max(0.,dot(rn,light));
                color+=vec3(.11,.09,.06)*lighting*exp(-rd*.18);
            }
            color+=vec3(.018,.007,.002)*exp(-length(p.xz)*.7)*resolve;
        }
        color=mix(color,background,1.-exp(-distance*.045));
        color=mix(color,background,smoothstep(16.,34.,distance));
    }
    /* Sparks are procedural world-space points. Their positions converge
     * onto the crown, rather than changing to an unrelated effect. */
    for(int i=0;i<38;i++){
        float id=float(i),a=id*2.399963+T*.11;
        vec3 chaotic=vec3(hash(id+3.)-.5,hash(id+31.)-.5,hash(id+47.)-.5)*8.;
        chaotic+=.45*sin(vec3(T*.5+id,T*.7+id*3.,T*.4+id*7.));
        vec3 organized=vec3(2.5*cos(a),.9*sin(a*3.+T*.2),2.5*sin(a));
        vec3 pos=mix(chaotic,organized,order*.92)-eye;
        float z=dot(pos,f);
        if(z>0.){
            vec2 q=1.75*vec2(dot(pos,right),dot(pos,up))/z;
            float r=length(uv-q);
            float intensity=(.7+.3*sin(T*3.+id))*(.4+.6*order)*(.65+.35*u.w);
            color+=vec3(.42,.20,.065)*exp(-r*r*4000.)*intensity;
            color+=vec3(1.6,.9,.3)*exp(-r*r*250000.)*intensity;
        }
    }
    color*=1.-.85*coda;
    gl_FragColor=vec4(finish(color,uv),1);
}
