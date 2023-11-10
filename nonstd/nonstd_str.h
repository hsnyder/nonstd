/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	nonstd_str.h: tools for manipulating strings in C, with slightly less agony.

	No unicode support yet.

	This file is a single-header library (credit to Sean Barrett for the
	idea). It includes both the header and the actual definitions in
	a single file. To use this library, copy it  into your project, and
	define NONSTD_STR_IMPLEMENTATION in exactly one .c file, immediately before 
	you include str.h
*/

#ifndef NONSTD_STR_H
#define NONSTD_STR_H

#ifndef NONSTD_STR_API
#define NONSTD_STR_API 
#endif


///////////   GENERAL ASCII TOOLS

NONSTD_STR_API int clean_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where bytes that 
// aren't ASCII-printable or ASCII-whitespace characters are removed.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_STR_API int clean_whitespace_ascii(char *dest, char *src, int src_len);
// Writes into `dest` a modified copy of `src`: 
//   - Bytes that aren't ASCII-printable chars are removed.
//   - All whitespace characters are replaced with ' '.
//   - Consecutive whitespace characters are merged into a single ' '.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_STR_API int strip_whitespace_ascii(char *dest, char *src, int src_len);
// Writes into `dest` a modified copy of `src`: 
//   - Leading and trailing ASCII whitespace chars are removed.
// `dest` must be at a buffer at least as long as `src`, or must be null.
// Returns the number of chars that were (or would be) written to `dest`.
// In-place use is safe.

NONSTD_STR_API void lowercase_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where uppercase letters
// are converted to lowercase. `dest` must be a buffer at least as long
// as `src`. In-place use is safe.

NONSTD_STR_API void uppercase_ascii(char *dest, char *src, int len);
// Writes into `dest` a modified copy of `src`, where lowercase letters
// are converted to uppercase. `dest` must be a buffer at least as long
// as `src`. In-place use is safe.

NONSTD_STR_API int is_character_in_set(char c, char *set, int len);
// Returns 1/0 if `c` is/isn't found in the string `set`.

NONSTD_STR_API int is_ascii_punctuation(char c);
// Returns 1/0 if `c` is/isn't ASCII punctuation.
// i.e.:  !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~

NONSTD_STR_API int is_ascii_whitespace(char c);
// Returns 1/0 if `c` is/isn't ASCII whitespace.
// i.e. 0x20 or: \f\n\r\t\v 

NONSTD_STR_API int is_ascii_alphanumeric(char c);
// Returns 1/0 if `c` is/isn't ASCII alphanumeric.
// i.e. a-zA-Z0-9

NONSTD_STR_API int is_ascii_letter(char c);
// Returns 1/0 if `c` is/isn't an ASCII letter.
// i.e. a-zA-Z

NONSTD_STR_API int is_ascii_lower(char c);
// Returns 1/0 if `c` is/isn't an ASCII lowercase letter.
// i.e. a-z

NONSTD_STR_API int is_ascii_upper(char c);
// Returns 1/0 if `c` is/isn't an ASCII uppercase letter.
// i.e. a-z

NONSTD_STR_API int is_ascii_digit(char c);
// Returns 1/0 if `c` is/isn't an ASCII digit;
// i.e. 0-9

NONSTD_STR_API int is_ascii_hexdigit(char c);
// Returns 1/0 if `c` is/isn't an ASCII hex digit;
// i.e. 0-9a-f

NONSTD_STR_API int is_ascii_control(char c);
// Returns 1/0 if `c` is/isn't an ASCII control character;
// i.e. 0x00-0x1f and 0x7f

NONSTD_STR_API int cstr_endswith(const char *str, const char *ending); 
// checks if a null terminated string ends with the specified (null terminated string) ending



///////////   NUMERIC CONVERSION

NONSTD_STR_API int parse_hexdigit(char c);
// Parses the given character as a hexidecimal digit and returns the value,
// or returns -1 if `c` is not a valid hex digit (0-9a-fA-F)

NONSTD_STR_API int parse_hex_ull(char *str, int len, unsigned long long *result);
// Parses a hexidecimal ullong, which must not be '0x' prefixed, must not 
// be '+'/'-' prefixed, and must begin exactly at `str`. Returns the number
// of characters that were used for the conversion (conversion stops if it
// encounters a non-ASCII-hex character), or -1 on overflow. NB a return 
// of zero means couldn't parse anything.
	
NONSTD_STR_API int parse_decimal_ull(char *str, int len, unsigned long long *result);
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

NONSTD_STR_API CompiledStrPattern pattern_compile_ascii(char *pattern, int pattern_len);
// Given a text pattern, produces a bytecode program that can be 
// executed with pattern_match_ascii() to do text matching.
// Sets the error property of the returned struct if something
// goes wrong.

NONSTD_STR_API int pattern_match_ascii(char *string, int string_len, CompiledStrPattern *program, int *match_len);
// Searches `string` for the first occurrence of `pattern`.
// Also sets `match_len` to the length of the match, if applicable.
// Returns -2 if the program contained an error, -1 if the 
// pattern does not match, or the index of the match otherwise.


#ifdef NONSTD_STR_DEBUG
NONSTD_STR_API int debug_dump_program(char *buffer, int buffer_len, CompiledStrPattern *p);
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

NONSTD_STR_API Str str_strip(Str s);
// Returns a copy of s where leading and trailing ASCII whitespace have been removed.

NONSTD_STR_API Str str_split(Str* s, char delim);
// Pops the first substring (delimited by `delim`) off of `s` (modifying it).
// `s` will have zero-length if there's nothing left to pop.

NONSTD_STR_API Str str_split_str(Str* s, Str delim);
// Pops the first substring (delimited by `delim`) off of `s` (modifying it).
// `s` will have zero-length if there's nothing left to pop.

NONSTD_STR_API int str_equal(Str a, Str b);
// Returns 1 if `a` and `b` are equal, 0 otherwise

NONSTD_STR_API int str_startswith(Str s, Str startswith);
// Returns 1 if `s` begins with `startswith`, 0 otherwise

NONSTD_STR_API int str_endswith(Str s, Str endswith);
// Returns 1 if `s` ends with `endswith`, 0 otherwise

NONSTD_STR_API int str_search(Str haystack, Str needle);
// Searches `haystack` for `needle`, returning the index at which is is found
// or -1 if it is not found at all.

NONSTD_STR_API int str_pattern_match(Str *match, Str *string, CompiledStrPattern *program);
// Calls pattern_match_ascii() to match the specified pattern against `string`.
// Updates `string` to point to the text after the match, and sets `match` to
// the actual match. Returns 1 if a match was found, 0 if it was not (or if 
// the program contains an error).



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
#ifdef NONSTD_STR_IMPLEMENTATION

#ifndef assert
#  if defined(NONSTD_STR_DISABLE_ASSERTIONS) || defined(NDEBUG)
#    define assert(c)
#  else
#    if defined(_MSC_VER)
#      define assert(c) if(!(c)){__debugbreak();}
#    else
#      if defined(__GNUC__) || defined(__clang__)
#        define assert(c) if(!(c)){__builtin_trap();}
#      else 
#        define assert(c) if(!(c)){*(volatile int*)0=0;}
#      endif 
#    endif
#  endif
#endif

#include <limits.h>
#include <stdint.h>

NONSTD_STR_API int
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

NONSTD_STR_API int
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

NONSTD_STR_API int
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

NONSTD_STR_API void
lowercase_ascii(char *dest, char *src, int len)
{
	for(int i = 0; i < len; i++) {
		if(src[i] > 64 && src[i] < 91) dest[i] = src[i]+32;
		else dest[i] = src[i];
	}
}

NONSTD_STR_API void
uppercase_ascii(char *dest, char *src, int len)
{
	for(int i = 0; i < len; i++) {
		if(src[i] > 96 && src[i] < 123) dest[i] = src[i]-32;
		else dest[i] = src[i];
	}
}

NONSTD_STR_API int 
is_character_in_set(char c, char *set, int len)
{
	char *set_end = set + len;
	while(set<set_end) {
		if(*set++==c) return 1;
	}
	return 0;
}

NONSTD_STR_API int 
is_ascii_punctuation(char c)
{
	char *p = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	return is_character_in_set(c,p,32);
}

NONSTD_STR_API int 
is_ascii_whitespace(char c)
{
	char *s = " \t\r\n\f\v";
	return is_character_in_set(c,s,6);
}

NONSTD_STR_API int
is_ascii_alphanumeric(char c)
{
	return
	(c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z') ||
	(c >= '0' && c <= '9');
}

NONSTD_STR_API int
is_ascii_letter(char c)
{
	return
	(c >= 'a' && c <= 'z') ||
	(c >= 'A' && c <= 'Z');
}

NONSTD_STR_API int
is_ascii_lower(char c)
{
	return (c >= 'a' && c <= 'z');
}

NONSTD_STR_API int
is_ascii_upper(char c)
{
	return (c >= 'A' && c <= 'Z');
}

NONSTD_STR_API int
is_ascii_digit(char c)
{
	return (c >= '0' && c <= '9');
}

NONSTD_STR_API int
is_ascii_hexdigit(char c)
{
	return
	(c >= 'a' && c <= 'f') ||
	(c >= '0' && c <= '9');
}

NONSTD_STR_API int
is_ascii_control(char c)
{
	return (c >= 0 && c <= 0x1f) || c == 0x7f;
}



NONSTD_STR_API int 
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



NONSTD_STR_API int
parse_hexdigit(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return -1;
}

NONSTD_STR_API int
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

NONSTD_STR_API int
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
			int result = 0;
			int input = pattern_machine_get_input(m);

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


NONSTD_STR_API CompiledStrPattern
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
				if(is_character_in_set(next, TOKENS_MAPPED_TO_CHAR2, sizeof(TOKENS_MAPPED_TO_CHAR2)-1)) {

					if(!invert_class) program_add(OP_MATCH_AND_RET_T, next, &program);
					else              program_add(OP_MATCH_AND_RET_F, next, &program);

				} else if(is_character_in_set(next, TOKENS_MAPPED_TO_GROUP, sizeof(TOKENS_MAPPED_TO_GROUP)-1)) {

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
				if(is_character_in_set(next, TOKENS_MAPPED_TO_CHAR, sizeof(TOKENS_MAPPED_TO_CHAR)-1)) {
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
				} else if(is_character_in_set(next, TOKENS_MAPPED_TO_GROUP, sizeof(TOKENS_MAPPED_TO_GROUP)-1)) {
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




NONSTD_STR_API int
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

NONSTD_STR_API Str 
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

NONSTD_STR_API Str
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

NONSTD_STR_API Str
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

NONSTD_STR_API int 
str_equal(Str a, Str b)
{
	if(a.len==b.len) {
		for(int i = 0; i < a.len; i++)
			if(a.ptr[i]!=b.ptr[i]) goto nope;
		return 1;
	}
	nope: return 0;
}

NONSTD_STR_API int
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

NONSTD_STR_API int 
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

NONSTD_STR_API int
str_startswith(Str s, Str startswith)
{
	if (s.len >= startswith.len) {
		for(int i = 0; i < startswith.len; i++)
			if(s.ptr[i]!=startswith.ptr[i]) goto nope;
		return 1;
	}
	nope: return 0;
}

NONSTD_STR_API int
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

#ifdef NONSTD_STR_DEBUG
#include <stdio.h>
NONSTD_STR_API int
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

