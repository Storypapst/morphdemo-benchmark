#version 430
layout(location=0) uniform ivec3 u;
layout(binding=0) uniform sampler2D tx;
out vec4 o;
float T=float(u.x)*.001;
void main(){
  vec2 uv=gl_FragCoord.xy/vec2(u.yz);
  float O=ORD(T),S=STA(T);
  vec2 ca=(uv-.5)*.003*(1.+4.*S);
  vec3 c=vec3(texture(tx,uv+ca).r,texture(tx,uv).g,texture(tx,uv-ca).b);
  vec3 b=vec3(0);float w=1.;
  vec2 px=1./vec2(u.yz);
  for(int l=2;l<9;l++){float s=exp2(float(l))*.7;
    b+=(textureLod(tx,uv+vec2(s,s)*px,float(l)).rgb+textureLod(tx,uv+vec2(-s,s)*px,float(l)).rgb+textureLod(tx,uv+vec2(s,-s)*px,float(l)).rgb+textureLod(tx,uv-vec2(s,s)*px,float(l)).rgb+textureLod(tx,uv,float(l)).rgb)*.2*w;w*=.8;}
  c+=b*.22;
  float n=F(gl_FragCoord.x+gl_FragCoord.y*4099.+floor(T*24.)*77777.);
  c*=1.-.35*dot(uv-.5,uv-.5)*2.;
  c=1.-exp(-c*1.5);
  float SS=1.-smoothstep(.5,8.,T);                                          // static: television snow that tunes out during the first seconds
  float st=step(.5,F(floor(gl_FragCoord.y*.25)+floor(T*20.)*991.));
  c=mix(c,vec3(n*n)*(.55+.45*st),.9*SS*step(F(floor(gl_FragCoord.y*.25)*13.+floor(T*15.)*991.+5.),SS*1.1));   // bands of the picture drop out to snow, never the whole frame at once
  c+=(n-.5)*(.02+.06*S*S);
  c*=1.-smoothstep(58.,60.,T);
  o=vec4(pow(max(c,0.),vec3(.4545)),1);
}
