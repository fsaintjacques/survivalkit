cmake_minimum_required(VERSION 3.0)
project(survivalkit)


add_definitions(-Wall -Werror -Wswitch -pedantic -Wextra -Wshadow -Wfloat-equal -Wundef
                -Wwrite-strings -Wunreachable-code -Wformat=2 -Winit-self -Wstrict-aliasing)

add_definitions(-ggdb -std=gnu11)

# deps
include(FindPkgConfig)
pkg_check_modules(CK REQUIRED ck)
pkg_check_modules(CMOCKA cmocka)

include_directories(include)
set(SK_SOURCES
    src/sk_healthcheck.c
    src/sk_lifecycle.c)

add_library(sk_static STATIC ${SK_SOURCES})
add_library(sk SHARED ${SK_SOURCES})
set(SK_DEPS sk_static)

if(CMOCKA_FOUND)
    set(SK_TEST_DEPS
        pthread
        ${SK_DEPS}
        ${CMOCKA_LIBRARIES})

    function(sk_test name)
        add_executable(${name}_test test/${name}_test.c)
        target_link_libraries(${name}_test ${SK_TEST_DEPS})
        set_property(TARGET ${name}_test APPEND PROPERTY COMPILE_OPTIONS ${CMOCKA_CFLAGS})

        add_test(${name}_test ${name}_test)
        set_tests_properties(${name}_test PROPERTIES LABELS "test")
    endfunction()

    enable_testing()
    sk_test(sk_healthcheck)
    sk_test(sk_lifecycle)
endif()
