/*
    Copyright 2019-2022 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations with pde solvers using jacobi iteration
        (common in imaging, simulation, and scientific computations)


Assumptions:

    1) Most likely, there will be no single best implementation for all types.
        "Best" performance depends a lot on instruction latencies and register availability.

    2) Better implementations should be able to vectorize much of the computation.



NOTE - Somehow LLVM is doing a better job vectorizing the reverse loops than the forward loops ????



Laplace 1D can be solved directly, so not too interesting
Laplace 2D and 3D need numerical approximation methods

NOTE - on large problems: Jacobi methods doesn't so much "converge" as "run out of bits and give up"

NOTE - with integer types: Jacobi methods probably won't "converge", but just oscillate and lose precision


TODO - Jacobi 8 neighbors
TODO - SOR 8 neighbors

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"
#include "benchmark_shared_pde.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 600;


// ~ 6 million items (src plus dest), intended to be larger than L2 cache on common CPUs
const int WIDTH = 1500;
const int HEIGHT = 2000;

const int SIZE = HEIGHT*WIDTH;


// smaller buffers for testing convergence rate
const int SMALL_WIDTH = 250;
const int SMALL_HEIGHT = 300;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

/*
    2D convolution
    hard coded filter
    edges are constant
    
          1
        1 0 1
          1
    result divided by 4
*/
template <typename T, typename TS>
struct jacobi2D_simple {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-1); ++x) {
            
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// swap loops (bad cache order)

template <typename T, typename TS>
struct jacobi2D_swapped {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (x = 1; x < (cols-1); ++x) {
        
            for (y = 1; y < (rows-1); ++ y) {

                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// reverse loops (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi2D_reversed {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = (rows-2); y >= 1; --y) {

            for (x = (cols-2); x >= 1; --x) {
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// reverse one loop (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi2D_reversedX {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = (cols-2); x >= 1; --x) {
            
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// reverse one loop (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi2D_reversedY {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = (rows-2); y >= 1; --y) {

            for (x = 1; x < (cols-1); ++x) {
            
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// unroll inner loop

template <typename T, typename TS>
struct jacobi2D_unrolled {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-4); x += 4) {
            
                TS sum0 = source[((y-1)*rowStep)+(x+0)];
                sum0 +=   source[((y+0)*rowStep)+(x-1)];
                sum0 +=   source[((y+0)*rowStep)+(x+1)];
                sum0 +=   source[((y+1)*rowStep)+(x+0)];
                
                TS sum1 = source[((y-1)*rowStep)+(x+1)];
                sum1 +=   source[((y+0)*rowStep)+(x-0)];
                sum1 +=   source[((y+0)*rowStep)+(x+2)];
                sum1 +=   source[((y+1)*rowStep)+(x+1)];
                
                TS sum2 = source[((y-1)*rowStep)+(x+2)];
                sum2 +=   source[((y+0)*rowStep)+(x+1)];
                sum2 +=   source[((y+0)*rowStep)+(x+3)];
                sum2 +=   source[((y+1)*rowStep)+(x+2)];
                
                TS sum3 = source[((y-1)*rowStep)+(x+3)];
                sum3 +=   source[((y+0)*rowStep)+(x+2)];
                sum3 +=   source[((y+0)*rowStep)+(x+4)];
                sum3 +=   source[((y+1)*rowStep)+(x+3)];
                
                T temp0 = T( (sum0+half) / TS(4) );
                T temp1 = T( (sum1+half) / TS(4) );
                T temp2 = T( (sum2+half) / TS(4) );
                T temp3 = T( (sum3+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp0;
                dest[((y+0)*rowStep)+(x+1)] = temp1;
                dest[((y+0)*rowStep)+(x+2)] = temp2;
                dest[((y+0)*rowStep)+(x+3)] = temp3;
            }

            for ( ; x < (cols-1); ++x) {
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)] + source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }

};

/******************************************************************************/

// unroll inner loop
// reorder operations and make it look like a 4 item vector

template <typename T, typename TS>
struct jacobi2D_unrolled2 {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-4); x += 4) {
                TS sum_vec[4];
                T temp_vec[4];
                
                sum_vec[0] = source[((y-1)*rowStep)+(x+0)];
                sum_vec[1] = source[((y-1)*rowStep)+(x+1)];
                sum_vec[2] = source[((y-1)*rowStep)+(x+2)];
                sum_vec[3] = source[((y-1)*rowStep)+(x+3)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x-1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x-0)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+2)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+4)];
                
                sum_vec[0] += source[((y+1)*rowStep)+(x+0)];
                sum_vec[1] += source[((y+1)*rowStep)+(x+1)];
                sum_vec[2] += source[((y+1)*rowStep)+(x+2)];
                sum_vec[3] += source[((y+1)*rowStep)+(x+3)];
                
                temp_vec[0] = T( (sum_vec[0]+half) / TS(4) );
                temp_vec[1] = T( (sum_vec[1]+half) / TS(4) );
                temp_vec[2] = T( (sum_vec[2]+half) / TS(4) );
                temp_vec[3] = T( (sum_vec[3]+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp_vec[0];
                dest[((y+0)*rowStep)+(x+1)] = temp_vec[1];
                dest[((y+0)*rowStep)+(x+2)] = temp_vec[2];
                dest[((y+0)*rowStep)+(x+3)] = temp_vec[3];
            }

            for ( ; x < (cols-1); ++x) {
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)] + source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
    
};

/******************************************************************************/

// unroll inner loop
// reorder operations and make it look like an 8 item vector

template <typename T, typename TS>
struct jacobi2D_unrolled3 {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);

        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-8); x += 8) {
                TS sum_vec[8];
                T temp_vec[8];
                
                sum_vec[0] = source[((y-1)*rowStep)+(x+0)];
                sum_vec[1] = source[((y-1)*rowStep)+(x+1)];
                sum_vec[2] = source[((y-1)*rowStep)+(x+2)];
                sum_vec[3] = source[((y-1)*rowStep)+(x+3)];
                sum_vec[4] = source[((y-1)*rowStep)+(x+4)];
                sum_vec[5] = source[((y-1)*rowStep)+(x+5)];
                sum_vec[6] = source[((y-1)*rowStep)+(x+6)];
                sum_vec[7] = source[((y-1)*rowStep)+(x+7)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x-1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x-0)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[4] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[5] += source[((y+0)*rowStep)+(x+4)];
                sum_vec[6] += source[((y+0)*rowStep)+(x+5)];
                sum_vec[7] += source[((y+0)*rowStep)+(x+6)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+4)];
                sum_vec[4] += source[((y+0)*rowStep)+(x+5)];
                sum_vec[5] += source[((y+0)*rowStep)+(x+6)];
                sum_vec[6] += source[((y+0)*rowStep)+(x+7)];
                sum_vec[7] += source[((y+0)*rowStep)+(x+8)];
                
                sum_vec[0] += source[((y+1)*rowStep)+(x+0)];
                sum_vec[1] += source[((y+1)*rowStep)+(x+1)];
                sum_vec[2] += source[((y+1)*rowStep)+(x+2)];
                sum_vec[3] += source[((y+1)*rowStep)+(x+3)];
                sum_vec[4] += source[((y+1)*rowStep)+(x+4)];
                sum_vec[5] += source[((y+1)*rowStep)+(x+5)];
                sum_vec[6] += source[((y+1)*rowStep)+(x+6)];
                sum_vec[7] += source[((y+1)*rowStep)+(x+7)];
                
                temp_vec[0] = T( (sum_vec[0]+half) / TS(4) );
                temp_vec[1] = T( (sum_vec[1]+half) / TS(4) );
                temp_vec[2] = T( (sum_vec[2]+half) / TS(4) );
                temp_vec[3] = T( (sum_vec[3]+half) / TS(4) );
                temp_vec[4] = T( (sum_vec[4]+half) / TS(4) );
                temp_vec[5] = T( (sum_vec[5]+half) / TS(4) );
                temp_vec[6] = T( (sum_vec[6]+half) / TS(4) );
                temp_vec[7] = T( (sum_vec[7]+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp_vec[0];
                dest[((y+0)*rowStep)+(x+1)] = temp_vec[1];
                dest[((y+0)*rowStep)+(x+2)] = temp_vec[2];
                dest[((y+0)*rowStep)+(x+3)] = temp_vec[3];
                dest[((y+0)*rowStep)+(x+4)] = temp_vec[4];
                dest[((y+0)*rowStep)+(x+5)] = temp_vec[5];
                dest[((y+0)*rowStep)+(x+6)] = temp_vec[6];
                dest[((y+0)*rowStep)+(x+7)] = temp_vec[7];
            }

            for ( ; x < (cols-1); ++x) {
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)] + source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
        
    }
};

/******************************************************************************/

// TODO - abstract and centralize the half and orFactor values/calcs    constexpr templated on type.

template <typename T, typename TS>
struct jacobi_sor2D_simple {
    void operator()(T *source, T *dest, int rows, int cols, int rowStep, int /* iter */ ) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;

        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-1); ++x) {
            
                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T new_value = T( (sum+half) / TS(4) );
                T diff = new_value - old_value;
                T temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }

    }
};

/******************************************************************************/

// swap loops (bad cache order)

template <typename T, typename TS>
struct jacobi_sor2D_swapped {
    void operator()(T *source, T *dest, int rows, int cols, int rowStep, int /* iter */ ) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;

        for (x = 1; x < (cols-1); ++x) {
        
            for (y = 1; y < (rows-1); ++ y) {

                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T new_value = T( (sum+half) / TS(4) );
                T diff = new_value - old_value;
                T temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }

    }
};

/******************************************************************************/

// reverse loops (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi_sor2D_reversed {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;
        
        for (y = (rows-2); y >= 1; --y) {

            for (x = (cols-2); x >= 1; --x) {

                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T new_value = T( (sum+half) / TS(4) );
                T diff = new_value - old_value;
                T temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// reverse one loop (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi_sor2D_reversedX {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;
        

        for (y = 1; y < (rows-1); ++ y) {

            for (x = (cols-2); x >= 1; --x) {

                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T new_value = T( (sum+half) / TS(4) );
                T diff = new_value - old_value;
                T temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};

/******************************************************************************/

// reverse one loop (harder to vectorize, can be bad predictor order)

template <typename T, typename TS>
struct jacobi_sor2D_reversedY {
    void operator()(const T *source, T *dest, int rows, int cols, int rowStep, int /* iter */) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;
        
        for (y = (rows-2); y >= 1; --y) {

            for (x = 1; x < (cols-1); ++x) {

                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T new_value = T( (sum+half) / TS(4) );
                T diff = new_value - old_value;
                T temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }
    }
};


/******************************************************************************/

template <typename T, typename TS>
struct jacobi_sor2D_unrolled {
    void operator()(T *source, T *dest, int rows, int cols, int rowStep, int /* iter */ ) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;

        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-4); x += 4) {
            
                T old_value0 = dest[((y+0)*rowStep)+(x+0)];
                T old_value1 = dest[((y+0)*rowStep)+(x+1)];
                T old_value2 = dest[((y+0)*rowStep)+(x+2)];
                T old_value3 = dest[((y+0)*rowStep)+(x+3)];
                
                TS sum0 = source[((y-1)*rowStep)+(x+0)];
                sum0 +=   source[((y+0)*rowStep)+(x-1)];
                sum0 +=   source[((y+0)*rowStep)+(x+1)];
                sum0 +=   source[((y+1)*rowStep)+(x+0)];
                
                TS sum1 = source[((y-1)*rowStep)+(x+1)];
                sum1 +=   source[((y+0)*rowStep)+(x-0)];
                sum1 +=   source[((y+0)*rowStep)+(x+2)];
                sum1 +=   source[((y+1)*rowStep)+(x+1)];
                
                TS sum2 = source[((y-1)*rowStep)+(x+2)];
                sum2 +=   source[((y+0)*rowStep)+(x+1)];
                sum2 +=   source[((y+0)*rowStep)+(x+3)];
                sum2 +=   source[((y+1)*rowStep)+(x+2)];
                
                TS sum3 = source[((y-1)*rowStep)+(x+3)];
                sum3 +=   source[((y+0)*rowStep)+(x+2)];
                sum3 +=   source[((y+0)*rowStep)+(x+4)];
                sum3 +=   source[((y+1)*rowStep)+(x+3)];
                
                T temp0 = T( (sum0+half) / TS(4) );
                T temp1 = T( (sum1+half) / TS(4) );
                T temp2 = T( (sum2+half) / TS(4) );
                T temp3 = T( (sum3+half) / TS(4) );
                
                T diff0 = temp0 - old_value0;
                T diff1 = temp1 - old_value1;
                T diff2 = temp2 - old_value2;
                T diff3 = temp3 - old_value3;
                
                temp0 = old_value0 + scaleValue( diff0, orFactor, intShift );
                temp1 = old_value1 + scaleValue( diff1, orFactor, intShift );
                temp2 = old_value2 + scaleValue( diff2, orFactor, intShift );
                temp3 = old_value3 + scaleValue( diff3, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp0;
                dest[((y+0)*rowStep)+(x+1)] = temp1;
                dest[((y+0)*rowStep)+(x+2)] = temp2;
                dest[((y+0)*rowStep)+(x+3)] = temp3;
            }

            for ( ; x < (cols-1); ++x) {
            
                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                T diff = temp - old_value;
                temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }

    }
};

/******************************************************************************/

// unroll inner loop
// reorder operations and make it look like a 4 item vector

template <typename T, typename TS>
struct jacobi_sor2D_unrolled2 {
    void operator()(T *source, T *dest, int rows, int cols, int rowStep, int /* iter */ ) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;

        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-4); x += 4) {
                TS sum_vec[4];
                T temp_vec[4];
                T old_value[4];
                T diff[4];
                
                old_value[0] = dest[((y+0)*rowStep)+(x+0)];
                old_value[1] = dest[((y+0)*rowStep)+(x+1)];
                old_value[2] = dest[((y+0)*rowStep)+(x+2)];
                old_value[3] = dest[((y+0)*rowStep)+(x+3)];
                
                sum_vec[0] = source[((y-1)*rowStep)+(x+0)];
                sum_vec[1] = source[((y-1)*rowStep)+(x+1)];
                sum_vec[2] = source[((y-1)*rowStep)+(x+2)];
                sum_vec[3] = source[((y-1)*rowStep)+(x+3)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x-1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x-0)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+2)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+4)];
                
                sum_vec[0] += source[((y+1)*rowStep)+(x+0)];
                sum_vec[1] += source[((y+1)*rowStep)+(x+1)];
                sum_vec[2] += source[((y+1)*rowStep)+(x+2)];
                sum_vec[3] += source[((y+1)*rowStep)+(x+3)];
                
                temp_vec[0] = T( (sum_vec[0]+half) / TS(4) );
                temp_vec[1] = T( (sum_vec[1]+half) / TS(4) );
                temp_vec[2] = T( (sum_vec[2]+half) / TS(4) );
                temp_vec[3] = T( (sum_vec[3]+half) / TS(4) );
                
                diff[0] = temp_vec[0] - old_value[0];
                diff[1] = temp_vec[1] - old_value[1];
                diff[2] = temp_vec[2] - old_value[2];
                diff[3] = temp_vec[3] - old_value[3];
                
                temp_vec[0] = old_value[0] + scaleValue( diff[0], orFactor, intShift );
                temp_vec[1] = old_value[1] + scaleValue( diff[1], orFactor, intShift );
                temp_vec[2] = old_value[2] + scaleValue( diff[2], orFactor, intShift );
                temp_vec[3] = old_value[3] + scaleValue( diff[3], orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp_vec[0];
                dest[((y+0)*rowStep)+(x+1)] = temp_vec[1];
                dest[((y+0)*rowStep)+(x+2)] = temp_vec[2];
                dest[((y+0)*rowStep)+(x+3)] = temp_vec[3];
            }

            for ( ; x < (cols-1); ++x) {
            
                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                T diff = temp - old_value;
                temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }

    }
};

/******************************************************************************/

// unroll inner loop
// reorder operations and make it look like an 8 item vector

template <typename T, typename TS>
struct jacobi_sor2D_unrolled3 {
    void operator()(T *source, T *dest, int rows, int cols, int rowStep, int /* iter */ ) {
        const bool isFloat = (T(2.9) > T(2.0));
        const TS half = isFloat ? TS(0) : TS(2);
        const float orFactor = isFloat ? 1.9765 : 1.775;    // integer doesn't converge nearly as well
        const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway
        int x, y;

        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-8); x += 8) {
                TS sum_vec[8];
                T temp_vec[8];
                T old_value[8];
                T diff[8];
                
                old_value[0] = dest[((y+0)*rowStep)+(x+0)];
                old_value[1] = dest[((y+0)*rowStep)+(x+1)];
                old_value[2] = dest[((y+0)*rowStep)+(x+2)];
                old_value[3] = dest[((y+0)*rowStep)+(x+3)];
                old_value[4] = dest[((y+0)*rowStep)+(x+4)];
                old_value[5] = dest[((y+0)*rowStep)+(x+5)];
                old_value[6] = dest[((y+0)*rowStep)+(x+6)];
                old_value[7] = dest[((y+0)*rowStep)+(x+7)];
                
                sum_vec[0] = source[((y-1)*rowStep)+(x+0)];
                sum_vec[1] = source[((y-1)*rowStep)+(x+1)];
                sum_vec[2] = source[((y-1)*rowStep)+(x+2)];
                sum_vec[3] = source[((y-1)*rowStep)+(x+3)];
                sum_vec[4] = source[((y-1)*rowStep)+(x+4)];
                sum_vec[5] = source[((y-1)*rowStep)+(x+5)];
                sum_vec[6] = source[((y-1)*rowStep)+(x+6)];
                sum_vec[7] = source[((y-1)*rowStep)+(x+7)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x-1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x-0)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[4] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[5] += source[((y+0)*rowStep)+(x+4)];
                sum_vec[6] += source[((y+0)*rowStep)+(x+5)];
                sum_vec[7] += source[((y+0)*rowStep)+(x+6)];
                
                sum_vec[0] += source[((y+0)*rowStep)+(x+1)];
                sum_vec[1] += source[((y+0)*rowStep)+(x+2)];
                sum_vec[2] += source[((y+0)*rowStep)+(x+3)];
                sum_vec[3] += source[((y+0)*rowStep)+(x+4)];
                sum_vec[4] += source[((y+0)*rowStep)+(x+5)];
                sum_vec[5] += source[((y+0)*rowStep)+(x+6)];
                sum_vec[6] += source[((y+0)*rowStep)+(x+7)];
                sum_vec[7] += source[((y+0)*rowStep)+(x+8)];
                
                sum_vec[0] += source[((y+1)*rowStep)+(x+0)];
                sum_vec[1] += source[((y+1)*rowStep)+(x+1)];
                sum_vec[2] += source[((y+1)*rowStep)+(x+2)];
                sum_vec[3] += source[((y+1)*rowStep)+(x+3)];
                sum_vec[4] += source[((y+1)*rowStep)+(x+4)];
                sum_vec[5] += source[((y+1)*rowStep)+(x+5)];
                sum_vec[6] += source[((y+1)*rowStep)+(x+6)];
                sum_vec[7] += source[((y+1)*rowStep)+(x+7)];
                
                temp_vec[0] = T( (sum_vec[0]+half) / TS(4) );
                temp_vec[1] = T( (sum_vec[1]+half) / TS(4) );
                temp_vec[2] = T( (sum_vec[2]+half) / TS(4) );
                temp_vec[3] = T( (sum_vec[3]+half) / TS(4) );
                temp_vec[4] = T( (sum_vec[4]+half) / TS(4) );
                temp_vec[5] = T( (sum_vec[5]+half) / TS(4) );
                temp_vec[6] = T( (sum_vec[6]+half) / TS(4) );
                temp_vec[7] = T( (sum_vec[7]+half) / TS(4) );
                
                diff[0] = temp_vec[0] - old_value[0];
                diff[1] = temp_vec[1] - old_value[1];
                diff[2] = temp_vec[2] - old_value[2];
                diff[3] = temp_vec[3] - old_value[3];
                diff[4] = temp_vec[4] - old_value[4];
                diff[5] = temp_vec[5] - old_value[5];
                diff[6] = temp_vec[6] - old_value[6];
                diff[7] = temp_vec[7] - old_value[7];
                
                temp_vec[0] = old_value[0] + scaleValue( diff[0], orFactor, intShift );
                temp_vec[1] = old_value[1] + scaleValue( diff[1], orFactor, intShift );
                temp_vec[2] = old_value[2] + scaleValue( diff[2], orFactor, intShift );
                temp_vec[3] = old_value[3] + scaleValue( diff[3], orFactor, intShift );
                temp_vec[4] = old_value[4] + scaleValue( diff[4], orFactor, intShift );
                temp_vec[5] = old_value[5] + scaleValue( diff[5], orFactor, intShift );
                temp_vec[6] = old_value[6] + scaleValue( diff[6], orFactor, intShift );
                temp_vec[7] = old_value[7] + scaleValue( diff[7], orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp_vec[0];
                dest[((y+0)*rowStep)+(x+1)] = temp_vec[1];
                dest[((y+0)*rowStep)+(x+2)] = temp_vec[2];
                dest[((y+0)*rowStep)+(x+3)] = temp_vec[3];
                dest[((y+0)*rowStep)+(x+4)] = temp_vec[4];
                dest[((y+0)*rowStep)+(x+5)] = temp_vec[5];
                dest[((y+0)*rowStep)+(x+6)] = temp_vec[6];
                dest[((y+0)*rowStep)+(x+7)] = temp_vec[7];
            }

            for ( ; x < (cols-1); ++x) {
            
                T old_value = dest[((y+0)*rowStep)+(x+0)];
                TS sum = source[((y-1)*rowStep)+(x+0)];
                sum +=   source[((y+0)*rowStep)+(x-1)];
                sum +=   source[((y+0)*rowStep)+(x+1)];
                sum +=   source[((y+1)*rowStep)+(x+0)];
                
                T temp = T( (sum+half) / TS(4) );
                T diff = temp - old_value;
                temp = old_value + scaleValue( diff, orFactor, intShift );
                
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        }

    }
};

/******************************************************************************/
/******************************************************************************/

// fill interior values with weighted average of edges
template <typename T>
void average_edges(T *source, int rows, int cols, int rowStep ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const T half = isFloat ? T(0) : T(1);
    const int intShift = 6;     // shift = 7 barely fits in int16_t, leave some leeway

    int x, y;
    
    for (y = 1; y < (rows-1); ++ y) {
        float yweight = float(y) / float(rows-1);

        T left = source[ y*rowStep + 0 ];
        T right = source[ y*rowStep + (cols-1) ];
        
        for (x = 1; x < (cols-1); ++x) {
            float xweight = float(x) / float(cols-1);
            
            T top = source[ ((0)*rowStep)+x ];
            T bottom = source[ ((rows-1)*rowStep)+x ];

            T xval = left + scaleValue( (right - left), xweight, intShift );
            T yval = top + scaleValue( (bottom - top), yweight, intShift );
            T avg = (xval + yval + half) / T(2);
            source[(y*rowStep)+x] = avg;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

#define WRITE_PGM_FILES     0

template <typename T, typename TS, typename MM >
void convergenceLaplace2D(T *source, T *dest, int rows, int cols, int rowStep, MM calculator, const std::string &label, int minimum_iter = 1) {
    int i;
    TS total = 0;
    T max = 0;
    const bool isFloat = (T(2.9) > T(2.0));
    TS total_tolerance = 10;
    T max_tolerance = isFloat ? 0.01 : 1;
    
    T average = laplace_initial_conditions( source, rows, cols );
    //average_edges(source,rows,cols,rowStep);
    std::copy_n( source, rows*cols, dest );

#if WRITE_PGM_FILES
    std::string tempname1 = label + "00000start.pgm";
    write_PGM( source, rows, cols, T(250), tempname1 );
#endif

#if WRITE_PGM_FILES
    std::string tempname2 = label + "00000dest.pgm";
    write_PGM( dest, rows, cols, T(250), tempname2 );
#endif

    // What is a reasonable limit?
    // We're using smaller blocks, and Jacobi will take a while to converge in floating point.
    // But we don't want non-converging routines to take forever.
    auto base_iterations = iterations;
    iterations = 10000;
    
    start_timer();

    for( i = 0; i < iterations; ++i ) {
        
        calculator( source, dest, rows, cols, rowStep, i );

#if WRITE_PGM_FILES
        std::string tempname = label + to_string(i) + ".pgm";
        write_PGM( dest, rows, cols, T(250), tempname );
#endif

        total = total_difference<T,TS>(source,dest,rows,cols);
        max = max_difference(source,dest,rows,cols);
        
        if (i > minimum_iter) {
            if (total < total_tolerance || (total != total))
                break;
            
            if (max < max_tolerance || (max != max))
                break;
        }
        
        // exchange buffers
        std::swap( source, dest );
    }
    
    double total_time = timer();
    
    T center = dest[ (rows/2)*cols + (cols/2) ];
    T center_delta = average - center;

    if ((total != total) || (max != max))     // meaningless for int, but shows NaNs in float
        std::cout << label << " diverged to NaN";
    else if (i >= iterations && total > total_tolerance && max > max_tolerance)
        std::cout << label << " did not converge";
    else
        std::cout << label << " converged";
    
    std::cout << " in " << i << " iterations";
    std::cout << " ( total: " << total << ", max: " << max << ", center_delta: " << center_delta << ", time: " << total_time << ")\n";
    
    iterations = base_iterations;
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename TS, typename MM >
void testLaplace2D(T *source, T *dest, int rows, int cols, int rowStep, MM calculator, const std::string &label) {
    int i;
    
    laplace_initial_conditions( source, rows, cols);
    //average_edges(source,rows,cols,rowStep);
    std::copy_n( source, rows*cols, dest );

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        
        calculator( source, dest, rows, cols, rowStep, i );
        
        // exchange buffers
        std::swap( source, dest );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T, typename TS >
void TestOneType()
{
    const int base_iterations = iterations;

    std::string myTypeName( getTypeName<T>() );
    std::string myTypeNameA( getTypeName<TS>() );
    
    gLabels.clear();


    std::unique_ptr<T> data_flat_unique( new T[ HEIGHT*WIDTH ] );
    std::unique_ptr<T> data_flatDst_unique( new T[ HEIGHT*WIDTH ] );
    T *data_flat = data_flat_unique.get();
    T *data_flatDst = data_flatDst_unique.get();
    
    
    // make sure iteration count is even
    iterations = (iterations + 1) & ~1;


    // test basic performance
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_simple<T,TS>(), myTypeName + " jacobi 2D simple");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_swapped<T,TS>(), myTypeName + " jacobi 2D swapped");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_reversed<T,TS>(), myTypeName + " jacobi 2D reversed");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_reversedX<T,TS>(), myTypeName + " jacobi 2D reversedX");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_reversedY<T,TS>(), myTypeName + " jacobi 2D reversedY");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_unrolled<T,TS>(), myTypeName + " jacobi 2D unrolled");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_unrolled2<T,TS>(), myTypeName + " jacobi 2D unrolled2");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi2D_unrolled3<T,TS>(), myTypeName + " jacobi 2D unrolled3");
    
    std::string temp1( myTypeName + " PDE_laplace_2D jacobi" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    // test convergence
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_simple<T,TS>(), myTypeName + " jacobi 2D simple");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_swapped<T,TS>(), myTypeName + " jacobi 2D swapped");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_reversed<T,TS>(), myTypeName + " jacobi 2D reversed");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_reversedX<T,TS>(), myTypeName + " jacobi 2D reversedX");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_reversedY<T,TS>(), myTypeName + " jacobi 2D reversedY");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_unrolled<T,TS>(), myTypeName + " jacobi 2D unrolled");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_unrolled2<T,TS>(), myTypeName + " jacobi 2D unrolled2");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi2D_unrolled3<T,TS>(), myTypeName + " jacobi 2D unrolled3");



    // test basic performance
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_simple<T,TS>(), myTypeName + " jacobi SOR 2D simple");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_swapped<T,TS>(), myTypeName + " jacobi SOR 2D swapped");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_reversed<T,TS>(), myTypeName + " jacobi SOR 2D reversed");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_reversedX<T,TS>(), myTypeName + " jacobi SOR 2D reversedX");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_reversedY<T,TS>(), myTypeName + " jacobi SOR 2D reversedY");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_unrolled<T,TS>(), myTypeName + " jacobi SOR 2D unrolled");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_unrolled2<T,TS>(), myTypeName + " jacobi SOR 2D unrolled2");
    testLaplace2D<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, jacobi_sor2D_unrolled3<T,TS>(), myTypeName + " jacobi SOR 2D unrolled3");
    
    std::string temp2( myTypeName + " PDE_laplace_2D jacobi_SOR" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    // test convergence
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_simple<T,TS>(), myTypeName + " jacobi SOR 2D simple");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_swapped<T,TS>(), myTypeName + " jacobi SOR 2D swapped");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_reversed<T,TS>(), myTypeName + " jacobi SOR 2D reversed");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_reversedX<T,TS>(), myTypeName + " jacobi SOR 2D reversedX");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_reversedY<T,TS>(), myTypeName + " jacobi SOR 2D reversedY");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_unrolled<T,TS>(), myTypeName + " jacobi SOR 2D unrolled");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_unrolled2<T,TS>(), myTypeName + " jacobi SOR 2D unrolled2");
    convergenceLaplace2D<T,TS>( data_flat, data_flatDst, SMALL_HEIGHT, SMALL_WIDTH, SMALL_WIDTH, jacobi_sor2D_unrolled3<T,TS>(), myTypeName + " jacobi SOR 2D unrolled3");



    
    iterations = base_iterations;

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);


    TestOneType<int16_t, int32_t>();

    TestOneType<int32_t, int64_t>();
    
    TestOneType<int64_t, int64_t>();

    TestOneType<float,float>();

    TestOneType<double,double>();
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
