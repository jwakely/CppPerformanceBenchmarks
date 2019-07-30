/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


The purpose of this source file is to report information about the compiler,
OS and machine running the benchmark.

When adding reporting for your compiler, OS and CPU:
    Please remember that this source file has to compile everywhere else as well.


All trademarks used herein are the property of their owner, and are only used
for correct identification of their products.


See https://sourceforge.net/p/predef/wiki/OperatingSystems/ for some older compilers and architectures.
See source for Unix hostinfo.
See https://gist.github.com/hi2p-perim/7855506  for Intel CPUID (not portable!)

*/

/******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "benchmark_stdint.hpp"


// various semi-current flavors of BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

// one of these should be defined on Linux derived OSes
#if defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)
#define isLinux 1
#endif


// This should be defined on Mach derived OSes (MacOS, etc.)
// FreeBSD/NetBSD/OpenBSD also have sysctl, with different strings
#if defined(_MACHTYPES_H_) || isBSD
#include <sys/sysctl.h>
#include <strings.h>
#endif

#if isLinux || isBSD
#include <sys/utsname.h>
#include <unistd.h>
#include <strings.h>
#endif

#if isLinux
// BSD doesn't have this header or function
#include <sys/sysinfo.h>
#endif

// present on most Linux versions, missing from Solaris
#if isLinux && !defined (__sun)
#include <gnu/libc-version.h>
#endif

#if _WIN32
#include <windows.h>
#include <atlbase.h>
#include <lm.h>
#include <Lmwksta.h>
#include <Lmapibuf.h>
#include <intrin.h>
#endif


/******************************************************************************/

void VerifyTypeSizes()
{
    if (sizeof(int8_t) != 1)
        printf("Found size of int8_t was %d instead of 1\n", (int)sizeof(int8_t) );
    if (sizeof(uint8_t) != 1)
        printf("Found size of uint8_t was %d instead of 1\n", (int)sizeof(uint8_t) );
    if (sizeof(int16_t) != 2)
        printf("Found size of int16_t was %d instead of 2\n", (int)sizeof(int16_t) );
    if (sizeof(uint16_t) != 2)
        printf("Found size of uint16_t was %d instead of 2\n", (int)sizeof(uint16_t) );
    if (sizeof(int32_t) != 4)
        printf("Found size of int32_t was %d instead of 4\n", (int)sizeof(int32_t) );
    if (sizeof(uint32_t) != 4)
        printf("Found size of uint32_t was %d instead of 4\n", (int)sizeof(uint32_t) );
    if (sizeof(int64_t) != 8)
        printf("Found size of int64_t was %d instead of 8\n", (int)sizeof(int64_t) );
    if (sizeof(uint64_t) != 8)
        printf("Found size of uint64_t was %d instead of 8\n", (int)sizeof(uint64_t) );
    
    if (sizeof(float) != 4)
        printf("Found size of float was %d instead of 4\n", (int)sizeof(float) );
    if (sizeof(double) != 8)
        printf("Found size of double was %d instead of 8\n", (int)sizeof(double) );
}

/******************************************************************************/

// compiler version and any major targetting options (such as 32 vs 64 bit)
void ReportCompiler() 
{

    printf("##Compiler\n");

#if __INTEL_COMPILER

    printf("Intel Compiler version %d\n", __INTEL_COMPILER );
    printf("Build %d\n", __INTEL_COMPILER_BUILD_DATE );

    #if defined(_M_X64) || defined(__x86_64__) || defined(__WIN64__)
        printf("Compiling for Windows 64 bit\n" );
    #elif defined(__WIN32__) || defined(_WIN32)
        printf("Compiling for Windows 32 bit\n" );
    #endif

#elif _MSC_VER

    printf("Microsoft VisualC++ version %d\n", _MSC_VER );
    
    #if defined(_M_X64) || defined(__x86_64__) || defined(__WIN64__)
        printf("Compiling for Windows 64 bit\n" );
    #elif defined(__WIN32__) || defined(_WIN32)
        printf("Compiling for Windows 32 bit\n" );
    #endif
    
    #if defined(__CLR_VER)
        printf("CLR version %s\n", __CLR_VER );
    #endif

    /*
    See http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx#_predir_table_1..3
    */

#elif __GNUC__

    printf("GCC version %s\n", __VERSION__ );
    
    /*
    printf("%d.%d", __GNUC__, __GNUC_MINOR__ );
    #if defined(__GNUC_PATCHLEVEL__)
        printf(" . %d", __GNUC_PATCHLEVEL__ );
    #endif
    printf("\n");
    */
    
    #if __LP64__
        printf("Compiled for LP64\n");
    #endif
    
    
    /*
    Other useful macros:
    __OPTIMIZE__
    __OPTIMIZE_SIZE__
    __NO_INLINE__
    
    See http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html

    See http://developer.apple.com/documentation/DeveloperTools/gcc-4.0.1/cpp/Common-Predefined-Macros.html
    */
    
#elif __MWERKS__
    printf("Metrowerks CodeWarrior version 0x%8.8lX\n", __MWERKS__ );
#elif __MRC__
    printf("Apple MrC[pp] version 0x%8.8lX\n", __MRC__ );
#elif __MOTO__
    printf("Motorola MCC version 0x%8.8lX\n", __MOTO__ );
#else
    printf("********\n" );
    printf("Unknown compiler, please update %s for your compiler\n", __FILE__ );
    printf("********\n" );
#endif


#if isLinux && !defined (__sun)
    printf("glibc version: %s\n", gnu_get_libc_version());
#endif

}

/******************************************************************************/

// what kind of CPU is the compiler targetting?
void ReportCPUTarget()
{

    printf("##Target CPU\n");

#if _MANAGED

    printf("Compiled for Microsoft managed code (CLR)\n");

#elif defined(__arm__) || defined(__ARMEL__) || defined(__ARMEB__) \
    || defined(_M_ARM) || defined(__ARM_ARCH) \
    || defined(__thumb__) || defined(_M_ARMT) \
    || defined(__aarch64__)

    #if defined(__aarch64__)
        printf("Compiled for ARM 64bit\n");
    #elif defined(__thumb__) || defined(_M_ARMT)
        printf("Compiled for ARM Thumb\n");
    #else
        printf("Compiled for ARM\n");
    #endif
    
    #if __ARM_FP
        printf("Hardware floating point enabled\n");
    #endif
    
    #if __ARM_NEON_FP
        printf("ARRM Neon floating point enabled\n");
    #endif

#elif defined(__m68k__) || defined(__MC68K__)

    printf("Compiled for m68k\n");

#elif defined(_POWER)

    printf("Compiled for Power Series\n");

#elif defined(__ppc64__) || defined(__powerpc64__) || defined(__PPC64__) || defined(_ARCH_PPC64)

    printf("Compiled for PowerPC 64bit\n");

#elif defined(__powerc) || defined(__ppc__) || defined(powerpc) \
    || defined(ppc) || defined(_M_PPC) \
    || defined(__powerpc__)  || defined(__POWERPC__) \
    || defined(__PPC__)  || defined(_ARCH_PPC)

    printf("Compiled for PowerPC 32bit\n");

#elif defined(_M_IA64) || defined(__ia64__) \
    || defined(__IA64__) || defined(__itanium__)

    printf("Compiled for Itanium 64bit\n");

#elif defined(_M_X64) || defined(__x86_64__) || defined(__amd64__) \
    || defined(__amd64) || defined(_M_AMD64_)

    printf("Compiled for x86 64bit\n");

#elif defined(__i386__) || defined(i386) || defined(_X86_) || defined(_M_IX86)

    printf("Compiled for x86 32bit\n" );

    #if _M_IX86
    switch( _M_IX86) {
        case 300:
            printf("Compiled for 80386\n");
            break;
        case 400:
            printf("Compiled for 80486\n");
            break;
        case 500:
            printf("Compiled for Pentium\n");
            break;
        case 600:
            printf("Compiled for PentiumII\n");
            break;
        default:
            printf("********\n" );
            printf("Unknown x86 target, please update %s for your cpu\n", __FILE__ );
            printf("********\n" );
            break;
    }
    #endif

#elif defined(_ALPHA_) || defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA)

    printf("Compiled for Alpha\n");

#elif defined(__MIPS__) || defined(_MIPSEB) || defined (__MIPSEB) || defined(__MIPSEB__) \
        || defined(_MIPSEL) || defined (__MIPSEL) || defined(__MIPSEL__)

    printf("Compiled for MIPS\n");

#elif defined(_TMS320C2XX) || defined(__TMS320C2000__)

    printf("Compiled for TMS320 2xxx\n");

#elif defined(_TMS320C5X) || defined(__TMS320C55X__)

    printf("Compiled for TMS320 5xxx\n");

#elif defined(_TMS320C6X) || defined(__TMS320C6X__)

    printf("Compiled for TMS320 6xxx\n");

#elif defined(__TMS470__)

    printf("Compiled for TMS470\n");

#elif defined(__AVR_MEGA__) || defined(__AVR_ARCH__) || defined(__AVR) || defined(__AVR__)

    printf("Compiled for AVR\n");
    
    #if defined(__AVR_ARCH__)
        printf("AVR arch = %d\n", __AVR_ARCH__);
    #endif
    
    #if __AVR_HAVE_MUL__
        printf("Hardware multiply enabled\n");
    #endif

#elif defined(__riscv) || defined(__riscv__) \
    || defined(__RISCVEL) || defined(__RISCVEL__) \
    || defined(__RISCVEB) || defined(__RISCVEB__)

    printf("Compiled for RISC V\n");
    
    #if __riscv_hard_float
        printf("Hardware floating point enabled\n");
    #endif

#else
    printf("********\n" );
    printf("Unknown target CPU, please update %s for your cpu\n", __FILE__ );
    printf("********\n" );
#endif

}

/******************************************************************************/

// byte order of the CPU we're running on
void ReportEndian()
{
    static uint32_t cookie = 0x01020304;
    unsigned char *testPtr = (unsigned char *) &cookie;
    
    if (*testPtr == 0x01) {
        printf( "CPU is Big Endian\n" );
    } else if (*testPtr == 0x04) {
        printf( "CPU is Little Endian\n" );
    } else {
        printf("********\n" );
        printf("Unknown byteorder, please update %s for your cpu\n", __FILE__ );
        printf("********\n" );
    }
}

/******************************************************************************/

#if isLinux
void parseLinuxCPUInfo()
{
    const int buffersize = 4096;
    char textBuffer[ buffersize ];
    
    FILE *procinfo = fopen("/proc/cpuinfo","r");
    if (procinfo == NULL) {
        printf("ERROR: could not open /proc/cpuinfo\n");
        return;
    }

    // get useful lines from cpuinfo
    while ( fgets(textBuffer, buffersize, procinfo) != NULL ) {
        if (strstr(textBuffer, "vendor_id")
        || strstr(textBuffer, "cpu family")
        || strstr(textBuffer, "model")         // model and model name
        || strstr(textBuffer, "stepping")
        || strstr(textBuffer, "microcode")
        || strstr(textBuffer, "cpu MHz")
        || strstr(textBuffer, "cache")          // cache and cache alignment
        || strstr(textBuffer, "fpu")            // fpu and fpu exceptions
        || strstr(textBuffer, "flags")  ) {
            int len = strlen(textBuffer);
            textBuffer[len-1] = 0;  // remove trailing newline
            puts(textBuffer);
        }
    
        // stop after first processor == stop on first blank line
        if (textBuffer[0] == '\r' || textBuffer[0] == '\n')
            break;
    }
    
    fclose(procinfo);
    
    
    
    // iterate over cache levels, painfully
    for (int L = 0; L < 10; ++L) {
        char levelBuffer[ buffersize ];
        char filename[ 1024 ];
        sprintf(filename, "/sys/devices/system/cpu/cpu0/cache/index%1d/level", L );
    
        FILE *cacheinfo = fopen(filename,"r");
        if (cacheinfo == NULL) {
            break;
        }
        if ( fgets(levelBuffer, buffersize, cacheinfo) != NULL ) {
            int len = strlen(levelBuffer);
            levelBuffer[len-1] = 0;  // remove trailing newline
        }
        fclose(cacheinfo);
        
        
        sprintf(filename, "/sys/devices/system/cpu/cpu0/cache/index%1d/size", L );
    
        FILE *sizeinfo = fopen(filename,"r");
        if (sizeinfo == NULL) {
            break;
        }
        if ( fgets(textBuffer, buffersize, sizeinfo) != NULL ) {
            int len = strlen(textBuffer);
            textBuffer[len-1] = 0;  // remove trailing newline
        }
        fclose(sizeinfo);
        
        printf("Cache Level %s = %s\n", levelBuffer, textBuffer );
    }

}
#endif //   isLinux


/******************************************************************************/

    
// what CPU are we actually running on
// architecture, revision, speed

// methods for obtaining this information are probably OS specific
    
void ReportCPUPhysical()
{
    const int one_million = 1000000L;
    
    printf("##Physical CPU\n");



// this should work for any Mach based OS (MacOS, etc.)
#if defined(_MACHTYPES_H_)

// NOTE - use command line "sysctl hw"
// to get a list of known strings

// see sysctl.h for the definitions
    {
    long returnBuffer=0, retval=0;
    long long bigBuffer = 0;
    char textBuffer[1024];
    size_t len;
    
    // this gets us the CPU family, but not the exact CPU model and rev!
    len = 4;
    retval = sysctlbyname("hw.cputype", &returnBuffer, &len, NULL, 0);
    if (retval == 0) {
        printf("Mach CPU type %ld\n", returnBuffer );
    
        // from sys/machine.h
        switch(returnBuffer) {
            case 1:
                printf("CPU_TYPE VAX\n");
                break;
            case 6:
                printf("CPU_TYPE MC680x0\n");
                break;
            case 7:
                printf("CPU_TYPE x86\n");
                break;
            case 8:
                printf("CPU_TYPE MIPS\n");
                break;
            case 10:
                printf("CPU_TYPE MC98000\n");
                break;
            case 11:
                printf("CPU_TYPE HPPA\n");
                break;
            case 12:
                printf("CPU_TYPE ARM\n");
                break;
            case 13:
                printf("CPU_TYPE MC8880x0\n");
                break;
            case 14:
                printf("CPU_TYPE SPARC\n");
                break;
            case 15:
                printf("CPU_TYPE i860\n");
                break;
            case 16:
                printf("CPU_TYPE Alpha\n");
                break;
            case 18:
                printf("CPU_TYPE PowerPC\n");
                break;
            default:
                printf("********\n" );
                printf("Unknown Mach CPU Type, please update %s for your cpu\n", __FILE__ );
                printf("********\n" );
                break;
        }
    
    }
    
    // corresponds to CPU types, but the list is kinda big and dependent on CPU major type
    len = 4;
    retval = sysctlbyname("hw.cpusubtype", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU subtype %ld\n", returnBuffer );
    
    len = 1024;
    retval = sysctlbyname("machdep.cpu.brand_string", textBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU brand string: %s\n", textBuffer );
    
    len = 4;
    retval = sysctlbyname("machdep.cpu.family", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU family %ld\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("machdep.cpu.model", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU model %ld\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("machdep.cpu.extfamily", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU extfamily %ld\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("machdep.cpu.stepping", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU stepping %ld\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("machdep.cpu.microcode_version", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Mach CPU microcode_version %ld\n", returnBuffer );
    
    len = 8;
    retval = sysctlbyname("hw.cpufrequency_max", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU frequency: %.2f Mhz\n", (double)bigBuffer/one_million );

    len = 8;
    retval = sysctlbyname("hw.cachelinesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU cache linesize: %lld bytes\n", bigBuffer );
    
    len = 8;
    retval = sysctlbyname("hw.l1dcachesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU L1 Dcache: %lld bytes\n", bigBuffer );
    
    len = 8;
    retval = sysctlbyname("hw.l1icachesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU L1 Icache: %lld bytes\n", bigBuffer );
        
    len = 8;
    retval = sysctlbyname("hw.l2cachesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU L2 cache: %lld bytes\n", bigBuffer );
        
    len = 8;
    retval = sysctlbyname("hw.l3cachesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("CPU L3 cache: %lld bytes\n", bigBuffer );

    
    // PowerPC CPU extensions
    len = 4;
    retval = sysctlbyname("hw.optional.floatingpoint", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has optional floating point instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.altivec", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has AltiVec instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.64bitops", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has 64 bit instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.fsqrt", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has fsqrt instruction\n" );
    
    
    // x86 CPU extension
    len = 4;
    retval = sysctlbyname("hw.optional.mmx", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has MMX instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse2", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE2 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse3", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE3 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.supplementalsse3", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has supplemental SSE3 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse4", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE4 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse4_1", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE4_1 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse4_2", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE4_2 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.sse5", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has SSE5 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.avx1_0", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has AVX1_0 instructions\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.avx2_0", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has AVX2_0 instructions\n" );

    len = 4;
    retval = sysctlbyname("hw.optional.rdrand", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has rdrand\n" );
    
    len = 4;
    retval = sysctlbyname("hw.optional.x86_64", &returnBuffer, &len, NULL, 0);
    if (retval == 0 && returnBuffer != 0)
        printf("CPU has x86_64 instructions\n" );
    
    }
    
#endif    // _MACHTYPES_H_



// BSD
#if defined(isBSD) && !defined (__OpenBSD__)
//OpenBSD apparently only has sysctl, not sysctlbyname

// NOTE - use command line "sysctl hw"
// to get a list of known strings

// see sysctl.h for the definitions
    {
    long returnBuffer=0, retval=0;
    //long long bigBuffer = 0;
    char textBuffer[1024];
    size_t len;
    
    len = 4;
    retval = sysctlbyname("hw.ncpu", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("BSD CPU count %ld\n", returnBuffer );
    
    len = 1024;
    retval = sysctlbyname("hw.machine", textBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("BSD machine type %s\n", textBuffer );
    
    len = 1024;
    retval = sysctlbyname("hw.model", textBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("BSD model %s\n", textBuffer );
    
    len = 4;
    retval = sysctlbyname("hw.clockrate", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("BSD CPU clockrate %ld\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("hw.floatingpoint", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("BSD CPU has floating point %ld\n", returnBuffer );
    
    }

#endif  // isBSD


#if isLinux
    parseLinuxCPUInfo();
#endif


#ifdef _WIN32
// why the heck doesn't Microsoft have a real API for CPU information?
// GetSystemInfo is pretty anemic, and forcing us to use an Intel specific instruction is BAD.

    int32_t CPUInfo[4] = {-1};
    char CPUBrandString[ 64 ];

    __cpuid(CPUInfo, 0x80000000);

    const unsigned nExIds = CPUInfo[0];
    printf("CPU extended ids: 0x%8.8X\n", nExIds );

    // only 0x80000000 + 2,3,4 are valid brand strings
    // 0x80000006 has some cache info
    for (unsigned i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);
        
        // get CPU "brand" string
        if  (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if  (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if  (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    
    CPUBrandString[32+16] = 0;  // make sure it is NULL terminated

    printf("CPU brand string: %s\n", CPUBrandString );
    
    
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    if (info.dwNumberOfProcessors != 0)
        printf("Machine has %d CPUs\n", info.dwNumberOfProcessors );
    
    switch (info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            printf("CPU_TYPE AMD64\n");
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            printf("CPU_TYPE x86\n");
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            printf("CPU_TYPE IA64\n");
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            printf("CPU_TYPE ARM32\n");
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            printf("CPU_TYPE ARM64\n");
            break;
        default:
            printf("********\n" );
            printf("Unknown Win CPU architecture, please update %s for your cpu\n", __FILE__ );
            printf("********\n" );
            break;
    
    }

    printf("Processor Level: %d\n", info.wProcessorLevel );
    printf("Processor Revision: %d\n", info.wProcessorRevision );
    
#endif  // _WIN32
    

    // useful information, and not so dependent on the OS
    ReportEndian();
}

/******************************************************************************/

// format a number of bytes and print (without return)
void printMemSize( long long input )
{
    double meg = (double)input / (1024.0*1024.0);
    double tera = (double)input / (1024.0*1024.0*1024.0*1024.0);

    if (input < 1024) {    // format as bytes
        printf("%lld bytes", input );
    } else if (input < (1024*1024)) {    // format as KB
        printf("%.2f KBytes", (double)input/1024.0 );
    } else if (meg < 1024.0) {    // format as MB
        printf("%.2f MBytes", meg );
    } else if (meg < (1024.0*1024.0)) {    // format as GB
        printf("%.2f GBytes", meg/1024.0 );
    } else if (tera < (1024.0)) {    // format as TB
        printf("%.2f TeraBytes", tera );
    } else if (tera < (1024.0*1024.0)) {    // format as PB
        printf("%.2f PetaBytes", tera/1024.0 );
    } else {    // format as EB
        printf("%.2f ExaBytes", tera/(1024.0*1024.0) );
    }
}

/******************************************************************************/

// information about the machine, outside of the CPU
void ReportMachinePhysical()
{
    printf("##Machine\n");
    

// this should work for any Mach based OS (MacOS, etc.)
// and BSD for most entries - but there are alternate APIs used below for BSD
#if defined(_MACHTYPES_H_)

// see sysctl.h for the definitions
    {
    long returnBuffer=0, retval=0;
    long long bigBuffer = 0;
    size_t len;
    
    len = 4;
    returnBuffer = 0;
    retval = sysctlbyname("hw.ncpu", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Machine has %ld CPUs\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("hw.physicalcpu_max", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Machine has %ld physical CPUs\n", returnBuffer );
    
    len = 4;
    retval = sysctlbyname("hw.logicalcpu_max", &returnBuffer, &len, NULL, 0);
    if (retval == 0)
        printf("Machine has %ld logical CPUs\n", returnBuffer );
    
    
    len = 8;
    retval = sysctlbyname("hw.memsize", &bigBuffer, &len, NULL, 0);
    if (retval == 0) {
        printf("Machine has ");
        printMemSize( bigBuffer );
        printf(" of RAM\n");
        }
    
    len = 8;
    retval = sysctlbyname("hw.pagesize", &bigBuffer, &len, NULL, 0);
    if (retval == 0) {
        printf("Machine using ");
        printMemSize( bigBuffer );
        printf(" pagesize\n");
        }

    }
    
#endif    // _MACHTYPES_H_


// this should work on Linux
#if isLinux

    int nprocs = get_nprocs();
    if (nprocs != 0)
        printf("Machine has %d CPUs\n", nprocs );
    
    int nprocs_conf = get_nprocs_conf();
    if (nprocs_conf != 0)
        printf("Machine has %d CPUs configured\n", nprocs_conf );
    
#endif


// Linux, Solaris, *BSD
#if isLinux || isBSD

#if !defined (__sun) && !defined(isBSD)
// safe for Linux
    struct sysinfo info;
    int retval = sysinfo(&info);
    if (retval == 0) {
        long long temp = info.mem_unit * (long long)info.totalram;
        printf("Machine has ");
        printMemSize( temp );
        printf(" of RAM\n");
    }
#else
// safe for Solaris and BSD
    long pageCount = sysconf( _SC_PHYS_PAGES );
    long pageSize2  = sysconf( _SC_PAGE_SIZE );
    long long temp = (long long)pageCount * (long long)pageSize2;
        printf("Machine has ");
        printMemSize( temp );
        printf(" of RAM\n");
#endif

    int pageSize = getpagesize();
    if (pageSize != 0) {
        printf("Machine using ");
        printMemSize( pageSize );
        printf(" pagesize\n");
    }
#endif


// Windows
#ifdef _WIN32

    SYSTEM_INFO info;
    GetSystemInfo(&info);

    if (info.dwPageSize != 0) {
        printf("Machine using ");
        printMemSize( info.dwPageSize  );
        printf(" pagesize\n");
        }

    unsigned long long totalRam = 0;    // in kilobytes
    if (GetPhysicallyInstalledSystemMemory( &totalRam )) {        // This is failing on Windows 10 VM
        totalRam *= 1024;
    } else {
        MEMORYSTATUSEX gmem;
        gmem.dwLength = sizeof(gmem);
        if (GlobalMemoryStatusEx( &gmem )) {    // this works on Windows 10 VM
            totalRam = gmem.ullTotalPhys;
        }
    }
    
    if (totalRam != 0) {
        printf("Machine has ");
        printMemSize( totalRam );
        printf(" of RAM\n");
    }

#endif  // WIN32
    
}

/******************************************************************************/

void ReportOS()
{
    printf("##Operating System\n");


// this should work on various flavors of Linux and BSD
#if isLinux || defined(__sun) || isBSD
    {
    struct utsname buf;
    bzero( &buf, sizeof(buf) );
    int retval = uname( &buf );
    // successful return is zero on some OSes, and 1 on Solaris
    
        if (buf.sysname[0] != 0)
            printf("Kernel OS Name: %s\n", buf.sysname );
        // nodename is useless
        if (buf.release[0] != 0)
            printf("Kernel OS Release: %s\n", buf.release );
        if (buf.version[0] != 0)
            printf("Kernel OS Version: %s\n", buf.version );
        if (buf.machine[0] != 0)
            printf("Kernel OS Machine: %s\n", buf.machine );
    }
#endif


#if defined(__ANDROID_API__)
    printf("Android API version: %\n", __ANDROID_API__ );
#endif


// this should work for any Mach based OS (MacOS, etc.)
#if defined(_MACHTYPES_H_)
// see sysctl.h for the definitions
    {
    char string_buffer[1024];
    long retval=0;
    int mib[4];
    size_t len;
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_VERSION;
    len = sizeof(string_buffer);
    retval = sysctl(mib, 2, string_buffer, &len, NULL, 0);
    if (retval == 0)
        printf("Kernel OS Version: %s\n", string_buffer );
    
    }
#endif    // _MACHTYPES_H_


#if _WIN32
    {
    OSVERSIONINFO verInfo;
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    
    // NOTE - this API lies after Windows 8.1
    // the Microsoft "solution" is to add a new manifest UID entry for each supported OS (which means you cannot support future OSes!)
    if (GetVersionEx(&verInfo)) {
        printf("Windows GetVersionEx OS Version: %d.%d, build %d\n", verInfo.dwMajorVersion, verInfo.dwMinorVersion, verInfo.dwBuildNumber );
        if (verInfo.szCSDVersion[0] != 0)
            printf("Windows GetVersionEx update %s\n", verInfo.szCSDVersion );
        }
    
    // workaround 1
    LPBYTE rawData = NULL;
    NET_API_STATUS result = NetWkstaGetInfo( NULL, 100, &rawData );
    
    if ( result == ERROR_ACCESS_DENIED ) {
        printf("insufficient rights for NetWkstaGetInfo\n");
    } else if ( result == ERROR_INVALID_LEVEL ) {
        printf("invalid level for NetWkstaGetInfo\n");
        }
    else if ( result == 0 && rawData != NULL ) {   // NERR_Success
        WKSTA_INFO_100 *info = (WKSTA_INFO_100 *)rawData;
        printf("Windows NetWkstaGetInfo OS Version: %d.%d\n", info->wki100_ver_major, info->wki100_ver_minor );
        }
    if (rawData != NULL)
        NetApiBufferFree(rawData);
    
    
    // workaround 2
/*
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion
    \BuildLabEx        STRING
    \CurrentBuild      STRING
    \ProductName       STRING
    \CurrentMajorVersionNumber DWORD
    \CurrentMinorVersionNumber DWORD
*/
    const int bufferLen = 2000;
    char value[bufferLen];
    DWORD bufferSize = bufferLen;

    auto err = RegGetValueA(HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"),
            _T("BuildLabEx"),
            RRF_RT_ANY,
            NULL,
            value,
            &bufferSize);
    if (err == 0) {
        printf("Windows BuildLabEx OS Version: %s\n", value );
        }
    
    bufferSize = bufferLen;
    err = RegGetValueA(HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"),
            _T("CurrentBuild"),
            RRF_RT_ANY,
            NULL,
            value,
            &bufferSize);
    if (err == 0) {
        printf("Windows CurrentBuild OS Version: %s\n", value );
        }
    
    bufferSize = bufferLen;
    err = RegGetValueA(HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"),
            _T("ProductName"),
            RRF_RT_ANY,
            NULL,
            value,
            &bufferSize);
    if (err == 0) {
        printf("Windows ProductName: %s\n", value );
        }
    
    }

#endif

}

/******************************************************************************/

int main (int argc, char *argv[])
{
    // this should only be changed when the reporting tags have changed in an incompatible way
    const char version[] = "version 1.0";

    printf("##Start machine report %s\n", version );
    VerifyTypeSizes();
    ReportCompiler();
    ReportCPUTarget();
    ReportCPUPhysical();
    ReportMachinePhysical();
    ReportOS();
    printf("##End machine report\n");

    return 0;
}

/******************************************************************************/
/******************************************************************************/
