hdr2exr
=======

This is a small tool which can convert image files (mostly HDR environment maps) from
formats, which trinity doesn't understand, into EXR, which it does.
Also, there's resampling code which can convert a spherical environment into a
cubemap environment and vice-versa (and other transformations are possible as well).

The supported input file formats are HDR (Radiance HDR, aka RGBE),
PFM (used in Paul Debevec's HDR probes), EXR (of course) and BMP (duh).

Usage
-----

Just run "hdr2exr -h" to see the command line options and usage.

How to get it
-------------

If you're using windows, just download the [openexr-devel-trinity.zip](http://raytracing-bg.net/lib/openexr-devel-trinity.zip)
package. The tool is located in the "/bin" folder.

If you're under Linux, read on.

Compiling
---------

The tool has the same prerequisites as trinity. Ensure you have GNU make. Build using the supplied makefile.
Under linux, this is as simple as `make'. On Windows, you need to set up your path:

		PATH=%PATH%;C:\Program Files\CodeBlocks\MinGW\bin

(or other folder where your MinGW is located).

Then, issue `mingw32-make'.
