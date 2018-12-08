/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


    Some custom, but relatively simple, data types used in multiple tests.

*/

/******************************************************************************/

// a single value wrapped in a struct
template< typename T>
struct SingleItemClass {
    T value;
    SingleItemClass() {}
    SingleItemClass(const T& x) : value(x) {}
};

template <typename T>
inline SingleItemClass<T> &operator+=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.value += y.value;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator-=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.value -= y.value;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator*=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.value *= y.value;
    return x;
}
template <typename T>
inline SingleItemClass<T> &operator/=(SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    x.value /= y.value;
    return x;
}
template <typename T>
inline SingleItemClass<T> operator-(const SingleItemClass<T>& y) {
    return SingleItemClass<T>(- y.value);
}
template <typename T>
inline SingleItemClass<T> operator~(const SingleItemClass<T>& y) {
    return SingleItemClass<T>(~ y.value);
}
template <typename T>
inline SingleItemClass<T> operator+(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value + y.value);
}
template <typename T>
inline SingleItemClass<T> operator-(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value - y.value);
}
template <typename T>
inline SingleItemClass<T> operator*(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value * y.value);
}
template <typename T>
inline SingleItemClass<T> operator/(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value / y.value);
}
template <typename T>
inline SingleItemClass<T> operator%(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value % y.value);
}
template <typename T>
inline SingleItemClass<T> operator&(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value & y.value);
}
template <typename T>
inline SingleItemClass<T> operator|(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value | y.value);
}
template <typename T>
inline SingleItemClass<T> operator^(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return SingleItemClass<T>(x.value ^ y.value);
}
template <typename T>
inline bool operator==(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value == y.value);
}
template <typename T>
inline bool operator!=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value != y.value);
}
template <typename T>
inline bool operator<=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value <= y.value);
}
template <typename T>
inline bool operator>=(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value >= y.value);
}
template <typename T>
inline bool operator<(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value < y.value);
}
template <typename T>
inline bool operator>(const SingleItemClass<T>& x, const SingleItemClass<T>& y) {
    return (x.value > y.value);
}
template <typename T>
inline T abs(const SingleItemClass<T> &x) {
    T temp = x.value;
    if (temp < 0)
        temp = -temp;
    return temp;
}

/******************************************************************************/

// Two values wrapped in a struct
template <typename T>
struct TwoItemClass {
    T aa;
    T bb;
    TwoItemClass() {}
    TwoItemClass(const T& x) : aa(x), bb(x) {}
    TwoItemClass(const T& x, const T &y) : aa(x), bb(y) {}
};

template <typename T>
inline TwoItemClass<T> &operator+=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.aa += y.aa;
    x.bb += y.bb;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator-=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.aa -= y.aa;
    x.bb -= y.bb;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator*=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.aa *= y.aa;
    x.bb *= y.bb;
    return x;
}
template <typename T>
inline TwoItemClass<T> &operator/=(TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    x.aa /= y.aa;
    x.bb /= y.bb;
    return x;
}
template <typename T>
inline TwoItemClass<T> operator-(const TwoItemClass<T>& y) {
    return TwoItemClass<T>(- y.aa, - y.bb);
}
template <typename T>
inline TwoItemClass<T> operator~(const TwoItemClass<T>& y) {
    return TwoItemClass<T>(~ y.aa, ~ y.bb);
}
template <typename T>
inline TwoItemClass<T> operator+(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa + y.aa, x.bb + y.bb);
}
template <typename T>
inline TwoItemClass<T> operator-(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa - y.aa, x.bb - y.bb);
}
template <typename T>
inline TwoItemClass<T> operator*(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa * y.aa, x.bb * y.bb);
}
template <typename T>
inline TwoItemClass<T> operator/(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa / y.aa, x.bb / y.bb);
}
template <typename T>
inline TwoItemClass<T> operator%(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa % y.aa, x.bb % y.bb);
}
template <typename T>
inline TwoItemClass<T> operator&(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa & y.aa, x.bb & y.bb);
}
template <typename T>
inline TwoItemClass<T> operator|(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa | y.aa, x.bb | y.bb);
}
template <typename T>
inline TwoItemClass<T> operator^(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return TwoItemClass<T>(x.aa ^ y.aa, x.bb ^ y.bb);
}
template <typename T>
inline bool operator==(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa == y.aa) && (x.bb == y.bb);
}
template <typename T>
inline bool operator!=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa != y.aa) || (x.bb != y.bb);
}
template <typename T>
inline bool operator<=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa <= y.aa) && (x.bb <= y.bb);
}
template <typename T>
inline bool operator>=(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa >= y.aa) && (x.bb >= y.bb);
}
template <typename T>
inline bool operator<(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa < y.aa) && (x.bb < y.bb);
}
template <typename T>
inline bool operator>(const TwoItemClass<T>& x, const TwoItemClass<T>& y) {
    return (x.aa > y.aa) && (x.bb > y.bb);
}
template <typename T>
inline T abs(const TwoItemClass<T> &x) {
    T temp = x.aa + x.bb;
    if (temp < 0)
        temp = -temp;
    return temp;
}

/******************************************************************************/

// Four values wrapped in a struct
template <typename T>
struct FourItemClass {
    T aa;
    T bb;
    T cc;
    T dd;

    FourItemClass() {}
    FourItemClass(const T& x) : aa(x), bb(x), cc(x), dd(x) {}
    FourItemClass(const T& x, const T &y, const T &z, const T &w) :
            aa(x), bb(y), cc(z), dd(w) {}
};

template <typename T>
inline FourItemClass<T> &operator+=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.aa += y.aa;
    x.bb += y.bb;
    x.cc += y.cc;
    x.dd += y.dd;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator-=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.aa -= y.aa;
    x.bb -= y.bb;
    x.cc -= y.cc;
    x.dd -= y.dd;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator*=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.aa *= y.aa;
    x.bb *= y.bb;
    x.cc *= y.cc;
    x.dd *= y.dd;
    return x;
}
template <typename T>
inline FourItemClass<T> &operator/=(FourItemClass<T>& x, const FourItemClass<T>& y) {
    x.aa /= y.aa;
    x.bb /= y.bb;
    x.cc /= y.cc;
    x.dd /= y.dd;
    return x;
}
template <typename T>
inline FourItemClass<T> operator-(const FourItemClass<T>& y) {
    return FourItemClass<T>(- y.aa, - y.bb, - y.cc, - y.dd);
}
template <typename T>
inline FourItemClass<T> operator~(const FourItemClass<T>& y) {
    return FourItemClass<T>(~ y.aa, ~ y.bb, ~ y.cc, ~ y.dd);
}
template <typename T>
inline FourItemClass<T> operator+(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa + y.aa, x.bb + y.bb, x.cc + y.cc, x.dd + y.dd);
}
template <typename T>
inline FourItemClass<T> operator-(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa - y.aa, x.bb - y.bb, x.cc - y.cc, x.dd - y.dd);
}
template <typename T>
inline FourItemClass<T> operator*(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa * y.aa, x.bb * y.bb, x.cc * y.cc, x.dd * y.dd);
}
template <typename T>
inline FourItemClass<T> operator/(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa / y.aa, x.bb / y.bb, x.cc / y.cc, x.dd / y.dd);
}
template <typename T>
inline FourItemClass<T> operator%(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa % y.aa, x.bb % y.bb, x.cc % y.cc, x.dd % y.dd);
}
template <typename T>
inline FourItemClass<T> operator&(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa & y.aa, x.bb & y.bb, x.cc & y.cc, x.dd & y.dd);
}
template <typename T>
inline FourItemClass<T> operator|(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa | y.aa, x.bb | y.bb, x.cc | y.cc, x.dd | y.dd);
}
template <typename T>
inline FourItemClass<T> operator^(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return FourItemClass<T>(x.aa ^ y.aa, x.bb ^ y.bb, x.cc ^ y.cc, x.dd ^ y.dd);
}
template <typename T>
inline bool operator==(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa == y.aa) && (x.bb == y.bb) && (x.cc == y.cc) && (x.dd == y.dd);
}
template <typename T>
inline bool operator!=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa != y.aa) || (x.bb != y.bb) || (x.cc != y.cc) || (x.dd != y.dd);
}
template <typename T>
inline bool operator<=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa <= y.aa) && (x.bb <= y.bb) && (x.cc <= y.cc) && (x.dd <= y.dd);
}
template <typename T>
inline bool operator>=(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa >= y.aa) && (x.bb >= y.bb) && (x.cc >= y.cc) && (x.dd >= y.dd);
}
template <typename T>
inline bool operator<(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa < y.aa) && (x.bb < y.bb) && (x.cc < y.cc) && (x.dd < y.dd);
}
template <typename T>
inline bool operator>(const FourItemClass<T>& x, const FourItemClass<T>& y) {
    return (x.aa > y.aa) && (x.bb > y.bb) && (x.cc > y.cc) && (x.dd > y.dd);
}
template <typename T>
inline T abs(const FourItemClass<T> &x) {
    T temp = x.aa + x.bb + x.cc + x.dd;
    if (temp < 0)
        temp = -temp;
    return temp;
}

/******************************************************************************/

// Six values wrapped in a struct
template <typename T>
struct SixItemClass {
    T aa;
    T bb;
    T cc;
    T dd;
    T ee;
    T ff;

    SixItemClass() {}
    SixItemClass(const T& x) : aa(x), bb(x), cc(x), dd(x), ee(x), ff(x) {}
    SixItemClass(const T& x, const T &y, const T &z, const T &w, const T &u, const T &v) :
            aa(x), bb(y), cc(z), dd(w), ee(u), ff(v) {}
};

template <typename T>
inline SixItemClass<T> &operator+=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.aa += y.aa;
    x.bb += y.bb;
    x.cc += y.cc;
    x.dd += y.dd;
    x.ee += y.ee;
    x.ff += y.ff;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator-=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.aa -= y.aa;
    x.bb -= y.bb;
    x.cc -= y.cc;
    x.dd -= y.dd;
    x.ee -= y.ee;
    x.ff -= y.ff;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator*=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.aa *= y.aa;
    x.bb *= y.bb;
    x.cc *= y.cc;
    x.dd *= y.dd;
    x.ee *= y.ee;
    x.ff *= y.ff;
    return x;
}
template <typename T>
inline SixItemClass<T> &operator/=(SixItemClass<T>& x, const SixItemClass<T>& y) {
    x.aa /= y.aa;
    x.bb /= y.bb;
    x.cc /= y.cc;
    x.dd /= y.dd;
    x.ee /= y.ee;
    x.ff /= y.ff;
    return x;
}
template <typename T>
inline SixItemClass<T> operator-(const SixItemClass<T>& y) {
    return SixItemClass<T>(- y.aa, - y.bb, - y.cc, - y.dd, - y.ee, - y.ff);
}
template <typename T>
inline SixItemClass<T> operator~(const SixItemClass<T>& y) {
    return SixItemClass<T>(~ y.aa, ~ y.bb, ~ y.cc, ~ y.dd, ~ y.ee, ~ y.ff);
}
template <typename T>
inline SixItemClass<T> operator+(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa + y.aa, x.bb + y.bb, x.cc + y.cc, x.dd + y.dd, x.ee + y.ee, x.ff + y.ff);
}
template <typename T>
inline SixItemClass<T> operator-(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa - y.aa, x.bb - y.bb, x.cc - y.cc, x.dd - y.dd, x.ee - y.ee, x.ff - y.ff);
}
template <typename T>
inline SixItemClass<T> operator*(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa * y.aa, x.bb * y.bb, x.cc * y.cc, x.dd * y.dd, x.ee * y.ee, x.ff * y.ff);
}
template <typename T>
inline SixItemClass<T> operator/(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa / y.aa, x.bb / y.bb, x.cc / y.cc, x.dd / y.dd, x.ee / y.ee, x.ff / y.ff);
}
template <typename T>
inline SixItemClass<T> operator%(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa % y.aa, x.bb % y.bb, x.cc % y.cc, x.dd % y.dd, x.ee % y.ee, x.ff % y.ff);
}
template <typename T>
inline SixItemClass<T> operator&(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa & y.aa, x.bb & y.bb, x.cc & y.cc, x.dd & y.dd, x.ee & y.ee, x.ff & y.ff);
}
template <typename T>
inline SixItemClass<T> operator|(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa | y.aa, x.bb | y.bb, x.cc | y.cc, x.dd | y.dd, x.ee | y.ee, x.ff | y.ff);
}
template <typename T>
inline SixItemClass<T> operator^(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return SixItemClass<T>(x.aa ^ y.aa, x.bb ^ y.bb, x.cc ^ y.cc, x.dd ^ y.dd, x.ee ^ y.ee, x.ff ^ y.ff);
}
template <typename T>
inline bool operator==(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa == y.aa) && (x.bb == y.bb) && (x.cc == y.cc) && (x.dd == y.dd) && (x.ee == y.ee) && (x.ff == y.ff);
}
template <typename T>
inline bool operator!=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa != y.aa) || (x.bb != y.bb) || (x.cc != y.cc) || (x.dd != y.dd) || (x.ee != y.ee) || (x.ff != y.ff);
}
template <typename T>
inline bool operator<=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa <= y.aa) && (x.bb <= y.bb) && (x.cc <= y.cc) && (x.dd <= y.dd)& & (x.ee <= y.ee) && (x.ff <= y.ff);
}
template <typename T>
inline bool operator>=(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa >= y.aa) && (x.bb >= y.bb) && (x.cc >= y.cc) && (x.dd >= y.dd) && (x.ee >= y.ee) && (x.ff >= y.ff);
}
template <typename T>
inline bool operator<(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa < y.aa) && (x.bb < y.bb) && (x.cc < y.cc) && (x.dd < y.dd) && (x.ee < y.ee) && (x.ff < y.ff);
}
template <typename T>
inline bool operator>(const SixItemClass<T>& x, const SixItemClass<T>& y) {
    return (x.aa > y.aa) && (x.bb > y.bb) && (x.cc > y.cc) && (x.dd > y.dd) && (x.ee > y.ff) && (x.ee > y.ff);
}
template <typename T>
inline T abs(const SixItemClass<T> &x) {
    T temp = x.aa + x.bb + x.cc + x.dd + x.ee + x.ff;
    if (temp < 0)
        temp = -temp;
    return temp;
}

/******************************************************************************/
