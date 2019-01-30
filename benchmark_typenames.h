/*
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )
    
    Shared source file for quick function to get type names.
    Sure, it would be nice to use some facilities in C++17, but going
      by history, we won't see those available everywhere until 2031.

*/

#include <string>
#include <typeinfo>

namespace benchmark {

// This ignores const, volatile and other qualifiers, so those fall into the generic case
template<typename T>
std::string getTypeName() { return typeid(T).name(); };

template<>
std::string getTypeName<uint8_t>() { return std::string("uint8_t"); }

template<>
std::string getTypeName<uint16_t>() { return std::string("uint16_t"); }

template<>
std::string getTypeName<uint32_t>() { return std::string("uint32_t"); }

template<>
std::string getTypeName<uint64_t>() { return std::string("uint64_t"); }

template<>
std::string getTypeName<int8_t>() { return std::string("int8_t"); }

#if !defined (__sun)
// char gets special treatment under some compilers, different from int8_t and uint8_t
// except on Solaris
template<>
std::string getTypeName<char>() { return std::string("int8_t"); }
#endif

template<>
std::string getTypeName<int16_t>() { return std::string("int16_t"); }

template<>
std::string getTypeName<int32_t>() { return std::string("int32_t"); }

template<>
std::string getTypeName<int64_t>() { return std::string("int64_t"); }

template<>
std::string getTypeName<float>() { return std::string("float"); }

template<>
std::string getTypeName<double>() { return std::string("double"); }

template<>
std::string getTypeName<long double>() { return std::string("long double"); }

template<>
std::string getTypeName<uint8_t *>() { return std::string("uint8_t*"); }

template<>
std::string getTypeName<uint16_t *>() { return std::string("uint16_t*"); }

template<>
std::string getTypeName<uint32_t *>() { return std::string("uint32_t*"); }

template<>
std::string getTypeName<uint64_t *>() { return std::string("uint64_t*"); }

template<>
std::string getTypeName<int8_t *>() { return std::string("int8_t*"); }

template<>
std::string getTypeName<int16_t *>() { return std::string("int16_t*"); }

template<>
std::string getTypeName<int32_t *>() { return std::string("int32_t*"); }

template<>
std::string getTypeName<int64_t *>() { return std::string("int64_t*"); }

template<>
std::string getTypeName<float *>() { return std::string("float*"); }

template<>
std::string getTypeName<double *>() { return std::string("double*"); }

template<>
std::string getTypeName<long double *>() { return std::string("long double*"); }

}    // end namespace benchmark

using namespace benchmark;

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
