/*  Copyright (C) 2002-2004 Gabriel Maldonado */

/*  The gab library is free software; you can redistribute it */
/*  and/or modify it under the terms of the GNU Lesser General Public */
/*  License as published by the Free Software Foundation; either */
/*  version 2.1 of the License, or (at your option) any later version. */

/*  The gab library is distributed in the hope that it will be useful, */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/*  GNU Lesser General Public License for more details. */

/*  You should have received a copy of the GNU Lesser General Public */
/*  License along with the gab library; if not, write to the Free Software */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA */
/*  02111-1307 USA */

/* Ported to csound5 by: Andres Cabrera */
/* This file includes the opcodes from newopcodes.c */
/* TODO: Check if the authors (Peter Neubaeker and Jens Groh) are correct */

/*printi removed? I can't find the corresponding OENTRY- Left them commented out */
/*how should exitnow be safely implemented? */
/*Check the zak opcodes */
/*changed some comments to c-style */
/*Check csystem, exitnow */
/*Check other opcodes commented out in Oentry */

#include "csdl.h"
#include "gab.h"
#include <math.h>

static int krsnsetx(CSOUND *csound, KRESONX *p)
  /* Gabriel Maldonado, modifies for arb order  */
{
    int scale;
    p->scale = scale = (int) *p->iscl;
    if ((p->loop = (int) (*p->ord + FL(0.5))) < 1)
      p->loop = 4; /*default value*/
    if (!*p->istor && (p->aux.auxp == NULL ||
                      (int)(p->loop*2*sizeof(MYFLT)) > p->aux.size))
      csound->AuxAlloc(csound, (long)(p->loop*2*sizeof(MYFLT)), &p->aux);
    p->yt1 = (MYFLT*)p->aux.auxp; p->yt2 = (MYFLT*)p->aux.auxp + p->loop;
    if (scale && scale != 1 && scale != 2) {
      return csound->InitError(csound,"illegal reson iscl value, %f",*p->iscl);
    }
    p->prvcf = p->prvbw = -FL(100.0);

    if (!(*p->istor)) {
      int j;
      for (j=0; j< p->loop; j++) p->yt1[j] = p->yt2[j] = FL(0.0);
    }
    return OK;
}

static int kresonx(CSOUND *csound, KRESONX *p) /* Gabriel Maldonado, modified */
{
    int flag = 0, j;
    MYFLT       *ar, *asig;
    MYFLT       c3p1, c3t4, omc3, c2sqr;
    MYFLT *yt1, *yt2, c1,c2,c3;

    if (*p->kcf != p->prvcf) {
      p->prvcf = *p->kcf;
      p->cosf = (MYFLT) cos((double)(*p->kcf * csound->tpidsr * csound->ksmps));
      flag = 1;
    }
    if (*p->kbw != p->prvbw) {
      p->prvbw = *p->kbw;
      p->c3 = (MYFLT) exp((double)(*p->kbw * csound->mtpdsr * csound->ksmps));
      flag = 1;
    }
    if (flag) {
      c3p1 = p->c3 + FL(1.0);
      c3t4 = p->c3 * FL(4.0);
      omc3 = FL(1.0)- p->c3;
      p->c2 = c3t4 * p->cosf / c3p1;            /* -B, so + below */
      c2sqr = p->c2 * p->c2;
      if (p->scale == 1)
        p->c1 = omc3 * (MYFLT)sqrt(1.0 - (double)(c2sqr / c3t4));
      else if (p->scale == 2)
        p->c1 = (MYFLT)sqrt((double)((c3p1*c3p1-c2sqr) * omc3/c3p1));
      else p->c1 = FL(1.0);
    }
    c1   = p->c1;
    c2   = p->c2;
    c3   = p->c3;
    yt1  = p->yt1;
    yt2  = p->yt2;
    asig = p->asig;
    ar   = p->ar;
    for (j=0; j< p->loop; j++) {
      *ar = c1 * *asig + c2 * *yt1 - c3 * *yt2;
      *yt2 = *yt1;
      *yt1 = *ar;
      yt1++;
      yt2++;
      asig= p->ar;
    }
    return OK;
}

/* /////////////////////////////////////////// */

static int fastab_set(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "fastab: incorrect table number");
    }
    p->table = ftp->ftable;
    p->xmode = (int) *p->ixmode;
    if (p->xmode)
      p->xbmul = (MYFLT) ftp->flen;
    else
      p->xbmul = FL(1.0);
    return OK;
}

static int fastabw(CSOUND *csound, FASTAB *p)
{
    int nsmps = csound->ksmps, n;
    MYFLT *tab = p->table;
    MYFLT *rslt = p->rslt, *ndx = p->xndx;
    if (p->xmode) {
      MYFLT xbmul = p->xbmul;   /* load once */
      for (n=0; n<nsmps; n++)   /* for loops compile better */
        tab[(int)(ndx[n]*xbmul)] = rslt[n];
/*       do *(tab + (long) (*ndx++ * p->xbmul)) = *rslt++; */
/*       while(--nsmps); */
    }
    else {
      for (n=0; n<nsmps; n++)
        tab[(int)ndx[n]] = rslt[n];
/*       do *(tab + (long) *ndx++) = *rslt++; */
/*       while(--nsmps); */
    }
    return OK;
}

static int fastabk(CSOUND *csound, FASTAB *p)
{
    if (p->xmode)
      *p->rslt =  *(p->table + (int) (*p->xndx * p->xbmul));
    else
      *p->rslt =  *(p->table + (int) *p->xndx);
    return OK;
}

static int fastabkw(CSOUND *csound, FASTAB *p)
{
    if (p->xmode)
      *(p->table + (int) (*p->xndx * p->xbmul)) = *p->rslt;
    else
      *(p->table + (int) *p->xndx) = *p->rslt;
    return OK;
}

static int fastabi(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    /*ftp = csound->FTFind(p->xfn); */

    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "tab_i: incorrect table number");
    }
    if (*p->ixmode)
      *p->rslt =  *(ftp->ftable + (int) (*p->xndx * ftp->flen));
    else
      *p->rslt =  *(ftp->ftable + (int) *p->xndx);
    return OK;
}

static int fastabiw(CSOUND *csound, FASTAB *p)
{
    FUNC *ftp;
    /*ftp = csound->FTFind(p->xfn); */
    if ((ftp = csound->FTnp2Find(csound, p->xfn)) == NULL) {
      return csound->InitError(csound, "tabw_i: incorrect table number");
    }
    if (*p->ixmode)
      *(ftp->ftable + (int) (*p->xndx * ftp->flen)) = *p->rslt;
    else
      *(ftp->ftable + (int) *p->xndx) = *p->rslt;
    return OK;
}

static int fastab(CSOUND *csound, FASTAB *p)
{
    int   i = 0, nsmps = csound->ksmps;
    MYFLT *tab = p->table;
    MYFLT *rslt = p->rslt, *ndx = p->xndx;
    if (p->xmode) {
      MYFLT xbmul = p->xbmul;
      do {
        rslt[i] = tab[(int) (ndx[i] * xbmul)];
      } while (++i < nsmps);
    }
    else {
      do {
        rslt[i] = tab[(int) ndx[i]];
      } while (++i < nsmps);
    }
    return OK;
}

static CS_NOINLINE int tab_init(CSOUND *csound, TB_INIT *p, int ndx)
{
    MYFLT             *ft;
    STDOPCOD_GLOBALS  *pp;
    if (csound->GetTable(csound, &ft, (int) *(p->ifn)) < 0)
      return csound->InitError(csound, Str("tab_init: incorrect table number"));
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    pp->tb_ptrs[ndx] = ft;
    return OK;
}

static CS_NOINLINE int tab_perf(CSOUND *csound, FASTB *p)
{
    *p->r = (*p->tb_ptr)[(int) *p->ndx];
    return OK;
}

static CS_NOINLINE int tab_i_tmp(CSOUND *csound, FASTB *p, int ndx)
{
    STDOPCOD_GLOBALS  *pp;
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    p->tb_ptr = &(pp->tb_ptrs[ndx]);
    p->h.iopadr = (SUBR) tab_perf;
    return tab_perf(csound, p);
}

static CS_NOINLINE int tab_k_tmp(CSOUND *csound, FASTB *p, int ndx)
{
    STDOPCOD_GLOBALS  *pp;
    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    p->tb_ptr = &(pp->tb_ptrs[ndx]);
    p->h.opadr = (SUBR) tab_perf;
    return tab_perf(csound, p);
}

#ifdef TAB_MACRO
#undef TAB_MACRO
#endif

#define TAB_MACRO(W,X,Y,Z)                  \
static int W(CSOUND *csound, TB_INIT *p)    \
{   return tab_init(csound, p, Z);  }       \
static int X(CSOUND *csound, FASTB *p)      \
{   return tab_i_tmp(csound, p, Z); }       \
static int Y(CSOUND *csound, FASTB *p)      \
{   return tab_k_tmp(csound, p, Z); }

TAB_MACRO(tab0_init, tab0_i_tmp, tab0_k_tmp, 0)
TAB_MACRO(tab1_init, tab1_i_tmp, tab1_k_tmp, 1)
TAB_MACRO(tab2_init, tab2_i_tmp, tab2_k_tmp, 2)
TAB_MACRO(tab3_init, tab3_i_tmp, tab3_k_tmp, 3)
TAB_MACRO(tab4_init, tab4_i_tmp, tab4_k_tmp, 4)
TAB_MACRO(tab5_init, tab5_i_tmp, tab5_k_tmp, 5)
TAB_MACRO(tab6_init, tab6_i_tmp, tab6_k_tmp, 6)
TAB_MACRO(tab7_init, tab7_i_tmp, tab7_k_tmp, 7)
TAB_MACRO(tab8_init, tab8_i_tmp, tab8_k_tmp, 8)
TAB_MACRO(tab9_init, tab9_i_tmp, tab9_k_tmp, 9)
TAB_MACRO(tab10_init, tab10_i_tmp, tab10_k_tmp, 10)
TAB_MACRO(tab11_init, tab11_i_tmp, tab11_k_tmp, 11)
TAB_MACRO(tab12_init, tab12_i_tmp, tab12_k_tmp, 12)
TAB_MACRO(tab13_init, tab13_i_tmp, tab13_k_tmp, 13)
TAB_MACRO(tab14_init, tab14_i_tmp, tab14_k_tmp, 14)
TAB_MACRO(tab15_init, tab15_i_tmp, tab15_k_tmp, 15)

/************************************************************* */
/* Opcodes from Peter Neubaeker                                */
/* *********************************************************** */

#if 0
static void printi(CSOUND *csound, PRINTI *p)
{
    char    *sarg;

    if ((*p->ifilcod != sstrcod) || (*p->STRARG == 0)) {
      csound->InitError(csound, "printi parameter was not a \"quoted string\"");
      return NOTOK;
    }
    else {
      sarg = p->STRARG;
      do {
        putchar(*sarg);
      } while (*++sarg != 0);
      putchar(10);
      putchar(13);
    }
    return OK;
}
#endif

/* ====================== */
/* opcodes from Jens Groh */
/* ====================== */

static int nlalp_set(CSOUND *csound,NLALP *p)
{
   if (!(*p->istor)) {
     p->m0 = 0.0;
     p->m1 = 0.0;
   }
   return OK;
}

static int nlalp(CSOUND *csound,NLALP *p)
{
   int nsmps;
   MYFLT *rp;
   MYFLT *ip;
   double m0;
   double m1;
   double tm0;
   double tm1;
   double klfact;
   double knfact;

   nsmps = csound->ksmps;
   rp = p->aresult;
   ip = p->ainsig;
   klfact = (double)*p->klfact;
   knfact = (double)*p->knfact;
   tm0 = p->m0;
   tm1 = p->m1;
   if (knfact == 0.0) { /* linear case */
     if (klfact == 0.0) { /* degenerated linear case */
       m0 = (double)*ip++ - tm1;
       *rp++ = (MYFLT)(tm0);
       while (--nsmps) {
         *rp++ = (MYFLT)(m0);
         m0 = (double)*ip++;
       }
       tm0 = m0;
       tm1 = 0.0;
     } else { /* normal linear case */
       do {
         m0 = (double)*ip++ - tm1;
         m1 = m0 * klfact;
         *rp++ = (MYFLT)(tm0 + m1);
         tm0 = m0;
         tm1 = m1;
       } while (--nsmps);
     }
   } else { /* non-linear case */
     if (klfact == 0.0) { /* simplified non-linear case */
       do {
         m0 = (double)*ip++ - tm1;
         m1 = fabs(m0) * knfact;
         *rp++ = (MYFLT)(tm0 + m1);
         tm0 = m0;
         tm1 = m1;
       } while (--nsmps);
     } else { /* normal non-linear case */
       do {
         m0 = (double)*ip++ - tm1;
         m1 = m0 * klfact + fabs(m0) * knfact;
         *rp++ = (MYFLT)(tm0 + m1);
         tm0 = m0;
         tm1 = m1;
       } while (--nsmps);
     }
   }
   p->m0 = tm0;
   p->m1 = tm1;
   return OK;
}

/* ----------------------------------------------- */

static int adsynt2_set(CSOUND *csound,ADSYNT2 *p)
{
    FUNC    *ftp;
    MYFLT fmaxlen = (MYFLT)MAXLEN;
    int     count;
    long    *lphs;
    MYFLT       *pAmp;
    p->inerr = 0;

    if ((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
    }
    else {
      p->inerr = 1;
      /*csound->InitError(csound, Str("adsynt: wavetable not found!")); */
      return csound->InitError(csound, "adsynt2: wavetable not found!");
    }

    count = (int)*p->icnt;
    if (count < 1)
      count = 1;
    p->count = count;

    if ((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL) {
      p->freqtp = ftp;
    }
    else {
      p->inerr = 1;
      /*csound->InitError(csound, Str("adsynt: freqtable not found!")); */
      return csound->InitError(csound, "adsynt: freqtable not found!");
    }
    if (ftp->flen < count) {
      p->inerr = 1;
/* csound->InitError(csound, Str(
             "adsynt: partial count is greater than freqtable size!")); */
      return csound->InitError(csound, "adsynt: partial count is greater than freqtable size!");
    }

    if ((ftp = csound->FTnp2Find(csound, p->ifn)) != NULL) {
      p->amptp = ftp;
    }
    else {
      p->inerr = 1;
      /*       csound->InitError(csound, Str("adsynt: amptable not found!")); */
      return csound->InitError(csound, "adsynt: amptable not found!");
    }
    if (ftp->flen < count) {
      p->inerr = 1;
      /* csound->InitError(csound, Str(
                   "adsynt: partial count is greater than amptable size!")); */
      return csound->InitError(csound, "adsynt: partial count is greater than amptable size!");
    }

    if (p->lphs.auxp==NULL ||
        p->lphs.size < (long)(sizeof(long)+sizeof(MYFLT))*count)
      csound->AuxAlloc(csound, (sizeof(long)+sizeof(MYFLT))*count, &p->lphs);

    lphs = (long*)p->lphs.auxp;
    if (*p->iphs > 1) {
      do {
        *lphs++ = ((long)
                   ((MYFLT) ((double) (csound->Rand31(&(csound->randSeed1)) - 1)
                             / 2147483645.0) * fmaxlen)) & PHMASK;
      } while (--count);
    }
    else if (*p->iphs >= 0) {
      do {
        *lphs++ = ((long)(*p->iphs * fmaxlen)) & PHMASK;
      } while (--count);
    }
    pAmp = p->previousAmp = (MYFLT *) lphs + sizeof(MYFLT)*count;
    count = (int)*p->icnt;
    do {
      *pAmp++ = FL(0.0);
    } while (--count);
    return OK;
}

static int adsynt2(CSOUND *csound,ADSYNT2 *p)
{
    FUNC    *ftp, *freqtp, *amptp;
    MYFLT   *ar, *ar0, *ftbl, *freqtbl, *amptbl, *prevAmp;
    MYFLT   amp0, amp, cps0, cps, ampIncr,amp2;
    long    phs, inc, lobits;
    long    *lphs;
    int     nsmps, count;

    if (p->inerr) {
      /*csound->InitError(csound, Str("adsynt: not initialized")); */
      return csound->InitError(csound, "adsynt: not initialized");
    }
    ftp = p->ftp;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    freqtp = p->freqtp;
    freqtbl = freqtp->ftable;
    amptp = p->amptp;
    amptbl = amptp->ftable;
    lphs = (long*)p->lphs.auxp;
    prevAmp = p->previousAmp;

    cps0 = *p->kcps;
    amp0 = *p->kamp;
    count = p->count;

    ar0 = p->sr;
    ar = ar0;
    nsmps = csound->ksmps;
    do {
      *ar++ = FL(0.0);
    } while (--nsmps);

    do {
      ar = ar0;
      nsmps = csound->ksmps;
      amp2 = *prevAmp;
      amp = *amptbl++ * amp0;
      cps = *freqtbl++ * cps0;
      inc = (long) (cps * csound->sicvt);
      phs = *lphs;
      ampIncr = (amp - *prevAmp) * csound->onedksmps;
      do {
        *ar++ += *(ftbl + (phs >> lobits)) * amp2;
        phs += inc;
        phs &= PHMASK;
        amp2 += ampIncr;
      }
      while (--nsmps);
      *prevAmp++ = amp;
      *lphs++ = phs;
    }
    while (--count);
    return OK;
}

static int exitnow(CSOUND *csound, EXITNOW *p)
{
    (void) p;
    csound->LongJmp(csound, 0);
    return OK;  /* compiler only */
}

/*-----zak opcodes */
/* int zread(CSOUND *csound,ZKR *p) */
/* { */
/*      *p->rslt = zkstart[(long) *p->ndx]; */
/*      return OK; */
/* } */

/*void a_k_set(INDIFF *p)
{
        p->prev = FL(0.0);
}*/

static int tabrec_set(CSOUND *csound,TABREC *p)
{
    /*FUNC *ftp; */
    /*if ((ftp = csound->FTFind(p->ifn)) == NULL) { */
    /*  csound->InitError(csound, Str("tabrec: incorrect table number")); */
    /*  return; */
    /*} */
    /*p->table = ftp->ftable; */
    /*p->tablen = ftp->flen; */
    p->recording = 0;
    p->currtic = 0;
    p->ndx = 0;
    p->numins = p->INOCOUNT-4;
    return OK;
}

static int tabrec_k(CSOUND *csound,TABREC *p)
{
    if (*p->ktrig_start) {
      if (*p->kfn != p->old_fn) {
        int flen;
        if ((flen = csound->GetTable(csound, &(p->table), (int) *p->kfn)) < 0)
          return csound->PerfError(csound, "Invalid ftable no. %f", *p->kfn);
        p->tablen = (long) flen;
        *(p->table++) = *p->numtics;
        p->old_fn = *p->kfn;
      }
      p->recording = 1;
      p->ndx = 0;
      p->currtic = 0;
    }
    if (*p->ktrig_stop) {

      if (p->currtic >= *p->numtics) {
        p->recording = 0;
        return OK;
      }
      p->currtic++;
    }
    if (p->recording) {
      int j, curr_frame = p->ndx * p->numins;

      MYFLT *table = p->table;
      MYFLT **inargs = p->inargs;
      if (curr_frame + p->numins < p->tablen) {
        /* record only if table is not full */
        for (j = 0; j < p->numins; j++)
          table[curr_frame + j] = *inargs[j];
      }
      (p->ndx)++;
    }
    return OK;
}
/*-------------------------*/
static int tabplay_set(CSOUND *csound,TABPLAY *p)
{
    /*   FUNC *ftp; */
    /* if ((ftp = csound->FTFind(p->ifn)) == NULL) { */
    /*   csound->InitError(csound, Str("tabplay: incorrect table number")); */
    /*   return; */
    /* } */
    /*  p->table = ftp->ftable; */
    /*  p->tablen = ftp->flen; */
    p->playing = 0;
    p->currtic = 0;
    p->ndx = 0;
    p->numouts = p->INOCOUNT-3;
    return OK;
}

static int tabplay_k(CSOUND *csound,TABPLAY *p)
{
    if (*p->ktrig) {
      if (*p->kfn != p->old_fn) {
        int flen;
        if ((flen = csound->GetTable(csound, &(p->table), (int) *p->kfn)) < 0)
          return csound->PerfError(csound, "Invalid ftable no. %f", *p->kfn);
        p->tablen = (long) flen;
        p->currtic = 0;
        p->ndx = 0;
        *(p->table++) = *p->numtics;
        p->old_fn = *p->kfn;
      }
      p->playing = 1;
      if (p->currtic == 0)
        p->ndx = 0;
      if (p->currtic >= *p->numtics) {
        p->playing = 0;
        return OK;
      }
      p->currtic++;
      p->currtic %= (long) *p->numtics;

    }
    if (p->playing) {
      int j, curr_frame = p->ndx * p->numouts;
      MYFLT *table = p->table;
      MYFLT **outargs = p->outargs;
      if (curr_frame + p->numouts < p->tablen) { /* play only if ndx is inside table */
        /*for (j = p->ndx* p->numouts; j < end; j++) */
        /*      *outargs[j] = table[j]; */

        for (j = 0; j < p->numouts; j++)
          *outargs[j] = table[curr_frame+j];
      }
      (p->ndx)++;
    }
    return OK;
}

static int isChanged_set(CSOUND *csound,ISCHANGED *p)
{
    p->numargs = p->INOCOUNT;
    return OK;
}

static int isChanged(CSOUND *csound,ISCHANGED *p)
{
    MYFLT **inargs = p->inargs;
    MYFLT *old_inargs = p->old_inargs;
    int numargs = p->numargs, ktrig = 0, j;

    for (j =0; j< numargs; j++) {
      if (*inargs[j] != old_inargs[j]) {
        ktrig = 1;
        break;
      }
    }

    if (ktrig) {
      for (j =0; j< numargs; j++) {
        old_inargs[j] = *inargs[j];
      }
    }
    *p->ktrig = (MYFLT) ktrig;
    return OK;
}

/* ------------------------- */

static int partial_maximum_set(CSOUND *csound,P_MAXIMUM *p)
{
    p->max = 0;
    p->counter = 0;
    return OK;
}

static int partial_maximum(CSOUND *csound,P_MAXIMUM *p)
{
    int n = csound->ksmps, flag = (int) *p->imaxflag;
    MYFLT *a = p->asig;
    MYFLT max = p->max;
    switch(flag) {
    case 0: /* absolute maximum */
      do {
        MYFLT temp;
        if ((temp = (MYFLT) fabs(*a++)) > max) max = temp;
      } while (--n);
      if (max > p->max) p->max = max;
      break;
    case 1: /* actual maximum */
      do {
        if (*a > max) max = *a;
        ++a;
      } while (--n);
      if (max > p->max) p->max = max;
      break;
    case 2: /* actual minimum */
      do {
        if (*a < max) max = *a;
        ++a;
      } while (--n);
      if (max < p->max) p->max = max;
      break;
    case 3: { /* average */
        MYFLT temp = FL(0.0);
        p->counter += n;
        do {
          temp += *a++;
        } while (--n);
        p->max += temp;
      }
      break;
    default:
      return csound->PerfError(csound, "max_k: invalid imaxflag value");
    }
    if (*p->ktrig) {
      if (flag == 3) {
        *p->kout = p->max / (MYFLT) p->counter;
        p->counter = 0;
      }
      else *p->kout = p->max;
      p->max = FL(0.0);
    }
    return OK;
}

/* From fractals.c */
/* mandelbrot set scanner  */
static int mandel_set(CSOUND *csound,MANDEL *p)
{
    p->oldx=-99999; /*probably unused values  */
    p->oldy=-99999;
    p->oldCount = -1;
    return OK;
}

static int mandel(CSOUND *csound,MANDEL *p)
{
    MYFLT px=*p->kx, py=*p->ky;
    if (*p->ktrig && (px != p->oldx || py != p->oldy)) {
      int maxIter = (int) *p->kmaxIter, j;
      MYFLT x=FL(0.0), y=FL(0.0), newx, newy;
      for (j=0; j<maxIter; j++) {
        newx = x*x - y*y + px;
        newy = FL(2.0)*x*y + py;
        x=newx;
        y=newy;
        if (x*x+y*y >= FL(4.0)) break;
      }
      p->oldx = px;
      p->oldy = py;
      if (p->oldCount != j) *p->koutrig = FL(1.0);
      else *p->koutrig = FL(0.0);
      *p->kr = (MYFLT) (p->oldCount = j);
    }
    else {
      *p->kr = (MYFLT) p->oldCount;
      *p->koutrig = FL(0.0);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"resonxk", S(KRESONX),    3,   "k",    "kkkooo",
                            (SUBR) krsnsetx, (SUBR) kresonx, NULL },
  { "tab_i",S(FASTAB),       1,   "i",    "iio", (SUBR) fastabi, NULL, NULL },
  { "tab",S(FASTAB),         7,   "s",    "xio",
                            (SUBR) fastab_set, (SUBR)fastabk, (SUBR) fastab },
  { "tabw_i",S(FASTAB),      1,   "",    "iiio", (SUBR) fastabiw, NULL, NULL },
  { "tabw",S(FASTAB),        7,   "",    "xxio",
                            (SUBR)fastab_set, (SUBR)fastabkw, (SUBR)fastabw },
  { "tb0_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab0_init},
  { "tb1_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab1_init},
  { "tb2_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab2_init},
  { "tb3_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab3_init},
  { "tb4_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab4_init},
  { "tb5_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab5_init},
  { "tb6_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab6_init},
  { "tb7_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab7_init},
  { "tb8_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab8_init},
  { "tb9_init", S(TB_INIT),  1,   "",      "i",    (SUBR)tab9_init},
  { "tb10_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab10_init},
  { "tb11_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab11_init},
  { "tb12_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab12_init},
  { "tb13_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab13_init},
  { "tb14_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab14_init},
  { "tb15_init", S(TB_INIT), 1,   "",      "i",    (SUBR)tab15_init},
  { "tb0.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab0_k_tmp,  NULL  },
  { "tb1.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab1_k_tmp,  NULL  },
  { "tb2.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab2_k_tmp,  NULL  },
  { "tb3.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab3_k_tmp,  NULL  },
  { "tb4.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab4_k_tmp,  NULL  },
  { "tb5.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab5_k_tmp,  NULL  },
  { "tb6.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab6_k_tmp,  NULL  },
  { "tb7.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab7_k_tmp,  NULL  },
  { "tb8.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab8_k_tmp,  NULL  },
  { "tb9.k",  S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab9_k_tmp,  NULL  },
  { "tb10.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab10_k_tmp, NULL  },
  { "tb11.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab11_k_tmp, NULL  },
  { "tb12.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab12_k_tmp, NULL  },
  { "tb13.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab13_k_tmp, NULL  },
  { "tb14.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab14_k_tmp, NULL  },
  { "tb15.k", S(FASTB), 2,  "k",    "k",    NULL, (SUBR) tab15_k_tmp, NULL  },
  /* tbx_t (t-rate version removed here) */
  { "tb0.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab0_i_tmp    },
  { "tb1.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab1_i_tmp    },
  { "tb2.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab2_i_tmp    },
  { "tb3.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab3_i_tmp    },
  { "tb4.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab4_i_tmp    },
  { "tb5.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab5_i_tmp    },
  { "tb6.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab6_i_tmp    },
  { "tb7.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab7_i_tmp    },
  { "tb8.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab8_i_tmp    },
  { "tb9.i",      S(FASTB), 1,    "i",     "i",    (SUBR) tab9_i_tmp    },
  { "tb10.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab10_i_tmp   },
  { "tb11.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab11_i_tmp   },
  { "tb12.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab12_i_tmp   },
  { "tb13.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab13_i_tmp   },
  { "tb14.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab14_i_tmp   },
  { "tb15.i",     S(FASTB), 1,    "i",     "i",    (SUBR) tab15_i_tmp   },
  { "nlalp", S(NLALP),      5,    "a",    "akkoo",
                            (SUBR) nlalp_set, NULL, (SUBR) nlalp   },
  { "adsynt2",S(ADSYNT2),   5,    "a",  "kkiiiio",
                            (SUBR) adsynt2_set, NULL, (SUBR)adsynt2 },
  { "exitnow",S(EXITNOW),   1,    "",  "", (SUBR) exitnow, NULL, NULL },
/* { "zr_i",  S(ZKR),     1,  "i",  "i",  (SUBR)zread, NULL, NULL}, */
/* { "zr_k",  S(ZKR),     2,  "k",  "k",  NULL, (SUBR)zread, NULL}, */
/* { "zr_a",  S(ZAR),     5,  "a",  "a",  (SUBR)zaset, NULL, (SUBR)zar}, */
/* { "k_i",   S(ASSIGN),  1,  "k",  "i",  (SUBR)assign}, */
/* { "k_t",   S(ASSIGN),  2,  "k",  "t",  NULL, (SUBR)assign}, */
/* { "a_k",   S(INDIFF),  5,  "a",  "k",  (SUBR)a_k_set,NULL, (SUBR)interp }, */
  { "tabrec",   S(TABREC),  3,     "",      "kkkkz",
                            (SUBR) tabrec_set, (SUBR) tabrec_k, NULL },
  { "tabplay",  S(TABPLAY), 3,     "",      "kkkz",
                            (SUBR) tabplay_set, (SUBR) tabplay_k, NULL },
  { "changed", S(ISCHANGED), 3,     "k",     "z",
                            (SUBR) isChanged_set, (SUBR)isChanged, NULL },
  /*{ "ftlen_k",S(EVAL),    2,      "k",    "k", NULL,      (SUBR)ftlen   }, */
  { "max_k",  S(P_MAXIMUM), 5,      "k",    "aki",
            (SUBR) partial_maximum_set, (SUBR) NULL, (SUBR) partial_maximum },
  { "maxk",   S(P_MAXIMUM), 5,      "k",    "aki",
            (SUBR) partial_maximum_set, (SUBR) NULL, (SUBR) partial_maximum },
  { "mandel",S(MANDEL),     3,      "kk",    "kkkk",
                            (SUBR) mandel_set, (SUBR) mandel, NULL }
};

int gab_gab_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

