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
#CC = gcc
#CXX = g++


# GCC 8.2
#CC = gcc-8
#CXX = g++-8



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
stepanov_inherit \
stepanov_array \
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
shift \
absolute_value \
divide \
loop_normalize \
random_numbers \
clock_time \
inner_product \
rotate_bits \
rtti \
locales \
lookup_table \
loop_induction \
loop_removal \
minmax \
bitarrays \
histogram \
iostreams \
loop_fusion \
count_sequence \
memset \
reference_normalization \
simple_types_algebraic_simplification





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
	$(CC) $(CFLAGS) -o $@ exceptions.c $(CLIBS)

exceptions_cpp : exceptions.c
	$(CXX) $(CPPFLAGS) -D TEST_WITH_EXCEPTIONS=1 -o $@ exceptions.c $(CPPLIBS)




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
	./stepanov_inherit >> $(REPORT_FILE)
	./stepanov_array >> $(REPORT_FILE)
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
	./absolute_value >> $(REPORT_FILE)
	./divide >> $(REPORT_FILE)
	./loop_normalize >> $(REPORT_FILE)
	./random_numbers >> $(REPORT_FILE)
	./clock_time >> $(REPORT_FILE)
	./inner_product >> $(REPORT_FILE)
	./rotate_bits >> $(REPORT_FILE)
	./rtti >> $(REPORT_FILE)
	./locales >> $(REPORT_FILE)
	./lookup_table >> $(REPORT_FILE)
	./loop_induction >> $(REPORT_FILE)
	./loop_removal >> $(REPORT_FILE)
	./minmax >> $(REPORT_FILE)
	./bitarrays >> $(REPORT_FILE)
	./histogram >> $(REPORT_FILE)
	./iostreams $(REPORT_FILE) $(iostreamtemp)
	rm $(iostreamtemp)
	./loop_fusion >> $(REPORT_FILE)
	./count_sequence >> $(REPORT_FILE)
	./memset >> $(REPORT_FILE)
	./reference_normalization >> $(REPORT_FILE)
	./simple_types_algebraic_simplification >> $(REPORT_FILE)
	date >> $(REPORT_FILE)
	echo "##END Version 1.0" >> $(REPORT_FILE)

