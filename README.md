# nonstd - my supplement for the C standard library

The standard library is one of the weaker aspects of C.
Many people who write a lot of C end up carrying around little snippets that they've used in the past to solve problems that repeatedly come up. 
This repository is my collection of such snippets. 
All code herein is in the public domain. 

- `nonstd.h` contains useful tools that are written in pure C. It depends on the C standard library, but there's no OS or architecture specific code.
- `nonstd_platform.h` contains platform, compiler, and architecture-specific code. Depends on `nonstd.h` and on the C standard library.

This repository is inspired by the "single-header library" technique (popularized by Sean Barrett https://github.com/nothings/stb).
`nonstd.h` and `nonstd_platform.h` are each single files which contain both the header and the implementation. See the comments at the top of each file for instructions on how to use them.

Contributions are welcome. Most of the existing code was authored by me (Harris Snyder), but I've also added public domain code from other programmers. I've tried to give credit in comments wherever I've done this. 


## Architecture, OS, and compiler support

I'd like to support x86-64, 64-bit arm, and ppc64le.
In practice, x86-64 is the best supported at the moment. 

I try to support as many OSes as possible. 
In practice I test Linux the most, and other OSes are not well tested at the moment.
I'd happily accept patches that aim to improve OS compatibility. 

I mostly test with GCC (including on Windows, see https://github.com/skeeto/w64devkit) or Clang. 
GCC is the only compiler that I guarantee all features will work with.
MSVC support is currently lagging, but I plan to fix this.
