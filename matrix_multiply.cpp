/*
    Copyright 2008 Adobe Systems Incorporated
    Coppyright 2019-2021 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to matrix multiplication.


Assumptions:

    1) The compiler will recognize matrix multiplication patterns
        and substitute optimal patterns.

    2) The compiler will apply loop optimizations that improve
        performance of naively written matrix multiply like operations.

    3) The compiler will at least apply the textbook optimizations
        for matrix multiplication.



NOTE - the techniques used to optimize a matrix multiply, if done generally, help many other problems.
        Image processing, volume processing, convolutions, etc.

NOTE - performance will suffer on CPUs without a multiply-add operation




SEE ALSO:
    DGEMM in BLAS and LAPACK
    TIFAMMY
        http://sourceforge.net/projects/tifammy/
    Tiling/blocking for GPU calculation
        https://devblogs.nvidia.com/cutlass-linear-algebra-cuda/

TODO - sub cubic algorithms
    https://en.wikipedia.org/wiki/Matrix_multiplication_algorithm
    https://en.wikipedia.org/wiki/Strassen_algorithm

TODO - threaded matrix multiplies ?

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <string>
#include <memory>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
size_t iterations = 450;


// 60k items, or about 480k of data per matrix - intended to exceed the L1 cache
const size_t WIDTH = 200;
const size_t HEIGHT = 300;

const size_t SIZE = HEIGHT*WIDTH;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
  T temp = (T)(HEIGHT*WIDTH*WIDTH) * (T)init_value * (T)init_value;
  if (!tolerance_equal<T>(result,temp))
    printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct matmul_KIJ {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( k = 0; k < cols; ++k ) {
            for ( i = 0; i < cols; ++i ) {
                for ( j = 0 ; j < rows ; ++j ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matmul_KJI {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( k = 0; k < cols; ++k ) {
            for ( j = 0 ; j < rows ; ++j ) {
                for ( i = 0; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matmul_JKI {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( j = 0 ; j < rows ; ++j ) {
            for ( k = 0; k < cols; ++k ) {
                for ( i = 0; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matmul_JIK {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( j = 0 ; j < rows ; ++j ) {
            for ( i = 0; i < cols; ++i ) {
                for ( k = 0; k < cols; ++k ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matmul_IJK {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( i = 0; i < cols; ++i ) {
            for ( j = 0 ; j < rows ; ++j ) {
                for ( k = 0; k < cols; ++k ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matmul_IKJ {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( i = 0; i < cols; ++i ) {
            for ( k = 0; k < cols; ++k ) {
                for ( j = 0 ; j < rows ; ++j ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

// one value is constant in inner loop, pull it out
template <typename T >
struct matmul_KIJ_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( k = 0; k < cols; ++k ) {
            for ( i = 0; i < cols; ++i ) {
                T temp = yy[k*cols+i] ;
                for ( j = 0 ; j < rows ; ++j ) {
                    zz[j*cols+i] += temp * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

// one value is constant in inner loop, pull it out
template <typename T >
struct matmul_KJI_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( k = 0; k < cols; ++k ) {
            for ( j = 0 ; j < rows ; ++j ) {
                T temp = xx[j*cols+k];
                for ( i = 0; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * temp;
                }
            }
        }
    }
};

/******************************************************************************/

// one value is constant in inner loop, pull it out
template <typename T >
struct matmul_JKI_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( j = 0 ; j < rows ; ++j ) {
            for ( k = 0; k < cols; ++k ) {
                T temp = xx[j*cols+k];
                for ( i = 0; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * temp;
                }
            }
        }
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
template <typename T >
struct matmul_JIK_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( j = 0 ; j < rows ; ++j ) {
            for ( i = 0; i < cols; ++i ) {
                T temp = 0;
                for ( k = 0; k < cols; ++k ) {
                    temp += yy[k*cols+i] * xx[j*cols+k];
                }
                zz[j*cols+i] += temp;
            }
        }
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
template <typename T >
struct matmul_IJK_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( i = 0; i < cols; ++i ) {
            for ( j = 0 ; j < rows ; ++j ) {
                T temp = 0;
                for ( k = 0; k < cols; ++k ) {
                    temp += yy[k*cols+i] * xx[j*cols+k];
                }
                zz[j*cols+i] += temp;
            }
        }
    }
};

/******************************************************************************/

// one value is constant in inner loop, pull it out
template <typename T >
struct matmul_IKJ_temp {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        for ( i = 0; i < cols; ++i ) {
            for ( k = 0; k < cols; ++k ) {
                T temp = yy[k*cols+i];
                for ( j = 0 ; j < rows ; ++j ) {
                    zz[j*cols+i] += temp * xx[j*cols+k];
                }
            }
        }
    }
};

/******************************************************************************/

// unroll the inner loops
// pull constant terms out of loops

template <typename T >
struct matmul_KJI_unrolled {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( j = 0 ; j < rows ; ++j ) {
                
                T xx0 = xx[j*cols+k+0];
                T xx1 = xx[j*cols+k+1];
                T xx2 = xx[j*cols+k+2];
                T xx3 = xx[j*cols+k+3];
                
                for ( i = 0; i < (cols-3); i += 4 ) {
                    zz[j*cols+i+0] += yy[k*cols+i+0] * xx0 + yy[(k+1)*cols+i+0] * xx1 + yy[(k+2)*cols+i+0] * xx2 + yy[(k+3)*cols+i+0] * xx3;
                    zz[j*cols+i+1] += yy[k*cols+i+1] * xx0 + yy[(k+1)*cols+i+1] * xx1 + yy[(k+2)*cols+i+1] * xx2 + yy[(k+3)*cols+i+1] * xx3;
                    zz[j*cols+i+2] += yy[k*cols+i+2] * xx0 + yy[(k+1)*cols+i+2] * xx1 + yy[(k+2)*cols+i+2] * xx2 + yy[(k+3)*cols+i+2] * xx3;
                    zz[j*cols+i+3] += yy[k*cols+i+3] * xx0 + yy[(k+1)*cols+i+3] * xx1 + yy[(k+2)*cols+i+3] * xx2 + yy[(k+3)*cols+i+3] * xx3;
                }
                
                for ( ; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i+0] * xx0 + yy[(k+1)*cols+i+0] * xx1 + yy[(k+2)*cols+i+0] * xx2 + yy[(k+3)*cols+i+0] * xx3;
                }
            }
        }
        
        for ( ; k < cols; ++k ) {
            for ( j = 0 ; j < rows ; ++j ) {
            
                T xx0 = xx[j*cols+k];
                
                for ( i = 0; i < (cols-3); i += 4 ) {
                    zz[j*cols+i+0] += yy[k*cols+i+0] * xx0;
                    zz[j*cols+i+1] += yy[k*cols+i+1] * xx0;
                    zz[j*cols+i+2] += yy[k*cols+i+2] * xx0;
                    zz[j*cols+i+3] += yy[k*cols+i+3] * xx0;
                }
                
                for ( ; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx0;
                }
            }
        }

    }
};

/******************************************************************************/

// unroll the inner loops
// pull constant terms out of loops

template <typename T >
struct matmul_JKI_unrolled {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k;
        
        for ( j = 0 ; j < rows ; ++j ) {
        
            for ( k = 0; k < (cols-3); k += 4 ) {
                
                T xx0 = xx[j*cols+k+0];
                T xx1 = xx[j*cols+k+1];
                T xx2 = xx[j*cols+k+2];
                T xx3 = xx[j*cols+k+3];
                
                for ( i = 0; i < (cols-3); i += 4 ) {
                    zz[j*cols+i+0] += yy[k*cols+i+0] * xx0 + yy[(k+1)*cols+i+0] * xx1 + yy[(k+2)*cols+i+0] * xx2 + yy[(k+3)*cols+i+0] * xx3;
                    zz[j*cols+i+1] += yy[k*cols+i+1] * xx0 + yy[(k+1)*cols+i+1] * xx1 + yy[(k+2)*cols+i+1] * xx2 + yy[(k+3)*cols+i+1] * xx3;
                    zz[j*cols+i+2] += yy[k*cols+i+2] * xx0 + yy[(k+1)*cols+i+2] * xx1 + yy[(k+2)*cols+i+2] * xx2 + yy[(k+3)*cols+i+2] * xx3;
                    zz[j*cols+i+3] += yy[k*cols+i+3] * xx0 + yy[(k+1)*cols+i+3] * xx1 + yy[(k+2)*cols+i+3] * xx2 + yy[(k+3)*cols+i+3] * xx3;
                }
                
                for ( ; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx0 + yy[(k+1)*cols+i] * xx1 + yy[(k+2)*cols+i] * xx2 + yy[(k+3)*cols+i] * xx3;
                }
            }
            
            for ( ; k < cols; ++k ) {
            
                T xx0 = xx[j*cols+k];
                
                for ( i = 0; i < (cols-3); i += 4 ) {
                    zz[j*cols+i+0] += yy[k*cols+i+0] * xx0;
                    zz[j*cols+i+1] += yy[k*cols+i+1] * xx0;
                    zz[j*cols+i+2] += yy[k*cols+i+2] * xx0;
                    zz[j*cols+i+3] += yy[k*cols+i+3] * xx0;
                }
                
                for ( ; i < cols; ++i ) {
                    zz[j*cols+i] += yy[k*cols+i] * xx0;
                }
            }

        }
    }
};

/******************************************************************************/

/*
NOTE - we ideally would measure optimal block sizes for each routine

Optimum performance curve appears to be close to parabolic wrt. block size.
Our matrices are only 200x300, so larger blocks won't help much.
The best block size depends on data size in cache for each processor and type.
Note that cache issues (and thread mobility) will cause random slowdowns for cache filling algorithms.

blockSize ~= (int)sqrt(BLOCK_MEM/sizeof(T));
*/

const size_t blockSize = 128;          // small, to fit in cache

const size_t blockSizeINF = 900;       // huge, might be infinite...


// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// A textbook example.

template <typename T >
struct matmul_JIK_blocked {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( ii = 0; ii < cols; ii += blockSize ) {
            size_t iend = ii + blockSize;
            if (iend > cols)    iend = cols;
        
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( j = 0 ; j < rows ; ++ j ) {
    
                    for ( i = ii; i < iend; ++i ) {
                        T temp = 0;
                        for ( k = kk; k < kend; ++k ) {
                            temp += yy[k*cols+i] * xx[j*cols+k];
                        }
                        zz[j*cols+i] += temp;
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// one input is constant over the inner loop, pull it out
// iterate over subblocks to improve cache usage
// Another textbook example.

template <typename T >
struct matmul_JKI_blocked {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( kk = 0; kk < cols; kk += blockSize ) {
            size_t kend = kk + blockSize;
            if (kend > cols)    kend = cols;
        
            for ( ii = 0; ii < cols; ii += blockSizeINF ) {
                size_t iend = ii + blockSizeINF;
                if (iend > cols)    iend = cols;
                
                for ( j = 0 ; j < rows ; ++ j ) {
    
                    for ( k = kk; k < kend; ++k ) {
                        T temp = xx[j*cols+k];
                        for ( i = ii; i < iend; ++i ) {
                            zz[j*cols+i] += yy[k*cols+i] * temp;
                        }
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// Another textbook example.

template <typename T >
struct matmul_IJK_blocked {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, jj, kk;

        for ( jj = 0; jj < rows; jj += blockSize ) {
            size_t jend = jj + blockSize;
            if (jend > rows)    jend = rows;
            
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( i = 0; i < cols; ++i ) {
                
                    for ( j = jj; j < jend ; ++j ) {
    
                        T temp = 0;
                        for ( k = kk; k < kend; ++k ) {
                            temp += yy[k*cols+i] * xx[j*cols+k];
                        }
                        zz[j*cols+i] += temp;
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only

template <typename T >
struct matmul_JIK_blocked_unrolled1 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( ii = 0; ii < cols; ii += blockSize ) {
            size_t iend = ii + blockSize;
            if (iend > cols)    iend = cols;
        
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;

                for ( j = 0; j < rows; ++ j ) {
    
                    for ( i = ii; i < iend; ++i ) {
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }

            }
        }
        
    }
};


/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll 2 inner loops

template <typename T >
struct matmul_JIK_blocked_unrolled2 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( ii = 0; ii < cols; ii += blockSize ) {
            size_t iend = ii + blockSize;
            if (iend > cols)    iend = cols;
        
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;

                for ( j = 0 ; j < (rows-3) ; j += 4 ) {
    
                    for ( i = ii; i < iend; ++i ) {
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        // this should be vectorizable
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            T yy0 = yy[(k+0)*cols+i];
                            T yy1 = yy[(k+1)*cols+i];
                            T yy2 = yy[(k+2)*cols+i];
                            T yy3 = yy[(k+3)*cols+i];

                            temp1 += yy0 * xx[(j+0)*cols+k+0] + yy1 * xx[(j+0)*cols+k+1] + yy2 * xx[(j+0)*cols+k+2] + yy3 * xx[(j+0)*cols+k+3];
                            temp2 += yy0 * xx[(j+1)*cols+k+0] + yy1 * xx[(j+1)*cols+k+1] + yy2 * xx[(j+1)*cols+k+2] + yy3 * xx[(j+1)*cols+k+3];
                            temp3 += yy0 * xx[(j+2)*cols+k+0] + yy1 * xx[(j+2)*cols+k+1] + yy2 * xx[(j+2)*cols+k+2] + yy3 * xx[(j+2)*cols+k+3];
                            temp4 += yy0 * xx[(j+3)*cols+k+0] + yy1 * xx[(j+3)*cols+k+1] + yy2 * xx[(j+3)*cols+k+2] + yy3 * xx[(j+3)*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            T yy0 = yy[(k+0)*cols+i];
                            temp1 += yy0 * xx[(j+0)*cols+k];
                            temp2 += yy0 * xx[(j+1)*cols+k];
                            temp3 += yy0 * xx[(j+2)*cols+k];
                            temp4 += yy0 * xx[(j+3)*cols+k];
                        }
                        
                        temp1 += zz[(j+0)*cols+i];
                        temp2 += zz[(j+1)*cols+i];
                        temp3 += zz[(j+2)*cols+i];
                        temp4 += zz[(j+3)*cols+i];
                        
                        zz[(j+0)*cols+i] = temp1;
                        zz[(j+1)*cols+i] = temp2;
                        zz[(j+2)*cols+i] = temp3;
                        zz[(j+3)*cols+i] = temp4;
                    }
                }
                
                for ( ; j < rows; ++ j ) {
    
                    for ( i = ii; i < iend; ++i ) {
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }

            }
        }
        
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll 2 inner loops

template <typename T >
struct matmul_JIK_blocked_unrolled3 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( ii = 0; ii < cols; ii += blockSize ) {
            size_t iend = ii + blockSize;
            if (iend > cols)    iend = cols;
        
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( j = 0; j < rows ; ++j ) {
    
                    for ( i = ii; i < (iend - 3); i += 4 ) {
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        // this should be trivially vectorizable
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            T xx0 = xx[j*cols+k+0];
                            T xx1 = xx[j*cols+k+1];
                            T xx2 = xx[j*cols+k+2];
                            T xx3 = xx[j*cols+k+3];
                
                            temp1 += yy[(k+0)*cols+i+0] * xx0 + yy[(k+1)*cols+i+0] * xx1 + yy[(k+2)*cols+i+0] * xx2 + yy[(k+3)*cols+i+0] * xx3;
                            temp2 += yy[(k+0)*cols+i+1] * xx0 + yy[(k+1)*cols+i+1] * xx1 + yy[(k+2)*cols+i+1] * xx2 + yy[(k+3)*cols+i+1] * xx3;
                            temp3 += yy[(k+0)*cols+i+2] * xx0 + yy[(k+1)*cols+i+2] * xx1 + yy[(k+2)*cols+i+2] * xx2 + yy[(k+3)*cols+i+2] * xx3;
                            temp4 += yy[(k+0)*cols+i+3] * xx0 + yy[(k+1)*cols+i+3] * xx1 + yy[(k+2)*cols+i+3] * xx2 + yy[(k+3)*cols+i+3] * xx3;
                        }
                        
                        for ( ; k < kend; ++k ) {
                            T xx0 = xx[j*cols+k];
                            temp1 += yy[k*cols+i+0] * xx0;
                            temp2 += yy[k*cols+i+1] * xx0;
                            temp3 += yy[k*cols+i+2] * xx0;
                            temp4 += yy[k*cols+i+3] * xx0;
                        }
                        
                        temp1 += zz[j*cols+i+0];
                        temp2 += zz[j*cols+i+1];
                        temp3 += zz[j*cols+i+2];
                        temp4 += zz[j*cols+i+3];
                        
                        zz[j*cols+i+0] = temp1;
                        zz[j*cols+i+1] = temp2;
                        zz[j*cols+i+2] = temp3;
                        zz[j*cols+i+3] = temp4;
                    }
    
                    for ( ; i < iend; ++i ) {
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }

            }
        }
        
    }
};


/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only

template <typename T >
struct matmul_IJK_blocked_unrolled1 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, jj, kk;

        // small block in yy, strip in xx, strip in zz
        for ( jj = 0; jj < rows; jj += blockSize ) {
            size_t jend = jj + blockSize;
            if (jend > rows)    jend = rows;
            
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( i = 0; i < cols; ++i ) {
                
                    for ( j = jj; j < jend ; ++j ) {
                        
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        // this should be vectorizable
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll 2 inner loops

template <typename T >
struct matmul_IJK_blocked_unrolled2 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, jj, kk;

        // small block in yy, strip in xx, strip in zz
        for ( jj = 0; jj < rows; jj += blockSize ) {
            size_t jend = jj + blockSize;
            if (jend > rows)    jend = rows;
            
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( i = 0; i < (cols-3); i += 4 ) {
                
                    for ( j = jj; j < jend ; ++j ) {
                        
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        // this should be trivially vectorizable
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            T xx0 = xx[j*cols+k+0];
                            T xx1 = xx[j*cols+k+1];
                            T xx2 = xx[j*cols+k+2];
                            T xx3 = xx[j*cols+k+3];
                
                            temp1 += yy[(k+0)*cols+i+0] * xx0 + yy[(k+1)*cols+i+0] * xx1 + yy[(k+2)*cols+i+0] * xx2 + yy[(k+3)*cols+i+0] * xx3;
                            temp2 += yy[(k+0)*cols+i+1] * xx0 + yy[(k+1)*cols+i+1] * xx1 + yy[(k+2)*cols+i+1] * xx2 + yy[(k+3)*cols+i+1] * xx3;
                            temp3 += yy[(k+0)*cols+i+2] * xx0 + yy[(k+1)*cols+i+2] * xx1 + yy[(k+2)*cols+i+2] * xx2 + yy[(k+3)*cols+i+2] * xx3;
                            temp4 += yy[(k+0)*cols+i+3] * xx0 + yy[(k+1)*cols+i+3] * xx1 + yy[(k+2)*cols+i+3] * xx2 + yy[(k+3)*cols+i+3] * xx3;
                        }
                        
                        for ( ; k < kend; ++k ) {
                            T xx0 = xx[j*cols+k];
                            temp1 += yy[k*cols+i+0] * xx0;
                            temp2 += yy[k*cols+i+1] * xx0;
                            temp3 += yy[k*cols+i+2] * xx0;
                            temp4 += yy[k*cols+i+3] * xx0;
                        }
                        
                        temp1 += zz[j*cols+i+0];
                        temp2 += zz[j*cols+i+1];
                        temp3 += zz[j*cols+i+2];
                        temp4 += zz[j*cols+i+3];
                        
                        zz[j*cols+i+0] = temp1;
                        zz[j*cols+i+1] = temp2;
                        zz[j*cols+i+2] = temp3;
                        zz[j*cols+i+3] = temp4;
                    }
                }
                
                for ( ; i < cols; ++i ) {
                
                    for ( j = jj; j < jend ; ++j ) {
                        
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll 2 inner loops

template <typename T >
struct matmul_IJK_blocked_unrolled3 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, jj, kk;

        // small block in yy, strip in xx, strip in zz
        for ( jj = 0; jj < rows; jj += blockSize ) {
            size_t jend = jj + blockSize;
            if (jend > rows)    jend = rows;
            
            for ( kk = 0; kk < cols; kk += blockSize ) {
                size_t kend = kk + blockSize;
                if (kend > cols)    kend = cols;
                
                for ( i = 0; i < cols; ++i ) {
                
                    for ( j = jj; j < (jend-3) ; j += 4 ) {
                        
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        // this should be vectorizable
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            T yy0 = yy[(k+0)*cols+i];
                            T yy1 = yy[(k+1)*cols+i];
                            T yy2 = yy[(k+2)*cols+i];
                            T yy3 = yy[(k+3)*cols+i];

                            temp1 += yy0 * xx[(j+0)*cols+k+0] + yy1 * xx[(j+0)*cols+k+1] + yy2 * xx[(j+0)*cols+k+2] + yy3 * xx[(j+0)*cols+k+3];
                            temp2 += yy0 * xx[(j+1)*cols+k+0] + yy1 * xx[(j+1)*cols+k+1] + yy2 * xx[(j+1)*cols+k+2] + yy3 * xx[(j+1)*cols+k+3];
                            temp3 += yy0 * xx[(j+2)*cols+k+0] + yy1 * xx[(j+2)*cols+k+1] + yy2 * xx[(j+2)*cols+k+2] + yy3 * xx[(j+2)*cols+k+3];
                            temp4 += yy0 * xx[(j+3)*cols+k+0] + yy1 * xx[(j+3)*cols+k+1] + yy2 * xx[(j+3)*cols+k+2] + yy3 * xx[(j+3)*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            T yy0 = yy[(k+0)*cols+i];
                            temp1 += yy0 * xx[(j+0)*cols+k];
                            temp2 += yy0 * xx[(j+1)*cols+k];
                            temp3 += yy0 * xx[(j+2)*cols+k];
                            temp4 += yy0 * xx[(j+3)*cols+k];
                        }
                        
                        temp1 += zz[(j+0)*cols+i];
                        temp2 += zz[(j+1)*cols+i];
                        temp3 += zz[(j+2)*cols+i];
                        temp4 += zz[(j+3)*cols+i];
                        
                        zz[(j+0)*cols+i] = temp1;
                        zz[(j+1)*cols+i] = temp2;
                        zz[(j+2)*cols+i] = temp3;
                        zz[(j+3)*cols+i] = temp4;
                    }
                
                    for ( ; j < jend ; ++j ) {
                        
                        T temp1 = T(0);
                        T temp2 = T(0);
                        T temp3 = T(0);
                        T temp4 = T(0);
                        
                        for ( k = kk; k < (kend-3); k += 4 ) {
                            temp1 += yy[(k+0)*cols+i] * xx[j*cols+k+0];
                            temp2 += yy[(k+1)*cols+i] * xx[j*cols+k+1];
                            temp3 += yy[(k+2)*cols+i] * xx[j*cols+k+2];
                            temp4 += yy[(k+3)*cols+i] * xx[j*cols+k+3];
                        }
                        
                        for ( ; k < kend; ++k ) {
                            temp1 += yy[k*cols+i] * xx[j*cols+k];
                        }
                        
                        zz[j*cols+i] += temp1 + temp2 + temp3 + temp4;
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// one input is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll just the inner loop

template <typename T >
struct matmul_JKI_blocked_unrolled1 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( kk = 0; kk < cols; kk += blockSize ) {
            size_t kend = kk + blockSize;
            if (kend > cols)    kend = cols;
        
            for ( ii = 0; ii < cols; ii += blockSizeINF ) {
                size_t iend = ii + blockSizeINF;
                if (iend > cols)    iend = cols;
                
                for ( j = 0 ; j < rows ; ++ j ) {
                    
                    for ( k = kk; k < kend; ++k ) {
                        T temp = xx[j*cols+k];
                        
                        // this should be trivially vectorizable (2 loads, 1 MULADD, 1 store)
                        for ( i = ii; i < (iend - 3); i += 4 ) {
                            zz[j*cols+i+0] += yy[k*cols+i+0] * temp;
                            zz[j*cols+i+1] += yy[k*cols+i+1] * temp;
                            zz[j*cols+i+2] += yy[k*cols+i+2] * temp;
                            zz[j*cols+i+3] += yy[k*cols+i+3] * temp;
                        }
                        
                        for ( ; i < iend; ++i ) {
                            zz[j*cols+i] += yy[k*cols+i] * temp;
                        }
                        
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// one input is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll one inner loop

template <typename T >
struct matmul_JKI_blocked_unrolled2 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( kk = 0; kk < cols; kk += blockSize ) {
            size_t kend = kk + blockSize;
            if (kend > cols)    kend = cols;
        
            for ( ii = 0; ii < cols; ii += blockSizeINF ) {
                size_t iend = ii + blockSizeINF;
                if (iend > cols)    iend = cols;
                
                for ( j = 0 ; j < rows ; ++ j ) {
                    
                    for ( k = kk; k < (kend - 3); k += 4 ) {
                        T temp0 = xx[j*cols+k+0];
                        T temp1 = xx[j*cols+k+1];
                        T temp2 = xx[j*cols+k+2];
                        T temp3 = xx[j*cols+k+3];
                        
                        // this could be unrolled and vectorized
                        for ( i = ii; i < iend; ++i ) {
                            zz[j*cols+i] += yy[(k+0)*cols+i] * temp0 + yy[(k+1)*cols+i] * temp1 + yy[(k+2)*cols+i] * temp2 + yy[(k+3)*cols+i] * temp3;
                        }
                        
                    }
                    
                    for ( ; k < kend; ++k ) {
                        T temp = xx[j*cols+k];
                        
                        for ( i = ii; i < iend; ++i ) {
                            zz[j*cols+i] += yy[k*cols+i] * temp;
                        }
                        
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/

// one input is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll 2 inner loops
// NOTE - further unrolling has not helped on any available compiler

template <typename T >
struct matmul_JKI_blocked_unrolled3 {
    void operator()(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
        size_t i, j, k, ii, kk;

        for ( kk = 0; kk < cols; kk += blockSize ) {
            size_t kend = kk + blockSize;
            if (kend > cols)    kend = cols;
        
            for ( ii = 0; ii < cols; ii += blockSizeINF ) {
                size_t iend = ii + blockSizeINF;
                if (iend > cols)    iend = cols;
                
                for ( j = 0 ; j < rows ; ++ j ) {
                    
                    for ( k = kk; k < (kend - 3); k += 4 ) {
                        T temp0 = xx[j*cols+k+0];
                        T temp1 = xx[j*cols+k+1];
                        T temp2 = xx[j*cols+k+2];
                        T temp3 = xx[j*cols+k+3];
                        
                        // this should be trivially vectorizable (5 loads, 4 MULADD, 1 store)
                        for ( i = ii; i < (iend - 3); i += 4 ) {
                            zz[j*cols+i+0] += yy[(k+0)*cols+i+0] * temp0 + yy[(k+1)*cols+i+0] * temp1 + yy[(k+2)*cols+i+0] * temp2 + yy[(k+3)*cols+i+0] * temp3;
                            zz[j*cols+i+1] += yy[(k+0)*cols+i+1] * temp0 + yy[(k+1)*cols+i+1] * temp1 + yy[(k+2)*cols+i+1] * temp2 + yy[(k+3)*cols+i+1] * temp3;
                            zz[j*cols+i+2] += yy[(k+0)*cols+i+2] * temp0 + yy[(k+1)*cols+i+2] * temp1 + yy[(k+2)*cols+i+2] * temp2 + yy[(k+3)*cols+i+2] * temp3;
                            zz[j*cols+i+3] += yy[(k+0)*cols+i+3] * temp0 + yy[(k+1)*cols+i+3] * temp1 + yy[(k+2)*cols+i+3] * temp2 + yy[(k+3)*cols+i+3] * temp3;
                        }
                        
                        for ( ; i < iend; ++i ) {
                            zz[j*cols+i] += yy[(k+0)*cols+i] * temp0 + yy[(k+1)*cols+i] * temp1 + yy[(k+2)*cols+i] * temp2 + yy[(k+3)*cols+i] * temp3;
                        }
                        
                    }
                    
                    for ( ; k < kend; ++k ) {
                        T temp = xx[j*cols+k];
                        
                        // this should be trivially vectorizable (2 loads, 1 MULADD, 1 store)
                        for ( i = ii; i < (iend - 3); i += 4 ) {
                            zz[j*cols+i+0] += yy[k*cols+i+0] * temp;
                            zz[j*cols+i+1] += yy[k*cols+i+1] * temp;
                            zz[j*cols+i+2] += yy[k*cols+i+2] * temp;
                            zz[j*cols+i+3] += yy[k*cols+i+3] * temp;
                        }
                        
                        for ( ; i < iend; ++i ) {
                            zz[j*cols+i] += yy[k*cols+i] * temp;
                        }
                        
                    }
                }
            }
        }
        
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
void zero_matrix(T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
    T value = T(0);
    for (size_t j = 0 ; j < rows ; ++j ) {
        for (size_t k = 0; k < cols; ++k ) {
            zz[j*cols+k] = value;
        }
    }
}

/******************************************************************************/

template <typename T>
T sum_matrix(const T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {
    T result = T(0);
    for (size_t j = 0 ; j < rows ; ++j ) {
        for (size_t k = 0; k < cols; ++k ) {
            result += zz[j*cols+k];
        }
    }
    return result;
}
/******************************************************************************/

template <typename T>
bool matrix_equal(const T zz[HEIGHT*WIDTH], const T xx[HEIGHT*WIDTH], size_t rows, size_t cols) {
    for (size_t j = 0 ; j < rows ; ++j ) {
        for (size_t k = 0; k < cols; ++k ) {
            if (!tolerance_equal(zz[j*cols+k], xx[j*cols+k]))
                return false;
        }
    }
    return true;
}

/******************************************************************************/

template <typename T>
void fill_matrix_pattern1(T *zz, size_t rows, size_t cols) {
    for (size_t j = 0 ; j < rows ; ++j ) {
        for (size_t k = 0; k < cols; ++k ) {
            zz[j * cols + k] = T(j);
        }
    }

}

/******************************************************************************/

template <typename T>
void fill_matrix_diagonal(T *zz, size_t rows, size_t cols) {
    for (size_t j = 0 ; j < rows ; ++j ) {
        for (size_t k = 0; k < cols; ++k ) {
            zz[j * cols + k] = ((k == j) ? T(1) : T(0));
        }
    }
}

/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename MM >
void test_matmul(const T xx[HEIGHT*WIDTH], const T yy[WIDTH*HEIGHT],
                T zz[HEIGHT*WIDTH], size_t rows, size_t cols, MM multiplier, const std::string label) {

    start_timer();

    for(size_t i = 0; i < iterations; ++i) {

        // zero the accumulator
        zero_matrix(zz, rows, cols);

        // call our multiplier function
        multiplier(xx, yy, zz, rows, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    // sum the accumulator
    T result = sum_matrix(zz, rows, cols);

    // and verify the result
    check_sum<T>(result, label);
}

/******************************************************************************/

template <typename T >
void verify_matmul( T xx[HEIGHT*WIDTH], T yy[WIDTH*HEIGHT], T zz[HEIGHT*WIDTH], size_t rows, size_t cols) {

    T master_result[HEIGHT*WIDTH];
    
    // zero the accumulator
    zero_matrix( master_result, rows, cols );
    
    // fill the inputs with known values
    fill_matrix_pattern1( (T*)xx, rows, cols );
    fill_matrix_pattern1( (T*)yy, cols, rows );
    //fill_matrix_diagonal( (T*)yy, cols, rows );
    
    // perform our reference multiply (simplest code)
    matmul_KIJ<T>()( xx, yy, master_result, rows, cols );
    
    
    zero_matrix(zz, rows, cols);
    matmul_KJI<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_KJI failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JIK<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_IJK<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_IKJ<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IKJ failed verification\n");
        
        
        
    zero_matrix(zz, rows, cols);
    matmul_KIJ_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_KIJ_temp failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_KJI_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_KJI_temp failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_temp failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JIK_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK_temp failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_IJK_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK_temp failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_IKJ_temp<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IKJ_temp failed verification\n");
        
    
    zero_matrix(zz, rows, cols);
    matmul_KJI_unrolled<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_KJI_unrolled failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_JKI_unrolled<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_unrolled failed verification\n");
    
        
    zero_matrix(zz, rows, cols);
    matmul_JIK_blocked<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK_blocked failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI_blocked<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_blocked failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_IJK_blocked<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK_blocked failed verification\n");
    
    
        
    zero_matrix(zz, rows, cols);
    matmul_JIK_blocked_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK_blocked_unrolled1 failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_JIK_blocked_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK_blocked_unrolled2 failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_JIK_blocked_unrolled3<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JIK_blocked_unrolled3 failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_IJK_blocked_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK_blocked_unrolled1 failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_IJK_blocked_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK_blocked_unrolled2 failed verification\n");
        
    zero_matrix(zz, rows, cols);
    matmul_IJK_blocked_unrolled3<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_IJK_blocked_unrolled3 failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI_blocked_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_blocked_unrolled1 failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI_blocked_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_blocked_unrolled2 failed verification\n");
    
    zero_matrix(zz, rows, cols);
    matmul_JKI_blocked_unrolled3<T>()( xx, yy, zz, rows, cols );
    if (!matrix_equal( master_result, zz, rows, cols ))
        printf("matmul_JKI_blocked_unrolled3 failed verification\n");

}

/******************************************************************************/
/******************************************************************************/
template< typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    // if these are stack arrays, MSVC silently exits the app
    // while all other compilers work without a problem
    std::unique_ptr<T> dX( new T[HEIGHT * WIDTH] );
    std::unique_ptr<T> dY( new T[HEIGHT * WIDTH] );
    std::unique_ptr<T> dZ( new T[HEIGHT * WIDTH]);
    T *dataX = dX.get();
    T *dataY = dY.get();
    T *dataZ = dZ.get();

    // verify results (unit test)
    verify_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH );
    
    ::fill( dataX, dataX+(HEIGHT*WIDTH), T(init_value));
    ::fill( dataY, dataY+(HEIGHT*WIDTH), T(init_value));
    ::fill( dataZ, dataZ+(HEIGHT*WIDTH), T(init_value));

    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_KIJ<T>(), myTypeName + " matrix multiply KIJ" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_KJI<T>(), myTypeName + " matrix multiply KJI" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI<T>(), myTypeName + " matrix multiply JKI" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK<T>(), myTypeName + " matrix multiply JIK" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK<T>(), myTypeName + " matrix multiply IJK" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IKJ<T>(), myTypeName + " matrix multiply IKJ" );

    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_KIJ_temp<T>(), myTypeName + " matrix multiply KIJ temp" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_KJI_temp<T>(), myTypeName + " matrix multiply KJI temp" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_temp<T>(), myTypeName + " matrix multiply JKI temp" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK_temp<T>(), myTypeName + " matrix multiply JIK temp" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK_temp<T>(), myTypeName + " matrix multiply IJK temp" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IKJ_temp<T>(), myTypeName + " matrix multiply IKJ temp" );

    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_KJI_unrolled<T>(), myTypeName + " matrix multiply KJI unrolled" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_unrolled<T>(), myTypeName + " matrix multiply JKI unrolled" );
    
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK_blocked<T>(), myTypeName + " matrix multiply JIK blocked" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_blocked<T>(), myTypeName + " matrix multiply JKI blocked" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK_blocked<T>(), myTypeName + " matrix multiply IJK blocked" );
    
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK_blocked_unrolled1<T>(), myTypeName + " matrix multiply JIK blocked unrolled1" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK_blocked_unrolled2<T>(), myTypeName + " matrix multiply JIK blocked unrolled2" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JIK_blocked_unrolled3<T>(), myTypeName + " matrix multiply JIK blocked unrolled3" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK_blocked_unrolled1<T>(), myTypeName + " matrix multiply IJK blocked unrolled1" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK_blocked_unrolled2<T>(), myTypeName + " matrix multiply IJK blocked unrolled2" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_IJK_blocked_unrolled3<T>(), myTypeName + " matrix multiply IJK blocked unrolled3" );
    
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_blocked_unrolled1<T>(), myTypeName + " matrix multiply JKI blocked unrolled1" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_blocked_unrolled2<T>(), myTypeName + " matrix multiply JKI blocked unrolled2" );
    test_matmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matmul_JKI_blocked_unrolled3<T>(), myTypeName + " matrix multiply JKI blocked unrolled3" );

    std::string temp1( myTypeName + " matrix multiply" );
    summarize( temp1.c_str(), (2*HEIGHT*WIDTH*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );

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


    TestOneType<int32_t>();
    TestOneType<float>();

    iterations /= 2;
    TestOneType<double>();


#if WORKS_BUT_SLOW
    TestOneType<int64_t>();
    TestOneType<long double>();
#endif
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
