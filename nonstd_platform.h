/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	nonstd_base.h is part of 'nonstd': an attempt to supplement the C 
	standard library. See the comments in `nonstd.h` for an overview.
*/
#ifndef NONSTD_PLATFORM_H
#define NONSTD_PLATFORM_H

#ifndef NONSTD_PLATFORM_API
#define NONSTD_PLATFORM_API 
#endif

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

/* 
   ============================================================================
		TIMING AND PROFILING 
   ============================================================================
*/

/*
	Very low-overhead high resolution timer.
	The units aren't guaranteed to be any particular thing
	(use cpu_time_to_sec() to convert a difference of times to seconds).
*/
static uint64_t
read_cpu_timer(void) 
{
#if   defined(__x86_64__)
	return __builtin_ia32_rdtsc(); 
#elif defined (__aarch64__)
	return __builtin_readcyclecounter();
#else
	return 0;
#endif 
}


/* 
	Converts a difference of values from read_cpu_timer() to (approx) seconds. 
	Will block for 100ms the first time it's called!!!
*/
NONSTD_PLATFORM_API double cpu_time_to_sec(uint64_t cpu_time_elapsed) ;

/* 
	Return wall-clock time in seconds. 
	What point is defined as "zero" time is undefined,
	so differences are meaningful but not an individual time. 
	Uses cpu_time_to_sec, so be aware of the one time 100ms block.
*/
NONSTD_PLATFORM_API double get_wtime(void); 


/*
	Returns the frequency of the OS timer in counts per second
*/
NONSTD_PLATFORM_API uint64_t get_os_timer_freq(void);

/*
	Query the current OS time. Zero reference time is not guaranteed to be any particular thing
*/
NONSTD_PLATFORM_API uint64_t read_os_timer(void);



/* 
   ============================================================================
		CONCURRENCY SUPPORT
   ============================================================================
*/

/*
	Spin-locking ticket-taking mutex.
*/
typedef struct {
	uint32_t ticket;
	uint32_t serving;
} TicketMutex;

NONSTD_PLATFORM_API void ticket_mutex_lock(TicketMutex *m);    
NONSTD_PLATFORM_API void ticket_mutex_unlock(TicketMutex *m);


/*
	"once barrier".. useful if you need some initialization code to be called exactly once.
	usage is:

	static int b = 0; // init to zero is important

	if (once_enter(&b)) {
		do_initialization_work();
		once_commit(&b);
	} 
*/

NONSTD_PLATFORM_API int once_enter(int *b); 
// Returns true if you are the thread that needs to do the init work.

NONSTD_PLATFORM_API void once_commit(int *b); 
// Call this once you're done doing init work.

/*
	Lock free concurrent queue.
	Credit to Chris Wellons for the idea: 
        https://nullprogram.com/blog/2022/05/14/ (public domain)
	Operation fully explained in the above link.
*/
NONSTD_PLATFORM_API int  queue_push(uint32_t *q, int exp);
NONSTD_PLATFORM_API void queue_push_commit(uint32_t *q);

NONSTD_PLATFORM_API int  queue_pop(uint32_t *q, int exp);
NONSTD_PLATFORM_API void queue_pop_commit(uint32_t *q);

NONSTD_PLATFORM_API int  queue_mpop(uint32_t *q, int exp, uint32_t *save);
NONSTD_PLATFORM_API int  queue_mpop_commit(uint32_t *q, uint32_t save);


/*
	Manual-reset event.
	- No system call on post if no threads waiting.
	- No system call on wait if event already posted.
	- Reset does not wake sleepers, it's just a relaxed atomic store
	  (so don't rely on reset for any type of syncrhonization).
*/
NONSTD_PLATFORM_API void event_wait(uint32_t *event);
NONSTD_PLATFORM_API void event_post(uint32_t *event);
NONSTD_PLATFORM_API void event_reset(uint32_t *event);

/*
	Unfair blocking semaphore.

	Uses futexes on supported operating systems to put threads to sleep until 
	the resource is available (uses spin-locking if futexes aren't availble).
	The implementation isn't optimal: semaphore_post makes a system call even
	if there are no waiters, but since only one waiter is woken each post, 
	there's no thundering herd effect.

	Note: the maximum supported value for `sem` is INT32_MAX, not UINT32_MAX.
*/
NONSTD_PLATFORM_API void semaphore_wait(uint32_t *sem);
NONSTD_PLATFORM_API void semaphore_post(uint32_t *sem);

/*
	Blocking concurrent queue (multi-producer, multi-consumer)

	The memory for the acutal queue entries is externally managed, like the 
	above queue. The number of slots must be a power of 2.

	This can't be zero-initialized, but it can be STATICALLY initialized.
	The requirements are a bit complicated, so it's best to just use the 
	convenience macro BLOCKING_CONCURRENT_QUEUE_INITIALIZER, which you just
	give it the exponent - the queue has 2^n slots where n is the exponent
	that you provide.

	But if you want to know, the initialization requirements are:
	- set exp to the exponent (2^n) indicating how many slots exist.
	- set procucer slots to 2^n-1
	- set access_semaphore to 1
*/

typedef struct {
	int exp;
	uint32_t producer_slots;
	uint32_t consumer_slots;
	uint32_t access_semaphore;
	uint32_t q;
} BlockingConcurrentQueue;

#define BLOCKING_CONCURRENT_QUEUE_INITIALIZER(exponent) \
	(BlockingConcurrentQueue){.exp=exponent, .producer_slots=((1<<exponent)-1), .access_semaphore=1}

NONSTD_PLATFORM_API int  blocking_queue_push(BlockingConcurrentQueue *q);
NONSTD_PLATFORM_API void blocking_queue_push_commit(BlockingConcurrentQueue *q);

NONSTD_PLATFORM_API int  blocking_queue_pop(BlockingConcurrentQueue *q);
NONSTD_PLATFORM_API void blocking_queue_pop_commit(BlockingConcurrentQueue *q);

/* 
   ============================================================================
		MEMORY MANAGEMENT
   ============================================================================
*/
NONSTD_PLATFORM_API  void * xmalloc(i64 bytes);
// calls malloc(), calls die() if malloc() fails

NONSTD_PLATFORM_API  void * xrealloc(void *p, i64 bytes);
// calls realloc(), calls die() if realloc() fails

NONSTD_PLATFORM_API  i64 get_total_mem_bytes (void);  
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

NONSTD_PLATFORM_API  void  arena_clear(Arena *a, int reclaim); // deletes everything in the arena but keeps the arena around
NONSTD_PLATFORM_API  void  arena_destroy(Arena *a); // deletes everything in the arena and destroys the arena

NONSTD_PLATFORM_API  int arena_dump_file(Arena *a, char * filename); // dump contents of arena to a file.
NONSTD_PLATFORM_API  i64 arena_dump(i64 bufsz, void *buf, Arena *a); // dump contents of arena to a supplied buffer, returns the required size.
NONSTD_PLATFORM_API  Arena  arena_load_file(char * filename, i64 sz_reserve_extra); // load contents of an arena from a file.

NONSTD_PLATFORM_API  void* allocate(Arena *a, i64 sz); // allocate and zero some memory
NONSTD_PLATFORM_API  void* allocate_empty(Arena *a, i64 sz); // allocate some uninitialized memory
NONSTD_PLATFORM_API  void* allocate_named(Arena *a, i64 sz, char *name, int name_len); // allocate and assign a name
NONSTD_PLATFORM_API  void* allocate_empty_named(Arena *a, i64 sz, char *name, int name_len); // allocate and zero, and assign a name
									//
NONSTD_PLATFORM_API  void* allocation_copy(Arena *a, void *src_data); // copies *src_data from another Arena to a

NONSTD_PLATFORM_API  void* allocation_lookup(Arena *a, char *name, int name_len); // finds an allocation by name

NONSTD_PLATFORM_API  int allocation_check_name(void *p, char *name, int name_len); // check that a previous allocation has the specified name

NONSTD_PLATFORM_API  i64 allocation_size(void *p); // gets size of an allocation that was allocated by a Arena
NONSTD_PLATFORM_API  i64 allocation_capacity(void *p); // gets capacity of an allocation that was allocated by a Arena (may be > size due to alignment padding)

NONSTD_PLATFORM_API  void arena_mem_lock(Arena *a); // locks memory, preventing it from being swapped
NONSTD_PLATFORM_API  void arena_mem_unlock(Arena *a); // unlocks memory, allowing it to be swapped
				 
NONSTD_PLATFORM_API  i64 arena_get_used_memory(Arena *a); // gets the number of bytes in use by the arena
                                     // exists only b/c python can't easily access the .used struct member
NONSTD_PLATFORM_API  i64  arena_checkpoint(Arena *a);
NONSTD_PLATFORM_API  void arena_rollback(Arena *a, i64 checkpoint);

NONSTD_PLATFORM_API  char* allocate_sprintf(Arena *a, char *fmt, ...);
NONSTD_PLATFORM_API  char* allocate_cstrdup(Arena *a, char *cstr);

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

NONSTD_PLATFORM_API  AllocationHeader * arena_foreach(Arena *a, i64 *state);

NONSTD_PLATFORM_API  void print_allocation_header(AllocationHeader* x) ;

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
		I/O
   ============================================================================
*/
typedef struct {
	i64 len;
	void *mem;
} FileContents;

NONSTD_PLATFORM_API  FileContents platform_read_file(char *filename);

NONSTD_PLATFORM_API  int platform_read_file_into_buffer(i64 buffer_size, void *buffer, i64 *file_size, char *filename);
NONSTD_PLATFORM_API  int platform_read_file_into_arena(Arena *a, void **file_bytes, i64 *file_size, char *filename);
NONSTD_PLATFORM_API  int platform_write_file(char * filename, void *what, size_t bytes);

NONSTD_PLATFORM_API  i64 platform_get_file_size(char *filename);

// Writes out the message from errno or GetLastError with a user-provided message prefix
NONSTD_PLATFORM_API  void errmsg_from_platform(char * prefix);


/* 
   ============================================================================
		PLATFORM-SPECIFIC LOW LEVEL MEMORY MANAGEMENT
   ============================================================================
*/

// returns 0 on failure
NONSTD_PLATFORM_API  void* platform_reserve_mem(size_t size);

// returns 0 on failure, true on success.
// NOTE: start is rounded DOWN to the page size, and len is rounded UP to the end of the page. 
NONSTD_PLATFORM_API  int platform_unreserve_mem(void *start, size_t len);
NONSTD_PLATFORM_API  int platform_decommit_mem (void* start, size_t len);
NONSTD_PLATFORM_API  int platform_commit_mem   (void* start, size_t len); 
NONSTD_PLATFORM_API  int platform_lock_mem     (void *start, size_t len);
NONSTD_PLATFORM_API  int platform_unlock_mem   (void *start, size_t len);


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
#ifdef NONSTD_PLATFORM_IMPLEMENTATION
#include <string.h>
#include <limits.h>
#include <math.h>


// Most architectures have special instructions which hint to the CPU that we're in a spin-lock loop.
#if   defined(__x86_64__)
#define SPIN_LOOP_HINT()  __asm __volatile ("pause"); 
#elif defined(__arm__)
#define SPIN_LOOP_HINT()  __asm __volatile ("yield"); 
#else
#define SPIN_LOOP_HINT() 
#endif

NONSTD_PLATFORM_API void 
ticket_mutex_lock(TicketMutex *m)
{
	uint32_t my_ticket = __atomic_fetch_add(&m->ticket, 1, __ATOMIC_RELAXED);
	while (my_ticket != __atomic_load_n(&m->serving, __ATOMIC_ACQUIRE)) {
		SPIN_LOOP_HINT();
	}
}

NONSTD_PLATFORM_API void 
ticket_mutex_unlock(TicketMutex *m) 
{
        (void) __atomic_fetch_add(&m->serving, 1, __ATOMIC_RELEASE);
}

NONSTD_PLATFORM_API int 
once_enter(int *b)
{
	if(2 == __atomic_load_n(b, __ATOMIC_SEQ_CST)) return 0;

	int zero = 0;
	int got_lock = __atomic_compare_exchange_n(b, &zero, 1, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	if (got_lock) return 1;

	while (2 != __atomic_load_n(b, __ATOMIC_SEQ_CST)) {
		SPIN_LOOP_HINT();
	};
	return 0;
}

NONSTD_PLATFORM_API void 
once_commit(int *b)
{
	(void) __atomic_store_n(b, 2, __ATOMIC_SEQ_CST);
}


NONSTD_PLATFORM_API int
queue_push(uint32_t *q, int exp)
{
	uint32_t r = __atomic_load_n(q, __ATOMIC_ACQUIRE);
	int mask = (1u << exp) - 1;
	int head = r     & mask;
	int tail = r>>16 & mask;
	int next = (head + 1u) & mask;
	if (r & 0x8000) {  // avoid overflow on commit
		__atomic_and_fetch(q, ~0x8000, __ATOMIC_RELEASE);
	}
	return next == tail ? -1 : head;
}

NONSTD_PLATFORM_API void
queue_push_commit(uint32_t *q)
{
	__atomic_add_fetch(q, 1, __ATOMIC_RELEASE);
}

NONSTD_PLATFORM_API int
queue_pop(uint32_t *q, int exp)
{
	uint32_t r = __atomic_load_n(q, __ATOMIC_ACQUIRE);
	int mask = (1u << exp) - 1;
	int head = r     & mask;
	int tail = r>>16 & mask;
	return head == tail ? -1 : tail;
}

NONSTD_PLATFORM_API void
queue_pop_commit(uint32_t *q)
{
	__atomic_add_fetch(q, 0x10000, __ATOMIC_RELEASE);
}

NONSTD_PLATFORM_API int
queue_mpop(uint32_t *q, int exp, uint32_t *save)
{
	uint32_t r = *save = __atomic_load_n(q, __ATOMIC_ACQUIRE);
	int mask = (1u << exp) - 1;
	int head = r     & mask;
	int tail = r>>16 & mask;
	return head == tail ? -1 : tail;
}

NONSTD_PLATFORM_API int
queue_mpop_commit(uint32_t *q, uint32_t save)
{
	return __atomic_compare_exchange_n(q, &save, save+0x10000, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

//////////////////////////////////////////////////////////////////////////
// FUTEXES are highly os-specifc, so they get their own section
//
#include <limits.h>
#if defined(__linux__) 
// LINUX
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
static void futex_wait(uint32_t *f, uint32_t expected) { syscall(SYS_futex, f, FUTEX_WAIT, expected, 0, 0, 0); }
static void futex_wake_one(uint32_t *f) { syscall(SYS_futex, f, FUTEX_WAKE, 1, 0, 0, 0); }
static void futex_wake_all(uint32_t *f) { syscall(SYS_futex, f, FUTEX_WAKE, INT_MAX, 0, 0, 0); }
#elif defined(__OPENBSD__) 
// OPENBSD
#include <sys/futex.h>
static void futex_wait(uint32_t *f, uint32_t expected) { futex(f, FUTEX_WAIT, expected, 0, 0); }
static void futex_wake_one(uint32_t *f) { futex(f, FUTEX_WAKE, 1, 0, 0); }
static void futex_wake_all(uint32_t *f) { futex(f, FUTEX_WAKE, INT_MAX, 0, 0); }
#elif defined(__FreeBSD__) 
// FREEBSD
#include <sys/types.h>
#include <sys/umtx.h>
static void futex_wait(uint32_t *f, uint32_t expected) { _umtx_op(f, UMTX_OP_WAIT_UINT, expected, 0, 0); }
static void futex_wake_one(uint32_t *f) { _umtx_op(f, UMTX_OP_WAKE, 1, 0, 0); }
static void futex_wake_all(uint32_t *f) { _umtx_op(f, UMTX_OP_WAKE, INT_MAX, 0, 0); }
#elif defined (_WIN32) 
// WINDOWS
#ifdef _MSC_VER
#  pragma comment(lib, "ntdll.lib")
#endif
__declspec(dllimport) long __stdcall RtlWaitOnAddress(void *, void *, size_t, void *);
__declspec(dllimport) long __stdcall RtlWakeAddressAll(void *);
__declspec(dllimport) long __stdcall RtlWakeAddressSingle(void *);
static void futex_wait(uint32_t *f, uint32_t expected) { RtlWaitOnAddress(f, &expected, sizeof(*f), 0); }
static void futex_wake_one(uint32_t *f) { RtlWakeAddressSingle(f); }
static void futex_wake_all(uint32_t *f) { RtlWakeAddressAll(f); }
#else 
// UNSUPPORTED PLATFORM 
// no-op (hopefully the use case will fall back on a spin lock)
static void futex_wait(uint32_t *f, uint32_t expected) { SPIN_LOOP_HINT(); }
static void futex_wake_one(uint32_t *f) { }
static void futex_wake_all(uint32_t *f) { }
#endif


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


NONSTD_PLATFORM_API void 
event_wait(uint32_t *event)
{
	// 1-bit set: there are waiters
	// 2-bit set: the event has been posted
	while(1) {
		uint32_t v = __atomic_or_fetch(event, 0x1, __ATOMIC_ACQUIRE);
		if (v & 0x02) break;
		futex_wait(event, v);
	}
}

NONSTD_PLATFORM_API void 
event_post(uint32_t *event)
{
	uint32_t v = __atomic_fetch_or(event, 0x2, __ATOMIC_RELEASE);
	if (v & 0x1) futex_wake_all(event);

}

NONSTD_PLATFORM_API void 
event_reset(uint32_t *event)
{
	__atomic_store_n(event, 0, __ATOMIC_RELAXED);
}


NONSTD_PLATFORM_API void 
semaphore_wait(uint32_t *sem)
{
	uint32_t v = 1;
	while(!__atomic_compare_exchange_n(sem, &v, v-1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		if(v == 0) {
			futex_wait(sem, v);
			v = 1;
		}
	}
}

NONSTD_PLATFORM_API void 
semaphore_post(uint32_t *sem)
{
	uint32_t v = __atomic_fetch_add(sem, 1, __ATOMIC_RELEASE);
	assert(v < INT32_MAX);
	//if (v == 0) futex_wake_one(sem); // <-- bug
	//TODO(performance): no syscall if no waiters
	futex_wake_one(sem);
}

NONSTD_PLATFORM_API int  
blocking_queue_push(BlockingConcurrentQueue *q)
{
	semaphore_wait(&q->producer_slots);
	semaphore_wait(&q->access_semaphore);
	int i = queue_push(&q->q, q->exp);
	assert(i >= 0);
	return i;
}

NONSTD_PLATFORM_API void 
blocking_queue_push_commit(BlockingConcurrentQueue *q)
{
	queue_push_commit(&q->q);
	semaphore_post(&q->access_semaphore);
	semaphore_post(&q->consumer_slots);
}

NONSTD_PLATFORM_API int  
blocking_queue_pop(BlockingConcurrentQueue *q)
{
	semaphore_wait(&q->consumer_slots);
	semaphore_wait(&q->access_semaphore);
	int i = queue_pop(&q->q, q->exp);
	assert(i >= 0);
	return i;
}

NONSTD_PLATFORM_API void 
blocking_queue_pop_commit(BlockingConcurrentQueue *q)
{
	queue_pop_commit(&q->q);
	semaphore_post(&q->access_semaphore);
	semaphore_post(&q->producer_slots);
}


/* 
   ........................................
		UNIX-SPECIFC
   ........................................
*/
#if defined(__linux__) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <sys/time.h> // gettimeofday
NONSTD_PLATFORM_API uint64_t
get_os_timer_freq(void) {
	return 1000000ull;
}

NONSTD_PLATFORM_API uint64_t 
read_os_timer(void) {
	struct timeval tval;
	gettimeofday(&tval, 0);
	return (uint64_t)tval.tv_sec * get_os_timer_freq() + (uint64_t)tval.tv_usec;
}


/* 
   ........................................
		WINDOWS-SPECIFC
   ........................................
*/
#elif defined(_WIN32)
#include <windows.h>

NONSTD_PLATFORM_API uint64_t
get_os_timer_freq(void) {
	static uint64_t tick_freq = 1.0;
	static int b = 0;
	if (once_enter(&b)) {
		LARGE_INTEGER x = {0};
		QueryPerformanceFrequency(&x);
		tick_freq = x.QuadPart;
		once_commit(&b);
	}
	return tick_freq;
}

NONSTD_PLATFORM_API uint64_t 
read_os_timer(void) 
{
	LARGE_INTEGER now = {0};
	QueryPerformanceCounter(&now);
	uint64_t wtime = now.QuadPart;
}
#endif

NONSTD_PLATFORM_API double 
cpu_time_to_sec(uint64_t cpu_time_elapsed) 
{
	static int b = 0;
	static uint64_t cpu_freq = 0;
	static double cpu_freq_fp = 0.0;

	if(once_enter(&b)) {
		uint64_t start_cpu = read_cpu_timer();
		uint64_t start_os  = read_os_timer();
		uint64_t elapsed_os = 0;
		while(elapsed_os < 100000) { // 100ms, not 1 full second!
			elapsed_os = read_os_timer()-start_os;
		}
		uint64_t end_cpu = read_cpu_timer();
		uint64_t elapsed_cpu = end_cpu - start_cpu;

		cpu_freq = 1000000ull * elapsed_cpu / elapsed_os;
		cpu_freq_fp = cpu_freq;

		once_commit(&b);
	}

	return (double)cpu_time_elapsed / cpu_freq_fp;
}


NONSTD_PLATFORM_API double 
get_wtime(void) 
{
	return cpu_time_to_sec(read_cpu_timer());
}



/* 
   ........................................
		LAZY LIBC CODE THAT COULD BE PLATFORM-SPECIFIC IN THE FUTURE
   ........................................
*/

#include <stdio.h>
#include <stdlib.h>

NONSTD_PLATFORM_API int
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

NONSTD_PLATFORM_API i64
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

NONSTD_PLATFORM_API FileContents 
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

NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API int 
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
	NONSTD_PLATFORM_API void error_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_PLATFORM_API void warning_message (char * str)
	{
		fprintf(stderr, "%s\n", str);
		fflush(stderr);
	}
	NONSTD_PLATFORM_API void info_message (char * str)
	{
		fprintf(stdout, "%s\n", str);
		fflush(stdout);
	}
#endif

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>


NONSTD_PLATFORM_API _Noreturn void 
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

NONSTD_PLATFORM_API void 
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

NONSTD_PLATFORM_API void 
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
NONSTD_PLATFORM_API i64 platform_get_page_size(void)
{
	return sysconf(_SC_PAGE_SIZE);
}

NONSTD_PLATFORM_API i64 get_total_mem_bytes (void) 
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

NONSTD_PLATFORM_API void 
errmsg_from_platform(char * prefix) 
{
	char msg[128] = {0};
	int e = errno; 
	xsnprintf(msg, sizeof(msg), "%s: %s", prefix, strerror(e));
	error_message(msg);
}

NONSTD_PLATFORM_API void* 
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


NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API int
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

NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API i64 
platform_get_page_size(void)
{
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	return si.dwAllocationGranularity;
}

NONSTD_PLATFORM_API i64 
get_total_mem_bytes (void) 
{
	// Negative return value = error.
	ULONGLONG totalmem_kb = 0;
	if(!GetPhysicallyInstalledSystemMemory(&totalmem_kb)) return -1;
	LONGLONG kb = totalmem_kb;
	return kb * 1024;
}

#include <stdio.h>

NONSTD_PLATFORM_API void 
errmsg_from_platform(char * prefix) {
	char msg[256] = {0};
	unsigned e = GetLastError(); 
	
	xsnprintf(msg, sizeof(msg), "%s: win32 error code %u (0x%x)", prefix, e, e);
	error_message(msg);
}


NONSTD_PLATFORM_API void* 
platform_reserve_mem(size_t size)
{
	void *p = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
	if(p == NULL) {
		errmsg_from_platform("platform_reserve_mem: VirtualAlloc");
		return 0;
	}
	return p;
}


NONSTD_PLATFORM_API int 
platform_commit_mem(void* start, size_t len)
{
	if(NULL == VirtualAlloc(start, len, MEM_COMMIT, PAGE_READWRITE)){
		errmsg_from_platform("platform_commit_mem: VirtualAlloc");
		return 0;
	}
	return 1;
}

NONSTD_PLATFORM_API int 
platform_lock_mem(void *start, size_t len)
{
	if(!VirtualLock(start, len)) {
		errmsg_from_platform("platform_lock_mem: VirtualLock");
		return 0;
	}
	return 1;
}

NONSTD_PLATFORM_API int 
platform_unlock_mem(void *start, size_t len)
{
	if(!VirtualUnlock(start, len)) {
		errmsg_from_platform("platform_unlock_mem: VirtualUnlock");
		return 0;
	}
	return 1;
}

NONSTD_PLATFORM_API int 
platform_decommit_mem(void* start, size_t len)
{
	if(!VirtualFree(start, len, MEM_DECOMMIT)) {
		errmsg_from_platform("platform_decommit_mem: VirtualFree");
		return 0;
	}
	return 1;
}

NONSTD_PLATFORM_API int 
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


NONSTD_PLATFORM_API void * 
xmalloc(i64 bytes) 
{
	void *p = malloc(bytes);
	if(!p) die("xmalloc failed to allocate %lli bytes", (long long) bytes);
	memset(p,0,bytes);
	return p;
}

NONSTD_PLATFORM_API void * 
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

NONSTD_PLATFORM_API int 
allocation_check_name(void *p, char *name, int name_len)
{
	AllocationHeader *h = get_header(p);
	assert(name_len < (i64)sizeof(h->padding));
	return name_len == h->name_len && 0 == memcmp(name,h->padding,name_len);
}


NONSTD_PLATFORM_API i64 
arena_get_used_memory(Arena *a)
{
	return a->used;
}
					    
NONSTD_PLATFORM_API i64 
allocation_size(void *p)
{
	return get_header(p)->sz;
}

NONSTD_PLATFORM_API i64 
allocation_capacity(void *p)
{
	return get_header(p)->cap;
}

NONSTD_PLATFORM_API i64 
arena_checkpoint(Arena *a)
{
	return a->used;
}

NONSTD_PLATFORM_API void 
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

NONSTD_PLATFORM_API void* 
allocate_named  (Arena *a, i64 sz_, char *name, int name_len) 
{
	// zeros memory
	void *mem = allocate_named_(a, sz_, name, name_len);
	memset(mem,0,sz_);
	return mem;
}

NONSTD_PLATFORM_API void* 
allocate (Arena *a, i64 sz_) 
{
	// zeros memory
	return allocate_named(a,sz_,0,0);
}


NONSTD_PLATFORM_API void* 
allocate_empty(Arena *a, i64 sz_) 
{
	// leaves memory uninitialized
	return allocate_named_(a,sz_,0,0);
}

NONSTD_PLATFORM_API void* 
allocate_empty_named  (Arena *a, i64 sz_, char *name, int name_len) 
{
	// leaves memory uninitialized
	return allocate_named_(a, sz_, name, name_len);
}



NONSTD_PLATFORM_API void 
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

NONSTD_PLATFORM_API void 
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

NONSTD_PLATFORM_API int 
arena_dump_file(Arena *a, char * filename) 
{
	return platform_write_file(filename, a->mem, a->used);
}


NONSTD_PLATFORM_API i64 
arena_dump(i64 bufsz, void *buf, Arena *a)
{
	i64 cpysz = bufsz < a->used  ?  bufsz  :  a->used;
	assert(cpysz==0 || a->mem);
	memcpy(buf, a->mem, cpysz);
	return a->used;
}

NONSTD_PLATFORM_API Arena 
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



NONSTD_PLATFORM_API void *
allocation_copy(Arena *a, void *src_data)
{
	AllocationHeader *src_hdr = get_header(src_data);

	void * dst_data = allocate(a, src_hdr->sz);
	AllocationHeader *dst_hdr = get_header(dst_data);

	memcpy(dst_hdr, src_hdr, sizeof(*dst_hdr));
	memcpy(dst_data, src_data, src_hdr->sz);

	return dst_data;
}

NONSTD_PLATFORM_API void 
arena_mem_lock(Arena *a)
{
	assert(platform_lock_mem(a->mem, a->used));
}

NONSTD_PLATFORM_API void 
arena_mem_unlock(Arena *a)
{
	assert(platform_unlock_mem(a->mem, a->used));
}


NONSTD_PLATFORM_API void *
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

NONSTD_PLATFORM_API char* 
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

NONSTD_PLATFORM_API char* 
allocate_cstrdup(Arena *a, char *cstr)
{
        if(!string) return 0;
        int len = strlen(string);
        char *mem = allocate(a, len+1);
        memcpy(mem, string, len);
        return mem;
}

NONSTD_PLATFORM_API AllocationHeader * 
arena_foreach(Arena *a, i64 *state)
{
	assert(*state > -1 && *state <= a->used);
	if (*state == a->used) return 0;
	AllocationHeader *h = (AllocationHeader*) (a->mem + *state);
	assert(h->magic == TALLOC_HEADER_MAGIC);
	*state += h->cap + sizeof(*h);
	return h;
}


NONSTD_PLATFORM_API int 
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

NONSTD_PLATFORM_API void 
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
