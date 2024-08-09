#ifndef NONSTD_PROGRESSBAR

#include <stdio.h>
#include <stdint.h>
typedef struct {
        // you can update these at any time
        int current, maximum;  // required for the bar to work 
			       // if maximum is 0, only rate info printed
        char prefix_label[32]; // optional, safe to leave zero
        char suffix_label[64]; // optional, safe to leave zero
        FILE *output_stream;   // optional, defaults to stderr

        // don't manually edit these
        int last_current, last_output_len;
        uint64_t last_update_time;
} ProgressBar;

int progress_bar_ratelimit(ProgressBar *b);
//  returns true if at least 100 ms (by default) has elapsed since last print

void progress_bar_print(ProgressBar *b);
// print the bar in its current state


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
#ifdef NONSTD_PROGRESSBAR_TEST
#define NONSTD_PLATFORM_IMPLEMENTATION
#define NONSTD_IMPLEMENTATION
#define NONSTD_PROGRESSBAR_IMPLEMENTATION

#include <limits.h>
int main(void)
{
        ProgressBar bar = {.maximum=INT_MAX};
        sprintf(bar.prefix_label, "Progress: ");

        for (int i = 0; i < INT_MAX; i++)
        {
                if (progress_bar_ratelimit(&bar)) {
                        bar.current = i;
                        progress_bar_print(&bar);
                }
        }
        printf("\n");

}
#endif



#ifdef NONSTD_PROGRESSBAR_IMPLEMENTATION

#include "nonstd_platform.h" // cpu timer
			     
#define NONSTD_PROGRESSBAR_RATELIMIT_TIME_MS 100.0
#ifndef NONSTD_PROGRESSBAR_FILL_CHARACTER
#define NONSTD_PROGRESSBAR_FILL_CHARACTER '#'
#endif


int
progress_bar_ratelimit(ProgressBar *b)
{
        uint64_t delta_ticks = read_cpu_timer() - b->last_update_time;
        float delta_t = cpu_time_to_sec(delta_ticks);
        return delta_t*1000.0f >= NONSTD_PROGRESSBAR_RATELIMIT_TIME_MS;
}

void 
progress_bar_print(ProgressBar *b)
{
        uint64_t now = read_cpu_timer();

        FILE * f = b->output_stream ? b->output_stream : stderr;
        for (int i = 0; i < b->last_output_len; i++)
                fputc('\b',f);

	uint64_t delta_ticks = now - b->last_update_time;
	float delta_seconds = cpu_time_to_sec(delta_ticks);
	int delta_iters = b->current - b->last_current;
	float it_per_s = delta_iters / delta_seconds;

	int n = 0;
	if(b->maximum > 0) {
		int cutoff = (60.0f * b->current) / b->maximum + 0.5f;

		char bar[61] = {0};
		for (int i = 0; i < 60; i++)
			bar[i] = i <= cutoff ? NONSTD_PROGRESSBAR_FILL_CHARACTER : ' ';
		n = fprintf(f, "%s[%s] (%.2f it/s) %s", b->prefix_label, bar, it_per_s, b->suffix_label);
	} else {
		n = fprintf(f, "%s %iit (%.2f it/s) %s", b->prefix_label, b->current, it_per_s, b->suffix_label);
	}

        b->last_current = b->current;
        b->last_output_len = (n > 0) ? n : 0;
        b->last_update_time = now;
}
#endif


