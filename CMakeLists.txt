cmake_minimum_required(VERSION 3.10)
project(archiver C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------ #
#  Compiler warnings                                                   #
# ------------------------------------------------------------------ #
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------------ #
#  Library: all modules except main                                    #
# ------------------------------------------------------------------ #
add_library(huffman_core STATIC
    src/pqueue.c
    src/huffman.c
    src/bitstream.c
    src/compress.c
)
target_include_directories(huffman_core PUBLIC include)

# ------------------------------------------------------------------ #
#  Main executable                                                     #
# ------------------------------------------------------------------ #
add_executable(archiver main.c)
target_link_libraries(archiver PRIVATE huffman_core)

# ------------------------------------------------------------------ #
#  Tests                                                               #
# ------------------------------------------------------------------ #
enable_testing()

foreach(TEST_NAME pqueue huffman bitstream compress)
    add_executable(test_${TEST_NAME} tests/test_${TEST_NAME}.c)
    target_link_libraries(test_${TEST_NAME} PRIVATE huffman_core)
    add_test(NAME ${TEST_NAME} COMMAND test_${TEST_NAME})
endforeach()

# ------------------------------------------------------------------ #
#  AddressSanitizer + UBSan (optional, Debug build only)              #
# ------------------------------------------------------------------ #
option(ENABLE_SANITIZERS "Build with AddressSanitizer and UBSan" OFF)

if(ENABLE_SANITIZERS AND NOT MSVC)
    set(SAN_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${SAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SAN_FLAGS}")
endif()

# ------------------------------------------------------------------ #
#  Install                                                             #
# ------------------------------------------------------------------ #
install(TARGETS archiver RUNTIME DESTINATION bin)
