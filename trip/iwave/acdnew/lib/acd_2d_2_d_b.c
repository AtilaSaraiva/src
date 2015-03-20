/*        Generated by TAPENADE     (INRIA, Tropics team)
    Tapenade 3.7 (r4786) - 21 Feb 2013 15:53
*/
#include "cstd.h"

/*
  Differentiation of acd_2d_2_d in reverse (adjoint) mode:
   gradient     of useful results: **ucd **upd **uc **up
   with respect to varying inputs: **ucd **upd **csq **uc **up
   RW status of diff variables: **ucd:incr **upd:in-out **csq:out
                **uc:incr **up:in-out
   Plus diff mem management of: ucd:in *ucd:in upd:in *upd:in
                csq:in *csq:in uc:in *uc:in up:in *up:in


  Differentiation of acd_2d_2 in forward (tangent) mode:
   variations   of useful results: **up
   with respect to varying inputs: **csq **uc **up
   RW status of diff variables: **csq:in **uc:in **up:in-out
   Plus diff mem management of: csq:in *csq:in uc:in *uc:in up:in
                *up:in
*/
void acd_2d_2_d_b(float **uc, float **ucb, float **ucd, float **ucdb,
                  float **up, float **upb, float **upd, float **updb,
                  float **csq,float **csqb,float **csqd,
                  int *s, int *e, float c0, float *c1) {
    int i0, i1;
    float tempb1;
    float tempb0;
    float tempb;
    /* **csqb = 0.0; */
    for (i1 = e[1]; i1 > s[1]-1; --i1)
        for (i0 = e[0]; i0 > s[0]-1; --i0) {
            float lap=(c0*uc[i1][i0]+
                       c1[0]*(uc[i1][i0+1]+uc[i1][i0-1])+
                       c1[1]*(uc[i1+1][i0]+uc[i1-1][i0]));
            float lapd=(c0*ucd[i1][i0]+
                        c1[0]*(ucd[i1][i0+1]+ucd[i1][i0-1])+
                        c1[1]*(ucd[i1+1][i0]+ucd[i1-1][i0]));
            
            tempb = csq[i1][i0]*upb[i1][i0];
            ucb[i1][i0] = ucb[i1][i0] + c0*tempb + 2.0*upb[i1][i0];
            csqb[i1][i0] = csqb[i1][i0] + lap*upb[i1][i0];
            ucb[i1][i0 + 1] = ucb[i1][i0 + 1] + c1[0]*tempb;
            ucb[i1][i0 - 1] = ucb[i1][i0 - 1] + c1[0]*tempb;
            ucb[i1 + 1][i0] = ucb[i1 + 1][i0] + c1[1]*tempb;
            ucb[i1 - 1][i0] = ucb[i1 - 1][i0] + c1[1]*tempb;
            upb[i1][i0] = -upb[i1][i0];
            tempb0 = csqd[i1][i0]*updb[i1][i0];
            tempb1 = csq[i1][i0]*updb[i1][i0];
            ucdb[i1][i0] = ucdb[i1][i0] + c0*tempb1 + 2.0*updb[i1][i0];
            ucb[i1][i0] = ucb[i1][i0] + c0*tempb0;
            ucb[i1][i0 + 1] = ucb[i1][i0 + 1] + c1[0]*tempb0;
            ucb[i1][i0 - 1] = ucb[i1][i0 - 1] + c1[0]*tempb0;
            ucb[i1 + 1][i0] = ucb[i1 + 1][i0] + c1[1]*tempb0;
            ucb[i1 - 1][i0] = ucb[i1 - 1][i0] + c1[1]*tempb0;
            csqb[i1][i0] = csqb[i1][i0] + lapd*updb[i1][i0];
            ucdb[i1][i0 + 1] = ucdb[i1][i0 + 1] + c1[0]*tempb1;
            ucdb[i1][i0 - 1] = ucdb[i1][i0 - 1] + c1[0]*tempb1;
            ucdb[i1 + 1][i0] = ucdb[i1 + 1][i0] + c1[1]*tempb1;
            ucdb[i1 - 1][i0] = ucdb[i1 - 1][i0] + c1[1]*tempb1;
            updb[i1][i0] = -updb[i1][i0];
        }
}