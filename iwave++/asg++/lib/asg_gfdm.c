#include "asg_gfdm.h"

int asg_step(RDOM *, int, void*);
int duhasgfm24_2d(RDOM *, RDOM *, int, void*);
int duhasgam24_2d(RDOM *, RDOM *, int, void*);

#define DUH

int asg_tsf(RDOM * d, int ia, void * fdpars) {
  return asg_step(d,ia,fdpars);
}

int asg_tsfm(RDOM * d, RDOM * rd, int ia, void * fdpars) {
  SGN_TS_PARS * pars = (SGN_TS_PARS*)(fdpars);
  if (pars->k == 2 && pars->ndim == 2) {
#ifndef DUH
    return asg_ftsm2d_24(d,rd,ia,fdpars);
#else
    return duhasgfm24_2d(d,rd,ia,fdpars);
#endif
  }
  else {
    return E_NOTIMESTEP;
  }
}

int asg_tsam(RDOM * d, RDOM * rd, int ia, void * fdpars) {
  SGN_TS_PARS * pars = (SGN_TS_PARS*)(fdpars);
  if (pars->k == 2 && pars->ndim == 2) {
#ifndef DUH
    return asg_atsm2d_24(d,rd,ia,fdpars);
#else
    return duhasgam24_2d(d,rd,ia,fdpars);
#endif
  }
  else {
    return E_NOTIMESTEP;
  }
}

int asg_udfm(int ia, int iv, const IMODEL * m) {
  return ((FD_MODEL *)(m->specs))->update(ia,iv);
}

int asg_udam(int ia, int iv, const IMODEL * m) {
  if (isdyn((FD_MODEL *)(m->specs),ia)) 
    return !(((FD_MODEL *)(m->specs))->update(ia,iv));
  return 0;
}

void asgadj_refsubstep(int* it,int* iv, const IMODEL* m) {
  if ((m->tsind).iv > 0) *it = (m->tsind).it;
  else *it = (m->tsind).it-1;
  *iv = 1;
}

void asglin_refsubstep(int* it,int* iv, const IMODEL* m) {
  *it = (m->tsind).it;
  *iv = 1;
}

int asg_gfdm(GFD_MODEL * gfdm) {

  gfdm->gfd_model_init = asg_gfdm;
  gfdm->fd_model_init  = asg_modelinit;
  gfdm->tsf            = asg_tsf;
  gfdm->tsfm           = asg_tsfm;
  gfdm->tsam           = asg_tsam;
  gfdm->udfm           = asg_udfm;
  gfdm->udam           = asg_udam;
  gfdm->linrefsubstep  = asglin_refsubstep;
  gfdm->adjrefsubstep  = asgadj_refsubstep;

  return 0;
 
}

