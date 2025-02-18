project(
  'chatroom_server',
  'c',
  version : '1.0.0',
  default_options : [
    'debug=true',
    'c_std=c99',
    'buildtype=debug',
    'warning_level=2',
    'werror=true',
    'b_sanitize=address,undefined',
    'optimization=plain',
  ],
)

deps = []
pubinc = []
inc = [include_directories('./')]
src = files(
  'connection.c',
  'chatroom_server.c',
  'utils/xalloc.c'
)

executable(
  'chatroom_server',
  sources : src,
  include_directories : pubinc + inc,
  dependencies : deps,
  c_args : '-D_DEFAULT_SOURCE',
  install : true,
)
