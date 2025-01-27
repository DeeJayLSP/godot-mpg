<img src="https://github.com/DeeJayLSP/godot-mpg/raw/master/editor/icons/VideoStreamMPG.svg" alt="VideoStreamMPG.svg" width=94/>

# Godot MPG

A module that adds MPEG-1 video support to Godot.

This was originally part of a proposal to replace Godot's built-in Theora decoder due to bugs. However, fixes were found, so I released as a separate module instead.

## Usage
`.mpg` files in your project should appear in the FileSystem dock. Drag them into a VideoStreamPlayer and it should work. All decoding is done on CPU.

Note that video and audio streams should be MPEG-1 Video and MPEG-1 Audio Layer II, respectively. This shouldn't be a problem since `.mpg` usually defaults to those.

## Building
Clone the repository into `modules/` under the name "mpg". Then proceed to build the engine as per the usual instructions.

In my own tests, this module seems to have a weight of 72 KB in a production template release binary. At least this is still better than Theora's 104 KB penalty.

## Third-party
This module uses the [PL_MPEG](https://github.com/phoboslab/pl_mpeg) library by Dominic Szablewski with a few patches.

## Known issues
- A blank frame can be seen when looping.
- Performance issues. Theora is still faster despite its bugs. Proposed optimizations in PL_MPEG might fix this.
