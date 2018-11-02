/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )

    This test file started as ideas from
        ISO/IEC TR 18015:2006(E) Appendix D.5


Goal:  Compare the performance of standard iostreams and C-style stdio for simple IO.
        Also test any overhead of sync_with_stdio(true).


Assumptions:
    1) Basic IO should be fast to/from console and files.

    2) iostreams should be about the same speed as stdio function calls, to/from console and files.

    3) Using sync=true for iostreams should only have an impact on console IO.
 
    4) Posix low level IO functions will generally be slower due to lack of buffering.
 
    5) using std::endl will generally be slower than outputting '\n', because std::endl causes a stream flush.
        (this really should be revisited by the standards committee -- it is causing confusion and very poor performance)


**** NOTE: Order of tests and sync calls has an effect with GCC, sync=true must be tested before sync=false

*/

/******************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
#include <ctime>
#include "benchmark_timer.h"
#include "benchmark_results.h"

#include <errno.h>
#include <fcntl.h>

// various semi-current flavors of BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

#if isBSD
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif

/******************************************************************************/

typedef enum testIOMode {
    MODE_INT = 1,
    MODE_HEX = 2,
    MODE_FLOAT = 3,
    MODE_WORD = 4,
    MODE_ENDL = 5,
    MODE_NEWLINE = 6,
} testIOMode;

uint64_t gGlobalSum = 0;

/******************************************************************************/
/******************************************************************************/


void test_stdio_out(int n, const char *filename, const string &label, const testIOMode mode )
{
    int i;
    
    // if a target file is specified, open it
    FILE * stdio_target;
    stdio_target = stdout;
    if (filename != NULL) { // place output in file
        stdio_target = fopen( filename, "w" );
    }
    

    start_timer();
    switch( mode ) {
        case MODE_INT:
            gGlobalSum = 0;
            for (i = 0; i < n; ++i)
                {
                fprintf ( stdio_target, "%d ", i);
                gGlobalSum += i;
                }
            break;
        case MODE_HEX:
            for (i = 0; i < n; ++i)
                {
                fprintf ( stdio_target, "%x ", i);
                }
            break;
        case MODE_FLOAT:
            for (i = 0; i < n; ++i)
                {
                fprintf ( stdio_target, "%lf ", (double)i);
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                fprintf ( stdio_target, "%s ", tempString.c_str() );
                }
            break;
        case MODE_ENDL:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                fprintf ( stdio_target, "%s\n", tempString.c_str() );
                fflush( stdio_target ); // mimic behavior of endl
                }
            break;
        case MODE_NEWLINE:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                fprintf ( stdio_target, "%s\n", tempString.c_str() );
                }
            break;
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }
    record_result( timer(), label.c_str() );
    
    fprintf(stdio_target,"\n\n");
    fflush(stdio_target);
    if (filename != NULL)
        fclose(stdio_target);
}

/******************************************************************************/

void test_stdio_in(int n, const char *filename, const string &label, const testIOMode mode )
{
    int i;
    
    // if a target file is specified, open it
    FILE * stdio_target;
    stdio_target = stdin;
    if (filename != NULL) { // place output in file
        stdio_target = fopen( filename, "r" );
    }

    uint64_t sum = 0;
    start_timer();
    switch( mode ) {
        case MODE_INT:
            for (i = 0; i < n; ++i)
                {
                int temp;
                if (1 != fscanf ( stdio_target, "%d ", &temp))
                    break;
                sum += temp;
                }
            break;
        case MODE_HEX:
            for (i = 0; i < n; ++i)
                {
                int temp;
                if (1 != fscanf ( stdio_target, "%x ", &temp))
                    break;
                sum += temp;
                }
            break;
        case MODE_FLOAT:
            for (i = 0; i < n; ++i)
                {
                double temp;
                if (1 != fscanf ( stdio_target, "%lf ", &temp))
                    break;
                sum += uint64_t(temp);
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                char tempString[1000];
                if (1 != fscanf ( stdio_target, "%s ", tempString))
                    break;
                sum += atol( tempString );
                }
            break;
        case MODE_ENDL:
        case MODE_NEWLINE:
            for (i = 0; i < n; ++i)
                {
                char tempString[1000];
                if (1 != fscanf ( stdio_target, "%s\n", tempString))
                    break;
                sum += atol( tempString );
                }
            break;
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }
    
    if (sum != gGlobalSum) {
        //fprintf(stderr,"test %s failed (expect %llu, got %llu)\n", label.c_str(), gGlobalSum, sum);
        std::cerr << "test " << label << " failed, got " << sum << ", expected " << gGlobalSum << std::endl;
    }
    
    record_result( timer(), label.c_str() );
    
    if (filename != NULL)
        fclose(stdio_target);
}

#ifndef _WIN32
/******************************************************************************/

static void write_string(int posix_out, std::string m) {
    size_t len = m.length();
    const char *bytes = m.c_str();
    ssize_t written = write( posix_out, bytes, len );
    if (written != len) {
        fprintf(stderr,"write failed\n");
        exit(-2);
    }
}

/******************************************************************************/

void test_posix_out(int n, const char *filename, const string &label, const testIOMode mode )
{
    int i;
    
    // if a target file is specified, open it
    int posix_target = -1;
    if (filename != NULL) { // place output in file
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        posix_target = open( filename, O_CREAT | O_WRONLY | O_TRUNC, mode );
    }
    
    if (posix_target < 0) {
        fprintf(stderr,"Could not open %s for writing, errno %d\n", filename, errno);
        return;
    }
    

    start_timer();
    switch( mode ) {
        case MODE_INT:
            gGlobalSum = 0;
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i) + " ";
                write_string(posix_target,tempString);
                gGlobalSum += i;
                }
            break;
        case MODE_FLOAT:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string( (double)i ) + " ";
                write_string(posix_target,tempString);
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i) + " ";
                write_string(posix_target,tempString);
                }
            break;
        case MODE_HEX:
        case MODE_ENDL:
        case MODE_NEWLINE:
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }
    record_result( timer(), label.c_str() );
    
    close(posix_target);
    sleep(2);
}

/******************************************************************************/

static std::string read_string(int posix_in) {
    const size_t limit = 1000;
    size_t len = 0;
    char buf[limit];
    char temp[4];
    
    size_t tmp = read(posix_in,temp,1);
    if (tmp == 0) {
        fprintf(stderr,"Could not read input\n");
        exit(-1);
    }
    
    char last = temp[0];
    while (last != ' ' && last != '\n' && len < (limit-1)) {
        buf[len] = last;
        len++;
        tmp = read(posix_in,temp,1);
        if (tmp == 0)
            break;
        last = temp[0];
    }
    
    buf[len] = 0;

    return std::string(buf);
}

/******************************************************************************/
void test_posix_in(int n, const char *filename, const string &label, const testIOMode mode )
{
    int i;
    
    // if a target file is specified, open it
    int posix_target = -1;
    if (filename != NULL) { // place output in file
        posix_target = open( filename, O_RDONLY );
    }
    
    if (posix_target < 0) {
        fprintf(stderr,"Could not open %s for reading, errno %d\n", filename, errno );
        return;
    }


    uint64_t sum = 0;
    start_timer();
    switch( mode ) {
        case MODE_INT:
            for (i = 0; i < n; ++i)
                {
                std::string str = read_string(posix_target);
                int temp = std::stoi(str);                    // 49.66 sec
                //int temp = strtol( str.c_str(), NULL, 0 );  // 50.44 sec
                sum += temp;
                }
            break;
        case MODE_FLOAT:
            for (i = 0; i < n; ++i)
                {
                std::string str = read_string(posix_target);
                double temp = std::stod(str);                 // 93.11 sec
                //double temp = strtod( str.c_str(), NULL );  // 96.07 sec
                sum += uint64_t(temp);
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                std::string str = read_string(posix_target);
                sum += atol( str.c_str() );             // 49.67 sec, 50.50 sec
                }
            break;
        case MODE_HEX:
        case MODE_ENDL:
        case MODE_NEWLINE:
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }

    if (sum != gGlobalSum) {
        //fprintf(stderr,"test %s failed (expect %llu, got %llu)\n", label.c_str(), gGlobalSum, sum);
        std::cerr << "test " << label << " failed, got " << sum << ", expected " << gGlobalSum << std::endl;
    }
    
    record_result( timer(), label.c_str() );
    
    close(posix_target);
    sleep(2);
}
#endif  // !_WIN32

/******************************************************************************/

void test_iostreams_out(int n, const char *filename, bool sync, const string &label, const testIOMode mode )
{
    int i;

    // if a target file is specified for stream IO, open it
    ofstream iostream_target;
    ostream* op = &cout;
    if (filename!= NULL) { // place output in file
        iostream_target.open(filename);
        op = &iostream_target;
    }
    ostream& out = *op;
    
    
    // this must be called before any output using the C++ stream objects
    // only applies to (cin, cout, cerr, clog, etc.), this should have no effect on file streams
    out.sync_with_stdio (sync);

    start_timer();
    switch( mode ) {
        case MODE_INT:
            out << dec;
            for ( i = 0; i < n; ++i)
                {
                out << i << ' ';
                }
            break;
        case MODE_HEX:
            out << hex;
            for ( i = 0; i < n; ++i)
                {
                out << i << ' ';
                }
            break;
        case MODE_FLOAT:
            out << dec;
            for ( i = 0; i < n; ++i)
                {
                out << double(i) << ' ';
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                out << tempString << " ";
                }
            break;
        case MODE_ENDL:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                out << tempString << std::endl;
                }
            break;
        case MODE_NEWLINE:
            for (i = 0; i < n; ++i)
                {
                std::string tempString = std::to_string(i);
                out << tempString << "\n";
                }
            break;
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }
    record_result( timer(), label.c_str() );

    out << "\n\n";
    out.flush();
}

/******************************************************************************/

void test_iostreams_in(int n, const char *filename, bool sync, const string &label, const testIOMode mode )
{
    int i;

    // if a target file is specified for stream IO, open it
    ifstream iostream_target;
    istream* op = &cin;
    if (filename!= NULL) { // place output in file
        iostream_target.open(filename);
        op = &iostream_target;
    }
    istream& input = *op;


    // this must be called before any output using the C++ stream objects
    // only applies to (cin, cout, cerr, clog, etc.), this should have no effect on file streams
    input.sync_with_stdio (sync);

    uint64_t sum = 0;
    start_timer();
    switch( mode ) {
        case MODE_INT:
            input >> dec;
            for (i = 0; i < n; ++i)
                {
                int temp;
                input >> temp;
                sum += temp;
                }
            break;
        case MODE_HEX:
            input >> hex;
            for (i = 0; i < n; ++i)
                {
                unsigned int temp;
                input >> temp;
                sum += temp;
                }
            break;
        case MODE_FLOAT:
            input >> dec;
            for (i = 0; i < n; ++i)
                {
                double temp;
                input >> temp;
                sum += uint64_t(temp);
                }
            break;
        case MODE_WORD:
            for (i = 0; i < n; ++i)
                {
                std::string tempString;
                input >> tempString;
                sum += atol( tempString.c_str() );
                }
            break;
        case MODE_ENDL:
        case MODE_NEWLINE:
            for (i = 0; i < n; ++i)
                {
                std::string tempString;
                input >> tempString;
                sum += atol( tempString.c_str() );
                }
            break;
        default:
            fprintf (stderr, "Unknown mode %d", int(mode) );
            exit(-1);
            break;
    }
    if (sum != gGlobalSum)    {
        //fprintf(stderr,"test %s failed (expect %llu, got %llu)\n", label.c_str(), gGlobalSum, sum);
        std::cerr << "test " << label << " failed, got " << sum << ", expected " << gGlobalSum << std::endl;
    }
    record_result( timer(), label.c_str() );

}

/******************************************************************************/
/******************************************************************************/

// TODO - put this in benchmark_results.h

void summarize2(FILE *out, const char *name, int size, int iterations, int show_penalty ) {
    int i;
    const double millions = ((double)(size) * iterations)/1000000.0;
    double total_absolute_times = 0.0;
    double gmean_ratio = 0.0;
    
    
    if (out == NULL)
        out = stdout;
    
    /* find longest label so we can adjust formatting
        12 = strlen("description")+1 */
    int longest_label_len = 12;
    for (i = 0; i < current_test; ++i) {
        int len = (int)strlen(results[i].label);
        if (len > longest_label_len)
            longest_label_len = len;
    }

    fprintf(out,"\ntest %*s description   absolute   operations   ratio with\n", longest_label_len-12, " ");
    fprintf(out,"number %*s time       per second   test0\n\n", longest_label_len, " ");

    for (i = 0; i < current_test; ++i) {
        const double timeThreshold = 1.0e-4;
        double timeRatio = 1.0;
        double speed = 1.0;
        
        if (results[0].time < timeThreshold) {
            if(results[i].time < timeThreshold)
                timeRatio = 1.0;
            else
                timeRatio = INFINITY;
        } else
            timeRatio = results[i].time/results[0].time;
        
        if (results[i].time < timeThreshold) {
            speed = INFINITY;
        } else
            speed = millions/results[i].time;
        
        fprintf(out,"%2i %*s\"%s\"  %5.2f sec   %5.2f M     %.2f\n",
                i,
                (int)(longest_label_len - strlen(results[i].label)),
                "",
                results[i].label,
                results[i].time,
                speed,
                timeRatio);
    }

    // calculate total time
    for (i = 0; i < current_test; ++i) {
        total_absolute_times += results[i].time;
    }

    // report total time
    fprintf(out,"\nTotal absolute time for %s: %.2f sec\n", name, total_absolute_times);

    if ( current_test > 1 && show_penalty ) {
    
        // calculate gmean of tests compared to baseline
        for (i = 1; i < current_test; ++i) {
            gmean_ratio += log(results[i].time/results[0].time);
        }
        
        // report gmean of tests as the penalty
        fprintf(out,"\n%s Penalty: %.2f\n\n", name, exp(gmean_ratio/(current_test-1)));
    }

    // reset the test counter so we can run more tests
    current_test = 0;
}

/******************************************************************************/
/******************************************************************************/

int main (int argc, char *argv[])
{
    int i; // for-loop variable
    const char defaultName[] = "test_tmp.txt";

    // report that a filename is required
    if (argc < 2) {
        printf("usage: %s reportfile [outputfile] [count]\n", argv[0] );
        exit(1);
    }

    char *reportfilename = argv[1];
    const char *filename = (2 < argc) ? argv[2] : defaultName;
    const int count = (3 < argc) ? atoi(argv[3]) : 5000000;
    
    
    // open up our reporting file (so we can catch failures early)
    FILE * report_file;
    report_file = stderr;    // just in case
    if (reportfilename != NULL) {
        report_file = fopen( reportfilename, "a" );
        if (report_file == NULL)
            {
            printf("Coould not open %s for report\n", reportfilename );
            exit(-2);
            }
    }

    // output command for documentation:
    for (i = 0; i < argc; ++i)
        fprintf( report_file, "%s ", argv[i]) ;
    fprintf( report_file, "\n\n" );



    // do the tests

    string labelStdio ("fprintf integers to stdio");
    test_stdio_out(count, NULL, labelStdio, MODE_INT );

    string labelStdio2 ("fprintf hex to stdio");
    test_stdio_out(count, NULL, labelStdio2, MODE_HEX );

    string labelStdio3 ("fprintf float to stdio");
    test_stdio_out(count, NULL, labelStdio3, MODE_FLOAT );

    string labelStdio4 ("fprintf words to stdio");
    test_stdio_out(count, NULL, labelStdio4, MODE_WORD );

    string labelStdio5 ("fprintf words endl to stdio");
    test_stdio_out(count, NULL, labelStdio5, MODE_ENDL );

    string labelStdio6 ("fprintf words newline to stdio");
    test_stdio_out(count, NULL, labelStdio6, MODE_NEWLINE );
    
    
    string labelStdioF ("fprintf integers to file");
    test_stdio_out(count, filename, labelStdioF, MODE_INT );
    string labelStdioF4 ("fscanf integers from file");
    test_stdio_in(count, filename, labelStdioF4, MODE_INT );

    string labelStdioF2 ("fprintf hex to file");
    test_stdio_out(count, filename, labelStdioF2, MODE_HEX );
    string labelStdioF5 ("fscanf hex from file");
    test_stdio_in(count, filename, labelStdioF5, MODE_HEX );

    string labelStdioF3 ("fprintf float to file");
    test_stdio_out(count, filename, labelStdioF3, MODE_FLOAT );
    string labelStdioF6 ("fscanf float from file");
    test_stdio_in(count, filename, labelStdioF6, MODE_FLOAT );

    string labelStdioF7 ("fprintf words to file");
    test_stdio_out(count, filename, labelStdioF7, MODE_WORD );
    string labelStdioF8 ("fscanf words from file");
    test_stdio_in(count, filename, labelStdioF8, MODE_WORD );

    string labelStdioF9 ("fprintf words endl to file");
    test_stdio_out(count, filename, labelStdioF9, MODE_ENDL );
    string labelStdioF10 ("fscanf words endl from file");
    test_stdio_in(count, filename, labelStdioF10, MODE_ENDL );

    string labelStdioF11 ("fprintf words newline to file");
    test_stdio_out(count, filename, labelStdioF11, MODE_NEWLINE );
    string labelStdioF12 ("fscanf words newline from file");
    test_stdio_in(count, filename, labelStdioF12, MODE_NEWLINE );
    
    
    string labelstreamInt ("iostream integers (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt, MODE_INT );
    
    string labelstreamInt2 ("iostream hex (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt2, MODE_HEX );
    
    string labelstreamInt3 ("iostream float (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt3, MODE_FLOAT );
    
    string labelstreamInt4 ("iostream words (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt4, MODE_WORD );
    
    string labelstreamInt5 ("iostream words endl (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt5, MODE_ENDL );
    
    string labelstreamInt6 ("iostream words newline (sync = true) to stdio");
    test_iostreams_out(count, NULL, true, labelstreamInt6, MODE_NEWLINE );
    
    
    string labelstreamIntF ("iostream integers (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF, MODE_INT );
    string labelstreamIntF4 ("iostream integers (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF4, MODE_INT );
    
    string labelstreamIntF2 ("iostream hex (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF2, MODE_HEX );
    string labelstreamIntF5 ("iostream hex (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF5, MODE_HEX );
    
    string labelstreamIntF3 ("iostream float (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF3, MODE_FLOAT );
    string labelstreamIntF6 ("iostream float (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF6, MODE_FLOAT );
    
    string labelstreamIntF7 ("iostream words (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF7, MODE_WORD );
    string labelstreamIntF8 ("iostream words (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF8, MODE_WORD );
    
    string labelstreamIntF9 ("iostream words endl (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF9, MODE_ENDL );
    string labelstreamIntF10 ("iostream words endl (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF10, MODE_ENDL );
    
    string labelstreamIntF11 ("iostream words newline (sync = true) to file");
    test_iostreams_out(count, filename, true, labelstreamIntF11, MODE_NEWLINE );
    string labelstreamIntF12 ("iostream words newline (sync = true) from file");
    test_iostreams_in(count, filename, true, labelstreamIntF12, MODE_NEWLINE );
    
    
    string labelstreamIntA ("iostream integers (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA, MODE_INT );
    
    string labelstreamIntA2 ("iostream hex (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA2, MODE_HEX );
    
    string labelstreamIntA3 ("iostream float (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA3, MODE_FLOAT );
    
    string labelstreamIntA4 ("iostream words (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA4, MODE_WORD );
    
    string labelstreamIntA5 ("iostream words endl (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA5, MODE_ENDL );
    
    string labelstreamIntA6 ("iostream words newline (sync = false) to stdio");
    test_iostreams_out(count, NULL, false, labelstreamIntA6, MODE_NEWLINE );

    
    string labelstreamIntB ("iostream integers (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB, MODE_INT );
    string labelstreamIntB4 ("iostream integers (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB4, MODE_INT );
    
    string labelstreamIntB2 ("iostream hex (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB2, MODE_HEX );
    string labelstreamIntB5 ("iostream hex (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB5, MODE_HEX );
    
    string labelstreamIntB3 ("iostream float (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB3, MODE_FLOAT );
    string labelstreamIntB6 ("iostream float (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB6, MODE_FLOAT );
    
    string labelstreamIntB7 ("iostream words (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB7, MODE_WORD );
    string labelstreamIntB8 ("iostream words (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB8, MODE_WORD );
    
    string labelstreamIntB9 ("iostream words endl (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB9, MODE_ENDL );
    string labelstreamIntB10 ("iostream words endl (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB10, MODE_ENDL );
    
    string labelstreamIntB11 ("iostream words newline (sync = false) to file");
    test_iostreams_out(count, filename, false, labelstreamIntB11, MODE_NEWLINE );
    string labelstreamIntB12 ("iostream words newline (sync = false) from file");
    test_iostreams_in(count, filename, false, labelstreamIntB12, MODE_NEWLINE );

    // output results
    summarize2(report_file, "iostreams", 1, count, false );


#ifndef _WIN32
    // These are REALLY slow on Linux (was like 7000 seconds)
    // But we can make operations/second still comparable, while keeping total time sort of reasonable.
    const int posix_scale = 18;

    string labelPosixF ("posix integers to file");
    test_posix_out(count/posix_scale, filename, labelPosixF, MODE_INT );
    string labelPosixF4 ("posix integers from file");
    test_posix_in(count/posix_scale, filename, labelPosixF4, MODE_INT );
    
    // can't do hex IO as easily here

    string labelPosixF3 ("posix float to file");
    test_posix_out(count/posix_scale, filename, labelPosixF3, MODE_FLOAT );
    string labelPosixF6 ("posix float from file");
    test_posix_in(count/posix_scale, filename, labelPosixF6, MODE_FLOAT );

    string labelPosixF7 ("posix words to file");
    test_posix_out(count/posix_scale, filename, labelPosixF7, MODE_WORD );
    string labelPosixF8 ("posix words from file");
    test_posix_in(count/posix_scale, filename, labelPosixF8, MODE_WORD );
    
    // output results
    summarize2(report_file, "iostreams posix", 1, count/posix_scale, false );
#endif  // !_WIN32


    // done with reports
    fclose(report_file);
    
    // try to clean up
    remove(filename);

    return 0;
}

/******************************************************************************/
/******************************************************************************/

