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
CMAKE_COMMAND = /snap/clion/114/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /snap/clion/114/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/spectocol.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/spectocol.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/spectocol.dir/flags.make

CMakeFiles/spectocol.dir/main.c.o: CMakeFiles/spectocol.dir/flags.make
CMakeFiles/spectocol.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/spectocol.dir/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/spectocol.dir/main.c.o   -c "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/main.c"

CMakeFiles/spectocol.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/spectocol.dir/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/main.c" > CMakeFiles/spectocol.dir/main.c.i

CMakeFiles/spectocol.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/spectocol.dir/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/main.c" -o CMakeFiles/spectocol.dir/main.c.s

# Object files for target spectocol
spectocol_OBJECTS = \
"CMakeFiles/spectocol.dir/main.c.o"

# External object files for target spectocol
spectocol_EXTERNAL_OBJECTS =

spectocol: CMakeFiles/spectocol.dir/main.c.o
spectocol: CMakeFiles/spectocol.dir/build.make
spectocol: CMakeFiles/spectocol.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable spectocol"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/spectocol.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/spectocol.dir/build: spectocol

.PHONY : CMakeFiles/spectocol.dir/build

CMakeFiles/spectocol.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/spectocol.dir/cmake_clean.cmake
.PHONY : CMakeFiles/spectocol.dir/clean

CMakeFiles/spectocol.dir/depend:
	cd "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour" "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour" "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug" "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug" "/home/sebastian/MSc Computer Science/Predictive Image Synthesis Technologies/Assignments/SpectrumToColour/cmake-build-debug/CMakeFiles/spectocol.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/spectocol.dir/depend

