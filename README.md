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
   2.2. Unpack it anywhere (mine is in F:\develop\SDK\SDL-1.2.15) and create a substitution
        using subst:
                     
                     subst L: F:\develop\SDK
                     
        This will create a virtual drive L:, pointing to your F:\develop\SDK.
        Of course, on your machine the latter path can be different.
   2.3. Copy SDL-1.2.15\bin\SDL.dll in the root directory of the raytracer (i.e., where the *.cbp
        files are).
   2.4. Download the OpenEXR libraries for Windows from
        http://raytracing-bg.net/lib/openexr-devel-trinity.zip
   2.5. Unpack it in the SDK drive (copy to L:, and right click -> "Extract Here...")
        This will create a directory "L:\OpenEXR-mingw"
   3.6. Copy trinity-win32.cbp to trinity.cbp and open it. You're all set.

3. If you're using Linux:
   
   3.1. Ensure you installed the gcc compiler, the SDL-development package and the OpenEXR
        development package ("SDL-devel" and "OpenEXR-devel" on Fedora-based distros,
        "libsdl-dev" and "libopenexr-dev" on Ubuntu et al).
   3.2. Copy trinity-linux.cbp to trinity.cbp and open it. You're all set.
