# Godot MPG

A module that adds MPEG-1 video support to Godot.

## Usage
Toss a `.mpg` file into a VideoStreamPlayer and it should just work. All decoding is done on CPU.

Note that video and audio streams should be MPEG-1 Video and MPEG-1 Audio Layer II, respectively.


## Building
Clone the repository into `modules/` under the name "mpg". Then proceed to build the engine as usual.

## Third-party
This module uses the [PL_MPEG](https://github.com/phoboslab/pl_mpeg) library by Dominic Szablewski with a few patches.

## Known issues
- Unlike Godot's built-in Theora decoder, this one reads data from a copy in memory instead of the file.
