import Options
from os import unlink, symlink, popen
from os.path import exists

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
	opt.tool_options("compiler_cxx")
	opt.add_option('--with-pulseaudio', action='store', default=1, help='Enable PulseAudio backend support [Default: True]')

def configure(conf):
	conf.check_tool("compiler_cxx")
	conf.check_tool("node_addon")

	if Options.options.with_pulseaudio:
		print "Enable PulseAudio backend support"
		conf.env["WITH_PULSEAUDIO"] = True
		conf.check_cfg(package='libpulse', uselib_store='PULSEAUDIO', args='--cflags --libs')

def build(bld):
	obj = bld.new_task_gen("cxx", "shlib", "node_addon")
	obj.cxxflags = ["-Wall", "-ansi", "-pedantic"]
	obj.target = "jsdx_soundman"
	obj.source = """
		src/jsdx_soundman.cpp
		"""
	obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
	obj.uselib = ""

	if bld.env["WITH_PULSEAUDIO"]:
		obj.cxxflags.append("-DUSE_PULSEAUDIO");

		obj.uselib += " PULSEAUDIO"

def shutdown():
	if Options.commands['clean']:
		if exists('jsdx_soundman.node'): unlink('jsdx_soundman.node')
	else:
		if exists('build/default/jsdx_soundman.node') and not exists('jsdx_soundman.node'):
			symlink('build/default/jsdx_soundman.node', 'jsdx_soundman.node')
