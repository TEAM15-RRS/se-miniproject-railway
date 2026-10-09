cmake_minimum_required(VERSION 3.14)
project(RailwayReservation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RRS_SANITIZE "Build with AddressSanitizer + UBSan" OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(RRS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(rrs_core STATIC ${CORE_SOURCES})
target_include_directories(rrs_core PUBLIC include)

add_executable(rrs src/main.cpp)
target_link_libraries(rrs PRIVATE rrs_core)

enable_testing()

file(GLOB UNIT_TESTS CONFIGURE_DEPENDS tests/unit/*.cpp)
add_executable(unit_tests tests/test_main.cpp ${UNIT_TESTS})
target_link_libraries(unit_tests PRIVATE rrs_core)
target_include_directories(unit_tests PRIVATE tests)

file(GLOB INTEGRATION_TESTS CONFIGURE_DEPENDS tests/integration/*.cpp)
add_executable(integration_tests tests/test_main.cpp ${INTEGRATION_TESTS})
target_link_libraries(integration_tests PRIVATE rrs_core)
target_include_directories(integration_tests PRIVATE tests)

add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME integration_tests COMMAND integration_tests)
