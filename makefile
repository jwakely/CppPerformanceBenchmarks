#
#    Copyright 2007-2010 Adobe Systems Incorporated
#    Copyright 2018 Chris Cox
#    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
#     or a copy at http://stlab.adobe.com/licenses.html )
#

###################################################
#
# Makefile for C++ benchmarks
#
###################################################


#
# Macros
#

INCLUDE = -I.

# GCC
CC = /usr/bin/gcc
CXX = /usr/bin/g++


# GCC 8.1
#CC = /usr/bin/gcc-8
#CXX = /usr/bin/g++-8



CFLAGS = $(INCLUDE) -O3
CPPFLAGS = -std=c++11 $(INCLUDE) -O3

CLIBS = -lm
CPPLIBS = -lm

DEPENDENCYFLAG = -M


#
# our target programs
#

BINARIES = machine \
stepanov_abstraction \
stepanov_vector \
loop_unroll \
simple_types_loop_invariant \
functionobjects \
simple_types_constant_folding \
atol \
ctype \
scalar_replacement_arrays \
scalar_replacement_array_reduction \
scalar_replacement_structs \
byte_order \
exceptions \
exceptions_cpp \
mathlib \
shift




#
# Build rules
#

all : $(BINARIES)


SUFFIXES:
.SUFFIXES: .c .cpp


# declare some targets to be fakes without real dependencies

.PHONY : clean dependencies

# remove all the stuff we build

clean : 
		rm -f *.o $(BINARIES)


# generate dependency listing from all the source files
# used for double checking problems with headers
# this does NOT go in the makefile

SOURCES = $(wildcard *.c)  $(wildcard *.cpp)
dependencies :   $(SOURCES)
	$(CXX) $(DEPENDENCYFLAG) $(CPPFLAGS) $^


#
# special case compiles
#

exceptions : exceptions.c
	$(CC) $(CFLAGS) -o $@ $< $(CLIBS)

exceptions_cpp : exceptions.c
	$(CXX) $(CPPFLAGS) -D TEST_WITH_EXCEPTIONS=1 -o $@ $< $(CPPLIBS)




#
# Run the benchmarks and generate a report
#
REPORT_FILE = report.txt

report:  $(BINARIES)
	echo "##STARTING Version 1.0" > $(REPORT_FILE)
	date >> $(REPORT_FILE)
	echo "##CFlags: $(CFLAGS)" >> $(REPORT_FILE)
	echo "##CPPFlags: $(CPPFLAGS)" >> $(REPORT_FILE)
	./machine >> $(REPORT_FILE)
	./stepanov_abstraction >> $(REPORT_FILE)
	./stepanov_vector >> $(REPORT_FILE)
	./functionobjects >> $(REPORT_FILE)
	./simple_types_constant_folding >> $(REPORT_FILE)
	./simple_types_loop_invariant >> $(REPORT_FILE)
	./loop_unroll >> $(REPORT_FILE)
	./atol >> $(REPORT_FILE)
	./ctype >> $(REPORT_FILE)
	./scalar_replacement_arrays >> $(REPORT_FILE)
	./scalar_replacement_array_reduction >> $(REPORT_FILE)
	./scalar_replacement_structs  >> $(REPORT_FILE)
	./byte_order >> $(REPORT_FILE)
	./exceptions >> $(REPORT_FILE)
	./exceptions_cpp >> $(REPORT_FILE)
	./mathlib >> $(REPORT_FILE)
	./shift >> $(REPORT_FILE)
	date >> $(REPORT_FILE)
	echo "##END Version 1.0" >> $(REPORT_FILE)

