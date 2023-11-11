/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	The C standard library isn't great. Nonstd is my attempt to fill in some
	of the gaps and increase the convenience of programming in C. It consists
	of three distinct parts:

	- nonstd_arch.h, which contains features that have OS-specific and 
	  architecture-specific code which may not be implemented on all archs.
	- nonstd_base.h, which contains most of the library. It contains some
	  OS-specific code, but nothing that requries a specific compiler or arch.
	- nonstd_str.h, which contains a number of tools to work with strings,
	  and nothing OS or architecture specific.

	Those three parts can be used independently of each other, for example if
	some of the features aren't implemented on your architecture, you can omit
	nonstd_arch. However, nonstd_base has some features that are disabled if
	nonstd_arch isn't being used, and likewise nonstd_str has some disabled
	features if nonstd_base isn't being used. Unless you have reason to do 
	otherwise, just include "nonstd.h", which includes all three.

	If you need to manually include a subset of these, make sure you 
	include them in order: nonstd_arch before nonstd_base before nonstd_str.
	This will make sure as many optional features as possible are enabled.

	These files are single-header libraries (credit to Sean Barrett for the
	idea). They include both the header and the actual definitions in
	a single file. To use them, copy the .h files into your project and 
	define NONSTD_IMPLEMENTATION in exactly one .c file, immediately before 
	you include nonstd.h.

	You can find the documentation for each sub-library in the header portion
	of each file. 
*/

#ifndef NONSTD_H
#define NONSTD_H

#ifndef NONSTD_API
#define NONSTD_API
#endif

#define NONSTD_ARCH_API NONSTD_API
#define NONSTD_BASE_API NONSTD_API
#define NONSTD_STR_API NONSTD_API

#include "nonstd_arch.h"
#include "nonstd_base.h"
#include "nonstd_str.h"

#endif


#ifdef NONSTD_IMPLEMENTATION

#define NONSTD_ARCH_IMPLEMENTATION
#include "nonstd_arch.h"

#define NONSTD_BASE_IMPLEMENTATION
#include "nonstd_base.h"

#define NONSTD_STR_IMPLEMENTATION
#include "nonstd_str.h"

#endif


