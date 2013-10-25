trinity
=======

Simple raytracer in C++11 for the FMI raytracing course.
The course site is http://raytracing-bg.net/

How to set it up on your machine
--------------------------------

1. Copy the appropriate for your OS trinity-{win32,linux}.cbp to trinity.cbp
   This step is important to avoid source-control thinking that you modified the project file

2. If you're using Windows:

   2.1. Download the SDL-devel package for mingw32 from http://libsdl.org (Download > SDL 1.2)
   2.2.1. Variant 1: Unpack it in C:\SDL-1.2.15 (so that it contains C:\SDL-1.2.15\include, etc.)
                     Then, open your local copy (trinity.cbp copied from trinity-win32.cbp)
                     and replace all cases of "L:" to "C:"
   2.2.2. Variant 2: Unpack it anywhere (mine is in F:\develop\SDK\SDL-1.2.15) and create a 
                     substitution using subst:
                     
                     subst L: F:\develop\SDK
                     
                     This will create a virtual drive L:, pointing to your F:\develop\SDK.
                     Of course, on your machine the latter path can be different.

3. If you're using Linux:
   
   3.1. Ensure you installed the gcc compiler and the SDL-development package (SDL-devel on
        Fedora-based distros, libsdl-dev on Ubuntu et al).
   3.2. Copy trinity-linux.cbp to trinity.cbp and open it. You're all set.
