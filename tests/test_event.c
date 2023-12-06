#define NONSTD_IMPLEMENTATION
#define NONSTD_API static
#include "../nonstd/nonstd.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#define NTHD 6

int main (void)
{
	uint32_t ev = 0;
	int val = 0;
	for (int i = 0; i < 1000; i++)
	{
		#pragma omp parallel for num_threads(NTHD)
		for(int t = 0; t < NTHD; t++)
		{
			if (t == 0) {
				usleep(1000);
				val++;
				event_post(&ev);
			} else {
				event_wait(&ev);
				printf("%i read: %i\n", t, val);
			} 
		}
		event_reset(&ev);
		printf("--\n");
	}
}
