# nonstd - my supplement for the C standard library

The standard library is one of the weaker aspects of C.
Many people who write a lot of C end up carrying around little snippets that they've used in the past to solve problems that repeatedly come up. 
This repository is my collection of such snippets, cleaned up for consumption by others. 

Contributions are welcome. Most of the existing code was authored by me (Harris Snyder), but in a few places I've added public domain code from other programmers. 
I've tried to give credit in comments wherever I've done this. 

All code herein is released into the public domain. 


## Repository map

In the `nonstd` folder, you'll find the main library. 
Refer to the comments at the top of `nonstd.h` for details.

In the `numerics` folder, you'll find a separate library for math and numerical computing. 
The comments at the top of `numerics.h` explain much more. 

I chose to keep these two separate becuase:
- Many programs won't need the numerics library
- The `numerics` library leans quite heavily on compiler-specific features. 
  I wouldn't want to prevent someone from using `nonstd` just because `numerics` isn't supported in their environment. 

As a general note, I'm a huge fan of the "single-header library" technique (popularized by Sean Barrett https://github.com/nothings/stb).
Although this repository isn't really a single-header library, 
I divide things into multiple files only when I've carefully thought about it and determined that there's good reason to do so. 

## Architecture, OS, and compiler support

I'd like to support x86-64, 64-bit arm, and ppc64le.
In practice, x86-64 is the best supported at the moment. 
Other architectures aren't currently a priority.

I try to support as many OSes as possible. 
In practice I test Linux the most often, and I test FreeBSD, Windows, and Mac OS occasionally.
I'd happily accept patches that aim to improve OS compatibility. 

I mostly test with GCC (including on Windows, see https://github.com/skeeto/w64devkit) or Clang. 
GCC is the only compiler that I guarantee all features will work with.
MSVC support is currently lagging, but I plan to fix this.
In particular, the `numerics` library currently relies quite a bit on GCC features.
