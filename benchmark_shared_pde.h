/*
    Copyright 2019-2022 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)
    
    Shared source file for numerical pde solution testing, and debugging
*/

#include "benchmark_stdint.hpp"

/******************************************************************************/
/******************************************************************************/

template <typename T>
inline T scaleValue( T value, float factor, int shift ) {
    // adding rounding makes results go off into the weeds
    const T intFactor = T( factor * (1L << shift) );
    return (intFactor * value) >> shift;
}

template <>
inline float scaleValue( float value, float factor, int /* shift */ ) {
    return value * factor;
}

template <>
inline double scaleValue( double value, float factor, int /* shift */ ) {
    return value * factor;
}

template <>
inline long double scaleValue( long double value, float factor, int /* shift */ ) {
    return value * factor;
}

/******************************************************************************/
/******************************************************************************/

template <typename Iterator, typename T>
void fill_step(Iterator first, int count, int step, const T value) {
    for (int i = 0; i < count; ++i) {
        *first = value;
        first += step;
    }
}

/******************************************************************************/

template <typename RandomAccessIterator, typename T>
void fill_rect(RandomAccessIterator first, int rows, int cols, int rowStep, T value) {
    for (size_t y = 0; y < rows; ++y) {
        RandomAccessIterator rowBuffer = first;
        for (size_t x = 0; x < cols; ++x) {
            *rowBuffer++ = value;
        }
        first += rowStep;
    }
}

/******************************************************************************/

template <typename RandomAccessIterator, typename T>
void fill_rect_step(RandomAccessIterator first, int rows, int cols, int rowStep, int colStep, T value) {
    for (size_t y = 0; y < rows; ++y) {
        RandomAccessIterator rowBuffer = first;
        for (size_t x = 0; x < cols; ++x) {
            *rowBuffer = value;
            rowBuffer += colStep;
        }
        first += rowStep;
    }
}

/******************************************************************************/

template <typename RandomAccessIterator>
void copy_rect(RandomAccessIterator dest, RandomAccessIterator source,
                int rows, int cols, int inRowStep, int outRowStep ) {
    for (int y = 0; y < rows; ++y) {
        RandomAccessIterator inBuffer = source;
        RandomAccessIterator outBuffer = dest;
        for (int x = 0; x < cols; ++x) {
            outBuffer[x] = inBuffer[x];
        }
        source += inRowStep;
        dest += outRowStep;
    }
}

/******************************************************************************/

template <typename RandomAccessIterator>
void copy_rect_step(RandomAccessIterator dest, RandomAccessIterator source,
                    int rows, int cols, int inRowStep, int outRowStep, int inColStep, int outColStep ) {
    for (int y = 0; y < rows; ++y) {
        RandomAccessIterator inBuffer = source;
        RandomAccessIterator outBuffer = dest;
        for (int x = 0; x < cols; ++x) {
            outBuffer[x*outColStep] = inBuffer[x*inColStep];
        }
        source += inRowStep;
        dest += outRowStep;
    }
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
T max_difference( const T* first, const T* second, size_t count) {
    T max = 0;
    for (size_t x = 0; x < count; ++x) {
        T val1 = first[x];
        T val2 = second[x];
        T diff = val1 - val2;
        if (diff < 0)
            diff = -diff;
        max = std::max( max, diff );
    }
    return max;
}

/******************************************************************************/

template <typename T>
T max_difference( const T* first, const T* second, int rows, int cols) {
    return max_difference( first, second, rows*cols );
}

/******************************************************************************/

template <typename T, typename TS>
TS total_difference( const T* first, const T* second, size_t count) {
    TS sum = 0;
    for (size_t x = 0; x < count; ++x) {
        T val1 = first[x];
        T val2 = second[x];
        T diff = val1 - val2;
        if (diff < 0)
            diff = -diff;
        sum += diff;
    }
    return sum;
}

/******************************************************************************/

template <typename T, typename TS>
TS total_difference( const T* first, const T* second, int rows, int cols) {
    return total_difference<T,TS>(first,second,rows*cols);
}

/******************************************************************************/
/******************************************************************************/

void swab16( uint16_t *buf, size_t count ) {
    for (size_t i = 0; i < count; ++i) {
        uint16_t input = buf[i];
        buf[i] = (input >> 8) | (input << 8);
    }
}

/******************************************************************************/

bool isLittleEndian()       // Difficult More Much Debugging and Reading Makes Endian Little
{
#if defined(__LITTLE_ENDIAN__) || defined( _LIBCPP_LITTLE_ENDIAN )
    return true;
#elif defined(__BIG_ENDIAN__) || defined(_LIBCPP_BIG_ENDIAN)
    return false;
#else
    uint32_t pattern = 0x01020304;
    return ( *((uint8_t*)(&pattern)) == 0x04);
#endif
}

/******************************************************************************/

template <typename T>
void write_PGM( const T* buf, int rows, int cols, T max_value, std::string filename) {
    
    std::fstream outfile(filename, std::ios::out | std::ios::binary | std::ios::trunc );

    std::unique_ptr<uint16_t> outputBuff( new uint16_t[ rows*cols ] );
    uint16_t *output = outputBuff.get();
    
    outfile << "P5\n" << cols << " " << rows << "\n65535\n";
    
    for (size_t y = 0; y < rows; ++y) {
    
        for (size_t x = 0; x < cols; ++x) {
            T value_in = *buf++;
            if (value_in < 0)
                value_in = 0;
            if (value_in > max_value)
                value_in = max_value;
            uint16_t value_out = 65535 * value_in / max_value;  // don't care about rounding
            output[x] = value_out;
        }
        
        // PBM is big endian
        if (isLittleEndian())
            swab16( output, cols );
        
        outfile.write( (const char *)output, cols*sizeof(uint16_t) );
    }
    
    outfile.close();
}

/******************************************************************************/
/******************************************************************************/

// top, left, bottom, right
template <typename T>
T laplace_initial_condition_set( T *dest, int rows, int cols, const T values[4] ) {

    T average = (values[0] + values[1] + values[2] + values[3]) / 4;
//   ::fill( dest, dest+rows*cols, T(average) );    // initial condition = average, shows close to best case - but integer calcs are SLOW to converge
    std::fill( dest, dest+rows*cols, T(0) );    // initial condition = black, shows worst case
    
    fill_step( dest, rows, cols, values[1] );  // left
    fill_step( dest+cols-1, rows, cols, values[3] );   // right
    fill_step( dest+(rows-1)*cols, cols, 1, values[2] );   // bottom
    fill_step( dest, cols, 1, values[0] ); // top
    
    return average;
}

/******************************************************************************/

template <typename T>
T laplace_initial_conditions( T *dest, int rows, int cols ) {
    T values[4] = { 20, 80, 200, 100 }; // top, left, bottom, right
    return laplace_initial_condition_set( dest, rows, cols, values );
}

/******************************************************************************/

template <typename T>
T laplace_initial_conditions2( T *dest, int rows, int cols ) {
    T values[4] = { 200, 100, 90, 70 }; // top, left, bottom, right
    return laplace_initial_condition_set( dest, rows, cols, values );
}

/******************************************************************************/
/******************************************************************************/
