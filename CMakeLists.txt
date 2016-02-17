project("insituc")

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    message(FATAL_ERROR "only clang supported currently")
endif()

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" COMPILER_SUPPORTS_LIBCXX)
if(NOT COMPILER_SUPPORTS_LIBCXX)
    message(FATAL_ERROR "libc++ not installed or libc++ not supported by clang++")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

CHECK_CXX_COMPILER_FLAG("-std=gnu++1z" COMPILER_SUPPORTS_CXX1Z)
if(NOT COMPILER_SUPPORTS_CXX1Z)
    message(FATAL_ERROR "Compiler does not support C++1z standard")
endif()
add_compile_options("-std=gnu++1z")

#set(CXX_EXTENSIONS ON)
#set_property(TARGET ${TARGETS} PROPERTY CXX_STANDARD 17)
#set_property(TARGET ${TARGETS} PROPERTY CXX_STANDARD_REQUIRED ON)
add_compile_options(-fconstexpr-steps=50000000 -ftemplate-depth=200)
add_compile_options(-ftemplate-backtrace-limit=0 -fconstexpr-backtrace-limit=0 -fdiagnostics-show-template-tree)
add_compile_options(-W -Weverything -Wmissing-include-dirs -Wconversion -Wmismatched-tags -Wuninitialized -Wfatal-errors -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded)
add_compile_options(-Wno-gnu-anonymous-struct -Wno-nested-anon-types)
#add_compile_options(-ftime-report)

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -fno-inline -DDEBUG=1 -D_DEBUG=1 -D_GLIBCXX_DEBUG=1 -march=x86-64 -mtune=generic")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG=1 -march=native")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -gline-tables-only -DNDEBUG=1 -march=native")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG=1 -march=native")
else()
    message(STATUS "Wrong build type selected, default to Debug.")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

include_directories("include/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPAGESIZE=\"`getconf PAGESIZE`\"")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed -Wl,-lunwind")

set(HEADERS
    "include/insituc/debug/demangle.hpp"
    "include/insituc/debug/backtrace.hpp"


    "include/insituc/utility/numeric/safe_convert.hpp"
    "include/insituc/utility/numeric/safe_compare.hpp"

    "include/insituc/utility/append.hpp"
    "include/insituc/utility/head.hpp"
    "include/insituc/utility/tail.hpp"
    "include/insituc/utility/reverse.hpp"
    "include/insituc/utility/static_const.hpp"
    "include/insituc/utility/to_string.hpp"


    "include/insituc/variant.hpp"


    "include/floating_point_type_wrapper.hpp"
    "include/insituc/floating_point_type.hpp"


    "include/insituc/base_types.hpp"
    "include/insituc/type_traits.hpp"
    "include/insituc/shunting_yard_algorithm.hpp"


    "include/insituc/ast/tokens.hpp"
    "include/insituc/ast/ast.hpp"
    "include/insituc/ast/adaptation.hpp"
    "include/insituc/ast/compare.hpp"
    "include/insituc/ast/io.hpp"


    "include/insituc/parser/base_types.hpp"
    "include/insituc/parser/annotation.hpp"
    "include/insituc/parser/skipper.hpp"
    "include/insituc/parser/parser.hpp"
    "include/insituc/parser/implementation/skipper.hpp"
    "include/insituc/parser/implementation/parser.hpp"
    "include/insituc/parser/pragma_parser.hpp"


    "include/insituc/transform/evaluator/subexpression.hpp"
    "include/insituc/transform/evaluator/intrinsic.hpp"
    "include/insituc/transform/evaluator/expression.hpp"
    "include/insituc/transform/evaluator/statement.hpp"
    "include/insituc/transform/evaluator/evaluator.hpp"

    "include/insituc/transform/derivator/context.hpp"
    "include/insituc/transform/derivator/intrinsic.hpp"
    "include/insituc/transform/derivator/derivator.hpp"

    "include/insituc/transform/optimizer/leaf.hpp"

    "include/insituc/transform/transform.hpp"


    "include/insituc/meta/base_types.hpp"
    "include/insituc/meta/mnemocodes.hpp"
    "include/insituc/meta/instructions.hpp"
    "include/insituc/meta/function.hpp"
    "include/insituc/meta/assembler.hpp"
    "include/insituc/meta/compiler.hpp"
    "include/insituc/meta/io.hpp"


    "include/insituc/memory/xallocator.hpp"

    "include/insituc/runtime/jit_compiler/base_types.hpp"
    "include/insituc/runtime/jit_compiler/instance.hpp"
    "include/insituc/runtime/jit_compiler/translator.hpp"

    "include/insituc/runtime/interpreter/base_types.hpp"
    "include/insituc/runtime/interpreter/virtual_machine.hpp"
    )

set(SOURCE_LIB
    "src/debug/demangle.cpp"
    "src/debug/backtrace_on_terminate.cpp"
    "src/debug/backtrace.cpp"

    "src/floating_point_type.cpp"

    "src/parser/skipper.cpp"
    "src/parser/parser.cpp"

    "src/transform/evaluator/subexpression.cpp"
    "src/transform/evaluator/intrinsic.cpp"
    "src/transform/evaluator/expression.cpp"
    "src/transform/evaluator/statement.cpp"
    "src/transform/evaluator/evaluator.cpp"

    "src/transform/derivator/intrinsic.cpp"
    "src/transform/derivator/derivator.cpp"

    "src/transform/optimizer/leaf.cpp"

    "src/transform/transform.cpp"

    "src/meta/assembler.cpp"
    "src/meta/compiler.cpp"

    "src/runtime/virtual_machine.cpp"
    "src/runtime/translator.cpp"
    )

add_library("insituc" STATIC ${SOURCE_LIB})
set_target_properties("insituc" PROPERTIES DEBUG_POSTFIX "d")

add_executable("test_parser"    "test/src/parser/parser_test.cpp"                 ${HEADERS})
add_executable("test_evaluator" "test/src/transform/evaluator/evaluator_test.cpp" ${HEADERS})
add_executable("test_derivator" "test/src/transform/derivator/derivator_test.cpp" ${HEADERS})
add_executable("test_meta"      "test/src/meta/meta_test.cpp"                     ${HEADERS})
add_executable("test_runtime"   "test/src/runtime/runtime_test.cpp"               ${HEADERS})

set(TESTING_TARGETS
    "test_parser"
    "test_meta"
    "test_evaluator"
    "test_derivator"
    "test_runtime"
    )

foreach(t ${TESTING_TARGETS})
    set_target_properties(${t} PROPERTIES DEBUG_POSTFIX "d")
    target_link_libraries(${t} "insituc")
endforeach()

#[[
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
install(TARGETS "insituc" ${TESTING_TARGETS}
    ARCHIVE DESTINATION "lib/"
    RUNTIME DESTINATION "bin/"
    )
]]
