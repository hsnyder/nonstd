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
#if defined(__x86_64__)
	return __builtin_ia32_rdtsc(); 
#else 
#if defined (__aarch64__)
	return __builtin_readcyclecounter();
#endif
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
	Lock free concurrent spin-lock queue.
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


#ifdef __x86_64__
// on x86, the 'pause' instruction informs the CPU that it is in a spin-lock, which allows it
// to make the appropriate decisions with regard to speculative execution, etc.
#define PAUSE()	__asm __volatile ("pause"); 
#else
#define PAUSE() 
#endif

NONSTD_ARCH_API void 
ticket_mutex_lock(TicketMutex *m)
{
	uint32_t my_ticket = __atomic_fetch_add(&m->ticket, 1, __ATOMIC_RELAXED);
	while (my_ticket != __atomic_load_n(&m->serving, __ATOMIC_ACQUIRE)) {
		PAUSE();
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
		PAUSE();
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
