# Copyright © 2017-2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


libdrm_c_args = cc.get_supported_arguments([
  '-Wsign-compare', '-Werror=undef', '-Werror=implicit-function-declaration',
  '-Wpointer-arith', '-Wwrite-strings', '-Wstrict-prototypes',
  '-Wmissing-prototypes', '-Wmissing-declarations', '-Wnested-externs',
  '-Wpacked', '-Wswitch-enum', '-Wmissing-format-attribute',
  '-Wstrict-aliasing=2', '-Winit-self', '-Winline', '-Wshadow',
  '-Wdeclaration-after-statement', '-Wold-style-definition',
  '-Wno-unused-parameter', '-Wno-attributes', '-Wno-long-long',
  '-Wno-missing-field-initializers'])

config = configuration_data()

config.set10('HAVE_VISIBILITY', cc.has_function_attribute('visibility:hidden'))
config.set10('HAVE_LIBDRM_ATOMIC_PRIMITIVES', true)
config.set10('HAVE_LIB_ATOMIC_OPS', false)

foreach header : ['sys/select.h', 'alloca.h']
  config.set10('HAVE_' + header.underscorify().to_upper(), cc.check_header(header))
endforeach

config_file = configure_file(
  configuration : config,
  output : 'config.h',
)
add_project_arguments('-include', '@0@'.format(config_file), language : 'c')

datadir_amdgpu = join_paths(get_option('prefix'), get_option('datadir'), 'libdrm')

libdrm_amdgpu = library(
  'drm_amdgpu',
  [
    files(
      'amdgpu_asic_id.c',
      'amdgpu_bo.c',
      'amdgpu_cs.c',
      'amdgpu_device.c',
      'amdgpu_gpu_info.c',
      'amdgpu_vamgr.c',
      'amdgpu_vm.c',
      'handle_table.c',
      'amdgpu_device_haiku.cpp',
    ),
    config_file,
  ],
  c_args : [
    libdrm_c_args,
    '-DAMDGPU_ASIC_ID_TABLE="@0@"'.format(join_paths(datadir_amdgpu, 'amdgpu.ids')),
  ],
  include_directories : [inc_libdrm],
  dependencies: [dep_libaccelerant],
  version : '1.0.0',
  install : true,
)

install_data(
  'amdgpu.ids',
  install_mode : 'rw-r--r--',
  install_dir : datadir_amdgpu,
)

install_headers('amdgpu.h', subdir : 'libdrm')

pkg.generate(
  libdrm_amdgpu,
  name : 'libdrm_amdgpu',
  subdirs : ['.', 'libdrm'],
  description : 'Userspace interface to kernel DRM services for amdgpu',
)
