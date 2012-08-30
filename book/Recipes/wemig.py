try:    from rsf.cluster import *
except: from rsf.proj    import *
import spmig,sgmig,zomig,fdmod
import random

def param(par):
    p  = ' '
    p = p + ' --readwrite=y'
    if(par.has_key('verb')):
        p = p + ' verb='  +     par['verb']
    if(par.has_key('nrmax')):
        p = p + ' nrmax=' + str(par['nrmax'])
    if(par.has_key('dtmax')):
        p = p + ' dtmax=' + str(par['dtmax'])
    if(par.has_key('eps')):
        p = p + ' eps='   + str(par['eps'])
    if(par.has_key('tmx')):
        p = p + ' tmx='   + str(par['tmx'])
    if(par.has_key('tmy')):
        p = p + ' tmy='   + str(par['tmy'])
    if(par.has_key('pmx')):
        p = p + ' pmx='   + str(par['pmx'])
    if(par.has_key('pmy')):
        p = p + ' pmy='   + str(par['pmy'])
    if(par.has_key('misc')):
        p = p + ' '       +     par['misc']
    p = p + ' '
    return p

# ------------------------------------------------------------
def wempar(par):
    if(not par.has_key('verb')):    par['verb']='y'
    if(not par.has_key('eps')):     par['eps']=0.1

    if(not par.has_key('nrmax')):   par['nrmax']=1
    if(not par.has_key('dtmax')):   par['dtmax']=0.00005

    if(not par.has_key('tmx')):     par['tmx']=16
    if(not par.has_key('tmy')):     par['tmy']=16
    
    if(not par.has_key('incore')):  par['incore']='y'

# ------------------------------------------------------------
def slowness(slow,velo,par):
    Flow(slow,velo,
         '''
         math "output=1/input" |
         transp plane=12 | 	
	 transp plane=23
         ''')

# ------------------------------------------------------------
# WEM
# ------------------------------------------------------------

# WR: forward in time
def fWRwem(data,wfld,slow,par):
#    Flow(wfld,[data,slow],
#         '''
#         wex causal=y %s slo=${SOURCES[1]} |
#         window |
#         transp
#         ''' % param(par))
    Flow(wfld+'_tmp',[data,slow],
         'wexwfl causal=y %s slo=${SOURCES[1]}' % param(par))
    Flow(wfld,wfld+'_tmp','window | transp')

# WR: backward in time
def bWRwem(data,wfld,slow,par):
#    Flow(wfld,[data,slow],
#         '''
#         wex causal=n %s slo=${SOURCES[1]} |
#         window |
#         transp
#         ''' % param(par))
    Flow(wfld+'_tmp',[data,slow],
         'wexwfl causal=n %s slo=${SOURCES[1]}' % param(par))
    Flow(wfld,wfld+'_tmp','window | transp')


def wemWR(data,wfld,slow,causal,par):
    Flow(wfld,[data,slow],
         'wexwfl %s slo=${SOURCES[1]}' % param(par) + ' causal=%s '%causal) 

# ------------------------------------------------------------
# RTM
# ------------------------------------------------------------

# WR: forward in time
def fWRrtm(data,wfld,velo,dens,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    fdmod.anifd2d(wfld+'_out',wfld,
                  data,velo,dens,coor,coor,iwindow+custom,par)

# WR: backward in time
def bWRrtm(data,wfld,velo,dens,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    Flow(data+'_rev',data,'reverse which=2 opt=i verb=y')
    fdmod.anifd2d(wfld+'_out',wfld+'_rev',
                  data+'_rev',velo,dens,coor,coor,iwindow+custom,par)
    Flow(wfld,wfld+'_rev','reverse which=4 opt=i verb=y')



# WR: forward in time
def fWRawe(data,wfld,velo,dens,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    fdmod.awefd2d(wfld+'_out',wfld,
                  data,velo,dens,coor,coor,iwindow+custom,par)

# WR: backward in time
def bWRawe(data,wfld,velo,dens,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    Flow(data+'_rev',data,'reverse which=2 opt=i verb=y')
    fdmod.awefd2d(wfld+'_out',wfld+'_rev',
                  data+'_rev',velo,dens,coor,coor,iwindow+custom,par)
    Flow(wfld,wfld+'_rev','reverse which=4 opt=i verb=y')

# WR: forward in time
def fWRcda(data,wfld,velo,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    fdmod.cdafd(wfld+'_out',wfld,
                  data,velo,coor,coor,iwindow+custom,par)

# WR: backward in time
def bWRcda(data,wfld,velo,coor,custom,par):
    iwindow = ' ' + \
        '''
        nqz=%(nqz)d oqz=%(oqz)g
        nqx=%(nqx)d oqx=%(oqx)g
        jsnap=%(jdata)d jdata=%(jdata)d
        ''' % par + ' '

    Flow(data+'_rev',data,'reverse which=2 opt=i verb=y')
    fdmod.cdafd(wfld+'_out',wfld+'_rev',
                  data+'_rev',velo,coor,coor,iwindow+custom,par)
    Flow(wfld,wfld+'_rev','reverse which=4 opt=i verb=y')

# ------------------------------------------------------------
# constant-density shot-record migration
def cdrtm(imag,velo,
          sdat,scoo,
          rdat,rcoo,
          custom,par):

    M8R='$RSFROOT/bin/sf'
    DPT=os.environ.get('TMPDATAPATH')

    awewin = 'nqz=%(nqz)d oqz=%(oqz)g nqx=%(nqx)d oqx=%(oqx)g'%par
    awepar = 'ompchunk=%(ompchunk)d ompnth=%(ompnth)d verb=y free=n snap=%(snap)s jsnap=%(jdata)d jdata=%(jdata)d dabc=%(dabc)s nb=%(nb)d'%par + ' ' + custom

    swfl=imag+'swfl'
    rdrv=imag+'rdrv'
    rwrv=imag+'rwrv'
    rwfl=imag+'rwfl'

    Flow(imag,[sdat,scoo,rdat,rcoo,velo],
         '''
         %sawefd2d < ${SOURCES[0]} cden=y %s
         vel=${SOURCES[4]}
         sou=${SOURCES[1]}
         rec=${SOURCES[1]}
         wfl=%s datapath=%s/
         >/dev/null;
         '''%(M8R,awewin+' '+awepar,swfl,DPT) +
         '''
         %sreverse < ${SOURCES[2]} which=2 opt=i verb=y >%s datapath=%s/;
         '''%(M8R,rdrv,DPT) +
         '''
         %sawefd2d < %s cden=y %s
         vel=${SOURCES[4]}
         sou=${SOURCES[3]}
         rec=${SOURCES[3]}
         wfl=%s datapath=%s/
         >/dev/null;
         '''%(M8R,rdrv,awewin+' '+awepar,rwrv,DPT) +
         '''
         %sreverse < %s which=4 opt=i verb=y >%s datapath=%s/;
         '''%(M8R,rwrv,rwfl,DPT) +
         '''
         %sxcor2d <%s uu=%s axis=3 verb=y %s >${TARGETS[0]};
         '''%(M8R,swfl,rwfl,custom) +
         '''
         %srm %s %s %s %s
         '''%(M8R,swfl,rdrv,rwrv,rwfl),
              stdin=0,
              stdout=0)

# ------------------------------------------------------------
# IC
# ------------------------------------------------------------

# CIC: cross-correlation
def cic(imag,swfl,rwfl,custom,par):
    par['ciccustom'] = custom

    Flow(imag,[swfl,rwfl],
         '''
         xcor2d 
         uu=${SOURCES[1]} axis=3 verb=y ompnth=%(ompnth)d 
         %(ciccustom)s
         ''' % par)

def cic3d(imag,swfl,rwfl,par):
    Flow(imag,[swfl,rwfl],
         '''
         cic uu=${SOURCES[1]} axis=3 verb=y ompnth=%(ompnth)d
         ''' % par)

# EIC
def eic(cip,swfl,rwfl,cc,custom,par):
    par['eiccustom'] = custom
    
    Flow(cip,[swfl,rwfl,cc],
         '''
         laps2d verb=y
         nhx=%(nhx)d nhz=%(nhz)d nht=%(nht)d dht=%(dht)g
         ur=${SOURCES[1]}
         cc=${SOURCES[2]}
         %(eiccustom)s
         ''' %par)

def eic3d(cip,swfl,rwfl,cc,custom,par):
    par['eiccustom'] = custom
   
    Flow(cip,[swfl,rwfl,cc],
         '''
         eic verb=y
         nhx=%(nhx)d nhy=%(nhy)d nhz=%(nhz)d nht=%(nht)d dht=%(dht)g
         ur=${SOURCES[1]}
         cc=${SOURCES[2]}
         %(eiccustom)s
         ''' %par)


# CIC: deconvolution
def dic(imag,swfl,rwfl,eps,custom,par):
    par['diccustom'] = custom
    
    Flow(imag,[swfl,rwfl],
         '''
         math s=${SOURCES[0]} r=${SOURCES[1]} 
         output="-(conj(s)*r)/(conj(s)*s+%g)" |
         transp plane=23 | stack | real
         ''' %eps)

# ------------------------------------------------------------
def wem(imag,sdat,rdat,slow,custom,par):
    
    fWRwem(sdat,swfl,slow,par)
    bWRwem(rdat,rwfl,slow,par)

    cic(imag,swfl,rwfl,custom,par)

# ------------------------------------------------------------
def rtm(imag,sdat,rdat,velo,dens,custom,par):
    
    fWRrtm(sdat,swfl,velo,dens,par)
    bWRrtm(rdat,rwfl,velo,dens,par)

    cic(imag,swfl,rwfl,custom,par)

# ------------------------------------------------------------
# SURVEY-SINKING migration
# ------------------------------------------------------------
#def sinking(par):
#    # surface data
#    sgmig.wflds('d0','cmps',par)
#
#    # datuned data
#    sgmig.datum('d1','sd','d0',par)
#    
#    for k in ('0','1'):
#        s = 's' + k # slowness
#        d = 'd' + k # prestack data
#        i = 'i' + k # prestack image
#        e = 'e' + k # inner offset data
#        z = 'z' + k # inner offset image
#
#        # prestack migration
#        sgmig.image(i,s,d,par)
#        
#        # near offset migration
#        Flow(e,d,'window squeeze=n n3=8')
#        sgmig.image(z,s,e,par)


# ------------------------------------------------------------
# SHOT-PROFILE MIGRATION
# ------------------------------------------------------------
#def profile(par):
#    # surface wavefields
#    spmig.wflds('d0s','d0r','wave','shot',par)
#    
#    # datumed wavefields
#    spmig.datum('d1s','d1r','sd','d0s','d0r',par)
#    
#    for k in ('0','1'):
#        s = 's' + k       # slowness
#        j = 'j' + k       # image
#        ds= 'd' + k + 's' # source   wavefield
#        dr= 'd' + k + 'r' # receiver wavefield
#        
#        spmig.image(j,s,ds,dr,par)
#        
# ------------------------------------------------------------
# RESULTS
# ------------------------------------------------------------
#def igrey(custom,par):
#    return '''
#    grey labelrot=n title="" %s min2=%g max2=%g min1=%g max1=%g
#    ''' % (custom,par['xmin'],par['xmax'],par['zmin'],par['zmax'])

#def result(par):
#
#    for k in ('0','1','2','d','o'):
#        s = 's' + k
#        Result(s,s,'window      | transp |'
#               +igrey('title=s pclip=100 color=j allpos=y',par))
#
#        for k in ('0','1'):
#            z = 'z' + k
#            i = 'i' + k
#            j = 'j' + k
#            
#            Result(z,z,'window n3=1 | transp |'
#                   +igrey('title=z pclip=99',par))
#            Result(i,i,'window n3=1 | transp |'
#                   +igrey('title=i pclip=99',par))
#            Result(j,j,'window      | transp |'
#                   +igrey('title=j pclip=99',par))
#
# ------------------------------------------------------------
