/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	nonstd_base.h is part of 'nonstd': an attempt to supplement the C 
	standard library. See the comments in `nonstd.h` for an overview.
*/
#ifndef NONSTD_BASE_H
#define NONSTD_BASE_H

#ifndef NONSTD_BASE_API
#define NONSTD_BASE_API 
#endif

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

// This file has an optional dependency on nonstd_arch.h
// If nonstd_arch is included first, then features from that file are used
// to make certain features thread-safe. If nonstd_arch is not being used,
// then the memory Arenas are not thread-safe and these dummy functions are
// used instead
#ifndef NONSTD_ARCH_H
typedef struct {int _;} TicketMutex;
static void ticket_mutex_lock(TicketMutex *m){(void)m;}
static void ticket_mutex_unlock(TicketMutex *m){(void)m;}
#endif

/* 
   ============================================================================
		TYPEDEFS AND ASSORTED CONVENIENCE MACROS/FUNCTIONS
   ============================================================================
*/
typedef int8_t i8  ;
typedef int16_t i16 ;
typedef int32_t i32 ;
typedef int64_t i64 ;
#define I64(x) ((i64)(x))

typedef uint8_t u8  ;
typedef uint16_t u16 ;
typedef uint32_t u32 ;
typedef uint64_t u64 ;

#ifndef assert
#  ifdef DISABLE_ASSERTIONS
#    define assert(c)
#  elif defined(_MSC_VER)
#    define assert(c) if(!(c)){__debugbreak();}
#  elif defined(__GNUC__) || defined(__clang__)
#    define assert(c) if(!(c)){__builtin_trap();}
#  else 
#    define assert(c) if(!(c)){*(volatile int*)0=0;}
#  endif
#endif

#define INVALID_CODE_PATH() assert(!"Invalid code path");

#define ssizeof(x) ((i64)sizeof(x))
#define COUNT_ARRAY(x) (ssizeof(x)/ssizeof(x[0]))

#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define CONCATENATE_(a,b) a ## b
#define CONCATENATE(a,b) CONCATENATE_(a,b)

#define KILOBYTES(n) (1024ll*n)
#define MEGABYTES(n) (1024ll*KILOBYTES(n))
#define GIGABYTES(n) (1024ll*MEGABYTES(n))

// Macro hackery to allow overloading functions on number of arguments
#define OVERLOAD_2(_1,_2,NAME,...) NAME
#define OVERLOAD_3(_1,_2,_3,NAME,...) NAME
#define OVERLOAD_4(_1,_2,_3,_4,NAME,...) NAME
#define OVERLOAD_5(_1,_2,_3,_4,_5,NAME,...) NAME
#define OVERLOAD_6(_1,_2,_3,_4,_5,_6,NAME,...) NAME

#define MUL64_1(a)           ((i64)(a))
#define MUL64_2(a,b)         ((i64)(a) * (i64)(b))
#define MUL64_3(a,b,c)       ((i64)(a) * (i64)(b) * (i64)(c))
#define MUL64_4(a,b,c,d)     ((i64)(a) * (i64)(b) * (i64)(c) * (i64)(d))
#define MUL64_5(a,b,c,d,e)   ((i64)(a) * (i64)(b) * (i64)(c) * (i64)(d) * (i64)(e))
#define MUL64_6(a,b,c,d,e,f) ((i64)(a) * (i64)(b) * (i64)(c) * (i64)(d) * (i64)(e) * (i64)(f))

#define MUL64(...) OVERLOAD_6(__VA_ARGS__, MUL64_6, MUL64_5, MUL64_4, MUL64_3, MUL64_2, MUL64_1) (__VA_ARGS__)

static i64 
round_up (i64 value, i64 to)
{
	i64 mod = value%to;	
	if(mod) return value+to-(value%to);
	return value;
}

static i64 
round_down (i64 value, i64 to)
{
	return (value/to)*to;
}

static int 
partition (int N, int P, int i)
// If partitioning N items into P partitions, this returns 
// the number of items in the i-th partition (i from 0 to P-1)
{
	assert(i >= 0 && i < P);
	assert(N >= 0);
	assert(P >= 0);

	int r = N % P;
	int m = (N / P) + (r != 0);

	return (r == 0 || i < r)  ?  m  :  m-1;
}

static i64 
partition64 (i64 N, i64 P, i64 i)
// If partitioning N items into P partitions, this returns 
// the number of items in the i-th partition (i from 0 to P-1)
{
	assert(i >= 0 && i < P);
	assert(N >= 0);
	assert(P >= 0);

	i64 r = N % P;
	i64 m = (N / P) + (r != 0);

	return (r == 0 || i < r)  ?  m  :  m-1;
}



/* 
   ============================================================================
		RANDOM NUMBERS
   ============================================================================
*/
NONSTD_BASE_API uint32_t rand_pcg32 (uint64_t state[static 1]); 
// Generate a random uint32, uniform distribution. 
// Permuted congruential generator (32-bit)
									       
NONSTD_BASE_API float randn_pcg32 (uint64_t state[static 1]);
// Generate a random float, normal distribution. 
// Permuted congruential generator (32-bit)

NONSTD_BASE_API float randp_pcg32 (uint64_t state[static 1], float lambda);
// Generate a random float, poisson distribution. 
// Permuted congruential generator (32-bit)



/* 
   ============================================================================
		HASH TABLES AND OTHER DATA STRUCTURES
   ============================================================================
*/
NONSTD_BASE_API int32_t msi_ht_lookup(uint64_t hash, int exp, int32_t idx);
// MSI hash table, see https://nullprogram.com/blog/2022/08/08/
// Compute the next candidate index. Initialize idx to the hash.

NONSTD_BASE_API uint64_t hash_cstr_FNV1a(char *s, int len);
// FNV-1a hash function (useful for strings)

NONSTD_BASE_API uint64_t hash_i64(int64_t x);
// Hashes an int64 with FNV-1a, as though it were a byte string

NONSTD_BASE_API uint64_t hash_u64(uint64_t x);
// Hashes a uint64 with FNV-1a, as though it were a byte string


/* 
   ============================================================================
		SORTING 
   ============================================================================
*/
typedef struct {
	
	int a;
	int b;
	int swap;

	int private[2];

} BubbleSort;

NONSTD_BASE_API int bubblesort_step (BubbleSort *state, int N);

/*
	
	Example test program:

int main(void)
{
	float n[10];
	u64 state = time(NULL);
	for(int i = 0; i < 10; i++) n[i] = randn_pcg32(&state);

	for(int i = 0; i < 10; i++) printf("%f\n",n[i]);
	printf("\n\n");
	
	BubbleSort s = {0};
	while(bubblesort_step(&s, 10)) {

		s.swap = n[s.a] < n[s.b];

		if(s.swap) {
			float tmp = n[s.b];
			n[s.b] = n[s.a];
			n[s.a] = tmp;
		} 
	}

	for(int i = 0; i < 10; i++) printf("%f\n",n[i]);
}

*/


/* 
   ============================================================================
		MEMORY MANAGEMENT
   ============================================================================
*/
NONSTD_BASE_API  void * xmalloc(i64 bytes);
// calls malloc(), calls die() if malloc() fails

NONSTD_BASE_API  void * xrealloc(void *p, i64 bytes);
// calls realloc(), calls die() if realloc() fails

NONSTD_BASE_API  i64 get_total_mem_bytes (void);  
// return total machine memory size in bytes


/*
	Arena object allows you to allocate a bunch of stuff and free it
	all at once, rather than tracking and freeing each individual array.

	Aligns everything to 64 byte boundaries!

	Use it like:

	Arena arena = {0};
	while (not_done) 
	{
		// call some code path that needs to make many
		// allocations for scratch memory
		process_micrograph(&arena, ...);

		// free everything all at once so nothing inside the above code path
		// needs to worry about freeing.
		arena_clear(&arena); 
	}

	NOTE: you don't need to check allocate()'s return value for null, but as a side 
	effect of that if it runs out of memory it just terminates the program.
*/

typedef struct {
	unsigned char *mem;
	i64 reservation;
	i64 committed;
	i64 used;
	TicketMutex mtx; 
	jmp_buf *oom_handler; // if you run out of memory, this will be longjmp'd. if null, then die() is called
} Arena;

NONSTD_BASE_API  void  arena_clear(Arena *a, int reclaim); // deletes everything in the arena but keeps the arena around
NONSTD_BASE_API  void  arena_destroy(Arena *a); // deletes everything in the arena and destroys the arena

NONSTD_BASE_API  int arena_dump_file(Arena *a, char * filename); // dump contents of arena to a file.
NONSTD_BASE_API  i64 arena_dump(i64 bufsz, void *buf, Arena *a); // dump contents of arena to a supplied buffer, returns the required size.
NONSTD_BASE_API  Arena  arena_load_file(char * filename, i64 sz_reserve_extra); // load contents of an arena from a file.

NONSTD_BASE_API  void* allocate(Arena *a, i64 sz); // allocate and zero some memory
NONSTD_BASE_API  void* allocate_empty(Arena *a, i64 sz); // allocate some uninitialized memory
NONSTD_BASE_API  void* allocate_named(Arena *a, i64 sz, char *name, int name_len); // allocate and assign a name
NONSTD_BASE_API  void* allocate_empty_named(Arena *a, i64 sz, char *name, int name_len); // allocate and zero, and assign a name
									//
NONSTD_BASE_API  void* allocation_copy(Arena *a, void *src_data); // copies *src_data from another Arena to a

NONSTD_BASE_API  void* allocation_lookup(Arena *a, char *name, int name_len); // finds an allocation by name

NONSTD_BASE_API  int allocation_check_name(void *p, char *name, int name_len); // check that a previous allocation has the specified name

NONSTD_BASE_API  i64 allocation_size(void *p); // gets size of an allocation that was allocated by a Arena
NONSTD_BASE_API  i64 allocation_capacity(void *p); // gets capacity of an allocation that was allocated by a Arena (may be > size due to alignment padding)

NONSTD_BASE_API  void arena_mem_lock(Arena *a); // locks memory, preventing it from being swapped
NONSTD_BASE_API  void arena_mem_unlock(Arena *a); // unlocks memory, allowing it to be swapped
				 
NONSTD_BASE_API  i64 arena_get_used_memory(Arena *a); // gets the number of bytes in use by the arena
                                     // exists only b/c python can't easily access the .used struct member
NONSTD_BASE_API  i64  arena_checkpoint(Arena *a);
NONSTD_BASE_API  void arena_rollback(Arena *a, i64 checkpoint);

NONSTD_BASE_API  char* allocate_sprintf(Arena *a, char *fmt, ...);
NONSTD_BASE_API  char* allocate_cstrdup(Arena *a, char *cstr);

#define TALLOC_ALIGN 64
#define TALLOC_HEADER_MAGIC 0xa110c8ed // "allocated :)"
typedef struct {
	i64 sz;
	i64 cap;
	u32 magic;
	i8 name_len;
	char padding[TALLOC_ALIGN-21];
	char data[];
} AllocationHeader;
_Static_assert(sizeof(AllocationHeader) == TALLOC_ALIGN, "TALLOC_ALIGN value or size of AllocationHeader is wrong");

NONSTD_BASE_API  AllocationHeader * arena_foreach(Arena *a, i64 *state);

NONSTD_BASE_API  void print_allocation_header(AllocationHeader* x) ;

/*
	
  	ALLOCATE convenience macro usage:

	    float *my_array = 0;
	    ALLOCATE(arena, my_array, N*M);

	which is the same as:

	    float *my_array = allocate(arena, N*M*sizeof(*my_array));

	Note the difference: the macro automatically multiplies by the correct type size,
	allowing you to think in array elements instead of thinking in bytes. 

*/

#define ALLOCATE(arena, array_var, len) array_var = allocate_named((arena), (len)*ssizeof((array_var)[0]), #array_var, 0)
#define ZERO_FILL(array_var, len) memset((array_var), 0, sizeof((array_var)[0])*(len))

/* 
   ============================================================================
		ERROR HANDLING
   ============================================================================
*/



/*
	die(), warn() and logmsg() provide convenient printf-like functions to emit 
	messages. They do the familiar printf-like formatting to build a string, 
	and then they call error_messge(), warning_message(), or info_message() respectively.
	die() and warn() automatically include strerror(errno). die() terminates the program.

	The aforementioned functions are suitable for messages up to 1000 characters. Longer
	messages will be truncated.

	error_message(), warning_message() and info_message() can be overridden by the user.
	In the translation unit where you include util.h, define NONSTD_OVERRIDE_MESSAGE_FUNCTIONS
	and then provide your own definitions for the three. Default implementations are provided.
	The defaults:
	- error_message() sends the message to stderr
	- warning_message() sends the message to stderr
        - info_message() sends the message to stdout	
	- all three append a newline.
*/


NONSTD_BASE_API _Noreturn void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
die (char *fmt, ...);

NONSTD_BASE_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
warn (char *fmt, ...);

NONSTD_BASE_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
logmsg (char *fmt, ...);

NONSTD_BASE_API  void error_message   (char * str);
NONSTD_BASE_API  void warning_message (char * str);
NONSTD_BASE_API  void info_message    (char * str);



/* 
   ============================================================================
		I/O
   ============================================================================
*/
typedef struct {
	i64 len;
	void *mem;
} FileContents;

NONSTD_BASE_API  FileContents platform_read_file(char *filename);

NONSTD_BASE_API  int platform_read_file_into_buffer(i64 buffer_size, void *buffer, i64 *file_size, char *filename);
NONSTD_BASE_API  int platform_read_file_into_arena(Arena *a, void **file_bytes, i64 *file_size, char *filename);
NONSTD_BASE_API  int platform_write_file(char * filename, void *what, size_t bytes);

NONSTD_BASE_API  i64 platform_get_file_size(char *filename);

// Writes out the message from errno or GetLastError with a user-provided message prefix
NONSTD_BASE_API  void errmsg_from_platform(char * prefix);


/* 
   ============================================================================
		PLATFORM-SPECIFIC LOW LEVEL MEMORY MANAGEMENT
   ============================================================================
*/

// returns 0 on failure
NONSTD_BASE_API  void* platform_reserve_mem(size_t size);

// returns 0 on failure, true on success.
// NOTE: start is rounded DOWN to the page size, and len is rounded UP to the end of the page. 
NONSTD_BASE_API  int platform_unreserve_mem(void *start, size_t len);
NONSTD_BASE_API  int platform_decommit_mem (void* start, size_t len);
NONSTD_BASE_API  int platform_commit_mem   (void* start, size_t len); 
NONSTD_BASE_API  int platform_lock_mem     (void *start, size_t len);
NONSTD_BASE_API  int platform_unlock_mem   (void *start, size_t len);


#endif 
/* 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ----------------------------------------------------------------------------
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

   		END OF HEADER SECTION

		Implementation follows

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ----------------------------------------------------------------------------
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#ifdef NONSTD_BASE_IMPLEMENTATION
#include <string.h>

NONSTD_BASE_API int32_t 
msi_ht_lookup(uint64_t hash, int exp, int32_t idx)
{
	u32 mask = ((u32)1 << exp) - 1;
	u32 step = (hash >> (64 - exp)) | 1;
	return (idx + step) & mask;
}

NONSTD_BASE_API uint64_t 
hash_cstr_FNV1a(char *s, int len)
{
	uint64_t h = 0x2b992ddfa23249d6;
	for(int32_t i = 0; i < len; i++)
	{
		h ^= s[i] & 255;
		h *= 1111111111111111111;
	}
	return h ^ h>>32;
}

NONSTD_BASE_API uint64_t 
hash_i64(int64_t x)
{
	char s[sizeof(x)];
	memcpy(s,&x,sizeof(x));
	return hash_cstr_FNV1a(s,sizeof(x));
}

NONSTD_BASE_API uint64_t 
hash_u64(uint64_t x)
{
	char s[sizeof(x)];
	memcpy(s,&x,sizeof(x));
	return hash_cstr_FNV1a(s,sizeof(x));
}

#include <limits.h>
#include <math.h>

NONSTD_BASE_API uint32_t 
rand_pcg32 (uint64_t state[static 1])
{
	// Pseudorandom number generator - (simplified) Permuted Congruential Generator
	uint64_t m = 0x9b60933458e17d7d; // prime
	uint64_t a = 0xd737232eeccdf7ed; // prime
	state[0] = state[0] * m + a;
	int shift = 29 - (state[0] >> 61);
	return state[0] >> shift;
}


NONSTD_BASE_API float 
randn_pcg32 (uint64_t state[static 1])
{
	const float pi = 3.141592653589793238462643383f;
	const float u32max = (float)UINT32_MAX;
	// standard normal distributed random double generator
	float u1 = rand_pcg32(state);
	float u2 = rand_pcg32(state);
	return sqrtf(-2.0f*logf(u1/u32max)) * cosf(2.0f*pi*(u2/u32max));
}

NONSTD_BASE_API float 
randp_pcg32 (uint64_t state[static 1], float lambda)
{
	const float u32max = (float)UINT32_MAX;
	// poisson distribution random double generator
	// slow for large lambda
	int k = 0; 
	float p = 1;
	float L = expf(-lambda);
	do {
		k++;
		p *= rand_pcg32(state)/u32max;
	} while (p > L);
	return --k;
}

NONSTD_BASE_API int
bubblesort_step (BubbleSort *state, int N)
{
	int *c = &state->private[0];
	int *i = &state->private[1];

	if (state->a || state->b) {
		if(state->swap) *c = 1;
		goto innerloop_continuation;
	}

	do {
		*c = 0;
		for (*i = 1; *i < N; (*i)++) {
			state->a = *i-1;
			state->b = *i;
			return 1;
			innerloop_continuation: continue;
		}
	} while (*c);

	return 0;
}



// marginally-documented feature - supply your own printf implementation!
// but I'm not sure there are actually used everywhere yet
#ifndef xsnprintf
#define xsnprintf(...) snprintf(__VA_ARGS__)
#endif
#ifndef xvsnprintf
#define xvsnprintf(...) vsnprintf(__VA_ARGS__)
#endif 


/* 
   ........................................
		LAZY LIBC CODE THAT COULD BE PLATFORM-SPECIFIC IN THE FUTURE
   ........................................
*/

#include <stdio.h>
#include <stdlib.h>

NONSTD_BASE_API int
platform_write_file(char * filename, void *what, size_t bytes) 
{
	FILE *f = fopen(filename, "wb");

	if (!f) {
		errmsg_from_platform("platform_write_file: fopen");
		return 0; 
	}

	if(bytes != fwrite(what, 1, bytes, f)) {
		errmsg_from_platform("platform_write_file: fwrite");
		return 0; 
	}

	fclose(f);
	return 1;
}

NONSTD_BASE_API i64
platform_get_file_size(char *filename)
{
	FILE *f = fopen(filename, "rb");

	if (!f) {
		errmsg_from_platform("platform_get_file_size: fopen");
		return 0; 
	}

	if(fseek(f, 0, SEEK_END)) {
		errmsg_from_platform("platform_get_file_size: fseek(end)");
		return 0; 
	}
	
	#ifdef _WIN32 // TODO fix
	#define FTELL(x) _ftelli64(x)
	#else
	#define FTELL(x) ftell(x)
	#endif
	
	i64 pos = FTELL(f);
	
	if(pos == -1L) {
		errmsg_from_platform("platform_get_file_size: ftell");
		return 0; 
	}

	return pos;
}

NONSTD_BASE_API FileContents 
platform_read_file(char *filename)
{
	FILE * f = fopen(filename, "rb");
	if(!f) die("couldn't read %s", filename);
	fseek(f, 0, SEEK_END);
	i64 len = FTELL(f);
	fseek(f, 0, SEEK_SET);
	void * mem = malloc(len);
	if(!mem) die("couldn't allocate %lli bytes", (long long) len);
	fread(mem, 1, len, f);
	fclose(f);

	return (FileContents) {
		.len = len,
		.mem = mem,
	};
}

NONSTD_BASE_API int 
platform_read_file_into_buffer(i64 buffer_size, void *buffer, i64 *file_size, char *filename)
{
	FILE *f = fopen(filename, "rb");

	if (!f) {
		errmsg_from_platform("platform_read_file_into_buffer: fopen");
		return 0; 
	}

	if(fseek(f, 0, SEEK_END)) {
		errmsg_from_platform("platform_read_file_into_buffer: fseek(end)");
		return 0; 
	}
	
	i64 pos = FTELL(f);
	
	if(pos == -1L) {
		errmsg_from_platform("platform_read_file_into_buffer: ftell");
		return 0; 
	}
	*file_size = pos;

	if(fseek(f, 0, SEEK_SET)) {
		errmsg_from_platform("platform_read_file_into_buffer: fseek(start)");
		return 0; 
	}

	if(*file_size <= buffer_size) {
		if(*file_size != (i64)fread(buffer, 1, *file_size, f)) {
			errmsg_from_platform("platform_read_file_into_buffer: fread");
			return 0; 
		}
	}

	fclose(f);
	return 1;
}

NONSTD_BASE_API int 
platform_read_file_into_arena(Arena *a, void **file_bytes, i64 *file_size, char *filename)
{
	FILE *f = fopen(filename, "rb");

	if (!f) {
		errmsg_from_platform("platform_read_file_into_arena: fopen");
		return 0; 
	}

	if(fseek(f, 0, SEEK_END)) {
		errmsg_from_platform("platform_read_file_into_arena: fseek(end)");
		return 0; 
	}
	
	i64 pos = FTELL(f);
	
	if(pos == -1L) {
		errmsg_from_platform("platform_read_file_into_arena: ftell");
		return 0; 
	}
	*file_size = pos;

	if(fseek(f, 0, SEEK_SET)) {
		errmsg_from_platform("platform_read_file_into_arena: fseek(start)");
		return 0; 
	}

	*file_bytes = allocate(a, *file_size);

	if(*file_size != (i64)fread(*file_bytes, 1, *file_size, f)) {
		errmsg_from_platform("platform_read_file_into_arena: fread");
		return 0; 
	}

	fclose(f);
	return 1;

}



///  error messages


#include <stdio.h>
#include <inttypes.h>

#ifndef NONSTD_OVERRIDE_MESSAGE_FUNCTIONS
	NONSTD_BASE_API void error_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_BASE_API void warning_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_BASE_API void info_message (char * str)
	{
		fprintf(stdout, "%s\n", str);
		fflush(stdout);
	}
#endif

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>


NONSTD_BASE_API _Noreturn void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
die (char *fmt, ...)
{
	char buf[1000] = {0};
	memcpy(buf,"DIE: ",5);
	va_list args;
	va_start(args, fmt);
	xvsnprintf(buf+5, sizeof(buf)-5, fmt, args);
	va_end(args);
	error_message(buf);
	exit(EXIT_FAILURE);
}

NONSTD_BASE_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
warn (char *fmt, ...)
{
	char buf[1000] = {0};
	memcpy(buf,"WARNING: ",9);
	va_list args;
	va_start(args, fmt);
	xvsnprintf(buf+9, sizeof(buf)-9, fmt, args);
	va_end(args);
	warning_message(buf);
}

NONSTD_BASE_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
logmsg (char *fmt, ...)
{
	char buf[1000]  = {0};
	va_list args;
	va_start(args, fmt);
	xvsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	info_message(buf);
}


/*
   ........................................
		LINUX-SPECIFIC
   ........................................
*/

#if defined(__linux__)
#define _GNU_SOURCE
#include <unistd.h>   // _SC_PAGE_SIZE, etc
NONSTD_BASE_API i64 platform_get_page_size(void)
{
	return sysconf(_SC_PAGE_SIZE);
}

NONSTD_BASE_API i64 get_total_mem_bytes (void) 
{
	// Negative return value = error.
	i64 ps = platform_get_page_size();
	i64 pp = sysconf(_SC_PHYS_PAGES);
	return ps*pp;
}
#endif

/* 
   ........................................
		ALL POSIX OSes
   ........................................
*/
#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <sys/time.h> // gettimeofday
#include <pthread.h>

#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>

NONSTD_BASE_API void 
errmsg_from_platform(char * prefix) 
{
	char msg[128] = {0};
	int e = errno; 
	xsnprintf(msg, sizeof(msg), "%s: %s", prefix, strerror(e));
	error_message(msg);
}

NONSTD_BASE_API void* 
platform_reserve_mem(size_t size)
{
	void* p = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if(p == MAP_FAILED) {
		errmsg_from_platform("platform_reserve_mem: mmap");
		return 0;
	}
	return p;
}

static i64 offset_from_prev_page_boundary(void* addr)
{
	i64 start_of_page = round_down((intptr_t)addr, platform_get_page_size());
	i64 rtn_val = ((intptr_t)addr) - start_of_page;
	assert(rtn_val >= 0);
	return rtn_val;
}


NONSTD_BASE_API int 
platform_commit_mem(void* start, size_t len)
{
	i64 offset = offset_from_prev_page_boundary(start);
	start = ((char*)start)-offset;
	len += offset;

	int rc = mprotect(start, len, PROT_READ | PROT_WRITE);
	if(rc != 0) {
		errmsg_from_platform("platform_commit_mem: mprotect");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_lock_mem(void *start, size_t len)
{
	i64 offset = offset_from_prev_page_boundary(start);
	start = ((char*)start)-offset;
	len += offset;

	int rc = mlock(start, len);
	if(rc != 0) {
		errmsg_from_platform("platform_lock_mem: mlock");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_unlock_mem(void *start, size_t len)
{
	i64 offset = offset_from_prev_page_boundary(start);
	start = ((char*)start)-offset;
	len += offset;

	int rc = munlock(start, len);
	if(rc != 0) {
		errmsg_from_platform("platform_unlock_mem: munlock");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int
platform_decommit_mem(void* start, size_t len)
{
	i64 offset = offset_from_prev_page_boundary(start);
	start = ((char*)start)-offset;
	len += offset;

	int rc = mprotect(start, len, PROT_NONE);
	if(rc != 0) {
		errmsg_from_platform("platform_decommit_mem: mprotect");
		return 0;
	}

	rc = madvise(start, len, MADV_DONTNEED);
	if(rc != 0) {
		errmsg_from_platform("platform_decommit_mem: madvise");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_unreserve_mem(void *start, size_t len)
{
	int rc = munmap(start,len);
	if(rc != 0) {
		errmsg_from_platform("platform_unreserve_mem: munmap");
		return 0;
	}
	return 1;
	
}



/* 
   ........................................
		WINDOWS-SPECIFC
   ........................................
*/
#elif defined(_WIN32)
#include <windows.h>

NONSTD_BASE_API i64 
platform_get_page_size(void)
{
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	return si.dwAllocationGranularity;
}

NONSTD_BASE_API i64 
get_total_mem_bytes (void) 
{
	// Negative return value = error.
	ULONGLONG totalmem_kb = 0;
	if(!GetPhysicallyInstalledSystemMemory(&totalmem_kb)) return -1;
	LONGLONG kb = totalmem_kb;
	return kb * 1024;
}

#include <stdio.h>

NONSTD_BASE_API void 
errmsg_from_platform(char * prefix) {
	char msg[256] = {0};
	unsigned e = GetLastError(); 
	
	xsnprintf(msg, sizeof(msg), "%s: win32 error code %u (0x%x)", prefix, e, e);
	error_message(msg);
}


NONSTD_BASE_API void* 
platform_reserve_mem(size_t size)
{
	void *p = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
	if(p == NULL) {
		errmsg_from_platform("platform_reserve_mem: VirtualAlloc");
		return 0;
	}
	return p;
}


NONSTD_BASE_API int 
platform_commit_mem(void* start, size_t len)
{
	if(NULL == VirtualAlloc(start, len, MEM_COMMIT, PAGE_READWRITE)){
		errmsg_from_platform("platform_commit_mem: VirtualAlloc");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_lock_mem(void *start, size_t len)
{
	if(!VirtualLock(start, len)) {
		errmsg_from_platform("platform_lock_mem: VirtualLock");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_unlock_mem(void *start, size_t len)
{
	if(!VirtualUnlock(start, len)) {
		errmsg_from_platform("platform_unlock_mem: VirtualUnlock");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_decommit_mem(void* start, size_t len)
{
	if(!VirtualFree(start, len, MEM_DECOMMIT)) {
		errmsg_from_platform("platform_decommit_mem: VirtualFree");
		return 0;
	}
	return 1;
}

NONSTD_BASE_API int 
platform_unreserve_mem(void *start, size_t len)
{
	if(!VirtualFree(start, len, MEM_DECOMMIT | MEM_RELEASE)) {
		errmsg_from_platform("platform_unreserve_mem: VirtualFree");
		return 0;
	}
	return 1;
}

// end of windows OS-specific code
#endif

/* 
   ........................................
		OS AGNOSTIC
   ........................................
*/

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>

#include <math.h>


NONSTD_BASE_API void * 
xmalloc(i64 bytes) 
{
	void *p = malloc(bytes);
	if(!p) die("xmalloc failed to allocate %lli bytes", (long long) bytes);
	memset(p,0,bytes);
	return p;
}

NONSTD_BASE_API void * 
xrealloc(void *p, i64 bytes)
{
	p = realloc(p,bytes);
	if(!p) die("xrealloc failed to allocate %lli bytes", (long long) bytes);
	return p;
}


static AllocationHeader * 
get_header(void *p) {
	char *x = p;
	AllocationHeader *h = (AllocationHeader*) (x - offsetof(AllocationHeader, data));
	assert(h->magic == TALLOC_HEADER_MAGIC);
	return h;
}

NONSTD_BASE_API int 
allocation_check_name(void *p, char *name, int name_len)
{
	AllocationHeader *h = get_header(p);
	assert(name_len < (i64)sizeof(h->padding));
	return name_len == h->name_len && 0 == memcmp(name,h->padding,name_len);
}


NONSTD_BASE_API i64 
arena_get_used_memory(Arena *a)
{
	return a->used;
}
					    
NONSTD_BASE_API i64 
allocation_size(void *p)
{
	return get_header(p)->sz;
}

NONSTD_BASE_API i64 
allocation_capacity(void *p)
{
	return get_header(p)->cap;
}

NONSTD_BASE_API i64 
arena_checkpoint(Arena *a)
{
	return a->used;
}

NONSTD_BASE_API void 
arena_rollback(Arena *a, i64 checkpoint)
{
	assert(checkpoint <= a->used);
	ticket_mutex_lock(&a->mtx);
	a->used = checkpoint;
	ticket_mutex_unlock(&a->mtx);
}

static void* 
allocate_named_ (Arena *a, i64 sz_, char *name, int name_len) 
{
	i64 cap_for_header = round_up((i64)sz_, TALLOC_ALIGN);
	i64 sz = cap_for_header + sizeof(AllocationHeader);

	if(name_len == 0 && name != 0) name_len = strlen(name);

	static AllocationHeader AllocationHeader_dummy = {0};
	assert(name_len <= (i64)sizeof(AllocationHeader_dummy.padding));

	ticket_mutex_lock(&a->mtx);

	if(a->reservation == 0) a->reservation = GIGABYTES(20);

	if(!a->mem) {
		void *p = platform_reserve_mem(a->reservation);
		if(!p) die("Couldn't reserve %" PRIi64 " B of virtual memory", a->reservation);
		assert((intptr_t)p % TALLOC_ALIGN == 0); // TODO make this better
		a->mem = p;
	}

	if(a->used + sz > a->reservation) {
		if(a->oom_handler) longjmp(a->oom_handler[0],1); 
		die("allocate: out of memory (reservation insufficient)"); 
	}

	if(a->used + sz > a->committed) {
		// commit more memory
		i64 needed_amount = a->used + sz - a->committed;
		assert(platform_commit_mem(a->mem + a->committed, needed_amount));
		a->committed += needed_amount;
	}

	AllocationHeader *new_alloc = (AllocationHeader*)(a->mem + a->used);
	a->used += sz;

	new_alloc->sz    = sz_;
	new_alloc->cap   = cap_for_header;
	new_alloc->magic = TALLOC_HEADER_MAGIC;
	new_alloc->name_len = name_len;
	memcpy(new_alloc->padding, name, name_len);

	void *rtn = &new_alloc->data;
	assert((intptr_t)rtn % TALLOC_ALIGN == 0);

	ticket_mutex_unlock(&a->mtx);
	return rtn;
}

NONSTD_BASE_API void* 
allocate_named  (Arena *a, i64 sz_, char *name, int name_len) 
{
	// zeros memory
	void *mem = allocate_named_(a, sz_, name, name_len);
	memset(mem,0,sz_);
	return mem;
}

NONSTD_BASE_API void* 
allocate (Arena *a, i64 sz_) 
{
	// zeros memory
	return allocate_named(a,sz_,0,0);
}


NONSTD_BASE_API void* 
allocate_empty(Arena *a, i64 sz_) 
{
	// leaves memory uninitialized
	return allocate_named_(a,sz_,0,0);
}

NONSTD_BASE_API void* 
allocate_empty_named  (Arena *a, i64 sz_, char *name, int name_len) 
{
	// leaves memory uninitialized
	return allocate_named_(a, sz_, name, name_len);
}



NONSTD_BASE_API void 
arena_clear(Arena *a, int reclaim)
{
	// note to editors: make sure this always works on zero-initialized arenas (={0})
	ticket_mutex_lock(&a->mtx);
	if (reclaim && a->mem) {
		assert(platform_decommit_mem(a->mem, a->committed));
		a->committed = 0;
	}
	a->used = 0;
	ticket_mutex_unlock(&a->mtx);
}

NONSTD_BASE_API void 
arena_destroy(Arena *a)
{
	ticket_mutex_lock(&a->mtx);
	if (a->mem) {
		assert(platform_decommit_mem(a->mem, a->committed));
		assert(platform_unreserve_mem(a->mem, a->reservation));
	}
	TicketMutex m = a->mtx;
	*a = (Arena) {.mtx = m,};
	ticket_mutex_unlock(&a->mtx);
}

NONSTD_BASE_API int 
arena_dump_file(Arena *a, char * filename) 
{
	return platform_write_file(filename, a->mem, a->used);
}


NONSTD_BASE_API i64 
arena_dump(i64 bufsz, void *buf, Arena *a)
{
	i64 cpysz = bufsz < a->used  ?  bufsz  :  a->used;
	assert(cpysz==0 || a->mem);
	memcpy(buf, a->mem, cpysz);
	return a->used;
}

NONSTD_BASE_API Arena 
arena_load_file(char * filename, i64 sz_reserve_extra)
{
	i64 sz = 0;
	if(!platform_read_file_into_buffer(0, 0, &sz, filename)) die("Failed to read %s", filename);

	Arena a = {.reservation=sz+sz_reserve_extra};
	void *p = platform_reserve_mem(a.reservation);

	if(!p) die("Couldn't reserve %" PRIi64 " B of virtual memory", a.reservation);
	assert((intptr_t)p % TALLOC_ALIGN == 0); // TODO make this better
	a.mem = p;

	assert(platform_commit_mem(a.mem, sz));
	a.committed = sz;

	if(!platform_read_file_into_buffer(sz, a.mem, &sz, filename)) die("Failed to read %s", filename);
	a.used = sz;

	return a;
}



NONSTD_BASE_API void *
allocation_copy(Arena *a, void *src_data)
{
	AllocationHeader *src_hdr = get_header(src_data);

	void * dst_data = allocate(a, src_hdr->sz);
	AllocationHeader *dst_hdr = get_header(dst_data);

	memcpy(dst_hdr, src_hdr, sizeof(*dst_hdr));
	memcpy(dst_data, src_data, src_hdr->sz);

	return dst_data;
}

NONSTD_BASE_API void 
arena_mem_lock(Arena *a)
{
	assert(platform_lock_mem(a->mem, a->used));
}

NONSTD_BASE_API void 
arena_mem_unlock(Arena *a)
{
	assert(platform_unlock_mem(a->mem, a->used));
}


NONSTD_BASE_API void *
allocation_lookup(Arena *a, char *name, int name_len)
{
	assert(name);
	if(name_len == 0 && name != 0) name_len = strlen(name);

	static AllocationHeader AllocationHeader_dummy = {0};
	assert(name_len <= (i64)sizeof(AllocationHeader_dummy.padding));

	// for now, easy but garbage search, can improve later with a hash table.
	i64 offset = 0;
	while (offset < a->used)
	{
		AllocationHeader *h = (void*)(a->mem + offset);
		if(name_len == h->name_len && 0 == memcmp(name, h->padding, name_len)) {
			return h->data;
		}	
		offset += sizeof(AllocationHeader) + h->cap;
	}
	return 0;
}

NONSTD_BASE_API char* 
allocate_sprintf(Arena *a, char *fmt, ...)
{
	va_list args1, args2;
	va_start(args1, fmt);
	va_copy(args2, args1);
	int n = 1 + xvsnprintf(0, 0, fmt, args1);
	char *mem = allocate(a, n);
	xvsnprintf(mem, n, fmt, args2);
	va_end(args1);
	va_end(args2);
	return mem;
}

NONSTD_BASE_API char* 
allocate_cstrdup(Arena *a, char *cstr)
{
        if(!string) return 0;
        int len = strlen(string);
        char *mem = allocate(a, len+1);
        memcpy(mem, string, len);
        return mem;
}

NONSTD_BASE_API AllocationHeader * 
arena_foreach(Arena *a, i64 *state)
{
	assert(*state > -1 && *state <= a->used);
	if (*state == a->used) return 0;
	AllocationHeader *h = (AllocationHeader*) (a->mem + *state);
	assert(h->magic == TALLOC_HEADER_MAGIC);
	*state += h->cap + sizeof(*h);
	return h;
}


NONSTD_BASE_API int 
fmt_mem_quantity(i64 sz, char * buf, i64 quantity, int print_if_small) 
{
	if (quantity >= GIGABYTES(1024)) 
		return xsnprintf(buf, sz, "%.3f TiB", ((double)quantity) / GIGABYTES(1024));
	else if (quantity >= GIGABYTES(1)) 
		return xsnprintf(buf, sz, "%.3f GiB", ((double)quantity) / GIGABYTES(1));
	else if (quantity >= MEGABYTES(1)) 
		return xsnprintf(buf, sz, "%.3f MiB", ((double)quantity) / MEGABYTES(1));
	else if (quantity >= KILOBYTES(1)) 
		return xsnprintf(buf, sz, "%.3f KiB", ((double)quantity) / KILOBYTES(1));
	else if (print_if_small)
		return xsnprintf(buf, sz, "%"PRIi64" B", quantity);
	else return 0;
}

NONSTD_BASE_API void 
print_allocation_header(AllocationHeader* x) 
{
	char name_buf[100] = {0};
	if (x->name_len > 0) 
		memcpy(name_buf, x->padding, x->name_len);
	else 
		memcpy(name_buf, "[NO NAME]", 9);

	printf("%s\n\t", name_buf);

	char szbuf[100] = {0};
	printf("sz:  %" PRIi64 " ", x->sz);
	fmt_mem_quantity(100, szbuf, x->sz, 0);
	printf("%s\n\t", szbuf);

	printf("cap: %" PRIi64 " ", x->cap);
	fmt_mem_quantity(100, szbuf, x->cap, 0);
	printf("%s\n\t", szbuf);

	printf("magic: %x\n\tname_len: %"PRIi8"\n\tpadding:", x->magic, x->name_len);
	for(int i = 0; i < COUNT_ARRAY(x->padding); i++) printf(" %.2hhx", x->padding[i]);
	printf("\n");
	fflush(stdout);
}


#endif
