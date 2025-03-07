# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := split.exe

BUILD_DIR := ./build
SRC_DIRS := ./src

ifeq ($(origin CC),default)
CC = x86_64-w64-mingw32-gcc
endif

ifeq ($(origin CXX),default)
CXX = x86_64-w64-mingw32-g++
endif

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP -static

EXTRA_FLAGS := $(shell cat compile_flags.txt)

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) compile_flags.txt
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(EXTRA_FLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c compile_flags.txt
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_FLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp compile_flags.txt
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(EXTRA_FLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(TARGET_EXEC)
	$(BUILD_DIR)/$(TARGET_EXEC)

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
