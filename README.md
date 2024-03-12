# nonstd - my supplement for the C standard library

The standard library is one of the weaker aspects of C.
Many people who write a lot of C end up carrying around little snippets that they've used in the past to solve problems that repeatedly come up. 
This repository is my collection of such snippets. 
All code herein is in the public domain. 

- `nonstd.h` contains useful tools that are written in pure C, with no platform or architecture specific code
- `nonstd_platform.h` contains platform, compiler, or architecture-specific code. 

Both files depend on the C standard library. 

Contributions are welcome. Most of the existing code was authored by me (Harris Snyder), but I've also added public domain code from other programmers. I've tried to give credit in comments wherever I've done this. 

As a general note, I'm a huge fan of the "single-header library" technique (popularized by Sean Barrett https://github.com/nothings/stb).
Although this repository isn't really a single-header library, 
I divide things into multiple files only when there's good reason to. 

## Architecture, OS, and compiler support

I'd like to support x86-64, 64-bit arm, and ppc64le.
In practice, x86-64 is the best supported at the moment. 

I try to support as many OSes as possible. 
In practice I test Linux the most, and other OSes are not well tested at the moment.
I'd happily accept patches that aim to improve OS compatibility. 

I mostly test with GCC (including on Windows, see https://github.com/skeeto/w64devkit) or Clang. 
GCC is the only compiler that I guarantee all features will work with.
MSVC support is currently lagging, but I plan to fix this.
