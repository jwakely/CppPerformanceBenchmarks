/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


    Some custom, but relatively simple, data types used in multiple tests.

*/

/******************************************************************************/

// a single value wrapped in a struct
template <typename T>
struct SingleItemClass {
    T a;
    SingleItemClass() {}
    SingleItemClass(const T& x) : a(x) {}
    template<typename TT>
        inline operator TT () const { return TT(a); }
    template<typename TT>
        SingleItemClass(const TT& x) : a(x) {}
    T &operator*() const { return *a; }
};

template <typename T>
inline SingleItemClass<T> &operator+=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.a += y.a;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator-=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.a -= y.a;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator*=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.a *= y.a;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator/=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.a /= y.a;
    return x;
}
template <typename T>
inline SingleItemClass<T> operator-(const SingleItemClass<T>& x) {
    return SingleItemClass<T>( - x.a );
}
template <typename T>
inline SingleItemClass<T> operator~(const SingleItemClass<T>& x) {
    return SingleItemClass<T>( ~ x.a );
}
template <typename T>
inline SingleItemClass<T> operator+(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a + y.a );
}
template <typename T>
inline SingleItemClass<T> operator-(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a - y.a );
}
template <typename T>
inline SingleItemClass<T> operator*(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a * y.a );
}
template <typename T>
inline SingleItemClass<T> operator/(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a / y.a );
}
template <typename T>
inline SingleItemClass<T> operator%(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a % y.a );
}
template <typename T>
inline SingleItemClass<T> operator&(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a & y.a );
}
template <typename T>
inline SingleItemClass<T> operator|(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a | y.a );
}
template <typename T>
inline SingleItemClass<T> operator^(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>( x.a ^ y.a );
}
template <typename T>
inline bool operator==(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a == y.a);
}
template <typename T>
inline bool operator!=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a != y.a);
}
template <typename T>
inline bool operator>=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a >= y.a);
}
template <typename T>
inline bool operator<=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a <= y.a);
}
template <typename T>
inline bool operator>(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a > y.a);
}
template <typename T>
inline bool operator<(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.a < y.a);
}
template <typename T>
inline T abs(const SingleItemClass<T>& x) {
    T temp = x.a;
    if (temp < 0) temp = -temp;
    return temp;
}


/******************************************************************************/

// Two values wrapped in a struct
template <typename T>
struct TwoItemClass {
    T a,b;
    TwoItemClass() {}
    TwoItemClass(const T& x) : a(x), b(x) {}
    TwoItemClass(const T& in_a, const T& in_b) : a(in_a), b(in_b) {}
};

template <typename T>
inline TwoItemClass<T> &operator+=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.a += y.a; x.b += y.b;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator-=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.a -= y.a; x.b -= y.b;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator*=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.a *= y.a; x.b *= y.b;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator/=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.a /= y.a; x.b /= y.b;
    return x;
}
template <typename T>
inline TwoItemClass<T> operator-(const TwoItemClass<T>& x) {
    return TwoItemClass<T>( - x.a, - x.b );
}
template <typename T>
inline TwoItemClass<T> operator~(const TwoItemClass<T>& x) {
    return TwoItemClass<T>( ~ x.a, ~ x.b );
}
template <typename T>
inline TwoItemClass<T> operator+(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a + y.a, x.b + y.b );
}
template <typename T>
inline TwoItemClass<T> operator-(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a - y.a, x.b - y.b );
}
template <typename T>
inline TwoItemClass<T> operator*(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a * y.a, x.b * y.b );
}
template <typename T>
inline TwoItemClass<T> operator/(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a / y.a, x.b / y.b );
}
template <typename T>
inline TwoItemClass<T> operator%(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a % y.a, x.b % y.b );
}
template <typename T>
inline TwoItemClass<T> operator&(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a & y.a, x.b & y.b );
}
template <typename T>
inline TwoItemClass<T> operator|(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a | y.a, x.b | y.b );
}
template <typename T>
inline TwoItemClass<T> operator^(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>( x.a ^ y.a, x.b ^ y.b );
}
template <typename T>
inline bool operator==(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a == y.a) && (x.b == y.b);
}
template <typename T>
inline bool operator!=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a != y.a) || (x.b != y.b);
}
template <typename T>
inline bool operator>=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a >= y.a) && (x.b >= y.b);
}
template <typename T>
inline bool operator<=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a <= y.a) && (x.b <= y.b);
}
template <typename T>
inline bool operator>(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a > y.a) && (x.b > y.b);
}
template <typename T>
inline bool operator<(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.a < y.a) && (x.b < y.b);
}
template <typename T>
inline T abs(const TwoItemClass<T>& x) {
    T temp = x.a + x.b;
    if (temp < 0) temp = -temp;
    return temp;
}


/******************************************************************************/

// Four values wrapped in a struct
template <typename T>
struct FourItemClass {
    T a,b,c,d;
    FourItemClass() {}
    FourItemClass(const T& x) : a(x), b(x), c(x), d(x) {}
    FourItemClass(const T& in_a, const T& in_b, const T& in_c, const T& in_d) : a(in_a), b(in_b), c(in_c), d(in_d) {}
};

template <typename T>
inline FourItemClass<T> &operator+=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.a += y.a; x.b += y.b; x.c += y.c; x.d += y.d;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator-=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.a -= y.a; x.b -= y.b; x.c -= y.c; x.d -= y.d;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator*=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.a *= y.a; x.b *= y.b; x.c *= y.c; x.d *= y.d;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator/=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.a /= y.a; x.b /= y.b; x.c /= y.c; x.d /= y.d;
    return x;
}
template <typename T>
inline FourItemClass<T> operator-(const FourItemClass<T>& x) {
    return FourItemClass<T>( - x.a, - x.b, - x.c, - x.d );
}
template <typename T>
inline FourItemClass<T> operator~(const FourItemClass<T>& x) {
    return FourItemClass<T>( ~ x.a, ~ x.b, ~ x.c, ~ x.d );
}
template <typename T>
inline FourItemClass<T> operator+(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d );
}
template <typename T>
inline FourItemClass<T> operator-(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d );
}
template <typename T>
inline FourItemClass<T> operator*(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a * y.a, x.b * y.b, x.c * y.c, x.d * y.d );
}
template <typename T>
inline FourItemClass<T> operator/(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a / y.a, x.b / y.b, x.c / y.c, x.d / y.d );
}
template <typename T>
inline FourItemClass<T> operator%(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a % y.a, x.b % y.b, x.c % y.c, x.d % y.d );
}
template <typename T>
inline FourItemClass<T> operator&(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a & y.a, x.b & y.b, x.c & y.c, x.d & y.d );
}
template <typename T>
inline FourItemClass<T> operator|(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a | y.a, x.b | y.b, x.c | y.c, x.d | y.d );
}
template <typename T>
inline FourItemClass<T> operator^(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>( x.a ^ y.a, x.b ^ y.b, x.c ^ y.c, x.d ^ y.d );
}
template <typename T>
inline bool operator==(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a == y.a) && (x.b == y.b) && (x.c == y.c) && (x.d == y.d);
}
template <typename T>
inline bool operator!=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a != y.a) || (x.b != y.b) || (x.c != y.c) || (x.d != y.d);
}
template <typename T>
inline bool operator>=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a >= y.a) && (x.b >= y.b) && (x.c >= y.c) && (x.d >= y.d);
}
template <typename T>
inline bool operator<=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a <= y.a) && (x.b <= y.b) && (x.c <= y.c) && (x.d <= y.d);
}
template <typename T>
inline bool operator>(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a > y.a) && (x.b > y.b) && (x.c > y.c) && (x.d > y.d);
}
template <typename T>
inline bool operator<(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.a < y.a) && (x.b < y.b) && (x.c < y.c) && (x.d < y.d);
}
template <typename T>
inline T abs(const FourItemClass<T>& x) {
    T temp = x.a + x.b + x.c + x.d;
    if (temp < 0) temp = -temp;
    return temp;
}


/******************************************************************************/

// Six values wrapped in a struct
template <typename T>
struct SixItemClass {
    T a,b,c,d,e,f;
    SixItemClass() {}
    SixItemClass(const T& x) : a(x), b(x), c(x), d(x), e(x), f(x) {}
    SixItemClass(const T& in_a, const T& in_b, const T& in_c, const T& in_d, const T& in_e, const T& in_f) : a(in_a), b(in_b), c(in_c), d(in_d), e(in_e), f(in_f) {}
};

template <typename T>
inline SixItemClass<T> &operator+=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.a += y.a; x.b += y.b; x.c += y.c; x.d += y.d; x.e += y.e; x.f += y.f;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator-=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.a -= y.a; x.b -= y.b; x.c -= y.c; x.d -= y.d; x.e -= y.e; x.f -= y.f;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator*=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.a *= y.a; x.b *= y.b; x.c *= y.c; x.d *= y.d; x.e *= y.e; x.f *= y.f;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator/=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.a /= y.a; x.b /= y.b; x.c /= y.c; x.d /= y.d; x.e /= y.e; x.f /= y.f;
    return x;
}
template <typename T>
inline SixItemClass<T> operator-(const SixItemClass<T>& x) {
    return SixItemClass<T>( - x.a, - x.b, - x.c, - x.d, - x.e, - x.f );
}
template <typename T>
inline SixItemClass<T> operator~(const SixItemClass<T>& x) {
    return SixItemClass<T>( ~ x.a, ~ x.b, ~ x.c, ~ x.d, ~ x.e, ~ x.f );
}
template <typename T>
inline SixItemClass<T> operator+(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d, x.e + y.e, x.f + y.f );
}
template <typename T>
inline SixItemClass<T> operator-(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d, x.e - y.e, x.f - y.f );
}
template <typename T>
inline SixItemClass<T> operator*(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a * y.a, x.b * y.b, x.c * y.c, x.d * y.d, x.e * y.e, x.f * y.f );
}
template <typename T>
inline SixItemClass<T> operator/(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a / y.a, x.b / y.b, x.c / y.c, x.d / y.d, x.e / y.e, x.f / y.f );
}
template <typename T>
inline SixItemClass<T> operator%(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a % y.a, x.b % y.b, x.c % y.c, x.d % y.d, x.e % y.e, x.f % y.f );
}
template <typename T>
inline SixItemClass<T> operator&(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a & y.a, x.b & y.b, x.c & y.c, x.d & y.d, x.e & y.e, x.f & y.f );
}
template <typename T>
inline SixItemClass<T> operator|(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a | y.a, x.b | y.b, x.c | y.c, x.d | y.d, x.e | y.e, x.f | y.f );
}
template <typename T>
inline SixItemClass<T> operator^(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>( x.a ^ y.a, x.b ^ y.b, x.c ^ y.c, x.d ^ y.d, x.e ^ y.e, x.f ^ y.f );
}
template <typename T>
inline bool operator==(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a == y.a) && (x.b == y.b) && (x.c == y.c) && (x.d == y.d) && (x.e == y.e) && (x.f == y.f);
}
template <typename T>
inline bool operator!=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a != y.a) || (x.b != y.b) || (x.c != y.c) || (x.d != y.d) || (x.e != y.e) || (x.f != y.f);
}
template <typename T>
inline bool operator>=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a >= y.a) && (x.b >= y.b) && (x.c >= y.c) && (x.d >= y.d) && (x.e >= y.e) && (x.f >= y.f);
}
template <typename T>
inline bool operator<=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a <= y.a) && (x.b <= y.b) && (x.c <= y.c) && (x.d <= y.d) && (x.e <= y.e) && (x.f <= y.f);
}
template <typename T>
inline bool operator>(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a > y.a) && (x.b > y.b) && (x.c > y.c) && (x.d > y.d) && (x.e > y.e) && (x.f > y.f);
}
template <typename T>
inline bool operator<(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.a < y.a) && (x.b < y.b) && (x.c < y.c) && (x.d < y.d) && (x.e < y.e) && (x.f < y.f);
}
template <typename T>
inline T abs(const SixItemClass<T>& x) {
    T temp = x.a + x.b + x.c + x.d + x.e + x.f;
    if (temp < 0) temp = -temp;
    return temp;
}


/******************************************************************************/

// Eight values wrapped in a struct
template <typename T>
struct EightItemClass {
    T a,b,c,d,e,f,g,h;
    EightItemClass() {}
    EightItemClass(const T& x) : a(x), b(x), c(x), d(x), e(x), f(x), g(x), h(x) {}
    EightItemClass(const T& in_a, const T& in_b, const T& in_c, const T& in_d, const T& in_e, const T& in_f, const T& in_g, const T& in_h) : a(in_a), b(in_b), c(in_c), d(in_d), e(in_e), f(in_f), g(in_g), h(in_h) {}
};

template <typename T>
inline EightItemClass<T> &operator+=(EightItemClass<T>& x, const EightItemClass<T>& y) {
    x.a += y.a; x.b += y.b; x.c += y.c; x.d += y.d; x.e += y.e; x.f += y.f; x.g += y.g; x.h += y.h;
    return x;
}
template <typename T>
inline EightItemClass<T> &operator-=(EightItemClass<T>& x, const EightItemClass<T>& y) {
    x.a -= y.a; x.b -= y.b; x.c -= y.c; x.d -= y.d; x.e -= y.e; x.f -= y.f; x.g -= y.g; x.h -= y.h;
    return x;
}
template <typename T>
inline EightItemClass<T> &operator*=(EightItemClass<T>& x, const EightItemClass<T>& y) {
    x.a *= y.a; x.b *= y.b; x.c *= y.c; x.d *= y.d; x.e *= y.e; x.f *= y.f; x.g *= y.g; x.h *= y.h;
    return x;
}
template <typename T>
inline EightItemClass<T> &operator/=(EightItemClass<T>& x, const EightItemClass<T>& y) {
    x.a /= y.a; x.b /= y.b; x.c /= y.c; x.d /= y.d; x.e /= y.e; x.f /= y.f; x.g /= y.g; x.h /= y.h;
    return x;
}
template <typename T>
inline EightItemClass<T> operator-(const EightItemClass<T>& x) {
    return EightItemClass<T>( - x.a, - x.b, - x.c, - x.d, - x.e, - x.f, - x.g, - x.h );
}
template <typename T>
inline EightItemClass<T> operator~(const EightItemClass<T>& x) {
    return EightItemClass<T>( ~ x.a, ~ x.b, ~ x.c, ~ x.d, ~ x.e, ~ x.f, ~ x.g, ~ x.h );
}
template <typename T>
inline EightItemClass<T> operator+(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d, x.e + y.e, x.f + y.f, x.g + y.g, x.h + y.h );
}
template <typename T>
inline EightItemClass<T> operator-(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d, x.e - y.e, x.f - y.f, x.g - y.g, x.h - y.h );
}
template <typename T>
inline EightItemClass<T> operator*(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a * y.a, x.b * y.b, x.c * y.c, x.d * y.d, x.e * y.e, x.f * y.f, x.g * y.g, x.h * y.h );
}
template <typename T>
inline EightItemClass<T> operator/(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a / y.a, x.b / y.b, x.c / y.c, x.d / y.d, x.e / y.e, x.f / y.f, x.g / y.g, x.h / y.h );
}
template <typename T>
inline EightItemClass<T> operator%(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a % y.a, x.b % y.b, x.c % y.c, x.d % y.d, x.e % y.e, x.f % y.f, x.g % y.g, x.h % y.h );
}
template <typename T>
inline EightItemClass<T> operator&(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a & y.a, x.b & y.b, x.c & y.c, x.d & y.d, x.e & y.e, x.f & y.f, x.g & y.g, x.h & y.h );
}
template <typename T>
inline EightItemClass<T> operator|(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a | y.a, x.b | y.b, x.c | y.c, x.d | y.d, x.e | y.e, x.f | y.f, x.g | y.g, x.h | y.h );
}
template <typename T>
inline EightItemClass<T> operator^(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return EightItemClass<T>( x.a ^ y.a, x.b ^ y.b, x.c ^ y.c, x.d ^ y.d, x.e ^ y.e, x.f ^ y.f, x.g ^ y.g, x.h ^ y.h );
}
template <typename T>
inline bool operator==(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a == y.a) && (x.b == y.b) && (x.c == y.c) && (x.d == y.d) && (x.e == y.e) && (x.f == y.f) && (x.g == y.g) && (x.h == y.h);
}
template <typename T>
inline bool operator!=(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a != y.a) || (x.b != y.b) || (x.c != y.c) || (x.d != y.d) || (x.e != y.e) || (x.f != y.f) || (x.g != y.g) || (x.h != y.h);
}
template <typename T>
inline bool operator>=(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a >= y.a) && (x.b >= y.b) && (x.c >= y.c) && (x.d >= y.d) && (x.e >= y.e) && (x.f >= y.f) && (x.g >= y.g) && (x.h >= y.h);
}
template <typename T>
inline bool operator<=(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a <= y.a) && (x.b <= y.b) && (x.c <= y.c) && (x.d <= y.d) && (x.e <= y.e) && (x.f <= y.f) && (x.g <= y.g) && (x.h <= y.h);
}
template <typename T>
inline bool operator>(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a > y.a) && (x.b > y.b) && (x.c > y.c) && (x.d > y.d) && (x.e > y.e) && (x.f > y.f) && (x.g > y.g) && (x.h > y.h);
}
template <typename T>
inline bool operator<(const EightItemClass<T>& x, const EightItemClass<T>& y) {
    return (x.a < y.a) && (x.b < y.b) && (x.c < y.c) && (x.d < y.d) && (x.e < y.e) && (x.f < y.f) && (x.g < y.g) && (x.h < y.h);
}
template <typename T>
inline T abs(const EightItemClass<T>& x) {
    T temp = x.a + x.b + x.c + x.d + x.e + x.f + x.g + x.h;
    if (temp < 0) temp = -temp;
    return temp;
}


/******************************************************************************/

// Ten values wrapped in a struct
template <typename T>
struct TenItemClass {
    T a,b,c,d,e,f,g,h,i,j;
    TenItemClass() {}
    TenItemClass(const T& x) : a(x), b(x), c(x), d(x), e(x), f(x), g(x), h(x), i(x), j(x) {}
    TenItemClass(const T& in_a, const T& in_b, const T& in_c, const T& in_d, const T& in_e, const T& in_f, const T& in_g, const T& in_h, const T& in_i, const T& in_j) : a(in_a), b(in_b), c(in_c), d(in_d), e(in_e), f(in_f), g(in_g), h(in_h), i(in_i), j(in_j) {}
};

template <typename T>
inline TenItemClass<T> &operator+=(TenItemClass<T>& x, const TenItemClass<T>& y) {
    x.a += y.a; x.b += y.b; x.c += y.c; x.d += y.d; x.e += y.e; x.f += y.f; x.g += y.g; x.h += y.h; x.i += y.i; x.j += y.j;
    return x;
}
template <typename T>
inline TenItemClass<T> &operator-=(TenItemClass<T>& x, const TenItemClass<T>& y) {
    x.a -= y.a; x.b -= y.b; x.c -= y.c; x.d -= y.d; x.e -= y.e; x.f -= y.f; x.g -= y.g; x.h -= y.h; x.i -= y.i; x.j -= y.j;
    return x;
}
template <typename T>
inline TenItemClass<T> &operator*=(TenItemClass<T>& x, const TenItemClass<T>& y) {
    x.a *= y.a; x.b *= y.b; x.c *= y.c; x.d *= y.d; x.e *= y.e; x.f *= y.f; x.g *= y.g; x.h *= y.h; x.i *= y.i; x.j *= y.j;
    return x;
}
template <typename T>
inline TenItemClass<T> &operator/=(TenItemClass<T>& x, const TenItemClass<T>& y) {
    x.a /= y.a; x.b /= y.b; x.c /= y.c; x.d /= y.d; x.e /= y.e; x.f /= y.f; x.g /= y.g; x.h /= y.h; x.i /= y.i; x.j /= y.j;
    return x;
}
template <typename T>
inline TenItemClass<T> operator-(const TenItemClass<T>& x) {
    return TenItemClass<T>( - x.a, - x.b, - x.c, - x.d, - x.e, - x.f, - x.g, - x.h, - x.i, - x.j );
}
template <typename T>
inline TenItemClass<T> operator~(const TenItemClass<T>& x) {
    return TenItemClass<T>( ~ x.a, ~ x.b, ~ x.c, ~ x.d, ~ x.e, ~ x.f, ~ x.g, ~ x.h, ~ x.i, ~ x.j );
}
template <typename T>
inline TenItemClass<T> operator+(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d, x.e + y.e, x.f + y.f, x.g + y.g, x.h + y.h, x.i + y.i, x.j + y.j );
}
template <typename T>
inline TenItemClass<T> operator-(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d, x.e - y.e, x.f - y.f, x.g - y.g, x.h - y.h, x.i - y.i, x.j - y.j );
}
template <typename T>
inline TenItemClass<T> operator*(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a * y.a, x.b * y.b, x.c * y.c, x.d * y.d, x.e * y.e, x.f * y.f, x.g * y.g, x.h * y.h, x.i * y.i, x.j * y.j );
}
template <typename T>
inline TenItemClass<T> operator/(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a / y.a, x.b / y.b, x.c / y.c, x.d / y.d, x.e / y.e, x.f / y.f, x.g / y.g, x.h / y.h, x.i / y.i, x.j / y.j );
}
template <typename T>
inline TenItemClass<T> operator%(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a % y.a, x.b % y.b, x.c % y.c, x.d % y.d, x.e % y.e, x.f % y.f, x.g % y.g, x.h % y.h, x.i % y.i, x.j % y.j );
}
template <typename T>
inline TenItemClass<T> operator&(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a & y.a, x.b & y.b, x.c & y.c, x.d & y.d, x.e & y.e, x.f & y.f, x.g & y.g, x.h & y.h, x.i & y.i, x.j & y.j );
}
template <typename T>
inline TenItemClass<T> operator|(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a | y.a, x.b | y.b, x.c | y.c, x.d | y.d, x.e | y.e, x.f | y.f, x.g | y.g, x.h | y.h, x.i | y.i, x.j | y.j );
}
template <typename T>
inline TenItemClass<T> operator^(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return TenItemClass<T>( x.a ^ y.a, x.b ^ y.b, x.c ^ y.c, x.d ^ y.d, x.e ^ y.e, x.f ^ y.f, x.g ^ y.g, x.h ^ y.h, x.i ^ y.i, x.j ^ y.j );
}
template <typename T>
inline bool operator==(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a == y.a) && (x.b == y.b) && (x.c == y.c) && (x.d == y.d) && (x.e == y.e) && (x.f == y.f) && (x.g == y.g) && (x.h == y.h) && (x.i == y.i) && (x.j == y.j);
}
template <typename T>
inline bool operator!=(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a != y.a) || (x.b != y.b) || (x.c != y.c) || (x.d != y.d) || (x.e != y.e) || (x.f != y.f) || (x.g != y.g) || (x.h != y.h) || (x.i != y.i) || (x.j != y.j);
}
template <typename T>
inline bool operator>=(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a >= y.a) && (x.b >= y.b) && (x.c >= y.c) && (x.d >= y.d) && (x.e >= y.e) && (x.f >= y.f) && (x.g >= y.g) && (x.h >= y.h) && (x.i >= y.i) && (x.j >= y.j);
}
template <typename T>
inline bool operator<=(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a <= y.a) && (x.b <= y.b) && (x.c <= y.c) && (x.d <= y.d) && (x.e <= y.e) && (x.f <= y.f) && (x.g <= y.g) && (x.h <= y.h) && (x.i <= y.i) && (x.j <= y.j);
}
template <typename T>
inline bool operator>(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a > y.a) && (x.b > y.b) && (x.c > y.c) && (x.d > y.d) && (x.e > y.e) && (x.f > y.f) && (x.g > y.g) && (x.h > y.h) && (x.i > y.i) && (x.j > y.j);
}
template <typename T>
inline bool operator<(const TenItemClass<T>& x, const TenItemClass<T>& y) {
    return (x.a < y.a) && (x.b < y.b) && (x.c < y.c) && (x.d < y.d) && (x.e < y.e) && (x.f < y.f) && (x.g < y.g) && (x.h < y.h) && (x.i < y.i) && (x.j < y.j);
}
template <typename T>
inline T abs(const TenItemClass<T>& x) {
    T temp = x.a + x.b + x.c + x.d + x.e + x.f + x.g + x.h + x.i + x.j;
    if (temp < 0) temp = -temp;
    return temp;
}

/******************************************************************************/
