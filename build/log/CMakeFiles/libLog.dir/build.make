# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
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

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/lxc/coding/myProject/LinuxWebServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/lxc/coding/myProject/LinuxWebServer/build

# Include any dependencies generated for this target.
include log/CMakeFiles/libLog.dir/depend.make

# Include the progress variables for this target.
include log/CMakeFiles/libLog.dir/progress.make

# Include the compile flags for this target's objects.
include log/CMakeFiles/libLog.dir/flags.make

log/CMakeFiles/libLog.dir/log.cpp.o: log/CMakeFiles/libLog.dir/flags.make
log/CMakeFiles/libLog.dir/log.cpp.o: ../log/log.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/lxc/coding/myProject/LinuxWebServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object log/CMakeFiles/libLog.dir/log.cpp.o"
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/libLog.dir/log.cpp.o -c /home/lxc/coding/myProject/LinuxWebServer/log/log.cpp

log/CMakeFiles/libLog.dir/log.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/libLog.dir/log.cpp.i"
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/lxc/coding/myProject/LinuxWebServer/log/log.cpp > CMakeFiles/libLog.dir/log.cpp.i

log/CMakeFiles/libLog.dir/log.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/libLog.dir/log.cpp.s"
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/lxc/coding/myProject/LinuxWebServer/log/log.cpp -o CMakeFiles/libLog.dir/log.cpp.s

# Object files for target libLog
libLog_OBJECTS = \
"CMakeFiles/libLog.dir/log.cpp.o"

# External object files for target libLog
libLog_EXTERNAL_OBJECTS =

../lib/liblibLog.a: log/CMakeFiles/libLog.dir/log.cpp.o
../lib/liblibLog.a: log/CMakeFiles/libLog.dir/build.make
../lib/liblibLog.a: log/CMakeFiles/libLog.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/lxc/coding/myProject/LinuxWebServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../lib/liblibLog.a"
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && $(CMAKE_COMMAND) -P CMakeFiles/libLog.dir/cmake_clean_target.cmake
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/libLog.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
log/CMakeFiles/libLog.dir/build: ../lib/liblibLog.a

.PHONY : log/CMakeFiles/libLog.dir/build

log/CMakeFiles/libLog.dir/clean:
	cd /home/lxc/coding/myProject/LinuxWebServer/build/log && $(CMAKE_COMMAND) -P CMakeFiles/libLog.dir/cmake_clean.cmake
.PHONY : log/CMakeFiles/libLog.dir/clean

log/CMakeFiles/libLog.dir/depend:
	cd /home/lxc/coding/myProject/LinuxWebServer/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/lxc/coding/myProject/LinuxWebServer /home/lxc/coding/myProject/LinuxWebServer/log /home/lxc/coding/myProject/LinuxWebServer/build /home/lxc/coding/myProject/LinuxWebServer/build/log /home/lxc/coding/myProject/LinuxWebServer/build/log/CMakeFiles/libLog.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : log/CMakeFiles/libLog.dir/depend

