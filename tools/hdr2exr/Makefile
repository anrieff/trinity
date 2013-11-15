# Makefile for the hdr2exr tool, written from scratch by Veselin Georgiev
# (c) Veselin Georgiev, 2013-11-14

OBJECTS = \
	bitmap.o \
	hdr.o \
	rgbe.o \
	pfm.o \
	exr.o \
	util.o\
	main.o 

OPTFLAGS = -g -O0
STDFLAGS = -std=c++0x

INCLUDES = -I/usr/include/OpenEXR
LINKLIBS = -lIlmImf -lHalf -lIex -lSDL -lpthread -lm

all: hdr2exr

.cpp.o:
	g++ -c $< $(INCLUDES) $(OPTFLAGS) $(STDFLAGS)

.c.o:
	gcc -c $< $(INCLUDES) $(OPTFLAGS)

hdr2exr: $(OBJECTS)
	g++ $(OPTFLAGS) $(STDFLAGS) -o hdr2exr $(OBJECTS) $(LINKLIBS)

clean:
	-rm hdr2exr
	-rm *.o

.PHONY: all clean
