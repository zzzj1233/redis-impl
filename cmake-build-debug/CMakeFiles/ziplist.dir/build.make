# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.17

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "D:\CodeIde\CLion 2020.3\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "D:\CodeIde\CLion 2020.3\bin\cmake\win\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\zzzj\CLionProjects\redis-impl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/ziplist.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ziplist.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ziplist.dir/flags.make

CMakeFiles/ziplist.dir/src/ziplist.c.obj: CMakeFiles/ziplist.dir/flags.make
CMakeFiles/ziplist.dir/src/ziplist.c.obj: ../src/ziplist.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/ziplist.dir/src/ziplist.c.obj"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\ziplist.dir\src\ziplist.c.obj   -c C:\Users\zzzj\CLionProjects\redis-impl\src\ziplist.c

CMakeFiles/ziplist.dir/src/ziplist.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ziplist.dir/src/ziplist.c.i"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\Users\zzzj\CLionProjects\redis-impl\src\ziplist.c > CMakeFiles\ziplist.dir\src\ziplist.c.i

CMakeFiles/ziplist.dir/src/ziplist.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ziplist.dir/src/ziplist.c.s"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\Users\zzzj\CLionProjects\redis-impl\src\ziplist.c -o CMakeFiles\ziplist.dir\src\ziplist.c.s

CMakeFiles/ziplist.dir/src/sds.c.obj: CMakeFiles/ziplist.dir/flags.make
CMakeFiles/ziplist.dir/src/sds.c.obj: ../src/sds.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/ziplist.dir/src/sds.c.obj"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\ziplist.dir\src\sds.c.obj   -c C:\Users\zzzj\CLionProjects\redis-impl\src\sds.c

CMakeFiles/ziplist.dir/src/sds.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ziplist.dir/src/sds.c.i"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\Users\zzzj\CLionProjects\redis-impl\src\sds.c > CMakeFiles\ziplist.dir\src\sds.c.i

CMakeFiles/ziplist.dir/src/sds.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ziplist.dir/src/sds.c.s"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\Users\zzzj\CLionProjects\redis-impl\src\sds.c -o CMakeFiles\ziplist.dir\src\sds.c.s

CMakeFiles/ziplist.dir/src/util.c.obj: CMakeFiles/ziplist.dir/flags.make
CMakeFiles/ziplist.dir/src/util.c.obj: ../src/util.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/ziplist.dir/src/util.c.obj"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles\ziplist.dir\src\util.c.obj   -c C:\Users\zzzj\CLionProjects\redis-impl\src\util.c

CMakeFiles/ziplist.dir/src/util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/ziplist.dir/src/util.c.i"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\Users\zzzj\CLionProjects\redis-impl\src\util.c > CMakeFiles\ziplist.dir\src\util.c.i

CMakeFiles/ziplist.dir/src/util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/ziplist.dir/src/util.c.s"
	D:\devTools\mingw64\bin\gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\Users\zzzj\CLionProjects\redis-impl\src\util.c -o CMakeFiles\ziplist.dir\src\util.c.s

# Object files for target ziplist
ziplist_OBJECTS = \
"CMakeFiles/ziplist.dir/src/ziplist.c.obj" \
"CMakeFiles/ziplist.dir/src/sds.c.obj" \
"CMakeFiles/ziplist.dir/src/util.c.obj"

# External object files for target ziplist
ziplist_EXTERNAL_OBJECTS =

ziplist.exe: CMakeFiles/ziplist.dir/src/ziplist.c.obj
ziplist.exe: CMakeFiles/ziplist.dir/src/sds.c.obj
ziplist.exe: CMakeFiles/ziplist.dir/src/util.c.obj
ziplist.exe: CMakeFiles/ziplist.dir/build.make
ziplist.exe: CMakeFiles/ziplist.dir/linklibs.rsp
ziplist.exe: CMakeFiles/ziplist.dir/objects1.rsp
ziplist.exe: CMakeFiles/ziplist.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable ziplist.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\ziplist.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ziplist.dir/build: ziplist.exe

.PHONY : CMakeFiles/ziplist.dir/build

CMakeFiles/ziplist.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\ziplist.dir\cmake_clean.cmake
.PHONY : CMakeFiles/ziplist.dir/clean

CMakeFiles/ziplist.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\zzzj\CLionProjects\redis-impl C:\Users\zzzj\CLionProjects\redis-impl C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug C:\Users\zzzj\CLionProjects\redis-impl\cmake-build-debug\CMakeFiles\ziplist.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ziplist.dir/depend

