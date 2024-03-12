/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	The implementation section of nonstd_platform.h depends on nonstd.h.
*/
#ifndef NONSTD_PLATFORM_H
#define NONSTD_PLATFORM_H

#ifndef NONSTD_PLATFORM_API
#define NONSTD_PLATFORM_API 
#endif

#include <stdint.h>
#include <stddef.h>

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
		I/O
   ============================================================================
*/
typedef struct {
	int64_t len;
	void *mem;
} FileContents;

NONSTD_PLATFORM_API  FileContents platform_read_file(char *filename);

NONSTD_PLATFORM_API  int platform_read_file_into_buffer(int64_t buffer_size, void *buffer, int64_t *file_size, char *filename);
NONSTD_PLATFORM_API  int platform_write_file(char * filename, void *what, size_t bytes);

NONSTD_PLATFORM_API  int64_t platform_get_file_size(char *filename);

// Writes out the message from errno or GetLastError with a user-provided message prefix
NONSTD_PLATFORM_API  void errmsg_from_platform(char * prefix);



/* 
   ============================================================================
		MEMORY MANAGEMENT
   ============================================================================
*/

NONSTD_PLATFORM_API  int64_t get_total_mem_bytes (void);  
// return total machine memory size in bytes




/* 
   ============================================================================
		VIRTUAL MEMORY MANAGEMENT
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

#include "nonstd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


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

NONSTD_PLATFORM_API int64_t
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
	
	int64_t pos = FTELL(f);
	
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
	int64_t len = FTELL(f);
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
platform_read_file_into_buffer(int64_t buffer_size, void *buffer, int64_t *file_size, char *filename)
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
	
	int64_t pos = FTELL(f);
	
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
		if(*file_size != (int64_t)fread(buffer, 1, *file_size, f)) {
			errmsg_from_platform("platform_read_file_into_buffer: fread");
			return 0; 
		}
	}

	fclose(f);
	return 1;
}






/*
   ........................................
		LINUX-SPECIFIC
   ........................................
*/

#if defined(__linux__)
#define _GNU_SOURCE
#include <unistd.h>   // _SC_PAGE_SIZE, etc
NONSTD_PLATFORM_API int64_t platform_get_page_size(void)
{
	return sysconf(_SC_PAGE_SIZE);
}

NONSTD_PLATFORM_API int64_t get_total_mem_bytes (void) 
{
	// Negative return value = error.
	int64_t ps = platform_get_page_size();
	int64_t pp = sysconf(_SC_PHYS_PAGES);
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

NONSTD_PLATFORM_API void 
errmsg_from_platform(char * prefix) 
{
	char msg[128] = {0};
	int e = errno; 
	snprintf(msg, sizeof(msg), "%s: %s", prefix, strerror(e));
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

static int64_t offset_from_prev_page_boundary(void* addr)
{
	int64_t start_of_page = round_down((intptr_t)addr, platform_get_page_size());
	int64_t rtn_val = ((intptr_t)addr) - start_of_page;
	assert(rtn_val >= 0);
	return rtn_val;
}


NONSTD_PLATFORM_API int 
platform_commit_mem(void* start, size_t len)
{
	int64_t offset = offset_from_prev_page_boundary(start);
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
	int64_t offset = offset_from_prev_page_boundary(start);
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
	int64_t offset = offset_from_prev_page_boundary(start);
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
	int64_t offset = offset_from_prev_page_boundary(start);
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

NONSTD_PLATFORM_API int64_t 
platform_get_page_size(void)
{
	SYSTEM_INFO si = {0};
	GetSystemInfo(&si);
	return si.dwAllocationGranularity;
}

NONSTD_PLATFORM_API int64_t 
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
	
	snprintf(msg, sizeof(msg), "%s: win32 error code %u (0x%x)", prefix, e, e);
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


#endif
