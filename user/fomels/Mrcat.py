#!/usr/bin/env python
'Recursive sfcat (useful for a long list of files)'

##   Copyright (C) 2007 University of Texas at Austin
##  
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as published by
##   the Free Software Foundation; either version 2 of the License, or
##   (at your option) any later version.
##  
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##  
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import os,sys,string,tempfile
import rsf.path

bindir = os.path.join(os.environ.get('RSFROOT',sys.prefix),'bin')
sfcat = os.path.join(bindir,'sfcat')
sfrm  = os.path.join(bindir,'sfrm')
datapath = rsf.path.datapath().rstrip('/')

def rcat(files,options,out):
    'Call sfcat recursively on a list of files'
    global sfcat, sfrm, datapath
    if len(files) <= 3:
        stdout = os.dup(1)
        os.dup2(out,1)
        os.spawnv(os.P_WAIT,sfcat,['sfcat',]+options+files)
        os.dup2(stdout,1)
        os.close(out)
    else:
        middle = len(files)/2
        first = files[:middle]
        secon = files[middle:]
        fd,ffile = tempfile.mkstemp(dir=datapath)
        sd,sfile = tempfile.mkstemp(dir=datapath)
        rcat(first,options,fd)
        rcat(secon,options,sd)
        rcat([ffile,sfile],options,out)
        for tmp in ((fd,ffile),(sd,sfile)):
            try:
                os.system(sfrm + ' ' + tmp[1])
                os.close(tmp[0])
            except:
                pass

if __name__ == "__main__":
    options = []
    files = []
    for arg in sys.argv[1:]:
        if '=' in arg:
            options.append(arg)
        else:
            files.append(arg)
    rcat(files,options,1)

    sys.exit(0)


