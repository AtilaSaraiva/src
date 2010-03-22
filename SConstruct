EnsureSConsVersion(0, 96)

import bldutil, configure, os

env = Environment()

root = os.environ.get('RSFROOT',os.getcwd())

bindir = os.path.join(root,'bin')
libdir = os.path.join(root,'lib')
incdir = os.path.join(root,'include')
docdir = os.path.join(root,'doc')
spcdir = os.path.join(root,'spec')
mandir = os.path.join(root,'man')

##########################################################################
# CONFIGURATION
##########################################################################
opts = configure.options('config.py')
opts.Add('RSFROOT','RSF installation root',root)
opts.Update(env)

if not os.path.isfile('config.py'):
    conf = Configure(env,custom_tests={'CheckAll':configure.check_all})
    conf.CheckAll()
    env = conf.Finish()

Help(opts.GenerateHelpText(env,cmp))
opts.Save('config.py',env)
config = env.Command('config.py','configure.py','')
env.Precious(config)

env.InstallAs(os.path.join(libdir,'rsfconfig.py'),'config.py')
env.InstallAs(os.path.join(libdir,'rsfconf.py'),'configure.py')
env.InstallAs(os.path.join(libdir,'rsfconf.pyc'),'configure.pyc')
Clean(config,['#/config.log','#/.sconf_temp','configure.pyc'])
env.Alias('config',config)

##########################################################################
# CUSTOM BUILDERS
##########################################################################

env.Append(BUILDERS={'RSF_Include':bldutil.Header,
                     'RSF_Place':configure.Place,
                     'RSF_Pycompile':configure.Pycompile,
                     'RSF_Docmerge':bldutil.Docmerge},
           SCANNERS=[bldutil.Include])

##########################################################################
# FRAMEWORK BUILD
##########################################################################

Depends('bldutil.pyc','configure.pyc')
env.RSF_Pycompile('bldutil.pyc','bldutil.py')
env.InstallAs(os.path.join(libdir,'rsfbld.py'),  'bldutil.py')
env.InstallAs(os.path.join(libdir,'rsfbld.pyc'), 'bldutil.pyc')

system = filter(lambda x: x[0] != '.', os.listdir('system'))
user = filter(lambda x: x[0] != '.' and x != 'nobody', os.listdir('user'))
# Avoid crashing when user places some files in RSFSRC/user
user = filter(lambda x: os.path.isdir(os.path.join('user',x)), user)

SConscript(dirs='framework',name='SConstruct',
           exports='env root bindir libdir docdir spcdir mandir system user')

##########################################################################
# API BUILD
##########################################################################
api = env.get('API',[])
if type(api) is str:
    api = [api]
api.insert(0,'c')

Default('build/include')
Default('build/lib')
for dir in map(lambda x: os.path.join('api',x), api):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',exports='env root libdir incdir')
    Default(build)


##########################################################################
# SYSTEM BUILD
##########################################################################
for dir in map(lambda x: os.path.join('system',x), system):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',exports='env root bindir libdir')
    Default(build)

##########################################################################
# USER BUILD
##########################################################################
for dir in map(lambda x: os.path.join('user',x), user):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',exports='env root bindir libdir')
    Default(build)

##########################################################################
# PLOT BUILD
##########################################################################
pdirs = ('lib','main','test')

for dir in map(lambda x: os.path.join('plot',x), pdirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',
               exports='env root libdir bindir incdir')
    Default(build)

##########################################################################
# PENS BUILD
##########################################################################
pdirs = ('fonts','include','utilities','genlib','main','docs','scripts')

for dir in map(lambda x: os.path.join('pens',x), pdirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',
               exports='env root incdir libdir bindir')
    Default(build)

##########################################################################
# SU BUILD
##########################################################################
sudirs = ('lib','main','plot')

for dir in map(lambda x: os.path.join('su',x), sudirs):
    build = os.path.join('build',dir)
    BuildDir(build,dir)
    SConscript(dirs=build,name='SConstruct',
               exports='env root libdir bindir incdir')
    Default(build)

##########################################################################
# INSTALLATION
##########################################################################

rsfuser = os.path.join(libdir,'rsfuser')
env.Install(rsfuser,'__init__.py')

env.Alias('install',[incdir,bindir,libdir,rsfuser,docdir,spcdir,mandir])
env.Clean('install', rsfuser)

