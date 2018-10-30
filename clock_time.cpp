/*
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Examine performance of standard time and date functions.

    NOTE: Many XML writers use the ASCII time conversion routines.
    NOTE: Benchmarks will make heavy use of timing functions.
    NOTE: Web servers and many applications will use time functions for testing/invalidating caches.


Assumptions:

    1) Library/OS time and date functions should be be fast.

    2) Library/OS time and date functions should have reasonable precision.
 
    3) Library time conversions to and from ASCII should be well optimized.

    4) STD library functions should be fastest and best precision available.



NOTE - slowdowns seen in some virtual environments due to VM software poorly caching host time values.
        ie: Windows10 on VirtualBox, Linux on AWS.
NOTE - getdate crashes with an internal NULL pointer on Linux and MacOS, does not exist on Windows.



TODO - std::chrono
    https://en.cppreference.com/w/cpp/chrono
    http://www.cplusplus.com/reference/chrono/
C++11
    DONE - had to be careful to keep it portable though, some compilers are fragile (bug reports filed).
C++17/20
    Totally unsafe at this time.

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <chrono>
#include "benchmark_results.h"
#include "benchmark_timer.h"

// TODO - ccox - clean up the macro tests, separate into semi-sane groups
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

// one of these should be defined on Linux derived OSes
#if defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H) || defined(isBSD)
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/utsname.h>
#if !defined(isBSD)
#include <sys/sysinfo.h>
#endif
#include <sys/resource.h>
#endif

#if defined(_MACHTYPES_H_)
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/types.h>
#include <mach/mach_time.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2500;


// no data here, so this is just a block loop count
#define SIZE     8000


// volatile value to store sums, should prevent over-optimization
volatile static double sFake = 0.0;

/******************************************************************************/
/******************************************************************************/

template <typename T>
inline void check_fake_sum(T result) {
    sFake += result;
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_noarg_retval(int count, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift();
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

#ifndef _WIN32
void test_getitimer(int count, const char *label) {
  int i;
  
  itimerval timerData;
  timeval tempTimeVal;
  
  tempTimeVal.tv_sec = 5000;
  tempTimeVal.tv_usec = 0;
  
  timerData.it_interval = tempTimeVal;
  timerData.it_value = tempTimeVal;
  
  setitimer( ITIMER_REAL, &timerData, NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        itimerval this_timer;
        getitimer( ITIMER_REAL, &this_timer );
        result += this_timer.it_value.tv_sec;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );
  
  // stop the timer
  tempTimeVal.tv_sec = 0;
  tempTimeVal.tv_usec = 0;
  
  timerData.it_interval = tempTimeVal;
  timerData.it_value = tempTimeVal;
  
  setitimer( ITIMER_REAL, &timerData, NULL);

}

/******************************************************************************/

void test_setitimer(int count, const char *label) {
  int i;
  
  start_timer();
  itimerval timerData;
  timeval tempTimeVal;
  
  tempTimeVal.tv_sec = 5000;
  tempTimeVal.tv_usec = 0;
  
  timerData.it_interval = tempTimeVal;
  timerData.it_value = tempTimeVal;
  
  for(i = 0; i < iterations; ++i) {
    for (int n = 0; n < count; ++n) {
        setitimer( ITIMER_REAL, &timerData, NULL);
    }
  }
  
  record_result( timer(), label );
  
  // stop the timer
  tempTimeVal.tv_sec = 0;
  tempTimeVal.tv_usec = 0;
  
  timerData.it_interval = tempTimeVal;
  timerData.it_value = tempTimeVal;
  
  setitimer( ITIMER_REAL, &timerData, NULL);

}
#endif // WIN32

/******************************************************************************/
/******************************************************************************/

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined(__sun)
struct clock_sysinfo {
    static long do_shift() { struct sysinfo temp; sysinfo(&temp); return (temp.uptime); }
    static double seconds( double old ) { time_t now = do_shift();  return ( now - old ); }
};
#endif

/******************************************************************************/

#ifndef _WIN32
template <clockid_t TYPE>
    struct clock_clock_gettime {
      static long do_shift() { struct timespec tt; clock_gettime(TYPE, &tt); return tt.tv_nsec; }
      static double seconds( double old ) {
                    struct timespec tt; clock_gettime(TYPE, &tt);
                    double now = tt.tv_sec + (1.0e-9 * tt.tv_nsec);
                    return ( now - old ); }
    };
#endif

/******************************************************************************/

#ifndef _WIN32
struct clock_gettimeofday {
  static suseconds_t do_shift() { struct timeval tt; gettimeofday(&tt, NULL); return tt.tv_usec; }
  static double seconds( double old ) {
                struct timeval tt; gettimeofday(&tt, NULL);
                double now = tt.tv_sec + (1.0e-6 * tt.tv_usec);
                return ( now - old ); }
};
#endif

/******************************************************************************/

#ifndef _WIN32
struct clock_times {
  static clock_t do_shift() { struct tms temp; return (times(&temp)); }
  static double seconds( double old ) {
                struct tms tt; times(&tt);
                double now = tt.tms_utime / (double)(CLOCKS_PER_SEC);
                return ( now - old ); }
};

struct clock_getrusage {
  static clock_t do_shift() { struct rusage temp; getrusage(RUSAGE_SELF, &temp); return temp.ru_utime.tv_usec; }
  static double seconds( double old ) {
                struct rusage tt; getrusage(RUSAGE_SELF, &tt);
                double now = tt.ru_utime.tv_sec + (1.0e-6 * tt.ru_utime.tv_usec);
                return ( now - old ); }
};
#endif

/******************************************************************************/

struct clock_clock {
    static clock_t do_shift() { return (clock()); }
    static double seconds( double old ) { clock_t now = clock();  return ( (now/(double)(CLOCKS_PER_SEC)) - old ); }
};

/******************************************************************************/

struct clock_time {
    static time_t do_shift() { return (time(NULL)); }
    static double seconds( double old ) { time_t now = time(NULL);  return ( now - old ); }
};

/******************************************************************************/

#ifdef _WIN32

// as is, this can fail at the end of a month as the day rolls over
struct clock_GetLocalTime {
  static WORD do_shift() { SYSTEMTIME temp; GetLocalTime( &temp ); return temp.wMilliseconds; }
  static double seconds( double old ) {
                SYSTEMTIME tt; GetLocalTime( &tt );
                double now = tt.wSecond + (1.0e-3 * tt.wMilliseconds) + (60 * tt.wMinute) + (60*60 * tt.wHour) + (24*60*60 * tt.wDay);
                return ( now - old ); }
};

// as is, this can fail at the end of a month as the day rolls over
struct clock_GetSystemTime {
  static WORD do_shift() { SYSTEMTIME temp; GetSystemTime( &temp ); return temp.wMilliseconds; }
  static double seconds( double old ) {
                SYSTEMTIME tt; GetSystemTime( &tt );
                double now = tt.wSecond + (1.0e-3 * tt.wMilliseconds) + (60 * tt.wMinute) + (60*60 * tt.wHour) + (24*60*60 * tt.wDay);
                return ( now - old ); }
};

struct clock_GetSystemTimeAsFileTime {
  static WORD do_shift() { FILETIME temp; GetSystemTimeAsFileTime( &temp ); return temp.dwLowDateTime; }
  static double seconds( double old ) {
                FILETIME tt; GetSystemTimeAsFileTime( &tt );
                uint64_t singleValue = ((uint64_t)tt.dwHighDateTime << 32) + tt.dwLowDateTime;
                double now = 1.0e-7 * singleValue;      // 100 nanosecond intervals
                return ( now - old ); }
};

struct clock_GetTickCount {
  static DWORD do_shift() { return GetTickCount(); }
  static double seconds( double old ) {
                double now = 1.0e-3 * GetTickCount();
                return ( now - old ); }
};

struct clock_GetTickCount64 {
  static ULONGLONG do_shift() { return GetTickCount64(); }
  static double seconds( double old ) {
                double now = 1.0e-3 * GetTickCount64();
                return ( now - old ); }
};

struct clock_GetSystemTimes {
  static DWORD do_shift() { FILETIME temp; GetSystemTimes(NULL,NULL,&temp); return temp.dwLowDateTime; }
  static double seconds( double old ) {
                FILETIME tt; GetSystemTimes(NULL,NULL,&tt);
                uint64_t singleValue = ((uint64_t)tt.dwHighDateTime << 32) + tt.dwLowDateTime;
                double now = 1.0e-7 * singleValue;      // 100 nanosecond intervals
                return ( now - old ); }
};

struct clock_QueryPerformanceCounter {
  static LONGLONG do_shift() { LARGE_INTEGER temp; QueryPerformanceCounter(&temp); return temp.QuadPart; }
  static double seconds( double old ) {
                static double freq = 0;
                if (freq == 0) {
                    LARGE_INTEGER temp;
                    QueryPerformanceFrequency(&temp);
                    freq = (double) temp.QuadPart;
                }
                LARGE_INTEGER tt; QueryPerformanceCounter(&tt);
                double now = tt.QuadPart / (double)freq;
                return ( now - old ); }
};

#endif  // def _WIN32

/******************************************************************************/

#if defined(_MACHTYPES_H_)

struct clock_mach_absolute_time {
  static uint64_t do_shift() { return (mach_absolute_time()); }
      static double seconds( double old ) {
                    // @#^@%$!@#$% Apple deprecated the conversion function for this
                    double now = mach_absolute_time() * 1.0e-9;
                    return ( now - old ); }
};

struct clock_mach_approximate_time {
  static uint64_t do_shift() { return (mach_approximate_time()); }
      static double seconds( double old ) {
                    // @#^@%$!@#$% Apple deprecated the conversion function for this
                    double now = mach_approximate_time() * 1.0e-9;
                    return ( now - old ); }
};

struct clock_mach_continuous_time {
  static uint64_t do_shift() { return (mach_continuous_time()); }
      static double seconds( double old ) {
                    // @#^@%$!@#$% Apple deprecated the conversion function for this
                    double now = mach_continuous_time() * 1.0e-9;
                    return ( now - old ); }
};

struct clock_mach_continuous_approximate_time {
  static uint64_t do_shift() { return (mach_continuous_approximate_time()); }
      static double seconds( double old ) {
                    // @#^@%$!@#$% Apple deprecated the conversion function for this
                    double now = mach_continuous_approximate_time() * 1.0e-9;
                    return ( now - old ); }
};

template <clockid_t TYPE>
    struct clock_clock_gettime_nsec {
      static uint64_t do_shift() { return clock_gettime_nsec_np(TYPE); }
      static double seconds( double old ) {
                    double now = clock_gettime_nsec_np(TYPE) * 1.0e-9;
                    return ( now - old ); }
    };

#endif  // defined(_MACHTYPES_H_)

/******************************************************************************/

// C++11 timers  (std::chrono is obviously the result of the obfuscated C++ contest)
// Yes, this could be expressed more cleanly - but I had to go with what worked on the most compilers at the time I wrote it.
// Compiler vendors with problems are working on the issues - and once those are fixed, I will revisit this code.
template< typename CLK >
struct clock_std_chrono {
    static double do_shift() { return seconds(0.0); }
    static double seconds( double old ) {
                    auto now = CLK::now();
                    auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now).time_since_epoch().count();
                    double now_seconds = now_ns * 1.0e-9;
                    return ( now_seconds - old ); }
};

/******************************************************************************/

void test_asctime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        char *string = asctime( & tempTime );
        result += string[0];
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_ctime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        char *string = ctime( & tempClock );
        result += string[0];
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_difftime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  time_t tempClock1 = time(NULL);
  
  for(i = 0; i < iterations; ++i) {
    double result = 0;
    for (int n = 0; n < count; ++n) {
        double diff = difftime( tempClock, tempClock1 );
        result += diff;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_localtime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        struct tm *tempTimePtr = localtime(&tempClock);
        result += tempTimePtr->tm_sec;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_gmtime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        struct tm *tempTimePtr = gmtime(&tempClock);
        result += tempTimePtr->tm_sec;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_mktime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        time_t temp = mktime( &tempTime );
        result += temp;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

#ifndef _WIN32
void test_timegm(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        time_t temp = timegm( &tempTime );
        result += temp;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_timelocal(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        time_t temp = timelocal( &tempTime );
        result += temp;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_asctimer(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        char tempStr[200];
        char *string = asctime_r( &tempTime, tempStr );
        result += string[0];
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_ctimer(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        char tempStr[200];
        char *string = ctime_r( &tempClock, tempStr );
        result += string[0];
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_localtimer(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        struct tm tempTime;
        struct tm *tempTimePtr = localtime_r( &tempClock, &tempTime );
        result += tempTimePtr->tm_sec;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_gmtimer(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        struct tm tempTime;
        struct tm *tempTimePtr = gmtime_r( &tempClock, &tempTime );
        result += tempTimePtr->tm_sec;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}
#endif  // ndef _WIN32

/******************************************************************************/

#if defined(_MACHTYPES_H_)
void test_time2posix(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        time_t temp = time2posix( tempClock );
        result += temp;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_posix2time(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        time_t temp = posix2time( tempClock );
        result += temp;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}
#endif  // defined(_MACHTYPES_H_)

/******************************************************************************/

void test_strftime(int count, const char *label) {
  int i;
  
  time_t tempClock = time(NULL);
  struct tm *tempTimePtr = localtime(&tempClock);
  struct tm tempTime = *tempTimePtr;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    long result = 0;
    for (int n = 0; n < count; ++n) {
        const size_t maxSize = 200;
        char tempStr[maxSize];
        size_t ignored = strftime( tempStr, maxSize, "%F %T", &tempTime );
        result += tempStr[0] + ignored;
    }
    check_fake_sum(result);
  }
  
  record_result( timer(), label );

}

/******************************************************************************/
/******************************************************************************/

// basic idea is from Livermore Loops (Fortran)
template <typename Timer>
double timer_precision( const char *label )
{
    const int64_t loopLimit = SIZE * iterations;
    double totalDelta = 0.0;
    int64_t measurements = 0;

    double time1 = Timer::seconds(0.0);
    double startTime = time1;

    for ( int64_t k = 0 ; k < loopLimit ; ++k ) {
        double time2 = Timer::seconds(0.0);
        if( time2 > time1 ) {                   // some timers can go backward due to date changes, network time adjustments, and approximations
            totalDelta += (time2 - time1);
            time1 = time2;
            measurements++;
            if( measurements >= 200 )   // if the timer is really fast and we have plenty of samples, finish early
                break;
        }
    }

    if ( measurements <= 2 )    // really poor resolution clock
        return 1.0;

    double stopDelta = time1 - startTime;   // Timer::seconds(startTime);

    double resolution = totalDelta / (double)measurements;

    double testDelta = fabs(stopDelta - totalDelta);
    if ( testDelta > resolution ) {
        printf("\ttimer %s may not be reliable, difference = %g\n", label, testDelta );
    }

    return resolution;
}

/******************************************************************************/

template <typename Timer>
void test_timer_precision( const char *label )
{
    double precision = timer_precision< Timer > ( label );
    
    if ( fabs(precision - 1.0) < 1.0e-4 )
        printf( "%s precision is approximately 1 seconds\n", label );
    else
        printf( "%s precision is approximately %.2e seconds\n", label, precision );
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



    test_noarg_retval<clock_t, clock_clock >(SIZE,"clock");
    test_noarg_retval<time_t, clock_time >(SIZE,"time");

#ifndef _WIN32
    test_noarg_retval<suseconds_t, clock_gettimeofday >(SIZE,"gettimeofday");
    test_noarg_retval<clock_t, clock_getrusage >(SIZE,"getrusage");
    test_noarg_retval<clock_t, clock_times >(SIZE,"times");

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined(__sun) && !defined(isBSD)
    test_noarg_retval<long, clock_sysinfo >(SIZE,"sysinfo uptime");
#endif

    test_noarg_retval<long, clock_clock_gettime<CLOCK_REALTIME> >(SIZE,"clock_gettime realtime");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_MONOTONIC> >(SIZE,"clock_gettime monotonic");
#if !defined(__sun) && !defined(isBSD)
    test_noarg_retval<long, clock_clock_gettime<CLOCK_MONOTONIC_RAW> >(SIZE,"clock_gettime monotonic_raw");
#endif
    test_noarg_retval<long, clock_clock_gettime<CLOCK_PROCESS_CPUTIME_ID> >(SIZE,"clock_gettime process_cputime");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_THREAD_CPUTIME_ID> >(SIZE,"clock_gettime thread_cputime");
#endif  // not _WIN32

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined(__sun) && !defined(isBSD)
    test_noarg_retval<long, clock_clock_gettime<CLOCK_REALTIME_COARSE> >(SIZE,"clock_gettime realtime_coarse");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_MONOTONIC_COARSE> >(SIZE,"clock_gettime monotonic_coarse");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_BOOTTIME> >(SIZE,"clock_gettime boottime");
#endif

#if defined(__sun)
    test_noarg_retval<long, clock_clock_gettime<CLOCK_HIGHRES> >(SIZE,"clock_gettime highres");
#endif

#if defined(_MACHTYPES_H_)
    test_noarg_retval<long, clock_clock_gettime<CLOCK_UPTIME_RAW> >(SIZE,"clock_gettime uptime_raw");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_UPTIME_RAW_APPROX> >(SIZE,"clock_gettime uptime_approx");
    test_noarg_retval<long, clock_clock_gettime<CLOCK_MONOTONIC_RAW_APPROX> >(SIZE,"clock_gettime monotonic_approx");
    
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_REALTIME> >(SIZE,"clock_gettime_nsec realtime");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_MONOTONIC> >(SIZE,"clock_gettime_nsec monotonic");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_MONOTONIC_RAW> >(SIZE,"clock_gettime_nsec monotonic_raw");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_PROCESS_CPUTIME_ID> >(SIZE,"clock_gettime_nsec process_cputime");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_THREAD_CPUTIME_ID> >(SIZE,"clock_gettime_nsec thread_cputime");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_UPTIME_RAW> >(SIZE,"clock_gettime_nsec uptime_raw");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_UPTIME_RAW_APPROX> >(SIZE,"clock_gettime_nsec uptime_approx");
    test_noarg_retval<uint64_t, clock_clock_gettime_nsec<CLOCK_MONOTONIC_RAW_APPROX> >(SIZE,"clock_gettime_nsec monotonic_approx");
#endif

#ifndef _WIN32
    test_setitimer(SIZE,"setitimer");
    test_getitimer(SIZE,"getitimer");
#endif

#if defined(_MACHTYPES_H_)
    test_noarg_retval<uint64_t, clock_mach_absolute_time >(SIZE,"mach_absolute_time");
    test_noarg_retval<uint64_t, clock_mach_approximate_time >(SIZE,"mach_approximate_time");
    test_noarg_retval<uint64_t, clock_mach_continuous_time >(SIZE,"mach_continuous_time");
    test_noarg_retval<uint64_t, clock_mach_continuous_approximate_time >(SIZE,"mach_continuous_approximate_time");
#endif

#ifdef _WIN32
    test_noarg_retval<WORD, clock_GetLocalTime >(SIZE,"GetLocalTime");
    test_noarg_retval<WORD, clock_GetSystemTime >(SIZE,"GetSystemTime");
    test_noarg_retval<WORD, clock_GetSystemTimeAsFileTime >(SIZE,"GetSystemTimeAsFileTime");
    test_noarg_retval<DWORD, clock_GetTickCount >(SIZE,"GetTickCount");
    test_noarg_retval<ULONGLONG, clock_GetTickCount64 >(SIZE,"GetTickCount64");
    test_noarg_retval<DWORD, clock_GetSystemTimes >(SIZE,"GetSystemTimes");
    test_noarg_retval<LONGLONG, clock_QueryPerformanceCounter >(SIZE,"QueryPerformanceCounter");
#endif

// C++11
    test_noarg_retval<double, clock_std_chrono< std::chrono::system_clock > >(SIZE,"std::system_clock");
    test_noarg_retval<double, clock_std_chrono< std::chrono::steady_clock > >(SIZE,"std::steady_clock");
    test_noarg_retval<double, clock_std_chrono< std::chrono::high_resolution_clock > >(SIZE,"std::high_resolution_clock");    // usually an alias for another clock

    
    summarize("clock_time", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );




    // test precision of most clocks
    printf("\n\n");
    test_timer_precision< clock_clock > ("clock");
    test_timer_precision< clock_time > ("time");

#ifndef _WIN32

    test_timer_precision< clock_gettimeofday > ("gettimeofday");
    test_timer_precision< clock_getrusage > ("getrusage");
    test_timer_precision< clock_times > ("times");

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined(__sun) && !defined(isBSD)
    test_timer_precision< clock_sysinfo > ("sysinfo uptime");
#endif

    test_timer_precision< clock_clock_gettime<CLOCK_REALTIME> >("clock_gettime realtime");
    test_timer_precision< clock_clock_gettime<CLOCK_MONOTONIC> >("clock_gettime monotonic");
#if !defined(__sun) && !defined(isBSD)
    test_timer_precision< clock_clock_gettime<CLOCK_MONOTONIC_RAW> >("clock_gettime monotonic_raw");
#endif
    test_timer_precision< clock_clock_gettime<CLOCK_PROCESS_CPUTIME_ID> >("clock_gettime process_cputime");
    test_timer_precision< clock_clock_gettime<CLOCK_THREAD_CPUTIME_ID> >("clock_gettime thread_cputime");

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined(__sun) && !defined(isBSD)
    test_timer_precision< clock_clock_gettime<CLOCK_REALTIME_COARSE> >("clock_gettime realtime_coarse");
    test_timer_precision< clock_clock_gettime<CLOCK_MONOTONIC_COARSE> >("clock_gettime monotonic_coarse");
    test_timer_precision< clock_clock_gettime<CLOCK_BOOTTIME> >("clock_gettime boottime");
#endif

#if defined(__sun)
    test_timer_precision< clock_clock_gettime<CLOCK_HIGHRES> >("clock_gettime highres");
#endif

#if defined(_MACHTYPES_H_)
    test_timer_precision< clock_clock_gettime<CLOCK_UPTIME_RAW> >("clock_gettime uptime_raw");
    test_timer_precision< clock_clock_gettime<CLOCK_UPTIME_RAW_APPROX> >("clock_gettime uptime_approx");
    test_timer_precision< clock_clock_gettime<CLOCK_MONOTONIC_RAW_APPROX> >("clock_gettime monotonic_approx");
    
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_REALTIME> >("clock_gettime_nsec realtime");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_MONOTONIC> >("clock_gettime_nsec monotonic");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_MONOTONIC_RAW> >("clock_gettime_nsec monotonic_raw");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_PROCESS_CPUTIME_ID> >("clock_gettime_nsec process_cputime");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_THREAD_CPUTIME_ID> >("clock_gettime_nsec thread_cputime");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_UPTIME_RAW> >("clock_gettime_nsec uptime_raw");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_UPTIME_RAW_APPROX> >("clock_gettime_nsec uptime_approx");
    test_timer_precision< clock_clock_gettime_nsec<CLOCK_MONOTONIC_RAW_APPROX> >("clock_gettime_nsec monotonic_approx");
#endif

#if defined(_MACHTYPES_H_)
    test_timer_precision< clock_mach_absolute_time >("mach_absolute_time");
    test_timer_precision< clock_mach_approximate_time >("mach_approximate_time");
    test_timer_precision< clock_mach_continuous_time >("mach_continuous_time");
    test_timer_precision< clock_mach_continuous_approximate_time >("mach_continuous_approximate_time");
#endif

#else   // !_WIN32
    test_timer_precision< clock_GetLocalTime >("GetLocalTime");
    test_timer_precision< clock_GetSystemTime >("GetSystemTime");
    test_timer_precision< clock_GetSystemTimeAsFileTime >("GetSystemTimeAsFileTime");
    test_timer_precision< clock_GetTickCount >("GetTickCount");
    test_timer_precision< clock_GetTickCount64 >("GetTickCount64");
    test_timer_precision< clock_GetSystemTimes >("GetSystemTimes");
    test_timer_precision< clock_QueryPerformanceCounter >("QueryPerformanceCounter");
#endif

// C++11
    test_timer_precision< clock_std_chrono< std::chrono::system_clock > >("std::system_clock");
    test_timer_precision< clock_std_chrono< std::chrono::steady_clock > >("std::steady_clock");
    test_timer_precision< clock_std_chrono< std::chrono::high_resolution_clock > >("std::high_resolution_clock");





    // test ASCII conversions of time values, and other misc conversions
    printf("\n\n");
    test_asctime(SIZE,"asctime");
    test_ctime(SIZE,"ctime");
    test_strftime(SIZE,"strftime");
    test_difftime(SIZE,"diffftime");
    test_localtime(SIZE,"localtime");
    test_gmtime(SIZE,"gmtime");
    test_mktime(SIZE,"mktime");

#if defined(_MACHTYPES_H_)
    test_time2posix(SIZE,"time2posix");
    test_posix2time(SIZE,"posix2time");
#endif

#ifndef _WIN32
    test_timegm(SIZE,"timegm");
    test_timelocal(SIZE,"timelocal");
    test_asctimer(SIZE,"asctime_r");
    test_ctimer(SIZE,"ctime_r");
    test_localtimer(SIZE,"localtime_r");
    test_gmtimer(SIZE,"gmtime_r");
#endif

    summarize("clock_time conversions", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
