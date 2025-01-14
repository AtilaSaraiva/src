/* Multiple dip estimation by chain of PWDs */
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

#include "pwdchain.h"
#include "smooth1.h"

int main(int argc, char* argv[])
{
    bool verb, drift, proj;
    int i, ic, m1, m2, n, nc, n2, iter, niter, liter, rect1, rect2, it, nt, nw, nr, k;
    float *xn, *x1, *y1, *dx, *r, lam, scale, step, rsum, rsum2, *x0;
    sf_file inp, out, dip, dipin;

    sf_init(argc,argv);
    inp = sf_input("in");
    dip = sf_output("dip");
    out = sf_output("out");

    if (NULL != sf_getstring("dipin")) {
	dipin = sf_input("dipin");
    } else {
	dipin = NULL;
    }

    if (SF_FLOAT != sf_gettype(inp)) sf_error("Need float input");
    if (!sf_histint(inp,"n1",&m1)) sf_error("No n1= in input");
    if (!sf_histint(inp,"n2",&m2)) sf_error("No n2= in input");
    n = m1*m2;
    nt = sf_leftsize(inp,2);

    if (!sf_getint("nc",&nc)) nc=1; /* number of components */

    sf_putint(dip,"n3",nc);
    sf_shiftdim(inp, dip, 3);
    
    n2 = (2*nc-1)*n;

    xn = sf_floatalloc(n2);
    dx = sf_floatalloc(n2);
    x0 = sf_floatalloc(n2);
    
    x1 = sf_floatalloc(n);
    y1 = sf_floatalloc(n);

    if (!sf_getint("order",&nw)) nw=1; /* PWD order */

    if (!sf_getbool("drift",&drift)) drift=false;
    /* if shift filter */

    if (!sf_getbool("vp",&proj)) proj=false;
    /* if use variable projection */

    nr = n*nc;
    
    pwdchain_init(m1,m2,nw,nc,drift,x1,xn,xn+nr);
    r = sf_floatalloc(nr);

    if (!sf_getbool("verb",&verb)) verb=(bool) (1 == nt);
    /* verbosity flag */
    if (!sf_getint("niter",&niter)) niter=0;
    /* number of iterations */
    if (!sf_getint("liter",&liter)) liter=50;
    /* number of linear iterations */

    if (!sf_getint("rect1",&rect1)) rect1=1;
    if (!sf_getint("rect2",&rect2)) rect2=1;
    /* smoothing radius */

     if (!sf_getfloat("lambda",&lam)) lam=1.0f;
    /* scaling */

    smooth1_init(m1,m2,nc,rect1,rect2);

    for (it=0; it < nt; it++) {
	sf_warning("slice %d of %d;",it+1,nt);
	sf_floatread(x1,n,inp);

	/* rescale */
	scale = 0.0f;
	for (i=0; i < n; i++) {
	    scale += x1[i]*x1[i];
	}
	scale = sqrtf(n/scale);
	
	/* initialize */
	for (i=0; i < n; i++) {
	    y1[i] = 0.0f;
	    x1[i] *= scale;
	}
	
	if (NULL != dipin) {
	    sf_floatread(xn,nr,dipin);
	} else {
	    if (1==nc) {
		for (i=0; i < n; i++) {
		    xn[i] = 0.0f;
		}
	    } else {
		for (ic=0; ic < nc; ic++) {
		    sf_warning("dip %1d = %g",ic+1, -nw + 2.0*nw*ic/(nc-1));
		    for (i=0; i < n; i++) {
			xn[ic*n+i] =  -nw + 2.0*nw*ic/(nc-1);
			/* distribute from -nw to nw */
		    }
		}
	    }
	}
	    
	for (ic=0; ic < nc-1; ic++) {
	    for (i=0; i < n; i++) {
		xn[(nc+ic)*n+i] = 0.0f; 
	    }
	}
	
	sf_conjgrad_init(n2, n2, nr, nr, lam, 1.e-6, verb, false);

	for (iter=0; iter < niter; iter++) {
	    pwdchain_apply(y1,r);
	    rsum = 0.0f;
	    for (i=0; i < nr; i++) {
		r[i] = -r[i];
		rsum += r[i]*r[i];
	    } 
	    
	    sf_conjgrad(NULL, pwdchain_lop, smooth1_lop,x0,dx,r,liter);

	    for (i=0; i < n2; i++) {
		x0[i] = xn[i];
	    }
	    
            /* line search */
	    step = 1.0f;
	    for (k=0; k < 8; k++) {
		for (i=0; i < n2; i++) {
		    xn[i] = x0[i] + step*dx[i];
		}
		
		pwdchain_apply(y1,r);

		rsum2 = 0.0f;
		for (i=0; i < nr; i++) {
		    r[i] = -r[i];
		    rsum2 += r[i]*r[i];
		} 

		if (rsum2 < rsum) break;

		step *= 0.5;
	    }

	    /* variable projection */
	    if (nc > 1 && proj) {
		sf_solver(pwdchainx_lop,sf_cgstep,n2-nr,nr,dx+nr,r,liter,"verb",true,"end");
		sf_cgstep_close();
		
		for (i=nr; i < n2; i++) {
		    xn[i] += dx[i];
		}
	    }
	}

	sf_cconjgrad_close();

	sf_floatwrite(xn,nr,dip);

	pwdchain(y1);

	for (i=0; i < n; i++) {
	    y1[i] /= scale;
	}
	
	sf_floatwrite(y1,n,out);
    }
    sf_warning(".");

    exit(0);
}
