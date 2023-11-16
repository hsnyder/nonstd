#define NONSTD_IMPLEMENTATION
#define NONSTD_API static 
#include "../nonstd/nonstd.h"


#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

u64 state = 0xdeadbeefdeadbeef;
int32_t sem = 1;

void *tfn (void *nothing)
{
	(void) nothing;

	for(int i = 0; i < 100000; i++) {
		semaphore_wait(&sem);
		int v = 1 + (int)(rand_pcg32(&state) & 0xffffu);
		if(i % 1000 == 0) printf("%i\n",i);
		semaphore_post(&sem);
		while (v--) SPIN_LOOP_HINT();
	}

	semaphore_wait(&sem);
	printf("done\n");
	semaphore_post(&sem);

	return 0;
}

int main (void)
{

	pthread_t t[8] = {0};
	for (int i = 0; i < COUNT_ARRAY(t); i++) {
		pthread_create(&t[i], 0, tfn, 0);
	}
	for (int i = 0; i < COUNT_ARRAY(t); i++) {
		void *nothing = 0;
		pthread_join(t[i], &nothing);
	}
}
