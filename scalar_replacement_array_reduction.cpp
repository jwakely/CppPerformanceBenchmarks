/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to scalar replacement of array references
        as applied to reductions of arrays and matrices.

Assumptions:

    1) The compiler will convert array references to scalar calculations when necessary
        input[0] += 2;        ==>        input[0] += 14;
        input[0] += 5;
        input[0] += 7;
    
    2) The compiler will apply further optimization to the resulting values.
        Loop combining, loop unrolling, loop reordering, etc.


*/

/******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_stdint.hpp"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 200000;

const int WIDTH = 200;
const int HEIGHT = 300;

// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 1;

/******************************************************************************/

// our global arrays of numbers to be operated upon

int32_t data32_A[WIDTH];
int32_t data32_B[HEIGHT];
int32_t data32_Matrix[HEIGHT][WIDTH];

uint64_t data64_A[WIDTH];
uint64_t data64_B[HEIGHT];
uint64_t data64_Matrix[HEIGHT][WIDTH];

double dataDouble_A[WIDTH];
double dataDouble_B[HEIGHT];
double dataDouble_Matrix[HEIGHT][WIDTH];

/******************************************************************************/

template <class Iterator, class T>
void fill(Iterator first, Iterator last, T value) {
    while (first != last) *first++ = value;
}

/******************************************************************************/

template<class T>
inline void check_sums_reduction( T *input ) {

    int x;
    for (x = 0; x < WIDTH; ++x) {
        T result = init_value * HEIGHT + init_value;
        if (input[x] != result ) {
            printf("test %i failed\n", current_test);
            break;
        }
    }
}

/******************************************************************************/

template<class T>
inline void check_sums_dmxpy( T *input ) {

    int x;
    for (x = 0; x < WIDTH; ++x) {
        T result = init_value + (init_value * init_value) * HEIGHT;
        if (input[x] != result ) {
            printf("test %i failed\n", current_test);
            break;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

// an unoptimized reduction, as it is likely to appear in the real-world
template <typename T>
void test_array_reduction( T *inputA, const T *inputB, const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < WIDTH; ++x) {
            for (y = 0; y < HEIGHT; ++y) {
                inputA[x] = inputA[x] + inputB[y];
            }
        }
        
        check_sums_reduction(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// simple scalar replacement of the unchanging inner array reference
// could also happen via loop invariant movement
template <typename T>
void test_array_reduction_opt1( T *inputA, const T *inputB, const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < WIDTH; ++x) {
            T valueAX = inputA[x];
            for (y = 0; y < HEIGHT; ++y) {
                valueAX += inputB[y];
            }
            inputA[x] = valueAX;
        }
        
        check_sums_reduction(inputA);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// Further optimized with loop unrolling and splitting the sum
// this is a likely pattern for real-world code
// not always applicable to floating point data
template <typename T>
void test_array_reduction_opt2( T *inputA, const T *inputB, const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < WIDTH; ++x) {
            T valueAX0 = inputA[x];
            T valueAX1 = T(0);
            T valueAX2 = T(0);
            T valueAX3 = T(0);
            
            for (y = 0; y < (HEIGHT - 3); y += 4) {
                valueAX0 += inputB[y+0];
                valueAX1 += inputB[y+1];
                valueAX2 += inputB[y+2];
                valueAX3 += inputB[y+3];
            }
            
            for (; y < HEIGHT; ++y) {
                valueAX0 += inputB[y];
            }
            
            inputA[x] = valueAX0 + valueAX1 + valueAX2 + valueAX3;
        }
        
        check_sums_reduction(inputA);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// Further optimized with loop unrolling of a different loop
// this is a likely pattern for real-world code
// this is sometimes referred to as "unroll and jam"
template <typename T>
void test_array_reduction_opt3( T *inputA, const T *inputB, const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (x = 0; x < (WIDTH - 3); x += 4) {
            T valueAX0 = T(init_value);
            T valueAX1 = T(init_value);
            T valueAX2 = T(init_value);
            T valueAX3 = T(init_value);
            
            for (y = 0; y < HEIGHT; ++y) {
                T valueB = inputB[y];
                valueAX0 += valueB;
                valueAX1 += valueB;
                valueAX2 += valueB;
                valueAX3 += valueB;
            }
            inputA[x+0] = valueAX0;
            inputA[x+1] = valueAX1;
            inputA[x+2] = valueAX2;
            inputA[x+3] = valueAX3;
        }
        
        for (; x < WIDTH; ++x) {
            T valueAX = T(init_value);
            for (y = 0; y < HEIGHT; ++y) {
                valueAX += inputB[y];
            }
            inputA[x] = valueAX;
        }
        
        check_sums_reduction(inputA);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// A smart compiler will notice that the result is the same for each x
// This is only optimizable this far because of the contrived nature of the benchmark
//    having the fill and calculation loops in the same place
template <typename T>
void test_array_reduction_opt4( T *inputA, const T *inputB, const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        // compute first result
            T valueAX0 = T(init_value);
            T valueAX1 = T(0);
            T valueAX2 = T(0);
            T valueAX3 = T(0);
            
            for (y = 0; y < (HEIGHT - 3); y += 4) {
                valueAX0 += inputB[y+0];
                valueAX1 += inputB[y+1];
                valueAX2 += inputB[y+2];
                valueAX3 += inputB[y+3];
            }
            
            for (; y < HEIGHT; ++y) {
                valueAX0 += inputB[y];
            }
            
            valueAX0 = valueAX0 + valueAX1 + valueAX2 + valueAX3;


        // copy result to all slots
        for (x = 0; x < WIDTH; ++x) {
            inputA[x] = valueAX0;
        }
        
        check_sums_reduction(inputA);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

// An unoptimized reduction, as it is likely to appear in the real-world.
// Pretty close to what appear in linpack.
template <typename T>
void test_array_dmxpy( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (y = 0; y < HEIGHT; ++y) {
            for (x = 0; x < WIDTH; ++x) {
                inputA[x] = inputA[x] + inputB[y] * matrix[y][x];
            }
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// straightforward scalar replacement (moving invariant reference out of loop)
// could also happen via loop invariant movement
template <typename T>
void test_array_dmxpy_opt1( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (y = 0; y < HEIGHT; ++y) {
            T inputBY = inputB[y];
            for (x = 0; x < WIDTH; ++x) {
                inputA[x] += inputBY * matrix[y][x];
            }
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// straightforward scalar replacement, and unroll in x
template <typename T>
void test_array_dmxpy_opt2( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (y = 0; y < HEIGHT; ++y) {
            T inputBY = inputB[y];
            
            for (x = 0; x < (WIDTH - 3); x += 4) {
                T inputAX0 = inputA[x+0];
                T inputAX1 = inputA[x+1];
                T inputAX2 = inputA[x+2];
                T inputAX3 = inputA[x+3];
                
                inputAX0 += inputBY * matrix[y][x+0];
                inputAX1 += inputBY * matrix[y][x+1];
                inputAX2 += inputBY * matrix[y][x+2];
                inputAX3 += inputBY * matrix[y][x+3];
                
                inputA[x+0] = inputAX0;
                inputA[x+1] = inputAX1;
                inputA[x+2] = inputAX2;
                inputA[x+3] = inputAX3;
            }
            for (; x < WIDTH; ++x) {
                inputA[x] += inputBY * matrix[y][x];
            }
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// straightforward scalar replacement, and unroll in y
template <typename T>
void test_array_dmxpy_opt3( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (y = 0; y < (HEIGHT -3); y += 4) {
            T inputBY0 = inputB[y+0];
            T inputBY1 = inputB[y+1];
            T inputBY2 = inputB[y+2];
            T inputBY3 = inputB[y+3];
            for (x = 0; x < WIDTH; ++x) {
                inputA[x] += inputBY0 * matrix[y+0][x]
                            + inputBY1 * matrix[y+1][x]
                            + inputBY2 * matrix[y+2][x]
                            + inputBY3 * matrix[y+3][x];
            }
        }
        
        for (; y < HEIGHT; ++y) {
            T inputBY = inputB[y];
            for (x = 0; x < WIDTH; ++x) {
                inputA[x] += inputBY * matrix[y][x];
            }
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// straightforward scalar replacement, and unroll in y and x
template <typename T>
void test_array_dmxpy_opt4( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (y = 0; y < (HEIGHT -3); y += 4) {
            T inputBY0 = inputB[y+0];
            T inputBY1 = inputB[y+1];
            T inputBY2 = inputB[y+2];
            T inputBY3 = inputB[y+3];
            
            for (x = 0; x < (WIDTH - 3); x += 4) {
                T inputAX0 = inputA[x+0];
                T inputAX1 = inputA[x+1];
                T inputAX2 = inputA[x+2];
                T inputAX3 = inputA[x+3];

                inputAX0 += inputBY0 * matrix[y+0][x+0]
                            + inputBY1 * matrix[y+1][x+0]
                            + inputBY2 * matrix[y+2][x+0]
                            + inputBY3 * matrix[y+3][x+0];
                
                inputAX1 += inputBY0 * matrix[y+0][x+1]
                            + inputBY1 * matrix[y+1][x+1]
                            + inputBY2 * matrix[y+2][x+1]
                            + inputBY3 * matrix[y+3][x+1];
                
                inputAX2 += inputBY0 * matrix[y+0][x+2]
                            + inputBY1 * matrix[y+1][x+2]
                            + inputBY2 * matrix[y+2][x+2]
                            + inputBY3 * matrix[y+3][x+2];
                
                inputAX3 += inputBY0 * matrix[y+0][x+3]
                            + inputBY1 * matrix[y+1][x+3]
                            + inputBY2 * matrix[y+2][x+3]
                            + inputBY3 * matrix[y+3][x+3];
                
                inputA[x+0] = inputAX0;
                inputA[x+1] = inputAX1;
                inputA[x+2] = inputAX2;
                inputA[x+3] = inputAX3;
            }
            
            for (; x < WIDTH; ++x) {
                inputA[x] += inputBY0 * matrix[y+0][x]
                            + inputBY1 * matrix[y+1][x]
                            + inputBY2 * matrix[y+2][x]
                            + inputBY3 * matrix[y+3][x];
            }
        }
        
        
        for (; y < HEIGHT; ++y) {
            T inputBY = inputB[y];
            
            for (x = 0; x < (WIDTH - 3); x += 4) {
                T inputAX0 = inputA[x+0];
                T inputAX1 = inputA[x+1];
                T inputAX2 = inputA[x+2];
                T inputAX3 = inputA[x+3];
                
                inputAX0 += inputBY * matrix[y][x+0];
                inputAX1 += inputBY * matrix[y][x+1];
                inputAX2 += inputBY * matrix[y][x+2];
                inputAX3 += inputBY * matrix[y][x+3];
                
                inputA[x+0] = inputAX0;
                inputA[x+1] = inputAX1;
                inputA[x+2] = inputAX2;
                inputA[x+3] = inputAX3;
            }
            for (; x < WIDTH; ++x) {
                inputA[x] += inputBY * matrix[y][x];
            }
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// loop inversion, followed by straightforward scalar replacement
template <typename T>
void test_array_dmxpy_opt5( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < WIDTH; ++x) {
            T inputAX = inputA[x];
            for (y = 0; y < HEIGHT; ++y) {
                inputAX += inputB[y] * matrix[y][x];
            }
            inputA[x] = inputAX;
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// loop inversion, followed by straightforward scalar replacement
// unroll in y
template <typename T>
void test_array_dmxpy_opt6( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < WIDTH; ++x) {
            T inputAX = inputA[x];
            for (y = 0; y < (HEIGHT - 3); y += 4) {
                inputAX += inputB[y+0] * matrix[y+0][x]
                            + inputB[y+1] * matrix[y+1][x]
                            + inputB[y+2] * matrix[y+2][x]
                            + inputB[y+3] * matrix[y+3][x];
            }
            for (; y < HEIGHT; ++y) {
                inputAX += inputB[y] * matrix[y][x];
            }
            inputA[x] = inputAX;
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// loop inversion, followed by straightforward scalar replacement
// unroll in x
template <typename T>
void test_array_dmxpy_opt7( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < (WIDTH - 3); x += 4) {
            T inputAX0 = inputA[x+0];
            T inputAX1 = inputA[x+1];
            T inputAX2 = inputA[x+2];
            T inputAX3 = inputA[x+3];
            
            for (y = 0; y < HEIGHT; ++y) {
                T inputBY = inputB[y];
                inputAX0 += inputBY * matrix[y][x+0];
                inputAX1 += inputBY * matrix[y][x+1];
                inputAX2 += inputBY * matrix[y][x+2];
                inputAX3 += inputBY * matrix[y][x+3];
            }
            
            inputA[x+0] = inputAX0;
            inputA[x+1] = inputAX1;
            inputA[x+2] = inputAX2;
            inputA[x+3] = inputAX3;
        }
        
        for (; x < WIDTH; ++x) {
            T inputAX = inputA[x];
            for (y = 0; y < HEIGHT; ++y) {
                inputAX += inputB[y] * matrix[y][x];
            }
            inputA[x] = inputAX;
        }
        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

// loop inversion, followed by straightforward scalar replacement
// unroll in y and x
template <typename T>
void test_array_dmxpy_opt8( T *inputA, const T *inputB, const T matrix[HEIGHT][WIDTH], const char *label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        ::fill(inputA, inputA+WIDTH, T(init_value));
        
        for (x = 0; x < (WIDTH - 3); x += 4) {
            T inputAX0 = inputA[x+0];
            T inputAX1 = inputA[x+1];
            T inputAX2 = inputA[x+2];
            T inputAX3 = inputA[x+3];
            
            for (y = 0; y < (HEIGHT - 3); y += 4) {
                    
                T inputBY0 = inputB[y+0];
                T inputBY1 = inputB[y+1];
                T inputBY2 = inputB[y+2];
                T inputBY3 = inputB[y+3];
            
                inputAX0 += inputBY0 * matrix[y+0][x+0];
                inputAX1 += inputBY0 * matrix[y+0][x+1];
                inputAX2 += inputBY0 * matrix[y+0][x+2];
                inputAX3 += inputBY0 * matrix[y+0][x+3];
            
                inputAX0 += inputBY1 * matrix[y+1][x+0];
                inputAX1 += inputBY1 * matrix[y+1][x+1];
                inputAX2 += inputBY1 * matrix[y+1][x+2];
                inputAX3 += inputBY1 * matrix[y+1][x+3];
            
                inputAX0 += inputBY2 * matrix[y+2][x+0];
                inputAX1 += inputBY2 * matrix[y+2][x+1];
                inputAX2 += inputBY2 * matrix[y+2][x+2];
                inputAX3 += inputBY2 * matrix[y+2][x+3];
            
                inputAX0 += inputBY3 * matrix[y+3][x+0];
                inputAX1 += inputBY3 * matrix[y+3][x+1];
                inputAX2 += inputBY3 * matrix[y+3][x+2];
                inputAX3 += inputBY3 * matrix[y+3][x+3];
            }

            for (; y < HEIGHT; ++y) {
                T inputBY = inputB[y];
                inputAX0 += inputBY * matrix[y][x+0];
                inputAX1 += inputBY * matrix[y][x+1];
                inputAX2 += inputBY * matrix[y][x+2];
                inputAX3 += inputBY * matrix[y][x+3];
            }
            
            inputA[x+0] = inputAX0;
            inputA[x+1] = inputAX1;
            inputA[x+2] = inputAX2;
            inputA[x+3] = inputAX3;
        }
        
        for (; x < WIDTH; ++x) {
            T inputAX = inputA[x];
            for (y = 0; y < (HEIGHT - 3); y += 4) {
                inputAX += inputB[y+0] * matrix[y+0][x]
                            + inputB[y+1] * matrix[y+1][x]
                            + inputB[y+2] * matrix[y+2][x]
                            + inputB[y+3] * matrix[y+3][x];
            }
            for (; y < HEIGHT; ++y) {
                inputAX += inputB[y] * matrix[y][x];
            }
            inputA[x] = inputAX;
        }

        
        check_sums_dmxpy(inputA);
    }
    
    record_result( timer(), label );
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
    if (argc > 2) init_value = atoi(argv[2]);


    ::fill(data32_B, data32_B+HEIGHT, int32_t(init_value));
    test_array_reduction_opt1( data32_A, data32_B, "int32_t scalar replacment of arrays reduction opt1");
    test_array_reduction_opt2( data32_A, data32_B, "int32_t scalar replacment of arrays reduction opt2");
    test_array_reduction_opt3( data32_A, data32_B, "int32_t scalar replacment of arrays reduction opt3");
    test_array_reduction_opt4( data32_A, data32_B, "int32_t scalar replacment of arrays reduction opt4");
    test_array_reduction( data32_A, data32_B, "int32_t scalar replacment of arrays reduction");
    
    summarize("int32_t scalar replacement of arrays reduction", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );


    ::fill(data64_B, data64_B+HEIGHT, uint64_t(init_value));
    test_array_reduction_opt1( data64_A, data64_B, "uint64_t scalar replacment of arrays reduction opt1");
    test_array_reduction_opt2( data64_A, data64_B, "uint64_t scalar replacment of arrays reduction opt2");
    test_array_reduction_opt3( data64_A, data64_B, "uint64_t scalar replacment of arrays reduction opt3");
    test_array_reduction_opt4( data64_A, data64_B, "uint64_t scalar replacment of arrays reduction opt4");
    test_array_reduction( data64_A, data64_B, "uint64_t scalar replacment of arrays reduction");
    
    summarize("uint64_t scalar replacement of arrays reduction", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    ::fill(dataDouble_B, dataDouble_B+HEIGHT, double(init_value));
    test_array_reduction_opt1( dataDouble_A, dataDouble_B, "double scalar replacment of arrays reduction opt1");
    test_array_reduction_opt2( dataDouble_A, dataDouble_B, "double scalar replacment of arrays reduction opt2");
    test_array_reduction_opt3( dataDouble_A, dataDouble_B, "double scalar replacment of arrays reduction opt3");
    test_array_reduction_opt4( dataDouble_A, dataDouble_B, "double scalar replacment of arrays reduction opt4");
    test_array_reduction( dataDouble_A, dataDouble_B, "double scalar replacment of arrays reduction");
    
    summarize("double scalar replacement of arrays reduction", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );



    ::fill(data32_B, data32_B+HEIGHT, int32_t(init_value));
    ::fill( &data32_Matrix[0][0], &data32_Matrix[HEIGHT-1][WIDTH-1]+1, int32_t(init_value));
    test_array_dmxpy_opt1( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt1");
    test_array_dmxpy_opt2( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt2");
    test_array_dmxpy_opt3( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt3");
    test_array_dmxpy_opt4( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt4");
    test_array_dmxpy_opt5( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt5");
    test_array_dmxpy_opt6( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt6");
    test_array_dmxpy_opt7( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt7");
    test_array_dmxpy_opt8( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy opt8");
    test_array_dmxpy( data32_A, data32_B, data32_Matrix, "int32_t scalar replacment of arrays dmxpy");

    summarize("int32_t scalar replacement of arrays dmxpy", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );


    ::fill(data64_B, data64_B+HEIGHT, uint64_t(init_value));
    ::fill( &data64_Matrix[0][0], &data64_Matrix[HEIGHT-1][WIDTH-1]+1, uint64_t(init_value));
    test_array_dmxpy_opt1( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt1");
    test_array_dmxpy_opt2( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt2");
    test_array_dmxpy_opt3( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt3");
    test_array_dmxpy_opt4( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt4");
    test_array_dmxpy_opt5( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt5");
    test_array_dmxpy_opt6( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt6");
    test_array_dmxpy_opt7( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt7");
    test_array_dmxpy_opt8( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy opt8");
    test_array_dmxpy( data64_A, data64_B, data64_Matrix, "uint64_t scalar replacment of arrays dmxpy");

    summarize("uint64_t scalar replacement of arrays dmxpy", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );


    ::fill(dataDouble_B, dataDouble_B+HEIGHT, double(init_value));
    ::fill( &dataDouble_Matrix[0][0], &dataDouble_Matrix[HEIGHT-1][WIDTH-1]+1, double(init_value));
    test_array_dmxpy_opt1( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt1");
    test_array_dmxpy_opt2( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt2");
    test_array_dmxpy_opt3( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt3");
    test_array_dmxpy_opt4( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt4");
    test_array_dmxpy_opt5( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt5");
    test_array_dmxpy_opt6( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt6");
    test_array_dmxpy_opt7( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt7");
    test_array_dmxpy_opt8( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy opt8");
    test_array_dmxpy( dataDouble_A, dataDouble_B, dataDouble_Matrix, "double scalar replacment of arrays dmxpy");

    summarize("double scalar replacement of arrays dmxpy", WIDTH*HEIGHT, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
