/*
	Harris M. Snyder, 2023
	This is free and unencumbered software released into the public domain.

	nonstd_arch.h is part of 'nonstd': an attempt to supplement the C 
	standard library. See the comments in `nonstd.h` for an overview.
*/
#ifndef NONSTD_ARCH_H
#define NONSTD_ARCH_H

#ifndef NONSTD_ARCH_API
#define NONSTD_ARCH_API 
#endif

#include <stdint.h>


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
NONSTD_ARCH_API double cpu_time_to_sec(uint64_t cpu_time_elapsed) ;

/* 
	Return wall-clock time in seconds. 
	What point is defined as "zero" time is undefined,
	so differences are meaningful but not an individual time. 
	Uses cpu_time_to_sec, so be aware of the one time 100ms block.
*/
NONSTD_ARCH_API double get_wtime(void); 


/*
	Returns the frequency of the OS timer in counts per second
*/
NONSTD_ARCH_API uint64_t get_os_timer_freq(void);

/*
	Query the current OS time. Zero reference time is not guaranteed to be any particular thing
*/
NONSTD_ARCH_API uint64_t read_os_timer(void);



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

NONSTD_ARCH_API void ticket_mutex_lock(TicketMutex *m);    
NONSTD_ARCH_API void ticket_mutex_unlock(TicketMutex *m);


/*
	"once barrier".. useful if you need some initialization code to be called exactly once.
	usage is:

	static int b = 0; // init to zero is important

	if (once_enter(&b)) {
		do_initialization_work();
		once_commit(&b);
	} 
*/

NONSTD_ARCH_API int once_enter(int *b); 
// Returns true if you are the thread that needs to do the init work.

NONSTD_ARCH_API void once_commit(int *b); 
// Call this once you're done doing init work.

/*
	Lock free concurrent queue.
	Credit to Chris Wellons for the idea: 
        https://nullprogram.com/blog/2022/05/14/ (public domain)
	Operation fully explained in the above link.
*/
NONSTD_ARCH_API int  queue_push(uint32_t *q, int exp);
NONSTD_ARCH_API void queue_push_commit(uint32_t *q);

NONSTD_ARCH_API int  queue_pop(uint32_t *q, int exp);
NONSTD_ARCH_API void queue_pop_commit(uint32_t *q);

NONSTD_ARCH_API int  queue_mpop(uint32_t *q, int exp, uint32_t *save);
NONSTD_ARCH_API int  queue_mpop_commit(uint32_t *q, uint32_t save);

/*
	Unfair blocking semaphore.

	Uses futexes on supported operating systems to put threads to sleep until 
	the resource is available (uses spin-locking if futexes aren't availble).
	The implementation isn't optimal: semaphore_post makes a system call even
	if there are no waiters, but since only one waiter is woken each post, 
	there's no thundering herd effect.

	Note: the maximum supported value for `sem` is INT32_MAX, not UINT32_MAX.
*/
NONSTD_ARCH_API void semaphore_wait(uint32_t *sem);
NONSTD_ARCH_API void semaphore_post(uint32_t *sem);

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

NONSTD_ARCH_API int  blocking_queue_push(BlockingConcurrentQueue *q);
NONSTD_ARCH_API void blocking_queue_push_commit(BlockingConcurrentQueue *q);

NONSTD_ARCH_API int  blocking_queue_pop(BlockingConcurrentQueue *q);
NONSTD_ARCH_API void blocking_queue_pop_commit(BlockingConcurrentQueue *q);

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
#ifdef NONSTD_ARCH_IMPLEMENTATION


// Most architectures have special instructions which hint to the CPU that we're in a spin-lock loop.
#if   defined(__x86_64__)
#define SPIN_LOOP_HINT()  __asm __volatile ("pause"); 
#elif defined(__arm__)
#define SPIN_LOOP_HINT()  __asm __volatile ("yield"); 
#else
#define SPIN_LOOP_HINT() 
#endif

NONSTD_ARCH_API void 
ticket_mutex_lock(TicketMutex *m)
{
	uint32_t my_ticket = __atomic_fetch_add(&m->ticket, 1, __ATOMIC_RELAXED);
	while (my_ticket != __atomic_load_n(&m->serving, __ATOMIC_ACQUIRE)) {
		SPIN_LOOP_HINT();
	}
}

NONSTD_ARCH_API void 
ticket_mutex_unlock(TicketMutex *m) 
{
        (void) __atomic_fetch_add(&m->serving, 1, __ATOMIC_RELEASE);
}

NONSTD_ARCH_API int 
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

NONSTD_ARCH_API void 
once_commit(int *b)
{
	(void) __atomic_store_n(b, 2, __ATOMIC_SEQ_CST);
}


NONSTD_ARCH_API int
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

NONSTD_ARCH_API void
queue_push_commit(uint32_t *q)
{
	__atomic_add_fetch(q, 1, __ATOMIC_RELEASE);
}

NONSTD_ARCH_API int
queue_pop(uint32_t *q, int exp)
{
	uint32_t r = __atomic_load_n(q, __ATOMIC_ACQUIRE);
	int mask = (1u << exp) - 1;
	int head = r     & mask;
	int tail = r>>16 & mask;
	return head == tail ? -1 : tail;
}

NONSTD_ARCH_API void
queue_pop_commit(uint32_t *q)
{
	__atomic_add_fetch(q, 0x10000, __ATOMIC_RELEASE);
}

NONSTD_ARCH_API int
queue_mpop(uint32_t *q, int exp, uint32_t *save)
{
	uint32_t r = *save = __atomic_load_n(q, __ATOMIC_ACQUIRE);
	int mask = (1u << exp) - 1;
	int head = r     & mask;
	int tail = r>>16 & mask;
	return head == tail ? -1 : tail;
}

NONSTD_ARCH_API int
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



NONSTD_ARCH_API void 
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

NONSTD_ARCH_API void 
semaphore_post(uint32_t *sem)
{
	uint32_t v = __atomic_fetch_add(sem, 1, __ATOMIC_RELEASE);
	assert(v < INT32_MAX);
	//if (v == 0) futex_wake_one(sem); // <-- bug
	//TODO(performance): no syscall if no waiters
	futex_wake_one(sem);
}

NONSTD_ARCH_API int  
blocking_queue_push(BlockingConcurrentQueue *q)
{
	semaphore_wait(&q->producer_slots);
	semaphore_wait(&q->access_semaphore);
	int i = queue_push(&q->q, q->exp);
	assert(i >= 0);
	return i;
}

NONSTD_ARCH_API void 
blocking_queue_push_commit(BlockingConcurrentQueue *q)
{
	queue_push_commit(&q->q);
	semaphore_post(&q->access_semaphore);
	semaphore_post(&q->consumer_slots);
}

NONSTD_ARCH_API int  
blocking_queue_pop(BlockingConcurrentQueue *q)
{
	semaphore_wait(&q->consumer_slots);
	semaphore_wait(&q->access_semaphore);
	int i = queue_pop(&q->q, q->exp);
	assert(i >= 0);
	return i;
}

NONSTD_ARCH_API void 
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
NONSTD_ARCH_API uint64_t
get_os_timer_freq(void) {
	return 1000000ull;
}

NONSTD_ARCH_API uint64_t 
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

NONSTD_ARCH_API uint64_t
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

NONSTD_ARCH_API uint64_t 
read_os_timer(void) 
{
	LARGE_INTEGER now = {0};
	QueryPerformanceCounter(&now);
	uint64_t wtime = now.QuadPart;
}
#endif

NONSTD_ARCH_API double 
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


NONSTD_ARCH_API double 
get_wtime(void) 
{
	return cpu_time_to_sec(read_cpu_timer());
}


#endif
