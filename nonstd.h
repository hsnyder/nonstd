/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	Nonstd is an attempt to fill in some of the gaps in the C standard library
	and increase the convenience of programming in C.

	nonstd.h is a single-header library. To use it: 
	- Copy it into your project,
	- Include the header as necessary,
	- Do the following in exactly one translation unit:

		#define NONSTD_PLATFORM_IMPLEMENTATION
		#include "nonstd_platform.h"


*/

#ifndef NONSTD_H
#define NONSTD_H

#ifndef NONSTD_API
#define NONSTD_API 
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
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

#define KILOBYTES(n) (1024ll*(n)) // yes yes "kibibyte", I know 
#define MEGABYTES(n) (1024ll*KILOBYTES(n))
#define GIGABYTES(n) (1024ll*MEGABYTES(n))
#define TERABYTES(n) (1024ll*GIGABYTES(n))

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
NONSTD_API uint32_t rand_pcg32 (uint64_t state[1]); 
// Generate a random uint32, uniform distribution. 
// Permuted congruential generator (32-bit)
									       
NONSTD_API float randn_pcg32 (uint64_t state[1]);
// Generate a random float, normal distribution. 
// Permuted congruential generator (32-bit)

NONSTD_API float randp_pcg32 (uint64_t state[1], float lambda);
// Generate a random float, poisson distribution. 
// Permuted congruential generator (32-bit)



/* 
   ============================================================================
		HASH TABLES AND OTHER DATA STRUCTURES
   ============================================================================
*/
NONSTD_API int32_t msi_ht_lookup(uint64_t hash, int exp, int32_t idx);
// MSI hash table, see https://nullprogram.com/blog/2022/08/08/
// Compute the next candidate index. Initialize idx to the hash.

NONSTD_API uint64_t hash_cstr_FNV1a(char *s, int len);
// FNV-1a hash function (useful for strings)

NONSTD_API uint64_t hash_i64(int64_t x);
// Hashes an int64 with FNV-1a, as though it were a byte string

NONSTD_API uint64_t hash_u64(uint64_t x);
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

	int priv[2];

} BubbleSort;

NONSTD_API int bubblesort_step (BubbleSort *state, int N);

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

#ifdef __cplusplus
#define _Noreturn [[noreturn]]
#endif

NONSTD_API _Noreturn void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
die (char *fmt, ...);

NONSTD_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
warn (char *fmt, ...);

NONSTD_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
logmsg (char *fmt, ...);

NONSTD_API  void error_message   (char * str);
NONSTD_API  void warning_message (char * str);
NONSTD_API  void info_message    (char * str);



/* 
   ============================================================================
		STRING TOOLS
   ============================================================================
*/


///////////   GENERAL ASCII TOOLS

NONSTD_API int clean_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where bytes that 
// aren't ASCII-printable or ASCII-whitespace characters are removed.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_API int clean_whitespace_ascii(char *dest, char *src, int src_len);
// Writes into `dest` a modified copy of `src`: 
//   - Bytes that aren't ASCII-printable chars are removed.
//   - All whitespace characters are replaced with ' '.
//   - Consecutive whitespace characters are merged into a single ' '.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_API int strip_whitespace_ascii(char *dest, char *src, int src_len);
// Writes into `dest` a modified copy of `src`: 
//   - Leading and trailing ASCII whitespace chars are removed.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_API void lowercase_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where uppercase letters
// are converted to lowercase. `dest` must be a buffer at least as long
// as `src`. In-place use is safe.

NONSTD_API void uppercase_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where lowercase letters
// are converted to uppercase. `dest` must be a buffer at least as long
// as `src`. In-place use is safe.

NONSTD_API int is_character_in_set(char c, char *set, int len);
// Returns 1/0 if `c` is/isn't found in the string `set`.

NONSTD_API int is_ascii_punctuation(char c);
// Returns 1/0 if `c` is/isn't ASCII punctuation.
// i.e.:  !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

NONSTD_API int is_ascii_whitespace(char c);
// Returns 1/0 if `c` is/isn't ASCII whitespace.
// i.e. 0x20 or: \f\n\r\t\v 

NONSTD_API int is_ascii_alphanumeric(char c);
// Returns 1/0 if `c` is/isn't ASCII alphanumeric.
// i.e. a-zA-Z0-9

NONSTD_API int is_ascii_letter(char c);
// Returns 1/0 if `c` is/isn't an ASCII letter.
// i.e. a-zA-Z

NONSTD_API int is_ascii_lower(char c);
// Returns 1/0 if `c` is/isn't an ASCII lowercase letter.
// i.e. a-z

NONSTD_API int is_ascii_upper(char c);
// Returns 1/0 if `c` is/isn't an ASCII uppercase letter.
// i.e. a-z

NONSTD_API int is_ascii_digit(char c);
// Returns 1/0 if `c` is/isn't an ASCII digit;
// i.e. 0-9

NONSTD_API int is_ascii_hexdigit(char c);
// Returns 1/0 if `c` is/isn't an ASCII hex digit;
// i.e. 0-9a-f

NONSTD_API int is_ascii_control(char c);
// Returns 1/0 if `c` is/isn't an ASCII control character;
// i.e. 0x00-0x1f and 0x7f

NONSTD_API int cstr_endswith(const char *str, const char *ending); 
// checks if a null terminated string ends with the specified (null terminated string) ending

NONSTD_API int cstr_startswith(const char *str, const char *start); 
// checks if a null terminated string starts with the specified (null terminated string) start



///////////   NUMERIC CONVERSION

NONSTD_API int parse_hexdigit(char c);
// Parses the given character as a hexidecimal digit and returns the value,
// or returns -1 if `c` is not a valid hex digit (0-9a-fA-F)

NONSTD_API int parse_hex_ull(char *str, int len, unsigned long long *result);
// Parses a hexidecimal ullong, which must not be '0x' prefixed, must not 
// be '+'/'-' prefixed, and must begin exactly at `str`. Returns the number
// of characters that were used for the conversion (conversion stops if it
// encounters a non-ASCII-hex character), or -1 on overflow. NB a return 
// of zero means couldn't parse anything.
	
NONSTD_API int parse_decimal_ull(char *str, int len, unsigned long long *result);
// Parses a decimal ullong, which must not be '0x' prefixed, must not 
// be '+'/'-' prefixed, and must begin exactly at `str`. Returns the number
// of characters that were used for the conversion (conversion stops if it
// encounters a non-ASCII-hex character), or -1 on overflow. NB a return 
// of zero means couldn't parse anything.
	

///////////   PATTERN MATCHING
// This section contains functions for text pattern matching (patterns are
// similar to regular expressions, but simpler). Pattern syntax is based on Lua
// patterns:
//
//   - %a matches ASCII letters
//   - %c matches ASCII control characters (0x00-0x1f and 0x7f)
//   - %d matches ASCII digits
//   - %l matches lowercase ASCII letters
//   - %p matches ASCII punctuation characters
//   - %s matches ASCII space characters
//   - %u matches uppercase ASCII letters
//   - %w matches alphanumeric characters
//   - %x matches hexadecimal digits
//   - %z matches the null character
//   - uppercased versions of the above match the complement of the 
//     class (e.g. %A matches bytes that are not ASCII letters)
//   - ^  matches the start of the string 
//   - $  matches the end of the string 
//   - .  matches any character
//   - +  matches one or more of the preceeding character/class (greedy)
//   - *  matches zero or more of the preceeding character/class (greedy)
//   - ?  matches zero or one of the preceeding character/class
//   - [  begins a character class (] closes it) (dash doesn't work inside classes yet)
//   - preceeding any of the above special characters with a % sign
//     escapes that character (i.e. matches it literally)
//   - %% matches a literal % character
//   - except as noted above, all characters are matched literally
//
//   Example: "^My name is %a[!.]?" will match strings that start with
//   'My name is ' followed by a string of contiguous letters, and then
//   optionally has a period or exclaimation mark.
//
//   Patterns are normal strings that get compiled down to a type of bytecode
//   that is interpreted at runtime to actually determine the match.


typedef struct
{
	int error;
	// if this is non-zero, it will be either 1 (meaning the program
	// was too large), or it will be a negative number, such that 
	// (-error-1) is the index into the pattern string where the 
	// error was encountered.
	
	int code_size;
	#define PATTERN_MACHINE_MAX_PROGRAM_SIZE 512
	unsigned short code[PATTERN_MACHINE_MAX_PROGRAM_SIZE];

} CompiledStrPattern;

NONSTD_API CompiledStrPattern pattern_compile_ascii(char *pattern, int pattern_len);
// Given a text pattern, produces a bytecode program that can be 
// executed with pattern_match_ascii() to do text matching.
// Sets the error property of the returned struct if something
// goes wrong.

NONSTD_API int pattern_match_ascii(char *string, int string_len, CompiledStrPattern *program, int *match_len);
// Searches `string` for the first occurrence of `pattern`.
// Also sets `match_len` to the length of the match, if applicable.
// Returns -2 if the program contained an error, -1 if the 
// pattern does not match, or the index of the match otherwise.


#ifdef NONSTD_DEBUG
NONSTD_API int debug_dump_program(char *buffer, int buffer_len, CompiledStrPattern *p);
// Disassembles a bytecode program into the provided buffer.
// Returns the buffer size (not including null char) that 
// would be needed to hold the entire disassembly.
#endif


///////////   STRING 'OBJECT'
// The Str struct aims to offer a relatively pain free way to do some string 
// manipulations. Note that string object doesn't necessarily "own" the 
// underlying buffer that stores the characters in the string, and all the 
// functions that operate on Strs leave the underlying buffer unchanged. 

typedef struct {
	char *ptr;
	int len;
} Str;

static Str mkstr(char *ptr, int len) {return (Str){ptr,len};}
#define cstr(string_literal) mkstr((string_literal),(int)(sizeof(string_literal)-1))

NONSTD_API Str str_strip(Str s);
// Returns a copy of s where leading and trailing ASCII whitespace have been removed.

NONSTD_API Str str_split(Str* s, char delim);
// Pops the first substring (delimited by `delim`) off of `s` (modifying it).
// `s` will have zero-length if there's nothing left to pop.

NONSTD_API Str str_split_str(Str* s, Str delim);
// Pops the first substring (delimited by `delim`) off of `s` (modifying it).
// `s` will have zero-length if there's nothing left to pop.

NONSTD_API int str_equals(Str a, Str b);
// Returns 1 if `a` and `b` are equal, 0 otherwise

NONSTD_API int str_startswith(Str s, Str startswith);
// Returns 1 if `s` begins with `startswith`, 0 otherwise

NONSTD_API int str_endswith(Str s, Str endswith);
// Returns 1 if `s` ends with `endswith`, 0 otherwise

NONSTD_API int str_search(Str haystack, Str needle);
// Searches `haystack` for `needle`, returning the index at which is is found
// or -1 if it is not found at all.

NONSTD_API int str_pattern_match(Str *match, Str *string, CompiledStrPattern *program);
// Calls pattern_match_ascii() to match the specified pattern against `string`.
// Updates `string` to point to the text after the match, and sets `match` to
// the actual match. Returns 1 if a match was found, 0 if it was not (or if 
// the program contains an error).

static uint64_t hash_str_FNV1a(Str s) {return hash_cstr_FNV1a(s.ptr, s.len);}
// Hashes a Str with FNV-1a




/* 
   ============================================================================
		LAZY MEMORY MANAGEMENT
   ============================================================================
*/
NONSTD_API  void * xmalloc(i64 bytes);
// calls malloc(), calls die() if malloc() fails

NONSTD_API  void * xrealloc(void *p, i64 bytes);
// calls realloc(), calls die() if realloc() fails



/* 
   ============================================================================
		ARENA MEMORY MANAGEMENT
   ============================================================================

   Some ideas drawn from Chris Wellons's excellent blog (https://nullprogram.com/) 
*/

typedef struct {
	// A simple arena type. The caller is responsible for actually allocating the underlying memory
	char *start; 
	char *one_past_end;

	// Notice we only track the current start, not the original start. This keeps the arena small
	// which may be useful if it is frequently passed by value (which causes memory to be
	// automatically "freed" upon return - great for scratch arenas).
} Arena;

enum {
	// Flags for the alloc function
	ALLOC_NO_ZERO   = 1<<0, // Don't zero the allocated memory
	ALLOC_SOFT_FAIL = 1<<1, // Don't abort if the allocation fails, just return 0
};

NONSTD_API  void *allocate(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags);
// Allocates memory in the arena. Supply the size and alignment of the type you're allocating.
// If you're allocating an array, supply the number of elements in `count` (otherwise, pass 1). 
// The flags are optional and can be zero or a bitwise or of the flags defined above.

#ifndef __cplusplus // because C++ doesn't like automatic conversions from void*

	#define ALLOCATE(a, var, count) \
		((var) = allocate((a), (ptrdiff_t)sizeof((var)[0]), (ptrdiff_t)alignof((var)[0]), (count), 0))
	// Convenience macro for allocating an array in an arena.
	// You can of course pass 1 for the count if you just want a single object.
	// Examples:
	//
	//	    float *my_array = 0;
	//	    ALLOCATE(&arena, my_array, N*M);
	//
	//	    float *other_array = ALLOCATE(&arena, other_array, N*M);

	#define ALLOCATE_EX(a, var, count, flags) \
		((var) = allocate((a), (ptrdiff_t)sizeof((var)[0]), (ptrdiff_t)alignof((var)[0]), (count), (flags)))
	// Extended version of ALLOCATE, which accepts a flags argument to be passed to allocate()

#else  // C++ versions of the above
	#define ALLOCATE(a, var, count) \
		((var) = (decltype(var))allocate((a), (ptrdiff_t)sizeof((var)[0]), (ptrdiff_t)alignof((var)[0]), (count), 0))
	#define ALLOCATE_EX(a, var, count, flags) \
		((var) = (decltype(var))allocate((a), (ptrdiff_t)sizeof((var)[0]), (ptrdiff_t)alignof((var)[0]), (count), (flags)))
#endif

#define ZERO_FILL(array_var, len) memset((array_var), 0, sizeof((array_var)[0])*(len))
// Convenience macro for zero-filling an array.


NONSTD_API  char* allocate_sprintf(Arena *a, int *len, const char *fmt, ...);
// Like `sprintf`, but allocates the string in the arena. The string is null-terminated.
// Also writes the length of the string (excluding NULL) to the optional len parameter, if provided.

NONSTD_API char *allocate_cstrdup(Arena *a, char *s);
// Like `strdup` but allocates the string in the arena. The string source and destination are 
// null-terminated.

NONSTD_API Str allocate_strdup(Arena *a, Str s);
// Copies a string into the specified arena. Destination is null-terminated.


NONSTD_API  Arena malloc_arena(ptrdiff_t cap);
// Creates a new arena with the given capacity, using malloc to allocate the memory.
// This function is for convenience and is best used only when the arena will last for the
// duration of the program. If you need to deallocate the underlying memory, you'll need to 
// retain a copy of the "start" pointer you get from this function, and pass that to free()




/* 
   ============================================================================
		HASH MAPS
   ============================================================================

   Credit to Chris Wellons (https://nullprogram.com/) and NRK (https://nrk.neocities.org/) 
   for the hash map design.

*/


///////////   HASH MAPS  ////////////
// 
// This section offers implementations of a hash map and an ordered hash map which are designed
// to be used with the Arena allocator. These data structures are not thread safe. Both are "intrusive"
// in that they require the user to embed the hash map in their own data structure if the user wishes to 
// store actual values in the hash map. The hash map can be used as a hash set with no need for
// user-defined structures.
//
//   Example usage:
// 
//   typedef stuct MyType {
//       int value;
//       HashMap hm;
//   } MyType;
// 
//   ...
// 
//   HashMap *hm = 0;
//   MyType *p = hash_map_upsert(&hm, "key", &a, MyType);
//   p->value = 666;
//  
// The ordered hash map is similar, but it also maintains a linked list of all the elements in 
// reverse-insertion order. 


// You can supply your own key type, but if you do so you must also supply a hash function, an equals function,
// and a function to copy a key into an areana. By default, the key type is a null terminated c-string (char*),
// and the hash function is FNV-1a.
#ifndef HASH_MAP_KEY_TYPE
  #ifndef NONSTD_HASHMAP_POLICY_USE_NONSTD_STR
    #define HASH_MAP_KEY_TYPE char*
    #define HASH_MAP_KEY_HASH_FN(k)      nonstd_hashmap_hash_cstr(k)
    #define HASH_MAP_KEY_EQUALS_FN(a,b)  nonstd_hashmap_equals_cstr(a,b)
    #define HASH_MAP_KEY_COPY_FN(a, k)   allocate_cstrdup(a, k)
  #else 
    #define HASH_MAP_KEY_TYPE Str
    #define HASH_MAP_KEY_HASH_FN(k)      hash_str_FNV1a(k)
    #define HASH_MAP_KEY_EQUALS_FN(a,b)  str_equals(a,b)
    #define HASH_MAP_KEY_COPY_FN(a, k)   allocate_strdup(a, k)
  #endif 
#else
  #ifndef HASH_MAP_KEY_HASH_FN
    #error "If you're supplying your own HASH_MAP_KEY_TYPE you also must supply a hash function:  uint64_t hashfn(KEY_TYPE k)"
  #endif
  #ifndef HASH_MAP_KEY_EQUALS_FN
    #error "If you're supplying your own HASH_MAP_KEY_TYPE you also must supply your own key comparison function:  int equalsfn(KEY_TYPE a, KEY_TYPE b)"
  #endif
  #ifndef HASH_MAP_KEY_COPY_FN
    #error "If you're supplying your own HASH_MAP_KEY_TYPE you also must supply your own key copy function:  HASH_MAP_KEY cpyfn(Arena a, KEY_TYPE k)"
  #endif
#endif

typedef struct HashMap{
	// Simple intrusive hash map. On it's own, can function as a hash set.
	struct HashMap *child[4];
	HASH_MAP_KEY_TYPE key;
} HashMap;

typedef struct OrderedHashMap{
	// Simple intrusive ordered hash map (tracks elements in reverse insertion order). 
	// On it's own, can function as an ordered hash set.
	struct OrderedHashMap *child[4];
	struct OrderedHashMap *next;
	HASH_MAP_KEY_TYPE key;
} OrderedHashMap;


enum {
	// Flags for the hash map upsert functions
	HASH_MAP_DUP_KEY = 1, // if an insertion is performed, this flag causes HASH_MAP_KEY_COPY_FN to be called.
};

NONSTD_API void *hash_map_upsert_general(
	HashMap **hm, 
	HASH_MAP_KEY_TYPE key, 
	Arena *a, 
	ptrdiff_t offset, 
	ptrdiff_t size, 
	ptrdiff_t align, 
	int flags, 
	int *insert_count);
// "Upsert" = update & insert, in one function.
// If the key is found, the function returns a pointer to the existing element. If the key is not found, 
// the function allocates a new element in the arena and returns a pointer to it. Since the hash map is 
// intrusive, the caller must supply the offset of the hash map within the data structure. The size and
// alignment of the data structure are also required. The flags are optional and can be zero or a bitwise
// OR of the flags defined above. The last parameter insert_count is optional, but if it's supplied, the
// referred integer will be incremented if an insertion is performed. 
// The returned pointer points to the user-defined data structure, not the hash map member within it.
// 
// Calling this function can be a bit cumbersome because of the need to supply sizes and alignments. 
// See the convenience macros below, or, define your own upsert macro along with your data structure. 

#define hash_map_upsert(hm, key, a, Type, member) \
	hash_map_upsert_general(hm, key, a, (ptrdiff_t)offsetof(Type, member), (ptrdiff_t)sizeof(Type), (ptrdiff_t)alignof(Type), 0, 0)
// Convenience macro: you can supply your data structure type and the name of the HashMap member, 
// instead of the size, alignment, anf offset.
// Example:
//	typedef struct { int value; HashMap hm; } MyType;
//	MyType *p = hash_map_upsert(&hm, "key", &a, MyType, hm);

#define hash_map_upsert_ex(hm, key, a, Type, member, flags, insert_count) \
	hash_map_upsert_general(hm, key, a, (ptrdiff_t)offsetof(Type, member), (ptrdiff_t)sizeof(Type), (ptrdiff_t)alignof(Type), (flags), (insert_count))
// Extended version of hash_map_upsert, which accepts a flags argument and an optional insert_count argument.


NONSTD_API void *ordered_hash_map_upsert_general(
	OrderedHashMap **hm,
	OrderedHashMap **list,
	HASH_MAP_KEY_TYPE key,
	Arena *a,
	ptrdiff_t offset,
	ptrdiff_t size,
	ptrdiff_t align,
	int flags,
	int *insert_count);
// This is the analagous upsert function for the ordered hash map. It's used in the same way as the 
// hash_map_upsert function, only that it also maintains a linked list of all the elements in reverse
// insertion order. The head of this list is tracked separately, via the list parameter.
//
// Again, some convenience macros are defined below, but you may want to define your own upsert macro
// to go along with your data structure.

#define ordered_hash_map_upsert(hm, list, key, a, Type, member) \
	ordered_hash_map_upsert_general((hm), (list), (key), (a), (ptrdiff_t)offsetof(Type, member), (ptrdiff_t)sizeof(Type), (ptrdiff_t)alignof(Type), 0, 0)		
// Convenience macro: you can supply your data structure type and the name of the HashMap member,
// instead of the size, alignment, and offset.

#define ordered_hash_map_upsert_ex(hm, list, key, a, Type, member, flags, insert_count) \
	ordered_hash_map_upsert_general((hm), (list), (key), (a), (ptrdiff_t)offsetof(Type, member), (ptrdiff_t)sizeof(Type), (ptrdiff_t)alignof(Type), (flags), (insert_count))
// Extended version of ordered_hash_map_upsert, which accepts a flags argument and an optional insert_count argument.


NONSTD_API  OrderedHashMap *ordered_hash_map_list_reverse(OrderedHashMap *list);
// Since the ordered hash map list is in reverse insertion order, 
// this convenience function reverses the list in place and returns the new head of the list.





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
#ifdef NONSTD_IMPLEMENTATION
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <stdarg.h>

NONSTD_API int32_t 
msi_ht_lookup(uint64_t hash, int exp, int32_t idx)
{
	u32 mask = ((u32)1 << exp) - 1;
	u32 step = (hash >> (64 - exp)) | 1;
	return (idx + step) & mask;
}

NONSTD_API uint64_t 
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

NONSTD_API uint64_t 
hash_i64(int64_t x)
{
	char s[sizeof(x)];
	memcpy(s,&x,sizeof(x));
	return hash_cstr_FNV1a(s,sizeof(x));
}

NONSTD_API uint64_t 
hash_u64(uint64_t x)
{
	char s[sizeof(x)];
	memcpy(s,&x,sizeof(x));
	return hash_cstr_FNV1a(s,sizeof(x));
}

NONSTD_API uint32_t 
rand_pcg32 (uint64_t state[1])
{
	// Pseudorandom number generator - (simplified) Permuted Congruential Generator
	uint64_t m = 0x9b60933458e17d7d; // prime
	uint64_t a = 0xd737232eeccdf7ed; // prime
	state[0] = state[0] * m + a;
	int shift = 29 - (state[0] >> 61);
	return state[0] >> shift;
}


NONSTD_API float 
randn_pcg32 (uint64_t state[1])
{
	const float pi = 3.141592653589793238462643383f;
	const float u32max = (float)UINT32_MAX;
	// standard normal distributed random double generator
	float u1 = rand_pcg32(state);
	float u2 = rand_pcg32(state);
	return sqrtf(-2.0f*logf(u1/u32max)) * cosf(2.0f*pi*(u2/u32max));
}

NONSTD_API float 
randp_pcg32 (uint64_t state[1], float lambda)
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

NONSTD_API int
bubblesort_step (BubbleSort *state, int N)
{
	int *c = &state->priv[0];
	int *i = &state->priv[1];

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



///  error messages


#ifndef NONSTD_OVERRIDE_MESSAGE_FUNCTIONS
	NONSTD_API void error_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_API void warning_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_API void info_message (char * str)
	{
		fprintf(stdout, "%s\n", str);
		fflush(stdout);
	}
#endif



NONSTD_API _Noreturn void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
die (char *fmt, ...)
{
	char buf[1000] = {0};
	memcpy(buf,"DIE: ",5);
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf+5, sizeof(buf)-5, fmt, args);
	va_end(args);
	error_message(buf);
	exit(EXIT_FAILURE);
}

NONSTD_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
warn (char *fmt, ...)
{
	char buf[1000] = {0};
	memcpy(buf,"WARNING: ",9);
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf+9, sizeof(buf)-9, fmt, args);
	va_end(args);
	warning_message(buf);
}

NONSTD_API void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
logmsg (char *fmt, ...)
{
	char buf[1000]  = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	info_message(buf);
}





NONSTD_API void * 
xmalloc(i64 bytes) 
{
	void *p = malloc(bytes);
	if(!p) die((char*)"xmalloc failed to allocate %lli bytes", (long long) bytes);
	memset(p,0,bytes);
	return p;
}

NONSTD_API void * 
xrealloc(void *p, i64 bytes)
{
	p = realloc(p,bytes);
	if(!p) die((char*)"xrealloc failed to allocate %lli bytes", (long long) bytes);
	return p;
}


NONSTD_API Arena malloc_arena(ptrdiff_t cap)
{
	Arena a = {0};
	a.start = (char*)malloc(cap);
	a.one_past_end = a.start ? a.start+cap : 0;
	return a;
}

NONSTD_API void *allocate(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags)
{
	ptrdiff_t padding = -(uintptr_t)a->start & (align - 1);
	ptrdiff_t available = a->one_past_end - a->start - padding;

	if (available < 0 || count > available/size) {
		if (flags & ALLOC_SOFT_FAIL) return 0;
		else abort();  
	}

	void *p = a->start + padding;
	a->start += padding + count*size;

	if (flags & ALLOC_NO_ZERO) return p;
	else return memset(p, 0, count*size);
}

NONSTD_API char* allocate_sprintf(Arena *a, int *len, const char *fmt, ...)
{
        va_list args1, args2;
        va_start(args1, fmt);
        va_copy(args2, args1);
        int n = 1 + vsnprintf(0, 0, fmt, args1);
        char *mem = 0;
	ALLOCATE(a, mem, n);
        vsnprintf(mem, n, fmt, args2);
        va_end(args1);
        va_end(args2);
		if(len) *len = n-1;
        return mem;
}

NONSTD_API char *allocate_cstrdup(Arena *a, char *s)
{
	int len = strlen(s);
	char *p = (char*)allocate(a, len+1, 1, 1, 0);
	return p ? (char*)memcpy(p, s, len+1) : 0;
}

NONSTD_API Str allocate_strdup(Arena *a, Str s)
{
	char *p = (char*)allocate(a, s.len+1, 1, 1, 0);
	if(p) {
		memcpy(p, s.ptr, s.len);
		return mkstr(p, s.len);
	}
	INVALID_CODE_PATH();
}

// FNV-1a hashing of null-terminated c-strings
static uint64_t nonstd_hashmap_hash_cstr(char *s)
{
	int len = strlen(s);
	return hash_cstr_FNV1a(s, len);	
}

// Returns true if equal, false if not equal
static int nonstd_hashmap_equals_cstr(char *a, char *b)
{
	return !strcmp(a, b);
}

NONSTD_API void *hash_map_upsert_general(
	HashMap **hm,
	HASH_MAP_KEY_TYPE key,
	Arena *a,
	ptrdiff_t offset,
	ptrdiff_t size,
	ptrdiff_t align,
	int flags,
	int *insert_count)
{
	for (uint64_t h = HASH_MAP_KEY_HASH_FN(key); *hm; h <<= 2) {
		if (HASH_MAP_KEY_EQUALS_FN(key, (*hm)->key)) {
			return (void*) ((char*)(*hm) - offset);
		}
		hm = &(*hm)->child[h>>62];
	}

	if (!a) return 0;

	*hm = (HashMap*) ((char*)allocate(a, size, align, 1, 0) + offset);

	if(insert_count) insert_count[0]++;

	if(flags & HASH_MAP_DUP_KEY){
		(*hm)->key = HASH_MAP_KEY_COPY_FN(a, key);
	} else {
		(*hm)->key = key;
	}

	return (void*) ((char*)(*hm) - offset);
}


NONSTD_API void *ordered_hash_map_upsert_general(OrderedHashMap **hm,
	OrderedHashMap **list,
	HASH_MAP_KEY_TYPE key,
	Arena *a,
	ptrdiff_t offset,
	ptrdiff_t size,
	ptrdiff_t align,
	int flags,
	int *insert_count)
{
	for (uint64_t h = HASH_MAP_KEY_HASH_FN(key); *hm; h <<= 2) {
		if (HASH_MAP_KEY_EQUALS_FN(key, (*hm)->key)) {
			return (void*) ((char*)(*hm) - offset);
		}
		hm = &(*hm)->child[h>>62];
	}

	if (!a) return 0;

	*hm = (OrderedHashMap*) ((char*)allocate(a, size, align, 1, 0) + offset);
	if(list) {
		(*hm)->next = *list;
		*list = *hm;
	}

	if(insert_count) insert_count[0]++;

	if(flags & HASH_MAP_DUP_KEY){
		(*hm)->key = HASH_MAP_KEY_COPY_FN(a, key);
	} else {
		(*hm)->key = key;
	}

	return (void*) ((char*)(*hm) - offset);
}	

OrderedHashMap *ordered_hash_map_list_reverse(OrderedHashMap *list) 
{
	OrderedHashMap *prev = 0;
	while (list) {
		OrderedHashMap *save_next = list->next;
		list->next = prev;

		prev = list;
		list = save_next;
	}
	return prev;
}


NONSTD_API int
clean_ascii(char *dest, char *src, int len)
{
	int o = 0;
	for(int i = 0; i < len; i++) {
		char s = src[i];
		if (s=='\t' 
		|| s=='\n' 
		|| s=='\r' 
		|| s=='\f' 
		|| s=='\v' 
		|| (s > 31 && s < 127)) {
			dest[o++] = s;	
		}
	}
	return o;
}

NONSTD_API int
clean_whitespace_ascii(char *dest, char *src, int len) 
{
	int last_char_was_space = 0;
	int o = 0;
	for(int i = 0; i < len; i++) {
		switch (src[i]) {
		case ' ':  case '\t': case '\n':
		case '\r': case '\f': case '\v':
			if(!last_char_was_space) {
				if(dest) dest[o] = ' ';
				++o;
			}
			last_char_was_space = 1;
			break;
		default:
			if(src[i] >= 32 && src[i] < 127) {
				if(dest) dest[o] = src[i];
				++o;
			}
			last_char_was_space = 0;
			break;
		}
	}
	return o;
}

NONSTD_API int
strip_whitespace_ascii(char *dest, char *src, int len) 
{
	int first=0,end=0;
	for(int i = 0; i < len; i++) {
		switch (src[i]) {
		case ' ':  case '\t': case '\n':
		case '\r': case '\f': case '\v':
			continue;
		}
		first = i;
		break;
	}

	for(int i = len-1; i >= 0; i--) {
		switch (src[i]) {
		case ' ':  case '\t': case '\n':
		case '\r': case '\f': case '\v':
			continue;
		}
		end = i+1;
		break;
	}

	int o = 0;
	for(int i = first; i < end; i++) {
		dest[o++] = src[i];
	}

	return o;
}

NONSTD_API void
lowercase_ascii(char *dest, char *src, int len)
{
	for(int i = 0; i < len; i++) {
		if(src[i] > 64 && src[i] < 91) dest[i] = src[i]+32;
		else dest[i] = src[i];
	}
}

NONSTD_API void
uppercase_ascii(char *dest, char *src, int len)
{
	for(int i = 0; i < len; i++) {
		if(src[i] > 96 && src[i] < 123) dest[i] = src[i]-32;
		else dest[i] = src[i];
	}
}

NONSTD_API int 
is_character_in_set(char c, char *set, int len)
{
	char *set_end = set + len;
	while(set<set_end) {
		if(*set++==c) return 1;
	}
	return 0;
}

NONSTD_API int 
is_ascii_punctuation(char c)
{
	char *p = (char*)"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	return is_character_in_set(c,p,32);
}

NONSTD_API int 
is_ascii_whitespace(char c)
{
	char *s = (char*)" \t\r\n\f\v";
	return is_character_in_set(c,s,6);
}

NONSTD_API int
is_ascii_alphanumeric(char c)
{
	return
	(c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9');
}

NONSTD_API int
is_ascii_letter(char c)
{
	return
	(c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z');
}

NONSTD_API int
is_ascii_lower(char c)
{
	return (c >= 'a' && c <= 'z');
}

NONSTD_API int
is_ascii_upper(char c)
{
	return (c >= 'A' && c <= 'Z');
}

NONSTD_API int
is_ascii_digit(char c)
{
	return (c >= '0' && c <= '9');
}

NONSTD_API int
is_ascii_hexdigit(char c)
{
	return
	(c >= 'a' && c <= 'f') ||
	(c >= '0' && c <= '9');
}

NONSTD_API int
is_ascii_control(char c)
{
	return (c >= 0 && c <= 0x1f) || c == 0x7f;
}



NONSTD_API int 
cstr_endswith(const char *str, const char *ending) 
{
	int len1 = 0;
	for (; str[len1]; len1++);
	Str _str = mkstr((char*)str,len1);

	int len2 = 0;
	for (; ending[len2]; len2++);
	Str _ending = mkstr((char*)ending,len2);

	return str_endswith(_str, _ending);
}

NONSTD_API int 
cstr_startswith(const char *str, const char *start) 
{
	int len1 = 0;
	for (; str[len1]; len1++);
	Str _str = mkstr((char*)str,len1);

	int len2 = 0;
	for (; start[len2]; len2++);
	Str _start = mkstr((char*)start,len2);

	return str_startswith(_str, _start);
}




NONSTD_API int
parse_hexdigit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return -1;
}

NONSTD_API int
parse_hex_ull(char *str, int len, unsigned long long *result)
{
	// returns number of chars consumed, or -1 on overflow
	unsigned long long overflow_limit = ULLONG_MAX / 16;

	unsigned long long v = 0;
	int n = 0;
	int idigit = 0;
	while (str < str+len  &&  (idigit=parse_hexdigit(*str), idigit >= 0))
	{
		unsigned digit = idigit;
		if(v > overflow_limit) return -1;
		v *= 16;
		if(UINT64_MAX-digit < v) return -1;
		v += digit;
		str++;
		n++;
	}

	if (n > 0) *result = v;
	return n;
}

NONSTD_API int
parse_decimal_ull(char *str, int len, unsigned long long *result)
{
	// returns number of chars consumed, or -1 on overflow
	unsigned long long overflow_limit = ULLONG_MAX / 10;

	unsigned long long v = 0;
	int n = 0;
	while (str < str+len  &&  *str >= '0'  &&  *str <= '9')
	{
		unsigned digit = *str - '0';
		if(v > overflow_limit) return -1;
		v *= 10;
		if(UINT64_MAX-digit < v) return -1;
		v += digit;
		str++;
		n++;
	}

	if (n > 0) *result = v;
	return n;
}

enum {
	OP_RET                     = 0x00,
	OP_JUMP                    = 0x01,
	OP_MATCH_START_END         = 0x02,
	OP_MATCH                   = 0x03,
	OP_MATCH_OR_RET_F          = 0x04,
	OP_MATCH_AND_RET_T         = 0x05,
	OP_MATCH_AND_RET_F         = 0x06,
	OP_MATCH_AND_RPT           = 0x07,
	OP_CALL                    = 0x08,
	OP_RPT_IF_RET_T            = 0x09,
	OP_RET_F_IF_RET_F          = 0x0a,
	OP_MATCH_BUILTIN           = 0x0b,
	OP_MATCH_BUILTIN_OR_RET_F  = 0x0c,
	OP_MATCH_BUILTIN_AND_RET_T = 0x0d,
	OP_MATCH_BUILTIN_AND_RET_F = 0x0e,
	OP_MATCH_BUILTIN_AND_RPT   = 0x0f,

	OP_MASK   = 0x0f,
	ARG_SHIFT = 4,
};

typedef struct
{
	char *input;
	int input_len;
	int input_counter;

	CompiledStrPattern *program;
	int program_counter;

	char *error;

	int stack_pointer;
	int return_register;
	#define PATTERN_MACHINE_STACK_MAX 8
	int stack[PATTERN_MACHINE_STACK_MAX];
} PatternMachineState;


static int
pattern_machine_get_input(PatternMachineState *m)
{
	if(m->input_counter < m->input_len)
		return m->input[m->input_counter];
	return INT_MAX;
}

static void
pattern_machine_advance_input(PatternMachineState *m)
{
	m->input_counter++;
}

static int
pattern_machine_run(PatternMachineState *m)
{
       	while(1) {
		assert(m->program_counter < m->program->code_size);

		unsigned short instr  = m->program->code[m->program_counter];
		unsigned short opcode = instr & OP_MASK;
		unsigned short arg    = instr >> ARG_SHIFT;
		char c = arg;

		int result=0,input=0; // have to forward-declare these to make C++ happy - can't jump over a declaration with a case label

		switch(opcode) {
		case OP_RET: 
			if(arg) {
				ret_yes:
				m->return_register = 1; 
				if(m->stack_pointer==0) 
					return 1;
				else {
					m->stack_pointer--;
					m->program_counter = m->stack[--(m->stack_pointer)];
					assert(m->stack_pointer >= 0);
				}
			} else {
				ret_no:
				m->return_register = 0; 
				if(m->stack_pointer==0) 
					return 0;
				else {
					m->input_counter   = m->stack[--(m->stack_pointer)];
					m->program_counter = m->stack[--(m->stack_pointer)];
					assert(m->stack_pointer >= 0);
				}
			}
			break;
		case OP_JUMP:
			m->program_counter = arg-1;
			break;
		case OP_MATCH_START_END:
			if(c == '^') {
				if(m->input_counter != 0) goto ret_no;
			} else {
				if(m->input_counter != m->input_len) goto ret_no;
			}
			break;
		case OP_MATCH_OR_RET_F:
			assert(arg < 128);
			if(pattern_machine_get_input(m) == c) 
				pattern_machine_advance_input(m);
			else goto ret_no;
			break;
		case OP_MATCH_AND_RET_T:
			assert(arg < 128);
			if(pattern_machine_get_input(m) == c) {
				pattern_machine_advance_input(m);
				goto ret_yes;
			}
			break;
		case OP_MATCH_AND_RET_F:
			assert(arg < 128);
			if(pattern_machine_get_input(m) == c) {
				pattern_machine_advance_input(m);
				goto ret_no;
			}
			break;

		case OP_MATCH:
			assert(arg < 128);
			if(pattern_machine_get_input(m) == c)
				pattern_machine_advance_input(m);
			break;
		case OP_MATCH_AND_RPT:
			assert(arg < 128);
			if(pattern_machine_get_input(m) == c) {
				pattern_machine_advance_input(m);
				m->program_counter--;
			}
			break;
		case OP_CALL:
			// TODO guard against stack overflow
			m->stack[m->stack_pointer++] = m->program_counter;
			m->stack[m->stack_pointer++] = m->input_counter;
			m->program_counter = arg-1;
			break;
		case OP_RPT_IF_RET_T:
			if (m->return_register) m->program_counter -= 2;
			break;
		case OP_RET_F_IF_RET_F:
			if (!m->return_register) goto ret_no;
			break;
		case OP_MATCH_BUILTIN_OR_RET_F:
		case OP_MATCH_BUILTIN_AND_RET_T:
		case OP_MATCH_BUILTIN_AND_RET_F:
		case OP_MATCH_BUILTIN:
		case OP_MATCH_BUILTIN_AND_RPT:
			assert(arg < 128);
			result = 0;
			input = pattern_machine_get_input(m);

			if(input <= CHAR_MAX && input >= CHAR_MIN)
			switch(c)
			{
				case '.': result = 1; break;

				case 'A': result = 1;
				case 'a':
					if (
					(input >= 'A' && input <= 'Z') ||
					(input >= 'a' && input <= 'z') )
					       	result = !result;
					break;

				case 'C': result = 1;
				case 'c':
					if(input == 0x7f || (input >= 0 && input <= 0x1f))
						result = !result;
					break;

				case 'D': result = 1;
				case 'd':
					if(input >= '0' && input <='9')
						result = !result;
					break;

				case 'L': result = 1;
				case 'l':
					if(input >= 'a' && input <= 'z')
						result = !result;
					break;

				case 'P': result = 1;
				case 'p':
					if(is_ascii_punctuation(input))
						result = !result;
					break;

				case 'S': result = 1;
				case 's':
					if(is_ascii_whitespace(input))
						result = !result;	
					break;

				case 'U': result = 1;
				case 'u':
					if(input >= 'A' && input <= 'Z')
						result = !result;
					break;

				case 'W': result = 1;
				case 'w':
					if(
					(input >= '0' && input <= '9') ||
					(input >= 'A' && input <= 'Z') ||
					(input >= 'a' && input <= 'z'))
						result = !result;
					break;

				case 'X': result = 1; 
				case 'x': if(
					(input >= '0' && input <= '9') ||
					(input >= 'A' && input <= 'F') ||
					(input >= 'a' && input <= 'f'))
						result = !result;
					break;

				case 'Z': result = 1;
				case 'z':
					if(input==0) result = !result;
					break;

				default:
					assert(!"Invalid built-in match group");
					break;
			}

			if (result) pattern_machine_advance_input(m);

			if (opcode == OP_MATCH_BUILTIN_OR_RET_F) {
				if (!result) goto ret_no;
			} else if (opcode == OP_MATCH_BUILTIN_AND_RET_T) {
				if (result) goto ret_yes;
			} else if (opcode == OP_MATCH_BUILTIN_AND_RET_F) {
				if (result) goto ret_no;
			} else if (opcode == OP_MATCH_BUILTIN_AND_RPT) {
				if (result) m->program_counter--;
			}
			break;
		default:
			assert(!"Invalid opcode");
			return 0;
			break;
		}

		m->program_counter++;
	}
}

static unsigned short
make_instruction(unsigned short opcode, unsigned short arg)
{
	opcode = (opcode & OP_MASK) | (arg << ARG_SHIFT);
	return opcode;
}

static void 
program_add(unsigned short opcode, unsigned short arg, CompiledStrPattern *program)
{
	assert(program);
	if(program->error) return;

	opcode = make_instruction(opcode, arg);
	if(program->code_size < PATTERN_MACHINE_MAX_PROGRAM_SIZE)
		program->code[program->code_size++] = opcode;
	else program->error = 1;
}


NONSTD_API CompiledStrPattern
pattern_compile_ascii(char *pattern, int pattern_len)
{
	CompiledStrPattern program = {0};
	int in_class = 0;
	int invert_class = 0;
	int class_pos = 0;

	char *limit = pattern+pattern_len;
	char *p = pattern;
	for (; p < limit; p++) {
		int c = p[0];
		int next  = CHAR_MAX+1;
		int nnext = CHAR_MAX+1;
		if(p+1 < limit) next  = p[1];
		if(p+2 < limit) nnext = p[2];
	
		if(in_class) {
			/* INSIDE A CHARACTER CLASS */
			if(c == ']') {
				program_add(OP_RET, 0, &program);

				in_class = 0;
				program.code[class_pos] = make_instruction(OP_JUMP,program.code_size);

				if(next == '?') {
					program_add(OP_CALL, class_pos+1, &program);
					p++;
				} else if(next == '*') { 
					program_add(OP_CALL, class_pos+1, &program);
					program_add(OP_RPT_IF_RET_T, 0, &program);
					p++;
				} else if(next == '+') {
					program_add(OP_CALL, class_pos+1, &program);
					program_add(OP_RET_F_IF_RET_F, 0, &program);
					program_add(OP_CALL,  class_pos+1, &program);
					program_add(OP_RPT_IF_RET_T, 0, &program);
					p++;
				} else {
					program_add(OP_CALL, class_pos+1, &program);
					program_add(OP_RET_F_IF_RET_F, 0, &program);
				}

			} else if(c == '%') {

				if(next > CHAR_MAX) goto error;
				#define TOKENS_MAPPED_TO_CHAR2  "%.+*?^$[]"
				#define TOKENS_MAPPED_TO_GROUP "acdlpsuwxzACDLPSUWXZ"
				if(is_character_in_set(next, (char*)TOKENS_MAPPED_TO_CHAR2, sizeof(TOKENS_MAPPED_TO_CHAR2)-1)) {

					if(!invert_class) program_add(OP_MATCH_AND_RET_T, next, &program);
					else              program_add(OP_MATCH_AND_RET_F, next, &program);

				} else if(is_character_in_set(next, (char*)TOKENS_MAPPED_TO_GROUP, sizeof(TOKENS_MAPPED_TO_GROUP)-1)) {

					if(!invert_class) program_add(OP_MATCH_BUILTIN_AND_RET_T, next, &program);
					else              program_add(OP_MATCH_BUILTIN_AND_RET_F, next, &program);

				} else goto error;
				p++;
			} else {
				if(!invert_class) program_add(OP_MATCH_AND_RET_T, c, &program);
				else              program_add(OP_MATCH_AND_RET_F, c, &program);
			}

		} else {
			/* NOT INSIDE A CHARACTER CLASS */

			if(c == '*') goto error;
			if(c == '+') goto error;
			if(c == '?') goto error;

			if(c == '%') {
				if(next > CHAR_MAX) goto error;
				#define TOKENS_MAPPED_TO_CHAR  "%.+*?^$["
				if(is_character_in_set(next, (char*)TOKENS_MAPPED_TO_CHAR, sizeof(TOKENS_MAPPED_TO_CHAR)-1)) {
					if(nnext == '+') {
						program_add(OP_MATCH_OR_RET_F, next, &program);
						program_add(OP_MATCH_AND_RPT, next, &program);
						p+=2;
					} else if(nnext == '*') {
						program_add(OP_MATCH_AND_RPT, next, &program);
						p+=2;
					} else if(nnext == '?') {
						program_add(OP_MATCH, next, &program);
						p+=2;
					} else {
						program_add(OP_MATCH_OR_RET_F, next, &program);
						p++;
					}
				} else if(is_character_in_set(next, (char*)TOKENS_MAPPED_TO_GROUP, sizeof(TOKENS_MAPPED_TO_GROUP)-1)) {
					if(nnext == '+') {
						program_add(OP_MATCH_BUILTIN_OR_RET_F, next, &program);
						program_add(OP_MATCH_BUILTIN_AND_RPT, next, &program);
						p+=2;
					} else if(nnext == '*') {
						program_add(OP_MATCH_BUILTIN_AND_RPT, next, &program);
						p+=2;
					} else if(nnext == '?') {
						program_add(OP_MATCH_BUILTIN, next, &program);
						p+=2;
					} else {
						program_add(OP_MATCH_BUILTIN_OR_RET_F, next, &program);
						p++;
					}
				} else goto error;
			} 
			else if (c == '^') program_add(OP_MATCH_START_END, c, &program);
			else if (c == '$') program_add(OP_MATCH_START_END, c, &program);
			else if (c == '[') {
				in_class = 1;
				class_pos = program.code_size;
				program_add(OP_RET, 0, &program); // this will later be replaced
				if (next == '^') {
					invert_class = 1;
					p++;
				} else {
					invert_class = 0;
				}
			} else {
				if(next == '+') {
					program_add(OP_MATCH_OR_RET_F, c, &program);
					program_add(OP_MATCH_AND_RPT, c, &program);
					p++;
				} else if(next == '*') {
					program_add(OP_MATCH_AND_RPT, c, &program);
					p++;
				} else if(next == '?') {
					program_add(OP_MATCH, c, &program);
					p++;
				} else	program_add(OP_MATCH_OR_RET_F, c, &program);
			}
		}
	}

	if(in_class) goto error;
	program_add(OP_RET, 1, &program);
	return program;

	error: program.error = -1 - (p-pattern);
	return program;
} 




NONSTD_API int
pattern_match_ascii(char *string, int string_len, CompiledStrPattern *program, int *match_len)
{
	if(program->error) return -2;

	int program_starts_with_anchor = 
		((program->code[0] & OP_MASK) == OP_MATCH_START_END) &&
		((program->code[0] >> ARG_SHIFT) == '^');

	for(int i = 0; i < string_len; i++) {
		PatternMachineState m = {
			.input = string,
			.input_len = string_len,
			.input_counter = i,
			.program = program,
		};

		int yes = pattern_machine_run(&m);
		if(yes) {
			*match_len = m.input_counter-i;
			return i;
		}

		if (program_starts_with_anchor) break;
	}
	return -1;
}

NONSTD_API Str 
str_strip(Str s)
{
	for(int i = 0; i < s.len; i++) {
		switch (s.ptr[i]) {
		case ' ':  case '\t': case '\n':
		case '\r': case '\f': case '\v':
			s.ptr++;
			s.len--;
			continue;
		}
		break;
	}

	for(int i = s.len-1; i >= 0; i--) {
		switch (s.ptr[i]) {
		case ' ':  case '\t': case '\n':
		case '\r': case '\f': case '\v':
			s.len--;
			continue;
		}
		break;
	}
	return s;
}

NONSTD_API Str
str_split(Str* s, char delim)
{
	Str rtn = { .ptr = s->ptr };
	for(int i = 0; i < s->len; i++){
		if(s->ptr[i]==delim) {
			rtn.len = i;
			s->ptr += (i+1);
			s->len -= (i+1);
			goto out;
		}
	}
	rtn.len = s->len;
	s->ptr += s->len+1;
	s->len = 0;
	out: return rtn;
}

NONSTD_API Str
str_split_str(Str* s, Str delim)
{
	int i = str_search(*s, delim);
	Str rtn = { .ptr = s->ptr };
	if (i > -1) {
		rtn.len = i;
		s->ptr += (i+delim.len);
		s->len -= (i+delim.len);
		return rtn;
	} else {
		rtn.len = s->len;
		s->ptr += s->len+delim.len;
		s->len = 0;
		return rtn;
	}
}

NONSTD_API int 
str_equals(Str a, Str b)
{
	if(a.len==b.len) {
		for(int i = 0; i < a.len; i++)
			if(a.ptr[i]!=b.ptr[i]) goto nope;
		return 1;
	}
	nope: return 0;
}

NONSTD_API int
str_search(Str haystack, Str needle)
{
	for (int offset = 0; offset <= haystack.len-needle.len; offset++) {
		for (int i = 0; i < needle.len; i++) {
			if (haystack.ptr[i+offset]!=needle.ptr[i]) goto nope;
		}
		return offset;
		nope: ;
	}
	return -1;
}

NONSTD_API int 
str_pattern_match(Str *match, Str *string, CompiledStrPattern *program)
{
	int match_len = 0;
	int loc = pattern_match_ascii(string->ptr, string->len, program, &match_len);
	if (loc >= 0) {
		*match  = mkstr(string->ptr+loc, match_len);
		*string = mkstr(match->ptr+match_len, string->len-match_len-loc);
		return 1;
	}
	return 0;
}

NONSTD_API int
str_startswith(Str s, Str startswith)
{
	if (s.len >= startswith.len) {
		for(int i = 0; i < startswith.len; i++)
			if(s.ptr[i]!=startswith.ptr[i]) goto nope;
		return 1;
	}
	nope: return 0;
}

NONSTD_API int
str_endswith(Str s, Str endswith)
{
	if (s.len >= endswith.len) {
		int offset = s.len-endswith.len;
		for(int i = 0; i < endswith.len; i++)
			if(s.ptr[i+offset]!=endswith.ptr[i]) goto nope;
		return 1;
	}
	nope: return 0;
}

#ifdef NONSTD_DEBUG
NONSTD_API int
debug_dump_program(char *buffer, int buffer_len, CompiledStrPattern *p)
{
	const char *mnemonics[] = {
		[OP_RET                    ] = "ret",
		[OP_JUMP                   ] = "jmp",
		[OP_MATCH_START_END        ] = "mse",
		[OP_MATCH                  ] = "m",
		[OP_MATCH_OR_RET_F         ] = "mof",
		[OP_MATCH_AND_RET_T        ] = "mat",
		[OP_MATCH_AND_RET_F        ] = "maf",
		[OP_MATCH_AND_RPT          ] = "marpt",
		[OP_CALL                   ] = "call",
		[OP_RPT_IF_RET_T           ] = "crpt",
		[OP_RET_F_IF_RET_F         ] = "crtnf",
		[OP_MATCH_BUILTIN          ] = "mb",
		[OP_MATCH_BUILTIN_OR_RET_F ] = "mbof",
		[OP_MATCH_BUILTIN_AND_RET_T] = "mbat",
		[OP_MATCH_BUILTIN_AND_RET_F] = "mbaf",
		[OP_MATCH_BUILTIN_AND_RPT  ] = "mbrpt",
	};

	int N = 0;
	for (int i = 0; i < p->code_size; i++)
	{
		unsigned short instr = p->code[i];
		unsigned short op  = instr & OP_MASK;
		unsigned short arg = instr >> ARG_SHIFT;
		
		int bl = buffer_len - N;
		if(N + 26 >= buffer_len-1) 
			bl = 0;

		N += snprintf(buffer+N, bl, "%0.4x: %-6s ", i, mnemonics[op]);
		switch(op) {
			case OP_RET:
			case OP_JUMP:
			case OP_CALL:
			case OP_RPT_IF_RET_T:
			case OP_RET_F_IF_RET_F:
				N += snprintf(buffer+N, bl-7, "%-#12hx\n",arg);
				break;
			default:
				N += snprintf(buffer+N, bl-7, "%-12c\n",(char)arg);
				break;
		}
	}	
	return N;
}
#endif

#endif
