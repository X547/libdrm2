project('libdrm', 'c', 'cpp')

compiler = meson.get_compiler('c')

LocksProj = subproject('Locks')
Locks = LocksProj.get_variable('Locks')

ThreadLinkProj = subproject('ThreadLink')
ThreadLink = ThreadLinkProj.get_variable('ThreadLink')

libbe = compiler.find_library('be')
libdrm0 = dependency('libdrm').partial_dependency(compile_args : true)

libdrm = shared_library(
	'drm',
	[
		'libdrm.cpp',
		'libdrmProxy.cpp',
		'libdrm_common.c',
		'libdrm_device.c',
		'libdrm_trace.c',
		'stub/libdrmStub.cpp',
		'stub/radeon_hd.c',
		'stub/radeon_hd_regs.cpp',
		'stub/memory.cpp',
		'stub/fdMapper.cpp',
	],
	include_directories: [
		'stub',
		'/boot/system/develop/headers/private/shared',
	],
	dependencies: [
		libbe,
		Locks,
		ThreadLink,
		libdrm0
	],
	gnu_symbol_visibility: 'hidden',
	version: '2.4.0',
	install: true,
	install_dir: 'lib'
)
