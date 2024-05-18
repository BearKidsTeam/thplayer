# TouHou Player

[![Linux Build](https://github.com/BearKidsTeam/thplayer/actions/workflows/linux.yml/badge.svg)](https://github.com/BearKidsTeam/thplayer/actions/workflows/linux.yml) [![Windows MSYS2 Build](https://github.com/BearKidsTeam/thplayer/actions/workflows/windows-msys2.yml/badge.svg)](https://github.com/BearKidsTeam/thplayer/actions/workflows/windows-msys2.yml) [![macOS Build](https://github.com/BearKidsTeam/thplayer/actions/workflows/macos.yml/badge.svg)](https://github.com/BearKidsTeam/thplayer/actions/workflows/macos.yml)

[Website](https://bearkidsteam.github.io/thplayer/)

TouHou BGM player for all platform.

## Usage

No need for config. Drag the TouHou game program into `thplayer` main window, or click the **Load** button and choose the game folder. `thplayer` will automatically load songs and then it's ready for use!

## Supported game versions

The Touhou game versions we support are: 

``` plain
th06, th07, th08, th09, th10, th11, th12, th13, th14, th15, th16, th17, th18, th19
th095, th125, th128, th143, th165, th185
```

Future TouHou releases should also supported as long as ZUN doesn't change the data format.

This is all made possible by the [thpatch/thtk](https://github.com/thpatch/thtk/) project. Thanks!

## Build

In order to build, you should get the source code first. We are using [thpatch/thtk](https://github.com/thpatch/thtk/) as a git submodule, since the *download as zip* option will **not** pack the submodules inside the zip file, you should use `git clone --recurse-submodules https://github.com/BearKidsTeam/thplayer.git` instead.

(Tip: If you forget to use `--recurse-submodules` when you clone this repo, you need do `git submodule update --init --recursive` then)

### Dependencies

Build system:

- CMake

This application requires the following dependencies:

- Qt 6 (with `qtmultimedia` module)
- ICU

You can get those dependencies from your system package manager, use something like `vcpkg` and `conan`, or you can also build and install those dependencies manually if preferred. Whatever which approach you use, please ensure CMake can use `find_package` to find those dependencies.

### Build using `cmake`

Regular `cmake` build steps applies.

``` shell
# Clone the repo
git clone --recurse-submodules https://github.com/BearKidsTeam/thplayer.git && cd thplayer
# Configure
cmake . -B build
# Build it
cmake --build build -j
```

After that, you'll get the runnable binary inside the build folder. Enjoy!

## License

### BSD 2-Clause License

``` plain
Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain this list
   of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce this
   list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
```
