// Context-mixing model experiments: computes the ideal code length of a file under a parametrised model.
// usage: cmexp file [key=value ...]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MAXM 16
static int NM = 6;
static int masks[MAXM] = {0x00,0x01,0x03,0x07,0x0f,0x3f};
static double W0 = 0.3, LR = 0.02;
static int LIM = 30;          // count limit for adaptive rate
static double RATE0 = 1.5;    // rate = 1/(n+RATE0)
static int WSEL = 0;          // 0 none, 1 by bitpos, 2 by c0
static int ADAPT = 1;         // adaptive weights
static int TBITS = 22;
static double SCALE = 1.0;
static int CNTMODE = 0;       // 0: prob+count, 1: (n0,n1) counts
static int NL = 24;           // count cap for mode 1
static double DELTA = 0.4;    // for mode 1 estimator
static int NONSTAT = 2;       // mode 1: reduce opposite count when > NONSTAT
static int APM = 0;
static int DUAL = 0, LIM2 = 4;
static int SSE = 0;
static int MIX = 0; static double AA = 0.1; static double WLIN[MAXM] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

static double stretch(double p){ return log(p/(1-p)); }
static double squash(double x){ return 1/(1+exp(-x)); }

int main(int argc,char**argv){
  if(argc<2) return 1;
  for(int i=2;i<argc;i++){
    char *e=strchr(argv[i],'='); if(!e) continue; *e=0; const char *k=argv[i],*v=e+1;
    if(!strcmp(k,"w0")) W0=atof(v); else if(!strcmp(k,"lr")) LR=atof(v); else if(!strcmp(k,"lim")) LIM=atoi(v);
    else if(!strcmp(k,"rate0")) RATE0=atof(v); else if(!strcmp(k,"wsel")) WSEL=atoi(v); else if(!strcmp(k,"adapt")) ADAPT=atoi(v);
    else if(!strcmp(k,"cnt")) CNTMODE=atoi(v); else if(!strcmp(k,"nl")) NL=atoi(v); else if(!strcmp(k,"delta")) DELTA=atof(v);
    else if(!strcmp(k,"dual")) DUAL=atoi(v); else if(!strcmp(k,"lim2")) LIM2=atoi(v); else if(!strcmp(k,"sse")) SSE=atoi(v);
    else if(!strcmp(k,"mix")) MIX=atoi(v); else if(!strcmp(k,"aa")) AA=atof(v);
    else if(!strcmp(k,"wl")){ char *s2=strdup(v),*t=strtok(s2,","); int q=0; while(t){ WLIN[q++]=atof(t); t=strtok(0,","); } }
    else if(!strcmp(k,"nonstat")) NONSTAT=atoi(v); else if(!strcmp(k,"tbits")) TBITS=atoi(v);
    else if(!strcmp(k,"masks")){ NM=0; char *s=strdup(v),*t=strtok(s,","); while(t){ masks[NM++]=strtol(t,0,16); t=strtok(0,","); } }
  }
  FILE *f=fopen(argv[1],"rb"); fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET); uint8_t *d=malloc(n); fread(d,1,n,f); fclose(f);
  size_t TS=(size_t)1<<TBITS;
  // slots: prob (double) + count / or n0,n1
  float *P=malloc(TS*sizeof(float)); float *Q=malloc(TS*sizeof(float)); uint8_t *C2=calloc(TS,1); for(size_t i=0;i<TS;i++) Q[i]=0.5f; uint8_t *C=calloc(TS,1); uint8_t *N0=calloc(TS,1),*N1=calloc(TS,1);
  for(size_t i=0;i<TS;i++) P[i]=0.5f;
  int nws = WSEL==0?1:WSEL==1?8:256;
  double *Wt=malloc(sizeof(double)*nws*2*MAXM); for(int i=0;i<nws*2*MAXM;i++) Wt[i]=W0*(DUAL?0.6:1);
  double cost=0; uint64_t hist=0;
  double st[2*MAXM]; size_t idx[MAXM];
  for(long pos=0;pos<n;pos++){
    uint32_t h[MAXM];
    for(int i=0;i<NM;i++){
      uint64_t m=0; for(int b=0;b<8;b++) if(masks[i]>>b&1) m|=0xffULL<<(8*b);
      uint64_t x=(hist&m)*0x9E3779B97F4A7C15ULL + (uint64_t)(i+1)*0xD6E8FEB86659FD93ULL; x^=x>>29; x*=0xBF58476D1CE4E5B9ULL; x^=x>>32;
      h[i]=(uint32_t)x;
    }
    int c0=1;
    for(int bit=7;bit>=0;bit--){
      int y=(d[pos]>>bit)&1;
      int ws = WSEL==0?0:WSEL==1?(7-bit):c0; double *w=Wt+ws*2*MAXM;
      double dot=0;
      for(int i=0;i<NM;i++){
        idx[i]=(h[i]+ (uint32_t)c0*0x9E3779B1u) & (TS-1);
        double p;
        if(CNTMODE==0) p=P[idx[i]];
        else { double a=N0[idx[i]],b=N1[idx[i]]; p=(b+DELTA)/(a+b+2*DELTA); }
        if(p<1e-4) p=1e-4; if(p>1-1e-4) p=1-1e-4;
        st[i]=stretch(p); dot+=w[i]*st[i];
        if(DUAL){ double q=Q[idx[i]]; if(q<1e-4) q=1e-4; if(q>1-1e-4) q=1-1e-4; st[NM+i]=stretch(q); dot+=w[NM+i]*st[NM+i]; }
      }
      double p;
      if(MIX==1){ double a=0,b=0; for(int i=0;i<NM;i++){ a+=WLIN[i]*N0[idx[i]]; b+=WLIN[i]*N1[idx[i]]; } p=(b+AA)/(a+b+2*AA); }
      else p=squash(dot);
      if(p<1.0/4096) p=1.0/4096; if(p>1-1.0/4096) p=1-1.0/4096;
      cost+= -log2(y?p:1-p);
      double err=y-p;
      if(ADAPT) for(int i=0;i<NM*(DUAL?2:1);i++) w[i]+=LR*err*st[i];
      for(int i=0;i<NM;i++){
        size_t k=idx[i];
        if(CNTMODE==0){ int c=C[k]; double r=1.0/(c+RATE0); P[k]+= (y-P[k])*r; if(c<LIM) C[k]=c+1; if(DUAL){ int c2=C2[k]; Q[k]+=(y-Q[k])/(c2+RATE0); if(c2<LIM2) C2[k]=c2+1; } }
        else { if(y){ if(N1[k]<NL) N1[k]++; if(N0[k]>NONSTAT) N0[k]=(N0[k]+NONSTAT+1)/2; } else { if(N0[k]<NL) N0[k]++; if(N1[k]>NONSTAT) N1[k]=(N1[k]+NONSTAT+1)/2; } }
      }
      c0=c0*2+y;
    }
    hist=(hist<<8)|d[pos];
  }
  printf("%ld -> %.1f bytes (%.2f%%)\n",n,cost/8,100*cost/8/n);
  return 0;
}
