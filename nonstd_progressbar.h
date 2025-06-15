#ifndef NONSTD_PROGRESSBAR
#define NONSTD_PROGRESSBAR

#include <stdio.h>
#include <stdint.h>
typedef struct {
    // you can update these at any time
    int64_t current, maximum;  // required for the bar to work
		       // if maximum is 0, only rate info printed
    char prefix_label[32]; // optional, safe to leave zero
    char suffix_label[64]; // optional, safe to leave zero
    FILE *output_stream;   // optional, defaults to stderr
	int always_show_absolute_progress;
	int width;

    // don't manually edit these
	int last_output_len;
    int64_t last_current;
	int64_t first_update_progress;
    uint64_t first_update_time;
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
#include <inttypes.h>
			     
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

	uint64_t delta_ticks = now - b->first_update_time;
	float delta_seconds = cpu_time_to_sec(delta_ticks);
	int64_t delta_iters = b->current - b->first_update_progress;
	float it_per_s = delta_iters / delta_seconds;

	if (b->width == 0) b->width = 60;
	if (b->width > 60) b->width = 60;

	int n = 0;
	if(b->maximum > 0) {
		int cutoff = ((float)b->width * b->current) / b->maximum + 0.5f;

		char bar[61] = {0};
		for (int i = 0; i < b->width; i++)
			bar[i] = i <= cutoff ? NONSTD_PROGRESSBAR_FILL_CHARACTER : ' ';
		if (it_per_s > 1.0f) {
			if (b->always_show_absolute_progress) {
				n = fprintf(f, "%s[%s] %" PRIi64 "/%" PRIi64 " (%.2f it/s) %s", b->prefix_label, bar, b->current, b->maximum, it_per_s, b->suffix_label);
			} else {
				n = fprintf(f, "%s[%s] (%.2f it/s) %s", b->prefix_label, bar, it_per_s, b->suffix_label);
			}
		} else {
			float s_per_it = 1.0f / it_per_s;
			if (b->always_show_absolute_progress) {
				n = fprintf(f, "%s[%s] %" PRIi64 "/%" PRIi64 " (%.2f s/it) %s", b->prefix_label, bar, b->current, b->maximum, s_per_it, b->suffix_label);
			} else {
				n = fprintf(f, "%s[%s] (%.2f s/it) %s", b->prefix_label, bar, s_per_it, b->suffix_label);
			}
		}
	} else {
		if (it_per_s > 1.0f) {
			n = fprintf(f, "%s %" PRIi64 "it (%.2f it/s) %s", b->prefix_label, b->current, it_per_s, b->suffix_label);
		} else {
			float s_per_it = 1.0f / it_per_s;
			n = fprintf(f, "%s %" PRIi64 "it (%.2f s/it) %s", b->prefix_label, b->current, s_per_it, b->suffix_label);
		}
	}

    b->last_current = b->current;
    b->last_output_len = (n > 0) ? n : 0;
	b->last_update_time = now;
	if(b->first_update_time == 0) {
		b->first_update_time = now;
		b->first_update_progress = b->current;
	}

}
#endif


