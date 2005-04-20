/* Multicomponent data registration analysis. */
/*
  Copyright (C) 2004 University of Texas at Austin
  
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

#include "divn.h"

static float *coord, ***out, *rat2, *num, *den, g0, dg, o1, d1, o2, d2;
static int n2g, ntr, n1, n2, ng, order;

void warpscan_init(int m1     /* input trace length */, 
		   float o11  /* input origin */,
		   float d11  /* float increment */,
		   int m2     /* output trace length */,
		   float o21  /* output origin */,
		   float d21  /* output increment */,
		   int ng1    /* number of scanned "gamma" values */,
		   float g01  /* first gamma */,
		   float dg1  /* gamma increment */,
		   int ntr1   /* number of traces */, 
		   int order1 /* interpolation accuracy */, 
		   int dim    /* dimensionality */, 
		   int *m     /* data dimensions [dim] */, 
		   int *rect  /* smoothing radius [dim] */, 
		   int niter  /* number of iterations */)
/*< initialize >*/
{
    n1 = m1;
    o1 = o11;
    d1 = d11;
    o2 = o21;
    d2 = d21;
    n2 = m2;
    ntr = ntr1;
    ng = ng1;
    g0 = g01;
    dg = dg1;
    n2g = n2*ng*ntr;
    order = order1;

    coord = sf_floatalloc (n2); 
    out =   sf_floatalloc3 (n2,ng,ntr);

    rat2 = sf_floatalloc (n2g);
    num = sf_floatalloc (n2g);
    den = sf_floatalloc (n2g);

    sf_prefilter_init (order, n1, order*10);     
    divn_init(dim, n2g, m, rect, niter);
}

void warpscan(float** inp /* input data [ntr][n1] */, 
	      float** oth /* target data [ntr][n2] */,
	      float* rat1)
/*< scan >*/
{
    float doth, dout, g;
    int i1, i2, ig, i;

    doth = 0.;
    dout = 0.;
    for (i2=0; i2 < ntr; i2++) {
	sf_prefilter_apply (n1, inp[i2]);

	for (i1=0; i1 < n2; i1++) {
	    doth += oth[i2][i1]*oth[i2][i1];
	}
	
	for (ig=0; ig < ng; ig++) {
	    g = g0 + ig*dg;

	    for (i1=0; i1 < n2; i1++) {
		coord[i1] = (o2+i1*d2)*g;
	    }

	    sf_int1_init (coord, o1, d1, n1, sf_spline_int, order, n2);

	    sf_int1_lop (false,false,n1,n2,inp[i2],out[i2][ig]);

	    for (i1=0; i1 < n2; i1++) {
		dout += out[i2][ig][i1]*out[i2][ig][i1];
	    }
	}
    }
    doth = sqrtf(ntr*n2/doth);
    dout = sqrtf(n2g/dout);

    for (i2=0; i2 < ntr; i2++) {
	for (ig=0; ig < ng; ig++) {
	    for (i1=0; i1 < n2; i1++) {
		i = (i2*ng + ig)*n2+i1;
		den[i] = out[i2][ig][i1]*dout;
		num[i] = oth[i2][i1]*dout;
	    }
	}
    }

    divn(num,den,rat1);
	
    for (i2=0; i2 < ntr; i2++) {
	for (ig=0; ig < ng; ig++) {
	    for (i1=0; i1 < n2; i1++) {
		i = (i2*ng+ig)*n2+i1;
		num[i] = out[i2][ig][i1]*doth;
		den[i] = oth[i2][i1]*doth;
	    }
	}
    }
    divn(num,den,rat2);
	
    for (i=0; i < n2g; i++) {
	if (rat1[i] > 0.) {
	    if (rat2[i] > 0. || -rat2[i] < rat1[i]) {
		rat1[i] = sqrtf(fabsf(rat1[i]*rat2[i]));
	    } else {
		rat1[i] = -sqrtf(fabsf(rat1[i]*rat2[i]));
	    }
	} else {
	    if (rat2[i] < 0. || rat2[i] < -rat1[i]) {
		rat1[i] = -sqrtf(fabsf(rat1[i]*rat2[i]));
	    } else {
		rat1[i] = sqrtf(fabsf(rat1[i]*rat2[i]));
	    }
	}
    }
}

/* 	$Id: Mwarpscan.c 744 2004-08-17 18:46:07Z fomels $	 */
