# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/carl/src/OpenDCP

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carl/src/OpenDCP

# Include any dependencies generated for this target.
include libopendcp/CMakeFiles/opendcp-lib.dir/depend.make

# Include the progress variables for this target.
include libopendcp/CMakeFiles/opendcp-lib.dir/progress.make

# Include the compile flags for this target's objects.
include libopendcp/CMakeFiles/opendcp-lib.dir/flags.make

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o: libopendcp/opendcp_xml_sign.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o   -c /home/carl/src/OpenDCP/libopendcp/opendcp_xml_sign.c

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/opendcp_xml_sign.c > CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/opendcp_xml_sign.c -o CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o: libopendcp/opendcp_j2k.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o   -c /home/carl/src/OpenDCP/libopendcp/opendcp_j2k.c

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/opendcp_j2k.c > CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/opendcp_j2k.c -o CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o: libopendcp/opendcp_xml.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_3)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o   -c /home/carl/src/OpenDCP/libopendcp/opendcp_xml.c

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/opendcp_xml.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/opendcp_xml.c > CMakeFiles/opendcp-lib.dir/opendcp_xml.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/opendcp_xml.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/opendcp_xml.c -o CMakeFiles/opendcp-lib.dir/opendcp_xml.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o: libopendcp/opendcp_common.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_4)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/opendcp_common.c.o   -c /home/carl/src/OpenDCP/libopendcp/opendcp_common.c

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/opendcp_common.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/opendcp_common.c > CMakeFiles/opendcp-lib.dir/opendcp_common.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/opendcp_common.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/opendcp_common.c -o CMakeFiles/opendcp-lib.dir/opendcp_common.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o: libopendcp/opendcp_log.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_5)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/opendcp_log.c.o   -c /home/carl/src/OpenDCP/libopendcp/opendcp_log.c

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/opendcp_log.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/opendcp_log.c > CMakeFiles/opendcp-lib.dir/opendcp_log.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/opendcp_log.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/opendcp_log.c -o CMakeFiles/opendcp-lib.dir/opendcp_log.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o: libopendcp/asdcp_intf.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_6)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o -c /home/carl/src/OpenDCP/libopendcp/asdcp_intf.cpp

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/asdcp_intf.cpp > CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.i

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/asdcp_intf.cpp -o CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.s

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o: libopendcp/image/opendcp_tif.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_7)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o   -c /home/carl/src/OpenDCP/libopendcp/image/opendcp_tif.c

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/image/opendcp_tif.c > CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/image/opendcp_tif.c -o CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o: libopendcp/image/opendcp_image.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_8)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o   -c /home/carl/src/OpenDCP/libopendcp/image/opendcp_image.c

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/image/opendcp_image.c > CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/image/opendcp_image.c -o CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.provides.build

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o: libopendcp/CMakeFiles/opendcp-lib.dir/flags.make
libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o: libopendcp/image/opendcp_dpx.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/carl/src/OpenDCP/CMakeFiles $(CMAKE_PROGRESS_9)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o   -c /home/carl/src/OpenDCP/libopendcp/image/opendcp_dpx.c

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.i"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/carl/src/OpenDCP/libopendcp/image/opendcp_dpx.c > CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.i

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.s"
	cd /home/carl/src/OpenDCP/libopendcp && /usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/carl/src/OpenDCP/libopendcp/image/opendcp_dpx.c -o CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.s

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.requires:
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.requires

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.provides: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.requires
	$(MAKE) -f libopendcp/CMakeFiles/opendcp-lib.dir/build.make libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.provides.build
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.provides

libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.provides.build: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.provides.build

# Object files for target opendcp-lib
opendcp__lib_OBJECTS = \
"CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o" \
"CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o" \
"CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o" \
"CMakeFiles/opendcp-lib.dir/opendcp_common.c.o" \
"CMakeFiles/opendcp-lib.dir/opendcp_log.c.o" \
"CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o" \
"CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o" \
"CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o" \
"CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o"

# External object files for target opendcp-lib
opendcp__lib_EXTERNAL_OBJECTS =

libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/build.make
libopendcp/libopendcp.a: libopendcp/CMakeFiles/opendcp-lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX static library libopendcp.a"
	cd /home/carl/src/OpenDCP/libopendcp && $(CMAKE_COMMAND) -P CMakeFiles/opendcp-lib.dir/cmake_clean_target.cmake
	cd /home/carl/src/OpenDCP/libopendcp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/opendcp-lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
libopendcp/CMakeFiles/opendcp-lib.dir/build: libopendcp/libopendcp.a
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/build

libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml_sign.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_j2k.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_xml.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_common.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/opendcp_log.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/asdcp_intf.cpp.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_tif.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_image.c.o.requires
libopendcp/CMakeFiles/opendcp-lib.dir/requires: libopendcp/CMakeFiles/opendcp-lib.dir/image/opendcp_dpx.c.o.requires
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/requires

libopendcp/CMakeFiles/opendcp-lib.dir/clean:
	cd /home/carl/src/OpenDCP/libopendcp && $(CMAKE_COMMAND) -P CMakeFiles/opendcp-lib.dir/cmake_clean.cmake
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/clean

libopendcp/CMakeFiles/opendcp-lib.dir/depend:
	cd /home/carl/src/OpenDCP && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carl/src/OpenDCP /home/carl/src/OpenDCP/libopendcp /home/carl/src/OpenDCP /home/carl/src/OpenDCP/libopendcp /home/carl/src/OpenDCP/libopendcp/CMakeFiles/opendcp-lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : libopendcp/CMakeFiles/opendcp-lib.dir/depend

