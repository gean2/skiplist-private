# taken from assignment 2
EXECUTABLE := benchmark
FILES   := benchmark test analysis ghc_benchmark
LOGS	   := logs

all: $(FILES)

###########################################################

ARCH=$(shell uname | sed -e 's/-.*//g')
OBJDIR=objs
CXX=g++ -m64
CXXFLAGS=-O3 -Wall -g -std=c++11 -fopenmp
HOSTNAME=$(shell hostname)

CC = gcc
CFLAGS = -g -O3 -Wall -std=c99 -fopenmp

LIBS       :=
FRAMEWORKS :=

LDLIBS  := $(addprefix -l, $(LIBS))
LDFRAMEWORKS := $(addprefix -framework , $(FRAMEWORKS))

OBJS= $(OBJDIR)/benchmark.o $(OBJDIR)/utils.o $(OBJDIR)/test.o $(OBJDIR)/analysis.o $(OBJDIR)/ghc_benchmark.o 

.PHONY: dirs clean

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)/

clean:
		rm -rf $(OBJDIR) *~ $(FILES) $(LOGS) *.ppm

export: $(EXFILES)
	cp -p $(EXFILES) $(STARTER)


benchmark: dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJDIR)/benchmark.o $(OBJDIR)/utils.o $(LDFLAGS) $(LDLIBS) $(LDFRAMEWORKS)

test: dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJDIR)/test.o  $(LDFLAGS) $(LDLIBS) $(LDFRAMEWORKS)

analysis: dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJDIR)/analysis.o $(OBJDIR)/utils.o  $(LDFLAGS) $(LDLIBS) $(LDFRAMEWORKS)

ghc_benchmark: dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJDIR)/ghc_benchmark.o $(OBJDIR)/utils.o  $(LDFLAGS) $(LDLIBS) $(LDFRAMEWORKS)


$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: %.cu
		$(NVCC) $< $(NVCCFLAGS) -c -o $@

$(OBJDIR)/%.o: %.c
		$(CC) $< $(CLAGS) -c -o $@