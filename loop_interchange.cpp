/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to changing the order of nested loops.

Assumptions:

    1) The compiler will change the order of loop nests to optimize memory access patterns and thus performance.
        aka: loop interchange, loop_reordering
        This should apply to all types of loops (for,while,do,goto) after loop normalization.
    
    2) The compiler will recognize loop access patterns for 2D and 3D arrays [y][x], [z][y][x], etc.
    
    3) The compiler will recognize loop access patterns for 1D arrays accessed by  [y*ystep + x], [ y*ystep + x*xstep + c ], etc.

    4) The compiler will pick the optimal loop ordering when there are multiple possibilities.
        The selection algorithm must take cache behavior into account.
        The selection algorithm must also take into account other optimization opportunities,
            such as scalar replacement and loop invariant code motion.

    5) The compiler will recognize loop access patterns in higher dimensions as well.



*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400;

// 16 million items (16-128 Meg), intended to be larger than L2 cache on common CPUs
const int WIDTH = 4000;
const int HEIGHT = 4000;

const int SIZE = HEIGHT*WIDTH;

// 18 million items (18-144 Meg), intended to be larger than L2 cache on common CPUs
const int sizeX = 198;
const int sizeY = 301;
const int sizeZ = 303;

// 32 million items (32-256 Meg), intended to be larger than L2 cache on common CPUs
const int sizeA = 75;
const int sizeB = 79;
const int sizeC = 74;
const int sizeD = 76;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

// because we need labels to stick around for a bit
static std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
  T temp = (T)(SIZE * (T)init_value);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, expected " << temp << " got " << result << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_channels(T result, size_t HEIGHT, size_t WIDTH, size_t channels, const std::string &label) {
  T temp = (T)(HEIGHT*WIDTH*channels * (T)init_value);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, expected " << temp << " got " << result << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_4D(T result, const std::string &label) {
  T temp = (T)(sizeA*sizeB*sizeC*sizeD * (T)init_value);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, expected " << temp << " got " << result << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_3D(T first[sizeZ][sizeY][sizeX], const std::string &label) {

    int x, y, z;
  
    for (z = 1; z < sizeZ; ++z) {
        T temp = (z+1) * T(init_value);
        
        for (y = 0; y < sizeY; ++y) {
            for (x = 0; x < sizeX; ++x) {
                T value = first[z][y][x];
                if (!tolerance_equal<T>(value,temp))
                    std::cout << "test " << label << " failed, expected " << temp << " got " << value << std::endl;
            }
        }
    }

}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_2D_opt(const T first[HEIGHT][WIDTH], int rows, int cols, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    for (y = 0; y < rows; ++y) {
        auto firstRow = first[y];
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            resultRow += firstRow[x];
        }
        result += resultRow;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// also testing loop invariant code motion
template <typename T >
void test_loop_2D_opt2(const T first[HEIGHT][WIDTH], int rows, int cols, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    for (y = 0; y < rows; ++y) {
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            resultRow += first[y][x];
        }
        result += resultRow;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_rev(const T first[HEIGHT][WIDTH], int rows, int cols, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    for (x = 0; x < cols; ++x) {
        T resultRow = 0;
        for (y = 0; y < rows; ++y) {
            resultRow += first[y][x];
        }
        result += resultRow;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_opt(const T* first, int rows, int cols, int rowStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    auto firstRow = first;
    for (y = 0; y < rows; ++y) {
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            resultRow += firstRow[ x ];    // first[ y * rowStep + x ];
        }
        result += resultRow;
        firstRow += rowStep;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_opt2(const T* first, int rows, int cols, int rowStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    for (y = 0; y < rows; ++y) {
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            resultRow += first[ (y * rowStep) + x ];
        }
        result += resultRow;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_rev(const T* first, int rows, int cols, int rowStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y;
    
    for (x = 0; x < cols; ++x) {
        T resultRow = 0;
        for (y = 0; y < rows; ++y) {
            resultRow += first[ y*rowStep + x ];
        }
        result += resultRow;
    }
    
    check_sum<T>(result,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// also testing loop invariant code motion
// and small loop unrolling
template <typename T >
void test_loop_2D_flat_chan_opt(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    if (channels == 3)
        for (y = 0; y < rows; ++y) {
            const T* firstRow = first + y*rowStep;
            T resultRow = 0;
            for (x = 0; x < cols; ++x) {
                const T* pixel = firstRow + x*colStep;
                resultRow += pixel[ 0 ];    // first[ y * rowStep + x*channels + c ];
                resultRow += pixel[ 1 ];    // first[ y * rowStep + x*channels + c ];
                resultRow += pixel[ 2 ];    // first[ y * rowStep + x*channels + c ];
            }
            result += resultRow;
        }
    else if (channels == 4)
        for (y = 0; y < rows; ++y) {
            const T* firstRow = first + y*rowStep;
            T resultRow = 0;
            for (x = 0; x < cols; ++x) {
                const T* pixel = firstRow + x*colStep;
                resultRow += pixel[ 0 ];    // first[ y * rowStep + x*channels + c ];
                resultRow += pixel[ 1 ];    // first[ y * rowStep + x*channels + c ];
                resultRow += pixel[ 2 ];    // first[ y * rowStep + x*channels + c ];
                resultRow += pixel[ 3 ];    // first[ y * rowStep + x*channels + c ];
            }
            result += resultRow;
        }
    else
        for (y = 0; y < rows; ++y) {
            const T* firstRow = first + y*rowStep;
            T resultRow = 0;
            for (x = 0; x < cols; ++x) {
                const T* pixel = firstRow + x*colStep;
                for (c = 0; c < channels; ++c) {
                    resultRow += pixel[ c ];    // first[ y * rowStep + x*channels + c ];
                }
            }
            result += resultRow;
        }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// also testing loop invariant code motion
template <typename T >
void test_loop_2D_flat_chan_opt2(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (y = 0; y < rows; ++y) {
        auto firstRow = first + y*rowStep;
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            auto pixel = firstRow + x*colStep;
            for (c = 0; c < channels; ++c) {
                resultRow += pixel[ c ];    // first[ y * rowStep + x*channels + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// also testing loop invariant code motion
template <typename T >
void test_loop_2D_flat_chan_opt3(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (y = 0; y < rows; ++y) {
        const T* firstRow = first + y*rowStep;
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            for (c = 0; c < channels; ++c) {
                resultRow += firstRow[ (x*colStep) + c ];    // first[ y * rowStep + x*channels + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// also testing loop invariant code motion
template <typename T >
void test_loop_2D_flat_chan_opt4(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (y = 0; y < rows; ++y) {
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            for (c = 0; c < channels; ++c) {
                resultRow += first[ (y*rowStep) + (x*colStep) + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_chan_revCXY(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (c = 0; c < channels; ++c) {
        T resultRow = 0;
        for (x = 0; x < cols; ++x) {
            for (y = 0; y < rows; ++y) {
                resultRow += first[ y * rowStep + x*colStep + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_chan_revCYX(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (c = 0; c < channels; ++c) {
        T resultRow = 0;
        for (y = 0; y < rows; ++y) {
            for (x = 0; x < cols; ++x) {
                resultRow += first[ y * rowStep + x*colStep + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_chan_revXYC(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (x = 0; x < cols; ++x) {
        T resultRow = 0;
        for (y = 0; y < rows; ++y) {
            for (c = 0; c < channels; ++c) {
                resultRow += first[ y * rowStep + x*colStep + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_2D_flat_chan_revXCY(const T* first, int rows, int cols, int channels, int rowStep, int colStep, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int x, y, c;
    
    for (x = 0; x < cols; ++x) {
        T resultRow = 0;
        for (c = 0; c < channels; ++c) {
            for (y = 0; y < rows; ++y) {
                resultRow += first[ y * rowStep + x*colStep + c ];
            }
        }
        result += resultRow;
    }
    
    check_sum_channels<T>(result,rows,cols,channels,label);
  }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template < typename T >
void test_loop_3D_ZYX( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (z = 1; z < sizeZ; ++z) {
            for (y = 0; y < sizeY; ++y) {
                for (x = 0; x < sizeX; ++x) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/

template < typename T >
void test_loop_3D_ZXY( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (z = 1; z < sizeZ; ++z) {
            for (x = 0; x < sizeX; ++x) {
                for (y = 0; y < sizeY; ++y) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    
    }    // iterations

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/

template < typename T >
void test_loop_3D_XYZ( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (x = 0; x < sizeX; ++x) {
            for (y = 0; y < sizeY; ++y) {
                for (z = 1; z < sizeZ; ++z) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    
    }    // iterations

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/

template < typename T >
void test_loop_3D_XZY( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (x = 0; x < sizeX; ++x) {
            for (z = 1; z < sizeZ; ++z) {
                for (y = 0; y < sizeY; ++y) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    
    }    // iterations

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/

template < typename T >
void test_loop_3D_YXZ( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (y = 0; y < sizeY; ++y) {
            for (x = 0; x < sizeX; ++x) {
                for (z = 1; z < sizeZ; ++z) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    
    }    // iterations

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/

template < typename T >
void test_loop_3D_YZX( T first[sizeZ][sizeY][sizeX], T second[sizeZ][sizeX], const std::string label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x, y, z;

        for (y = 0; y < sizeY; ++y) {
            for (x = 0; x < sizeX; ++x) {
                for (z = 1; z < sizeZ; ++z) {
                    first[z][y][x] = first[z-1][y][x] + second[z][x];
                }
            }
        }
    
    }    // iterations

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_sum_3D<T>(first,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_4D_ABCD(const T first[sizeA][sizeB][sizeC][sizeD], const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int a, b, c, d;

        for (a = 0; a < sizeA; ++a) {
            for (b = 0; b < sizeB; ++b) {
                T resultRow = 0;
                for (c = 0; c < sizeC; ++c) {
                    for (d = 0; d < sizeD; ++d) {
                        resultRow += first[a][b][c][d];
                    }
                }
                result += resultRow;
            }
        }

        check_sum_4D<T>(result,label);
    }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// some loop invariant code motion
template <typename T >
void test_loop_4D_ABCD2(const T first[sizeA][sizeB][sizeC][sizeD], const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int a, b, c, d;

        for (a = 0; a < sizeA; ++a) {
            auto firstSpace = first[a];
            for (b = 0; b < sizeB; ++b) {
                auto firstPlane = firstSpace[b];
                T resultRow = 0;
                for (c = 0; c < sizeC; ++c) {
                    auto firstRow = firstPlane[c];
                    for (d = 0; d < sizeD; ++d) {
                        resultRow += firstRow[d];
                    }
                }
                result += resultRow;
            }
        }

        check_sum_4D<T>(result,label);
    }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T >
void test_loop_4D_DCBA(const T first[sizeA][sizeB][sizeC][sizeD], const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int a, b, c, d;

        for (d = 0; d < sizeD; ++d) {
            for (c = 0; c < sizeC; ++c) {
                T resultRow = 0;
                for (b = 0; b < sizeB; ++b) {
                    for (a = 0; a < sizeA; ++a) {
                        resultRow += first[a][b][c][d];
                    }
                }
                result += resultRow;
            }
        }

        check_sum_4D<T>(result,label);
    }

    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T, size_t firstDim, size_t secondDim >
void fill2D( T array[firstDim][secondDim], const T value )
{
    for (size_t y = 0; y < firstDim; ++y)
        for (size_t x = 0; x < secondDim; ++x)
            array[y][x] = value;
}

/******************************************************************************/

template< typename T, size_t firstDim, size_t secondDim, size_t thirdDim >
void fill3D( T array[firstDim][secondDim][thirdDim], const T value )
{
    for (size_t y = 0; y < firstDim; ++y)
        for (size_t x = 0; x < secondDim; ++x)
            for (size_t z = 0; z < thirdDim; ++z)
                array[y][x][z] = value;
}

/******************************************************************************/

template< typename T, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim >
void fill4D( T array[firstDim][secondDim][thirdDim][fourthDim], const T value )
{
    for (size_t y = 0; y < firstDim; ++y)
        for (size_t x = 0; x < secondDim; ++x)
            for (size_t z = 0; z < thirdDim; ++z)
                for (size_t k = 0; k < fourthDim; ++k)
                    array[y][x][z][k] = value;
}

/******************************************************************************/
/******************************************************************************/

template< typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();


    auto data32 = new T[HEIGHT][WIDTH];
    fill2D<T,HEIGHT,WIDTH>( data32, T(init_value) );
    test_loop_2D_opt( data32, HEIGHT, WIDTH, myTypeName + " loop interchange 2D optimal" );
    test_loop_2D_opt2( data32, HEIGHT, WIDTH, myTypeName + " loop interchange 2D optimal2" );
    test_loop_2D_rev( data32, HEIGHT, WIDTH, myTypeName + " loop interchange 2D reversed" );
    
    std::string temp1( myTypeName + " loop interchange 2D" );
    summarize(temp1.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    delete[] data32;


    auto data32_flat = new T[HEIGHT*WIDTH];
    fill( data32_flat, data32_flat+(HEIGHT*WIDTH), T(init_value));
    test_loop_2D_flat_opt( data32_flat, HEIGHT, WIDTH, WIDTH, myTypeName + " loop interchange 2D flat optimal" );
    test_loop_2D_flat_opt2( data32_flat, HEIGHT, WIDTH, WIDTH, myTypeName + " loop interchange 2D flat optimal2" );
    test_loop_2D_flat_rev( data32_flat, HEIGHT, WIDTH, WIDTH, myTypeName + " loop interchange 2D lat reversed" );
    
    std::string temp2( myTypeName + " loop interchange 2D flat" );
    summarize(temp2.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_2D_flat_chan_opt( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4,  myTypeName + " loop interchange 2D flat 4channels optimal" );
    test_loop_2D_flat_chan_opt2( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels optimal2" );
    test_loop_2D_flat_chan_opt3( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels optimal3" );
    test_loop_2D_flat_chan_opt4( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels optimal4" );
    test_loop_2D_flat_chan_revCYX( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels reversedCYX" );
    test_loop_2D_flat_chan_revXYC( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels reversedXYC" );
    test_loop_2D_flat_chan_revXCY( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels reversedXCY" );
    test_loop_2D_flat_chan_revCXY( data32_flat, HEIGHT, WIDTH/4, 4, WIDTH, 4, myTypeName + " loop interchange 2D flat 4channels reversedCXY" );
    
    std::string temp3( myTypeName + " loop interchange flat 4channels" );
    summarize(temp3.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_2D_flat_chan_opt( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4,  myTypeName + " loop interchange 2D flat 3channels optimal" );
    test_loop_2D_flat_chan_opt2( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels optimal2" );
    test_loop_2D_flat_chan_opt3( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels optimal3" );
    test_loop_2D_flat_chan_opt4( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels optimal4" );
    test_loop_2D_flat_chan_revCYX( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels reversedCYX" );
    test_loop_2D_flat_chan_revXYC( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels reversedXYC" );
    test_loop_2D_flat_chan_revXCY( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels reversedXCY" );
    test_loop_2D_flat_chan_revCXY( data32_flat, HEIGHT, WIDTH/4, 3, WIDTH, 4, myTypeName + " loop interchange 2D flat 3channels reversedCXY" );
    
    std::string temp4( myTypeName + " loop interchange flat 3channels" );
    summarize(temp4.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    delete[] data32_flat;




    auto data32_3D = new T[sizeZ][sizeY][sizeX];
    auto data32_2D = new T[sizeZ][sizeX];
    fill3D<T, sizeZ, sizeY, sizeX>( data32_3D, T(init_value) );
    fill2D<T, sizeZ, sizeX>( data32_2D, T(init_value) );

    test_loop_3D_ZYX( data32_3D, data32_2D, myTypeName + " loop interchange 3D ZYX" );
    test_loop_3D_ZXY( data32_3D, data32_2D, myTypeName + " loop interchange 3D ZXY" );
    test_loop_3D_YZX( data32_3D, data32_2D, myTypeName + " loop interchange 3D YZX" );
    test_loop_3D_YXZ( data32_3D, data32_2D, myTypeName + " loop interchange 3D YXZ" );
    test_loop_3D_XYZ( data32_3D, data32_2D, myTypeName + " loop interchange 3D XYZ" );
    test_loop_3D_XZY( data32_3D, data32_2D, myTypeName + " loop interchange 3D XZY" );

    std::string temp5( myTypeName + " loop interchange 3D" );
    summarize(temp5.c_str(), (sizeX*sizeY*sizeZ), iterations, kDontShowGMeans, kDontShowPenalty );
    delete[] data32_3D;
    delete[] data32_2D;
    


    auto data32_4D = new T[sizeA][sizeB][sizeC][sizeD];
    fill4D<T, sizeA, sizeB, sizeC, sizeD>( data32_4D, T(init_value) );

    test_loop_4D_ABCD( data32_4D, myTypeName + " loop interchange 4D ABCD" );
    test_loop_4D_ABCD2( data32_4D, myTypeName + " loop interchange 4D ABCD2" );
    test_loop_4D_DCBA( data32_4D, myTypeName + " loop interchange 4D DCBA" );

    std::string temp6( myTypeName + " loop interchange 3D" );
    summarize(temp6.c_str(), (sizeX*sizeY*sizeZ), iterations, kDontShowGMeans, kDontShowPenalty );
    delete[] data32_4D;

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


    // Results are pretty similar across types, so we don't have to test all of them right now.
    // Which is good, because these tests are kinda slow.
    // NOTE - cache thrashing doesn't really care about types ;-)
    
    //TestOneType<int8_t>();
    TestOneType<uint8_t>();
    //TestOneType<int16_t>();
    //TestOneType<uint16_t>();
    TestOneType<int32_t>();
    //TestOneType<uint32_t>();
    //TestOneType<int64_t>();
    //TestOneType<uint64_t>();

    iterations /= 8;
    if (iterations == 0)
        iterations = 1;

    //TestOneType<float>();
    TestOneType<double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
