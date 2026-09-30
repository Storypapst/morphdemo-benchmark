// Shared by the audio and the visual shader: hash, order curve and the event schedule that both sides read.
uint H(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;}
float F(uint x){return float(H(x))*2.3283064e-10;}
float F(float x){return F(uint(x));}
float ORD(float t){return smoothstep(8.,30.,t);}          // order parameter: 0 = noise, 1 = crystal
float STA(float t){return 1.-smoothstep(4.,22.,t);}       // static level
// kick k (beat index): nominal time k/2 s, jittered while the piece is still noisy; sparse before 16 s
float KT(float k){float t=k*.5;return t+(1.-ORD(t))*(F(k+11.)-.5)*.2;}
float KV(float k){return step(F(k+99.),smoothstep(6.,16.,k*.5)*1.2)*step(k*.5,42.)+step(44.,k*.5)*step(k*.5,59.);}
float KE(float t){float e=0.,k=floor(t*2.);for(int j=0;j<3;j++){float a=k+1.-float(j),d=t-KT(a);e+=exp(-max(d,0.)*9.)*step(0.,d)*KV(a);}return e;}
// geiger clicks: 24 slots per second, density rises and fades out again
float CK(float t){float p=(.04+.5*smoothstep(0.,12.,t))*(1.-smoothstep(14.,26.,t));return step(1.-p,F(floor(t*24.)+777.))*exp(-fract(t*24.)*30.);}
