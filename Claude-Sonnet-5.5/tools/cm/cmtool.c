// cmtool: compress / verify / optimise with the cm4 model.
//   cmtool opt  in.bin params.txt [iterations] [seed]   optimise parameters, write params file
//   cmtool enc  in.bin params.txt out.cm                compress
//   cmtool dec  in.cm params.txt n out.bin              decompress (reference decoder)
//   cmtool cost in.bin params.txt                       print ideal and real size
#include <stdio.h>
#include "cm.h"

static void save(const char *fn,const CMParams *P){
  FILE *f=fopen(fn,"w"); fprintf(f,"nm %d\na0 %d\ncap %d\nns %d\ntbits %d\nmask",P->nm,P->a0,P->cap,P->ns,P->tbits);
  for(int i=0;i<P->nm;i++) fprintf(f," %d",P->mask[i]);
  fprintf(f,"\nwt"); for(int i=0;i<P->nm;i++) fprintf(f," %d",P->wt[i]); fprintf(f,"\n"); fclose(f);
}
static int load(const char *fn,CMParams *P){
  FILE *f=fopen(fn,"r"); if(!f) return 0; char k[32]; memset(P,0,sizeof *P);
  while(fscanf(f,"%31s",k)==1){
    if(!strcmp(k,"nm")) fscanf(f,"%d",&P->nm); else if(!strcmp(k,"a0")) fscanf(f,"%d",&P->a0); else if(!strcmp(k,"cap")) fscanf(f,"%d",&P->cap);
    else if(!strcmp(k,"ns")) fscanf(f,"%d",&P->ns); else if(!strcmp(k,"tbits")) fscanf(f,"%d",&P->tbits);
    else if(!strcmp(k,"mask")) for(int i=0;i<P->nm;i++){ int v; fscanf(f,"%d",&v); P->mask[i]=v; }
    else if(!strcmp(k,"wt")) for(int i=0;i<P->nm;i++){ int v; fscanf(f,"%d",&v); P->wt[i]=v; }
  }
  fclose(f); return 1;
}
static uint8_t *slurp(const char *fn,int *n){ FILE *f=fopen(fn,"rb"); if(!f){ perror(fn); exit(1);} fseek(f,0,SEEK_END); *n=ftell(f); fseek(f,0,SEEK_SET); uint8_t *b=calloc(*n+16,1); fread(b,1,*n,f); fclose(f); return b; }

static uint64_t rs=88172645463325252ULL;
static uint32_t rnd(void){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; return (uint32_t)(rs>>11); }

int main(int argc,char**argv){
  if(argc<3) return 1; int n; CMParams P;
  if(!strcmp(argv[1],"opt")){
    uint8_t *buf=slurp(argv[2],&n); int iters=argc>4?atoi(argv[4]):2000; if(argc>5) rs^=(uint64_t)atoi(argv[5])*0x9E3779B97F4A7C15ULL;
    if(!load(argv[3],&P)){ memset(&P,0,sizeof P); P.nm=8; uint8_t m[]={0,1,3,7,15,63,5,6}; uint8_t w[]={1,2,4,8,16,32,4,4}; memcpy(P.mask,m,8); memcpy(P.wt,w,8); P.a0=2; P.cap=60; P.ns=2; P.tbits=24; }
    if(P.tbits<16||P.tbits>26) P.tbits=24; if(!cm_valid(&P)){ fprintf(stderr,"invalid start params\n"); return 1; }
    double best=cm_cost(&P,buf,n); printf("start %.1f bytes\n",best/8);
    for(int it=0;it<iters;it++){
      CMParams Q=P; int r=rnd()%12; int i=rnd()%Q.nm;
      if(r>=10){   // add or remove a model (the extra cost of 2 header bytes per model is accounted for)
        if(r==10 && Q.nm<CM_MAXM){ Q.mask[Q.nm]=rnd()&255; Q.wt[Q.nm]=1+rnd()%8; Q.nm++; }
        else if(r==11 && Q.nm>3){ for(int k=i;k<Q.nm-1;k++){ Q.mask[k]=Q.mask[k+1]; Q.wt[k]=Q.wt[k+1]; } Q.nm--; }
        else continue;
        if(!cm_valid(&Q)) continue;
        double c=cm_cost(&Q,buf,n)+16.0*(Q.nm-P.nm);   // bits
        if(c<best){ best=c; P=Q; printf("it %d: %.1f bytes (nm=%d)\n",it,best/8,P.nm); fflush(stdout); save(argv[3],&P); }
        continue;
      }
      if(r<3) Q.wt[i]=(uint8_t)fmax(1,fmin(255,Q.wt[i]*(0.6+0.8*(rnd()%1000)/1000.0)+((int)(rnd()%3)-1)));
      else if(r<6) Q.mask[i]^=1<<(rnd()%8);
      else if(r<7) Q.mask[i]=rnd()&255;
      else if(r<8) Q.a0=fmax(1,Q.a0+(int)(rnd()%3)-1);
      else if(r<9) Q.cap=(int)fmax(4,fmin(255,Q.cap+(int)(rnd()%21)-10));
      else Q.ns=(int)fmax(0,fmin(20,Q.ns+(int)(rnd()%3)-1));
      if(!cm_valid(&Q)) continue;
      double c=cm_cost(&Q,buf,n);
      if(c<best){ best=c; P=Q; printf("it %d: %.1f bytes\n",it,best/8); fflush(stdout); save(argv[3],&P); }
    }
    save(argv[3],&P); printf("final %.1f bytes\n",best/8); return 0;
  }
  if(!strcmp(argv[1],"cost")||!strcmp(argv[1],"enc")){
    uint8_t *buf=slurp(argv[2],&n); if(!load(argv[3],&P)){ fprintf(stderr,"no params\n"); return 1; }
    uint8_t *out=calloc(n*2+64,1); int m=cm_compress(&P,buf,n,out);
    printf("%d -> %d bytes (ideal %.1f)\n",n,m,cm_cost(&P,buf,n)/8);
    if(!strcmp(argv[1],"enc")){ FILE *f=fopen(argv[4],"wb"); fwrite(out,1,m,f); fclose(f);
      uint8_t *chk=calloc(n+16,1); uint8_t *in=calloc(m+64,1);
      for(int trial=0;trial<3;trial++){   // the bytes after the stream are zeros, ones and pseudo random: all must decode identically
        memcpy(in,out,m); for(int k=0;k<64;k++) in[m+k]=trial==0?0:trial==1?0xff:(uint8_t)rnd();
        memset(chk,0,n+16); cm_decompress(&P,in,n,chk); if(memcmp(chk,buf,n)){ fprintf(stderr,"ROUNDTRIP FAILED (trailer variant %d)\n",trial); return 2; } }
      printf("roundtrip ok (with zero, 0xff and random bytes after the stream)\n"); }
    return 0;
  }
  if(!strcmp(argv[1],"prof")){   // prof in.bin params.txt out.txt : cumulative cost in bits before every byte
    uint8_t *buf=slurp(argv[2],&n); if(!load(argv[3],&P)) return 1;
    CMState S; cm_init(&S,&P); double bits=0; FILE *f=fopen(argv[4],"w");
    for(int i=0;i<n;i++){
      fprintf(f,"%d %.3f\n",i,bits);
      cm_byte_start(&S);
      for(int b=7;b>=0;b--){ int y=buf[i]>>b&1; uint32_t p0=cm_predict(&S); double q=(y?65536-p0:p0)/65536.0; bits-=log2(q); cm_update(&S,y); }
      cm_byte_end(&S);
    }
    fprintf(f,"%d %.3f\n",n,bits); fclose(f); cm_free(&S); return 0;
  }
  if(!strcmp(argv[1],"dec")){
    int m; uint8_t *in=slurp(argv[2],&m); if(!load(argv[3],&P)) return 1; int len=atoi(argv[4]); uint8_t *out=calloc(len+16,1);
    cm_decompress(&P,in,len,out); FILE *f=fopen(argv[5],"wb"); fwrite(out,1,len,f); fclose(f); return 0;
  }
  return 1;
}
