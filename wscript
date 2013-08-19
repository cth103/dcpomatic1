import subprocess
import os
import sys

APPNAME = 'dvdomatic'
VERSION = '0.123'

def options(opt):
    opt.load('compiler_cxx')
    opt.load('winres')

    opt.add_option('--enable-debug', action='store_true', default=False, help='build with debugging information and without optimisation')
    opt.add_option('--disable-gui', action='store_true', default=False, help='disable building of GUI tools')
    opt.add_option('--target-windows', action='store_true', default=False, help='set up to do a cross-compile to Windows')
    opt.add_option('--static', action='store_true', default=False, help='build statically, and link statically to libdcp and FFmpeg')
    opt.add_option('--magickpp-config', action='store', default='Magick++-config', help='path to Magick++-config')
    opt.add_option('--wx-config', action='store', default='wx-config', help='path to wx-config')

def configure(conf):
    conf.load('compiler_cxx')
    if conf.options.target_windows:
        conf.load('winres')

    conf.env.TARGET_WINDOWS = conf.options.target_windows
    conf.env.DISABLE_GUI = conf.options.disable_gui
    conf.env.STATIC = conf.options.static
    conf.env.VERSION = VERSION
    conf.env.TARGET_OSX = sys.platform == 'darwin'
    conf.env.TARGET_LINUX = not conf.env.TARGET_WINDOWS and not conf.env.TARGET_OSX

    conf.env.append_value('CXXFLAGS', ['-D__STDC_CONSTANT_MACROS', '-msse', '-mfpmath=sse', '-ffast-math', '-fno-strict-aliasing',
                                       '-Wall', '-Wno-attributes', '-Wextra', '-D_FILE_OFFSET_BITS=64'])

    if conf.env.TARGET_WINDOWS:
        conf.env.append_value('CXXFLAGS', ['-DDVDOMATIC_WINDOWS', '-DWIN32_LEAN_AND_MEAN', '-DBOOST_USE_WINDOWS_H', '-DUNICODE',
                                           '-DBOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN'])
        wxrc = os.popen('wx-config --rescomp').read().split()[1:]
        conf.env.append_value('WINRCFLAGS', wxrc)
        if conf.options.enable_debug:
            conf.env.append_value('CXXFLAGS', ['-mconsole'])
            conf.env.append_value('LINKFLAGS', ['-mconsole'])
        conf.check(lib = 'ws2_32', uselib_store = 'WINSOCK2', msg = "Checking for library winsock2")
        conf.check(lib = 'bfd', uselib_store = 'BFD', msg = "Checking for library bfd")
        conf.check(lib = 'dbghelp', uselib_store = 'DBGHELP', msg = "Checking for library dbghelp")
        conf.check(lib = 'iberty', uselib_store = 'IBERTY', msg = "Checking for library iberty")
        conf.check(lib = 'shlwapi', uselib_store = 'SHLWAPI', msg = "Checking for library shlwapi")
        boost_lib_suffix = '-mt'
        boost_thread = 'boost_thread_win32-mt'
    else:
        conf.env.append_value('CXXFLAGS', '-DDVDOMATIC_POSIX')
        conf.env.append_value('CXXFLAGS', '-DPOSIX_LOCALE_PREFIX="%s/share/locale"' % conf.env['PREFIX'])
        conf.env.append_value('CXXFLAGS', '-DPOSIX_ICON_PREFIX="%s/share/dvdomatic"' % conf.env['PREFIX'])
        conf.env.append_value('CXXFLAGS', '-D__STDC_LIMIT_MACROS')
        boost_lib_suffix = ''
        boost_thread = 'boost_thread'
        conf.env.append_value('LINKFLAGS', '-pthread')

    if conf.env.TARGET_LINUX:
        # libxml2 seems to be linked against this on Ubuntu but it doesn't mention it in its .pc file
        conf.env.append_value('LIB', 'lzma')
        conf.env.append_value('CXXFLAGS', '-DDVDOMATIC_LINUX')

    if conf.env.TARGET_OSX:
        conf.env.append_value('CXXFLAGS', '-DDVDOMATIC_OSX')
        conf.env.append_value('LINKFLAGS', '-headerpad_max_install_names')

    if conf.options.enable_debug:
        conf.env.append_value('CXXFLAGS', ['-g', '-DDVDOMATIC_DEBUG'])
    else:
        conf.env.append_value('CXXFLAGS', '-O2')

    if not conf.options.static:
        conf.check_cfg(package = 'libdcp', atleast_version = '0.73', args = '--cflags --libs', uselib_store = 'DCP', mandatory = True)
        # Remove erroneous escaping of quotes from xmlsec1 defines
        conf.env.DEFINES_DCP = [f.replace('\\', '') for f in conf.env.DEFINES_DCP]
        conf.check_cfg(package = 'libavformat', args = '--cflags --libs', uselib_store = 'AVFORMAT', mandatory = True)
        conf.check_cfg(package = 'libavfilter', args = '--cflags --libs', uselib_store = 'AVFILTER', mandatory = True)
        conf.check_cfg(package = 'libavcodec', args = '--cflags --libs', uselib_store = 'AVCODEC', mandatory = True)
        conf.check_cfg(package = 'libavutil', args = '--cflags --libs', uselib_store = 'AVUTIL', mandatory = True)
        conf.check_cfg(package = 'libswscale', args = '--cflags --libs', uselib_store = 'SWSCALE', mandatory = True)
        conf.check_cfg(package = 'libswresample', args = '--cflags --libs', uselib_store = 'SWRESAMPLE', mandatory = False)
        conf.check_cfg(package = 'libpostproc', args = '--cflags --libs', uselib_store = 'POSTPROC', mandatory = True)
    else:
        # This is hackio grotesquio for static builds (ie for .deb packages).  We need to link some things
        # statically and some dynamically, or things get horribly confused and the dynamic linker (I think)
        # crashes.  These calls do what the check_cfg calls would have done, but specify the
        # different bits as static or dynamic as required.  It'll break if you look at it funny, but
        # I think anyone else who builds would do so dynamically.
        conf.env.HAVE_CXML = 1
        conf.env.STLIB_CXML = ['cxml']
        conf.env.HAVE_DCP = 1
        conf.env.STLIB_DCP = ['dcp', 'asdcp-libdcp', 'kumu-libdcp']
        conf.env.LIB_DCP = ['glibmm-2.4', 'xml++-2.6', 'ssl', 'crypto', 'xmlsec1', 'xmlsec1-openssl', 'xslt', 'bz2']
        conf.check_cfg(package='libxml++-2.6', args='--cflags --libs', uselib_store='DCP', mandatory=True)
        conf.env.HAVE_AVFORMAT = 1
        conf.env.STLIB_AVFORMAT = ['avformat']
        conf.env.HAVE_AVFILTER = 1
        conf.env.STLIB_AVFILTER = ['avfilter', 'swresample']
        conf.env.HAVE_AVCODEC = 1
        conf.env.STLIB_AVCODEC = ['avcodec']
        conf.env.LIB_AVCODEC = ['z']
        conf.env.HAVE_AVUTIL = 1
        conf.env.STLIB_AVUTIL = ['avutil']
        conf.env.HAVE_SWSCALE = 1
        conf.env.STLIB_SWSCALE = ['swscale']
        conf.env.HAVE_SWRESAMPLE = 1
        conf.env.STLIB_SWRESAMPLE = ['swresample']
        conf.env.HAVE_POSTPROC = 1
        conf.env.STLIB_POSTPROC = ['postproc']

        # This doesn't seem to be set up, and we need it otherwise resampling support
        # won't be included.  Hack upon a hack, obviously
        conf.env.append_value('CXXFLAGS', ['-DHAVE_SWRESAMPLE=1'])

    conf.check_cfg(package = 'sndfile', args = '--cflags --libs', uselib_store = 'SNDFILE', mandatory = True)
    conf.check_cfg(package = 'glib-2.0', args = '--cflags --libs', uselib_store = 'GLIB', mandatory = True)

    if conf.env.TARGET_LINUX:
        conf.check_cfg(package='liblzma', args='--cflags --libs', uselib_store='LZMA', mandatory=True)
        if conf.env.STATIC:
            conf.check_cfg(package='gtk+-2.0', args='--cflags --libs', uselib_store='GTK', mandatory=True)
        else:
            # On Linux we need to be able to include <gtk/gtk.h> to check GTK's version
            conf.check_cfg(package='gtk+-2.0', args='--cflags', uselib_store='GTK', mandatory=True)

    conf.check_cfg(package = '', path = conf.options.magickpp_config, args = '--cppflags --cxxflags --libs', uselib_store = 'MAGICK', mandatory = True)

    if conf.options.static:
        conf.check_cc(fragment = """
    			#include <stdio.h>\n
			#include <openjpeg.h>\n
			int main () {\n
			void* p = (void *) opj_image_create;\n
			return 0;\n
			}
			""", msg = 'Checking for library openjpeg', stlib = 'openjpeg', uselib_store = 'OPENJPEG')
    else:
        # Only 1.5.0 and 1.5.1 have been tested.
        conf.check_cfg(package = 'libopenjpeg', args = '--cflags --libs', atleast_version = '1.5.0', uselib_store = 'OPENJPEG', mandatory = True)
        conf.check_cfg(package = 'libopenjpeg', args = '--cflags --libs', max_version = '1.5.1', mandatory = True)

    conf.check_cxx(fragment = """
                              #include <boost/version.hpp>\n
                              #if BOOST_VERSION < 104500\n
                              #error boost too old\n
                              #endif\n
                              int main(void) { return 0; }\n
                              """,
                   mandatory = True,
                   msg = 'Checking for boost library >= 1.45',
                   okmsg = 'yes',
                   errmsg = 'too old\nPlease install boost version 1.45 or higher.')

    conf.check_cc(fragment  = """
                              #include <libssh/libssh.h>\n
                              int main () {\n
                              ssh_session s = ssh_new ();\n
                              return 0;\n
                              }
                              """, msg = 'Checking for library libssh', mandatory = True, lib = 'ssh', uselib_store = 'SSH')

    conf.check_cxx(fragment = """
    			      #include <boost/thread.hpp>\n
    			      int main() { boost::thread t (); }\n
			      """, msg = 'Checking for boost threading library',
			      libpath = '/usr/local/lib',
                              lib = [boost_thread, 'boost_system%s' % boost_lib_suffix],
                              uselib_store = 'BOOST_THREAD')

    conf.check_cxx(fragment = """
    			      #include <boost/filesystem.hpp>\n
    			      int main() { boost::filesystem::copy_file ("a", "b"); }\n
			      """, msg = 'Checking for boost filesystem library',
                              libpath = '/usr/local/lib',
                              lib = ['boost_filesystem%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                              uselib_store = 'BOOST_FILESYSTEM')

    conf.check_cxx(fragment = """
    			      #include <boost/date_time.hpp>\n
    			      int main() { boost::gregorian::day_clock::local_day(); }\n
			      """, msg = 'Checking for boost datetime library',
                              libpath = '/usr/local/lib',
                              lib = ['boost_date_time%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                              uselib_store = 'BOOST_DATETIME')

    conf.check_cxx(fragment = """
    			      #include <boost/signals2.hpp>\n
    			      int main() { boost::signals2::signal<void (int)> x; }\n
			      """,
                              msg = 'Checking for boost signals2 library',
                              uselib_store = 'BOOST_SIGNALS2')

    conf.check_cc(fragment = """
                             #include <glib.h>
                             int main() { g_format_size (1); }
                             """, msg = 'Checking for g_format_size ()',
                             uselib = 'GLIB',
                             define_name = 'HAVE_G_FORMAT_SIZE',
                             mandatory = False)

    conf.find_program('msgfmt', var='MSGFMT')
    
    datadir = conf.env.DATADIR
    if not datadir:
        datadir = os.path.join(conf.env.PREFIX, 'share')
    
    conf.define('LOCALEDIR', os.path.join(datadir, 'locale'))
    conf.define('DATADIR', datadir)

    conf.recurse('src')
    conf.recurse('test')

def build(bld):
    create_version_cc(VERSION, bld.env.CXXFLAGS)

    bld.recurse('src')
    bld.recurse('test')
    if bld.env.TARGET_WINDOWS:
        bld.recurse('platform/windows')
    if bld.env.TARGET_LINUX:
        bld.recurse('platform/linux')
    if bld.env.TARGET_OSX:
        bld.recurse('platform/osx')

    for r in ['22x22', '32x32', '48x48', '64x64', '128x128']:
        bld.install_files('${PREFIX}/share/icons/hicolor/%s/apps' % r, 'icons/%s/dvdomatic.png' % r)

    if not bld.env.TARGET_WINDOWS:
        bld.install_files('${PREFIX}/share/dvdomatic', 'icons/taskbar_icon.png')

    bld.add_post_fun(post)

def dist(ctx):
    ctx.excl = """
               TODO core *~ src/wx/*~ src/lib/*~ builds/*~ doc/manual/*~ src/tools/*~ *.pyc .waf* build .git
               deps alignment hacks sync *.tar.bz2 *.exe .lock* *build-windows doc/manual/pdf doc/manual/html
               GRSYMS GRTAGS GSYMS GTAGS
               """

def create_version_cc(version, cxx_flags):
    if os.path.exists('.git'):
        cmd = "LANG= git log --abbrev HEAD^..HEAD ."
        output = subprocess.Popen(cmd, shell=True, stderr=subprocess.STDOUT, stdout=subprocess.PIPE).communicate()[0].splitlines()
        o = output[0].decode('utf-8')
        commit = o.replace ("commit ", "")[0:10]
    else:
        commit = 'release'

    try:
        text =  '#include "version.h"\n'
        text += 'char const * dvdomatic_git_commit = \"%s\";\n' % commit
        text += 'char const * dvdomatic_version = \"%s\";\n' % version

        t = ''
        for f in cxx_flags:
            f = f.replace('"', '\\"')
            t += f + ' '
        text += 'char const * dvdomatic_cxx_flags = \"%s\";\n' % t[:-1]

        print('Writing version information to src/lib/version.cc')
        o = open('src/lib/version.cc', 'w')
        o.write(text)
        o.close()
    except IOError:
        print('Could not open src/lib/version.cc for writing\n')
        sys.exit(-1)
    
def post(ctx):
    if ctx.cmd == 'install':
        ctx.exec_command('/sbin/ldconfig')

def pot(bld):
    bld.recurse('src')

def pot_merge(bld):
    bld.recurse('src')
