/* Clip the data.

The output is 
 clip if input > clip
-clip if input < -clip
input if |input| < clip 

See also sfclip2.
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
#include <math.h>

#include <rsf.h>
#include <omp.h>
int main(int argc, char* argv[])
{
    bool nan;
    int i, n, nbuf;
    float clip, *trace;
    sf_file in=NULL, out=NULL; /* Input and output files */

    /* Initialize RSF */
    sf_init(argc,argv);
    /* standard input */
    in = sf_input("in");
    /* standard output */
    out = sf_output("out");


    /*Initialize OpenMP */
    omp_init();
    

    /* check that the input is float */
    if (SF_FLOAT != sf_gettype(in)) sf_error("Need float input");

    if (!sf_getfloat("clip",&clip)) sf_error("Need clip=");
    /* clip value */

    /* allocate floating point buffer */
    nbuf = sf_bufsiz(in)/sizeof(float);
    trace = sf_floatalloc (nbuf);

    /* process buffers */
    for (n = sf_filesize(in); n > 0; n -= nbuf) {
	if (nbuf > n) nbuf=n;

	sf_floatread(trace,nbuf,in);
# pragma omp parallel for private(i) shared (trace)
	for (i=0; i < nbuf; i++) {
#ifdef sun
	    nan = (bool) !finite(trace[i]);
#else
	    nan = (bool) !isnormal(trace[i]);
#endif
	    if (nan) trace[i] = SF_SIG(trace[i])*clip; 
	    else if (trace[i] >  clip) trace[i]= clip;
	    else if (trace[i] < -clip) trace[i]=-clip;
	}

	sf_floatwrite(trace,nbuf,out);
    }


    exit(0);
}
