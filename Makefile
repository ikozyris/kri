# g++ (clang works too with -stdlib=libc++)
# C++20 minimum for branching prediction hints: [[likely]], otherwise C++11
CXX = g++ --std=c++20

# Compiler flags:
#  -O3			Safe as no floating point arithmetic is done
#  -DHIGHLIGHT		Enable syntax highlighting
#  -DRELEASE		Don't show debugging info
#  -lncursesw		Links to ncurses library for wide characters (unicode)

OPTIM = -O3 -s -flto -march=native -DRELEASE
SIZE = -Os -s -flto -fdata-sections -ffunction-sections -Wl,--gc-sections
DEBUG = -g #-DDEBUG
#CXXFLAGS = -Wall -Wextra -pedantic-errors $(DEBUG) -DHIGHLIGHT -lncursesw # Debug only
CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -DHIGHLIGHT -lncursesw

# the build target executable
TARGET = kri
PATHT = /usr/bin/
# root is not required to install
#PATHT = ~/.local/bin/

# source files
SRCS = main.cpp \
	utils/key_func.cpp \
	utils/io.cpp \
	screen/highlight.cpp \
	screen/init.cpp \
	utils/sizes.cpp \
	utils/gapbuffer.cpp \
	utils/search.cpp

# object files
OBJS = $(SRCS:.cpp=.o)

# default target
build: $(TARGET)

# link
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

# compile
%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

install:
	cp $(TARGET) $(PATHT)$(TARGET)

clean:
	rm $(OBJS) $(TARGET)

# phony targets
.PHONY: build install clean
