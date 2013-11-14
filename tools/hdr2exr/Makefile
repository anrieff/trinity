# Makefile for the hdr2exr tool, written from scratch by Veselin Georgiev
# (c) Veselin Georgiev, 2013-11-14

OBJECTS = \
	bitmap.o \
	hdr.o \
	pfm.o \
	exr.o \
	main.o

OPTFLAGS = -g -O0 -std=c++0x

INCLUDES = -I/usr/include/OpenEXR
LINKLIBS = -lIlmImf -lHalf -lIex -lm

all: hdr2exr

.cpp.o:
	g++ -c $< $(INCLUDES) $(OPTFLAGS)

hdr2exr: $(OBJECTS)
	g++ $(OPTFLAGS) -o hdr2exr $(OBJECTS) $(LINKLIBS)

clean:
	-rm hdr2exr
	-rm *.o

.PHONY: all clean
