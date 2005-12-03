/* Convolution with a Ricker wavelet. */
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

#include "ricker.h"

int main(int argc, char* argv[])
{
    int n1, n2, i2;
    float d1, freq, *trace;
    sf_file in, out;

    sf_init(argc,argv);
    in  = sf_input ( "in");
    out = sf_output("out");

    if (!sf_histint(in,"n1",&n1)) sf_error("No n1= in input");
    n2 = sf_leftsize(in,1);

    if (!sf_getfloat("frequency",&freq)) {
      /* peak frequency for Ricker wavelet (in Hz) */
      if (!sf_getfloat("freq",&freq)) freq=0.2;
      /* peak frequency for Ricker wavelet (as fraction of Nyquist) */
    } else {
      if (!sf_histfloat(in,"d1",&d1)) d1=1.;
      freq *= 2.*d1;
    }

    trace = sf_floatalloc(n1);
    ricker_init(sf_fftr_size2(n1, 2*n1),0.5*freq,0);

    for (i2=0; i2 < n2; i2++) {
	sf_floatread(trace,n1,in);
	sf_freqfilt(n1,trace);
	sf_floatwrite(trace,n1,out);
    }

    exit(0);
}

