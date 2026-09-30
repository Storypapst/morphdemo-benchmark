// Shared prelude (audio, scene, post): hash, order curve and the event schedule. 96 BPM, 24 bars = 60 s.
uint H(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;}
float F(uint x){return float(H(x))*2.3283064e-10;}
float F(float x){return F(uint(x));}
float ORD(float t){return smoothstep(8.,46.,t);}          // order parameter: 0 = noise, 1 = crystal
float STA(float t){return 1.-smoothstep(4.,30.,t);}       // grain / storm level
// kick k (beat index, 96 BPM): jittered while the piece is still noisy, sparse before 20 s, absent in the breakdown 40..45 s
float KT(float k){float t=k*.625;return t+(1.-ORD(t))*(F(k+11.)-.5)*.2;}
float KV(float k){float t=k*.625;return step(F(k+99.),smoothstep(8.,20.,t)*1.2)*(1.-step(40.,t)+step(45.,t))*step(t,58.);}
float KE(float t){float e=0.,k=floor(t/.625);for(int j=0;j<3;j++){float a=k+1.-float(j),d=t-KT(a);e+=exp(-max(d,0.)*7.)*step(0.,d)*KV(a);}return e;}
// geiger clicks / lightning seeds: 24 slots per second, density rises then fades out
float CK(float t){float p=(.03+.4*smoothstep(0.,10.,t))*(1.-smoothstep(12.,24.,t));return step(1.-p,F(floor(t*24.)+777.))*exp(-fract(t*24.)*30.);}
// lightning: at most one strike per half second slot, a main flash followed by a weaker second one; storms end around 24 s
float LS(float s){return step(.62,F(s+555.))*(1.-step(.62,F(s+554.)))*(1.-smoothstep(16.,24.,s*.5))*step(1.,s*.5);}   // strike in half-second slot s (never in two neighbouring slots)
float LG(float t){float s=floor(t*2.),ph=fract(t*2.)*.5;return LS(s)*(exp(-ph*28.)+.7*step(.12,ph)*exp(-(ph-.12)*24.));}
// seconds since the most recent kick that actually sounds (99 if none in the last three beats)
float KP(float t){float k=floor(t/.625),m=99.;for(int j=0;j<3;j++){float a=k+1.-float(j),d=t-KT(a);if(d>=0.&&KV(a)>.5)m=min(m,d);}return m;}

// thunder: rumble arriving 0.4 s after each lightning strike (same schedule as LG)
float TH(float t){
  float e=0.,s0=floor(t*2.);
  for(int j=0;j<6;j++){float s=s0-float(j),d=t-(s*.5+.4);e+=LS(s)*step(0.,d)*exp(-max(d,0.)*1.5)*(.5+.5*F(s+9.));}
  return e;
}
