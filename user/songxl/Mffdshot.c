/* 2-D FFD RTM: MPI + OMP*/
/*
  Copyright (C) 2009 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>
#include <math.h>
#include <limits.h>
#include <mpi.h>
#ifdef _OPENMP
#include <omp.h>
#endif
static int nx, nz, nbt, nbb, nbl, nbr;
static float ct, cb, cl, cr;
static float *wt, *wb, *wl, *wr;
static float **ws;

void itoa(int n, char *s);

void bd_init(int n1,  int n2    /*model size:x*/,
             int nb1, int nb2  /*top, bottom*/,
             int nb3, int nb4  /*left, right*/,
             float c1, float c2 /*top, bottom*/,
             float c3, float c4 /*left, right*/);
/*< initialization >*/


void bd_close(void);
/*< free memory allocation>*/


void bd_decay(float **a /*2-D matrix*/);
/*< boundary decay>*/


void abc_cal(int abc /* decaying type*/,
             int nb  /* absorbing layer length*/, 
             float c /* decaying parameter*/,
             float* w /* output weight[nb] */);
/*< find absorbing coefficients >*/

void srcsm_init(float dz, float dx/*grid size*/);
/*< initialization >*/


void srcsm_close(void);
/*< free memory allocation>*/


void source_smooth(float **source /*source matrix*/, 
                   int iz /*source index*/, 
                   int ix /*source index*/,
                   float val/*source value*/);
/*< smooth source input >*/


float dehf(float k /*current frequency*/,
	   float kn /*highest frequency*/,
	   float a /*suppress factor*/,
	   float factor /*propotion*/);
/*< high frequency depressing>*/

int main(int argc, char* argv[]) 
{
    int nx, nt, nkx, nkz, ix, it, ikx, ikz, nz, iz, nbt, nbb, nbl, nbr, nxb, nzb, isx, isz, irz, ir;
    float dt, dx, dkx, kx, dz, dkz, kz, tmpdt,  pi=SF_PI, o1, o2, kx0, kz0, knx, knz;
    float **new,  **old,  **cur, **ukr, **dercur, **derold, *wav, **rvr;
    float **v, v0, v02, v2;
    float ***aa, dx2, dz2, w, g1, g2, ct, cb, cl, cr; /* top, bottom, left, right */
    float tmpvk, k2, err, dt2;
    float alpha; /* source smoothing */
    kiss_fft_cpx **uk, **ctracex, **ctracez;
    sf_file vel, source, geo, output;
    bool opt;    /* optimal padding */
    float **fcos;
    int nth=1, ith=0, esize, shot_num, n1, n2;
    int i, rank, nodes;
    int nr, jr, r0, nl, fsize, tl, jm, nts, its;
    char *oname, *mm;
    float ax, az, factor;
    int sht;
    kiss_fft_cfg *cfgx, *cfgxi, *cfgz, *cfgzi;
    /* MPI_Status stat; */
     
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nodes);
    if(rank==0) sf_warning("nodes=%d",nodes);
/*
  if (nodes < 2) {
  fprintf(stderr,"Need at least two nodes!\n");
  MPI_Finalize();
  }
*/

    sf_init(argc,argv);
    geo = sf_input("geo");   /* geometry: source location isx*/
    vel = sf_input("vel");   /* velocity */
    source = sf_input("source");   /* source wavlet*/

    if (rank == 0){
	if (SF_FLOAT != sf_gettype(vel)) sf_error("Need float input");
	if (SF_FLOAT != sf_gettype(source)) sf_error("Need float input");
    }

    if (!sf_histint(vel,"n1",&nx)) sf_error("No n1= in input");
    if (!sf_histfloat(vel,"d1",&dx)) sf_error("No d1= in input");
    if (!sf_histint(vel,"n2",&nz)) sf_error("No n2= in input");
    if (!sf_histfloat(vel,"d2",&dz)) sf_error("No d2= in input");
    if (!sf_histfloat(vel,"o1",&o1)) o1=0.0;
    if (!sf_histfloat(vel,"o2",&o2)) o2=0.0;
    if (!sf_getbool("opt",&opt)) opt=true;
    /* if y, determine optimal size for efficiency */
    if (!sf_getfloat("dt",&dt)) sf_error("Need dt input");
    if (!sf_getint("nt",&nt)) sf_error("Need nt input");
    if (!sf_getint("isz",&isz)) sf_error("Need isz input");
    if (!sf_getint("irz",&irz)) irz=0;
    if (!sf_getint("jr",&jr)) jr=1;
    if (!sf_getint("r0",&r0)) r0=0;
    if (!sf_getint("jm",&jm)) jm=20;
//    if (!sf_getint("nr",&nr)) sf_error("Need nr input");/*streamer total length*/
    if (!sf_getfloat("err",&err)) err = 0.00001;
    if (!sf_getfloat("alpha",&alpha)) alpha=-0.7;

    if (!sf_getint("nbt",&nbt)) nbt=44;
    if (!sf_getint("nbb",&nbb)) nbb=44;
    if (!sf_getint("nbl",&nbl)) nbl=44;
    if (!sf_getint("nbr",&nbr)) nbr=44;

    if (!sf_getfloat("ct",&ct)) ct = 0.01; /*decaying parameter*/
    if (!sf_getfloat("cb",&cb)) cb = 0.01; /*decaying parameter*/
    if (!sf_getfloat("cl",&cl)) cl = 0.01; /*decaying parameter*/
    if (!sf_getfloat("cr",&cr)) cr = 0.01; /*decaying parameter*/

    if (!sf_getfloat("ax",&ax)) ax= 2.0; /*suppress HF parameter*/
    if (!sf_getfloat("az",&az)) az= 2.0; /*suppress HF parameter*/
    if (!sf_getfloat("factor",&factor)) factor= 2.0/3.0; /*suppress HF parameter*/
    if (!sf_getint("sht",&sht)) sht= 0; /*Time shift parameter*/
    
    if(SF_INT != sf_gettype(geo)) sf_error("Need int!");
    if(!sf_histint(geo,"esize",&esize)) esize = sizeof("int");
    /* if(!sf_histint(geo,"n1",&shot_num)) sf_error("No n1=!"); */
   // if(!sf_histint(geo,"n1",&n1)) sf_error("No n1=!");
    //if(!(n1==1)) sf_error("n1 in geo should be 4");
    //if(!sf_histint(geo,"n2",&shot_num)) sf_error("No n2=!");
    if(!sf_histint(geo,"n1",&shot_num)) sf_error("No n1=!");
    if(rank==0) sf_warning("%d shots!",shot_num);
    n2 = sf_leftsize(geo,1);


    nxb = nx + nbl + nbr;
    nzb = nz + nbt + nbb;
    tl = nx*nz;

    nkx = opt? kiss_fft_next_fast_size(nxb): nxb;
    nkz = opt? kiss_fft_next_fast_size(nzb): nzb;
    if (nkx != nxb) sf_warning("nkx padded to %d",nkx);
    if (nkz != nzb) sf_warning("nkz padded to %d",nkz);
    dkx = 1./(nkx*dx)*2.0*pi;
    kx0 = -0.5/dx*2.0*pi;
    dkz = 1./(nkz*dz)*2.0*pi;
    kz0 = -0.5/dz*2.0*pi;
    knx = 0.5/dx*2.0*pi;
    knz = 0.5/dz*2.0*pi;


#ifdef _OPENMP
#pragma omp parallel
    {nth = omp_get_num_threads();}
    if(rank==0)  sf_warning("using %d threads",nth);
#endif
    uk = (kiss_fft_cpx **) sf_complexalloc2(nkx,nkz);
    ctracex = (kiss_fft_cpx **) sf_complexalloc2(nkx,nth);
    ctracez = (kiss_fft_cpx **) sf_complexalloc2(nkz,nth);

    cfgx  = (kiss_fft_cfg *) sf_complexalloc(nth);
    cfgxi = (kiss_fft_cfg *) sf_complexalloc(nth);
    cfgz  = (kiss_fft_cfg *) sf_complexalloc(nth);
    cfgzi = (kiss_fft_cfg *) sf_complexalloc(nth);

    for (i=0; i < nth; i++) {
        cfgx[i] = kiss_fft_alloc(nkx,0,NULL,NULL);
        cfgxi[i] = kiss_fft_alloc(nkx,1,NULL,NULL);
        cfgz[i] = kiss_fft_alloc(nkz,0,NULL,NULL);
        cfgzi[i]= kiss_fft_alloc(nkz,1,NULL,NULL);
    }
   

    nr = (nx-1)/jr+1;
    nts = (nt-sht-1)/jm+1;
    wav    =  sf_floatalloc(nt);
    rvr    =  sf_floatalloc2(nts,nr);
    fsize = sizeof(float);
    sf_floatread(wav,nt,source);

    old    =  sf_floatalloc2(nxb,nzb);
    cur    =  sf_floatalloc2(nxb,nzb);
    new    =  sf_floatalloc2(nxb,nzb);
    ukr    =  sf_floatalloc2(nxb,nzb);
    fcos   =  sf_floatalloc2(nkx,nkz);
    derold =  sf_floatalloc2(nxb,nzb);
    dercur =  sf_floatalloc2(nxb,nzb);
    aa     =  sf_floatalloc3(3,nxb,nzb);
    
    bd_init(nx,nz,nbt,nbb,nbl,nbr,ct,cb,cl,cr);

    v = sf_floatalloc2(nxb,nzb);

    /*input & extend velocity model*/
    for (iz=nbt; iz<nz+nbt; iz++){
        sf_floatread(v[iz]+nbl,nx,vel);
	for (ix=0; ix<nbl; ix++){
	    v[iz][ix] = v[iz][nbl];
	}
	for (ix=0; ix<nbr; ix++){
	    v[iz][nx+nbl+ix] = v[iz][nx+nbl-1];
	}     
    }
    for (iz=0; iz<nbt; iz++){
        for (ix=0; ix<nxb; ix++){
            v[iz][ix] = v[nbt][ix];
        }
    }
    for (iz=0; iz<nbb; iz++){
        for (ix=0; ix<nxb; ix++){
            v[nz+nbt+iz][ix] = v[nz+nbt-1][ix];
        }
    }
    v0 =0.0;
    for (iz=0; iz < nzb; iz++) {
        for (ix=0; ix < nxb; ix++) {
            v0 += v[iz][ix]*v[iz][ix];
	}
    }

    v0 = sqrtf(v0/(nxb*nzb));
    v02=v0*v0; 
    dt2 = dt*dt;
    dx2 = dx*dx;
    dz2 = dz*dz;

    for (iz=0; iz < nzb; iz++){
        for (ix=0; ix < nxb; ix++) {
            v2 = v[iz][ix]*v[iz][ix];
             w = v2/v02;
            g1 = dt2*(v2-v02)/(12.0*dx2);
            g2 = dt2*(v2-v02)/(12.0*dz2);
            aa[iz][ix][1] = w*g1;
            aa[iz][ix][2] = w*g2;
            aa[iz][ix][0] = w-2.0*aa[iz][ix][1]-2.0*aa[iz][ix][2];
        }
    }
    free(*v);     
    free(v);     
    sf_fileclose(vel);
    sf_fileclose(source);
    for (ikz=0; ikz < nkz; ikz++) {
        kz = (kz0+ikz*dkz);
        for (ikx=0; ikx < nkx; ikx++) {
            kx = (kx0+ikx*dkx);
            tmpvk = v0*sqrtf(kx*kx+kz*kz)*dt;
            tmpdt = 2.0*(cosf(tmpvk)-1.0);
            fcos[ikz][ikx] = tmpdt;
        }
    }

    sf_seek(geo,rank*esize,SEEK_SET);
    srcsm_init(dz,dx);

    sf_warning("Rank= %d",rank);
    for (i=rank; i < shot_num; i+=nodes) {
        sf_warning("Shot No. %d",i);
        /* out = fopen("/local/lfs1/data/snap","w"); */
	/* out = fopen("/tmp/snap","w"); */
        sf_intread(&isx,1,geo);
        sf_seek(geo,(nodes-1)*esize,SEEK_CUR);
        mm = sf_charalloc(15);
        oname = sf_charalloc(25);
        itoa(isx,mm);
        mm = strcat(mm,".rsf");
        oname[0] = 'S';
        oname[1] = 'H';
        oname[2] = 'O';
        oname[3] = 'T';
        oname[4] = '/';
        oname[5] = 's';
        oname[6] = 'h';
        oname[7] = 'o';
        oname[8] = 't';
        oname[9] = '_';
        oname[10] = '\0';
        oname = strcat(oname,mm);
        free(mm);
        sf_warning("%s",oname);
        output = sf_output(oname);
        free(oname);
        sf_putint(output,"n1",nts);
        sf_putfloat(output,"d1",dt*jm);
        sf_putint(output,"n2",nr);
        sf_putfloat(output,"d2",dx*jr);
        sf_settype(output,SF_FLOAT);
         
        
        /* Initialization */
        for (iz=0; iz < nzb; iz++) {
            for (ix=0; ix < nxb; ix++) {
                cur[iz][ix] = 0.0;
            }
        }
        for (iz=0; iz < nzb; iz++) {
            for (ix=0; ix < nxb; ix++) {
                old[iz][ix] =  0.0; 
                derold[iz][ix] =cur[iz][ix]/dt;
	    }
	}
 
        /* propagation in time */
        for (it=0; it < nt; it++) {
            if(it<1500 )  { 
		cur[isz+nbt][isx+nbl] += wav[it];
		source_smooth(cur,isz+nbt,isx+nbl,wav[it]);
	    }
            
            if(!((it-sht)%jm)) {
               for (ir=0; ir < nr; ir++) {
                   ix = r0 + ir*jr;
                   its = (it-sht)/jm;
                   rvr[ir][its] =  cur[irz+nbt][ix+nbl]; 
               }
            }

#ifdef _OPENMP
#pragma omp parallel for private(iz,ix) 
#endif
	    for (iz=0; iz < nzb; iz++){
		for (ix=0; ix < nxb; ix++){ 
		    new[iz][ix] = 0.0; 
		    uk[iz][ix].r = cur[iz][ix];
		    uk[iz][ix].i = 0.0; 
		}
	    }  


/*      compute u(kx,kz) */
#ifdef _OPENMP
#pragma omp parallel for private(iz,ix,ikx,ith) 
#endif
	    for (iz=0; iz < nzb; iz++){
		/* Fourier transform x to kx */
		for (ix=1; ix < nxb; ix+=2){
		    uk[iz][ix] = sf_cneg(uk[iz][ix]);
		}
#ifdef _OPENMP
		ith = omp_get_thread_num();
#endif
		kiss_fft_stride(cfgx[ith],uk[iz],ctracex[ith],1); 
		for (ikx=0; ikx<nkx; ikx++) uk[iz][ikx] = ctracex[ith][ikx]; 
	    }
#ifdef _OPENMP
#pragma omp parallel for private(ikz,ikx,ith) 
#endif
	    for (ikx=0; ikx < nkx; ikx++){
		/* Fourier transform z to kz */
		for (ikz=1; ikz<nkz; ikz+=2){
		    uk[ikz][ikx] = sf_cneg(uk[ikz][ikx]);
		}
#ifdef _OPENMP
		ith = omp_get_thread_num();
#endif
		kiss_fft_stride(cfgz[ith],uk[0]+ikx,ctracez[ith],nkx); 
		for (ikz=0; ikz<nkz; ikz++) uk[ikz][ikx] = ctracez[ith][ikz]; 
	    }

#ifdef _OPENMP
#pragma omp parallel for private(ikz,ikx,tmpdt) 
#endif
	    for (ikz=0; ikz < nkz; ikz++) {
		for (ikx=0; ikx < nkx; ikx++) {
		    tmpdt = fcos[ikz][ikx];
		    uk[ikz][ikx] = sf_crmul(uk[ikz][ikx],tmpdt);
		}
	    }   
/*      Inverse FFT*/
#ifdef _OPENMP
#pragma omp parallel for private(ikz,ikx,ith) 
#endif
	    for (ikx=0; ikx < nkx; ikx++){
		/* Inverse Fourier transform kz to z */
#ifdef _OPENMP
		ith = omp_get_thread_num();
#endif
		kiss_fft_stride(cfgzi[ith],(kiss_fft_cpx *)uk[0]+ikx,ctracez[ith],nkx); 
		for (ikz=0; ikz < nkz; ikz++) uk[ikz][ikx] = sf_crmul(ctracez[ith][ikz],ikz%2?-1.0:1.0); 
	    }
#ifdef _OPENMP
#pragma omp parallel for private(ikz,ikx,ith) 
#endif
	    for (ikz=0; ikz < nkz; ikz++){
		/* Inverse Fourier transform kx to x */
#ifdef _OPENMP
		ith = omp_get_thread_num();
#endif
		kiss_fft_stride(cfgxi[ith],(kiss_fft_cpx *)uk[ikz],ctracex[ith],1); 
		for (ikx=0; ikx < nkx; ikx++) uk[ikz][ikx] = sf_crmul(ctracex[ith][ikx],ikx%2?-1.0:1.0); 
	    }
#ifdef _OPENMP
#pragma omp parallel for private(iz,ix) 
#endif
	    for (iz=0; iz < nzb; iz++){
		for (ix=0; ix < nxb; ix++){ 
		    ukr[iz][ix] = sf_crealf(uk[iz][ix]); 
		    ukr[iz][ix] /= (nkx*nkz); 
		}
	    }  

#ifdef _OPENMP
#pragma omp parallel for private(iz,ix) 
#endif
	    for (iz=1; iz < nzb-1; iz++) {  
		for (ix=1; ix < nxb-1; ix++) {  
		    new[iz][ix]  = ukr[iz][ix]*aa[iz][ix][0]
			+ (ukr[iz][ix-1]+ukr[iz][ix+1])*aa[iz][ix][1]
			+ (ukr[iz-1][ix]+ukr[iz+1][ix])*aa[iz][ix][2];
		}
	    }  
             

#ifdef _OPENMP
#pragma omp parallel for private(iz,ix) 
#endif
	    for (iz=0; iz < nzb; iz++) {  
		for (ix=0; ix < nxb; ix++) {
		    dercur[iz][ix]= derold[iz][ix] + new[iz][ix]/dt;
		    new[iz][ix] = cur[iz][ix] + dercur[iz][ix]*dt; 
		    /*     new[iz][ix] += 2.0*cur[iz][ix] -old[iz][ix]; */
		}
	    }
 
	    bd_decay(new); 
	    bd_decay(dercur); 
 
#ifdef _OPENMP
#pragma omp parallel for private(iz,ix) 
#endif
	    for (iz=0; iz < nzb; iz++) {  
		for(ix=0; ix < nxb; ix++) {
                    old[iz][ix] = cur[iz][ix]; 
                    cur[iz][ix] = new[iz][ix]; 
                    derold[iz][ix] = dercur[iz][ix]; 
		}
	    }
        }
        sf_floatwrite(rvr[0],nr*nts,output);
        sf_warning("shot output");
        sf_fileclose(output);
    } 
    bd_close();
    srcsm_close();
    free(**aa);
    free(*aa);
    free(aa);
    free(*new);     
    free(*cur);     
    free(*old);     
    free(*dercur);     
    free(*derold);     
    free(*uk);     
    free(*ukr);     
    free(new);     
    free(cur);     
    free(old);     
    free(dercur);     
    free(derold);     
    free(uk);     
    free(ukr);     
    MPI_Finalize();

    exit(0); 
}           



void itoa(int n, char *s)
{
    int i,j,sign;
    char c;
    sign = n;
    if (n < 0) n=-n;
    i = 0;
    do {
	s[i++] = n%10+'0';
	n = (int) n/10;
    } while(n > 0);
    if (sign <0) s[i++] = '-';
    s[i] = '\0';
    for (i=0,j=strlen(s)-1;i<j;i++,j--){
        c = s[i];
        s[i]=s[j];
        s[j]=c;
    }
}

void srcsm_init(float dz, float dx/*grid size*/)
/*< initialization >*/
{

    float dx2, dz2, R0, rxxz, rxzz;
    dx2 = dx*dx;
    dz2 = dz*dz;
    R0 = sqrtf(dx2+dz2);
    rxxz = sqrtf(4.0*dx2+dz2);
    rxzz = sqrtf(4.0*dz2+dx2);
    ws =  sf_floatalloc2(3,2);
    ws[0][0] = -0.5*dz2/(R0*R0)+1.0;
    if(2.0*dz < R0) { 
	ws[0][1]= -2.0*dz2/(R0*R0)+1.0;
    } else {
	ws[0][1]= 2.0*(dz-R0)*(dz-R0)/(R0*R0);
    }
    ws[0][2]= 0.5*(rxzz-2.0*R0)*(rxzz-2.0*R0)/(R0*R0);
    ws[1][0] = -0.5*dx2/(R0*R0)+1.0;
    if(2.0*dx < R0) { 
	ws[1][1] = -2.0*dx2/(R0*R0)+1.0;
    } else {
	ws[1][1] = 2.0*(dx-R0)*(dx-R0)/(R0*R0);
    }
    ws[1][2]= 0.5*(rxxz-2.0*R0)*(rxxz-2.0*R0)/(R0*R0);



}
   

void srcsm_close(void)
/*< free memory allocation>*/
{
    free(*ws);
    free(ws);

}



void source_smooth(float **source /*source matrix*/, 
                   int iz /*source index*/, 
                   int ix /*source index*/,
                   float val/*source value*/)
/*< smooth source input >*/
{
   
    source[iz-1][ix]   += val*ws[0][0]; 
    source[iz+1][ix]   += val*ws[0][0]; 

    source[iz+2][ix]   += val*ws[0][1];
    source[iz-2][ix]   += val*ws[0][1];

    source[iz+2][ix+1]   += val*ws[0][2]; 
    source[iz+2][ix-1]   += val*ws[0][2]; 
    source[iz-2][ix-1]   += val*ws[0][2]; 
    source[iz-2][ix+1]   += val*ws[0][2]; 

    source[iz-1][ix-1] += val*0.5; 
    source[iz-1][ix+1] += val*0.5; 
    source[iz+1][ix-1] += val*0.5; 
    source[iz+1][ix+1] += val*0.5; 

    source[iz][ix-1]   += val*ws[1][0];
    source[iz][ix+1]   += val*ws[1][0];

    source[iz][ix+2]   += val*ws[1][1];
    source[iz][ix-2]   += val*ws[1][1];

    
    source[iz+1][ix+2]   += val*ws[1][2]; 
    source[iz+1][ix-2]   += val*ws[1][2]; 
    source[iz-1][ix-2]   += val*ws[1][2]; 
    source[iz-1][ix+2]   += val*ws[1][2]; 

}



void bd_init(int n1,  int n2    /*model size:x*/,
             int nb1, int nb2  /*top, bottom*/,
             int nb3, int nb4  /*left, right*/,
             float c1, float c2 /*top, bottom*/,
             float c3, float c4 /*left, right*/)
/*< initialization >*/
{
    int c;
    nx = n1;  
    nz = n2;  
    nbt = nb1;  
    nbb = nb2;  
    nbl = nb3;  
    nbr = nb4;  
    ct = c1;  
    cb = c2;  
    cl = c3;  
    cr = c4;  
    if(nbt) wt =  sf_floatalloc(nbt);
    if(nbb) wb =  sf_floatalloc(nbb);
    if(nbl) wl =  sf_floatalloc(nbl);
    if(nbr) wr =  sf_floatalloc(nbr);
    c=0;
    abc_cal(c,nbt,ct,wt);
    abc_cal(c,nbb,cb,wb);
    abc_cal(c,nbl,cl,wl);
    abc_cal(c,nbr,cr,wr);
}
   

void bd_close(void)
/*< free memory allocation>*/
{
    if(nbt) free(wt);
    if(nbb) free(wb);
    if(nbl) free(wl);
    if(nbr) free(wr);
}

void bd_decay(float **a /*2-D matrix*/) 
/*< boundary decay>*/
{
    int iz, ix;
    for (iz=0; iz < nbt; iz++) {  
        for (ix=nbl; ix < nx+nbl; ix++) {
            a[iz][ix] *= wt[iz];
        }
    }
    for (iz=0; iz < nbb; iz++) {  
        for (ix=nbl; ix < nx+nbl; ix++) {
            a[iz+nz+nbt][ix] *= wb[nbb-1-iz];
        }
    }
    for (iz=nbt; iz < nz+nbt; iz++) {  
        for (ix=0; ix < nbl; ix++) {
            a[iz][ix] *= wl[ix];
        }
    }
    for (iz=nbt; iz < nz+nbt; iz++) {  
        for (ix=0; ix < nbr; ix++) {
            a[iz][nx+nbl+ix] *= wr[nbr-1-ix];
        }
    }
    for (iz=0; iz < nbt; iz++) {  
        for (ix=0; ix < nbl; ix++) {
            /* a[iz][ix] *= (float)iz/(float)(ix+iz)*wl[ix]+(float)ix/(float)(ix+iz)*wt[iz]; */
            a[iz][ix] *= iz>ix? wl[ix]:wt[iz];
        }
    }
    for (iz=0; iz < nbt; iz++) {  
        for (ix=0; ix < nbr; ix++) {
            a[iz][nx+nbl+nbr-1-ix] *= iz>ix? wr[ix]:wt[iz];
        }
    }
    for (iz=0; iz < nbb; iz++) {  
        for (ix=0; ix < nbl; ix++) {
            a[nz+nbt+nbb-1-iz][ix] *= iz>ix? wl[ix]:wb[iz];
        }
    }
    for (iz=0; iz < nbb; iz++) {  
        for (ix=0; ix < nbr; ix++) {
            a[nz+nbt+nbb-1-iz][nx+nbl+nbr-1-ix] *= iz>ix? wr[ix]:wb[iz];
        }
    }
}

void abc_cal(int abc /* decaying type*/,
             int nb  /* absorbing layer length*/, 
             float c /* decaying parameter*/,
             float* w /* output weight[nb] */)
/*< find absorbing coefficients >*/
{
    int ib;
    const float pi=SF_PI;
    if(!nb) return;
    switch(abc){
	default:
	    for(ib=0; ib<nb; ib++){
		w[ib]=exp(-c*c*(nb-1-ib)*(nb-1-ib));
	    }
	    break;
	case(1): 
	    for(ib=0; ib<nb; ib++){
		w[ib]=powf((1.0+0.9*cosf(((float)(nb-1.0)-(float)ib)/(float)(nb-1.0)*pi))/2.0,abc);
	    }
    }   
}
float dehf(float k /*current frequency*/,
	   float kn /*highest frequency*/,
	   float a /*suppress factor*/,
	   float factor /*propotion*/)
/*< high frequency depressing>*/
{
    float kmax, kmax2;
    float depress;
    /* float pi=SF_PI; */
    kmax =  (kn*factor);
    kmax2 = (kmax+kn)/2.0;
    if (fabs(k) < kmax) {
	depress = 1.0;
    }
    else {
	depress = exp(-a*(float)((k-kmax)*(k-kmax))/((float)((kn-kmax)*(kn-kmax))));
/*          depress =cosf(((fabs(k)-kmax))/((kn-kmax))*pi/2.0); */
	/*       depress = depress * depress; */
    }
    /*   depress = exp(-a*((fabs(k)-kmax)*(fabs(k)-kmax))/((kn-kmax)*(kn-kmax))); */
/*
  else if (fabs(k) < kmax2){
  depress =cosf(((fabs(k)-kmax))/((kmax2-kmax))*pi/2.0);
  depress = depress * depress;
  }    
  // depress = exp(-a*(float)((k-kmax)*(k-kmax))/((float)((kn-kmax)*(kn-kmax)))); 
  else
  depress = 0.0;
//       depress = exp(-a*(float)((k-kmax)*(k-kmax))/((float)((kn-kmax)*(kn-kmax))));
*/
    return(depress);
}

