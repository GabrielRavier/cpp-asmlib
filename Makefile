# Makefile for asmlib	2019-05-20 Gabriel Ravier

# Makefile for asmlib function library, g++ version

# Folder names
OBJECT_FOLDER = obj
LIBRARY_FOLDER = lib
INCLUDE_FOLDER = include
SOURCE_FOLDER = src
TESTS_FOLDER = tests

# Tool names
AR = ar

ifeq ($(RELEASE), 1)
	# Optimization flags
	# -O3 : Maximum standard-compliant optimization
	# -flto : Link-time optimizations
	# -s : Strip output file
	CXXFLAGS = -O3 -flto -s
	LDFLAGS = -s
	STATIC_LIBRARY_NAME_DEF = libasmlib.a
	SHARED_LIBRARY_NAME_DEF = shared/libasmlib.so
else
	# Debug flags
	# -Og : Optimize for debugging
	# -g3 : Maximum amount of debugging information
	CXXFLAGS = -Og -g3
	STATIC_LIBRARY_NAME_DEF = libasmlibdebug.a
	SHARED_LIBRARY_NAME_DEF = shared/libasmlibdebug.so
endif

STATIC_LIBRARY_NAME ?= $(STATIC_LIBRARY_NAME_DEF)
SHARED_LIBRARY_NAME ?= $(SHARED_LIBRARY_NAME_DEF)

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
SOURCES = cachesize cputype

OBJECTS = $(addprefix $(OBJECT_FOLDER)/$(STATIC_LIBRARY_NAME)/, $(addsuffix .o, $(SOURCES)))
OBJECTS_SHARED = $(addprefix $(OBJECT_FOLDER)/$(SHARED_LIBRARY_NAME)/, $(addsuffix .o, $(SOURCES)))
DEPENDENCIES := $(addsuffix .d, $(OBJECTS))
DEPENDENCIES = $(addprefix $(OBJECT_FOLDER)/$(STATIC_LIBRARY_NAME)/, $(addsuffix .o.d, $(SOURCES)))
DEPENDENCIES_SHARED := $(addsuffix .d, $(OBJECTS_SHARED))
DEPENDENCIES_SHARED = $(addprefix $(OBJECT_FOLDER)/$(SHARED_LIBRARY_NAME)/, $(addsuffix .o.d, $(SOURCES)))

# Main target are the .a library file and the shared .so file
all: $(LIBRARY_FOLDER)/$(STATIC_LIBRARY_NAME) $(LIBRARY_FOLDER)/$(SHARED_LIBRARY_NAME) tests
	@echo Build finished without errors !

# Does nothing for now
tests:

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

clean:
	@rm -rf $(LIBRARY_FOLDER) $(OBJECT_FOLDER)
