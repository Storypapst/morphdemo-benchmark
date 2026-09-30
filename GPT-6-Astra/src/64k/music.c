/* THE LOOM — original procedural composition. 120 BPM, thirty bars.
   No samples, external tables, or music engine. All instruments, delay,
   stereo placement, phrasing, and mastering are generated here. */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#define RATE 48000
#define LENGTH 60
#define NSAMPLES (RATE*LENGTH)
#define TAU 6.2831853071795864769f
static float *song;
static uint32_t rng=0x53bd96e1;
static float random_audio(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (float)(int32_t)rng/2147483648.f;}
static float notehz(int n){return 440.f*powf(2.f,(n-69)/12.f);}
static float clamp01(float a){return fminf(1.f,fmaxf(0.f,a));}
static void tone(float at,float duration,int midi,float amplitude,float pan,int kind){
 int start=(int)(at*RATE),count=(int)(duration*RATE);float hz=notehz(midi),phase=0.;
 float left=sqrtf((1.f-pan)*.5f),right=sqrtf((1.f+pan)*.5f);
 for(int i=0;i<count&&i+start<NSAMPLES;i++){
  float t=(float)i/RATE,v=0.f;
  phase+=TAU*hz/RATE;
  if(phase>TAU)phase-=TAU;
  if(kind==0){ // soft struck glass, inharmonic upper partials
   float env=(1.f-expf(-t*350.f))*expf(-t*3.6f);
   v=(sinf(phase+.55f*expf(-t*8.f)*sinf(phase*2.f))+.28f*sinf(t*TAU*hz*2.003f)*expf(-t*8.f)+.08f*sinf(t*TAU*hz*4.013f)*expf(-t*12.f))*env;
  }else if(kind==1){ // breathy, slowly moving four-voice pad
   float env=clamp01(t/1.2f)*clamp01((duration-t)/1.6f);
   v=(sinf(phase)+.5f*sinf(phase*2.f)+.2f*sinf(phase*3.f)+.35f*sinf(t*TAU*hz*1.002f))*.45f*env;
   v*=.92f+.08f*sinf(t*2.1f);
  }else if(kind==2){ // rounded plucked bass with pitch transient
   v=(sinf(phase+.65f*expf(-t*17.f)*sinf(phase))+.2f*sinf(phase*2.f))*expf(-t*4.5f)*(1.f-expf(-t*280.f));
  }else { // high answering motif with a long descending envelope
   v=(sinf(phase+.25f*sinf(phase*3.f)*expf(-t*3.f))+.15f*sinf(phase*2.f))*expf(-t*1.8f)*(1.f-expf(-t*40.f));
  }
  v*=amplitude*clamp01((duration-t)*30.f);
  song[(start+i)*2]+=v*left;song[(start+i)*2+1]+=v*right;
 }
}
static void drum(float at,float amplitude,int kind,float pan){
 int start=(int)(at*RATE),count=(int)(RATE*(kind==0?.55:kind==1?.25:.09));
 float last=0.f;
 for(int i=0;i<count&&start+i<NSAMPLES;i++){
  float t=(float)i/RATE,n=random_audio(),v;
  if(kind==0){float phase=TAU*(45.f*t+5.1f*(1.f-expf(-t*25.f)));v=sinf(phase)*expf(-t*10.f)+n*.13f*expf(-t*120.f);}
  else if(kind==1){last=.55f*last+.45f*n;v=(last*.8f+sinf(TAU*178.f*t)*.14f)*expf(-t*20.f)*(1.f-expf(-t*1400.f));}
  else{v=(n-last)*expf(-t*80.f)*.4f;last=n;}
  v*=amplitude;song[(start+i)*2]+=v*(.7f-pan*.35f);song[(start+i)*2+1]+=v*(.7f+pan*.35f);
 }
}
static float *compose(void){
 song=calloc(NSAMPLES*2,sizeof(float));if(!song)return 0;
 const int chords[4][4]={{50,57,60,65},{46,53,57,62},{53,60,64,69},{48,55,59,64}};
 const int melody[16]={74,0,77,76,72,0,69,72,74,77,81,79,77,76,72,69};
 const int pattern[8]={0,2,1,3,2,1,3,2};
 for(int bar=0;bar<13;bar++){
  float at=bar*4.f;int chord=bar%4;
  for(int n=0;n<4;n++)tone(at,5.6f,chords[chord][n],.10f*(bar<3?.55f:1.f),(n-1.5f)*.30f,1);
 }
 // Sparse unanswered tones open the work, before the motor finds its pulse.
 tone(1.8f,4.f,86,.11f,-.45f,3);tone(4.6f,4.f,81,.08f,.6f,3);tone(7.4f,4.f,77,.1f,-.2f,3);
 for(int beat=16;beat<104;beat++){
  float at=beat*.5f;int chord=(beat/8)%4;
  float power=beat<32?.55f:beat<56?.8f:1.f;
  if(beat%2==0||beat>=56)drum(at,.32f*power,0,0);
  if(beat%4==2&&beat>=32)drum(at,.20f,1,-.1f);
  drum(at+.25f,.07f*power,2,(beat%2==0?.55f:-.55f));
  if(beat>=40)drum(at,.032f,2,-.35f);
  if(beat%2==0||beat>=56)tone(at+.03f,.7f,chords[chord][0]-12,.25f*power,0,2);
  if(beat%4==3&&beat>=56)tone(at+.30f,.35f,chords[chord][0],.11f,-.15f,2);
 }
 for(int eighth=48;eighth<208;eighth++){
  float at=eighth*.25f;int chord=(eighth/16)%4;
  int p=pattern[eighth%8],oct=(eighth%8==7?24:12);
  float amp=.077f*(eighth<96?.7f:1.f);
  if(eighth<80&&eighth%2)continue;
  tone(at,1.9f,chords[chord][p]+oct,amp,(p-1.5f)*.28f,0);
 }
 for(int n=0;n<48;n++){
  int m=melody[n%16];if(!m)continue;
  float at=28.f+n*.5f;
  tone(at,2.7f,m,.135f,n%2?.32f:-.32f,3);
 }
 // A final major-third release turns the unresolved opening into consonance.
 for(int n=0;n<4;n++){int ending[4]={50,57,62,66};tone(54.f,6.f,ending[n],.12f,(n-1.5f)*.22f,1);}
 tone(54.f,5.8f,86,.14f,-.2f,3);tone(55.f,4.9f,81,.09f,.3f,3);
 // Diffuse stereo tail and beat-related ping-pong reflections.
 const int delays[5]={6000,9000,14413,19717,25331};
 float *reverb=calloc(NSAMPLES*2,sizeof(float));
 if(reverb){
  float lowL=0,lowR=0;
  for(int i=0;i<NSAMPLES;i++){
   float l=0,r=0;
   for(int j=0;j<5;j++)if(i>=delays[j]){int k=(i-delays[j])*2;l+=reverb[k+1]*.105f+song[k]*.070f;r+=reverb[k]*.105f+song[k+1]*.070f;}
   lowL=.6f*lowL+.4f*l;lowR=.6f*lowR+.4f*r;reverb[i*2]=lowL;reverb[i*2+1]=lowR;
  }
  for(int i=0;i<NSAMPLES*2;i++)song[i]+=reverb[i];free(reverb);
 }
 float peak=.001f;
 for(int i=0;i<NSAMPLES*2;i++){float s=fabsf(song[i]);if(s>peak)peak=s;}
 float gain=.87f/peak;
 for(int i=0;i<NSAMPLES;i++){
  float t=(float)i/RATE,fade=clamp01(t/1.5f)*clamp01((60.f-t)/3.2f);
  for(int c=0;c<2;c++)song[i*2+c]*=gain*fade;
 }
 return song;
}
static int write_wave(const char *path){
 FILE *f=fopen(path,"wb");if(!f)return 1;
 uint32_t data=NSAMPLES*4,header[11]={0x46464952,data+36,0x45564157,0x20746d66,16,0x00020001,RATE,RATE*4,0x00100004,0x61746164,data};
 fwrite(header,1,44,f);
 for(int i=0;i<NSAMPLES*2;i++){int16_t s=(int16_t)(song[i]*32767.f);fwrite(&s,2,1,f);}fclose(f);return 0;
}
