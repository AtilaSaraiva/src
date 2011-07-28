/* Concatenate datasets. 

Takes: [<file0.rsf] file1.rsf file2.rsf ... 

sfmerge inserts additional space between merged data.
*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <unistd.h>

#include <rsf.h>

static int *order, *sort;

static int sort_order(const void *a, const void *b) 
{
    int ia, ib;

    ia = sort[(int*) a - order];
    ib = sort[(int*) b - order];

    return (ia < ib)? -1: (ia > ib)? 1: 0;
}

static void check_compat (int esize, int nin, sf_file *in, int axis, 
			  int dim, const off_t *n, /*@out@*/ int *naxis);

int main (int argc, char* argv[])
{
    int i, j, k, axis, *naxis, nin, open_max;
    int dim, dim1, esize, nspace;
    float f;
    off_t ni, nbuf, n1, n2, i2, n[SF_MAX_DIM]; 
    sf_file *in, out;
    char *prog, key[3], buf[BUFSIZ];
    const char **filename;
    bool space;
    
    sf_init(argc,argv);
    
    filename = (const char**) sf_alloc ((size_t) argc,sizeof(char*));

    if (!sf_stdin()) { /* no input file in stdin */
	nin=0;
    } else {
	filename[0] = "in";
	nin=1;
    }

    for (i=1; i< argc; i++) { /* collect inputs */
	if (NULL != strchr(argv[i],'=')) 
	    continue; /* not a file */
	filename[nin] = argv[i];
	nin++;
    }
    if (0==nin) sf_error ("no input");

    order = sf_intalloc(nin);
    for (j=0; j < nin; j++) order[j]=j;

    sort = sf_intalloc(nin);
    if (sf_getints("order",sort,nin)) {
	/* concatenation order */
	qsort(order,nin,sizeof(int),sort_order);
    }
    free(sort);

    open_max = sysconf(_SC_OPEN_MAX);
    if (open_max > 0 && 2*nin+5 > open_max)
	sf_error("Too many files for sfcat, try sfrcat instead.");

    in = (sf_file*) sf_alloc ((size_t) nin,sizeof(sf_file));
    for (i=0; i< nin; i++) {
	in[i] = sf_input(filename[i]);
    }
    out = sf_output ("out");

    if (!sf_getbool("space",&space)) {
	/* Insert additional space.
	   y is default for sfmerge, n is default for sfcat */
	prog = sf_getprog();
	if (NULL != strstr (prog, "merge")) {
	    space = true;
	} else if (NULL != strstr (prog, "cat")) {
	    space = false;
	} else {
	    sf_warning("%s is neither merge nor cat,"
		       " assume merge",prog);
	    space = true;
	}
    }

    dim = sf_largefiledims(in[0],n);
    if (!sf_getint("axis",&axis)) axis=3;
    /* Axis being merged */
    if (1 > axis) sf_error("axis=%d < 1",axis);

    dim1 = dim;
    if (axis > dim) {
	while (dim < axis) {
	    n[dim++] = 1;
	}
    }

    n1=1;
    n2=1;
    for (i=1; i <= dim; i++) {
	if      (i < axis) n1 *= n[i-1];
	else if (i > axis) n2 *= n[i-1];
    }

    naxis = sf_intalloc(nin);

    if (!sf_histint(in[0],"esize",&esize)) {
	esize=4;
    } else if (0>=esize) {
	sf_error("cannot handle esize=%d",esize);
    }
    esize = sf_esize(in[0]);
    check_compat(esize,nin,in,axis,dim1,n,naxis);

    /* figure out the length of extended axis */
    ni = 0;
    for (j=0; j < nin; j++) {
	ni += naxis[j];
    }

    if (space) {
	if (!sf_getint("nspace",&nspace)) 
	    nspace = (int) (ni/(20*nin) + 1);
	/* if space=y, number of traces to insert */ 
	ni += nspace*(nin-1);
    } 

    (void) snprintf(key,3,"n%d",axis);
    sf_putint(out,key,(int) ni);
    
    if (sf_getfloat("o",&f)) {
	/* axis origin */
	(void) snprintf(key,3,"o%d",axis);
	sf_putfloat(out,key,f);
    }

    if (sf_getfloat("d",&f)) {
	/* axis sampling */
	(void) snprintf(key,3,"d%d",axis);	
	sf_putfloat(out,key,f);
    }

    sf_setformat(out,sf_histstring(in[0],"data_format"));
    sf_fileflush(out,in[0]);
    sf_setform(out,SF_NATIVE);
    for (j=0; j < nin; j++) {
    	sf_setform(in[j],SF_NATIVE);
    }

    for (i2=0; i2 < n2; i2++) {
	for (j=0; j < nin; j++) {
	    k = order[j];
	    for (ni = n1*naxis[j]*esize; ni > 0; ni -= nbuf) {
		nbuf = (BUFSIZ < ni)? BUFSIZ: ni;
		sf_charread (buf,nbuf,in[k]);
		sf_charwrite (buf,nbuf,out);
	    }
	    if (!space || j == nin-1) continue;
	    /* Add spaces */
	    memset(buf,0,BUFSIZ);
	    for (ni = n1*nspace*esize; ni > 0; ni -= nbuf) {
		nbuf = (BUFSIZ < ni)? BUFSIZ: ni;
		sf_charwrite (buf,nbuf,out);
	    }
	}
    }
    

    exit(0);
}

static void check_compat (int esize, int nin, sf_file *in, int axis, int dim, 
			  const off_t *n, /*@out@*/ int *naxis) 
/*< check if the file dimensions are compatible >*/
{
    int i, ni, id;
    float o, d, di, oi;
    char key[3];
    const float tol=1.e-3;
    
    naxis[0] = n[axis-1];
    for (i=1; i < nin; i++) {
	if (!sf_histint(in[i],"esize",&ni) || ni != esize)
	    sf_error ("esize mismatch: need %d",esize);
	for (id=1; id <= dim; id++) {
	    (void) snprintf(key,3,"n%d",id);
	    if (!sf_histint(in[i],key,&ni) || (id != axis && ni != n[id-1]))
#if defined(__cplusplus) || defined(c_plusplus)
		sf_error("%s mismatch: need %ld",key,(long int) n[id-1]);
#else
	        sf_error("%s mismatch: need %lld",key,(long long int) n[id-1]);
#endif
	    if (id == axis) naxis[i] = ni;
	    (void) snprintf(key,3,"d%d",id);
	    if (sf_histfloat(in[0],key,&d)) {
		if (!sf_histfloat(in[i],key,&di) || 
		    (id != axis && fabsf(di-d) > tol*fabsf(d)))
		    sf_warning("%s mismatch: need %g",key,d);
	    } else {
		d = 1.;
	    }
	    (void) snprintf(key,3,"o%d",id);
	    if (sf_histfloat(in[0],key,&o) && 
		(!sf_histfloat(in[i],key,&oi) || 
		 (id != axis && fabsf(oi-o) > tol*fabsf(d))))
		sf_warning("%s mismatch: need %g",key,o);
	}
	if (axis > dim) {
	    (void) snprintf(key,3,"n%d",axis);
	    if (!sf_histint(in[i],key,naxis+i)) naxis[i]=1;
	}
    }
}

/* 	$Id$	 */

