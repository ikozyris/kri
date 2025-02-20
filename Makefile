# g++ (clang works too with -stdlib=libc++)
# C++20 minimum for branching prediction hints: [[likely]], otherwise C++11
CC = g++ --std=c++20

# Compiler flags:
#  -Ofast		Safe as no floating point arithmetic is done
#  -march=native	Add optimization flags specific to the host platform
#  -flto		Link Time Optimizations
#  -pendatic		Comply with the standards
#  -DHIGHLIGHT		Enable syntax highlighting
#  -lncursesw		Links to ncurses library for wide characters (unicode)

OPTIM = -Ofast -flto -s -march=native -DRELEASE
SIZE = -Os -s -flto -fdata-sections -ffunction-sections -Wl,--gc-sections
DEBUG = -g #-DDEBUG
CXXFLAGS = -Wall -Wextra -pedantic-errors $(DEBUG) -DHIGHLIGHT -lncursesw # Debug only
#CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -DHIGHLIGHT -lncursesw

# the build target executable:
TARGET = kri
PATHT = /usr/bin/
# root is not required to install
#PATHT = ~/.local/bin/

# Source files
SRCS = main.cpp \
	utils/key_func.cpp \
	utils/io.cpp \
	screen/highlight.cpp \
	screen/init.cpp \
	utils/sizes.cpp \
	utils/gapbuffer.cpp \
	utils/search.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
build: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# Compile .cpp files
%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

install:
	cp $(TARGET) $(PATHT)$(TARGET)

# Clean up
clean:
	rm $(OBJS) $(TARGET)

# Phony targets
.PHONY: build install clean
