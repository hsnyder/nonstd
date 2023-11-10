// Don't include this file directly, it is included by numerics.h

/*
   ============================================================================
		BASIC REDUCTIONS
   ============================================================================
*/


// 1-dimensional min and max functions
// these return 0 if N is zero, otherwise 1.
int NAME(minmax)  (TYPE     *min,  TYPE     *max,  int64_t N, TYPE     *data) ;

TYPE   NAME(max)  (int64_t N, TYPE *data); 
TYPE   NAME(min)  (int64_t N, TYPE *data); 
double NAME(mean) (int64_t N, TYPE *data);
double NAME(stdev) (int64_t N, TYPE *data, double mean);

/*
   ============================================================================
		HISTOGRAMS
   ============================================================================
*/

/*
	The bins array should be Nbins+1 in size, each bin is the half-open set [bins[i], bins[i+1]).

	Setting auto_bins will make the function compute the bins for you, but you must still 
	provide Nbins, and a buffer big enough for Nbins+1.

	Returns the number of points of the data array that fell into the range covered by the bins.
	i.e. if the return value != Ndata, then some values were outside your bins. This will
	never happen if the bins are automatically generated.

	The bins do not need to be uniform, but they will be uniform if automatically generated.

	The counts array is the output histogram, and must be Nbins elements long.

*/

int64_t NAME(histogram) (int64_t Nbins, double *bins, int64_t *counts, bool auto_bins, int64_t Ndata, TYPE *data);


/*
   ============================================================================
		REVERSING AND TRANSPOSING
   ============================================================================
*/

void NAME(reverse) (int64_t N, TYPE *arr);
void NAME(transpose) (int64_t rows, int64_t cols, TYPE *output, TYPE *input);

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
#ifdef NUMERICS_MULTITYPE_IMPLEMENTATION 

#ifndef assert
#  ifdef DISABLE_ASSERTIONS
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

int NAME(minmax)  (TYPE *min,  TYPE *max,  int64_t N, TYPE  *data)  
{
	if (N==0) return 0; 
	assert(N>0); 
	assert(data); 
	TYPE max_ = SMALLEST_VALUE(*max); 
	TYPE min_ = LARGEST_VALUE(*min); 
	for (int64_t i = 0; i < N; i++) { 
		max_ = data[i] > max_ ? data[i] : max_; 
		min_ = data[i] < min_ ? data[i] : min_; 
	} 
	*max = max_;
	*min = min_; 
	return 1;
}

TYPE  NAME(min) (int64_t N, TYPE *data)    
{  
	TYPE min = 0, max = 0;    
	(void) NAME(minmax)  (&min, &max, N, data); 
	return min; 
}

TYPE  NAME(max) (int64_t N, TYPE *data)    
{  
	TYPE min = 0, max = 0;    
	(void) NAME(minmax)  (&min, &max, N, data); 
	return max; 
}

double  NAME(mean) (int64_t N, TYPE *data) 
{ 
	double sum = 0.0; 
	for (int64_t i = 0; i < N; i++) sum += data[i]; 
	return sum/N;
}

double  NAME(stdev) (int64_t N, TYPE *data, double mean) 
{ 
	if (mean == NAN) {
		mean = NAME(mean) (N, data);
	}

	double accum = 0.0; 
	for (int64_t i = 0; i < N; i++) accum += (data[i]-mean) * (data[i]-mean);
	return sqrt(accum/N);
}



int64_t  NAME(histogram) (int64_t Nbins, double *bins, int64_t *counts, bool auto_bins, int64_t Ndata, TYPE *data)
{
	// returns the number of values that fit into the specified bins.
	// i.e. caller can check that Ndata == histogramf(...), and if it doesn't,
	// then the caller provided bins that don't include the full range of data.
	
	assert (bins);
	assert (Nbins > 0);

	assert (data);
	assert (Ndata >= 0);

	if (Ndata == 0) 
		return 0;

	// the memory for bins should be Nbins+1 long.
	// Each point is the lower bound of bin N and bins are defined such that they
	// contain values [bins[i], bins[i]). The last value in Nbins (bins[Nbins]) is the 
	// upper bound of the last bin.

	if (auto_bins) {
		TYPE min = 0, max = 0;
		(void) NAME(minmax)(&min, &max, Ndata, data);
		max = nextafter((double)max, DBL_MAX); // bins are [a,b)

		double step = (max-min)/Nbins;
		for (int64_t i = 0; i < Nbins; i++)
			bins[i] = min + i * step; 
		bins[Nbins] = max;
	}

	for (int64_t b = 0; b < Nbins; b++)
		counts[b] = 0;

	int64_t Nfit = 0;

	for (int64_t i = 0; i < Ndata; i++) {
		for (int64_t b = 0; b < Nbins; b++) {
			double d = data[i];
			if (d >= bins[b] && d < bins[b+1]) {
				counts[b]++;
				Nfit++;
			}
		}
	}

	if(auto_bins) 
		assert(Nfit == Ndata); // if we made our own bins and we missed data, that's a bug

	return Nfit;
}

void NAME(reverse) (int64_t N, TYPE *arr)
{
	/* Simple enough that GCC figures this out and does the right SIMD thing */
	for (int64_t i = 0; i < N/2; i++) {
		TYPE t = arr[i];
		arr[i] = arr[N-i-1];
		arr[N-i-1] = t;
	}
}

void NAME(transpose) (int64_t rows, int64_t cols, TYPE *output, TYPE *input)
{
	/* This is unoptimized and quite slow */
	for (int i = 0; i < rows; i++) 
	for (int j = 0; j < cols; j++) { 
		output[j*rows + i] = input[i*cols + j];
	}
}

#endif
