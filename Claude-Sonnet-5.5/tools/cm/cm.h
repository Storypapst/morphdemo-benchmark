// "cm4": integer-only context-mixing model shared by the encoder and the reference decoder.
// Every operation here has an exact counterpart in the x86-64 decoder stub (tools/cm/dec4.inc).
//
// Model: NM contexts, each a hash of the previous bytes selected by an 8-bit mask, combined with the bits of the
// current byte. Each context indexes a 32-bit slot holding two counters (n0 in bits 0..7, n1 in bits 16..23).
// The prediction is  p0 = S0 / (S0 + S1)  with  S0 = a0 + sum(w_i * n0_i),  S1 = a0 + sum(w_i * n1_i).
// Both sums live in one 32-bit accumulator (low and high half), which is why sum(w_i) * cap + a0 must stay below 65536.
#ifndef CM_H
#define CM_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <nmmintrin.h>

#define CM_MAXM 16
typedef struct {
  int nm;                    // number of models (1..16)
  uint8_t mask[CM_MAXM];     // which of the previous 8 bytes each model sees
  uint8_t wt[CM_MAXM];       // integer weight of each model
  int a0;                    // pseudo count added to both sums
  int cap;                   // saturation of the two counters (<=255)
  int ns;                    // an opposite counter above this value is halved
  int tbits;                 // log2 of the number of 32-bit slots
} CMParams;

static inline int cm_valid(const CMParams *P){
  int sw=0; for(int i=0;i<P->nm;i++) sw+=P->wt[i];
  return P->nm>=1 && P->nm<=CM_MAXM && P->a0>=1 && P->cap>=1 && P->cap<=255 && (int64_t)sw*P->cap+P->a0<=65535;
}

typedef struct {
  const CMParams *P;
  uint32_t *T;
  uint64_t hist;
  uint32_t h[CM_MAXM];
  uint32_t idx[CM_MAXM];
  uint32_t c0;
} CMState;

static inline uint64_t cm_expand(uint8_t m){ uint64_t r=0; for(int b=0;b<8;b++) if(m>>b&1) r|=0xffULL<<(8*b); return r; }
static inline void cm_init(CMState *S,const CMParams *P){ S->P=P; S->T=calloc((size_t)1<<P->tbits,4); S->hist=0; }
static inline void cm_free(CMState *S){ free(S->T); }

static inline void cm_byte_start(CMState *S){
  const CMParams *P=S->P;
  for(int i=0;i<P->nm;i++) S->h[i]=(uint32_t)_mm_crc32_u64((uint64_t)(i+1),S->hist&cm_expand(P->mask[i]));
  S->c0=1;
}
// probability (16 bit) that the next bit is 0
static inline uint32_t cm_predict(CMState *S){
  const CMParams *P=S->P;
  uint32_t acc=(uint32_t)P->a0*65537u, msk=((uint32_t)1<<P->tbits)-1;
  for(int i=0;i<P->nm;i++){
    uint32_t ix=_mm_crc32_u8(S->h[i],(uint8_t)S->c0)&msk; S->idx[i]=ix;
    acc+=S->T[ix]*P->wt[i];
  }
  uint32_t s0=acc&0xffff,s1=acc>>16;
  return (s0<<16)/(s0+s1);
}
static inline void cm_update(CMState *S,int y){
  const CMParams *P=S->P;
  for(int i=0;i<P->nm;i++){
    uint8_t *b=(uint8_t*)&S->T[S->idx[i]];
    uint8_t *ny=b+2*y,*no=b+2*(y^1);
    if(*ny<P->cap) (*ny)++;
    if(*no>P->ns) *no>>=1;
  }
  S->c0=S->c0*2+y;
}
static inline void cm_byte_end(CMState *S){ S->hist=(S->hist<<8)|(S->c0&255); }

static double cm_cost(const CMParams *P,const uint8_t *buf,int n){
  CMState S; cm_init(&S,P); double bits=0;
  for(int i=0;i<n;i++){
    cm_byte_start(&S);
    for(int b=7;b>=0;b--){
      int y=buf[i]>>b&1; uint32_t p0=cm_predict(&S);
      double q=(y?65536-p0:p0)/65536.0; bits-=log2(q);
      cm_update(&S,y);
    }
    cm_byte_end(&S);
  }
  cm_free(&S); return bits;
}

// ---- LZMA style range coder, 16 bit probabilities of a zero bit ----
typedef struct { uint64_t low; uint32_t range; uint8_t cache; uint64_t csize; uint8_t *out; int n; int first; } CMEnc;
static inline void cmenc_init(CMEnc *E,uint8_t *out){ E->low=0; E->range=0xffffffffu; E->cache=0; E->csize=1; E->out=out; E->n=0; E->first=1; }
static inline void cmenc_put(CMEnc *E,uint8_t b){ if(E->first){ E->first=0; return; } E->out[E->n++]=b; }  // leading byte is always 0: dropped
static inline void cmenc_shift(CMEnc *E){
  if((uint32_t)E->low<0xff000000u || (E->low>>32)!=0){
    uint8_t t=E->cache;
    do { cmenc_put(E,(uint8_t)(t+(uint8_t)(E->low>>32))); t=0xff; } while(--E->csize);
    E->cache=(uint8_t)(E->low>>24);
  }
  E->csize++; E->low=(E->low&0x00ffffffu)<<8;
}
static inline void cmenc_bit(CMEnc *E,uint32_t p0,int y){
  uint32_t bound=(E->range>>16)*p0;
  if(!y) E->range=bound; else { E->low+=bound; E->range-=bound; }
  while(E->range<(1u<<24)){ E->range<<=8; cmenc_shift(E); }
}
// finish: pick the value with the most trailing zero BITS in [low, low+range) such that every completion of the dropped bytes
// (whatever bytes follow the stream) stays inside the interval, emit it, and drop exactly the bytes that are free.
// The decoder of the 4k is run with the stream at the very end of the file, followed by unrelated memory, so this must not rely on zeros.
static inline void cmenc_flush(CMEnc *E){
  uint64_t lo=E->low, hi=E->low+E->range-1, v=lo; int drop=0;
  for(int k=32;k>=8;k-=8){ uint64_t m=((uint64_t)1<<k)-1; uint64_t c=(lo+m)&~m; if(c+m<=hi){ v=c; drop=k/8; break; } }
  E->low=v;
  for(int i=0;i<5;i++) cmenc_shift(E);
  E->n-=drop;
}
typedef struct { uint32_t range,code; const uint8_t *in; } CMDec;
static inline void cmdec_init(CMDec *D,const uint8_t *in){ D->range=0xffffffffu; D->in=in; D->code=0; for(int i=0;i<4;i++) D->code=(D->code<<8)|*D->in++; }
static inline int cmdec_bit(CMDec *D,uint32_t p0){
  uint32_t bound=(D->range>>16)*p0; int y;
  if(D->code<bound){ D->range=bound; y=0; } else { D->range-=bound; D->code-=bound; y=1; }
  while(D->range<(1u<<24)){ D->range<<=8; D->code=(D->code<<8)|*D->in++; }
  return y;
}

static int cm_compress(const CMParams *P,const uint8_t *buf,int n,uint8_t *out){
  CMState S; cm_init(&S,P); CMEnc E; cmenc_init(&E,out);
  for(int i=0;i<n;i++){
    cm_byte_start(&S);
    for(int b=7;b>=0;b--){ int y=buf[i]>>b&1; uint32_t p0=cm_predict(&S); cmenc_bit(&E,p0,y); cm_update(&S,y); }
    cm_byte_end(&S);
  }
  cmenc_flush(&E); cm_free(&S); return E.n;
}
// `in` must be followed by zero bytes
static void cm_decompress(const CMParams *P,const uint8_t *in,int n,uint8_t *out){
  CMState S; cm_init(&S,P); CMDec D; cmdec_init(&D,in);
  for(int i=0;i<n;i++){
    cm_byte_start(&S);
    for(int b=7;b>=0;b--){ uint32_t p0=cm_predict(&S); int y=cmdec_bit(&D,p0); cm_update(&S,y); }
    cm_byte_end(&S); out[i]=S.c0&255;
  }
  cm_free(&S);
}
#endif
