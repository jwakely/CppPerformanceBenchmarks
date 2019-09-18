/*
    Coppyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to matrix vector multiplication

Assumptions:

    1) the compiler will recognize matrix vector multiplication patterns
        and substitute optimal patterns



NOTE - the techniques used to optimize a matrix vector product, if done generally, can help many other problems.

NOTE - ccox - performance may suffer without a multiply-add operation

NOTE - ccox - int32 is sometimes slower than double, despite double using twice the memory!

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
#include "benchmark_algorithms.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 8000;

// 300k items, or about 2.4 Meg of data per matrix - intended to exceed the L1 cache
const int WIDTH = 600;
const int HEIGHT = 800;

const int SIZE = HEIGHT*WIDTH;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
    T temp = (T)(HEIGHT*WIDTH) * (T)init_value * (T)init_value;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct matvecmul_IJ {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            for ( j = 0 ; j < cols ; ++j ) {
                yy[i] += zz[i*cols+j] * xx[j];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct matvecmul_JI {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( j = 0 ; j < cols ; ++j ) {
            for ( i = 0; i < rows; ++i ) {
                yy[i] += zz[i*cols+j] * xx[j];
            }
        }
    }
};

/******************************************************************************/

// one value is used repeatedly in inner loop, pull it out

template <typename T >
struct matvecmul_IJ_temp {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            T temp = yy[i];
            for ( j = 0 ; j < cols ; ++j ) {
                temp += zz[i*cols+j] * xx[j];
            }
            yy[i] = temp;
        }
    }
};

/******************************************************************************/

// one value is used repeatedly in inner loop, pull it out
// matrix first index is constant, pull it out

template <typename T >
struct matvecmul_IJ_temp1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            T temp = yy[i];
            const T *zI = zz+i*cols;
            for ( j = 0 ; j < cols ; ++j ) {
                temp += zI[j] * xx[j];
            }
            yy[i] = temp;
        }
    }
};

/******************************************************************************/

// one input is constant in inner loop, pull it out

template <typename T >
struct matvecmul_JI_temp {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( j = 0 ; j < cols ; ++j ) {
            T temp = xx[j];
            for ( i = 0; i < rows; ++i ) {
                yy[i] += zz[i*cols+j] * temp;
            }
        }
    }
};

/******************************************************************************/

// unroll the inner loop
// pull reused terms out of loops

template <typename T >
struct matvecmul_IJ_unrolled {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            T temp = yy[i];
            T temp1 = 0;
            T temp2 = 0;
            T temp3 = 0;
            const T *zI = zz+i*cols;
            
            for ( j = 0 ; j < (cols-3) ; j += 4 ) {
                temp  += zI[j+0] * xx[j+0];
                temp1 += zI[j+1] * xx[j+1];
                temp2 += zI[j+2] * xx[j+2];
                temp3 += zI[j+3] * xx[j+3];
            }
            
            for ( ; j < cols ; ++j ) {
                temp += zI[j] * xx[j];
            }
            
            yy[i] = temp + temp1 + temp2 + temp3;
        }
    }
};

/******************************************************************************/

// unroll the inner loop
// pull reused terms out of loops
// explicitly make it look like a 4 item vector operation

template <typename T >
struct matvecmul_IJ_unrolled1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            T temp[4];
            temp[0] = yy[i];
            temp[1] = 0;
            temp[2] = 0;
            temp[3] = 0;
            
            const T *zI = zz+i*cols;
            
            for ( j = 0 ; j < (cols-3) ; j += 4 ) {
                temp[0] += zI[j+0] * xx[j+0];
                temp[1] += zI[j+1] * xx[j+1];
                temp[2] += zI[j+2] * xx[j+2];
                temp[3] += zI[j+3] * xx[j+3];
            }
            
            for ( ; j < cols ; ++j ) {
                temp[0] += zI[j] * xx[j];
            }
            
            yy[i] = temp[0] + temp[1] + temp[2] + temp[3];
        }
    }
};

/******************************************************************************/

// unroll the inner loop
// pull reused terms out of loops
// explicitly make it look like an 8 item vector operation

template <typename T >
struct matvecmul_IJ_unrolled2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( i = 0; i < rows; ++i ) {
            T temp[8];
            temp[0] = yy[i];
            temp[1] = 0;
            temp[2] = 0;
            temp[3] = 0;
            temp[4] = 0;
            temp[2] = 0;
            temp[5] = 0;
            temp[6] = 0;
            temp[7] = 0;
            
            const T *zI = zz+i*cols;
            
            for ( j = 0 ; j < (cols-7) ; j += 8 ) {
                temp[0] += zI[j+0] * xx[j+0];
                temp[1] += zI[j+1] * xx[j+1];
                temp[2] += zI[j+2] * xx[j+2];
                temp[3] += zI[j+3] * xx[j+3];
                temp[4] += zI[j+4] * xx[j+4];
                temp[5] += zI[j+5] * xx[j+5];
                temp[6] += zI[j+6] * xx[j+6];
                temp[7] += zI[j+7] * xx[j+7];
            }
            
            for ( ; j < cols ; ++j ) {
                temp[0] += zI[j] * xx[j];
            }
            
            yy[i] = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
        }
    }
};

/******************************************************************************/

// unroll the inner loop
// pull constant terms out of loops

template <typename T >
struct matvecmul_JI_unrolled {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        for ( j = 0 ; j < cols ; ++j ) {
            T temp = xx[j];
            
            for ( i = 0; i < (rows-3); i += 4 ) {
                yy[i+0] += zz[(i+0)*cols+j] * temp;
                yy[i+1] += zz[(i+1)*cols+j] * temp;
                yy[i+2] += zz[(i+2)*cols+j] * temp;
                yy[i+3] += zz[(i+3)*cols+j] * temp;
            }
            
            for ( ; i < rows; ++i ) {
                yy[i] += zz[i*cols+j] * temp;
            }
            
        }
    }
};

/******************************************************************************/

// unroll both loops

template <typename T >
struct matvecmul_JI_unrolled1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        
        for ( j = 0 ; j < (cols-3) ; j+=4 ) {
            const T *zI = zz;
            
            for ( i = 0; i < (rows-3); i += 4 ) {
                yy[i+0] += zI[0*cols+j+0] * xx[j+0];
                yy[i+0] += zI[0*cols+j+1] * xx[j+1];
                yy[i+0] += zI[0*cols+j+2] * xx[j+2];
                yy[i+0] += zI[0*cols+j+3] * xx[j+3];
                
                yy[i+1] += zI[1*cols+j+0] * xx[j+0];
                yy[i+1] += zI[1*cols+j+1] * xx[j+1];
                yy[i+1] += zI[1*cols+j+2] * xx[j+2];
                yy[i+1] += zI[1*cols+j+3] * xx[j+3];
                
                yy[i+2] += zI[2*cols+j+0] * xx[j+0];
                yy[i+2] += zI[2*cols+j+1] * xx[j+1];
                yy[i+2] += zI[2*cols+j+2] * xx[j+2];
                yy[i+2] += zI[2*cols+j+3] * xx[j+3];
                
                yy[i+3] += zI[3*cols+j+0] * xx[j+0];
                yy[i+3] += zI[3*cols+j+1] * xx[j+1];
                yy[i+3] += zI[3*cols+j+2] * xx[j+2];
                yy[i+3] += zI[3*cols+j+3] * xx[j+3];
                
                zI += 4*cols;
            }
            
            for ( ; i < rows; ++i ) {
                yy[i] += zI[j+0] * xx[j+0];
                yy[i] += zI[j+1] * xx[j+1];
                yy[i] += zI[j+2] * xx[j+2];
                yy[i] += zI[j+3] * xx[j+3];
                zI += cols;
            }
        }
        
        for ( ; j < cols ; ++j ) {
            T temp = xx[j];
            
            for ( i = 0; i < (rows-3); i += 4 ) {
                yy[i+0] += zz[(i+0)*cols+j] * temp;
                yy[i+1] += zz[(i+1)*cols+j] * temp;
                yy[i+2] += zz[(i+2)*cols+j] * temp;
                yy[i+3] += zz[(i+3)*cols+j] * temp;
            }
            
            for ( ; i < rows; ++i ) {
                yy[i] += zz[i*cols+j] * temp;
            }
            
        }
    }
};

/******************************************************************************/

// unroll both loops
// reorder calculations (because instruction scheduling and vectorization appears problematic on some compilers)

template <typename T >
struct matvecmul_JI_unrolled2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH], int rows, int cols) {
        int i, j;
        
        for ( j = 0 ; j < (cols-3) ; j+=4 ) {
            const T *zI = zz;
            
            for ( i = 0; i < (rows-3); i += 4 ) {
                yy[i+0] += zI[0*cols+j+0] * xx[j+0];
                yy[i+1] += zI[1*cols+j+0] * xx[j+0];
                yy[i+2] += zI[2*cols+j+0] * xx[j+0];
                yy[i+3] += zI[3*cols+j+0] * xx[j+0];
                
                yy[i+0] += zI[0*cols+j+1] * xx[j+1];
                yy[i+1] += zI[1*cols+j+1] * xx[j+1];
                yy[i+2] += zI[2*cols+j+1] * xx[j+1];
                yy[i+3] += zI[3*cols+j+1] * xx[j+1];
                
                yy[i+0] += zI[0*cols+j+2] * xx[j+2];
                yy[i+1] += zI[1*cols+j+2] * xx[j+2];
                yy[i+2] += zI[2*cols+j+2] * xx[j+2];
                yy[i+3] += zI[3*cols+j+2] * xx[j+2];
                
                yy[i+0] += zI[0*cols+j+3] * xx[j+3];
                yy[i+1] += zI[1*cols+j+3] * xx[j+3];
                yy[i+2] += zI[2*cols+j+3] * xx[j+3];
                yy[i+3] += zI[3*cols+j+3] * xx[j+3];
                
                zI += 4*cols;
            }
            
            for ( ; i < rows; ++i ) {
                yy[i] += zI[j+0] * xx[j+0];
                yy[i] += zI[j+1] * xx[j+1];
                yy[i] += zI[j+2] * xx[j+2];
                yy[i] += zI[j+3] * xx[j+3];
                zI += cols;
            }
        }
        
        for ( ; j < cols ; ++j ) {
            T temp = xx[j];
            
            for ( i = 0; i < (rows-3); i += 4 ) {
                yy[i+0] += zz[(i+0)*cols+j] * temp;
                yy[i+1] += zz[(i+1)*cols+j] * temp;
                yy[i+2] += zz[(i+2)*cols+j] * temp;
                yy[i+3] += zz[(i+3)*cols+j] * temp;
            }
            
            for ( ; i < rows; ++i ) {
                yy[i] += zz[i*cols+j] * temp;
            }
            
        }
    }
};

/******************************************************************************/

/*
    The cache block sizes were experimentally/exhaustively
    determined on an Intel i7 X5550 2.66 Ghz system
    with LLVM/Clang as the compiler.
 
    See plot_cacheblock_sizes() below.
    Probably needs a revisit to investigate smoothness/stability.
*/

// IJ_blocked1: 80 int32, 30 float, 12 double
template<typename T> int blocksize1() { return 64; }
template<> int blocksize1<int32_t>() { return 80; }
template<> int blocksize1<float>() { return 30; }
template<> int blocksize1<double>() { return 12; }

// one term is reused in inner loop, pull it out
// iterate over 1D subblock of outer index to improve cache usage
// A textbook example.

template <typename T >
struct matvecmul_IJ_blocked1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockSize = blocksize1<T>() ) {
        int i, j, ii;
        
        for ( ii = 0; ii < rows; ii += blockSize ) {
            int iend = ii + blockSize;
            if (iend > rows)    iend = rows;
            
            for ( j = 0 ; j < cols ; ++j ) {
                const T temp = xx[j];
                
                for ( i = ii; i < iend; ++i ) {
                    yy[i] += zz[i*cols+j] * temp;
                }
            }
        }
    }
};

/******************************************************************************/

// JI_blocked1:  600/INF int32, 32 float, 600/INF double
template<typename T> int blocksize3() { return 32; }
template<> int blocksize3<int32_t>() { return 600; }
template<> int blocksize3<float>() { return 32; }       // Why ????
template<> int blocksize3<double>() { return 600; }


// one input is constant over the inner loop, pull it out
// iterate over 1D subblock of outer index to improve cache usage
// Another textbook example.

template <typename T >
struct matvecmul_JI_blocked1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                int rows, int cols, int blockSize = blocksize3<T>() ) {
        int i, j, jj;
        
        for ( jj = 0; jj < cols; jj += blockSize ) {
            int jend = jj + blockSize;
            if (jend > cols)    jend = cols;
            
            for ( i = 0; i < rows; ++i ) {
                T temp = yy[i];
                const T *zI = zz+i*cols;
                
                for ( j = jj ; j < jend ; ++j ) {
                    temp += zI[j] * xx[j];
                }
            
                yy[i] = temp;
            }
        }
    }
};

/******************************************************************************/

// IJ_blocked2:    A=8, B=300/INF int32,  A=27, B=20 float,  A=14, B=28 double
template<typename T> int blocksize6A() { return 16; }
template<typename T> int blocksize6B() { return 300; }
template<> int blocksize6A<int32_t>() { return 8; }
template<> int blocksize6B<int32_t>() { return 300; }
template<> int blocksize6A<float>() { return 27; }
template<> int blocksize6B<float>() { return 20; }
template<> int blocksize6A<double>() { return 14; }
template<> int blocksize6B<double>() { return 28; }


// summed term is reused in inner loop, pull it out
// iterate over 2D subblocks to improve cache usage
// A textbook example.

template <typename T >
struct matvecmul_IJ_blocked2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockA = blocksize6A<T>(), int blockB = blocksize6B<T>() ) {
        int i, j, ii, jj;
        
        for ( ii = 0; ii < rows; ii += blockA ) {
            int iend = ii + blockA;
            if (iend > rows)    iend = rows;
        
            for ( jj = 0; jj < cols; jj += blockB ) {
                int jend = jj + blockB;
                if (jend > cols)    jend = cols;
        
                for ( i = ii; i < iend; ++i ) {
                    T temp = yy[i];
                    const T *zI = zz+i*cols;
                    
                    for ( j = jj ; j < jend ; ++j ) {
                        temp += zI[j] * xx[j];
                    }
                    
                    yy[i] = temp;
                }
            }
        }
    }
};

/******************************************************************************/

// JI_blocked2:    A=300/INF, B=76 int32, A=300/INF, B=81 float, A=300/INF, B=15 double
template<typename T> int blocksize7A() { return 300; }
template<typename T> int blocksize7B() { return 64; }
template<> int blocksize7A<int32_t>() { return 300; }
template<> int blocksize7B<int32_t>() { return 76; }
template<> int blocksize7A<float>() { return 300; }
template<> int blocksize7B<float>() { return 81; }
template<> int blocksize7A<double>() { return 300; }
template<> int blocksize7B<double>() { return 15; }


// one input is constant over the inner loop, pull it out
// iterate over 2D subblocks to improve cache usage
// Another textbook example.

template <typename T >
struct matvecmul_JI_blocked2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockA = blocksize7A<T>(), int blockB = blocksize7B<T>() ) {
        int i, j, ii, jj;
        
        for ( jj = 0; jj < cols; jj += blockA ) {
            int jend = jj + blockA;
            if (jend > cols)    jend = cols;
        
            for ( ii = 0; ii < rows; ii += blockB ) {
                int iend = ii + blockB;
                if (iend > rows)    iend = rows;
        
                for ( j = jj ; j < jend ; ++j ) {
                    const T temp = xx[j];
                    
                    for ( i = ii; i < iend; ++i ) {
                        yy[i] += zz[i*cols+j] * temp;
                    }
                }
            }
        }
    }
};

/******************************************************************************/

// IJ_blocked_unrolled1: 80 int32, 80 float, 16 double
template<typename T> int blocksize4() { return 64; }
template<> int blocksize4<int32_t>() { return 80; }
template<> int blocksize4<float>() { return 80; }
template<> int blocksize4<double>() { return 16; }


// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only

template <typename T>
struct matvecmul_IJ_blocked_unrolled1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockSize = blocksize4<T>() ) {
        int i, j, ii;
        
        for ( ii = 0; ii < rows; ii += blockSize ) {
            int iend = ii + blockSize;
            if (iend > rows)    iend = rows;
            
            for ( j = 0 ; j < cols ; ++j ) {
                const T temp = xx[j];
                
                for ( i = ii; i < (iend-3); i += 4 ) {
                    yy[i+0] += zz[(i+0)*cols+j] * temp;
                    yy[i+1] += zz[(i+1)*cols+j] * temp;
                    yy[i+2] += zz[(i+2)*cols+j] * temp;
                    yy[i+3] += zz[(i+3)*cols+j] * temp;
                }
            
                for ( ; i < iend; ++i ) {
                    yy[i] += zz[i*cols+j] * temp;
                }
            }
        }
        
    }
};


/******************************************************************************/

// JI_blocked_unrolled1:   600/INF int32, 600/INF float, 600/INF double
template<typename T> int blocksize5() { return 600; }
template<> int blocksize5<int32_t>() { return 600; }
template<> int blocksize5<float>() { return 600; }
template<> int blocksize5<double>() { return 600; }


// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only
// explicitly make it look like a 4 item vector operation

template <typename T >
struct matvecmul_JI_blocked_unrolled1 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockSize = blocksize5<T>() ) {
        int i, j, jj;
        
        for ( jj = 0; jj < cols; jj += blockSize ) {
            int jend = jj + blockSize;
            if (jend > cols)    jend = cols;
            
            for ( i = 0; i < rows; ++i ) {
                T temp[4];
                temp[0] = yy[i];
                temp[1] = 0;
                temp[2] = 0;
                temp[3] = 0;
                const T *zI = zz+i*cols;
            
                for ( j = jj ; j < (jend-3) ; j += 4 ) {
                    temp[0] += zI[j+0] * xx[j+0];
                    temp[1] += zI[j+1] * xx[j+1];
                    temp[2] += zI[j+2] * xx[j+2];
                    temp[3] += zI[j+3] * xx[j+3];
                }
            
                for ( ; j < jend ; ++j ) {
                    temp[0] += zI[j] * xx[j];
                }
            
                yy[i] = temp[0] + temp[1] + temp[2] + temp[3];
            }
        }
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only
// explicitly make it look like a 8 item vector operation

template <typename T >
struct matvecmul_JI_blocked_unrolled1A {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockSize = blocksize5<T>() ) {
        int i, j, jj;
        
        for ( jj = 0; jj < cols; jj += blockSize ) {
            int jend = jj + blockSize;
            if (jend > cols)    jend = cols;
            
            for ( i = 0; i < rows; ++i ) {
                T temp[8];
                temp[0] = yy[i];
                temp[1] = 0;
                temp[2] = 0;
                temp[3] = 0;
                temp[4] = 0;
                temp[2] = 0;
                temp[5] = 0;
                temp[6] = 0;
                temp[7] = 0;
                
                const T *zI = zz+i*cols;
                
                for ( j = 0 ; j < (cols-7) ; j += 8 ) {
                    temp[0] += zI[j+0] * xx[j+0];
                    temp[1] += zI[j+1] * xx[j+1];
                    temp[2] += zI[j+2] * xx[j+2];
                    temp[3] += zI[j+3] * xx[j+3];
                    temp[4] += zI[j+4] * xx[j+4];
                    temp[5] += zI[j+5] * xx[j+5];
                    temp[6] += zI[j+6] * xx[j+6];
                    temp[7] += zI[j+7] * xx[j+7];
                }
                
                for ( ; j < cols ; ++j ) {
                    temp[0] += zI[j] * xx[j];
                }
                
                yy[i] = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
            }
        }
    }
};

/******************************************************************************/


// IJ_BlockUnroll2:   A=11, B=300/INF int32, A=25, B=256 float, A=12, B=116 double
template<typename T> int blocksize8A() { return 16; }
template<typename T> int blocksize8B() { return 256; }
template<> int blocksize8A<int32_t>() { return 11; }
template<> int blocksize8B<int32_t>() { return 300; }
template<> int blocksize8A<float>() { return 25; }
template<> int blocksize8B<float>() { return 256; }
template<> int blocksize8A<double>() { return 12; }
template<> int blocksize8B<double>() { return 116; }


// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only
// explicitly make it look like a 4 item vector operation

template <typename T >
struct matvecmul_IJ_blocked_unrolled2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockA = blocksize8A<T>(), int blockB = blocksize8B<T>() ) {
        int i, j, ii, jj;
        
        for ( ii = 0; ii < rows; ii += blockA ) {
            int iend = ii + blockA;
            if (iend > rows)    iend = rows;
        
            for ( jj = 0; jj < cols; jj += blockB ) {
                int jend = jj + blockB;
                if (jend > cols)    jend = cols;
        
                for ( i = ii; i < iend; ++i ) {
                    T temp[4];
                    temp[0] = yy[i];
                    temp[1] = 0;
                    temp[2] = 0;
                    temp[3] = 0;
                    const T *zI = zz+i*cols;
                    
                    for ( j = jj ; j < (jend-3) ; j += 4 ) {
                        temp[0] += zI[j+0] * xx[j+0];
                        temp[1] += zI[j+1] * xx[j+1];
                        temp[2] += zI[j+2] * xx[j+2];
                        temp[3] += zI[j+3] * xx[j+3];
                    }
                    
                    for ( ; j < jend ; ++j ) {
                        temp[0] += zI[j] * xx[j];
                    }
            
                    yy[i] = temp[0] + temp[1] + temp[2] + temp[3];
                }
            }
        }
    }
};

/******************************************************************************/

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only
// explicitly make it look like an 8 item vector operation

template <typename T >
struct matvecmul_IJ_blocked_unrolled2A {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockA = blocksize8A<T>(), int blockB = blocksize8B<T>() ) {
        int i, j, ii, jj;
        
        for ( ii = 0; ii < rows; ii += blockA ) {
            int iend = ii + blockA;
            if (iend > rows)    iend = rows;
        
            for ( jj = 0; jj < cols; jj += blockB ) {
                int jend = jj + blockB;
                if (jend > cols)    jend = cols;
        
                for ( i = ii; i < iend; ++i ) {
                    T temp[8];
                    temp[0] = yy[i];
                    temp[1] = 0;
                    temp[2] = 0;
                    temp[3] = 0;
                    temp[4] = 0;
                    temp[2] = 0;
                    temp[5] = 0;
                    temp[6] = 0;
                    temp[7] = 0;
                    
                    const T *zI = zz+i*cols;
                    
                    for ( j = jj ; j < (jend-7) ; j += 8 ) {
                        temp[0] += zI[j+0] * xx[j+0];
                        temp[1] += zI[j+1] * xx[j+1];
                        temp[2] += zI[j+2] * xx[j+2];
                        temp[3] += zI[j+3] * xx[j+3];
                        temp[4] += zI[j+4] * xx[j+4];
                        temp[5] += zI[j+5] * xx[j+5];
                        temp[6] += zI[j+6] * xx[j+6];
                        temp[7] += zI[j+7] * xx[j+7];
                    }
                    
                    for ( ; j < jend ; ++j ) {
                        temp[0] += zI[j] * xx[j];
                    }
                    
                    yy[i] = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
                }
            }
        }
    }
};

/******************************************************************************/

// JIBlockUnroll2:   A=248, B=80 int32, A=300/INF, B=100 float, A=300/INF, B=16 double
template<typename T> int blocksize9A() { return 256; }
template<typename T> int blocksize9B() { return 80; }
template<> int blocksize9A<int32_t>() { return 248; }
template<> int blocksize9B<int32_t>() { return 80; }
template<> int blocksize9A<float>() { return 300; }
template<> int blocksize9B<float>() { return 100; }
template<> int blocksize9A<double>() { return 300; }
template<> int blocksize9B<double>() { return 16; }

// summed term is constant in inner loop, pull it out
// iterate over subblocks to improve cache usage
// unroll the inner loop only

template <typename T >
struct matvecmul_JI_blocked_unrolled2 {
    void operator()(const T xx[WIDTH], T yy[HEIGHT], const T zz[HEIGHT*WIDTH],
                    int rows, int cols, int blockA = blocksize9A<T>(), int blockB = blocksize9B<T>() ) {
        int i, j, ii, jj;
        
        for ( jj = 0; jj < cols; jj += blockA ) {
            int jend = jj + blockA;
            if (jend > cols)    jend = cols;
        
            for ( ii = 0; ii < rows; ii += blockB ) {
                int iend = ii + blockB;
                if (iend > rows)    iend = rows;
        
                for ( j = jj ; j < jend ; ++j ) {
                    T temp = xx[j];
                    
                    for ( i = ii; i < (iend-3); i += 4 ) {
                        yy[i+0] += zz[(i+0)*cols+j] * temp;
                        yy[i+1] += zz[(i+1)*cols+j] * temp;
                        yy[i+2] += zz[(i+2)*cols+j] * temp;
                        yy[i+3] += zz[(i+3)*cols+j] * temp;
                    }
                    
                    for ( ; i < iend; ++i ) {
                        yy[i] += zz[i*cols+j] * temp;
                    }
                }
            }
        }
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
void fill_matrix_pattern1(T *zz, int rows, int cols) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            zz[j * cols + k] = T(j);
        }
    }
}

/******************************************************************************/

template <typename T >
bool vector_equal( T *xx, T *yy, const size_t size ) {
    for (size_t i = 0; i < size; ++i)
        if ( !tolerance_equal<T>(xx[i],yy[i]) )
            return false;
    return true;
}

/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename MM >
void test_matvecmul(const T xx[WIDTH], T yy[HEIGHT],
                const T zz[HEIGHT*WIDTH], int rows, int cols,
                MM multiplier, const std::string label) {

    gLabels.push_back( label );
    start_timer();

    for(int i = 0; i < iterations; ++i) {

        // zero the accumulator
        ::fill(yy, yy+HEIGHT, 0);

        // call our multiplier function
        multiplier(xx, yy, zz, rows, cols );
    }
    
    record_result( timer(), gLabels.back().c_str() );
    
    // sum the accumulator
    T result = accumulate(yy, yy+HEIGHT, T(0));

    // and verify the result
    check_sum<T>(result, label);
}

/******************************************************************************/

template <typename T >
void verify_matvecmul( T xx[WIDTH], T yy[HEIGHT], T zz[HEIGHT*WIDTH], int rows, int cols) {

    T master_result[HEIGHT];
    
    // zero the accumulator
    ::fill( master_result, master_result+HEIGHT, 0 );
    
    // fill the inputs with known values
    benchmark::fill_ascending(xx,xx+WIDTH);
    fill_matrix_pattern1( zz, cols, rows );
    
    // perform our reference multiply (simplest code)
    matvecmul_IJ<T>()( xx, master_result, zz, rows, cols );
    
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI failed verification\n");
    
        
        
        
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_temp<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_temp failed verification\n");
    
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_temp1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_temp1 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_temp<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_temp failed verification\n");
    
    
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_unrolled<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_unrolled failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_unrolled2 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_unrolled2 failed verification\n");
        
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_unrolled<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_unrolled failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_unrolled1 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_unrolled2 failed verification\n");
    
        
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_blocked1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_blocked1 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_blocked2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_blocked2 failed verification\n");

    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_blocked1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_blocked1 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_blocked2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_blocked2 failed verification\n");

    
        
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_blocked_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_blocked_unrolled1 failed verification\n");
        
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_blocked_unrolled1<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_blocked_unrolled1 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_blocked_unrolled1A<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_blocked_unrolled1A failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_blocked_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_blocked_unrolled2 failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_IJ_blocked_unrolled2A<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_IJ_blocked_unrolled2A failed verification\n");
    
    // test one multiplier
    ::fill( yy, yy+HEIGHT, 0 );
    matvecmul_JI_blocked_unrolled2<T>()( xx, yy, zz, rows, cols );
    if (!vector_equal(master_result,yy,HEIGHT) )
        printf("matvecmul_JI_blocked_unrolled2 failed verification\n");

}


/******************************************************************************/

// test/plot cache performance for different block sizes

template <typename T, typename MM>
void plot_one_cacheblock_size(const int limit, MM multiplier, const std::string label) {
    
    std::string myTypeName( getTypeName<T>() );
    
    std::string desc = label + " " + myTypeName;

    // if these are stack arrays, MSVC silently exits the app
    // while all other compilers work without a problem
    std::unique_ptr<T> dX( new T[ WIDTH ] );
    std::unique_ptr<T> dY( new T[ HEIGHT ] );
    std::unique_ptr<T> dZ( new T[HEIGHT * WIDTH]);
    T *dataX = dX.get();
    T *dataY = dY.get();
    T *dataZ = dZ.get();
    
    
    ::fill( dataX, dataX+(WIDTH), T(init_value));
    ::fill( dataY, dataY+(HEIGHT), T(init_value));
    ::fill( dataZ, dataZ+(HEIGHT*WIDTH), T(init_value));
    
    int saved_iterations = iterations;
    
    iterations /= 4;

    printf("description, size, sec_min, sec_avg, sec_max, best_performance\n");
    
    double best_perf = 0.0;
    int best_perfA = 0;
    
    const int run_count = 200;
    const int iterations_per_run = std::min(iterations/run_count, 4);
    
    // block = 1 == just overhead, ignore
    for( int block = 2; block <= limit; ++block ) {

        double timer_min = 999.0;
        double timer_max = 0.0;
        double timer_sum = 0.0;
        
        for (int run = 0; run < run_count; ++run) {
        
            start_timer();

            for( int i = 0; i < iterations_per_run; ++i) {

                // zero the accumulator
                ::fill(dataY, dataY+HEIGHT, 0);

                // call our  function
                multiplier(dataX, dataY, dataZ, HEIGHT, WIDTH, block );
            }
            
            double run_time = timer();
            
            timer_min = std::min(timer_min,run_time);
            timer_max = std::max(timer_max,run_time);
            timer_sum += run_time;
        }

        double timer_average = timer_sum / run_count;
        
        // items * iterations != actual ops, but still a good measure
        double millions = ((double)(HEIGHT*WIDTH) * iterations_per_run) / 1000000.0;
        double perf = millions / timer_min;
    
        if (perf > best_perf) {
            best_perf = perf;
            best_perfA = block;
        }
        
        printf("%s, %d, %.6f, %.6f, %.6f, %.2f\n",
                desc.c_str(),
                block,
                timer_min,
                timer_average,
                timer_max,
                perf );

        // reset the test counter
        current_test = 0;
    }
    
    iterations = saved_iterations;

    printf("Best %s perf %.2f at A=%d\n\n", desc.c_str(), best_perf, best_perfA );
}

/******************************************************************************/

// test/plot cache performance for two different block sizes

template <typename T, typename MM>
void plot_one_cacheblock_size2D(const int limitA, const int limitB, MM multiplier, const std::string label) {
    
    std::string myTypeName( getTypeName<T>() );
    
    std::string desc = label + " " + myTypeName;

    // if these are stack arrays, MSVC silently exits the app
    // while all other compilers work without a problem
    std::unique_ptr<T> dX( new T[ WIDTH ] );
    std::unique_ptr<T> dY( new T[ HEIGHT ] );
    std::unique_ptr<T> dZ( new T[HEIGHT * WIDTH]);
    T *dataX = dX.get();
    T *dataY = dY.get();
    T *dataZ = dZ.get();
    
    
    ::fill( dataX, dataX+(WIDTH), T(init_value));
    ::fill( dataY, dataY+(HEIGHT), T(init_value));
    ::fill( dataZ, dataZ+(HEIGHT*WIDTH), T(init_value));
    
    int saved_iterations = iterations;
    
    iterations /= 16;

    printf("description, sizeA, sizeB, sec_min, sec_avg, sec_max, best_performance\n");
    
    double best_perf = 0.0;
    int best_perfA = 0;
    int best_perfB = 0;
    
    const int run_count = 50;
    const int iterations_per_run = std::min(iterations/run_count, 4);
    
    // block = 1 == just overhead, ignore
    for( int blockA = 2; blockA <= limitA; ++blockA ) {
        for( int blockB = 2; blockB <= limitB; ++blockB ) {

            double timer_min = 999.0;
            double timer_max = 0.0;
            double timer_sum = 0.0;
            
            for (int run = 0; run < run_count; ++run) {
            
                start_timer();

                for( int i = 0; i < iterations_per_run; ++i) {

                    // zero the accumulator
                    ::fill(dataY, dataY+HEIGHT, 0);

                    // call our  function
                    multiplier(dataX, dataY, dataZ, HEIGHT, WIDTH, blockA, blockB );
                }
                
                double run_time = timer();
                
                timer_min = std::min(timer_min,run_time);
                timer_max = std::max(timer_max,run_time);
                timer_sum += run_time;
            }

            double timer_average = timer_sum / run_count;
            
            // items * iterations != actual ops, but still a good measure
            double millions = ((double)(HEIGHT*WIDTH) * iterations_per_run) / 1000000.0;
            double perf = millions / timer_min;
            
            if (perf > best_perf) {
                best_perf = perf;
                best_perfA = blockA;
                best_perfB = blockB;
            }

            printf("%s, %d, %d, %.6f, %.6f, %.6f, %.2f\n",
                    desc.c_str(),
                    blockA,
                    blockB,
                    timer_min,
                    timer_average,
                    timer_max,
                    perf);

            // reset the test counter
            current_test = 0;
        }
    }
    
    iterations = saved_iterations;

    printf("Best %s perf %.2f at A=%d, B=%d\n\n", desc.c_str(), best_perf, best_perfA, best_perfB);

}

/******************************************************************************/

template <typename T>
void plot_cacheblock_sizes() {

    plot_one_cacheblock_size<T>( 200, matvecmul_IJ_blocked1<T>(), "IJBlock1" ); // blockSize1
    plot_one_cacheblock_size<T>( 700, matvecmul_JI_blocked1<T>(), "JIBlock1" ); // blockSize3
    plot_one_cacheblock_size<T>( 200, matvecmul_IJ_blocked_unrolled1<T>(), "IJBlockUnroll1" ); // blockSize4
    plot_one_cacheblock_size<T>( 700, matvecmul_JI_blocked_unrolled1<T>(), "JIBlockUnroll1" ); // blockSize5
    plot_one_cacheblock_size<T>( 700, matvecmul_JI_blocked_unrolled1A<T>(), "JIBlockUnroll1A" ); // blockSize5

    plot_one_cacheblock_size2D<T>( 300, 300, matvecmul_IJ_blocked2<T>(), "IJBlock2" ); // blockSize6
    plot_one_cacheblock_size2D<T>( 300, 300, matvecmul_JI_blocked2<T>(), "JIBlock2" ); // blockSize7
    plot_one_cacheblock_size2D<T>( 300, 300, matvecmul_IJ_blocked_unrolled2<T>(), "IJBlockUnroll2" ); // blockSize8
    plot_one_cacheblock_size2D<T>( 300, 300, matvecmul_IJ_blocked_unrolled2A<T>(), "IJBlockUnroll2A" ); // blockSize8
    plot_one_cacheblock_size2D<T>( 300, 300, matvecmul_JI_blocked_unrolled2<T>(), "JIBlockUnroll2" ); // blockSize9

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
    std::unique_ptr<T> dX( new T[ WIDTH ] );
    std::unique_ptr<T> dY( new T[ HEIGHT ] );
    std::unique_ptr<T> dZ( new T[HEIGHT * WIDTH]);
    T *dataX = dX.get();
    T *dataY = dY.get();
    T *dataZ = dZ.get();


    // verify results (unit test)
    verify_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH );
    
    ::fill( dataX, dataX+(WIDTH), T(init_value));
    ::fill( dataY, dataY+(HEIGHT), T(init_value));
    ::fill( dataZ, dataZ+(HEIGHT*WIDTH), T(init_value));

    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ<T>(), myTypeName + " matrix vector product IJ" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI<T>(), myTypeName + " matrix vector product JI" );

    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_temp<T>(), myTypeName + " matrix vector product IJ temp" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_temp1<T>(), myTypeName + " matrix vector product IJ temp1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_temp<T>(), myTypeName + " matrix vector product JI temp" );

    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_unrolled<T>(), myTypeName + " matrix vector product IJ unrolled" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_unrolled1<T>(), myTypeName + " matrix vector product IJ unrolled1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_unrolled2<T>(), myTypeName + " matrix vector product IJ unrolled2" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_unrolled<T>(), myTypeName + " matrix vector product JI unrolled" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_unrolled1<T>(), myTypeName + " matrix vector product JI unrolled1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_unrolled2<T>(), myTypeName + " matrix vector product JI unrolled2" );

    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_blocked1<T>(), myTypeName + " matrix vector product IJ blocked1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_blocked1<T>(), myTypeName + " matrix vector product JI blocked1" );
    
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_blocked2<T>(), myTypeName + " matrix vector product IJ blocked2" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_blocked2<T>(), myTypeName + " matrix vector product JI blocked2" );
    
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_blocked_unrolled1<T>(), myTypeName + " matrix vector product IJ blocked unrolled1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_blocked_unrolled1<T>(), myTypeName + " matrix vector product JI blocked unrolled1" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_blocked_unrolled1A<T>(), myTypeName + " matrix vector product JI blocked unrolled1A" );
    
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_blocked_unrolled2<T>(), myTypeName + " matrix vector product IJ blocked unrolled2" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_IJ_blocked_unrolled2A<T>(), myTypeName + " matrix vector product IJ blocked unrolled2A" );
    test_matvecmul( dataX, dataY, dataZ, HEIGHT, WIDTH, matvecmul_JI_blocked_unrolled2<T>(), myTypeName + " matrix vector product JI blocked unrolled2" );

    std::string temp1( myTypeName + " matrix vector product" );
    summarize( temp1.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );

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


#if 0
// used to determine best cacheblock values -- VERY SLOW (takes days)
    plot_cacheblock_sizes<int32_t>();
    plot_cacheblock_sizes<float>();
    plot_cacheblock_sizes<double>();
#endif


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
