cmake_minimum_required(VERSION 3.16.3)
project(waveshare-fan
        VERSION 0.1
        LANGUAGES C)

set(CMAKE_C_FLAGS_DEBUG "-std=c17 -no-pie -Wall -Werror=return-type -Werror=address -Werror=sequence-point -Werror=format-security -Wextra -pedantic -Wimplicit-fallthrough -Wsequence-point -Wswitch-unreachable -Wswitch-enum -Wstringop-truncation -Wbool-compare -Wtautological-compare -Wfloat-equal -Wshadow=global -Wpointer-arith -Wpointer-compare -Wcast-align -Wcast-qual -Wwrite-strings -Wdangling-else -Wlogical-op -Wconversion -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-no-pie -O3")

include_directories(${CMAKE_SOURCE_DIR}/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/lib LIB_SRC)
aux_source_directory(${CMAKE_SOURCE_DIR} SRC)
add_executable(waveshare-fan ${LIB_SRC} ${SRC})

find_package(ssd1306_i2c REQUIRED)
target_link_libraries(waveshare-fan ${ssd1306_i2c_LIBRARIES} ${WIRINGPI_LIBRARIES})

install(TARGETS waveshare-fan DESTINATION /home/pi/.waveshare-fan)
