project('libdrm', 'c', 'cpp')

cc = meson.get_compiler('c')

pkg = import('pkgconfig')

LocksProj = subproject('Locks')
Locks = LocksProj.get_variable('Locks')

ThreadLinkProj = subproject('ThreadLink')
ThreadLink = ThreadLinkProj.get_variable('ThreadLink')

dep_libbe = cc.find_library('be')
dep_libaccelerant = dependency('libaccelerant')

subdir('headers')
subdir('amdgpu')

libdrm = shared_library(
	'drm',
	[
		'libdrm.cpp',
		'libdrm_common.c',
		'libdrm_device.cpp',
		'libdrm_trace.c',
		'Poke.cpp',
		'stub/libdrmStub.cpp',
		'stub/radeon_hd.c',
		'stub/radeon_hd_regs.cpp',
		'stub/memory.cpp',
		'stub/fdMapper.cpp',
	],
	include_directories: [
		'stub',
		inc_libdrm,
		'/boot/system/develop/headers/private/shared',
	],
	dependencies: [
		dep_libbe,
		dep_libaccelerant,
		Locks,
		ThreadLink,
	],
	gnu_symbol_visibility: 'hidden',
	version: '2.4.0',
	install: true
)

pkg.generate(
  libdrm,
  name : 'libdrm',
  subdirs : ['.', 'libdrm'],
  description : 'Userspace interface to kernel DRM services',
)
