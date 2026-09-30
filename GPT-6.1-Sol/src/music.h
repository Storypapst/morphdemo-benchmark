#ifndef MUSIC_H
#define MUSIC_H
#include <math.h>
#include <stdatomic.h>

#define RATE 48000
#define END_SAMPLE (64 * RATE)
static _Atomic unsigned sample_cursor;
static float frequencies[256], roots[32];
static float echo[65536][2];
static unsigned echo_pos, noise_state=0x184793;
static float noise_previous;

static float clamp01(float x) { return fminf(1, fmaxf(0,x)); }
static float fade(float a,float b,float x) { return clamp01((x-a)/(b-a)); }
static float hz(int note) { return 440 * powf(2,(note-69)/12.f); }

static void prepare_music(void) {
    /* Two original, related scores. The last eight bars resolve to the tonic.
     * Integer sample scheduling keeps every beat exact. */
#if DEMO == 64
    const int progression[8]={38,34,41,36,38,34,43,45};
    const int figure[16]={24,31,36,39,31,43,36,31,24,34,36,43,39,36,31,34};
#else
    const int progression[8]={42,38,45,40,42,47,38,40};
    const int figure[16]={24,36,31,27,38,31,36,43,24,31,34,38,43,36,31,27};
#endif
    for(int bar=0;bar<32;bar++) {
        int r=bar<24?progression[(bar/2)%8]:progression[0];
        roots[bar]=hz(r);
        for(int k=0;k<8;k++) {
            int n=figure[(bar*8+k)%16];
            if(bar>=24 && n%12==3) n++;
            frequencies[bar*8+k]=hz(r+n);
        }
    }
}

static void audio(void *user,uint8_t *bytes,int length) {
    (void)user;
    float *out=(float *)bytes;
    unsigned cursor=atomic_load_explicit(&sample_cursor,memory_order_relaxed);
    for(int j=0;j<length/8;j++,cursor++) {
        if(cursor>=END_SAMPLE) { out[2*j]=out[2*j+1]=0; continue; }
        float t=cursor/(float)RATE;
        unsigned step=cursor/12000,bar=cursor/96000;
        float a=(cursor%12000)/(float)RATE,b=(cursor%24000)/(float)RATE;
        float r=roots[bar], f=frequencies[step];
        float structure=fade(10,22,t),climax=fade(34,44,t);
        float finish=1-fade(59,64,t);
        float major=bar>=24?1.259921f:1.189207f;
        float padEnvelope=fade(0,4,t)*(0.75f+0.25f*sinf(t*.31f));
        float pad=(sinf(6.283185f*r*t)+.42f*sinf(6.283185f*r*1.498307f*t)
             +.35f*sinf(6.283185f*r*major*t))*.085f*padEnvelope;
        /* A struck, slightly inharmonic bell acquires a consonant bass and
         * a regular pulse as the image gathers into its final form. */
        float bell=(sinf(6.283185f*f*a)+.26f*sinf(6.283185f*f*2.003f*a)
             +.12f*sinf(6.283185f*f*3.97f*a))*expf(-a*(10-5*climax))
             *fade(0,.006f,a)*(.065f+.11f*structure);
        if(t<12 && step%4!=0) bell*=.12f;
        float kick=sinf(6.283185f*(43*b+2.4f*(1-expf(-b*38))))*expf(-b*12)
             *.26f*structure*(1-fade(56,60,t));
        noise_state=noise_state*1664525u+1013904223u;
        float noise=(int32_t)noise_state/2147483648.f;
        float high=noise-noise_previous;
        noise_previous=noise;
        float hat=high*expf(-a*90)*.045f*structure;
        float snare=(step/2)%2 ? noise*expf(-b*22)*.11f*climax : 0;
        float bass=sinf(6.283185f*r*t)*.09f*structure*(.7f+.3f*expf(-b*8));
#if DEMO == 16
        bell+=sinf(6.283185f*f*a*.5f+.6f*sinf(6.283185f*f*a))
             *expf(-a*16)*.06f*climax;
        pad*=.72f;
#endif
        float grain=noise*.012f*(1-structure)*fade(0,3,t);
        float pan=.45f*sinf(step*2.399963f);
        float dry=pad+bass+kick+hat+snare+grain;
        float dl=dry+bell*(1-pan),dr=dry+bell*(1+pan);
        unsigned read1=(echo_pos-18000)&65535,read2=(echo_pos-24000)&65535;
        float l=dl+.28f*echo[read1][1]+.16f*echo[read2][0];
        float rr=dr+.28f*echo[read1][0]+.16f*echo[read2][1];
        echo[echo_pos][0]=l; echo[echo_pos][1]=rr;
        echo_pos=(echo_pos+1)&65535;
        /* A smooth saturator and a four-second coda prevent clipping/clicks. */
        out[2*j]=l/(1+fabsf(l))*.82f*finish;
        out[2*j+1]=rr/(1+fabsf(rr))*.82f*finish;
    }
    atomic_store_explicit(&sample_cursor,cursor,memory_order_release);
}
#endif
