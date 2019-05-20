# Makefile for asmlib	2019-05-20 Gabriel Ravier

# Makefile for asmlib function library, g++ version

# Folder names
OBJECT_FOLDER = obj
LIBRARY_FOLDER = lib
INCLUDE_FOLDER = include
SOURCE_FOLDER = src

# Tool names
AR = ar

ifeq ($(RELEASE), 1)
	# Optimization flags
	# -O3 : Maximum standard-compliant optimization
	# -flto : Link-time optimizations
	# -s : Strip output file
	CXXFLAGS = -O3 -flto -s
	LDFLAGS = -s
	LIBRARY_NAME_DEF = libasmlib.a
else
	# Debug flags
	# -Og : Optimize for debugging
	# -g3 : Maximum amount of debugging information
	CXXFLAGS = -Og -g3
	LIBRARY_NAME_DEF = libasmlibdebug.a
endif

LIBRARY_NAME ?= $(LIBRARY_NAME_DEF)

# Warnings
# -Wall : Common warnings
# -Wextra : More common warnings
CXXFLAGS += -Wall -Wextra

# 7z archive flags
# -t7z		7z archive
# -mx		Level of compression = Maximum
# -myx		Maximum file analysis
# -mt=on	Activate multi-threaded compression
7ZAFLAGS = a -t7z -mx -myx -mt=on

# Base command-line when calling the compiler
# -MMD -MP -MF $@.d : Make the compiler generate dependency files
# -std=c++17 : Accept C++17 code
# -I../$(INCLUDE_FOLDER) : Add include folder to include paths
CXXFLAGS += -MMD -MP -MF $@.d -std=c++17 -I$(INCLUDE_FOLDER)

# Files added to the main archive
FILES_TO_MAIN_ARCHIVE = $(LIBRARY_FOLDER)/$(LIBRARY_NAME) $(INCLUDE_FOLDER)/asmlib.h

# Misc source files
MISC_SOURCES = Makefile

# Space-separated list of source files without extension
SOURCES = cachesize

OBJECTS = $(addprefix $(OBJECT_FOLDER)/$(LIBRARY_NAME)/, $(addsuffix .o, $(SOURCES)))
DEPENDENCIES := $(addsuffix .d, $(OBJS))
DEPENDENCIES = $(addprefix $(OBJECT_FOLDER)/$(LIBRARY_NAME)/, $(addsuffix .o.d, $(SOURCES)))

# Main target is 7z file
all: asmlib.7z
	@echo Build finished without errors !

asmlib.7z: $(FILES_TO_MAIN_ARCHIVE)
	7z $(7ZAFLAGS) $@ $^

asmlibSrc.7z: $(SOURCES) $(MISC_SOURCES)
	7z $(7ZAFLAGS) $@ $^

$(LIBRARY_FOLDER)/$(LIBRARY_NAME): $(OBJECTS)
	@mkdir -p $(@D)
	@echo Making archive $@...
	@$(AR) rcs $@ $^
	@echo Made archive $@ !

$(OBJECT_FOLDER)/$(LIBRARY_NAME)/%.o: $(SOURCE_FOLDER)/%.cpp
	@mkdir -p $(@D)
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) $< -o $@ -c

# Include dependencies
include $(wildcard $(DEPENDENCIES))

clean:
	@rm -rf $(LIBRARY_FOLDER) $(OBJECT_FOLDER)
