#define NONSTD_IMPLEMENTATION
#define NONSTD_API static 
#include "../nonstd/nonstd.h"


#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define EXP 3
#define NTHD 2
#define NREP 10

u64 state = 0xdeadbeefdeadbeef;
BlockingConcurrentQueue q = BLOCKING_CONCURRENT_QUEUE_INITIALIZER(EXP);
uint32_t slots[1<<EXP] = {0};

void repr_q(BlockingConcurrentQueue q) {
	printf("\tq.exp = %i\n", q.exp);
	printf("\tq.producer_slots = %i\n", q.producer_slots);
	printf("\tq.consumer_slots = %i\n", q.consumer_slots);
	printf("\tq.access_semaphore = %i\n", q.access_semaphore);
	printf("\tq.q = %x\n", q.q);
}

void *pfn (void *threadid)
{
	int tid = (intptr_t)threadid;

	for(int i = 0; i < NREP; i++) {
		int k = blocking_queue_push(&q);
		uint32_t v = rand_pcg32(&state) & 0xffff;
		printf("%i %i producing %x\n",tid,i,v);
		//repr_q(q);
		slots[k] = v;
		blocking_queue_push_commit(&q);
	}

	printf("producer %i exit\n", tid);

	return 0;
}

void *cfn (void *threadid)
{
	int tid = (intptr_t)threadid;

	for(int i = 0; i < NREP; i++) {
		int k = blocking_queue_pop(&q);
		uint32_t v = slots[k];
		printf("%i %i received %x\n",tid,i,v);
		//repr_q(q);
		blocking_queue_pop_commit(&q);
	}

	printf("consumer %i exit\n", tid);

	return 0;
}

int main (void)
{

	pthread_t p[NTHD] = {0};
	pthread_t c[NTHD] = {0};
	for (int i = 0; i < COUNT_ARRAY(p); i++) {
		pthread_create(&p[i], 0, pfn, (intptr_t)i);
	}
	for (int i = 0; i < COUNT_ARRAY(c); i++) {
		pthread_create(&c[i], 0, cfn, (intptr_t)i);
	}

	for (int i = 0; i < COUNT_ARRAY(p); i++) {
		void *nothing = 0;
		pthread_join(p[i], &nothing);
	}
	for (int i = 0; i < COUNT_ARRAY(c); i++) {
		void *nothing = 0;
		pthread_join(c[i], &nothing);
	}
}
