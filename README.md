<img src="https://github.com/DeeJayLSP/godot-mpg/raw/master/editor/icons/VideoStreamMPG.svg" alt="VideoStreamMPG.svg" width=94/>

# Godot MPG

A module that adds MPEG-1 video support to Godot.

This was originally part of a proposal to replace Godot's built-in Theora decoder due to bugs. However, fixes were found, so I released as a separate module instead.

## Usage
`.mpg` files in your project should appear in the FileSystem dock. Drag them into a VideoStreamPlayer and it should work.

Video and audio streams should be MPEG-1 Video and MPEG-1 Audio Layer II (MP2), respectively. This shouldn't be a problem since `.mpg` usually defaults to those.

Like Theora (`.ogv` files), video decoding is done on CPU.

## Building
Clone the repository into `modules/` under the name "mpg". Then proceed to build the engine as per the usual instructions.

In my own tests, this module seems to have a weight of 72 KB in a production template release binary. For comparison, Theora's weight is 104 KB.

## Third-party
This module uses the [PL_MPEG](https://github.com/phoboslab/pl_mpeg) library by Dominic Szablewski with a few patches.

## Known issues
- There seems to be a lag spike when the video starts. This causes an empty frame to be seen at start and when looping.
- Theora is still a little faster despite its bugs. Proposed optimizations in PL_MPEG might fix this in the future.
