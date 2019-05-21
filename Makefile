# Makefile for asmlib	2019-05-20 Gabriel Ravier

# Makefile for asmlib function library, g++ version

# Folder names
BINARY_FOLDER ?= bin
OBJECT_FOLDER ?= obj
LIBRARY_FOLDER ?= lib
INCLUDE_FOLDER ?= include
SOURCE_FOLDER ?= src
TESTS_FOLDER ?= tests

# Tool names
AR = gcc-ar

ifeq ($(RELEASE), 1)
	# Optimization flags
	# -O3 : Maximum standard-compliant optimization
	# -flto : Link-time optimizations
	# -s : Strip output file
	CXXFLAGS = -O3 -flto -s -Wl,-flto
	LDFLAGS = -s
	LIBRARY_BASENAME_DEF = asmlib
else
	# Debug flags
	# -Og : Optimize for debugging
	# -g3 : Maximum amount of debugging information
	CXXFLAGS = -Og -g3
	LIBRARY_BASENAME_DEF = asmlibdebug
endif

LIBRARY_BASENAME ?= $(LIBRARY_BASENAME_DEF)
STATIC_LIBRARY_NAME = lib$(LIBRARY_BASENAME).a
SHARED_LIBRARY_NAME = shared/lib$(LIBRARY_BASENAME).so

# Warnings
# -Wall : Common warnings
# -Wextra : More common warnings
CXXFLAGS += -Wall -Wextra

# Base command-line when calling the compiler
# -MMD -MP -MF $@.d : Make the compiler generate dependency files
# -std=c++17 : Accept C++17 code
# -I../$(INCLUDE_FOLDER) : Add include folder to include paths
CXXFLAGS += -MMD -MP -MF $@.d -std=c++17 -I$(INCLUDE_FOLDER)

# Space-separated list of source files without extension
# Add strcpy when we can actually link it
SOURCES = cachesize cputype debugbreak round cpuid rdtsc stricmp divfixedi procname
TESTS = testDataCacheSize testCpuType testDebugBreak testRound testCpuidEx testReadTSC testStricmp testDivFixedI testProcessorName

OBJECTS = $(addprefix $(OBJECT_FOLDER)/$(STATIC_LIBRARY_NAME)/, $(addsuffix .o, $(SOURCES)))
OBJECTS_SHARED = $(addprefix $(OBJECT_FOLDER)/$(SHARED_LIBRARY_NAME)/, $(addsuffix .o, $(SOURCES)))
BINARIES_TESTS = $(addprefix $(TESTS_FOLDER)/$(BINARY_FOLDER)/, $(TESTS))
DEPENDENCIES = $(addprefix $(OBJECT_FOLDER)/$(STATIC_LIBRARY_NAME)/, $(addsuffix .o.d, $(SOURCES)))
DEPENDENCIES_SHARED = $(addprefix $(OBJECT_FOLDER)/$(SHARED_LIBRARY_NAME)/, $(addsuffix .o.d, $(SOURCES)))
DEPENDENCIES_TESTS = $(addprefix $(TESTS_FOLDER)/$(OBJECT_FOLDER)/, $(addsuffix .o.d, $(TESTS)))

# Main target are the .a library file and the shared .so file
all: $(LIBRARY_FOLDER)/$(STATIC_LIBRARY_NAME) $(LIBRARY_FOLDER)/$(SHARED_LIBRARY_NAME) tests
	@echo Build finished without errors !

tests: $(BINARIES_TESTS)

$(TESTS_FOLDER)/$(BINARY_FOLDER)/%: $(TESTS_FOLDER)/$(OBJECT_FOLDER)/%.o $(LIBRARY_FOLDER)/$(STATIC_LIBRARY_NAME)
	@mkdir -p $(@D)
	@echo Making binary $@ for testing...
	@$(CXX) $(CXXFLAGS) -o $@ $< -L$(LIBRARY_FOLDER) -l$(LIBRARY_BASENAME)
	@echo Executing $@...
	@./$@
	@echo Test $@ succeeded !

$(LIBRARY_FOLDER)/$(STATIC_LIBRARY_NAME): $(OBJECTS)
	@mkdir -p $(@D)
	@echo Making archive $@...
	@$(AR) rcs $@ $^
	@echo Made archive $@ !

$(LIBRARY_FOLDER)/$(SHARED_LIBRARY_NAME): $(OBJECTS_SHARED)
	@mkdir -p $(@D)
	@echo Linking shared library $@...
	@$(CXX) $(CXXFLAGS) -shared $^ -o $@
	@echo Linked $@ !

$(TESTS_FOLDER)/$(OBJECT_FOLDER)/%.o: $(TESTS_FOLDER)/%.cpp
	@mkdir -p $(@D)
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) $< -o $@ -c

$(OBJECT_FOLDER)/$(STATIC_LIBRARY_NAME)/%.o: $(SOURCE_FOLDER)/%.cpp
	@mkdir -p $(@D)
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) $< -o $@ -c

$(OBJECT_FOLDER)/$(SHARED_LIBRARY_NAME)/%.o: $(SOURCE_FOLDER)/%.cpp
	@mkdir -p $(@D)
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) $< -o $@ -c -fPIC

# Include dependencies
include $(wildcard $(DEPENDENCIES))
include $(wildcard $(DEPENDENCIES_SHARED))
include $(wildcard $(DEPENDENCIES_TESTS))

clean:
	@rm -rf $(LIBRARY_FOLDER) $(OBJECT_FOLDER) $(TESTS_FOLDER)/$(OBJECT_FOLDER) $(TESTS_FOLDER)/$(BINARY_FOLDER)
