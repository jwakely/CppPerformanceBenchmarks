cmake_minimum_required(VERSION 3.8)

project(CppPerformanceBenchmarks LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable)
endif()

enable_testing()

macro(add_generic_build_target test_case)
	set(sources)
	set(source_candidates
		${PROJECT_SOURCE_DIR}/${test_case}.cpp
		${PROJECT_SOURCE_DIR}/${test_case}.c
	)

	foreach(source_candidate IN LISTS source_candidates)
		if(EXISTS ${source_candidate} AND NOT IS_DIRECTORY ${source_candidate})
			list(APPEND sources ${source_candidate})
		endif()
	endforeach()

	if(sources)
		add_executable(${test_case} ${sources})
	else()
		message(FATAL_ERROR "Unable to find any source file for test case '${test_case}'")
	endif()
endmacro()

macro(add_generic_test_target test_case)
	if(TARGET ${test_case})
		add_test(
			NAME    test.${test_case}
			COMMAND $<TARGET_FILE:${test_case}>
		)
	else()
		message(FATAL_ERROR "Compilation target not defined for test case '${test_case}'")
	endif()
endmacro()

macro(add_targets test_case)
	if("${test_case}" STREQUAL "exceptions")
		add_build_target_exceptions()
	elseif("${test_case}" STREQUAL "exceptions_cpp")
		add_build_target_exceptions_cpp()
	else()
		add_generic_build_target(${test_case})
	endif()

	if("${test_case}" STREQUAL "iostreams")
		add_test_target_iostreams()
	else()
		add_generic_test_target(${test_case})
	endif()
endmacro()

macro(add_build_target_exceptions)
	add_executable(exceptions ${PROJECT_SOURCE_DIR}/exceptions.c)
	if(UNIX)
		target_link_libraries(exceptions m)
	endif()
endmacro()

macro(add_build_target_exceptions_cpp)
	add_custom_command(
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/exceptions.c ${PROJECT_BINARY_DIR}/exceptions.cpp
		DEPENDS ${PROJECT_SOURCE_DIR}/exceptions.c
		OUTPUT  ${PROJECT_BINARY_DIR}/exceptions.cpp
	)
	add_executable(exceptions_cpp ${PROJECT_BINARY_DIR}/exceptions.cpp)
	target_compile_definitions(exceptions_cpp PUBLIC TEST_WITH_EXCEPTIONS=1)
endmacro()

macro(add_test_target_iostreams)
	add_test(
		NAME    test.iostreams
		COMMAND $<TARGET_FILE:iostreams> ${PROJECT_BINARY_DIR}/iostreams-report.txt
	)
endmacro()

set(test_cases
	machine
	stepanov_abstraction
	stepanov_array
	stepanov_inherit
	stepanov_vector
	loop_unroll
	simple_types_loop_invariant
	simple_types_constant_folding
	functionobjects
	atol
	ctype
	scalar_replacement_arrays
	scalar_replacement_array_reduction
	scalar_replacement_structs
	byte_order
	exceptions
	exceptions_cpp
	mathlib
	shift
	absolute_value
	divide
	loop_normalize
	random_numbers
	clock_time
	inner_product
	rotate_bits
	rtti
	locales
	lookup_table
	loop_induction
	loop_removal
	minmax
	bitarrays
	histogram
	iostreams
	loop_fusion
	count_sequence
	memset
	reference_normalization
	simple_types_algebraic_simplification
	complex_type
	custom_types_loop_invariant
	loop_interchange
	memcmp
	simple_types_constant_propagation
	memcpy
	simple_types_copy_propagation
	simple_types_cse
	simple_types_value_range
	memmove
	minmax_sequence
	pointer_loop_invariant
	smart_pointers
	convolution
	reverse_sequence
	simple_types_strength_reduction
	sum_sequence
	convolution_box
	logic_sequence
	matrix_vector_product
	product_sequence
	binary_search
	loop_unswitching
	template_unroll
)

foreach(test_case IN LISTS test_cases)
	message(STATUS "Adding test: ${test_case}")
	add_targets(${test_case})
endforeach()
