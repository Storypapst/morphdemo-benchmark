// Shared prelude (audio, particles, post): hash, order curve and the event schedule. 128 BPM, 32 bars = 60 s.
uint H(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;}
float F(uint x){return float(H(x))*2.3283064e-10;}
float F(float x){return F(uint(x));}
float ORD(float t){return smoothstep(4.,34.,t);}          // order parameter: 0 = noise, 1 = word
float STA(float t){return 1.-smoothstep(2.,30.,t);}       // static / grain level
// kick k (beat index): jittered while noisy, sparse before 15 s, absent in the breakdown 41..45 s
float KT(float k){float t=k*.46875;return t+(1.-ORD(t))*(F(k+11.)-.5)*.16;}
float KV(float k){float t=k*.46875;return step(F(k+99.),smoothstep(6.,15.,t)*1.2)*(1.-step(41.,t)+step(45.,t))*step(t,58.);}
float KE(float t){float e=0.,k=floor(t/.46875);for(int j=0;j<3;j++){float a=k+1.-float(j),d=t-KT(a);e+=exp(-max(d,0.)*8.)*step(0.,d)*KV(a);}return e;}
// geiger clicks: 24 slots per second, density rises then fades out; the visual flashes a random cluster of dust per click
float CK(float t){float p=(.03+.5*smoothstep(0.,10.,t))*(1.-smoothstep(11.,20.,t));return step(1.-p,F(floor(t*24.)+777.))*exp(-fract(t*24.)*30.);}
// seconds since the most recent kick that actually sounds (99 if none in the last three beats)
float KP(float t){float k=floor(t/.46875),m=99.;for(int j=0;j<3;j++){float a=k+1.-float(j),d=t-KT(a);if(d>=0.&&KV(a)>.5)m=min(m,d);}return m;}
