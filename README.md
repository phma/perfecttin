PerfectTIN reads a point cloud in LAS or PLY format and computes a TIN, which approximates the point cloud to within a vertical tolerance. It can then export the TIN in DXF format, LandXML format, Carlson TIN format, or text TIN format.

I wrote this to enable surveyors to certify that a surface is tested for accuracy as specified in "ASPRS Positional Accuracy Standards for Digital Geospatial Data"; the list of tolerances in the GUI is taken from that document.

The .ptin file output by PerfectTIN contains the TIN and the point cloud, and is always in meters. If the conversion is interrupted, there will be a .ptin file (usually two) with an extra number in its name. Choose the one with the smaller number, unless PerfectTIN complains that it was not finished being written. You can then resume the conversion from where it left off.

Before compiling PerfectTIN, if you want it to read PLY files, clone the [plytapus](https://github.com/phma/plytapus) repo and compile and install it. This is Simon Rajotte's code; I just made it installable.

The GUI program needs Qt5; the command-line program needs Boost.

PerfectTIN compiles on Ubuntu Bionic and Eoan, DragonFly 5.5, and Windows 10. It does not compile on RHEL 7 because the CMake is too old. It does compile on Fedora 32, and probably also on RHEL 8. It may crash on Wayland.

If building on MinGW, clone the repo https://github.com/meganz/mingw-std-threads and put it beside this one.

To build the documentation, install Pandoc and TeXLive. If Pandoc is not installed, you'll get the program, but not the documentation. If Pandoc is installed, but TeXLive is not, you'll get an error.

To compile, if you're not developing the program:

1. Create a subdirectory build/ inside the directory where you untarred the source code.
2. cd build
3. cmake ..
4. make

If you are developing the program:

1. Create a directory build/perfecttin outside the directory where you cloned the source code.
2. cd build/perfecttin
3. cmake \<directory where the source code is\>
4. make

PLY and DXF files have no units in them. LAS files may have the unit indicated in the variable-length record, but PerfectTIN ignores it. LandXML files have units in them, but as LandXML does not support the Indian survey foot, PerfectTIN exports LandXML in meters if the unit is the Indian survey foot. You can specify the units on the command line with the -u option or in the GUI with Settings|Configure.
