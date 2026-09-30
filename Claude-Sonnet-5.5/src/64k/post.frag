#version 430
layout(location=0) uniform ivec3 u;
layout(binding=0) uniform sampler2D tx;
out vec4 o;
float T=float(u.x)*.001;
vec3 aces(vec3 x){return clamp((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14),0.,1.);}
void main(){
  vec2 uv=gl_FragCoord.xy/vec2(u.yz);
  float S=STA(T),K=KE(T);
  vec2 d=uv-.5;
  vec2 ca=d*(.002+.004*S+.003*K);
  vec3 c=vec3(texture(tx,uv+ca).r,texture(tx,uv).g,texture(tx,uv-ca).b);
  vec3 b=vec3(0);float w=1.;
  vec2 px=1./vec2(textureSize(tx,0));
  for(int l=2;l<9;l++){float s=exp2(float(l))*.7;
    b+=(textureLod(tx,uv+vec2(s,s)*px,float(l)).rgb+textureLod(tx,uv+vec2(-s,s)*px,float(l)).rgb+textureLod(tx,uv+vec2(s,-s)*px,float(l)).rgb+textureLod(tx,uv-vec2(s,s)*px,float(l)).rgb+textureLod(tx,uv,float(l)).rgb)*.2*w;w*=.75;}
  c+=b*(.22+.18*K)*mix(1.,.6,smoothstep(38.,52.,T));
  float n=F(gl_FragCoord.x+gl_FragCoord.y*4099.+floor(T*24.)*77777.);
  c*=1.-.5*dot(d,d)*1.6;
  c=aces(c*mix(1.15,.62,smoothstep(38.,52.,T)));
  c=pow(c,vec3(.4545));
  float SS=1.-smoothstep(.5,3.4,T);                                       // television snow that tunes out during the first seconds
  float st=step(.5,F(floor(gl_FragCoord.y*.25)+floor(T*20.)*991.));
  c=mix(c,vec3(n*n)*(.55+.45*st),.9*SS*step(F(floor(gl_FragCoord.y*.25)*13.+floor(T*15.)*991.+5.),SS*1.1));   // bands of the picture drop out to snow, never the whole frame at once
  c+=(n-.5)*(.02+.06*S);
  float fl=step(56.,T)*exp(-max(T-56.,0.)*7.);          // a short burst on the final chord, then it dies away in half a second
  c+=vec3(1.,.93,.8)*fl*.75;
  c*=1.-smoothstep(58.5,60.,T);
  o=vec4(c,1);
}
