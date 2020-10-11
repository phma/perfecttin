# Version 0.5.1
- PLY export is now colored.
- You can view a TIN colored by gradient or by elevation.
- Pale coloring indicates which areas are holes in the point cloud.
- You can now export a TIN in STL format and specify the size of your 3D printer.
- A bug inserted in the command-line program in version 0.5.0, which resulted in corrupt files or crashes, is fixed.

# Version 0.5.0
- Conversion is much faster.
- A longstanding bug, which could crash the program if you load a point cloud right after opening a PerfectTIN file, is fixed.

# Version 0.4.1
- You can now export a TIN in PLY format.
- The crash in 0.4.0 is fixed.

# Version 0.4.0
- Vertical spikes, which used to appear at the edges and in holes in point clouds, are gone.
- On Windows, this program may crash when run on a 50-million-point cloud or bigger.

# Version 0.3.6
- This version fixes a bug in LandXML export in which the *x* and *y* coordinates were exchanged.
