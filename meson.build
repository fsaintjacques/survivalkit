project('survivalkit', 'c',
	default_options : ['c_std=c11'],
	version: '0.1.0')

ck_dep = dependency('ck')
cmocka_dep = dependency('cmocka')
thread_dep = dependency('threads')

include = include_directories('include')

lib_hdrs = [
	'include/sk_cc.h',
	'include/sk_error.h',
	'include/sk_flag.h',
	'include/sk_healthcheck.h',
	'include/sk_label.h',
	'include/sk_label_default.h',
	'include/sk_lifecycle.h',
	'include/sk_listener.h',
	'include/sk_log.h',
	'include/sk_logger_drv.h',
]

lib_srcs = [
	'src/sk_alloc_priv.h',
	'src/sk_healthcheck.c',
	'src/sk_healthcheck_priv.h',
	'src/sk_label.c',
	'src/sk_lifecycle.c',
	'src/sk_listener.c',
	'src/sk_log.c',
	'src/sk_logger_drv.c',
]

cflags = [
	'-D_POSIX_C_SOURCE=200809L', # required for `strdup`
	'-D_GNU_SOURCE', # required for `syscall(gettid)`
	'-Wall',
	'-Werror',
]

sk = library('survivalkit', lib_srcs,
	c_args: cflags,
	include_directories: include,
	dependencies: [ck_dep],
	install: true)

test_deps = [
	cmocka_dep,
	thread_dep,
]

tests = [
	'sk_healthcheck_test',
	'sk_lifecycle_test',
	'sk_listener_test',
	'sk_log_test',
	'sk_label_test',
]

foreach test_name : tests
	test_bin = executable(test_name, 'test/' + test_name + '.c',
		c_args: cflags,
		include_directories: include,
		link_with: sk,
		dependencies: test_deps)
	test(test_name, test_bin)
endforeach

install_headers(lib_hdrs)

pkg = import('pkgconfig')

pkg.generate(libraries: sk,
	version: meson.project_version(),
	name: 'libsurvivalkit',
	filebase: 'survivalkit',
	description: 'SRE facilities for daemon',
	url: 'https://github.com/fsaintjacques/survivalkit')
