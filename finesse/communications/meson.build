
deps = []
deps += cc.find_library('rt')
deps += cc.find_library('uuid')
deps += cc.find_library('pthread')
deps += dependency('openssl')

libdl = cc.find_library('dl', required: false)
if libdl.found()
   deps += [ libdl ]
endif

finessecommmunications_sources = [
   'access.c',
   'buffer.c',
   'create.c',
   'dirmap.c',
   'fincomm.c',
   'fcc.c',
   'fcs.c',
   'namemap.c',
   'pathsearch.c',
   'serverstat.c',
   'stat.c',
   'statfs.c',
   'testmsg.c',
   'unlink.c',
   ]

finesscommunications = static_library('finessecommuncations',
                                      finessecommmunications_sources,
                                      dependencies: deps,
                                      c_args: ['-DFUSE_USE_VERSION=39', '-Wall', '-Wextra', '-Werror'],
                                      include_directories: [include_dirs, finesse_inc_dirs])

