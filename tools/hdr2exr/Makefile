# Makefile for the hdr2exr tool, written from scratch by Veselin Georgiev
# (c) Veselin Georgiev, 2013-11-14

OS_UNAME := $(shell uname -s)

OBJECTS = \
	bitmapext.o \
	bitmap.o \
	environment.o \
	hdr.o \
	rgbe.o \
	pfm.o \
	exr.o \
	util.o\
	main.o 

OPTFLAGS = -g -O2
STDFLAGS = -std=c++0x

ifeq ($(OS_UNAME), Linux)
  INCLUDES = -I/usr/include/OpenEXR -I../../src
  LINKLIBS = -lIlmImf -lHalf -lIex -lSDL -lpthread -lm
  BINARY = hdr2exr
else
  INCLUDES = -IL:/SDL-1.2.15/include -IL:/OpenEXR-mingw/include/OpenEXR -I../../src
  LINKLIBS = -mconsole -Wl,--enable-stdcall-fixup -LL:/SDL-1.2.15/lib -LL:/OpenEXR-mingw/lib -lIlmImf -lHalf -lIlmThread -lzlibstatic -lIex -lmingw32 -lm -lSDLmain -lSDL.dll
  BINARY = hdr2exr.exe
endif

all: $(BINARY)

.cpp.o:
	g++ -c $< $(INCLUDES) $(OPTFLAGS) $(STDFLAGS)

.c.o:
	gcc -c $< $(INCLUDES) $(OPTFLAGS)

bitmap.o: ../../src/bitmap.cpp
	g++ -c $< $(INCLUDES) $(OPTFLAGS) $(STDFLAGS)

environment.o: ../../src/environment.cpp
	g++ -c $< $(INCLUDES) $(OPTFLAGS) $(STDFLAGS)

util.o: ../../src/util.cpp
	g++ -c $< $(INCLUDES) $(OPTFLAGS) $(STDFLAGS)

$(BINARY): $(OBJECTS)
	g++ $(OPTFLAGS) $(STDFLAGS) -o hdr2exr $(OBJECTS) $(LINKLIBS)

clean:
	-rm $(BINARY)
	-rm *.o

.PHONY: all clean
