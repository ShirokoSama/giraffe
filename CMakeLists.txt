cmake_minimum_required(VERSION 3.10)
project(giraffe)

set(CMAKE_CXX_STANDARD 14)

add_executable(giraffe
        ext/concurrentqueue.h ext/blockingconcurrentqueue.h
        http_server.h http_server.cpp
        main.cpp
        system.cpp system.h
        args_tokenizer.h
        conn_producer.cpp conn_producer.h
        conn_consumer.cpp conn_consumer.h)

find_package(Threads)
find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(giraffe ${ZLIB_LIBRARIES})
endif(ZLIB_FOUND)
target_link_libraries(giraffe ${CMAKE_THREAD_LIBS_INIT})