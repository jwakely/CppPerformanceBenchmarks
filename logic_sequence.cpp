/*
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various idioms for calculating the logic reductions of a sequence


Assumptions:
    1) The compiler will optimize logic sequence operations.
    
    2) The compiler may recognize ineffecient logic sequence idioms and substitute efficient methods.



TODO - logic on float comparisons?

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <string>
#include <deque>
#include <memory>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 5000000;


// 4000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 57;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/
/******************************************************************************/

template<typename T>
inline void check_equal(T result, const std::string &label) {
    if ( result != T(init_value) )
        printf("test %s failed\n", label.c_str() );
}
/******************************************************************************/

inline void check_logical(bool result, const std::string &label) {
    if ( result != true )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial loop
template <typename T>
T bit_and1(const T* first, const int count, T initial) {
    T product( initial );
    
    for (int j = 0; j < count; ++j) {
        product &= first[j];
    }
    
    return product;
}

/******************************************************************************/

// baseline - a trivial loop
template <typename T>
T bit_or1(const T* first, const int count, T initial) {
    T product( initial );
    
    for (int j = 0; j < count; ++j) {
        product |= first[j];
    }
    
    return product;
}

/******************************************************************************/

// baseline - a trivial loop
template <typename T>
T bit_xor1(const T* first, const int count, T initial) {
    T product( initial );
    
    for (int j = 0; j < count; ++j) {
        product ^= first[j];
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
T bit_and2(const T* first, const int count, T initial) {
    T product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product &= *current++;
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
T bit_or2(const T* first, const int count, T initial) {
    T product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product |= *current++;
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
T bit_xor2(const T* first, const int count, T initial) {
    T product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product ^= *current++;
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
T bit_and3(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product &= first[j+0];
        product &= first[j+1];
    }
    
    for (; j < count; ++j) {
        product &= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
T bit_or3(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product |= first[j+0];
        product |= first[j+1];
    }
    
    for (; j < count; ++j) {
        product |= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
T bit_xor3(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product ^= first[j+0];
        product ^= first[j+1];
    }
    
    for (; j < count; ++j) {
        product ^= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
T bit_and4(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product &= first[j+0];
        product &= first[j+1];
        product &= first[j+2];
        product &= first[j+3];
    }
    
    for (; j < count; ++j) {
        product &= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
T bit_or4(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product |= first[j+0];
        product |= first[j+1];
        product |= first[j+2];
        product |= first[j+3];
    }
    
    for (; j < count; ++j) {
        product |= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
T bit_xor4(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product ^= first[j+0];
        product ^= first[j+1];
        product ^= first[j+2];
        product ^= first[j+3];
    }
    
    for (; j < count; ++j) {
        product ^= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
T bit_and5(const T* first, const int count, T initial) {
    T product( initial );
    T product1( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product &= first[j+0];
        product1 &= first[j+1];
    }
    
    for (; j < count; ++j) {
        product &= first[j];
    }
    
    product &= product1;
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
T bit_or5(const T* first, const int count, T initial) {
    T product( initial );
    T product1( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product |= first[j+0];
        product1 |= first[j+1];
    }
    
    for (; j < count; ++j) {
        product |= first[j];
    }
    
    product |= product1;
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
T bit_xor5(const T* first, const int count, T initial) {
    T product( initial );
    T product1( 0 );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product ^= first[j+0];
        product1 ^= first[j+1];
    }
    
    for (; j < count; ++j) {
        product ^= first[j];
    }
    
    product ^= product1;
    
    return product;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
T bit_and6(const T* first, const int count, T initial) {
    T product( initial );
    T product1( initial );
    T product2( initial );
    T product3( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product &= first[j+0];
        product1 &= first[j+1];
        product2 &= first[j+2];
        product3 &= first[j+3];
    }
    
    for (; j < count; ++j) {
        product &= first[j];
    }
    
    product &= product1 & product2 & product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
T bit_or6(const T* first, const int count, T initial) {
    T product( initial );
    T product1( initial );
    T product2( initial );
    T product3( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product |= first[j+0];
        product1 |= first[j+1];
        product2 |= first[j+2];
        product3 |= first[j+3];
    }
    
    for (; j < count; ++j) {
        product |= first[j];
    }
    
    product |= product1 | product2 | product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
T bit_xor6(const T* first, const int count, T initial) {
    T product( initial );
    T product1( 0 );
    T product2( 0 );
    T product3( 0 );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product ^= first[j+0];
        product1 ^= first[j+1];
        product2 ^= first[j+2];
        product3 ^= first[j+3];
    }
    
    for (; j < count; ++j) {
        product ^= first[j];
    }
    
    product ^= product1 ^ product2 ^ product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
T bit_and7(const T* first, const int count, T initial) {
    T product[4] = { initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] &= first[j+0];
        product[1] &= first[j+1];
        product[2] &= first[j+2];
        product[3] &= first[j+3];
    }
    
    for (; j < count; ++j) {
        product[0] &= first[j];
    }
    
    product[0] &= product[1] & product[2] & product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
T bit_or7(const T* first, const int count, T initial) {
    T product[4] = { initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] |= first[j+0];
        product[1] |= first[j+1];
        product[2] |= first[j+2];
        product[3] |= first[j+3];
    }
    
    for (; j < count; ++j) {
        product[0] |= first[j];
    }
    
    product[0] |= product[1] | product[2] | product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
T bit_xor7(const T* first, const int count, T initial) {
    T product[4] = { initial,0,0,0 };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] ^= first[j+0];
        product[1] ^= first[j+1];
        product[2] ^= first[j+2];
        product[3] ^= first[j+3];
    }
    
    for (; j < count; ++j) {
        product[0] ^= first[j];
    }
    
    product[0] ^= product[1] ^ product[2] ^ product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
T bit_and8(const T* first, const int count, T initial) {
    T product[8] = { initial,initial,initial,initial, initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] &= first[j+0];
        product[1] &= first[j+1];
        product[2] &= first[j+2];
        product[3] &= first[j+3];
        product[4] &= first[j+4];
        product[5] &= first[j+5];
        product[6] &= first[j+6];
        product[7] &= first[j+7];
    }
    
    for (; j < count; ++j) {
        product[0] &= first[j];
    }
    
    product[0] &= product[1] & product[2] & product[3];
    product[4] &= product[5] & product[6] & product[7];
    product[0] &= product[4];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
T bit_or8(const T* first, const int count, T initial) {
    T product[8] = { initial,initial,initial,initial, initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] |= first[j+0];
        product[1] |= first[j+1];
        product[2] |= first[j+2];
        product[3] |= first[j+3];
        product[4] |= first[j+4];
        product[5] |= first[j+5];
        product[6] |= first[j+6];
        product[7] |= first[j+7];
    }
    
    for (; j < count; ++j) {
        product[0] |= first[j];
    }
    
    product[0] |= product[1] | product[2] | product[3];
    product[4] |= product[5] | product[6] | product[7];
    product[0] |= product[4];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
T bit_xor8(const T* first, const int count, T initial) {
    T product[8] = { initial,0,0,0, 0,0,0,0 };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] ^= first[j+0];
        product[1] ^= first[j+1];
        product[2] ^= first[j+2];
        product[3] ^= first[j+3];
        product[4] ^= first[j+4];
        product[5] ^= first[j+5];
        product[6] ^= first[j+6];
        product[7] ^= first[j+7];
    }
    
    for (; j < count; ++j) {
        product[0] ^= first[j];
    }
    
    product[0] ^= product[1] ^ product[2] ^ product[3];
    product[4] ^= product[5] ^ product[6] ^ product[7];
    product[0] ^= product[4];
    
    return product[0];
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial loop
template <typename T>
bool logic_and1(const T* first, const int count, bool initial) {
    bool product( initial );
    
    for (int j = 0; j < count; ++j) {
        product = product && (first[j] != 0);
    }
    
    return product;
}

/******************************************************************************/

// baseline - a trivial loop
template <typename T>
bool logic_or1(const T* first, const int count, bool initial) {
    bool product( initial );
    
    for (int j = 0; j < count; ++j) {
        product = (first[j] != 0) || product;   // short circuit can mess this up if reversed
    }
    
    return product;
}

/******************************************************************************/

// A trivial loop, optimizezd by examining dependencies on the result
// After the return value becomes true, it will remain true, so we can return immediately.
template <typename T>
bool logic_or_opt(const T* first, const int count, bool initial) {
    bool product( initial );
    
    for (int j = 0; j < count && !product; ++j) {
        product = (first[j] != 0) || product;
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
bool logic_and2(const T* first, const int count, bool initial) {
    bool product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product = product && ((*current++) != 0);
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
bool logic_or2(const T* first, const int count, bool initial) {
    bool product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product = ((*current++) != 0) || product;   // short circuit can mess this up if reversed
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
bool logic_and3(const T* first, const int count, bool initial) {
    bool product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product = product && (first[j+0] != 0);
        product = product && (first[j+1] != 0);
    }
    
    for (; j < count; ++j) {
        product = product && (first[j] != 0);
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
bool logic_or3(const T* first, const int count, bool initial) {
    bool product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product = (first[j+0] != 0) || product;
        product = (first[j+1] != 0) || product;
    }
    
    for (; j < count; ++j) {
        product = (first[j] != 0) || product;
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
bool logic_and4(const T* first, const int count, bool initial) {
    bool product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product = product && (first[j+0] != 0);
        product = product && (first[j+1] != 0);
        product = product && (first[j+2] != 0);
        product = product && (first[j+3] != 0);
    }
    
    for (; j < count; ++j) {
        product = product && (first[j] != 0);
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
bool logic_or4(const T* first, const int count, bool initial) {
    bool product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product = (first[j+0] != 0) || product;
        product = (first[j+1] != 0) || product;
        product = (first[j+2] != 0) || product;
        product = (first[j+3] != 0) || product;
    }
    
    for (; j < count; ++j) {
        product = (first[j] != 0) || product;
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
bool logic_and5(const T* first, const int count, bool initial) {
    bool product( initial );
    bool product1( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product = product && (first[j+0] != 0);
        product1 = product1 && (first[j+1] != 0);
    }
    
    for (; j < count; ++j) {
        product = product && (first[j] != 0);
    }
    
    product = product && product1;
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
bool logic_or5(const T* first, const int count, bool initial) {
    bool product( initial );
    bool product1( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product = (first[j+0] != 0) || product;
        product1 = (first[j+1] != 0) || product1;
    }
    
    for (; j < count; ++j) {
        product = (first[j+0] != 0) || product;
    }
    
    product = product || product1;
    
    return product;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
bool logic_and6(const T* first, const int count, bool initial) {
    bool product( initial );
    bool product1( initial );
    bool product2( initial );
    bool product3( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product = product && (first[j+0] != 0);
        product1 = product1 && (first[j+1] != 0);
        product2 = product2 && (first[j+2] != 0);
        product3 = product3 && (first[j+3] != 0);
    }
    
    for (; j < count; ++j) {
        product = product && (first[j] != 0);
    }
    
    product = product && product1 && product2 && product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
bool logic_or6(const T* first, const int count, bool initial) {
    bool product( initial );
    bool product1( initial );
    bool product2( initial );
    bool product3( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product = (first[j+0] != 0) || product;
        product1 = (first[j+1] != 0) || product1;
        product2 = (first[j+2] != 0) || product2;
        product3 = (first[j+3] != 0) || product3;
    }
    
    for (; j < count; ++j) {
        product = (first[j+0] != 0) || product;
    }
    
    product = product || product1 || product2 || product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
bool logic_and7(const T* first, const int count, bool initial) {
    bool product[4] = { initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] = product[0] && (first[j+0] != 0);
        product[1] = product[1] && (first[j+1] != 0);
        product[2] = product[2] && (first[j+2] != 0);
        product[3] = product[3] && (first[j+3] != 0);
    }
    
    for (; j < count; ++j) {
        product[0] = product[0] && (first[j+0] != 0);
    }
    
    product[0] = product[0] && product[1] && product[2] && product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
bool logic_or7(const T* first, const int count, bool initial) {
    bool product[4] = { initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] = (first[j+0] != 0) || product[0];
        product[1] = (first[j+1] != 0) || product[1];
        product[2] = (first[j+2] != 0) || product[2];
        product[3] = (first[j+3] != 0) || product[3];
    }
    
    for (; j < count; ++j) {
        product[0] = (first[j+0] != 0) || product[0];
    }
    
    product[0] = product[0] || product[1] || product[2] || product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
bool logic_and8(const T* first, const int count, bool initial) {
    bool product[8] = { initial,initial,initial,initial, initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] = product[0] && (first[j+0] != 0);
        product[1] = product[1] && (first[j+1] != 0);
        product[2] = product[2] && (first[j+2] != 0);
        product[3] = product[3] && (first[j+3] != 0);
        product[4] = product[4] && (first[j+4] != 0);
        product[5] = product[5] && (first[j+5] != 0);
        product[6] = product[6] && (first[j+6] != 0);
        product[7] = product[7] && (first[j+7] != 0);
    }
    
    for (; j < count; ++j) {
        product[0] = product[0] && (first[j+0] != 0);
    }
    
    product[0] = product[0] && product[1] && product[2] && product[3];
    product[4] = product[4] && product[5] && product[6] && product[7];
    product[0] = product[0] && product[4];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
bool logic_or8(const T* first, const int count, bool initial) {
    bool product[8] = { initial,initial,initial,initial, initial,initial,initial,initial };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] = (first[j+0] != 0) || product[0];
        product[1] = (first[j+1] != 0) || product[1];
        product[2] = (first[j+2] != 0) || product[2];
        product[3] = (first[j+3] != 0) || product[3];
        product[4] = (first[j+4] != 0) || product[4];
        product[5] = (first[j+5] != 0) || product[5];
        product[6] = (first[j+6] != 0) || product[6];
        product[7] = (first[j+7] != 0) || product[7];
    }
    
    for (; j < count; ++j) {
        product[0] = (first[j+0] != 0) || product[0];
    }
    
    product[0] = product[0] || product[1] || product[2] || product[3];
    product[4] = product[4] || product[5] || product[6] || product[7];
    product[0] = product[0] || product[4];
    
    return product[0];
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename MM>
void testOneFunction_equal(const T* first, const int count, MM func, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
    
        T result = func(first,count,T(init_value));
        
        check_equal( result, label );
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename MM>
void testOneFunction_logical(const T* first, const int count, bool start, MM func, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
    
        bool result = func(first,count, start );
        
        check_logical( result, label );
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );

    std::unique_ptr<T> data_unique( new T[ SIZE ] );
    T *data = data_unique.get();
    
    fill(data, data+SIZE, T(init_value));
    

    testOneFunction_equal( data, SIZE, bit_and1<T>, myTypeName + " bit_and sequence1" );
    testOneFunction_equal( data, SIZE, bit_and2<T>, myTypeName + " bit_and sequence2" );
    testOneFunction_equal( data, SIZE, bit_and3<T>, myTypeName + " bit_and sequence3" );
    testOneFunction_equal( data, SIZE, bit_and4<T>, myTypeName + " bit_and sequence4" );
    testOneFunction_equal( data, SIZE, bit_and5<T>, myTypeName + " bit_and sequence5" );
    testOneFunction_equal( data, SIZE, bit_and6<T>, myTypeName + " bit_and sequence6" );
    testOneFunction_equal( data, SIZE, bit_and7<T>, myTypeName + " bit_and sequence7" );
    testOneFunction_equal( data, SIZE, bit_and8<T>, myTypeName + " bit_and sequence8" );
    
    std::string temp1( myTypeName + " bit_and sequence" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    testOneFunction_equal( data, SIZE, bit_or1<T>, myTypeName + " bit_or sequence1" );
    testOneFunction_equal( data, SIZE, bit_or2<T>, myTypeName + " bit_or sequence2" );
    testOneFunction_equal( data, SIZE, bit_or3<T>, myTypeName + " bit_or sequence3" );
    testOneFunction_equal( data, SIZE, bit_or4<T>, myTypeName + " bit_or sequence4" );
    testOneFunction_equal( data, SIZE, bit_or5<T>, myTypeName + " bit_or sequence5" );
    testOneFunction_equal( data, SIZE, bit_or6<T>, myTypeName + " bit_or sequence6" );
    testOneFunction_equal( data, SIZE, bit_or7<T>, myTypeName + " bit_or sequence7" );
    testOneFunction_equal( data, SIZE, bit_or8<T>, myTypeName + " bit_or sequence8" );
    
    std::string temp2( myTypeName + " bit_or sequence" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    testOneFunction_equal( data, SIZE, bit_xor1<T>, myTypeName + " bit_xor sequence1" );
    testOneFunction_equal( data, SIZE, bit_xor2<T>, myTypeName + " bit_xor sequence2" );
    testOneFunction_equal( data, SIZE, bit_xor3<T>, myTypeName + " bit_xor sequence3" );
    testOneFunction_equal( data, SIZE, bit_xor4<T>, myTypeName + " bit_xor sequence4" );
    testOneFunction_equal( data, SIZE, bit_xor5<T>, myTypeName + " bit_xor sequence5" );
    testOneFunction_equal( data, SIZE, bit_xor6<T>, myTypeName + " bit_xor sequence6" );
    testOneFunction_equal( data, SIZE, bit_xor7<T>, myTypeName + " bit_xor sequence7" );
    testOneFunction_equal( data, SIZE, bit_xor8<T>, myTypeName + " bit_xor sequence8" );
    
    std::string temp3( myTypeName + " bit_xor sequence" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    testOneFunction_logical( data, SIZE, true, logic_and1<T>, myTypeName + " logical_and sequence1" );
    testOneFunction_logical( data, SIZE, true, logic_and2<T>, myTypeName + " logical_and sequence2" );
    testOneFunction_logical( data, SIZE, true, logic_and3<T>, myTypeName + " logical_and sequence3" );
    testOneFunction_logical( data, SIZE, true, logic_and4<T>, myTypeName + " logical_and sequence4" );
    testOneFunction_logical( data, SIZE, true, logic_and5<T>, myTypeName + " logical_and sequence5" );
    testOneFunction_logical( data, SIZE, true, logic_and6<T>, myTypeName + " logical_and sequence6" );
    testOneFunction_logical( data, SIZE, true, logic_and7<T>, myTypeName + " logical_and sequence7" );
    testOneFunction_logical( data, SIZE, true, logic_and8<T>, myTypeName + " logical_and sequence8" );
    
    std::string temp4( myTypeName + " logical_and sequence" );
    summarize( temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    testOneFunction_logical( data, SIZE, false, logic_or1<T>, myTypeName + " logical_or sequence1" );
    testOneFunction_logical( data, SIZE, false, logic_or_opt<T>, myTypeName + " logical_or sequence optimal" );
    testOneFunction_logical( data, SIZE, false, logic_or2<T>, myTypeName + " logical_or sequence2" );
    testOneFunction_logical( data, SIZE, false, logic_or3<T>, myTypeName + " logical_or sequence3" );
    testOneFunction_logical( data, SIZE, false, logic_or4<T>, myTypeName + " logical_or sequence4" );
    testOneFunction_logical( data, SIZE, false, logic_or5<T>, myTypeName + " logical_or sequence5" );
    testOneFunction_logical( data, SIZE, false, logic_or6<T>, myTypeName + " logical_or sequence6" );
    testOneFunction_logical( data, SIZE, false, logic_or7<T>, myTypeName + " logical_or sequence7" );
    testOneFunction_logical( data, SIZE, false, logic_or8<T>, myTypeName + " logical_or sequence8" );
    
    std::string temp5( myTypeName + " logical_or sequence" );
    summarize( temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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


    // make sure the iteration count is even, for xor testing
    iterations = (iterations + 1) & ~1;
    
    TestOneType<uint8_t>();
    TestOneType<int8_t>();
    
    TestOneType<uint16_t>();
    TestOneType<int16_t>();
    
    iterations /= 4;
    iterations = (iterations + 1) & ~1;
    TestOneType<uint32_t>();
    TestOneType<int32_t>();
    
    iterations /= 2;
    iterations = (iterations + 1) & ~1;
    TestOneType<uint64_t>();
    TestOneType<int64_t>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
