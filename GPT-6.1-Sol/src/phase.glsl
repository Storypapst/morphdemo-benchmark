#version 130
uniform vec4 u;
void main(){
    float t=u.x;
    vec2 p=(gl_FragCoord.xy-u.yz*.5)/u.z;
    float r=length(p),a=atan(p.y,p.x),o=smoothstep(9.,30.,t),f=smoothstep(32.,49.,t);
    vec3 c=vec3(.005,.008,.015);
    for(int i=0;i<12;i++){
        float j=float(i),b=.06+j*.025;
        float w=.035*sin(3.*a+t*.4+j*.45)*(1.-f);
        w+=(1.-o)*.065*sin((mod(j,4.)+3.)*a+t*(.3+j*.045));
        w+=f*.075*cos(5.*a+t*.04);
        float d=abs(r-b-w);
        float appear=smoothstep(j*.8,j*.8+3.,t);
        vec3 color=mix(vec3(.03,.65,.85),vec3(1.,.4,.08),j/11.);
        c+=color*(.00035/(d+.002)+.55*exp(-d*d*1500000.))*appear*(.65+.25*u.w);
    }
    float spokes=abs(sin(a*5.-t*.02));
    c+=vec3(.09,.17,.2)*f*exp(-spokes*70.)*exp(-abs(r-.25)*12.);
    c*=1.-smoothstep(57.,63.,t)*.93;
    c=c/(1.+c);c=pow(c,vec3(.4545));
    const int g[5]=int[5](18405233,15255086,9616943,1097263,18415153);
    vec2 q=(p+vec2(.205,.36))/.014;
    int x=int(floor(q.x)),y=int(floor(q.y));
    if(x>=0&&x<29&&y>=0&&y<5&&x%6<5)
        c+=vec3(.78)*float((g[x/6]>>((4-y)*5+x%6))&1)*smoothstep(51.,55.,t);
    c*=smoothstep(0.,3.,t)*(1.-smoothstep(62.,64.,t));
    gl_FragColor=vec4(c,1);
}
